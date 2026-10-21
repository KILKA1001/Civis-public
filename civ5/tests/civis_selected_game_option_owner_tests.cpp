#include "civ5/ui/civis_selected_game_option_owner.hpp"
#include "civ5/runtime/file_identity.hpp"
#include <fstream>
#include <iostream>
#include <limits>
#include <sstream>
#include <thread>
using namespace civ5;
using namespace civ5::ui;
namespace {
void check(bool value,const char *why){if(!value)throw std::runtime_error(why);}
template<class F>void rejects(F call){
    bool rejected=false;try{call();}catch(const runtime::ScriptError &){rejected=true;}
    check(rejected,"Expected explicit selected option rejection");
}
std::string read(const std::filesystem::path &path){
    std::ifstream file(path,std::ios::binary);check(bool(file),"Pinned SDK source unavailable");
    return {std::istreambuf_iterator<char>(file),std::istreambuf_iterator<char>()};
}
void source_contract(const std::filesystem::path &sdk){
    for(const auto &pin:CivisSelectedGameOptionOwner::source_pins())
        check(runtime::identify_file(sdk/pin.sdk_relative_path).sha256==pin.sha256,"Selected SDK option source pin changed");
    const auto enums=read(sdk/CivisSelectedGameOptionOwner::source_pins()[0].sdk_relative_path);
    const auto begin=enums.find("enum GameOptionTypes"),end=enums.find("NUM_GAMEOPTION_TYPES",begin);
    check(begin!=enums.npos&&end!=enums.npos,"Pinned SDK enum declaration missing");
    std::istringstream lines(enums.substr(begin,end-begin));std::string line;std::int64_t ordinal=0,token=-1;
    while(std::getline(lines,line)){
        const auto first=line.find_first_not_of(" \t\r");
        if(first==line.npos||!std::string_view(line).substr(first).starts_with("GAMEOPTION_"))continue;
        const auto comma=line.find(',',first);check(comma!=line.npos,"Pinned SDK option member declaration changed");
        if(line.substr(first,comma-first)=="GAMEOPTION_NO_RELIGION")token=ordinal;
        ++ordinal;
    }
    check(token==21&&ordinal==22,"Selected SDK NO_RELIGION token must derive from enum ordinal21");
    const auto lua_enums=read(sdk/CivisSelectedGameOptionOwner::source_pins()[1].sdk_relative_path);
    check(lua_enums.find("EnumStart(L, \"GameOptionTypes\")")!=lua_enums.npos&&
          lua_enums.find("RegisterEnum(GAMEOPTION_NO_RELIGION)")!=lua_enums.npos,"SDK Lua token registration missing");
    const auto game=read(sdk/CivisSelectedGameOptionOwner::source_pins()[3].sdk_relative_path);
    const auto getter=game.find("bool CvGame::isOption(GameOptionTypes eIndex) const");
    check(getter!=game.npos&&game.find("return (i == 1);",getter)!=game.npos,"SDK IsOption equality contract changed");
    const auto pregame=read(sdk/CivisSelectedGameOptionOwner::source_pins()[4].sdk_relative_path);
    check(pregame.find("bytes = strlen(m_szOptionName);")!=pregame.npos&&
          pregame.find("strncmp(szCurrentOptionName, szOptionName, bytes) == 0")!=pregame.npos&&
          pregame.find("iValue = s_EnumBasedGameOptions[(size_t)eOption];")!=pregame.npos,
          "SDK first-prefix and derived-cache read contracts changed");
}
}
int main(int argc,char **argv){
    check(argc==2,"Expected installed Civ5 SDK root");source_contract(argv[1]);
    CivilopediaLayoutOwnerKey identity;identity.frame_number=31;identity.owner_token="Civis:selected-option-fixture";
    identity.player_id=0;identity.game_turn=123;identity.payload_sha256=std::string(64,'a');identity.rules_sha256=std::string(64,'b');
    saves::PregameObservation raw;raw.layout=saves::GameStateLayout::expansion2_version1;
    content::RulesDatabase rules;rules.apply_sql("CREATE TABLE GameOptions(Type TEXT,\"Default\" INTEGER);INSERT INTO GameOptions VALUES('GAMEOPTION_NO_RELIGION',0);");
    auto context=std::make_shared<CivisCivilopediaCallbackOwner>(identity);
    for(const auto value:{-1LL,0LL,1LL,2LL}){
        raw.game_options={{"UNRELATED",1},{"GAMEOPTION_NO_RELIGION",value},{"GAMEOPTION_NO_RELIGION",1}};
        const auto owner=CivisSelectedGameOptionOwner::open(raw,rules,identity,context);
        const auto proof=owner->resolution();const auto provider=owner->provider();
        check(provider.no_religion_token==21&&provider.is_option(21)==(value==1)&&
              proof.token==21&&proof.value==value&&proof.rules_sha256==identity.rules_sha256&&
              proof.origin==CivisGameOptionResolutionOrigin::saved_prefix&&proof.saved_ordinal==1,
              "First raw option and exact value==1 must override later duplicate and DB default");
        rejects([&]{provider.is_option(0);});rejects([&]{provider.is_option(-1);});
    }
    for(const auto &name:{std::string("GAMEOPTION_NO_RELIG"),std::string{},std::string("GAMEOPTION_NO_RELIGION")+std::string("\0ignored",8)}){
        raw.game_options={{name,1},{"GAMEOPTION_NO_RELIGION",0}};
        auto owner=CivisSelectedGameOptionOwner::open(raw,rules,identity,context);
        check(owner->provider().is_option(21)&&owner->resolution().saved_ordinal==0,
              "SDK length-of-saved-name prefix including empty and first-NUL must match");
    }
    raw.game_options={{"GAMEOPTION_NO_RELIGION_EXTRA",1}};
    auto longer=CivisSelectedGameOptionOwner::open(raw,rules,identity,context);
    check(!longer->provider().is_option(21)&&longer->resolution().origin==CivisGameOptionResolutionOrigin::selected_default&&
          !longer->resolution().saved_ordinal,"Longer saved name must not match the shorter queried key");
    raw.game_options.clear();
    for(const auto value:{0LL,1LL,2LL}){
        rules.apply_sql("UPDATE GameOptions SET \"Default\"="+std::to_string(value));
        auto owner=CivisSelectedGameOptionOwner::open(raw,rules,identity,context);
        check(owner->provider().is_option(21)==(value==1)&&owner->resolution().value==value&&
              owner->resolution().origin==CivisGameOptionResolutionOrigin::selected_default,
              "Selected default must retain exact integer and equality-with-one result");
    }
    rules.apply_sql("UPDATE GameOptions SET \"Default\"=1");
    auto immutable=CivisSelectedGameOptionOwner::open(raw,rules,identity,context);
    auto held=immutable->provider();
    raw.game_options={{"GAMEOPTION_NO_RELIGION",0}};rules.apply_sql("UPDATE GameOptions SET \"Default\"=0");
    check(held.is_option(21)&&immutable->resolution().value==1,"Owned derived cache must not borrow or refresh raw/database inputs");
    raw.game_options.clear();rules.apply_sql("DELETE FROM GameOptions");
    auto missing=CivisSelectedGameOptionOwner::open(raw,rules,identity,context);
    check(!missing->provider().is_option(21)&&missing->resolution().value==0&&
          missing->resolution().origin==CivisGameOptionResolutionOrigin::missing_default_zero,
          "Genuinely missing default must retain source sync's initial zero with explicit origin");
    rules.apply_sql("INSERT INTO GameOptions VALUES('GAMEOPTION_NO_RELIGION',2),('GAMEOPTION_NO_RELIGION',1)");
    auto first_default=CivisSelectedGameOptionOwner::open(raw,rules,identity,context);
    check(first_default->resolution().value==2&&!first_default->provider().is_option(21),"Native LIMIT1 must retain first selected default row");
    for(const auto &options:{std::vector<saves::PregameOption>{{std::string(64,'x'),0}},
                           std::vector<saves::PregameOption>{{"GAMEOPTION_NO_RELIGION",std::numeric_limits<std::int64_t>::max()}}}){
        raw.game_options=options;rejects([&]{CivisSelectedGameOptionOwner::open(raw,rules,identity,context);});
    }
    raw.game_options.clear();rules.apply_sql("DELETE FROM GameOptions;INSERT INTO GameOptions VALUES('GAMEOPTION_NO_RELIGION',2147483648)");
    rejects([&]{CivisSelectedGameOptionOwner::open(raw,rules,identity,context);});
    rules.apply_sql("DROP TABLE GameOptions");
    rejects([&]{CivisSelectedGameOptionOwner::open(raw,rules,identity,context);});
    bool foreign=false;
    std::thread thread([&]{try{held.is_option(21);}catch(const runtime::ScriptError &){foreign=true;}});
    thread.join();check(foreign&&held.is_option(21),"Foreign option read must reject without changing immutable cache");
    foreign=false;
    std::thread close_thread([&]{try{immutable->close();}catch(const runtime::ScriptError &){foreign=true;}});
    close_thread.join();check(foreign&&held.is_option(21),"Foreign close must reject before mutating owner");
    immutable->close();rejects([&]{held.is_option(21);});rejects([&]{immutable->resolution();});rejects([&]{immutable->provider();});
    first_default.reset();const auto expired=missing->provider();missing.reset();rejects([&]{expired.is_option(21);});
    auto changed=identity;++changed.frame_number;context->observe(changed);rejects([&]{longer->provider();});
    rejects([&]{CivisSelectedGameOptionOwner::open(raw,rules,identity,context);});
    auto closed=std::make_shared<CivisCivilopediaCallbackOwner>(identity);closed->close();
    rejects([&]{CivisSelectedGameOptionOwner::open(raw,rules,identity,closed);});
    auto current=std::make_shared<CivisCivilopediaCallbackOwner>(identity);
    raw.game_options={{"GAMEOPTION_NO_RELIGION",1}};
    auto context_held=CivisSelectedGameOptionOwner::open(raw,rules,identity,current);
    auto context_expired=context_held->provider();current.reset();rejects([&]{context_expired.is_option(21);});
    current=std::make_shared<CivisCivilopediaCallbackOwner>(identity);
    rejects([&]{CivisSelectedGameOptionOwner::open(raw,rules,changed,current);});
    raw.layout=saves::GameStateLayout::vp542_corpus;
    rejects([&]{CivisSelectedGameOptionOwner::open(raw,rules,identity,current);});
    std::cout<<"Immutable selected NO_RELIGION21 owner passed: pinned SDK enum/read sources, ordered first-prefix/default/==1 semantics, retained resolution origins and diagnostic lifecycle guards. Actual Gandhi/host binding and native Pregame hook receipt remain pending.\n";
}
