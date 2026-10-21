#include "civ5/runtime/bnw_fresh_pregame_speed.hpp"
#include "civ5/runtime/bnw_fresh_pregame_roster.hpp"
#include "civ5/runtime/lua_script_host.hpp"
#include "civ5/runtime/bnw_selected_locale_owner.hpp"
#include "civ5/runtime/bnw_map_script.hpp"
#include <algorithm>
#include <array>
#include <iostream>
#include <fstream>
#include <thread>
using namespace civ5;
namespace {
void need(bool v,const char* s){if(!v)throw std::runtime_error(s);}
template<class F>void refuses(F f,runtime::BnwFreshPregameSpeedError expected){
    try{f();}catch(const runtime::BnwFreshPregameSpeedRefusal& e){need(e.code()==expected,"Wrong Pregame refusal");return;}
    throw std::runtime_error("Pregame accepted unavailable operation");
}
runtime::BnwCreationChoices choices(const std::string& speed="GAMESPEED_STANDARD"){
    return {"WORLDSIZE_STANDARD","CLIMATE_TEMPERATE","SEALEVEL_MEDIUM",speed,"ERA_ANCIENT","HANDICAP_PRINCE"};
}
std::shared_ptr<runtime::BnwFreshGameContentResult> fixture(const std::string& sql={},const std::string& localization_sql={}){
    // Detached SQL source-order fixture, not a native completed session.
    auto p=std::make_shared<runtime::BnwFreshGameContentResult>();
    p->profile.kind=runtime::ContentProfileKind::dlc;p->profile.can_activate=p->profile.all_requirements_located=true;
    p->profile.enabled_dlc={0};runtime::ContentRequirement r;r.kind=content::PackageKind::dlc;
    r.id="6da07636-4123-4018-b643-6575b4ec336b";r.status=runtime::RequirementStatus::located;r.package=0;
    p->profile.requirements.push_back(r);p->activation.emplace();p->activation->profile_kind=p->profile.kind;
    p->activation->rules_database_ready=true;p->rules=std::make_unique<content::RulesDatabase>();
    p->rules->apply_sql(R"sql(
CREATE TABLE Defines(Name TEXT PRIMARY KEY,Value INTEGER);
INSERT INTO Defines VALUES('STANDARD_HANDICAP',9),('MULTIPLAYER_HANDICAP',10),('STANDARD_GAMESPEED',2),('STANDARD_ERA',0),('AI_HANDICAP',11);
CREATE TABLE PostDefines(Name TEXT,Key TEXT,"Table" TEXT);
CREATE TABLE Worlds(ID INTEGER,Type TEXT);INSERT INTO Worlds VALUES(0,'WORLDSIZE_STANDARD');
CREATE TABLE Climates(ID INTEGER,Type TEXT);INSERT INTO Climates VALUES(0,'CLIMATE_TEMPERATE');
CREATE TABLE SeaLevels(ID INTEGER,Type TEXT);INSERT INTO SeaLevels VALUES(0,'SEALEVEL_MEDIUM');
CREATE TABLE Eras(ID INTEGER,Type TEXT);INSERT INTO Eras VALUES(0,'ERA_ANCIENT');
CREATE TABLE HandicapInfos(ID INTEGER,Type TEXT);INSERT INTO HandicapInfos VALUES(0,'HANDICAP_PRINCE');
CREATE TABLE GameSpeeds(ID INTEGER,Type TEXT,TrainPercent INTEGER);
INSERT INTO GameSpeeds VALUES(0,'GAMESPEED_MARATHON',300),(2,'GAMESPEED_STANDARD',100);
CREATE TABLE TurnTimers(ID INTEGER,Type TEXT,BaseTime INTEGER,CityResource INTEGER,UnitResource INTEGER,FirstTurnMultiplier INTEGER);
INSERT INTO TurnTimers VALUES(1,'TURN_TIMER_SNAIL',8,2,3,4),(3,'TURN_TIMER_MEDIUM',12,3,4,5);
)sql");
    if(!sql.empty())p->rules->apply_sql(sql);
    if(!localization_sql.empty()) {
        content::RulesDatabase text;text.apply_sql(localization_sql);
        p->rules->attach_localization(text);
    }
    p->rules->seal_immutable_preview_fingerprints();return p;
}
void cases(const std::filesystem::path& selected_config_ini,const std::filesystem::path& scratch){
    using P=runtime::BnwFreshPregameSpeed;using E=runtime::BnwFreshPregameSpeedError;using B=runtime::BnwFreshPregameSpeedBoundary;
    refuses([&]{(void)P::from_creation_config({},selected_config_ini);},E::parent_missing);
    auto parent=fixture();auto config=runtime::BnwCreationConfig::from_selected_rules(parent,choices());auto p=P::from_creation_config(config,selected_config_ini);
    const auto initial=p->observe();need(initial.speed==-1 && initial.timer==-1 && initial.timer_numbers.at("ID")==-1 &&
        initial.timer_numbers.at("BaseTime")==0,"Actual static defaults/CvTurnTimerInfo constructor");
    need(initial.game_turn==-1 && initial.admin_password.empty(),"Actual Pregame namespace gameTurn-1/password empty; resetGame not run");
    bool foreign=false;std::thread t([&]{try{p->advance_selected_speed();}catch(const runtime::BnwFreshPregameSpeedRefusal& e){foreign=e.code()==E::wrong_thread;}});
    t.join();need(foreign && p->observe().boundary==B::constructed,"Constructor-thread first before source and stores");
    p->advance_selected_speed();const auto actual=p->observe();
    need(actual.boundary==B::selected_speed_setter_complete && actual.speed==2 && actual.timer==3 &&
         actual.timer_numbers.at("BaseTime")==12 && actual.timer_numbers.at("FirstTurnMultiplier")==5,"Actual standard numeric setter/cache path");
    refuses([&]{p->advance_selected_speed();},E::wrong_phase);
    const auto options0=p->observe().options;
    need(!options0.attempted && !options0.complete && options0.custom.empty() && options0.hash_indices.empty() &&
         std::none_of(options0.multiplayer.begin(),options0.multiplayer.end(),[](bool v){return v;}) &&
         std::all_of(options0.enum_values.begin(),options0.enum_values.end(),[](auto v){return v==0;}) && !options0.quick_combat,
         "Real native constructor empty raw/hash and zero enum22/MP4; no performed SP completion");
    need(!p->observe().init_inputs.attempted && p->observe().init_inputs.era==-1 && p->observe().init_inputs.game_type==-1,
         "Metadata choices do not perform native fresh setters");
    bool input_thread=false;std::thread input_worker([&]{try{p->advance_selected_init_inputs({0,0,runtime::BnwFreshGameType::single_player});}
        catch(const runtime::BnwFreshPregameSpeedRefusal& e){input_thread=e.code()==E::wrong_thread;}});
    input_worker.join();need(input_thread && !p->observe().init_inputs.attempted,"Init setter thread fence BEFORE Config/DB and attempt marker");
    p->advance_selected_init_inputs({0,4294967295ULL,runtime::BnwFreshGameType::network});const auto seeds=p->observe();
    need(seeds.init_inputs.complete && seeds.init_inputs.map_seed==0 && seeds.init_inputs.sync_seed==4294967295u &&
         seeds.init_inputs.era==0 && seeds.init_inputs.game_type==1 && seeds.init_inputs.game_start_type==0 && !seeds.init_inputs.internet_game,
         "Actual uint32 max/zero native inputs and GAME_NEW setter; not copied metadata");
    refuses([&]{p->advance_selected_init_inputs({1,1,runtime::BnwFreshGameType::single_player});},E::wrong_phase);
    for(const bool sync_overflow:{false,true}) {
        auto bad=P::from_creation_config(config,selected_config_ini);bad->advance_selected_speed();
        refuses([&]{bad->advance_selected_init_inputs({sync_overflow?0ULL:4294967296ULL,sync_overflow?4294967296ULL:17ULL,runtime::BnwFreshGameType::single_player});},E::native_source_unavailable);
        const auto x=bad->observe();need(x.init_inputs.attempted && !x.init_inputs.complete && x.init_inputs.era==0 && x.init_inputs.game_type==-1 &&
            x.init_inputs.map_seed==0 && x.init_inputs.sync_seed==(sync_overflow?0u:17u),"Reached seed width refusal preserves ordered setter prefix, not wrapped/zero-success");
        refuses([&]{bad->advance_selected_init_inputs({0,0,runtime::BnwFreshGameType::single_player});},E::wrong_phase);
    }
    for(const auto type:{runtime::BnwFreshGameType::single_player,runtime::BnwFreshGameType::hotseat,runtime::BnwFreshGameType::email}) {
        auto mode=P::from_creation_config(config,selected_config_ini);mode->advance_selected_speed();
        mode->advance_selected_init_inputs({1,2,type});const auto x=mode->observe();
        need(x.init_inputs.game_type==static_cast<std::int32_t>(type) && x.init_inputs.complete && x.order.back()=="s_isInternetGame <- false",
             "Actual non-network setGameType writes internet=false");
    }
    auto invalid=P::from_creation_config(config,selected_config_ini);invalid->advance_selected_speed();
    refuses([&]{invalid->advance_selected_init_inputs({19,23,static_cast<runtime::BnwFreshGameType>(4)});},E::native_source_unavailable);
    need(invalid->observe().init_inputs.map_seed==19 && invalid->observe().init_inputs.sync_seed==23 && !invalid->observe().init_inputs.complete,
         "Unsupported fresh type preserves earlier true seed setters without GAME_NEW completion");
    invalid.reset(); // Do not retain SAME config across the sole-parent test.
    {
        auto origin=fixture();auto retired=P::from_creation_config(runtime::BnwCreationConfig::from_selected_rules(origin,choices()),selected_config_ini);
        retired->advance_selected_speed();origin->profile.enabled_dlc.clear();bool denied=false;
        try{retired->advance_selected_init_inputs({0,0,runtime::BnwFreshGameType::single_player});}
        catch(const runtime::BnwCreationConfigRefusal& e){denied=e.code()==runtime::BnwCreationConfigError::rules_retired;}
        need(denied,"Actual parent retirement refuses fresh setters before DB/world work");
    }
    {
        auto aliases=P::from_creation_config(runtime::BnwCreationConfig::from_selected_rules(
            fixture("DELETE FROM Eras;INSERT INTO Eras VALUES(1,'ERA_ANCIENT'),(4,'era_ancient');"),choices()),selected_config_ini);
        aliases->advance_selected_speed();aliases->advance_selected_init_inputs({0,0,runtime::BnwFreshGameType::single_player});const auto x=aliases->observe();
        const auto first=std::find(x.order.begin(),x.order.end(),"s_era <- 1"),last=std::find(x.order.begin(),x.order.end(),"s_era <- 4");
        need(x.init_inputs.era==4 && first<last && last!=x.order.end(),"Actual sparse NULL-skipping era scan ALL matching native setter order");
    }

    {
        auto marathon=P::from_creation_config(runtime::BnwCreationConfig::from_selected_rules(fixture("DROP TABLE TurnTimers;"),choices("GAMESPEED_MARATHON")),scratch/"never-read-missing-marathon.ini");
        marathon->advance_selected_speed();const auto x=marathon->observe();
        need(x.boundary==B::selected_speed_setter_complete && x.speed==0 && x.timer==-1 && x.timer_numbers.at("ID")==-1,
             "Marathon does not read timer DB or overwrite prior timer/cache");
        auto aliases=P::from_creation_config(runtime::BnwCreationConfig::from_selected_rules(
            fixture("DELETE FROM GameSpeeds;INSERT INTO GameSpeeds VALUES(1,'GAMESPEED_STANDARD',50),(4,'gamespeed_standard',175);"),choices()),selected_config_ini);
        aliases->advance_selected_speed();const auto y=aliases->observe();
        need(y.boundary==B::selected_speed_setter_complete && y.speed==4 && y.timer==1 && y.timer_numbers.at("BaseTime")==8,
             "ALL native alias setters: later default retains earlier actual timer write");
        const auto first=std::find(y.order.begin(),y.order.end(),"s_gameSpeed <- 1");
        const auto timer=std::find(y.order.begin(),y.order.end(),"s_turnTimerType <- 1");
        const auto last=std::find(y.order.begin(),y.order.end(),"s_gameSpeed <- 4");
        need(first<timer && timer<last && last!=y.order.end(),"Native matching setter source order");
        auto text=P::from_creation_config(runtime::BnwCreationConfig::from_selected_rules(
            fixture("ALTER TABLE TurnTimers ADD COLUMN Text TEXT;UPDATE TurnTimers SET Text='TXT_TIMER';"
                    "ALTER TABLE TurnTimers ADD COLUMN Description TEXT;UPDATE TurnTimers SET Description='UNREACHED';"
                    "DELETE FROM GameSpeeds;INSERT INTO GameSpeeds VALUES(1,'GAMESPEED_STANDARD',50),(4,'gamespeed_standard',175);"),choices()),selected_config_ini);
        bool lookup_failed=false;
        try{text->advance_selected_speed();}catch(const runtime::ScriptError&){lookup_failed=true;}
        const auto z=text->observe();
        need(lookup_failed && z.boundary==B::failed_prefix && z.speed==1 && z.timer==1 && z.timer_text.at("Text")=="TXT_TIMER" &&
             z.timer_text.at("DescriptionKey").empty() && z.timer_numbers.at("BaseTime")==0,
             "Actual missing Localization DB error stops before future description/numeric/alias stores");
        refuses([&]{text->advance_selected_speed();},E::wrong_phase);
        auto missing=P::from_creation_config(runtime::BnwCreationConfig::from_selected_rules(fixture("DROP TABLE TurnTimers;"),choices()),selected_config_ini);
        refuses([&]{missing->advance_selected_speed();},E::native_source_unavailable);
        need(missing->observe().speed==2 && missing->observe().timer==3 && missing->observe().boundary==B::failed_prefix,
             "Missing reached timer DB preserves exact earlier native stores");
    }
    {
        // Explicit private test output; production has no user-path default.
        std::filesystem::create_directories(scratch);
        const auto ini=scratch/"pregame-selected-other.ini";
        {std::ofstream f(ini,std::ios::binary|std::ios::trunc);need(static_cast<bool>(f),"Private selected INI open");
            f<<"[User Settings]\nLanguage=en_US\n";need(static_cast<bool>(f),"Private selected INI write");}
        const std::string localized_schema=R"sql(
CREATE TABLE Languages(Type TEXT,Name TEXT);
INSERT INTO Languages VALUES('en_US','English'),('ru_RU','Russian');
CREATE TABLE LanguagePriorities(LanguageSet TEXT,Language TEXT,Priority INTEGER);
INSERT INTO LanguagePriorities VALUES('en_US','en_US',1),('ru_RU','ru_RU',1);
CREATE TABLE LocalizedText(Language TEXT,Tag TEXT,Text TEXT);
INSERT INTO LocalizedText VALUES('en_US','TXT_TIMER','private english'),('ru_RU','TXT_TIMER','private russian');
)sql";
        auto parent2=fixture("ALTER TABLE TurnTimers ADD COLUMN Description TEXT;UPDATE TurnTimers SET Description='TXT_TIMER';",localized_schema);auto config2=runtime::BnwCreationConfig::from_selected_rules(parent2,choices());
        auto other=P::from_creation_config(config2,ini);other->advance_selected_speed();const auto x=other->observe();
        need(x.boundary==B::selected_speed_setter_complete && x.timer_text.at("Description")=="private english" &&
             x.timer_text.at("LocalizedText")=="private english" && x.timer_text.at("Text")=="TXT_TIMER" &&
             x.timer_numbers.at("BaseTime")==12,"Chosen private INI propagates actual language, native fallback before Timer4");
        const auto locale=runtime::BnwSelectedLocaleOwner::from_creation_config(config2,ini);
        bool wrong=false;try{locale->require_same_config(*config);}catch(const runtime::BnwSelectedLocaleRefusal& e){
            wrong=e.code()==runtime::BnwSelectedLocaleError::wrong_config;}
        need(wrong,"Foreign Config cannot satisfy same-owner locale seam");
    }
    {
        auto nullable=P::from_creation_config(runtime::BnwCreationConfig::from_selected_rules(
            fixture("DELETE FROM GameSpeeds;INSERT INTO GameSpeeds VALUES(1,'GAMESPEED_STANDARD',50),(2,'gamespeed_standard',100);"
                    "UPDATE TurnTimers SET CityResource=NULL WHERE ID=3;"),choices()),selected_config_ini);
        refuses([&]{nullable->advance_selected_speed();},E::native_source_unavailable);
        const auto x=nullable->observe();
        need(x.boundary==B::failed_prefix && x.speed==2 && x.timer==3 &&
             x.timer_numbers.at("BaseTime")==12 && x.timer_numbers.at("CityResource")==2 &&
             x.timer_numbers.at("UnitResource")==3 && x.timer_numbers.at("FirstTurnMultiplier")==4,
             "Reached NULL refuses before mutation, preserving prior alias values and current numeric prefix");
        refuses([&]{nullable->advance_selected_speed();},E::wrong_phase);
        auto floating=P::from_creation_config(runtime::BnwCreationConfig::from_selected_rules(
            fixture("UPDATE TurnTimers SET BaseTime=1.5 WHERE ID=3;"),choices()),selected_config_ini);
        refuses([&]{floating->advance_selected_speed();},E::native_source_unavailable);
        need(floating->observe().timer_numbers.at("BaseTime")==0,
             "Closed noninteger GetValue mutation is unavailable before numeric store");
    }
    {
        auto duplicate=P::from_creation_config(runtime::BnwCreationConfig::from_selected_rules(
            fixture("INSERT INTO TurnTimers VALUES(3,'TURN_TIMER_DUPLICATE',99,99,99,99);"),choices()),selected_config_ini);
        refuses([&]{duplicate->advance_selected_speed();},E::native_source_unavailable);
        const auto x=duplicate->observe();
        need(x.boundary==B::failed_prefix && x.speed==2 && x.timer==3 &&
             x.timer_numbers.at("ID")==3 && x.timer_numbers.at("BaseTime")==0,
             "Ambiguous selected Timer row cannot lend another row's numeric storage proof");
    }
    std::weak_ptr<const runtime::BnwCreationConfig> wc=config;
    std::weak_ptr<runtime::BnwFreshGameContentResult> wp=parent;
    config.reset();parent.reset();need(!wc.expired() && !wp.expired() && p->observe().speed==2,"Sole genuine owner retains parents");
    p.reset();need(wc.expired() && wp.expired(),"Pregame parent graph is acyclic");
    auto retired=fixture();auto bad=P::from_creation_config(runtime::BnwCreationConfig::from_selected_rules(retired,choices()),selected_config_ini);
    retired->profile.enabled_dlc.clear();
    try{bad->advance_selected_speed();}catch(const runtime::BnwCreationConfigRefusal& e){
        need(e.code()==runtime::BnwCreationConfigError::rules_retired,"Actual profile retirement refusal");return;
    }
    throw std::runtime_error("Retired selected profile accepted Pregame setter");
}
void reset_cases(const std::filesystem::path& selected_config_ini) {
    using P=runtime::BnwFreshPregameSpeed;using E=runtime::BnwFreshPregameSpeedError;
    using B=runtime::BnwFreshPregameResetBoundary;
    const std::string schema="DELETE FROM Defines;INSERT INTO Defines VALUES('STANDARD_GAMESPEED',2),('STANDARD_ERA',0);"
        "CREATE TABLE Victories(ID INTEGER,Type TEXT);INSERT INTO Victories VALUES(0,'VICTORY_A'),(4,'VICTORY_B');";
    auto origin=fixture(schema);auto config=runtime::BnwCreationConfig::from_selected_rules(origin,choices());
    auto reset=P::from_creation_config_for_reset_setup(config,selected_config_ini);
    need(reset->observe().reset.boundary==B::reset_game_required && reset->observe().game_turn==-1,"No reset performed by metadata/factory");
    refuses([&]{reset->advance_selected_speed();},E::wrong_phase);
    bool foreign=false;std::thread worker([&]{try{reset->advance_reset_game();}catch(const runtime::BnwFreshPregameSpeedRefusal& e){foreign=e.code()==E::wrong_thread;}});
    worker.join();need(foreign && reset->observe().reset.boundary==B::reset_game_required,"Reset thread before source/stores/attempt");
    reset->advance_reset_game();const auto x=reset->observe();
    need(x.reset.boundary==B::reset_game_settings_assignment_required && x.game_turn==0 && x.speed==2 && x.timer==4 && x.init_inputs.era==0,
         "Actual native reset writes0/standard cache/timer4; no selected settings grant");
    need(x.timer_numbers.at("ID")==-1 && x.timer_numbers.at("BaseTime")==0,"resetGame direct timer4 assignment never executes setTurnTimer/CacheResults");
    need(x.reset.victory_count==5 && x.reset.victories.size()==5 && std::all_of(x.reset.victories.begin(),x.reset.victories.end(),[](bool v){return v;}),
         "Native Victory vector includes NULL holes alltrue, not count/maxrowid");
    need(x.reset.integer_stores.at("s_bandwidth")==2 && x.reset.integer_stores.at("s_numMinorCivs")==-1 &&
         x.reset.integer_stores.at("s_quickHandicap")==-1 && x.reset.integer_stores.at("s_activePlayer")==-1 &&
         x.reset.float_stores.at("s_autorunTurnDelay")==0.0f && x.reset.string_stores.at("s_mapScriptName").empty(),"Exact native enums/float/strings");
    const auto at=[&](const char* s){const auto i=std::find(x.order.begin(),x.order.end(),s);need(i!=x.order.end(),"Native reset journal missing");return i;};
    need(at("resetGame s_era")<at("resetGame s_gameSpeed") && at("resetGame s_numVictoryInfos")<at("resetGame s_gameTurn") &&
         at("resetGame s_gameTurn")<at("resetGame ResetMapOptions s_MapOptions.clear") &&
         at("resetGame ResetGameOptions s_GameOptionsHash.clear")<at("s_EnumBasedGameOptions[0]") &&
         at("s_EnumBasedGameOptions[21]")<at("resetGame ResetGameOptions s_numVictoryInfos"),"Native reset scalar/victory/option order");
    need(!x.options.complete && !x.init_inputs.complete,"Reset doesn't authorize SP init/setter completion");
    refuses([&]{reset->advance_reset_game();},E::wrong_phase);
    reset->advance_selected_speed();need(reset->observe().timer==3 && reset->observe().game_turn==0,"Selected settings AFTER native reset, same storage");
    reset->advance_selected_init_inputs({1,2,runtime::BnwFreshGameType::single_player});
    need(reset->observe().init_inputs.complete && reset->observe().game_turn==0,"Actual selected seeds/era after reset retain0");
    refuses([&]{reset->advance_reset_game();},E::wrong_phase);
    auto constructor=P::from_creation_config(config,selected_config_ini);
    refuses([&]{constructor->advance_reset_game();},E::wrong_phase);
    need(constructor->observe().game_turn==-1,"Unissued reset doesn't rewrite constructor diagnostic");
    {
        auto empty=P::from_creation_config_for_reset_setup(runtime::BnwCreationConfig::from_selected_rules(
            fixture("DELETE FROM Defines;CREATE TABLE Victories(ID INTEGER,Type TEXT);"),choices()),selected_config_ini);
        empty->advance_reset_game();const auto e=empty->observe();
        need(e.reset.cached_standard_speed==0 && e.reset.cached_standard_era==0 && e.speed==0 && e.reset.victory_count==0 && e.reset.victories.empty(),
             "Missing Define row cache0 and native empty Victory span0, not MaxRows empty1");
    }
    {
        auto dup=P::from_creation_config_for_reset_setup(runtime::BnwCreationConfig::from_selected_rules(
            fixture(schema+"INSERT INTO Victories VALUES(4,'VICTORY_DUP');"),choices()),selected_config_ini);
        dup->advance_reset_game();need(dup->observe().reset.victory_count==6,"Native duplicate ID append expands span instead of collapsing by ID");
    }
    {
        auto partial=P::from_creation_config_for_reset_setup(runtime::BnwCreationConfig::from_selected_rules(
            fixture(schema+"CREATE TABLE GameOptions(Type TEXT,\"Default\" INTEGER);INSERT INTO GameOptions VALUES('GAMEOPTION_NO_CITY_RAZING',2147483648);"),choices()),selected_config_ini);
        refuses([&]{partial->advance_reset_game();},E::native_source_unavailable);const auto z=partial->observe();
        need(z.reset.boundary==B::failed_prefix && z.game_turn==0 && z.timer==4 && z.reset.victories.size()==5 &&
             z.reset.string_stores.at("s_alias").empty(),"Later Sync overflow retains reached reset scalars/victories and no second completion");
        need(std::find(z.order.begin(),z.order.end(),"resetGame ResetGameOptions s_numVictoryInfos")==z.order.end(),"Failed Sync cannot execute later victory reset");
        refuses([&]{partial->advance_reset_game();},E::wrong_phase);
        refuses([&]{partial->advance_selected_speed();},E::wrong_phase);
    }
    {
        auto source=fixture(schema);auto held=P::from_creation_config_for_reset_setup(runtime::BnwCreationConfig::from_selected_rules(source,choices()),selected_config_ini);
        source->profile.enabled_dlc.clear();bool stopped=false;
        try{held->advance_reset_game();}catch(const runtime::BnwCreationConfigRefusal& e){stopped=e.code()==runtime::BnwCreationConfigError::rules_retired;}
        need(stopped,"Retired actual source denies reset before first store");
    }
    {
        auto parent=fixture(schema);auto c=runtime::BnwCreationConfig::from_selected_rules(parent,choices());
        std::weak_ptr<const runtime::BnwCreationConfig> wc=c;std::weak_ptr<runtime::BnwFreshGameContentResult> wp=parent;
        auto sole=P::from_creation_config_for_reset_setup(c,selected_config_ini);c.reset();parent.reset();sole->advance_reset_game();
        need(!wc.expired() && !wp.expired(),"Reset SAME strong parent graph survives external aliases");
        sole.reset();need(wc.expired() && wp.expired(),"Reset parent graph no cycle");
    }
}
void menu_prefix_check(const std::shared_ptr<runtime::BnwFreshPregameSpeed>& p,
                       std::int32_t expected_handicap=9) {
    using E=runtime::BnwFreshPregameSpeedError;
    using M=runtime::BnwFreshPregameMenuBoundary;using RB=runtime::BnwFreshPregameRosterBoundary;
    using W=runtime::BnwFreshPregameRosterWrite;
    const auto sibling=p->roster();const auto before=sibling->observe();
    need(before.boundary==RB::namespace_constructor && !before.reset_attempted && before.writes.empty() &&
         before.world_size==-1 && before.world_info_id==-1 && before.world_default_players==0,
         "Actual namespace world/cache IDs agree; Config World has not executed setter");
    need(p->observe().cached_globals.boundary==runtime::BnwFreshPregameCachedGlobalsBoundary::constructor_diagnostic,
         "Ctor cached1/3/8 are diagnostics only, no issued DB cache before actual first attempt");
    for(const auto& slot:before.slots)need(slot.status==0 && slot.claim==0 && slot.team==0 && slot.handicap==0 &&
        !slot.display_nickname,"Actual constructor vectors0 with display map absent");
    try{p->advance_single_player_menu_prefix();throw std::runtime_error("Menu skipped required settings continuation");}
    catch(const runtime::BnwFreshPregameSpeedRefusal& e){
        need(e.code()==E::native_source_unavailable && std::string(e.what()).find("ResetGameOptions")!=std::string::npos,
             "Only actual next menu ResetGameOptions frontier admits this source branch");
    }
    const auto x=p->observe();const auto z=sibling->observe();
    need(x.menu.boundary==M::reset_game_options_required && !x.menu.private_game && x.init_inputs.game_type==0 &&
         !x.init_inputs.internet_game && !x.init_inputs.attempted && !x.init_inputs.complete,
         "Performed SP/ResetSlots stores, no full menu/fresh Init grant");
    need(x.cached_globals.boundary==runtime::BnwFreshPregameCachedGlobalsBoundary::selected_database_subset_stored &&
         x.cached_globals.standard_handicap==expected_handicap,"Real retained cached Define, not Config Handicap ID");
    const auto at=[&](const char* name){const auto i=std::find(x.order.begin(),x.order.end(),name);need(i!=x.order.end(),"Missing native cache/menu order");return i;};
    need(at("PostDefines actual owned SQL clone source")<at("cacheGlobals STANDARD_HANDICAP") &&
         at("cacheGlobals STANDARD_HANDICAP")<at("cacheGlobals MULTIPLAYER_HANDICAP") &&
         at("cacheGlobals MULTIPLAYER_HANDICAP")<at("cacheGlobals STANDARD_GAMESPEED") &&
         at("cacheGlobals STANDARD_GAMESPEED")<at("cacheGlobals STANDARD_ERA") &&
         at("cacheGlobals STANDARD_ERA")<at("cacheGlobals AI_HANDICAP") &&
         at("cacheGlobals AI_HANDICAP")<at("SinglePlayer SetPrivateGame false"),"All5 same child DB caches BEFORE menu in relative SDK order");
    need(z.reset_attempted && z.boundary==RB::constructor_single_player_slots_reset && z.writes.size()==63*38+22,
         "All63 native slots plus22 observer overrides, no barbarian call");
    const std::vector<W> prefix{W::leader_name,W::civilization_description,W::civilization_short_description,
        W::civilization_adjective,W::civilization_password,W::email,W::white_flag,W::civilization,W::civilization_key,
        W::civilization_package,W::civilization_available,W::civilization_playable,W::civilization,W::civilization_key,
        W::civilization_package,W::civilization_available,W::civilization_playable,W::leader,W::leader_key,
        W::leader_package,W::leader_available,W::leader,W::leader_available,W::minor_civ,W::team};
    const std::vector<W> suffix{W::handicap,W::last_human_handicap,W::player_color,W::art_style,W::status,W::claim,
        W::playable,W::minor,W::net_id,W::ready,W::display_nickname,W::nickname,W::available_dlc};
    std::size_t pos=0;
    for(std::size_t i=0;i<63;++i) {
        for(const auto w:prefix)need(z.writes[pos++]==w,"Exact source resetPlayer25 order each native slot");
        for(const auto w:suffix)need(z.writes[pos++]==w,"Exact setter effects/suffix order each native slot");
        if(i<22)need(z.writes[pos++]==W::status,"Observer override only AFTER resetPlayer return");
        const auto& slot=z.slots[i];
        need(slot.civilization==-1 && slot.leader==-1 && slot.minor_civ==-1 && slot.team==static_cast<std::int32_t>(i) &&
             slot.civilization_available && slot.civilization_playable && slot.leader_available && slot.handicap==expected_handicap &&
             slot.last_human_handicap==-1 && slot.player_color==-1 && slot.art_style==-1 && slot.status==(i<22?4:2) &&
             slot.claim==0 && slot.net_id==-1 && !slot.playable && !slot.minor && !slot.ready &&
             slot.display_nickname && slot.display_nickname->empty() && slot.nickname.empty() && slot.available_dlc.empty(),
             "All native reset setters and empty display map insertion; slots0..21 OBSERVER,22..62 CLOSED");
    }
    need(pos==z.writes.size() && z.slots[63]==before.slots[63],"Native loop retains barbarian63 untouched");
    refuses([&]{p->advance_single_player_menu_prefix();},E::wrong_phase);
    refuses([&]{p->advance_reset_game();},E::wrong_phase);
    refuses([&]{p->advance_selected_speed();},E::wrong_phase);
    refuses([&]{p->advance_selected_init_inputs({0,0,runtime::BnwFreshGameType::single_player});},E::wrong_phase);
    need(sibling->observe().writes==z.writes && p->observe().order==x.order,"No replay or bypass settings after reached frontier");
}

