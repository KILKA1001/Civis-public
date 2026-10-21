#include "civ5/runtime/bnw_happiness_resources.hpp"
#include "civ5/runtime/file_identity.hpp"
#include "civ5/ui/selected_bnw_preview_host.hpp"
#include <algorithm>
#include <iostream>
#include <stdexcept>
using namespace civ5;using namespace civ5::runtime;
namespace {
void check(bool ok,const char *why) {if(!ok)throw std::runtime_error(why);}
template<class F>void rejects(F &&f,const char *why) {
    try{f();}catch(const std::exception &){return;}throw std::runtime_error(why);
}
auto &field(LiveSessionState &state,std::int64_t player,std::string_view name) {
    for(auto &f:state.world.all_players->players.at(player).core.fields)
        if(f.name && std::string_view(f.name)==name)return f;
    throw std::runtime_error("Missing resource fixture field");
}
void reset_resources(LiveSessionState &state,std::int64_t player) {
    for(const auto name:{"m_paiNumResourceUsed","m_paiNumResourceTotal","m_paiResourceExport",
        "m_paiResourceImport","m_paiResourceFromMinors","m_paiResourcesSiphoned"}) {
        auto &f=field(state,player,name);f.hashes.clear();f.values.clear();f.wire_count=0;
    }
    field(state,player,"m_iMinorResourceBonusCount").values={0};
    field(state,player,"m_iExtraHappinessPerLuxury").values={0};
    for(auto &f:state.world.all_players->players.at(player).traits.fields)
        if(f.name && std::string_view(f.name)=="m_iLuxuryHappinessRetention")f.values={0};
}
void append(LiveSessionState &state,std::int64_t player,const char *name,std::uint32_t hash,std::int32_t value) {
    auto &f=field(state,player,name);f.hashes.push_back(hash);if(hash)f.values.push_back(value);f.wire_count=f.hashes.size();
}
const auto &term(const BnwResourceHappinessQuote &quote,std::int64_t id) {
    const auto found=std::find_if(quote.luxuries.begin(),quote.luxuries.end(),[id](const auto &r){return r.resource==id;});
    check(found!=quote.luxuries.end(),"Missing luxury term");return *found;
}
}
int main(int argc,char **argv) {
    check(argc==3 || argc==4,"Expected selected save, installation and optional SDK source root");
    if(argc==4) {
        const auto root=std::filesystem::path(argv[3])/"CvGameCoreDLL_Expansion2";
        check(identify_file(root/"CvPlayer.cpp").sha256=="9635fe516e590e133c3907ee25c13128fb666e3e7817c967e247affb700a952d","Resource CvPlayer source pin differs");
        check(identify_file(root/"CvVotingClasses.cpp").sha256=="5c49da61bd5fe42f6aab75e94e96525e4472f959dd76fabc11bed8133657c322","Resource league source pin differs");
    }
    auto selected=ui::bootstrap_selected_bnw_preview(argv[1],argv[2],0);
    check(static_cast<bool>(selected.preview),"Selected resource test bootstrap failed");auto &host=*selected.preview.host;
    const auto player=host.active_player_id();const auto &source=host.session().state();
    const auto rules=BnwResourceHappinessRules::load(host.database());
    const auto baseline=quote_bnw_happiness_from_resources(source,rules,host.identity(),player);
    check(baseline.total==0 && !baseline.luxuries.empty(),"Actual selected resource happiness differs");
    check(rules.minor_quantity_multiplier()==200 && rules.minor_happiness_multiplier()==150 && rules.variety_bonus()==0,
          "Selected stock resource defines differ");
    const auto types=host.database().cache_info_types();const auto *gold=types.find("RESOURCE_GOLD"),*silver=types.find("RESOURCE_SILVER");
    check(gold && silver && rules.resources().at(gold->id).happiness==4 && rules.resources().at(silver->id).happiness==4,
          "Stock luxury fixture identity differs");
    auto state=source;reset_resources(state,player);
    append(state,player,"m_paiResourceFromMinors",gold->hash,1);
    append(state,player,"m_paiResourceImport",silver->hash,1);
    field(state,player,"m_iMinorResourceBonusCount").values={1};field(state,player,"m_iExtraHappinessPerLuxury").values={2};
    auto q=quote_bnw_happiness_from_resources(state,rules,host.identity(),player);
    check(q.total==14 && q.positive_resource_types==2 && term(q,gold->id).available==2 && term(q,gold->id).total==8 &&
          term(q,silver->id).total==6,"Minor quantity/happiness or extra luxury source order differs");
    // Native hashed data is an indexed overwrite, not a summed sparse vector.
    append(state,player,"m_paiResourceImport",0,0);append(state,player,"m_paiResourceImport",silver->hash,0);
    q=quote_bnw_happiness_from_resources(state,rules,host.identity(),player);
    check(q.total==8 && q.positive_resource_types==1,"Duplicate hash did not keep the last row or zero hash consumed a value");
    // Export retention is tested even when the used count exhausts an import.
    reset_resources(state,player);append(state,player,"m_paiNumResourceTotal",gold->hash,1);
    append(state,player,"m_paiResourceExport",gold->hash,1);
    for(auto &f:state.world.all_players->players.at(player).traits.fields)
        if(f.name && std::string_view(f.name)=="m_iLuxuryHappinessRetention")f.values={50};
    q=quote_bnw_happiness_from_resources(state,rules,host.identity(),player);
    check(q.total==2 && term(q,gold->id).retained_export && term(q,gold->id).available==0,"Retained exported luxury differs");
    append(state,player,"m_paiResourcesSiphoned",gold->hash,1);
    q=quote_bnw_happiness_from_resources(state,rules,host.identity(),player);
    check(q.total==4 && !term(q,gold->id).retained_export,"Siphoned luxury did not restore actual availability");
    append(state,player,"m_paiNumResourceUsed",gold->hash,1);
    check(quote_bnw_happiness_from_resources(state,rules,host.identity(),player).total==2,"Used resources ignored native subtraction");
    saves::LeagueObservation league;league.members.push_back({});league.members.back().player=player;
    saves::ResolutionObservation ban;ban.effects.flags.push_back({"no_resource_happiness",true});ban.proposer_decision.vote.choice=gold->id;
    league.active_resolutions.push_back(ban);state.world.leagues.leagues.push_back(league);
    q=quote_bnw_happiness_from_resources(state,rules,host.identity(),player);
    check(q.total==0 && term(q,gold->id).banned,"Member luxury ban did not zero base happiness before retention");
    state.world.leagues.leagues.back().members.front().player=(player+1)%63;
    check(quote_bnw_happiness_from_resources(state,rules,host.identity(),player).total==2,"Nonmember incorrectly received league ban");
    // Rule fixtures exercise nonzero variety and source integer truncation.
    auto modified=host.database().clone_gameplay();modified->apply_sql("UPDATE Defines SET Value=3 WHERE Name='HAPPINESS_PER_EXTRA_LUXURY'; UPDATE Defines SET Value=125 WHERE Name='MINOR_POLICY_RESOURCE_HAPPINESS_MULTIPLIER';");
    const auto altered=BnwResourceHappinessRules::load(*modified);auto identity=host.identity();identity.rules_sha256=altered.rules_sha256();
    reset_resources(state,player);state.world.leagues.leagues.clear();
    append(state,player,"m_paiResourceFromMinors",gold->hash,1);append(state,player,"m_paiResourceImport",silver->hash,1);
    field(state,player,"m_iMinorResourceBonusCount").values={1};
    q=quote_bnw_happiness_from_resources(state,altered,identity,player);
    check(q.total==12 && q.variety==3 && term(q,gold->id).after_minor_bonus==5,"Nonzero variety or fractional minor bonus differs");
    rejects([&]{quote_bnw_happiness_from_resources(state,altered,host.identity(),player);},"Different rules accepted");
    auto malformed=state;field(malformed,player,"m_paiResourceImport").values.clear();
    rejects([&]{quote_bnw_happiness_from_resources(malformed,rules,host.identity(),player);},"Missing hash value accepted");
    malformed=state;append(malformed,player,"m_paiResourceImport",0x12345678U,1);
    rejects([&]{quote_bnw_happiness_from_resources(malformed,rules,host.identity(),player);},"Unknown mandatory resource accepted");
    malformed=state;field(malformed,player,"m_paiResourceImport").values.push_back(2);
    rejects([&]{quote_bnw_happiness_from_resources(malformed,rules,host.identity(),player);},"Extra hash value accepted");
    malformed=state;append(malformed,player,"m_paiResourceImport",gold->hash,INT32_MAX);
    rejects([&]{quote_bnw_happiness_from_resources(malformed,rules,host.identity(),player);},"Native availability overflow accepted");
    check(quote_bnw_happiness_from_resources(source,rules,host.identity(),player).total==baseline.total &&
          !host.session().readiness().can_continue(),"Read-only happiness mutated source or readiness");
    std::cout<<"PASS selected BNW resource happiness; imports/exports/siphon/minor bonuses, hash overwrite, league bans, variety/truncation and malformed state\n";
}
