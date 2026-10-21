#include "civ5/ui/civis_selected_game_option_owner.hpp"
#include "civ5/runtime/lua_gameplay_services.hpp"
#include <fstream>
#include <iostream>
#include <thread>
using namespace civ5;using namespace civ5::ui;using namespace civ5::runtime;
namespace {
void check(bool value,const char *why){if(!value)throw std::runtime_error(why);}
template<class F>void rejects(F call){bool rejected=false;try{call();}catch(const ScriptError &){rejected=true;}check(rejected,"Expected guarded selected option rejection");}
std::unique_ptr<LuaScriptHost> vm(const std::filesystem::path &root,const SelectedBnwCivilopediaLayoutSources &sources,
    const CivilopediaLayoutOwnerKey &identity,const std::shared_ptr<CivisCivilopediaCallbackOwner> &owner,
    const ScriptGameplayProvider &fixture_gameplay,bool game=true,bool conflicting_enum=false){
    ScriptUiLayoutProvider layout;layout.context_id="CivilopediaScreen";
    layout.screen_size=[]{return std::pair<std::int64_t,std::int64_t>{1280,960};};
    layout.get_size=[](auto)->std::pair<std::int64_t,std::int64_t>{throw ScriptError("Unrequested fixture geometry");};
    layout.set_size=[](auto,auto,auto){throw ScriptError("Unrequested fixture geometry");};
    layout.set_offset=[](auto,auto,auto){throw ScriptError("Unrequested fixture geometry");};
    layout.calculate_internal_size=[](auto){throw ScriptError("Unrequested fixture geometry");};
    bind_civis_civilopedia_category_callbacks(layout,root,sources,identity,owner,77);
    ScriptGameplayProvider gameplay;
    if(game){
        // Existing database/getter services against a declared synthetic
        // fixture verify identity; they are not selected-game read receipts.
        gameplay=fixture_gameplay;
    }
    ScriptEnumProvider enums;if(conflicting_enum)enums.tables={{"GameOptionTypes",{{"GAMEOPTION_NO_RELIGION",21}}}};
    return std::make_unique<LuaScriptHost>(ScriptBudget{},ScriptSourceProvider{},ScriptModdingProvider{},std::move(gameplay),
        ScriptSaveDataProvider{},ScriptPregameProvider{},ScriptMapProvider{},ScriptUiEventProvider{},ScriptContentProvider{},
        ScriptUserDataProvider{},std::move(enums),ScriptLocaleProvider{},std::move(layout));
}
}
int main(int argc,char **argv){
    check(argc==2,"Expected installed game root");const std::filesystem::path root=argv[1];
    const auto sources=inspect_bnw_civilopedia_layout_sources(root);check(sources.verified,"Installed Civilopedia source unavailable");
    std::ifstream file(root/"Assets/DLC/Expansion2/UI/Civilopedia/CivilopediaScreen.lua");
    const std::string source{std::istreambuf_iterator<char>(file),std::istreambuf_iterator<char>()};
    const std::string predicate="Game == nil or not Game.IsOption(GameOptionTypes.GAMEOPTION_NO_RELIGION)";
    check(source.find(predicate)!=source.npos,"Installed numeric option predicate changed");
    CivilopediaLayoutOwnerKey identity;identity.frame_number=31;identity.owner_token="Civis:selected-option-Lua-fixture";
    identity.player_id=0;identity.game_turn=123;identity.payload_sha256=std::string(64,'a');identity.rules_sha256=std::string(64,'b');
    identity.styles_sha256=sources.styles_sha256;identity.xml_sha256=sources.xml_sha256;identity.civilopedia_sha256=sources.civilopedia_sha256;
    content::RulesDatabase rules;rules.apply_sql("CREATE TABLE GameOptions(Type TEXT,\"Default\" INTEGER);INSERT INTO GameOptions VALUES('GAMEOPTION_NO_RELIGION',0);");
    ResearchContext fixture_setup;fixture_setup.game_turn=123;fixture_setup.speed=2;
    const std::vector<TeamTechnologies> fixture_teams;const auto fixture_types=rules.cache_info_types();
    const auto fixture_services=lua_gameplay_services(rules,fixture_setup,fixture_teams,fixture_types);
    const auto make_vm=[&](const auto &owner,bool game=true,bool conflict=false){return vm(root,sources,identity,owner,fixture_services,game,conflict);};
    saves::PregameObservation raw;raw.layout=saves::GameStateLayout::expansion2_version1;raw.game_options={{"GAMEOPTION_NO_RELIGION",1}};
    auto context=std::make_shared<CivisCivilopediaCallbackOwner>(identity);
    auto option=CivisSelectedGameOptionOwner::open(raw,rules,identity,context);auto provider=option->provider();
    auto host=make_vm(context);
    rejects([&]{host->enable_civis_selected_game_option("Wrong",provider);});
    rejects([&]{host->enable_civis_selected_game_option("CivilopediaScreen",{});});
    auto wrong=provider;wrong.no_religion_token=22;rejects([&]{host->enable_civis_selected_game_option("CivilopediaScreen",wrong);});
    host->enable_civis_selected_game_option("CivilopediaScreen",provider);
    rejects([&]{host->enable_civis_selected_game_option("CivilopediaScreen",provider);});
    host->execute("OtherContext",R"lua(
        assert(Game.IsOption==nil and GameOptionTypes==nil)
        original_game=Game
        getmetatable(_G).__index.__option_original_turn=Game.GetGameTurn
        getmetatable(_G).__index.__option_original_speed=Game.GetGameSpeedType
    )lua","=unrequested-Game-before");
    host->execute("CivilopediaScreen",R"lua(
        assert(Game.GetGameTurn==__option_original_turn and Game.GetGameSpeedType==__option_original_speed)
        assert(Game.GetGameTurn()==123 and Game.GetGameSpeedType()==2)
        local count=0;for name,token in pairs(GameOptionTypes)do
            assert(name=='GAMEOPTION_NO_RELIGION' and token==21);count=count+1
        end;assert(count==1 and Game.IsOption(21)==true)
        getmetatable(_G).__index.__held_selected_option=Game.IsOption
        Controls.CategoryButton1:RegisterCallback(77,function()assert(Game.IsOption(21))end)
    )lua"+std::string("assert(not (")+predicate+"))","@installed-Civilopedia-selected-NO_RELIGION-predicate");
    host->execute("OtherContext","assert(Game==original_game and Game.IsOption==nil and GameOptionTypes==nil)","=unrequested-Game-after");
    host->execute("NewContext","assert(Game.IsOption==nil and GameOptionTypes==nil)","=new-unrequested-Game");
    rejects([&]{host->enable_civis_selected_game_option("CivilopediaScreen",provider);});
    rejects([&]{host->dispatch_ui_control("CivilopediaScreen","CategoryButton1",77);});
    host->complete_loading();const auto callback=host->dispatch_ui_control("CivilopediaScreen","CategoryButton1",77);
    check(callback.listeners==1&&callback.errors.empty(),"Ready declared owner callback must invoke the selected option cache");
    bool foreign=false;
    std::thread thread([&]{try{host->enable_civis_selected_game_option("CivilopediaScreen",provider);}catch(const ScriptError &){foreign=true;}});
    thread.join();check(foreign&&host->loading_complete(),"Foreign option declaration must reject before mutating readiness");
    foreign=false;
    std::thread call([&]{try{host->dispatch_ui_control("CivilopediaScreen","CategoryButton1",77);}catch(const ScriptError &){foreign=true;}});
    call.join();check(foreign&&host->loading_complete(),"Foreign option callback must reject before touching the VM");
    host->close_ui_context("CivilopediaScreen");
    host->execute("OtherContext","assert(not pcall(__held_selected_option,21))","=retained-closed-option-closure");
    rejects([&]{host->complete_loading();});
    for(const auto *bad:{"nil","'21'","21.1","22","0/0","","21,21"}){
        auto current=std::make_shared<CivisCivilopediaCallbackOwner>(identity);
        auto owned=CivisSelectedGameOptionOwner::open(raw,rules,identity,current);
        auto invalid=make_vm(current);invalid->enable_civis_selected_game_option("CivilopediaScreen",owned->provider());
        invalid->execute("CivilopediaScreen","assert(not pcall(Game.IsOption"+(std::string(bad).empty()?std::string{}:","+std::string(bad))+"))\nassert(Game.IsOption(21)==true)",
                         "=invalid-option-argument-stack");
        rejects([&]{invalid->complete_loading();});
    }
    {
        auto current=std::make_shared<CivisCivilopediaCallbackOwner>(identity);
        auto owned=CivisSelectedGameOptionOwner::open(raw,rules,identity,current);
        auto expired=make_vm(current);expired->enable_civis_selected_game_option("CivilopediaScreen",owned->provider());
        owned.reset();expired->execute("CivilopediaScreen","assert(not pcall(Game.IsOption,21))","=expired-option-provider");
        rejects([&]{expired->complete_loading();});
    }
    {
        auto current=std::make_shared<CivisCivilopediaCallbackOwner>(identity);
        auto owned=CivisSelectedGameOptionOwner::open(raw,rules,identity,current);
        auto stale=make_vm(current);stale->enable_civis_selected_game_option("CivilopediaScreen",owned->provider());
        stale->execute("CivilopediaScreen","assert(Game.IsOption(21))","=current-option");
        auto changed=identity;++changed.frame_number;current->observe(changed);
        stale->execute("CivilopediaScreen","assert(not pcall(Game.IsOption,21))","=stale-option");rejects([&]{stale->complete_loading();});
    }
    for(const auto [game,conflict]:{std::pair{false,false},std::pair{true,true}}){
        auto current=std::make_shared<CivisCivilopediaCallbackOwner>(identity);
        auto owned=CivisSelectedGameOptionOwner::open(raw,rules,identity,current);
        auto invalid=make_vm(current,game,conflict);invalid->enable_civis_selected_game_option("CivilopediaScreen",owned->provider());
        rejects([&]{invalid->execute("CivilopediaScreen","error('must not execute')","=option-binding-failure");});
        invalid->execute("OtherContext","assert(1+1==2)","=option-binding-failure-stack");rejects([&]{invalid->complete_loading();});
    }
    {
        auto current=std::make_shared<CivisCivilopediaCallbackOwner>(identity);
        auto old=make_vm(current);
        old->execute("CivilopediaScreen","assert(Game.IsOption==nil and GameOptionTypes==nil)","=old-option-frontier");
        rejects([&]{old->enable_civis_selected_game_option("CivilopediaScreen",provider);});
    }
    std::cout<<"Explicit scoped selected Game option Lua capability passed: copied Game/getter identity, installed numeric predicate, context isolation, exact21 arguments, lifecycle/readiness guards and protected binding failures. Fixture data only; selected Gandhi/native Pregame hook proof remain separate.\n";
}