void cache_cases(const std::filesystem::path& selected_config_ini) {
    using P=runtime::BnwFreshPregameSpeed;using E=runtime::BnwFreshPregameSpeedError;
    using C=runtime::BnwFreshPregameCachedGlobalsBoundary;using M=runtime::BnwFreshPregameMenuBoundary;
    {
        // Native ROWID, NOT authored ID or selected Config.handicap ID0.
        const auto source=fixture("INSERT INTO HandicapInfos(rowid,ID,Type) VALUES(71,5,'HANDICAP_CACHE_A'),(83,6,'HANDICAP_CACHE_B');"
            "INSERT INTO PostDefines VALUES('STANDARD_HANDICAP','HANDICAP_CACHE_A','HandicapInfos'),"
            "('STANDARD_HANDICAP','HANDICAP_CACHE_B','HandicapInfos');");
        auto config=runtime::BnwCreationConfig::from_selected_rules(source,choices());
        const auto before=source->rules->gameplay_fingerprint_sha256();
        const auto p=P::from_creation_config(config,selected_config_ini);menu_prefix_check(p,83);
        need(config->metadata().info[5].id==0 && source->rules->gameplay_fingerprint_sha256()==before &&
             std::get<std::int64_t>(source->rules->global_define("STANDARD_HANDICAP",content::GlobalDefineKind::integer).value)==9,
             "PostDefines last native write uses ROWID83 in owned clone; original Define9/Config ID0 unchanged");
    }
    {
        const auto p=P::from_creation_config(runtime::BnwCreationConfig::from_selected_rules(
            fixture("DELETE FROM Defines WHERE Name='STANDARD_HANDICAP';"),choices()),selected_config_ini);
        menu_prefix_check(p,0); // Native failed Step leaves getDefineINT initial0.
    }
    {
        const auto p=P::from_creation_config(runtime::BnwCreationConfig::from_selected_rules(
            fixture("UPDATE Defines SET Value=NULL WHERE Name='STANDARD_HANDICAP';"),choices()),selected_config_ini);
        menu_prefix_check(p,0); // Native GetInt(NULL)0, not ctor1/Config ID.
    }
    {
        const auto p=P::from_creation_config(runtime::BnwCreationConfig::from_selected_rules(
            fixture("UPDATE Defines SET Value=2147483648 WHERE Name='AI_HANDICAP';"),choices()),selected_config_ini);
        const auto sibling=p->roster();
        refuses([&]{p->advance_single_player_menu_prefix();},E::native_source_unavailable);
        const auto q=p->observe();
        need(q.cached_globals.boundary==C::failed_prefix && q.cached_globals.standard_handicap==9 &&
             q.cached_globals.multiplayer_handicap==10 && q.cached_globals.standard_speed==2 &&
             q.cached_globals.standard_era==0 && q.cached_globals.ai_handicap==8 && q.init_inputs.game_type==-1 &&
             sibling->observe().writes.empty() && q.menu.boundary==M::failed_prefix,
             "Later AI width refusal retains first4 real cache stores, no earlier menu/roster effects or substituted AI default grant");
        refuses([&]{p->advance_single_player_menu_prefix();},E::wrong_phase);
        refuses([&]{p->advance_selected_speed();},E::wrong_phase);
    }
    for(const auto sql:{"DROP TABLE Defines;","DROP TABLE PostDefines;",
                        "INSERT INTO PostDefines VALUES('STANDARD_HANDICAP','x','MissingHandicaps');"}) {
        const auto p=P::from_creation_config(runtime::BnwCreationConfig::from_selected_rules(fixture(sql),choices()),selected_config_ini);
        const auto sibling=p->roster();
        refuses([&]{p->advance_single_player_menu_prefix();},E::native_source_unavailable);
        const auto q=p->observe();
        need(q.cached_globals.boundary==C::failed_prefix && q.init_inputs.game_type==-1 && sibling->observe().writes.empty(),
             "SQL/producer errors do not become missing-row0 or execute menu/slot writes");
        refuses([&]{p->advance_single_player_menu_prefix();},E::wrong_phase);
    }
}
void menu_lifecycle_cases(const std::filesystem::path& selected_config_ini) {
    using P=runtime::BnwFreshPregameSpeed;using E=runtime::BnwFreshPregameSpeedError;
    using M=runtime::BnwFreshPregameMenuBoundary;
    {
        auto parent=fixture();auto c=runtime::BnwCreationConfig::from_selected_rules(parent,choices());
        auto p=P::from_creation_config(c,selected_config_ini);menu_prefix_check(p);
    }
    {
        auto parent=fixture();auto c=runtime::BnwCreationConfig::from_selected_rules(parent,choices());
        auto p=P::from_creation_config(c,selected_config_ini);auto old_sibling=p->roster();
        auto replacement=P::from_creation_config(c,selected_config_ini);
        *p=*replacement;
        refuses([&]{old_sibling->require_current();},E::retired);
        refuses([&]{(void)p->roster();},E::retired);
        // old_sibling keeps its old Impl alive even though p's allocation changed.
        old_sibling.reset();
    }
    {
        auto parent=fixture();auto c=runtime::BnwCreationConfig::from_selected_rules(parent,choices());
        auto p=P::from_creation_config(c,selected_config_ini);auto sibling=p->roster();
        sibling->require_current();
        need(p->observe().menu.boundary==M::menu_request_required && !p->observe().menu.private_game &&
             p->observe().init_inputs.game_type==-1,"Factory issues actual sibling without executing menu/SP setters");
        bool refused=false;
        std::thread worker([&]{try{p->advance_single_player_menu_prefix();}
            catch(const runtime::BnwFreshPregameSpeedRefusal& e){refused=e.code()==E::wrong_thread;}});
        worker.join();need(refused && p->observe().menu.boundary==M::menu_request_required,
            "Menu issuer thread first before source, once marker or slot stores");
        auto copied=*p;
        refuses([&]{(void)copied.roster();},E::retired);
        refuses([&]{copied.advance_single_player_menu_prefix();},E::retired);
        need(p->observe().menu.boundary==M::menu_request_required,"Copied facade cannot borrow original sibling issuer");
        p->advance_selected_speed();
        refuses([&]{p->advance_single_player_menu_prefix();},E::wrong_phase);
        need(p->observe().menu.boundary==M::menu_request_required,"Late menu cannot overwrite already selected settings");
    }
    {
        auto parent=fixture();auto c=runtime::BnwCreationConfig::from_selected_rules(parent,choices());
        auto p=P::from_creation_config(c,selected_config_ini);auto sibling=p->roster();
        std::weak_ptr<P> weak_pg=p;std::weak_ptr<const runtime::BnwCreationConfig> weak_c=c;
        std::weak_ptr<runtime::BnwFreshGameContentResult> weak_parent=parent;
        p.reset();c.reset();parent.reset();
        need(!weak_pg.expired() && !weak_c.expired() && !weak_parent.expired(),"Sibling alias retains original actual Pregame and parents");
        sibling->require_current();sibling.reset();
        need(weak_pg.expired() && weak_c.expired() && weak_parent.expired(),"Pregame roster/config alias graph is acyclic");
    }
    {
        auto parent=fixture();auto c=runtime::BnwCreationConfig::from_selected_rules(parent,choices());
        auto p=P::from_creation_config(c,selected_config_ini);auto sibling=p->roster();
        parent->profile.enabled_dlc.clear();
        try{p->advance_single_player_menu_prefix();throw std::runtime_error("Retired menu source accepted");}
        catch(const runtime::BnwCreationConfigRefusal& e){need(e.code()==runtime::BnwCreationConfigError::rules_retired,"Actual source retirement propagates before menu stores");}
        try{sibling->require_current();throw std::runtime_error("Retired sibling source accepted");}
        catch(const runtime::BnwCreationConfigRefusal& e){need(e.code()==runtime::BnwCreationConfigError::rules_retired,"Retained sibling still checks actual selected source");}
    }
}
void menu_options_cases(const std::filesystem::path& selected_config_ini) {
    using P=runtime::BnwFreshPregameSpeed;using E=runtime::BnwFreshPregameSpeedError;
    using M=runtime::BnwFreshPregameMenuBoundary;
    const std::string victories="CREATE TABLE Victories(ID INTEGER,Type TEXT);INSERT INTO Victories VALUES(0,'VICTORY_A'),(4,'VICTORY_B');";
    const std::string options="CREATE TABLE GameOptions(Type TEXT,\"Default\" INTEGER);"
        "INSERT INTO GameOptions VALUES('GAMEOPTION_NO_CITY_RAZING',7),('GAMEOPTION_NO_CITY_RAZING',19),"
        "('GAMEOPTION_NO_BARBARIANS',NULL),('GAMEOPTION_QUICK_COMBAT',-2),('GAMEOPTION_DISABLE_START_BIAS',2147483648);";
    const auto advance=[&](const std::shared_ptr<P>& p) {
        try{p->advance_single_player_menu_options_prefix();throw std::runtime_error("Menu skipped LoadPreGameSettings");}
        catch(const runtime::BnwFreshPregameSpeedRefusal& e){need(e.code()==E::native_source_unavailable &&
            std::string(e.what()).find("LoadPreGameSettings")!=std::string::npos,"Only exact reached LoadPreGameSettings frontier");}
    };
    {
        auto source=fixture(victories+options);auto c=runtime::BnwCreationConfig::from_selected_rules(source,choices());
        auto p=P::from_creation_config(c,selected_config_ini);
        refuses([&]{p->advance_single_player_menu_options_prefix();},E::wrong_phase);
        menu_prefix_check(p);const auto before=p->observe();
        bool denied=false;std::thread worker([&]{try{p->advance_single_player_menu_options_prefix();}
            catch(const runtime::BnwFreshPregameSpeedRefusal& e){denied=e.code()==E::wrong_thread;}});
        worker.join();need(denied && !p->observe().menu.options_reset_attempted,"Menu continuation thread guard before borrow/SQL/marker");
        auto copied=*p;refuses([&]{copied.advance_single_player_menu_options_prefix();},E::retired);
        advance(p);const auto q=p->observe();
        need(q.menu.boundary==M::load_pregame_settings_required && q.menu.options_reset_attempted && q.menu.map_options.empty() &&
            q.options.custom.empty() && q.options.hash_indices.empty() && !q.options.attempted && !q.options.complete,
            "Actual empty hash/vector reset, mapclear; separate Game SP completion not forged");
        need(q.options.enum_values[0]==7 && q.options.enum_values[1]==0 && q.options.enum_values[15]==-2 &&
            q.options.enum_values[16]==0 && q.options.quick_combat,"First SQL LIMIT1/defaultNULL0/nonzeroQuickCombat; native NULL-name16 not queried");
        need(q.reset.victory_count==5 && q.reset.victories.size()==5 &&
            std::all_of(q.reset.victories.begin(),q.reset.victories.end(),[](bool v){return v;}),"Native5 Info slots includes3 holes, all victory bits true");
        const auto at=[&](const char* name){const auto i=std::find(q.order.begin()+static_cast<std::ptrdiff_t>(before.order.size()),q.order.end(),name);
            need(i!=q.order.end(),"Missing native menu options order");return i;};
        need(at("SinglePlayer ResetGameOptions s_GameOptions.clear")<at("SinglePlayer ResetGameOptions s_GameOptionsHash.clear") &&
            at("SinglePlayer ResetGameOptions s_GameOptionsHash.clear")<at("s_EnumBasedGameOptions[0]") &&
            at("s_EnumBasedGameOptions[15]")<at("s_quickCombat <- enum15 nonzero") &&
            at("s_EnumBasedGameOptions[21]")<at("SinglePlayer ResetGameOptions s_numVictoryInfos") &&
            at("SinglePlayer ResetGameOptions s_numVictoryInfos")<at("SinglePlayer ResetGameOptions s_victories.clear") &&
            at("SinglePlayer ResetGameOptions s_victories.push_back true[4]")<at("SinglePlayer ResetMapOptions s_MapOptions.clear"),
            "Stock menu GameOptions->MapOptions source order (different from resetGame)");
        need(q.init_inputs.map_seed==before.init_inputs.map_seed && q.init_inputs.sync_seed==before.init_inputs.sync_seed &&
            p->roster()->observe().writes.size()==63*38+22,"No RNG/slot replay from options reset");
        refuses([&]{p->advance_single_player_menu_options_prefix();},E::wrong_phase);
        refuses([&]{p->advance_selected_speed();},E::wrong_phase);
        need(p->observe().order==q.order,"No settings bypass after required LoadPreGameSettings");
    }
    {
        auto p=P::from_creation_config(runtime::BnwCreationConfig::from_selected_rules(
            fixture(victories+options+"UPDATE GameOptions SET \"Default\"=2147483648 WHERE Type='GAMEOPTION_NO_BARBARIANS';"),choices()),selected_config_ini);
        menu_prefix_check(p);refuses([&]{p->advance_single_player_menu_options_prefix();},E::native_source_unavailable);
        const auto q=p->observe();
        need(q.menu.boundary==M::failed_prefix && q.menu.options_reset_attempted && q.options.enum_values[0]==7 &&
            q.options.enum_values[1]==0 && !q.options.quick_combat && q.reset.victory_count==0 && q.reset.victories.empty(),
            "Reached query callback width failure retains enum0, stops before enum1/victory producer/mapclear");
        need(std::find(q.order.begin(),q.order.end(),"SinglePlayer ResetMapOptions s_MapOptions.clear")==q.order.end(),"No future mapclear after callback exception");
        refuses([&]{p->advance_single_player_menu_options_prefix();},E::wrong_phase);
    }
    {
        auto p=P::from_creation_config(runtime::BnwCreationConfig::from_selected_rules(fixture(options),choices()),selected_config_ini);
        menu_prefix_check(p);refuses([&]{p->advance_single_player_menu_options_prefix();},E::native_source_unavailable);
        const auto q=p->observe();
        need(q.menu.boundary==M::failed_prefix && q.options.enum_values[15]==-2 && q.options.quick_combat &&
            std::find(q.order.begin(),q.order.end(),"s_EnumBasedGameOptions[21]")!=q.order.end() &&
            std::find(q.order.begin(),q.order.end(),"SinglePlayer ResetGameOptions s_numVictoryInfos")==q.order.end(),
            "Missing real Victory producer refuses after all22 Sync stores, not an invented zero span");
        refuses([&]{p->advance_single_player_menu_options_prefix();},E::wrong_phase);
    }
    {
        auto source=fixture("CREATE TABLE Victories(ID INTEGER,Type TEXT);");auto c=runtime::BnwCreationConfig::from_selected_rules(source,choices());
        auto p=P::from_creation_config(c,selected_config_ini);menu_prefix_check(p);advance(p);const auto q=p->observe();
        need(q.reset.victory_count==0 && q.reset.victories.empty() && !q.options.quick_combat &&
            std::all_of(q.options.enum_values.begin(),q.options.enum_values.end(),[](auto v){return v==0;}),
            "Native empty Victory collection0 and missing GameOptions Executefalse => each local initial0, no fake producer receipt");
    }
    {
        auto source=fixture(victories+options);auto c=runtime::BnwCreationConfig::from_selected_rules(source,choices());
        auto p=P::from_creation_config(c,selected_config_ini);menu_prefix_check(p);
        source->profile.enabled_dlc.clear();
        try{p->advance_single_player_menu_options_prefix();throw std::runtime_error("Retired options source accepted");}
        catch(const runtime::BnwCreationConfigRefusal& e){need(e.code()==runtime::BnwCreationConfigError::rules_retired,"Current parent invalidation refuses before options stores/SQL");}
    }
    {
        auto source=fixture(victories+options);auto c=runtime::BnwCreationConfig::from_selected_rules(source,choices());
        auto p=P::from_creation_config(c,selected_config_ini);menu_prefix_check(p);
        std::weak_ptr<P> weak_pg=p;std::weak_ptr<const runtime::BnwCreationConfig> weak_c=c;
        auto sibling=p->roster();p.reset();c.reset();source.reset();
        need(!weak_pg.expired() && !weak_c.expired(),"Alias retains original actual menu/DB owner for continuation");
        auto held=weak_pg.lock();advance(held);held.reset();sibling.reset();
        need(weak_pg.expired() && weak_c.expired(),"Retained option SQL child/roster graph acyclic");
    }
}

void environment_cases(const std::filesystem::path& selected_config_ini) {
    using P=runtime::BnwFreshPregameSpeed;using E=runtime::BnwFreshPregameSpeedError;
    using B=runtime::BnwFreshPregameEnvironmentSetterBoundary;
    const std::string schema=R"sql(
CREATE TABLE Victories(ID INTEGER,Type TEXT);
ALTER TABLE Climates ADD COLUMN DesertPercentChange INTEGER;
ALTER TABLE Climates ADD COLUMN JungleLatitude INTEGER;
ALTER TABLE Climates ADD COLUMN HillRange INTEGER;
ALTER TABLE Climates ADD COLUMN MountainPercent INTEGER;
ALTER TABLE Climates ADD COLUMN SnowLatitudeChange REAL;
ALTER TABLE Climates ADD COLUMN RandIceLatitude REAL;
UPDATE Climates SET DesertPercentChange=7,JungleLatitude=40,HillRange=2,MountainPercent=4,SnowLatitudeChange=0.125,RandIceLatitude=0.75;
INSERT INTO Climates VALUES(5,'CLIMATE_OTHER',-4,30,3,10,0.25,0.5);
INSERT INTO Climates VALUES(7,'CLIMATE_DUP',1,1,1,1,0.5,0.25),(9,'CLIMATE_DUP',2,2,2,2,0.75,0.125);
ALTER TABLE SeaLevels ADD COLUMN SeaLevelChange INTEGER;
UPDATE SeaLevels SET SeaLevelChange=0;
INSERT INTO SeaLevels VALUES(3,'SEALEVEL_HIGH',12),(5,'SEALEVEL_DUP',9),(6,'SEALEVEL_DUP',19);
)sql";
    const auto prepare=[&](const std::shared_ptr<P>& p) {
        menu_prefix_check(p);refuses([&]{p->advance_single_player_menu_options_prefix();},E::native_source_unavailable);
        need(p->observe().menu.boundary==runtime::BnwFreshPregameMenuBoundary::load_pregame_settings_required,
             "Environment calls require actual same menu prefix");
    };
    {
        auto source=fixture(schema);auto c=runtime::BnwCreationConfig::from_selected_rules(source,choices());
        auto p=P::from_creation_config(c,selected_config_ini);const auto initial=p->environment_observation();
        need(initial.climate.selected_id==-1 && initial.sea_level.selected_id==-1 &&
            initial.climate.numbers.at("ID")==-1 && initial.sea_level.numbers.at("ID")==-1 &&
            initial.climate.boundary==B::not_called && initial.climate.floats.at("IceLatitude")==0.0f,
            "Namespace IDs and actual empty native cache construction stay distinct");

        need(p->selected_climate_info().selected_id==-1 && p->selected_sea_level_info().selected_id==-1 &&
            p->selected_climate_info().text.at("Description").empty(),
            "Original namespace/cache -1 equality returns genuine empty Info without SQL/world authority");

        refuses([&]{p->set_selected_climate("CLIMATE_TEMPERATE");},E::wrong_phase);
        prepare(p);const auto hash=source->rules->gameplay_fingerprint_sha256();
        const auto old_slots=p->roster()->observe().writes;
        bool foreign=false;std::thread t([&]{try{p->set_selected_climate("CLIMATE_OTHER");}
            catch(const runtime::BnwFreshPregameSpeedRefusal& e){foreign=e.code()==E::wrong_thread;}});t.join();
        need(foreign && p->environment_observation().climate.call_count==0,"Foreign thread cannot mark an environment request");
        auto copied=*p;refuses([&]{copied.set_selected_sea_level("SEALEVEL_HIGH");},E::retired);
        p->set_selected_climate("CLIMATE_OTHER");auto a=p->environment_observation();
        need(a.climate.selected_id==5 && a.climate.numbers.at("ID")==5 &&
            a.climate.numbers.at("DesertPercentChange")==-4 && a.climate.floats.at("SnowLatitudeChange")==0.25f &&
            a.climate.floats.at("TundraLatitudeChange")==0.0f && a.climate.floats.at("RandIceLatitude")==0.5f &&
            a.climate.order[0]=="ID" && a.climate.order[6]=="DisabledHelp" &&
            a.climate.order[7]=="DesertPercentChange" && a.climate.order[17]=="RandIceLatitude" &&
            a.climate.order.back()=="s_climate <- cache.GetID" && a.climate.boundary==B::cache_and_selection_stored,
            "Actual Base7->Climate4int7float->selected ID source order and missing native getters0");
        const auto borrowed=p->selected_climate_info();
        need(borrowed.order==a.climate.order && borrowed.numbers==a.climate.numbers && borrowed.floats==a.climate.floats,
            "Actual equal-ID getter performs no second cache writes or Locale");
        refuses([&]{copied.selected_climate_info();},E::retired);
        p->set_selected_sea_level("SEALEVEL_HIGH");auto b=p->environment_observation();
        need(b.sea_level.selected_id==3 && b.sea_level.numbers.at("SeaLevelChange")==12 &&
            b.sea_level.order[7]=="SeaLevelChange" && b.sea_level.order.back()=="s_seaLevel <- cache.GetID",
            "Actual Base7->SeaLevel int->selected ID");
        p->set_selected_climate("CLIMATE_DUP");p->set_selected_sea_level("SEALEVEL_DUP");auto d=p->environment_observation();
        need(d.climate.selected_id==7 && d.sea_level.selected_id==5 && d.climate.call_count==2 && d.sea_level.call_count==2,
            "Exact SelectAt(Type) LIMIT1 uses first row, not catalog last-wins or dense IDs");
        p->set_selected_climate("CLIMATE_DUP");
        need(p->environment_observation().climate.call_count==3 && p->roster()->observe().writes==old_slots &&
            source->rules->gameplay_fingerprint_sha256()==hash &&
            p->observe().menu.boundary==runtime::BnwFreshPregameMenuBoundary::load_pregame_settings_required,
            "Repeated completed request recaches; no slots, original DB, or settings readiness mutation");
        refuses([&]{p->set_selected_climate("CLIMATE_MISSING");},E::native_source_unavailable);
        const auto failed=p->environment_observation();
        need(failed.climate.boundary==B::failed_prefix && failed.climate.call_count==4 && failed.climate.selected_id==7,
            "Missing genuine row quarantines request without guessed ID/cache");
        refuses([&]{p->set_selected_climate("CLIMATE_OTHER");},E::wrong_phase);
        refuses([&]{(void)p->selected_climate_info();},E::wrong_phase);
        refuses([&]{(void)p->selected_sea_level_info();},E::wrong_phase);
    }
    {
        auto source=fixture(schema+"UPDATE Climates SET MountainPercent=2147483648 WHERE Type='CLIMATE_OTHER';");
        auto p=P::from_creation_config(runtime::BnwCreationConfig::from_selected_rules(source,choices()),selected_config_ini);prepare(p);
        refuses([&]{p->set_selected_climate("CLIMATE_OTHER");},E::native_source_unavailable);
        const auto x=p->environment_observation().climate;
        need(x.numbers.at("ID")==5 && x.numbers.at("HillRange")==3 && x.numbers.at("MountainPercent")==0 &&
             x.floats.at("SnowLatitudeChange")==0.0f && x.selected_id==-1 && x.boundary==B::failed_prefix,
             "Reached native i32 overflow preserves earlier cache stores, stops before float/selection writes");
    }
    {
        auto source=fixture(schema+"ALTER TABLE Climates ADD COLUMN Text TEXT;UPDATE Climates SET Text='TXT_ENVIRONMENT_MISSING' WHERE Type='CLIMATE_OTHER';");
        auto p=P::from_creation_config(runtime::BnwCreationConfig::from_selected_rules(source,choices()),selected_config_ini);prepare(p);
        bool denied=false;try{p->set_selected_climate("CLIMATE_OTHER");}catch(const runtime::ScriptError&){denied=true;}
        const auto x=p->environment_observation().climate;
        need(denied && x.numbers.at("ID")==5 && x.text.at("Text")=="TXT_ENVIRONMENT_MISSING" &&
            x.numbers.at("DesertPercentChange")==0 && x.selected_id==-1 && x.boundary==B::failed_prefix,
            "Genuine Locale failure remains before Climate derived fields and selected ID");
        refuses([&]{p->set_selected_climate("CLIMATE_OTHER");},E::wrong_phase);
    }
}


void environment_numeric_getter_cases(const std::filesystem::path& selected_config_ini) {
    using P=runtime::BnwFreshPregameSpeed;using E=runtime::BnwFreshPregameSpeedError;
    const std::string schema=R"sql(
CREATE TABLE Victories(ID INTEGER,Type TEXT);
ALTER TABLE Climates ADD COLUMN DesertPercentChange INTEGER;
INSERT INTO Climates(ROWID,ID,Type,DesertPercentChange) VALUES(0,77,'CLIMATE_REAL_ROWID_ZERO',19);
ALTER TABLE SeaLevels ADD COLUMN SeaLevelChange INTEGER;
UPDATE SeaLevels SET SeaLevelChange=12;
)sql";
    auto source=fixture(schema);auto c=runtime::BnwCreationConfig::from_selected_rules(source,choices());
    auto p=P::from_creation_config_for_reset_setup(c,selected_config_ini);p->advance_reset_game();
    const auto hash=source->rules->gameplay_fingerprint_sha256();const auto before=p->environment_observation();
    need(before.climate.selected_id==0 && before.climate.numbers.at("ID")==-1 &&
        before.sea_level.selected_id==1 && before.sea_level.numbers.at("ID")==-1,
        "Actual reset0/1 is separate from untouched native Info caches");
    const auto first=p->selected_climate_info();
    need(first.selected_id==0 && first.numbers.at("ID")==77 && first.numbers.at("DesertPercentChange")==19 &&
        first.order.size()==18 && first.call_count==0 && first.requested_type.empty() &&
        first.boundary==runtime::BnwFreshPregameEnvironmentSetterBoundary::not_called,
        "Native mismatch getter selects ROWID0 authoredID77 and DOES NOT perform a setter or change selected ID");
    const auto second=p->selected_climate_info();
    need(second.selected_id==0 && second.numbers.at("ID")==77 && second.order.size()==36,
        "Original mismatch stays: second getter executes another actual numeric cache read, no fake once/cache-match repair");
    const auto sea=p->selected_sea_level_info();
    need(sea.selected_id==1 && sea.numbers.at("ID")==0 && sea.numbers.at("SeaLevelChange")==12 && sea.order.size()==8,
        "Numeric ROWID1 SeaLevel lookup must not accidentally select authoredID1 or update namespace ID");
    need(source->rules->gameplay_fingerprint_sha256()==hash && p->observe().game_turn==0,
        "Actual getter cache mutation does not mutate sealed rules or game clock");
    auto missing=P::from_creation_config_for_reset_setup(runtime::BnwCreationConfig::from_selected_rules(
        fixture("CREATE TABLE Victories(ID INTEGER,Type TEXT);"),choices()),selected_config_ini);
    missing->advance_reset_game();refuses([&]{(void)missing->selected_climate_info();},E::native_source_unavailable);
    need(missing->environment_observation().climate.numbers.at("ID")==-1,
        "Missing actual ROWID0 refuses before any cache store; authoredID0 at ROWID1 cannot substitute");
    refuses([&]{(void)missing->selected_sea_level_info();},E::wrong_phase);
}
void world_cases(const std::filesystem::path& selected_config_ini,const std::filesystem::path& scratch) {
    using P=runtime::BnwFreshPregameSpeed;using E=runtime::BnwFreshPregameSpeedError;
    using W=runtime::BnwFreshPregameWorldSetterBoundary;
    const std::string schema=R"sql(
ALTER TABLE Worlds ADD COLUMN DefaultPlayers INTEGER;
ALTER TABLE Worlds ADD COLUMN GridWidth INTEGER;
ALTER TABLE Worlds ADD COLUMN NumCitiesUnhappinessPercent INTEGER;
UPDATE Worlds SET DefaultPlayers=3,GridWidth=80,NumCitiesUnhappinessPercent=90;
INSERT INTO Worlds VALUES(5,'WORLDSIZE_SECOND',4,100,110);
INSERT INTO Worlds VALUES(8,'WORLDSIZE_ZERO',0,20,100);
INSERT INTO Worlds VALUES(10,'WORLDSIZE_NEGATIVE',-1,21,100);
INSERT INTO Worlds VALUES(12,'WORLDSIZE_ALL',2147483647,22,100);
INSERT INTO Worlds VALUES(14,'WORLDSIZE_DUP',6,200,999);
INSERT INTO Worlds VALUES(16,'WORLDSIZE_DUP',62,201,998);
CREATE TABLE Victories(ID INTEGER,Type TEXT);INSERT INTO Victories VALUES(0,'VICTORY_A');
)sql";
    const auto prepare=[&](const std::shared_ptr<P>& p) {
        menu_prefix_check(p);
        refuses([&]{p->advance_single_player_menu_options_prefix();},E::native_source_unavailable);
        need(p->observe().menu.boundary==runtime::BnwFreshPregameMenuBoundary::load_pregame_settings_required,
             "World call needs actual private menu, no supplied phase proof");
    };
    {
        auto parent=fixture(schema);auto c=runtime::BnwCreationConfig::from_selected_rules(parent,choices());
        auto p=P::from_creation_config(c,selected_config_ini);
        refuses([&]{p->set_selected_world_size("WORLDSIZE_STANDARD");},E::wrong_phase);
        const auto constructor_world=p->selected_world_info();
        need(constructor_world.world_size==-1 && constructor_world.world_info_id==-1 &&
             constructor_world.world_order.empty() && constructor_world.world_text.at("Description").empty(),
             "Genuine constructor WorldInfo equality returns empty Info, without SQL, guessed settings or map readiness");
        prepare(p);const auto source_hash=parent->rules->gameplay_fingerprint_sha256();
        const auto old=p->roster()->observe();
        bool foreign=false;std::thread t([&]{try{p->set_selected_world_size("WORLDSIZE_STANDARD");}
            catch(const runtime::BnwFreshPregameSpeedRefusal& e){foreign=e.code()==E::wrong_thread;}});t.join();
        need(foreign && p->observe().world_setter.call_count==0,"World thread fence before SQL/call marker");
        auto copied=*p;refuses([&]{copied.set_selected_world_size("WORLDSIZE_STANDARD");},E::retired);
        refuses([&]{(void)copied.selected_world_info();},E::retired);
        foreign=false;std::thread getter_thread([&]{try{(void)p->selected_world_info();}
            catch(const runtime::BnwFreshPregameSpeedRefusal& e){foreign=e.code()==E::wrong_thread;}});getter_thread.join();
        need(foreign,"WorldInfo getter checks constructor thread before cache/SQL");
        p->set_selected_world_size("WORLDSIZE_STANDARD");auto a=p->roster()->observe();
        need(a.world_size==0 && a.world_info_id==0 && a.world_default_players==3 &&
             a.world_numbers.at("GridWidth")==80 && a.world_numbers.at("NumCitiesUnhappinessPercent")==90 &&
             a.world_numbers.at("EstimatedNumCities")==0,"Exact first selected Type/LIMIT1 row, native missing-column GetInt0");
        need(a.world_order[0]=="ID" && a.world_order[6]=="DisabledHelp" && a.world_order[7]=="DefaultPlayers" &&
             a.world_order[26]=="EstimatedNumCities" && a.world_order[27]=="s_worldSize <- cache.GetID" &&
             a.world_order[28]=="ResetSlots getDefaultPlayers","Full Base→World20→worldSize→nested ResetSlots order");
        for(std::size_t i=0;i<63;++i) {
            const auto& s=a.slots[i];
            need(s.team==static_cast<std::int32_t>(i),"Filled slots preserve prior native team; reset slots store actual team");
            if(i<3)need(s.status==1 && s.claim==2 && s.civilization==-1 && s.civilization_available &&
                s.civilization_playable && s.handicap==(i==0?9:11),"Nonzero SP branch uses assigned2/AI cached11 exceptplayer0");
            else need(s.status==(i<22?4:2) && s.claim==0 && s.handicap==9,"Remaining ResetPlayer then observer store");
        }
        need(a.slots[63]==old.slots[63],"World ResetSlots never mutates barbarian63");
        const auto actual_info=p->selected_world_info();
        need(actual_info.world_numbers==a.world_numbers && actual_info.world_text==a.world_text &&
             actual_info.world_order==a.world_order && actual_info.slots==a.slots &&
             actual_info.selected_world_reset_calls==a.selected_world_reset_calls,
             "Native WorldInfo equal-ID getter performs no CacheResult, Locale or ResetSlots replay");
        const auto first=p->observe();
        need(first.world_setter.boundary==W::slots_reset_returned && first.world_setter.call_count==1 &&
             first.menu.boundary==runtime::BnwFreshPregameMenuBoundary::load_pregame_settings_required,
             "World setter completes only its own real nested return, not LoadPreGameSettings");
        p->set_selected_world_size("WORLDSIZE_SECOND");a=p->roster()->observe();
        need(a.world_size==5 && a.world_default_players==4 && a.selected_world_reset_calls==2 &&
             a.slots[3].status==1 && a.slots[3].claim==2 && a.slots[3].handicap==11,"Second native setter expands actual retained slots");
        p->set_selected_world_size("WORLDSIZE_SECOND");const auto same=p->roster()->observe();
        need(same.selected_world_reset_calls==3 && same.slots==a.slots,"Repeated completed call preserves existing COMPUTER slot state");
        p->set_selected_world_size("WORLDSIZE_ZERO");a=p->roster()->observe();
        for(std::size_t i=0;i<63;++i)need(a.slots[i].status==(i<22?4:2) && a.slots[i].handicap==9,
            "Real selected zero-count row performs all resets, not a constructor no-op");
        p->set_selected_world_size("WORLDSIZE_NEGATIVE");need(p->roster()->observe().slots==a.slots,"Signed negative suggestion follows native less-than branch");
        p->set_selected_world_size("WORLDSIZE_ALL");a=p->roster()->observe();
        need(a.slots[62].status==1 && a.slots[62].claim==2 && a.slots[62].handicap==11 && a.slots[63]==old.slots[63],
             "Large native suggestion assigns only reached63; no clamp or int32 overflow");
        p->set_selected_world_size("WORLDSIZE_DUP");a=p->roster()->observe();
        need(a.world_size==14 && a.world_default_players==6,"Native first LIMIT1 row differs from catalog last-wins; do not substitute global Type ID");
        need(p->observe().init_inputs.map_seed==first.init_inputs.map_seed &&
             p->observe().init_inputs.sync_seed==first.init_inputs.sync_seed && parent->rules->gameplay_fingerprint_sha256()==source_hash,
             "World/slot setter leaves RNG inputs and sealed original source unchanged");
        const auto before_fail=p->roster()->observe();
        refuses([&]{p->set_selected_world_size("NO_SUCH_WORLD");},E::native_source_unavailable);
        need(p->roster()->observe().slots==before_fail.slots && p->roster()->observe().world_numbers==before_fail.world_numbers,
             "Missing Type has no guessed cache/slot writes");
        refuses([&]{p->set_selected_world_size("WORLDSIZE_ZERO");},E::wrong_phase);
        refuses([&]{(void)p->selected_world_info();},E::wrong_phase);
    }
    {
        auto parent=fixture(schema+"UPDATE Worlds SET GridWidth=2147483648 WHERE ID=5;");
        auto c=runtime::BnwCreationConfig::from_selected_rules(parent,choices());auto p=P::from_creation_config(c,selected_config_ini);prepare(p);
        const auto before=p->roster()->observe();
        try{p->set_selected_world_size("WORLDSIZE_SECOND");throw std::runtime_error("Overflow world accepted");}
        catch(const runtime::BnwFreshPregameRosterRefusal& e){need(e.code()==runtime::BnwFreshPregameRosterError::native_source_unavailable,"Wrong reached overflow refusal");}
        const auto a=p->roster()->observe();
        need(a.world_info_id==5 && a.world_size==-1 && a.world_default_players==4 && a.world_numbers.at("GridWidth")==0 &&
             a.world_numbers.at("NumCitiesUnhappinessPercent")==100 && a.slots==before.slots && a.selected_world_reset_calls==0,
             "Reached cache overflow retains earlier fields and does not publish worldSize/slots/future fields");
        refuses([&]{p->set_selected_world_size("WORLDSIZE_ZERO");},E::wrong_phase);
    }
    {
        auto parent=fixture(schema+"ALTER TABLE Worlds ADD COLUMN Text TEXT;UPDATE Worlds SET Text='TXT_KEY_MISSING' WHERE ID=0;");
        auto c=runtime::BnwCreationConfig::from_selected_rules(parent,choices());auto p=P::from_creation_config(c,selected_config_ini);prepare(p);
        bool failed=false;try{p->set_selected_world_size("WORLDSIZE_STANDARD");}catch(const runtime::ScriptError&){failed=true;}
        need(failed && p->observe().world_setter.boundary==W::failed_prefix,"Reached real localization source failure is not success");
        const auto a=p->roster()->observe();need(a.world_info_id==0 && a.world_text.at("Text")=="TXT_KEY_MISSING" &&
            a.world_default_players==0 && a.world_size==-1 && a.selected_world_reset_calls==0,"Locale error after base keys before numeric/defaultPlayers/slots");
    }
    {
        const auto own_ini=scratch/"world-selected.ini";
        {std::ofstream f(own_ini,std::ios::binary|std::ios::trunc);need(static_cast<bool>(f),"World private INI open");
            f<<"[User Settings]\nLanguage=en_US\n";need(static_cast<bool>(f),"World private INI write");}
        const std::string text=R"sql(
CREATE TABLE Languages(Type TEXT,Name TEXT);INSERT INTO Languages VALUES('en_US','English'),('ru_RU','Russian');
CREATE TABLE LanguagePriorities(LanguageSet TEXT,Language TEXT,Priority INTEGER);
INSERT INTO LanguagePriorities VALUES('en_US','en_US',1),('ru_RU','ru_RU',1);
CREATE TABLE LocalizedText(Language TEXT,Tag TEXT,Text TEXT);INSERT INTO LocalizedText VALUES('en_US','TXT_WORLD','chosen world'),('ru_RU','TXT_WORLD','other world');
)sql";
        auto parent=fixture(schema+"ALTER TABLE Worlds ADD COLUMN Description TEXT;UPDATE Worlds SET Description='TXT_WORLD' WHERE ID=0;",text);
        auto c=runtime::BnwCreationConfig::from_selected_rules(parent,choices());auto p=P::from_creation_config(c,own_ini);prepare(p);
        p->set_selected_world_size("WORLDSIZE_STANDARD");const auto a=p->roster()->observe();
        need(a.world_text.at("DescriptionKey")=="TXT_WORLD" && a.world_text.at("Description")=="chosen world" &&
             a.world_text.at("Text")=="TXT_WORLD" && a.world_text.at("LocalizedText")=="chosen world",
             "Native Description fallback uses actual selected Locale after base field stores");
        p->set_selected_world_size("WORLDSIZE_SECOND");const auto b=p->roster()->observe();
        need(b.world_text.at("Text").empty() && b.world_text.at("Description")=="chosen world" &&
             b.world_text.at("LocalizedText")=="chosen world","NULL Description leaves previous native descriptions/text values unwritten");
        {std::ofstream f(own_ini,std::ios::binary|std::ios::trunc);f<<"[User Settings]\nLanguage=ru_RU\n";need(static_cast<bool>(f),"World private mutation");}
        bool changed=false;try{p->set_selected_world_size("WORLDSIZE_STANDARD");}
        catch(const runtime::BnwSelectedLocaleRefusal& e){changed=e.code()==runtime::BnwSelectedLocaleError::input_changed;}
        need(changed,"Held actual Locale source mutation retires before new world query/marker");
        // Private test input only; production never writes or clears selected INI.
        std::error_code cleanup;std::filesystem::remove(own_ini,cleanup);
    }
    {
        auto parent=fixture(schema);auto c=runtime::BnwCreationConfig::from_selected_rules(parent,choices());auto p=P::from_creation_config(c,selected_config_ini);prepare(p);
        const auto old=p->roster()->observe();parent->profile.can_activate=false;
        bool failed=false;try{p->set_selected_world_size("WORLDSIZE_STANDARD");}
        catch(const runtime::BnwCreationConfigRefusal&){failed=true;}
        need(failed,"Retired actual source refuses before world SQL/stores");(void)old;
    }
    {
        auto parent=fixture(schema);auto c=runtime::BnwCreationConfig::from_selected_rules(parent,choices());auto p=P::from_creation_config(c,selected_config_ini);prepare(p);
        auto alias=p->roster();std::weak_ptr<P> weak=p;std::weak_ptr<const runtime::BnwCreationConfig> wc=c;
        p.reset();c.reset();parent.reset();auto actual=weak.lock();
        need(actual && !wc.expired(),"External roster alias retains original PG and Config");
        actual->set_selected_world_size("WORLDSIZE_STANDARD");need(alias->observe().world_default_players==3,"Actual same alias observes canonical mutable world cache");
        actual.reset();alias.reset();need(weak.expired() && wc.expired(),"World child adds no PG/roster cycle");
    }
}
void actual(const std::filesystem::path& installation,const std::filesystem::path& selected_config_ini){
    content::Catalog catalog;catalog.packages.push_back(content::read_manifest(installation/"Assets/DLC/Expansion/Expansion1.Civ5Pkg"));
    catalog.packages.push_back(content::read_manifest(installation/"Assets/DLC/Expansion2/Expansion2.Civ5Pkg"));
    constexpr std::array<std::size_t,2> selected{0,1};
    auto parent=std::make_shared<runtime::BnwFreshGameContentResult>(runtime::bootstrap_bnw_fresh_game_content(installation,catalog,selected));
    need(parent->content_ready(),"Actual selected BNW source unavailable");parent->rules->seal_immutable_preview_fingerprints();
    auto c=runtime::BnwCreationConfig::from_selected_rules(parent,choices());auto p=runtime::BnwFreshPregameSpeed::from_creation_config(c,selected_config_ini);
    const auto source_before=parent->rules->gameplay_fingerprint_sha256();
    auto expected_rules=parent->rules->clone_gameplay();
    const auto expected_post=expected_rules->postprocess_gameplay(content::GameplayPostprocessProfile::bnw,{});
    need(expected_post.transaction_released,"Actual expected PostDefines transaction");
    const auto expected=expected_rules->global_define("STANDARD_HANDICAP",content::GlobalDefineKind::integer);
    need(expected.error.empty(),"Actual independent cache Define lookup");
    const auto expected_value=std::get<std::int64_t>(expected.value);
    need(expected_value>=-2147483648LL && expected_value<=2147483647LL,"Actual cache native width");
    auto menu=runtime::BnwFreshPregameSpeed::from_creation_config(c,selected_config_ini);
    menu_prefix_check(menu,static_cast<std::int32_t>(expected_value));
    try{menu->advance_single_player_menu_options_prefix();throw std::runtime_error("Actual menu skipped LoadPreGameSettings");}
    catch(const runtime::BnwFreshPregameSpeedRefusal& e){need(e.code()==runtime::BnwFreshPregameSpeedError::native_source_unavailable &&
        std::string(e.what()).find("LoadPreGameSettings")!=std::string::npos,"Actual selected options must reach exact next settings frontier");}
    const auto menu_options=menu->observe();
    const auto expected_victories=expected_rules->prefetch_info("Victories",{},content::InfoCollectionBudget{65536});
    need(expected_victories.all_succeeded() && expected_victories.slots<=65536 &&
        menu_options.menu.boundary==runtime::BnwFreshPregameMenuBoundary::load_pregame_settings_required &&
        menu_options.reset.victory_count==static_cast<std::int32_t>(expected_victories.slots) &&
        menu_options.reset.victories.size()==expected_victories.slots &&
        std::all_of(menu_options.reset.victories.begin(),menu_options.reset.victories.end(),[](bool v){return v;}) &&
        menu_options.options.custom.empty() && menu_options.options.hash_indices.empty() && menu_options.menu.map_options.empty(),
        "Actual same selected derived DB native Victory span/reset and reached options/map stores");
    need(menu_options.options.quick_combat==(menu_options.options.enum_values[15]!=0) &&
        menu_options.options.enum_values[16]==0,"Actual native Sync nonzeroQuickCombat and NULL conversion slot16");
    need(parent->rules->gameplay_fingerprint_sha256()==source_before,"Private cache/options SQL never mutates selected source DB");
    std::optional<std::int64_t> expected_world_id,expected_players;
    const auto world_type=c->metadata().info[0].type;
    const auto world_report=expected_rules->query_rows({"SELECT * from Worlds where Type = ? LIMIT 1",{world_type}},
        [&](const content::RulesRowView& row){expected_world_id=row.integer("ID");expected_players=row.integer("DefaultPlayers");},1);
    need(world_report.all_succeeded() && expected_world_id && expected_players && *expected_players>0 &&
         *expected_players<=63,"Actual selected nonzero world source row required");
    const auto old_world=menu->roster()->observe();menu->set_selected_world_size(world_type);
    const auto world=menu->roster()->observe();
    need(world.world_size==*expected_world_id && world.world_info_id==*expected_world_id &&
         world.world_default_players==*expected_players && world.selected_world_reset_calls==1 &&
         menu->observe().world_setter.boundary==runtime::BnwFreshPregameWorldSetterBoundary::slots_reset_returned &&
         menu->observe().menu.boundary==runtime::BnwFreshPregameMenuBoundary::load_pregame_settings_required,
         "Actual DB/cache/string setter returns all nonzero ResetSlots without LoadPreGameSettings grant");
    for(std::size_t i=0;i<63;++i) {
        const auto& slot=world.slots[i];
        if(static_cast<std::int64_t>(i)<*expected_players)
            need(slot.status==1 && slot.claim==2 && slot.civilization==-1 &&
                 slot.handicap==(i==0?old_world.slots[0].handicap:menu->observe().cached_globals.ai_handicap),
                 "Every actual admitted SP slot uses source-derived count/status/AI cache");
        else need(slot.status==(i<22?4:2),"Actual remaining resets/observer slots preserve native22/63 boundaries");
    }
    need(world.slots[63]==old_world.slots[63] && parent->rules->gameplay_fingerprint_sha256()==source_before,
         "Actual nonzero world reset never mutates barbarian or sealed source DB");
    std::cout<<"actual_world_setter_id="<<world.world_size<<" default_players="<<world.world_default_players
             <<" native_slots_reset=63 full_settings=0\n";

    // Actual installed rows independently read from SAME selected derived SQL.
    menu->set_selected_climate("CLIMATE_TEMPERATE");menu->set_selected_sea_level("SEALEVEL_MEDIUM");
    const auto environment=menu->environment_observation();
    const auto verify_environment=[&](bool climate) {
        const auto& observed=climate?environment.climate:environment.sea_level;
        const auto report=expected_rules->query_rows(
            {std::string("SELECT * from ")+(climate?"Climates":"SeaLevels")+" where Type = ? LIMIT 1",{observed.requested_type}},
            [&](const content::RulesRowView& row) {
                need(observed.selected_id==row.integer("ID") && observed.numbers.at("ID")==row.integer("ID"),
                    "Actual installed environment first SQL row/ID");
                for(const auto& [name,value]:observed.numbers)need(value==row.integer(name),"Every actual installed environment integer agrees");
                for(const auto& [name,value]:observed.floats)need(value==static_cast<float>(row.real(name)),"Every actual installed climate float agrees");
            },1);
        need(report.all_succeeded() && report.rows==1 && observed.boundary==runtime::BnwFreshPregameEnvironmentSetterBoundary::cache_and_selection_stored,
            "Actual installed cache setter must return from one genuine selected row");
    };
    verify_environment(true);verify_environment(false);

    const auto same_world=menu->selected_world_info();
    need(same_world.world_size==world.world_size && same_world.world_numbers==world.world_numbers &&
         same_world.world_text==world.world_text && same_world.world_order==world.world_order && same_world.slots==world.slots,
         "Actual installed WorldInfo matching getter retains same native cache/Locale and player slots");
    const auto climate_info=menu->selected_climate_info();const auto sea_info=menu->selected_sea_level_info();
    need(climate_info.selected_id==environment.climate.selected_id &&
        sea_info.selected_id==environment.sea_level.selected_id &&
        climate_info.order==environment.climate.order && sea_info.order==environment.sea_level.order,
        "Original matching cache getters reuse SAME actual Info with no cache/Locale/setter replay");

    // Component query deliberately does not issue a native Pregame/Game join.
    // Exact installed InlandSea bytes must execute and reach its genuine next
    // feature callback frontier, rather than a synthetic test-world PASS.
    const auto resources=runtime::build_bnw_map_script_resources(installation,parent->profile,catalog);
    runtime::BnwMapSetupContext map_setup;
    map_setup.world_size=world.world_size;map_setup.climate=environment.climate.selected_id;
    map_setup.custom_options=std::map<std::string,std::int64_t>{{"1",2},{"2",2},{"3",2}};
    map_setup.selected_rules=parent.get();map_setup.graphics_initialized=false;
    const auto map=runtime::generate_bnw_plot_map(resources,{"Assets/Maps/InlandSea.lua",std::nullopt,std::nullopt},map_setup,
        runtime::SimulationRandom(runtime::RandomAlgorithm::bnw,{123,0,0}));
    need(map.source && map.source->relative_path=="Assets/Maps/InlandSea.lua" && map.included_sources.size()==9 &&
        map.map && map.map->owned_setup_grid && map.map->width==52 && map.map->height==32 &&
        map.map_random_after && map.map_random_after->calls==300131 &&
        std::any_of(map.blockers.begin(),map.blockers.end(),[](const auto& block) {
            return block.code==runtime::BnwMapScriptBlockerCode::features_rivers_pending &&
                block.message.find("GameplayFeatureChanged")!=std::string::npos;
        }),"Actual installed InlandSea component must retain real feature callback frontier after genuine environment setters");
    need(menu->observe().menu.boundary==runtime::BnwFreshPregameMenuBoundary::load_pregame_settings_required,
        "Real script component result does not complete native LoadPreGameSettings or new game");
    std::cout<<"actual_environment_climate="<<environment.climate.selected_id<<" sea_level="<<environment.sea_level.selected_id
             <<" selected_inlandsea_component=1 native_map_join=0 full_new_game=0\n";

    p->advance_selected_speed();const auto x=p->observe();
    need(x.speed==c->metadata().info[3].id,"Actual performed setter agrees with independent selected source binding");
    need(x.boundary==runtime::BnwFreshPregameSpeedBoundary::selected_speed_setter_complete,"Actual selected localization/setter must complete");
    auto setup=runtime::BnwFreshPregameSpeed::from_creation_config_for_reset_setup(c,selected_config_ini);
    setup->advance_reset_game();const auto reset=setup->observe();
    const auto victories=parent->rules->prefetch_info("Victories",{},content::InfoCollectionBudget{65536});
    need(victories.all_succeeded() && reset.reset.victory_count==static_cast<std::int32_t>(victories.slots) &&
         reset.game_turn==0 && reset.reset.boundary==runtime::BnwFreshPregameResetBoundary::reset_game_settings_assignment_required,
         "Actual selected native reset must complete with all native Victory slots and real gameTurn0");
    const auto reset_climate=setup->selected_climate_info();const auto reset_sea=setup->selected_sea_level_info();
    need(reset_climate.selected_id==0 && reset_sea.selected_id==1,
        "Real numeric getter never replaces reset0/1 namespace IDs from cache authored ID");
    for(const bool climate:{true,false}) {
        const auto& info=climate?reset_climate:reset_sea;bool found=false;
        const auto row=expected_rules->query_rows({std::string("select * from ")+(climate?"Climates":"SeaLevels")+" where ROWID = ? LIMIT 1",
            {static_cast<std::int64_t>(info.selected_id)}},[&](const content::RulesRowView& value) {
                found=true;need(info.numbers.at("ID")==value.integer("ID"),"Actual independent ROWID getter cache binds source row");
            },1);
        need(row.all_succeeded() && found,"Actual installed reset selects a genuine numeric ROWID row");
    }
    setup->advance_selected_speed();need(setup->observe().game_turn==0 && setup->observe().speed==x.speed,"Actual SAME reset then selected assignment");
    std::cout<<"actual_reset_game_turn="<<reset.game_turn<<" victory_slots="<<reset.reset.victory_count<<" settings_slots_global_hooks=UNOBSERVED\n";
    std::cout<<"actual_pregame_speed="<<x.speed<<" timer="<<x.timer<<" completed_setter="
        <<(x.boundary==runtime::BnwFreshPregameSpeedBoundary::selected_speed_setter_complete)<<" full_pregame=0\n";
}
}
int main(int argc,char** argv){try{need(argc==4,"Expected BNW installation, selected INI and private scratch");cases(argv[2],argv[3]);reset_cases(argv[2]);cache_cases(argv[2]);menu_lifecycle_cases(argv[2]);menu_options_cases(argv[2]);environment_cases(argv[2]);environment_numeric_getter_cases(argv[2]);world_cases(argv[2],argv[3]);actual(argv[1],argv[2]);return 0;}
    catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
