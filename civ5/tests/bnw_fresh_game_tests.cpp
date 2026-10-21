#include "civ5/runtime/bnw_fresh_game.hpp"
#include "civ5/runtime/bnw_fresh_city_manager.hpp"
#include "civ5/runtime/bnw_fresh_tactical_map.hpp"
#include "civ5/runtime/bnw_fresh_advisors.hpp"
#include "sqlite3.h"
#include "civ5/runtime/lua_script_host.hpp"
#include <array>
#include <algorithm>
#include <iostream>
#include <limits>
#include <thread>
#include <utility>
using namespace civ5;
namespace {
void need(bool v,const char* message) {if(!v) throw std::runtime_error(message);}
template<class F> void refuses(F f,runtime::BnwFreshGameError expected) {
    try {f();} catch(const runtime::BnwFreshGameRefusal& e) {need(e.code()==expected,"Unexpected Game refusal");return;}
    throw std::runtime_error("Game accepted unavailable operation");
}
runtime::BnwCreationChoices choices() {
    return {"WORLDSIZE_STANDARD","CLIMATE_TEMPERATE","SEALEVEL_MEDIUM",
            "GAMESPEED_STANDARD","ERA_ANCIENT","HANDICAP_PRINCE"};
}
// Detached SQLite source-order fixture, not selected-installation, map or
// gameplay authority. The actual installation test below has separate origin.
std::shared_ptr<runtime::BnwFreshGameContentResult> fixture(const std::string& extra={}) {
    auto p=std::make_shared<runtime::BnwFreshGameContentResult>();
    p->profile.kind=runtime::ContentProfileKind::dlc;
    p->profile.can_activate=p->profile.all_requirements_located=true;
    p->profile.enabled_dlc={0};runtime::ContentRequirement r;
    r.kind=content::PackageKind::dlc;r.id="6da07636-4123-4018-b643-6575b4ec336b";
    r.status=runtime::RequirementStatus::located;r.package=0;p->profile.requirements.push_back(r);
    p->activation.emplace();p->activation->profile_kind=p->profile.kind;p->activation->rules_database_ready=true;
    p->rules=std::make_unique<content::RulesDatabase>();
    p->rules->apply_sql(R"sql(
CREATE TABLE Worlds(ID INTEGER,Type TEXT);INSERT INTO Worlds VALUES(5,'WORLDSIZE_STANDARD');
CREATE TABLE Climates(ID INTEGER,Type TEXT);INSERT INTO Climates VALUES(6,'CLIMATE_TEMPERATE');
CREATE TABLE SeaLevels(ID INTEGER,Type TEXT);INSERT INTO SeaLevels VALUES(7,'SEALEVEL_MEDIUM');
CREATE TABLE GameSpeeds(ID INTEGER,Type TEXT);INSERT INTO GameSpeeds VALUES(2,'GAMESPEED_STANDARD');
CREATE TABLE Eras(ID INTEGER,Type TEXT);INSERT INTO Eras VALUES(4,'ERA_ANCIENT');
CREATE TABLE HandicapInfos(ID INTEGER,Type TEXT);INSERT INTO HandicapInfos VALUES(7,'HANDICAP_PRINCE');
CREATE TABLE Units(ID INTEGER);INSERT INTO Units VALUES(2),(2),(5);
CREATE TABLE UnitClasses(ID INTEGER);INSERT INTO UnitClasses VALUES(1);
CREATE TABLE BuildingClasses(ID INTEGER);INSERT INTO BuildingClasses VALUES(3);
CREATE TABLE Projects(ID INTEGER);INSERT INTO Projects VALUES(2);
CREATE TABLE Votes(ID INTEGER);INSERT INTO Votes VALUES(1);
CREATE TABLE VoteSources(ID INTEGER);INSERT INTO VoteSources VALUES(2);
CREATE TABLE SpecialUnits(ID INTEGER);INSERT INTO SpecialUnits VALUES(1);
CREATE TABLE Victories(ID INTEGER);INSERT INTO Victories VALUES(1);
CREATE TABLE Defines(Name TEXT,Value INTEGER);INSERT INTO Defines VALUES('NUM_VICTORY_POINT_AWARDS',3);
INSERT INTO Defines VALUES('CITY_RING_1_MULTIPLIER',12),('CITY_RING_2_MULTIPLIER',-8),
 ('CITY_RING_3_MULTIPLIER',6),('CITY_RING_4_MULTIPLIER',1),('CITY_RING_5_MULTIPLIER',2);
CREATE TABLE Flavors(ID INTEGER,Type TEXT);INSERT INTO Flavors VALUES
 (7,'FLAVOR_GROWTH'),(2,'FLAVOR_EXPANSION'),(19,'FLAVOR_NAVAL');
)sql");
    if(!extra.empty()) p->rules->apply_sql(extra);
    p->rules->seal_immutable_preview_fingerprints();return p;
}
std::int32_t scalar(const runtime::BnwFreshGameObservation& x,const char* name) {
    return std::get<std::int32_t>(x.scalars.at(name));
}
void verify_database(const std::filesystem::path& path,bool require_empty=false) {
    sqlite3* db=nullptr;const auto u=path.u8string();
    need(sqlite3_open_v2(reinterpret_cast<const char*>(u.c_str()),&db,SQLITE_OPEN_READONLY,nullptr)==SQLITE_OK,
         "Actual private cache DB missing");
    struct Close {sqlite3* p;~Close(){sqlite3_close(p);}} close{db};
    sqlite3_stmt* statement=nullptr;
    need(sqlite3_prepare_v2(db,"SELECT sql FROM sqlite_master WHERE name='SimpleValues'",-1,&statement,nullptr)==SQLITE_OK,
         "Actual cache schema query missing");
    struct Finalize {sqlite3_stmt* p;~Finalize(){sqlite3_finalize(p);}} finalize{statement};
    need(sqlite3_step(statement)==SQLITE_ROW,"SimpleValues not actually created");
    const auto* value=sqlite3_column_text(statement,0);
    need(value && std::string(reinterpret_cast<const char*>(value))==
         "CREATE TABLE SimpleValues(Name TEXT Primary Key, Value VARIANT)","Native cache SQL differs");
    need(sqlite3_step(statement)==SQLITE_DONE,"Unexpected duplicate SimpleValues");
    if(require_empty) {
        sqlite3_stmt* count=nullptr;
        need(sqlite3_prepare_v2(db,"SELECT count(*) FROM SimpleValues",-1,&count,nullptr)==SQLITE_OK,"Cache row-count query");
        Finalize final_count{count};
        need(sqlite3_step(count)==SQLITE_ROW && sqlite3_column_int(count,0)==0,
             "Second reset must delete actual constructor cache rows");
    }
}

void owned_pregame_cases(const std::filesystem::path& selected_config_ini) {
    using G=runtime::BnwFreshGame;using E=runtime::BnwFreshGameError;using B=runtime::BnwFreshGameBoundary;
    auto p=fixture(R"sql(
CREATE TABLE PostDefines(Name TEXT,Key TEXT,"Table" TEXT);
INSERT INTO Defines VALUES('STANDARD_HANDICAP',9),('MULTIPLAYER_HANDICAP',10),('STANDARD_GAMESPEED',2),('STANDARD_ERA',0),('AI_HANDICAP',11);
ALTER TABLE Victories ADD COLUMN Type TEXT;UPDATE Victories SET Type='VICTORY_A';
CREATE TABLE GameOptions(Type TEXT,"Default" INTEGER);INSERT INTO GameOptions VALUES('GAMEOPTION_QUICK_COMBAT',-2);
)sql");
    auto c=runtime::BnwCreationConfig::from_selected_rules(p,choices());auto g=G::from_creation_config(c);
    const auto before=g->observe();const auto hash=p->rules->gameplay_fingerprint_sha256();
    refuses([&]{(void)g->pregame_setup_observation();},E::wrong_phase);
    refuses([&]{g->advance_single_player_pregame_menu_options_prefix();},E::wrong_phase);
    auto copy=*g;refuses([&]{copy.advance_single_player_pregame_menu_prefix(selected_config_ini);},E::parent_retired);
    bool denied=false;std::thread worker([&]{try{g->advance_single_player_pregame_menu_prefix(selected_config_ini);}
        catch(const runtime::BnwFreshGameRefusal& e){denied=e.code()==E::wrong_thread;}});worker.join();
    need(denied && g->observe().order==before.order,"Actual Game thread and original facade checks precede PG allocation/SQL");
    refuses([&]{g->advance_single_player_pregame_menu_prefix(selected_config_ini);},E::native_rules_unavailable);
    const auto menu=g->pregame_setup_observation();const auto stage=g->observe();
    need(stage.boundary==B::pregame_menu_options_required && menu.menu.boundary==runtime::BnwFreshPregameMenuBoundary::reset_game_options_required &&
         menu.cached_globals.standard_handicap==9 && menu.game_turn==-1 && !menu.init_inputs.attempted &&
         stage.cache_resets==before.cache_resets && stage.map_random==before.map_random && stage.gameplay_random==before.gameplay_random,
         "Actual retained PG cached SQL and menu stores preserve Game reset/RNG and mandatory options frontier");
    refuses([&]{g->advance_reset_prefix();},E::native_rules_unavailable);
    refuses([&]{copy.advance_single_player_pregame_menu_options_prefix();},E::parent_retired);
    refuses([&]{g->advance_single_player_pregame_menu_prefix(selected_config_ini);},E::wrong_phase);
    refuses([&]{g->advance_single_player_pregame_menu_options_prefix();},E::native_rules_unavailable);
    const auto options=g->pregame_setup_observation();const auto end=g->observe();
    refuses([&]{g->advance_pregame_settings_last_civilization_prefix({});},E::parent_missing);
    refuses([&]{copy.advance_pregame_settings_last_civilization_prefix({});},E::parent_retired);
    need(end.boundary==B::pregame_settings_source_required && options.menu.boundary==runtime::BnwFreshPregameMenuBoundary::load_pregame_settings_required &&
         options.reset.victory_count==2 && options.reset.victories==std::vector<bool>({true,true}) && options.menu.map_options.empty() &&
         options.options.quick_combat && options.options.enum_values[15]==-2 && !options.options.complete &&
         end.cache_resets==before.cache_resets && end.map_random==before.map_random && end.gameplay_random==before.gameplay_random &&
         p->rules->gameplay_fingerprint_sha256()==hash,
         "SAME retained PG authentic sparse native victories/enum options source without settings/Game/newgame readiness");
    refuses([&]{g->advance_reset_prefix();},E::native_rules_unavailable);
    refuses([&]{g->advance_init_prefix({1,2,runtime::BnwFreshGameType::single_player});},E::wrong_phase);
    refuses([&]{g->advance_single_player_pregame_menu_options_prefix();},E::wrong_phase);
    need(g->observe().order==end.order && g->pregame_setup_observation().order==options.order,
         "Mandatory setup refusal does not replay PG stores, replace realm or mutate Game cache/RNG");
    auto failing=G::from_creation_config(runtime::BnwCreationConfig::from_selected_rules(fixture(),choices()));
    const auto before_failure=failing->observe();bool failed=false;
    try{failing->advance_single_player_pregame_menu_prefix(selected_config_ini);}
    catch(const runtime::BnwFreshPregameSpeedRefusal& e){failed=e.code()==runtime::BnwFreshPregameSpeedError::native_source_unavailable;}
    need(failed && failing->observe().boundary==B::failed_prefix &&
         failing->pregame_setup_observation().cached_globals.boundary==runtime::BnwFreshPregameCachedGlobalsBoundary::failed_prefix,
         "Actual PostDefines failure preserves issued PG and Game failure rather than options-ready frontier");
    refuses([&]{failing->advance_single_player_pregame_menu_options_prefix();},E::wrong_phase);
    refuses([&]{failing->advance_reset_prefix();},E::native_rules_unavailable);
    need(failing->observe().cache_resets==before_failure.cache_resets && failing->observe().map_random==before_failure.map_random,
         "Failed native setup cannot replay Game reset or draw RNG");
}

void cases(const std::filesystem::path& selected_config_ini) {
    using G=runtime::BnwFreshGame;using E=runtime::BnwFreshGameError;using B=runtime::BnwFreshGameBoundary;
    refuses([]{(void)G::from_creation_config({});},E::parent_missing);
    auto p=fixture();auto c=runtime::BnwCreationConfig::from_selected_rules(p,choices());auto g=G::from_creation_config(c);
    const auto before=g->observe();
    need(before.boundary==B::constructor_reset_complete && before.cache_resets==1 && scalar(before,"m_eHandicap")==-1,
         "Constructor reset/order/NO_HANDICAP");
    need(before.map_random==runtime::SimulationRandomState{0,0,2} &&
         before.gameplay_random==runtime::SimulationRandomState{0,0,1},"Actual CvRandom constructors/reset counters");
    need(!std::get<bool>(before.scalars.at("m_mapRand.m_bSynchronous")) &&
         std::get<bool>(before.scalars.at("m_jonRand.m_bSynchronous")),"Distinct RNG constructor synchronous fields");
    need(!before.arrays.at("m_paiUnitCreatedCount").slots && scalar(before,"m_iNumSessions")==1,
         "Constructor must not allocate nonconstructor arrays");
    need(!before.scalars.contains("m_eEventPlayer") && !before.scalars.contains("m_bDynamicTurnsSimultMode"),
         "Unwritten native fields must not gain inferred defaults");
    verify_database(before.local_cache_database);
    // Actual cache writes are discarded by the SECOND reset, not reused.
    sqlite3* raw=nullptr;const auto path=before.local_cache_database.u8string();
    need(sqlite3_open(reinterpret_cast<const char*>(path.c_str()),&raw)==SQLITE_OK,"Cache witness open");
    const int insert=sqlite3_exec(raw,"INSERT INTO SimpleValues VALUES('old',17)",nullptr,nullptr,nullptr);
    sqlite3_close(raw);need(insert==SQLITE_OK,"Cache beforeimage witness insert");
    g->advance_reset_prefix();const auto after=g->observe();verify_database(after.local_cache_database,true);
    need(after.cache_resets==2 && after.boundary==B::settler_site_init_required && scalar(after,"m_eHandicap")==7,
         "Nonconstructor reset must reach exact Site Init boundary");
    const auto& unit=*after.arrays.at("m_paiUnitCreatedCount").slots;
    need(unit.size()==6 && !unit[0] && !unit[1] && unit[2]==0 && unit[3]==0 && !unit[4] && unit[5]==0,
         "Native overlapping-ID append plus null-hole unwritten semantics");
    const auto& project=*after.arrays.at("m_paiProjectCreatedCount").slots;
    need(project.size()==3 && project[0]==0 && project[1]==0 && project[2]==0,"Projects write even Info holes");
    need(after.arrays.at("m_aiVotesCast").slots->size()==63 && after.arrays.at("m_aiVotesCast").slots->at(62)==-1,
         "MAX_CIV_TEAMS physical63 differs MAX_TEAMS64");
    need(after.arrays.at("m_pabSpecialUnitValid").bool_slots->size()==2 &&
         !after.arrays.at("m_pabSpecialUnitValid").bool_slots->at(0),"SpecialUnits all slots native boolfalse");
    need(after.team_victory_rank && *after.team_victory_rank==std::vector<std::vector<std::int32_t>>(2,std::vector<std::int32_t>(3,-1)),
         "Victory holes get all award slots NO_TEAM");
    need(after.map_random==before.map_random && after.gameplay_random==before.gameplay_random &&
         scalar(after,"Settler.m_iExpansionIndex")==12 && scalar(after,"Settler.m_iGrowthIndex")==13,
         "Unreached reset tail RNG and actual Settler constructor");
    const auto& uninitialized=*after.arrays.at("Settler.m_iRingModifier").slots;
    need(uninitialized.size()==8 && std::all_of(uninitialized.begin(),uninitialized.end(),[](const auto& v){return !v;}),
         "Settler constructor must not initialize native inline ring fields");
    g->advance_settler_site_init();const auto initialized=g->observe();
    need(initialized.boundary==B::start_site_construction_required && initialized.cache_resets==2 &&
         initialized.map_random==after.map_random && initialized.gameplay_random==after.gameplay_random,
         "Site Init must not repeat cache/reset or execute RNG tail");
    const auto& ring=*initialized.arrays.at("Settler.m_iRingModifier").slots;
    need(ring==std::vector<std::optional<std::int32_t>>{1,12,-8,6,1,2,0,0},"Native eight ring stores, signed cached defines");
    need(scalar(initialized,"Settler.m_iGrowthIndex")==7 && scalar(initialized,"Settler.m_iExpansionIndex")==2 &&
         scalar(initialized,"Settler.m_iNavalIndex")==19,"Actual global Type IDs, not Prefetch positions/counts");
    need(scalar(initialized,"Settler.m_iBrazilMultiplier")==1000 && scalar(initialized,"Settler.m_iSpainMultiplier")==55000 &&
         scalar(initialized,"Settler.m_iMorrocoMultiplier")==1000 && scalar(initialized,"Settler.m_iNetherlandsMultiplier")==2000 &&
         scalar(initialized,"Settler.m_iIncaMultiplier")==500,"Native multiplier stores");
    const auto& flavor=*initialized.arrays.at("Settler.m_iFlavorMultiplier").slots;
    need(flavor.size()==9 && std::all_of(flavor.begin(),flavor.end(),[](const auto& v){return !v;}),
         "Init does not execute ComputeFlavorMultipliers or infer defaults");
    refuses([&]{g->advance_settler_site_init();},E::wrong_phase);
    bool foreign_site=false;
    std::thread site_thread([&]{try{g->advance_settler_site_init();}
        catch(const runtime::BnwFreshGameRefusal& e){foreign_site=e.code()==E::wrong_thread;}});
    site_thread.join();need(foreign_site && g->observe().cache_resets==2,
         "Site Init must reject foreign thread before phase/parent/DB effects");
    auto early=G::from_creation_config(c);refuses([&]{early->advance_settler_site_init();},E::wrong_phase);
    refuses([&]{early->advance_start_site_init();},E::wrong_phase);
    auto prerequisite=G::from_creation_config(c);prerequisite->advance_reset_prefix();
    refuses([&]{prerequisite->advance_start_site_init();},E::wrong_phase);
    prerequisite.reset(); // Do not retain SAME c/p across sole-parent checks.
    need(!initialized.arrays.contains("Start.m_iRingModifier"),"Start not allocated before its source call");
    g->advance_start_site_init();const auto start_state=g->observe();
    need(start_state.boundary==B::start_positioner_construction_required && start_state.cache_resets==2 &&
         start_state.map_random==initialized.map_random && start_state.gameplay_random==initialized.gameplay_random,
         "Start Init must not repeat reset/cache or execute later RNG tail");
    need(start_state.arrays.at("Start.m_iRingModifier").slots==initialized.arrays.at("Settler.m_iRingModifier").slots &&
         start_state.arrays.at("Settler.m_iRingModifier").slots==initialized.arrays.at("Settler.m_iRingModifier").slots,
         "Two actual owned evaluator arrays; Start cannot mutate already initialized Settler");
    for(const auto field:{"m_iGrowthIndex","m_iExpansionIndex","m_iNavalIndex","m_iBrazilMultiplier",
        "m_iSpainMultiplier","m_iMorrocoMultiplier","m_iNetherlandsMultiplier","m_iIncaMultiplier"})
        need(scalar(start_state,(std::string("Start.")+field).c_str())==
             scalar(initialized,(std::string("Settler.")+field).c_str()),"Start inherited native field value differs");
    const auto& start_flavors=*start_state.arrays.at("Start.m_iFlavorMultiplier").slots;
    need(start_flavors.size()==9 && std::all_of(start_flavors.begin(),start_flavors.end(),[](const auto& value){return !value;}),
         "Start Init must not prematurely execute StartPositioner ComputeFlavorMultipliers");
    const auto start_constructor=std::find(start_state.order.begin(),start_state.order.end(),"CvSiteEvaluatorForStart constructor");
    need(start_constructor!=start_state.order.end() && std::find(start_constructor,start_state.order.end(),"Start.m_iRingModifier[0]")!=start_state.order.end(),
         "Native Start constructor must precede inherited Init");
    refuses([&]{g->advance_start_site_init();},E::wrong_phase);
    bool foreign_start=false;std::thread start_thread([&]{try{g->advance_start_site_init();}
        catch(const runtime::BnwFreshGameRefusal& e){foreign_start=e.code()==E::wrong_thread;}});
    start_thread.join();need(foreign_start,"Start must check constructor thread before phase/DB reads");
    refuses([&]{early->advance_start_positioner_init();},E::wrong_phase);
    need(!start_state.scalars.contains("StartPositioner.m_iRequiredSeparation"),"StartPositioner not constructed early");
    g->advance_start_positioner_init();const auto positioned=g->observe();
    need(positioned.boundary==B::game_deals_init_required && scalar(positioned,"StartPositioner.m_iRequiredSeparation")==0 &&
         scalar(positioned,"StartPositioner.m_iBestFoundValueOnMap")==0,"Native positioner constructor stores");
    const auto& neutral=*positioned.arrays.at("Start.m_iFlavorMultiplier").slots;
    need(neutral.size()==9 && std::all_of(neutral.begin(),neutral.end(),[](const auto& v){return v==1;}),
         "Actual pointer Init must execute all9 Start flavor writes");
    need(positioned.arrays.at("Settler.m_iFlavorMultiplier").slots==start_state.arrays.at("Settler.m_iFlavorMultiplier").slots &&
         positioned.arrays.at("Start.m_iRingModifier").slots==start_state.arrays.at("Start.m_iRingModifier").slots,
         "Positioner must affect only SAME Start flavor array; other owned fields unchanged");
    need(scalar(positioned,"StartPositioner.m_ContinentVector.size")==0 && scalar(positioned,"StartPositioner.m_StartRegionVector.size")==0 &&
         scalar(positioned,"StartPositioner.m_PlayerOrder.size")==0 && !positioned.scalars.contains("StartPositioner.m_PlayerOrderIter"),
         "Native vectors default empty; gameplay iterator remains unwritten");
    const auto binding=std::find(positioned.order.begin(),positioned.order.end(),"StartPositioner.m_pSiteEvaluator <- SAME owned Start");
    const auto first_flavor=std::find(positioned.order.begin(),positioned.order.end(),"Start.m_iFlavorMultiplier[0]");
    const auto last_flavor=std::find(positioned.order.begin(),positioned.order.end(),"Start.m_iFlavorMultiplier[8]");
    need(binding<first_flavor && first_flavor<last_flavor && last_flavor!=positioned.order.end(),
         "Source pointer binding precedes ascending0..8 flavor stores");
    need(positioned.cache_resets==start_state.cache_resets && positioned.map_random==start_state.map_random &&
         positioned.gameplay_random==start_state.gameplay_random,"Positioner must not execute reset/cache/RNG tail");
    refuses([&]{g->advance_start_positioner_init();},E::wrong_phase);
    bool foreign_positioner=false;std::thread positioner_thread([&]{try{g->advance_start_positioner_init();}
        catch(const runtime::BnwFreshGameRefusal& e){foreign_positioner=e.code()==E::wrong_thread;}});
    positioner_thread.join();need(foreign_positioner,"Positioner constructor-thread guard must precede state/DB access");


    refuses([&]{early->advance_deals_game_religions_prefix();},E::wrong_phase);
    bool foreign_religions=false;std::thread religions_thread([&]{try{g->advance_deals_game_religions_prefix();}
        catch(const runtime::BnwFreshGameRefusal& e){foreign_religions=e.code()==E::wrong_thread;}});
    religions_thread.join();need(foreign_religions,"Religions thread-first before phase/world/DB");
    g->advance_deals_game_religions_prefix();const auto religion_prefix=g->observe();
    need(religion_prefix.boundary==B::game_religions_speed_source_required &&
         scalar(religion_prefix,"GameReligions.m_iMinimumFaithForNextPantheon.current")==0 &&
         scalar(religion_prefix,"GameReligions.m_CurrentReligions.size")==0 &&
         scalar(religion_prefix,"GameReligions.m_CurrentReligions.capacity")==16,
         "Missing cached Define yields0; native inline container no constructed elements");
    need(scalar(religion_prefix,"GameDeals.m_ProposedDeals.size")==0 &&
         scalar(religion_prefix,"GameDeals.m_CurrentDeals.size")==0 &&
         scalar(religion_prefix,"GameDeals.m_HistoricalDeals.size")==0 &&
         std::get<std::uint32_t>(religion_prefix.scalars.at("GameDeals.m_uiDealCounter.current"))==0,
         "Actual existing Deals clear3 retains constructor counter");
    const auto ordered=[&](const char* name){return std::find(religion_prefix.order.begin(),religion_prefix.order.end(),name);};
    need(ordered("GameDeals.m_ProposedDeals.clear")<ordered("GameDeals.m_CurrentDeals.clear") &&
         ordered("GameDeals.m_CurrentDeals.clear")<ordered("GameDeals.m_HistoricalDeals.clear") &&
         ordered("GameDeals.m_HistoricalDeals.clear")<ordered("GameReligions.m_iMinimumFaithForNextPantheon.constructor") &&
         ordered("GameReligions.m_iMinimumFaithForNextPantheon.constructor")<ordered("GameReligions.m_iMinimumFaithForNextPantheon.InitDefine"),
         "Native Deals->Religions constructor->first Init store order");
    need(religion_prefix.arrays.at("Start.m_iFlavorMultiplier").slots==positioned.arrays.at("Start.m_iFlavorMultiplier").slots &&
         religion_prefix.arrays.at("Settler.m_iRingModifier").slots==positioned.arrays.at("Settler.m_iRingModifier").slots && religion_prefix.cache_resets==positioned.cache_resets &&
         religion_prefix.map_random==positioned.map_random && religion_prefix.gameplay_random==positioned.gameplay_random,
         "No Config GameSpeed substitution, evaluator/reset/RNG tail stores");
    refuses([&]{g->advance_deals_game_religions_prefix();},E::wrong_phase);
    {
        // Detached SQL variants remain arithmetic/source-order fixtures only.
        const auto advance=[](const std::shared_ptr<G>& value){value->advance_reset_prefix();
            value->advance_settler_site_init();value->advance_start_site_init();value->advance_start_positioner_init();};
        auto negative=G::from_creation_config(runtime::BnwCreationConfig::from_selected_rules(
            fixture("INSERT INTO Defines VALUES('RELIGION_MIN_FAITH_FIRST_PANTHEON',-17);"),choices()));
        refuses([&]{negative->advance_culture_leagues_trade_init();},E::wrong_phase);
        refuses([&]{negative->advance_tactical_advisors_reset_tail();},E::wrong_phase);
        refuses([&]{negative->advance_city_manager_reset();},E::wrong_phase);
        refuses([&]{negative->advance_init_prefix({0,0,runtime::BnwFreshGameType::single_player});},E::wrong_phase);
        advance(negative);negative->advance_deals_game_religions_prefix();
        need(scalar(negative->observe(),"GameReligions.m_iMinimumFaithForNextPantheon.current")==-17,
             "Reached native cached int preserves negative value; no eager percent read");
        auto bad=G::from_creation_config(runtime::BnwCreationConfig::from_selected_rules(
            fixture("INSERT INTO Defines VALUES('RELIGION_MIN_FAITH_FIRST_PANTHEON',2147483648);"),choices()));
        advance(bad);refuses([&]{bad->advance_deals_game_religions_prefix();},E::native_rules_unavailable);
        const auto partial=bad->observe();
        need(partial.boundary==B::failed_prefix && scalar(partial,"GameReligions.m_iMinimumFaithForNextPantheon.current")==0 &&
             !partial.scalars.contains("GameReligions.m_iMinimumFaithForNextPantheon.InitDefine") &&
             scalar(partial,"GameDeals.m_HistoricalDeals.size")==0,
             "Later Define refusal retains Deals and actual constructor stores only");
        refuses([&]{bad->advance_deals_game_religions_prefix();},E::wrong_phase);
    }

    {
        const auto make=[](const std::string& extra){auto value=G::from_creation_config(
            runtime::BnwCreationConfig::from_selected_rules(fixture(extra),choices()));
            value->advance_reset_prefix();value->advance_settler_site_init();value->advance_start_site_init();
            value->advance_start_positioner_init();value->advance_deals_game_religions_prefix();return value;};
        const std::string schema="ALTER TABLE GameSpeeds ADD COLUMN TrainPercent INTEGER;"
            "CREATE TABLE TurnTimers(ID INTEGER,Type TEXT,BaseTime INTEGER,CityResource INTEGER,UnitResource INTEGER,FirstTurnMultiplier INTEGER);"
            "INSERT INTO TurnTimers VALUES(3,'TURN_TIMER_MEDIUM',12,3,4,5);";
        auto numeric=make(schema+"UPDATE GameSpeeds SET TrainPercent=150;INSERT INTO Defines VALUES('RELIGION_MIN_FAITH_FIRST_PANTHEON',-17);");
        numeric->advance_game_religions_speed_init(selected_config_ini);const auto done=numeric->observe();
        need(done.boundary==B::game_culture_construction_required && scalar(done,"Pregame.s_gameSpeed")==2 &&
             scalar(done,"Pregame.s_turnTimerType")==3 && scalar(done,"GameReligions.m_iMinimumFaithForNextPantheon.multiply")==-2550 &&
             scalar(done,"GameReligions.m_iMinimumFaithForNextPantheon.divide100")==-25,
             "Actual owned speed read then checked multiply and native negative truncation");
        refuses([&]{numeric->advance_game_religions_speed_init(selected_config_ini);},E::wrong_phase);
        bool foreign_services=false;std::thread service_thread([&]{try{numeric->advance_culture_leagues_trade_init();}
            catch(const runtime::BnwFreshGameRefusal& e){foreign_services=e.code()==E::wrong_thread;}});
        service_thread.join();need(foreign_services && numeric->observe().boundary==B::game_culture_construction_required,
                                  "Game services thread fence before stores");
        numeric->advance_culture_leagues_trade_init();const auto services=numeric->observe();
        need(services.boundary==B::tactical_map_construction_required &&
             !std::get<bool>(services.scalars.at("GameCulture.m_bReportedSomeoneInfluential")) &&
             scalar(services,"GameCulture.m_CurrentGreatWorks.size")==0 && scalar(services,"GameCulture.m_CurrentGreatWorks.capacity")==22 &&
             scalar(services,"GameLeagues.m_iNumLeaguesEverFounded")==0 && scalar(services,"GameLeagues.m_eDiplomaticVictor")==-1 &&
             scalar(services,"GameLeagues.m_iGeneratedIDCount")==0 && scalar(services,"GameLeagues.m_eLastEraTrigger")==-1 &&
             scalar(services,"GameLeagues.m_vActiveLeagues.size")==0 && scalar(services,"GameLeagues.m_vActiveLeagues.capacity")==2 &&
             scalar(services,"GameTrade.m_aTradeConnections.size")==0 && scalar(services,"GameTrade.m_aTradeConnections.capacity")==168,
             "Actual Culture and Leagues constructor stores and empty native Init");
        for(const auto* phase:{"GameTrade.constructor.Reset","GameTrade.Init.Reset"}) {
            need(scalar(services,(std::string(phase)+".m_iNextID").c_str())==0 && scalar(services,(std::string(phase)+".popup.iPlotX").c_str())==0 &&
                 scalar(services,(std::string(phase)+".popup.iPlotY").c_str())==0 && scalar(services,(std::string(phase)+".popup.type").c_str())==0,
                 "Actual Trade Reset5 executed twice, international enum0");
        }
        const auto& tech=*services.arrays.at("GameTrade.m_aaiTechDifference").slots;
        need(tech.size()==22*22 && std::none_of(tech.begin(),tech.end(),[](const auto& v){return v.has_value();}),
             "Trade constructor and Reset leave every tech-difference cell unwritten");
        const auto at=[&](const char* key){const auto i=std::find(services.order.begin(),services.order.end(),key);
            need(i!=services.order.end(),"Missing exact service order journal");return i-services.order.begin();};
        need(at("GameCulture.m_CurrentGreatWorks.clear")<at("GameCulture.m_bReportedSomeoneInfluential") &&
             at("GameCulture.m_bReportedSomeoneInfluential")<at("GameLeagues.m_vActiveLeagues.clear") &&
             at("GameLeagues.m_eLastEraTrigger")<at("GameLeagues.Init empty native body") &&
             at("GameLeagues.Init empty native body")<at("GameTrade.constructor.Reset.m_aTradeConnections.clear") &&
             at("GameTrade.constructor.Reset.popup.type")<at("GameTrade.Init.Reset.m_aTradeConnections.clear"),
             "Native consecutive service call/store order");
        need(services.map_random==done.map_random && services.gameplay_random==done.gameplay_random &&
             services.cache_resets==done.cache_resets && !services.scalars.contains("TacticalMap.initialized"),
             "Services do not execute reset-tail RNG/cache or Tactical map");
        refuses([&]{numeric->advance_culture_leagues_trade_init();},E::wrong_phase);
        refuses([&]{(void)numeric->tactical_analysis_map();},E::wrong_phase);
        bool foreign_tail=false;std::thread tail_thread([&]{try{numeric->advance_tactical_advisors_reset_tail();}
            catch(const runtime::BnwFreshGameRefusal& e){foreign_tail=e.code()==E::wrong_thread;}});
        tail_thread.join();need(foreign_tail && numeric->observe().boundary==B::tactical_map_construction_required,
                               "Tail thread guard before allocating actual children");
        numeric->advance_tactical_advisors_reset_tail();const auto tail=numeric->observe();
        auto tactical=numeric->tactical_analysis_map();auto counsel=numeric->advisor_counsel();auto recommender=numeric->advisor_recommender();
        auto realm=numeric->city_manager();const auto* realm_pointer=realm.get();
        need(realm->observe().city_entries==0 && realm->observe().empty_list_entries==0 &&
             scalar(tail,"CityManager.scopedResetCalls")==1 && scalar(tail,"CityManager.scopedShutdownCalls")==2,
             "Constructor Reset/nonconstructor Shutdown use same genuine pointer realm");
        need(tactical.get()==numeric->tactical_analysis_map().get() && counsel.get()==numeric->advisor_counsel().get(),
             "Actual getters return SAME Game-held allocations");
        const auto tm=tactical->observe();
        need(tm==runtime::BnwTacticalMapSnapshot{} && !tm.plots_allocated && tm.num_plots==0,
             "Real Tactical constructor only; no Init or invented map count");
        need(counsel->size()==50,"Actual Counsel constructor resize50");
        for(std::size_t i=0;i<counsel->size();++i)need(counsel->row(i).advisor==-1 && counsel->row(i).value==-1,
                                                   "Actual held Counsel Reset50 native sentinels");
        bool unwritten=false;try{(void)counsel->text_byte(0,0);}catch(const runtime::BnwFreshAdvisorRefusal& e){
            unwritten=e.code()==runtime::BnwFreshAdvisorError::field_unwritten;}
        need(unwritten,"Unwritten Counsel text is not fabricated");
        const auto rec=recommender->observe();
        for(std::size_t i=0;i<4;++i)need(rec.recommended_builds[i].type==0 && rec.recommended_builds[i].index==-1 &&
            rec.recommended_builds[i].turns==-1 && rec.recommended_techs[i]==-1 && rec.weighted_sizes[i]==0,
            "Actual Recommender member constructors then ordered resets");
        need(tail.boundary==B::city_manager_reset_required && scalar(tail,"m_iNumSessions")==1 &&
             scalar(tail,"reset-tail.advisorMessages.size")==0 && scalar(tail,"reset-tail.voteSelections.freeHead")==-2 &&
             scalar(tail,"reset-tail.votesTriggered.freeHead")==-2 && tail.cache_resets==services.cache_resets,
             "Native RemoveAll on NULL storage preserves metadata, session1/messages cleared, CityManager pending");
        need(tail.map_random==runtime::SimulationRandomState{0,services.map_random.calls,services.map_random.resets+1} &&
             tail.gameplay_random==runtime::SimulationRandomState{0,services.gameplay_random.calls,services.gameplay_random.resets+1},
             "Exactly one native reset on each actual retained RNG, no draw");
        const auto index=[&](const char* key){const auto i=std::find(tail.order.begin(),tail.order.end(),key);
            need(i!=tail.order.end(),"Tail order receipt missing");return i-tail.order.begin();};
        need(index("Game.new CvTacticalAnalysisMap constructor ONLY")<index("Game.new CvAdvisorCounsel") &&
             index("Game.new CvAdvisorCounsel")<index("Game.new CvAdvisorRecommender") &&
             index("Game.new CvAdvisorRecommender")<index("reset-tail m_voteSelections.RemoveAll") &&
             index("reset-tail m_voteSelections.RemoveAll")<index("reset-tail m_votesTriggered.RemoveAll") &&
             index("reset-tail m_votesTriggered.RemoveAll")<index("reset-tail m_mapRand.reset") &&
             index("reset-tail m_mapRand.reset")<index("reset-tail m_jonRand.reset") &&
             index("reset-tail m_jonRand.reset")<index("reset-tail m_AdvisorMessagesViewed.clear"),"Native actual allocation/reset-tail order");
        refuses([&]{numeric->advance_tactical_advisors_reset_tail();},E::wrong_phase);
        bool foreign_final=false;std::thread final_thread([&]{try{numeric->advance_city_manager_reset();}
            catch(const runtime::BnwFreshGameRefusal& e){foreign_final=e.code()==E::wrong_thread;}});
        final_thread.join();need(foreign_final && numeric->observe().boundary==B::city_manager_reset_required,
                                "Final Reset thread fence before cache mutation");
        numeric->advance_city_manager_reset();const auto complete=numeric->observe();
        need(complete.boundary==B::scoped_reset_complete_owned_cities_required && numeric->city_manager().get()==realm_pointer &&
             scalar(complete,"CityManager.scopedResetCalls")==2 && scalar(complete,"CityManager.scopedShutdownCalls")==2 &&
             complete.map_random==tail.map_random && complete.gameplay_random==tail.gameplay_random && complete.cache_resets==tail.cache_resets,
             "Final Reset acts on same realm without repeating previous tail/RNG/cache effects");
        refuses([&]{numeric->advance_city_manager_reset();},E::wrong_phase);
        bool foreign_init=false;std::thread init_thread([&]{try{numeric->advance_init_prefix({0,0,runtime::BnwFreshGameType::single_player});}
            catch(const runtime::BnwFreshGameRefusal& e){foreign_init=e.code()==E::wrong_thread;}});
        init_thread.join();need(foreign_init && numeric->observe().map_random==complete.map_random,"Init thread fence before setters/RNG");
        numeric->advance_init_prefix({4294967295ULL,0,runtime::BnwFreshGameType::single_player});const auto init=numeric->observe();
        need(init.boundary==B::init_mp_options_roster_required && init.map_random==runtime::SimulationRandomState{4294967295ULL%73637381ULL,0,complete.map_random.resets+1} &&
             init.gameplay_random==runtime::SimulationRandomState{0,0,complete.gameplay_random.resets+1} && init.cache_resets==complete.cache_resets &&
             scalar(init,"CityManager.scopedResetCalls")==2 && scalar(init,"Pregame.s_era")==4 && scalar(init,"Pregame.s_gameType")==0 &&
             std::get<bool>(init.scalars.at("m_jonRand.m_bSynchronous")),"Actual fresh max/zero residues; Init no draw/repeated reset/cache/synchronous store");
        for(const auto* name:{"voteSelections","votesTriggered"}) {
            const std::string prefix=name;need(scalar(init,(prefix+".slots").c_str())==8 && scalar(init,(prefix+".last").c_str())==-1 &&
                scalar(init,(prefix+".freeHead").c_str())==-1 && scalar(init,(prefix+".freeCount").c_str())==0 && scalar(init,(prefix+".currentID").c_str())==8192,
                "Native vote Init default8 and all five metadata fields");
            const auto& next=*init.arrays.at(prefix+".nextFree").slots;const auto& nulls=*init.arrays.at(prefix+".nullData").bool_slots;
            need(next.size()==8 && std::all_of(next.begin(),next.end(),[](const auto& v){return v==-1;}) && nulls.size()==8 &&
                 std::all_of(nulls.begin(),nulls.end(),[](bool v){return v;}),"Native all8 nodes initialized next=-1/dataNULL, no vote object");
        }
        const auto pos=[&](const char* name){return std::find(init.order.begin(),init.order.end(),name);};
        need(pos("init m_voteSelections.Init8")<pos("init m_votesTriggered.Init8") &&
             pos("init m_votesTriggered.Init8")<pos("init m_mapRand.init(seed % 73637381)") &&
             pos("init m_mapRand.init(seed % 73637381)")<pos("init m_jonRand.init(seed % 52319761)") &&
             pos("init m_jonRand.init(seed % 52319761)")<pos("init actual Pregame era verification complete"),"Actual Init native order");
        refuses([&]{numeric->advance_init_prefix({0,0,runtime::BnwFreshGameType::single_player});},E::wrong_phase);
        need(numeric->observe().map_random==init.map_random,"No replay after bounded Init prefix");
        bool options_thread=false;std::thread options_worker([&]{try{numeric->advance_single_player_options();}
            catch(const runtime::BnwFreshGameRefusal& e){options_thread=e.code()==E::wrong_thread;}});
        options_worker.join();need(options_thread && numeric->observe().boundary==B::init_mp_options_roster_required,
                                  "SP options thread fence before source/DB/attempt marker");
        numeric->advance_single_player_options();const auto options=numeric->observe();
        need(options.boundary==B::init_lock_mods_speed_required && options.map_random==init.map_random && options.gameplay_random==init.gameplay_random &&
            options.cache_resets==init.cache_resets && scalar(options,"Pregame.s_gameType")==0 && scalar(options,"Pregame.s_gameStartType")==0 &&
            std::get<bool>(options.scalars.at("Pregame.options.complete")),"Actual SP options complete without changing type/RNG/cache");
        const auto& mp=*options.arrays.at("Pregame.s_multiplayerOptions").bool_slots;
        const auto& enums=*options.arrays.at("Pregame.s_EnumBasedGameOptions").slots;
        need(mp.size()==4 && std::none_of(mp.begin(),mp.end(),[](bool v){return v;}) && enums.size()==22 &&
            std::all_of(enums.begin(),enums.end(),[](const auto& v){return v==0;}),"Native MP4 false; absent SQL default row leaves Sync initial0, all22 slots");
        const std::array<const char*,3> keys{"GAMEOPTION_SIMULTANEOUS_TURNS","GAMEOPTION_DYNAMIC_TURNS","GAMEOPTION_PITBOSS"};
        need(std::get<std::uint32_t>(options.scalars.at("Pregame.options.count"))==3 && std::get<std::uint32_t>(options.scalars.at("Pregame.options.hashCount"))==3,
             "Three real string CustomOption append/hash stores, not enum slots");
        for(std::size_t i=0;i<keys.size();++i)need(std::get<std::string>(options.scalars.at("Pregame.option["+std::to_string(i)+"].name"))==keys[i] &&
            scalar(options,("Pregame.option["+std::to_string(i)+"].value").c_str())==0,"Actual source three string setter order");
        const auto pg_order=numeric->pregame_order();
        const auto pgpos=[&](const std::string& key){return std::find(pg_order.begin(),pg_order.end(),key);};
        need(pgpos("setMultiplayerOption[0] false")<pgpos("setMultiplayerOption[3] false") &&
             pgpos("setMultiplayerOption[3] false")<pgpos("s_GameOptions.push_back GAMEOPTION_SIMULTANEOUS_TURNS") &&
             pgpos("s_GameOptions.push_back GAMEOPTION_SIMULTANEOUS_TURNS")<pgpos("s_GameOptions.push_back GAMEOPTION_DYNAMIC_TURNS") &&
             pgpos("s_GameOptions.push_back GAMEOPTION_DYNAMIC_TURNS")<pgpos("s_GameOptions.push_back GAMEOPTION_PITBOSS"),"Native MP calls then three option setters order");
        need(std::count(pg_order.begin(),pg_order.end(),"s_EnumBasedGameOptions[21]")==3 &&
             std::count(pg_order.begin(),pg_order.end(),"s_quickCombat <- enum15 nonzero")==3,
             "Each native string setter executes complete22-slot Sync and QuickCombat mirror");
        refuses([&]{numeric->advance_single_player_options();},E::wrong_phase);
        need(numeric->observe().map_random==options.map_random,"SP once does not replay RNG or prior setters");

        // Retained getter aliases own the SAME Game Impl, not borrowed pointers.
        numeric.reset();need(tactical->observe().num_plots==0 && counsel->size()==50,"Actual aliases retain Game allocation");
        tactical.reset();counsel.reset();recommender.reset();
        need(realm->observe().city_entries==0,"Sole realm alias retains actual Game allocation after other aliases drop");realm.reset();


        const auto ready=[&]{auto v=make(schema);v->advance_game_religions_speed_init(selected_config_ini);
            v->advance_culture_leagues_trade_init();v->advance_tactical_advisors_reset_tail();v->advance_city_manager_reset();return v;};
        for(const auto request:{runtime::BnwFreshPregameInitRequest{0,4294967295ULL,runtime::BnwFreshGameType::network},
                               runtime::BnwFreshPregameInitRequest{0,0,runtime::BnwFreshGameType::hotseat},
                               runtime::BnwFreshPregameInitRequest{17,23,runtime::BnwFreshGameType::email}}) {
            auto v=ready();const auto initial=v->observe();v->advance_init_prefix(request);const auto x=v->observe();
            need(x.map_random==runtime::SimulationRandomState{request.map_seed%73637381ULL,0,initial.map_random.resets+1} &&
                 x.gameplay_random==runtime::SimulationRandomState{request.sync_seed%52319761ULL,0,initial.gameplay_random.resets+1} &&
                 std::find(x.order.begin(),x.order.end(),"init jonRand.setCallStackDebuggingEnabled(false) Release empty body")==x.order.end(),
                 "All true multiplayer native types skip instrumentation call; exact seed residues/no draws");
            refuses([&]{v->advance_single_player_options();},E::native_rules_unavailable);
            need(v->observe().map_random==x.map_random && v->observe().boundary==B::init_mp_options_roster_required,
                 "Actual non-SP type cannot authorize SP stores or discard pending branch");
        }
        {
            auto v=ready();const auto initial=v->observe();bool seed_failed=false;
            try{v->advance_init_prefix({4294967296ULL,31,runtime::BnwFreshGameType::single_player});}
            catch(const runtime::BnwFreshPregameSpeedRefusal& e){seed_failed=e.code()==runtime::BnwFreshPregameSpeedError::native_source_unavailable;}
            const auto x=v->observe();need(seed_failed && x.boundary==B::failed_prefix && x.map_random==initial.map_random &&
                x.gameplay_random==initial.gameplay_random && scalar(x,"voteSelections.slots")==0 &&
                std::get<std::uint32_t>(x.scalars.at("Pregame.s_syncRandomSeed"))==31,"Actual failed setter prefix before vote/RNG stores");
            refuses([&]{v->advance_init_prefix({0,0,runtime::BnwFreshGameType::single_player});},E::wrong_phase);
        }
        const auto ready_options=[&](const std::string& sql){auto v=make(schema+sql);v->advance_game_religions_speed_init(selected_config_ini);
            v->advance_culture_leagues_trade_init();v->advance_tactical_advisors_reset_tail();v->advance_city_manager_reset();
            v->advance_init_prefix({17,23,runtime::BnwFreshGameType::single_player});return v;};
        {
            auto v=ready_options("CREATE TABLE GameOptions(ID INTEGER,Type TEXT,\"Default\" INTEGER);"
                "INSERT INTO GameOptions VALUES(99,'GAMEOPTION_NO_CITY_RAZING',2),(0,'GAMEOPTION_NO_CITY_RAZING',9),"
                "(7,'GAMEOPTION_QUICK_COMBAT',-1),(16,'GAMEOPTION_DISABLE_START_BIAS',1),(123,'GAMEOPTION_LOCK_MODS',1),"
                "(8,'GAMEOPTION_SIMULTANEOUS_TURNS',1),(9,'GAMEOPTION_DYNAMIC_TURNS',1),(10,'GAMEOPTION_PITBOSS',1);");
            const auto initial=v->observe();v->advance_single_player_options();const auto x=v->observe();
            const auto& cache=*x.arrays.at("Pregame.s_EnumBasedGameOptions").slots;
            need(cache[0]==2 && cache[8]==1 && cache[15]==-1 && cache[16]==0 && std::get<bool>(x.scalars.at("Pregame.s_quickCombat")) &&
                 x.map_random==initial.map_random && x.gameplay_random==initial.gameplay_random && x.boundary==B::init_lock_mods_speed_required,
                 "Native LIMIT1 first row independent Info/enum IDs; QuickCombat nonzero; omitted conversion16=0; no LOCK_MODS password draws");
            need(scalar(x,"Pregame.option[0].value")==0 && scalar(x,"Pregame.option[1].value")==0 && scalar(x,"Pregame.option[2].value")==0,
                 "Actual three source option stores override selected DB defaults1");
        }
        {
            auto v=ready_options("CREATE TABLE GameOptions(Type TEXT,\"Default\" INTEGER);"
                "INSERT INTO GameOptions VALUES('GAMEOPTION_NO_CITY_RAZING',3),('GAMEOPTION_NO_BARBARIANS',2147483648);");
            const auto initial=v->observe();bool failed=false;try{v->advance_single_player_options();}
            catch(const runtime::BnwFreshPregameSpeedRefusal& e){failed=e.code()==runtime::BnwFreshPregameSpeedError::native_source_unavailable;}
            const auto x=v->observe();const auto& cache=*x.arrays.at("Pregame.s_EnumBasedGameOptions").slots;
            need(failed && x.boundary==B::failed_prefix && cache[0]==3 && cache[1]==0 &&
                 std::get<std::uint32_t>(x.scalars.at("Pregame.options.count"))==1 && !std::get<bool>(x.scalars.at("Pregame.options.complete")) &&
                 x.map_random==initial.map_random && x.gameplay_random==initial.gameplay_random,
                 "Reached bounded GetInt32 refusal preserves MP calls, first raw option/hash and prior enum store; no future Dynamic/Pitboss stores");
            refuses([&]{v->advance_single_player_options();},E::wrong_phase);
        }
        const auto ready_turns=[&](const std::string& sql,std::uint64_t seed=23ULL){
            auto v=make(schema+sql);v->advance_game_religions_speed_init(selected_config_ini);
            v->advance_culture_leagues_trade_init();v->advance_tactical_advisors_reset_tail();v->advance_city_manager_reset();
            v->advance_init_prefix({17,seed,runtime::BnwFreshGameType::single_player});v->advance_single_player_options();return v;
        };
        const std::string turns_schema="CREATE TABLE GameSpeed_Turns(ID INTEGER,GameSpeedType TEXT,MonthIncrement INTEGER,TurnsPerIncrement INTEGER);";
        const std::string turns=turns_schema+"INSERT INTO GameSpeed_Turns VALUES(9,'GAMESPEED_STANDARD',120,3),(0,'GAMESPEED_STANDARD',60,4);";
        {
            auto v=ready_turns(turns+"CREATE TABLE GameOptions(Type TEXT,\"Default\" INTEGER);INSERT INTO GameOptions VALUES('GAMEOPTION_LOCK_MODS',2);");
            const auto before_turns=v->observe();
            bool wrong_thread=false;std::thread worker([&]{try{v->advance_constructor_scoped_lock_mods_turn_prefix();}
                catch(const runtime::BnwFreshGameRefusal& e){wrong_thread=e.code()==E::wrong_thread;}});
            worker.join();need(wrong_thread && v->observe().boundary==B::init_lock_mods_speed_required,"Turn thread fence before metadata/DB/effects");
            v->advance_constructor_scoped_lock_mods_turn_prefix();const auto x=v->observe();
            need(x.boundary==B::constructor_scoped_init_victory_setup_required && scalar(x,"Pregame.s_gameTurn")==-1 &&
                scalar(x,"m_iStartTurn")==-1 && scalar(x,"m_iDefaultEstimateEndTurn")==7 &&
                x.gameplay_random==before_turns.gameplay_random && x.map_random==before_turns.map_random &&
                std::get<std::string>(x.scalars.at("Pregame.s_adminPassword")).empty(),
                "LOCK_MODS2 is false; constructor gameTurn-1 remains real, no resetGame0/RNG draw/startPercent");
            need(!std::get<bool>(x.scalars.at("Globals.constructor.m_bLogging")) && scalar(x,"Globals.constructor.m_iRandLogging")==0,
                 "Actual issued scoped Globals constructor fields, no caller input");
            need(scalar(x,"m_iEstimateEndTurn")==0 && x.cache_resets==before_turns.cache_resets,
                 "Next maxTurns/victory/setup stage still unavailable, no future store");
            refuses([&]{v->advance_constructor_scoped_lock_mods_turn_prefix();},E::wrong_phase);
        }
        for(const std::uint64_t seed:{23ULL,66ULL}) {
            auto v=ready_turns(turns+"CREATE TABLE GameOptions(Type TEXT,\"Default\" INTEGER);INSERT INTO GameOptions VALUES('GAMEOPTION_LOCK_MODS',1);",seed);
            const auto before_password=v->observe();v->advance_constructor_scoped_lock_mods_turn_prefix();const auto x=v->observe();
            const std::string expected=seed==23ULL?std::string("t(\x19\x1c\x1c\x65\x18",7):std::string("z",1);
            const auto final_seed=seed==23ULL?813160558ULL:1114425925ULL;
            need(x.gameplay_random.seed==final_seed && x.gameplay_random.calls==before_password.gameplay_random.calls+7 &&
                 x.gameplay_random.resets==before_password.gameplay_random.resets && x.map_random==before_password.map_random &&
                 std::get<std::string>(x.scalars.at("Pregame.s_adminPassword"))==expected &&
                 scalar(x,"m_iDefaultEstimateEndTurn")==7 && x.boundary==B::constructor_scoped_init_victory_setup_required,
                 "Native golden seven password draws including embedded NUL still draw all7; actual C-string setter");
            const auto position=[&](const std::string& name){const auto i=std::find(x.order.begin(),x.order.end(),name);need(i!=x.order.end(),"Missing password source-order event");return i-x.order.begin();};
            need(position("init password RNG call counter[0]")<position("init password GC.getLogging actualfalse[0]") &&
                 position("init password GC.getLogging actualfalse[0]")<position("init password RNG seed then char[0]") &&
                 position("init password RNG seed then char[6]")<position("init CvPreGame.setAdminPassword actual C-string") &&
                 position("init CvPreGame.setAdminPassword actual C-string")<position("init actual GameSpeedInfo turn cache acquired"),
                 "Native counter before logging, seed after logging, all7 before setter and turn getter");
            refuses([&]{v->advance_constructor_scoped_lock_mods_turn_prefix();},E::wrong_phase);
            need(v->observe().gameplay_random==x.gameplay_random,"No password draw replay");
        }
        {
            auto v=ready_turns(turns_schema+"INSERT INTO GameSpeed_Turns VALUES(9,'GAMESPEED_STANDARD',120,2147483647),"
                "(0,'GAMESPEED_STANDARD',60,1),(1,'GAMESPEED_STANDARD',30,-1);");
            refuses([&]{v->advance_constructor_scoped_lock_mods_turn_prefix();},E::native_rules_unavailable);const auto x=v->observe();
            need(x.boundary==B::failed_prefix && scalar(x,"m_iStartTurn")==-1 && scalar(x,"m_iDefaultEstimateEndTurn")==0 &&
                 std::find(x.order.begin(),x.order.end(),"init estimate += turn increment[0]")!=x.order.end() &&
                 std::find(x.order.begin(),x.order.end(),"init estimate += turn increment[1]")==x.order.end(),
                 "Native SQL order before cancellation; reached first scalar store survives later addition overflow");
            refuses([&]{v->advance_constructor_scoped_lock_mods_turn_prefix();},E::wrong_phase);
        }
        {
            auto v=ready_turns(turns_schema+"INSERT INTO GameSpeed_Turns VALUES(9,'GAMESPEED_STANDARD',1,-2147483648),(0,'GAMESPEED_STANDARD',1,-1);");
            refuses([&]{v->advance_constructor_scoped_lock_mods_turn_prefix();},E::native_rules_unavailable);
            need(scalar(v->observe(),"m_iStartTurn")==-1 && scalar(v->observe(),"m_iDefaultEstimateEndTurn")==0,"Negative native32 overflow cannot wrap");
        }
        {
            auto v=ready_turns(turns_schema);v->advance_constructor_scoped_lock_mods_turn_prefix();
            need(scalar(v->observe(),"m_iDefaultEstimateEndTurn")==0 && v->observe().boundary==B::constructor_scoped_init_victory_setup_required,
                 "Actual count0 has real allocated zero-length turn array");
            auto missing=ready_turns("");missing->advance_constructor_scoped_lock_mods_turn_prefix();
            need(scalar(missing->observe(),"m_iDefaultEstimateEndTurn")==0,"Original CacheResults Execute false leaves constructor count0, no guessed slots");
        }
        {
            auto bad=ready_turns(turns_schema+"INSERT INTO GameSpeed_Turns VALUES(1,'GAMESPEED_STANDARD',2147483648,1);"
                "CREATE TABLE GameOptions(Type TEXT,\"Default\" INTEGER);INSERT INTO GameOptions VALUES('GAMEOPTION_LOCK_MODS',1);");
            bool failed=false;try{bad->advance_constructor_scoped_lock_mods_turn_prefix();}catch(const runtime::BnwFreshPregameSpeedRefusal& e){failed=e.code()==runtime::BnwFreshPregameSpeedError::native_source_unavailable;}
            const auto x=bad->observe();need(failed && x.boundary==B::failed_prefix && x.gameplay_random.calls==7 &&
                std::get<std::string>(x.scalars.at("Pregame.s_adminPassword"))==std::string("t(\x19\x1c\x1c\x65\x18",7) && scalar(x,"m_iStartTurn")==0,
                "Reached cached GetInt width refusal follows complete password, precedes Game StartTurn store");
            need(scalar(x,"GameSpeedInfo.m_iNumTurnIncrements")==1 && x.arrays.at("GameSpeedInfo.turn.MonthIncrement").slots->size()==1 &&
                 !x.arrays.at("GameSpeedInfo.turn.MonthIncrement").slots->at(0) && !x.arrays.at("GameSpeedInfo.turn.TurnsPerIncrement").slots->at(0),
                 "Actual allocated cache child retains UNWRITTEN slots after first reached width refusal");
            refuses([&]{bad->advance_constructor_scoped_lock_mods_turn_prefix();},E::wrong_phase);
        }
        auto overflow=make(schema+"UPDATE GameSpeeds SET TrainPercent=150;INSERT INTO Defines VALUES('RELIGION_MIN_FAITH_FIRST_PANTHEON',2147483647);");
        refuses([&]{overflow->advance_game_religions_speed_init(selected_config_ini);},E::native_rules_unavailable);
        const auto stopped=overflow->observe();
        need(stopped.boundary==B::failed_prefix && scalar(stopped,"GameReligions.m_iMinimumFaithForNextPantheon.current")==2147483647 &&
             !stopped.scalars.contains("GameReligions.m_iMinimumFaithForNextPantheon.multiply"),"Native intermediate overflow before store/cancellation");
        auto localized=make(schema+"ALTER TABLE TurnTimers ADD COLUMN Description TEXT;UPDATE TurnTimers SET Description='TXT_KEY_TIMER';");
        bool localized_failed=false;
        try{localized->advance_game_religions_speed_init(selected_config_ini);}catch(const runtime::ScriptError&){localized_failed=true;}
        const auto gap=localized->observe();
        need(localized_failed && gap.boundary==B::failed_prefix && scalar(gap,"Pregame.s_gameSpeed")==2 &&
             scalar(gap,"Pregame.s_turnTimerType")==3 && !gap.scalars.contains("GameReligions.m_iMinimumFaithForNextPantheon.multiply"),
             "Actual missing Localization DB error preserves speed/timer without numeric continuation");
    }

    auto missing=fixture("DELETE FROM Defines WHERE Name='CITY_RING_2_MULTIPLIER';DELETE FROM Flavors;");
    auto absent=G::from_creation_config(runtime::BnwCreationConfig::from_selected_rules(missing,choices()));
    absent->advance_reset_prefix();absent->advance_settler_site_init();const auto missing_state=absent->observe();
    need(missing_state.arrays.at("Settler.m_iRingModifier").slots->at(2)==0 &&
         scalar(missing_state,"Settler.m_iGrowthIndex")==-1 && scalar(missing_state,"Settler.m_iNavalIndex")==-1,
         "Missing cached Define overwrites constructor constant with0; missing native name gives-1");
    auto ring_bad=fixture("UPDATE Defines SET Value=2147483648 WHERE Name='CITY_RING_3_MULTIPLIER';");
    auto ring_partial=G::from_creation_config(runtime::BnwCreationConfig::from_selected_rules(ring_bad,choices()));
    ring_partial->advance_reset_prefix();refuses([&]{ring_partial->advance_settler_site_init();},E::native_rules_unavailable);
    const auto rings=ring_partial->observe();const auto& prefix=*rings.arrays.at("Settler.m_iRingModifier").slots;
    need(rings.boundary==B::failed_prefix && prefix[0]==1 && prefix[1]==12 && prefix[2]==-8 && !prefix[3] && !prefix[7] &&
         scalar(rings,"Settler.m_iGrowthIndex")==13 && !rings.scalars.contains("Settler.m_iBrazilMultiplier"),
         "Later Define refusal retains only prior actual stores, no later Info/multiplier operands");
    refuses([&]{ring_partial->advance_settler_site_init();},E::wrong_phase);
    auto type_bad=fixture("UPDATE Flavors SET ID=2147483648 WHERE Type='FLAVOR_EXPANSION';");
    auto type_partial=G::from_creation_config(runtime::BnwCreationConfig::from_selected_rules(type_bad,choices()));
    type_partial->advance_reset_prefix();refuses([&]{type_partial->advance_settler_site_init();},E::native_rules_unavailable);
    const auto ids=type_partial->observe();
    need(ids.arrays.at("Settler.m_iRingModifier").slots->at(7)==0 && scalar(ids,"Settler.m_iGrowthIndex")==7 &&
         scalar(ids,"Settler.m_iExpansionIndex")==12 && !ids.scalars.contains("Settler.m_iNavalIndex"),
         "ID width failure retains rings and reached Growth store before Expansion/Naval");
    auto overwrite=fixture("CREATE TABLE LateTypes(ID INTEGER,Type TEXT);INSERT INTO LateTypes VALUES(4,'FLAVOR_GROWTH');");
    auto last=G::from_creation_config(runtime::BnwCreationConfig::from_selected_rules(overwrite,choices()));
    last->advance_reset_prefix();last->advance_settler_site_init();
    need(scalar(last->observe(),"Settler.m_iGrowthIndex")==4,"Native global name map last-wins across tables");
    const auto second_cache=std::find(after.order.rbegin(),after.order.rend(),"cache DeleteFile/Open/Create SimpleValues").base();
    need(second_cache!=after.order.begin() && std::find(second_cache,after.order.end(),"m_eHandicap")!=after.order.end(),
         "Cache effect must precede selected Handicap store");
    refuses([&]{g->advance_reset_prefix();},E::already_attempted);
    bool foreign=false;std::thread t([&]{try{g->advance_reset_prefix();}catch(const runtime::BnwFreshGameRefusal& e){foreign=e.code()==E::wrong_thread;}});
    t.join();need(foreign && g->observe().cache_resets==2,"Foreign calls cannot query DB or reset files");
    auto original_realm=g->city_manager();
    need(scalar(g->observe(),"CityManager.scopedResetCalls")==1 && scalar(g->observe(),"CityManager.scopedShutdownCalls")==2,
         "Actual earlier constructor and Uninit share source owner");
    auto other=G::from_creation_config(c);need(other->city_manager().get()!=original_realm.get(),"Different Game allocations have isolated pointer realms");
    need(other->observe().local_cache_database!=after.local_cache_database,
         "Each Game has a unique actual local cache directory");
    std::weak_ptr<const runtime::BnwCreationConfig> weak_c=c;
    std::weak_ptr<const runtime::BnwFreshGameContentResult> weak_p=p;
    original_realm.reset();early.reset();other.reset();c.reset();p.reset();need(!weak_c.expired() && !weak_p.expired(),"Sole Game retains SAME config/rules parents");
    need(g->observe().boundary==B::game_religions_speed_source_required,"Retained Game state lifetime");g.reset();
    need(weak_c.expired() && weak_p.expired(),"Game parent graph must be acyclic");
    auto bad=fixture("DROP TABLE UnitClasses;");auto bad_c=runtime::BnwCreationConfig::from_selected_rules(bad,choices());
    auto partial=G::from_creation_config(bad_c);
    refuses([&]{partial->advance_reset_prefix();},E::native_rules_unavailable);
    const auto partial_state=partial->observe();
    need(partial_state.cache_resets==2 && scalar(partial_state,"m_eHandicap")==7 &&
         partial_state.arrays.at("m_paiUnitCreatedCount").slots &&
         !partial_state.arrays.at("m_paiUnitClassCreatedCount").slots &&
         partial_state.boundary==B::failed_prefix,"Later refusal retains actual reached stores only");
    refuses([&]{partial->advance_reset_prefix();},E::already_attempted);
    auto empty=fixture("DELETE FROM Victories;DROP TABLE Defines;");
    auto skip=G::from_creation_config(runtime::BnwCreationConfig::from_selected_rules(empty,choices()));
    skip->advance_reset_prefix();need(skip->observe().team_victory_rank->empty(),"Empty Victories must not read future Define");
    auto extreme=fixture("UPDATE Defines SET Value=2147483648;");
    auto overflow=G::from_creation_config(runtime::BnwCreationConfig::from_selected_rules(extreme,choices()));
    refuses([&]{overflow->advance_reset_prefix();},E::native_rules_unavailable);
    const auto reached=overflow->observe();
    need(reached.cache_resets==2 && reached.team_victory_rank && reached.team_victory_rank->size()==2 &&
         reached.team_victory_rank->at(0).empty() && !reached.scalars.contains("Settler.m_iExpansionIndex"),
         "Reached invalid award width must retain prior arrays but not construct SiteEvaluator");
    auto retired=fixture();auto old_c=runtime::BnwCreationConfig::from_selected_rules(retired,choices());auto old_g=G::from_creation_config(old_c);
    retired->rules.reset();
    bool stopped=false;try{old_g->advance_reset_prefix();}catch(const runtime::BnwCreationConfigRefusal& e){stopped=e.code()==runtime::BnwCreationConfigError::rules_retired;}
    need(stopped,"Retired rules must refuse before cache/world/state effects");
}
void actual(const std::filesystem::path& installation,const std::filesystem::path& selected_config_ini) {
    content::Catalog catalog;
    catalog.packages.push_back(content::read_manifest(installation/"Assets/DLC/Expansion/Expansion1.Civ5Pkg"));
    catalog.packages.push_back(content::read_manifest(installation/"Assets/DLC/Expansion2/Expansion2.Civ5Pkg"));
    constexpr std::array<std::size_t,2> selected{0,1};
    auto p=std::make_shared<runtime::BnwFreshGameContentResult>(runtime::bootstrap_bnw_fresh_game_content(installation,catalog,selected));
    need(p->content_ready(),"Actual selected BNW content unavailable");p->rules->seal_immutable_preview_fingerprints();
    auto c=runtime::BnwCreationConfig::from_selected_rules(p,choices());const auto chosen=c->metadata().info[5].id;

    auto menu_game=runtime::BnwFreshGame::from_creation_config(c);const auto constructor=menu_game->observe();
    refuses([&]{menu_game->advance_single_player_pregame_menu_prefix(selected_config_ini);},runtime::BnwFreshGameError::native_rules_unavailable);
    refuses([&]{menu_game->advance_single_player_pregame_menu_options_prefix();},runtime::BnwFreshGameError::native_rules_unavailable);
    const auto setup=menu_game->pregame_setup_observation();const auto setup_game=menu_game->observe();
    const auto victories=p->rules->prefetch_info("Victories",{},content::InfoCollectionBudget{65536});
    need(victories.all_succeeded() && setup.reset.victories.size()==victories.slots &&
         std::all_of(setup.reset.victories.begin(),setup.reset.victories.end(),[](bool value){return value;}) &&
         setup_game.boundary==runtime::BnwFreshGameBoundary::pregame_settings_source_required &&
         setup.menu.boundary==runtime::BnwFreshPregameMenuBoundary::load_pregame_settings_required &&
         setup_game.cache_resets==constructor.cache_resets && setup_game.map_random==constructor.map_random &&
         setup_game.gameplay_random==constructor.gameplay_random,
         "Actual installed SAME Game-issued PG menu cache/victory setup source must preserve real LoadPreGameSettings frontier");
    refuses([&]{menu_game->advance_reset_prefix();},runtime::BnwFreshGameError::native_rules_unavailable);
    std::cout<<"actual_owned_Pregame_menu=1 victory_slots="<<setup.reset.victories.size()
             <<" next=LoadPreGameSettings GameReset=UNREACHED full_new_game=0\n";

    auto g=runtime::BnwFreshGame::from_creation_config(c);g->advance_reset_prefix();const auto x=g->observe();
    need(x.cache_resets==2 && scalar(x,"m_eHandicap")==chosen &&
         x.boundary==runtime::BnwFreshGameBoundary::settler_site_init_required,"Actual selected Game prefix");
    need(x.team_victory_rank && !x.team_victory_rank->empty() && x.arrays.at("m_paiUnitCreatedCount").slots->size()>0,
         "Actual native DB reset arrays must be reached");
    verify_database(x.local_cache_database);
    g->advance_settler_site_init();const auto site=g->observe();
    need(site.boundary==runtime::BnwFreshGameBoundary::start_site_construction_required &&
         site.cache_resets==x.cache_resets && site.map_random==x.map_random && site.gameplay_random==x.gameplay_random,
         "Actual selected Site Init reaches next constructor without reset tail");
    const auto types=p->rules->cache_info_types();need(types.report().all_succeeded(),"Actual native type map unavailable");
    for(const auto& names:{std::pair{"FLAVOR_GROWTH","Settler.m_iGrowthIndex"},
                          std::pair{"FLAVOR_EXPANSION","Settler.m_iExpansionIndex"},
                          std::pair{"FLAVOR_NAVAL","Settler.m_iNavalIndex"}}) {
        const auto* entry=types.find(names.first);need(entry && scalar(site,names.second)==entry->id,"Actual native selected flavor Type binding");
    }
    for(std::size_t i=1;i<=5;++i) {
        const auto value=p->rules->global_define("CITY_RING_"+std::to_string(i)+"_MULTIPLIER",content::GlobalDefineKind::integer);
        need(value.error.empty() && site.arrays.at("Settler.m_iRingModifier").slots->at(i)==std::get<std::int64_t>(value.value),
             "Actual selected cached ring Define exact value");
    }
    g->advance_start_site_init();const auto actual_start=g->observe();
    need(actual_start.boundary==runtime::BnwFreshGameBoundary::start_positioner_construction_required &&
         actual_start.arrays.at("Start.m_iRingModifier").slots==site.arrays.at("Settler.m_iRingModifier").slots &&
         actual_start.cache_resets==site.cache_resets && actual_start.map_random==site.map_random &&
         actual_start.gameplay_random==site.gameplay_random,"Actual selected SAME Game Start prefix");
    g->advance_start_positioner_init();const auto actual_positioner=g->observe();
    need(actual_positioner.boundary==runtime::BnwFreshGameBoundary::game_deals_init_required &&
         actual_positioner.cache_resets==actual_start.cache_resets && actual_positioner.map_random==actual_start.map_random &&
         actual_positioner.gameplay_random==actual_start.gameplay_random,"Actual same-owner positioner prefix");
    const auto& actual_neutral=*actual_positioner.arrays.at("Start.m_iFlavorMultiplier").slots;
    need(actual_neutral.size()==9 && std::all_of(actual_neutral.begin(),actual_neutral.end(),[](const auto& v){return v==1;}),
         "Actual selected Start native neutral flavor9");
    g->advance_deals_game_religions_prefix();const auto actual_religions=g->observe();
    const auto faith=p->rules->global_define("RELIGION_MIN_FAITH_FIRST_PANTHEON",content::GlobalDefineKind::integer);
    need(faith.error.empty() && actual_religions.boundary==runtime::BnwFreshGameBoundary::game_religions_speed_source_required &&
         scalar(actual_religions,"GameReligions.m_iMinimumFaithForNextPantheon.current")==std::get<std::int64_t>(faith.value) &&
         scalar(actual_religions,"GameReligions.m_CurrentReligions.size")==0 &&
         actual_religions.cache_resets==actual_positioner.cache_resets && actual_religions.map_random==actual_positioner.map_random &&
         actual_religions.gameplay_random==actual_positioner.gameplay_random,"Actual SAME selected DB first religion store");
    g->advance_game_religions_speed_init(selected_config_ini);const auto full_religions=g->observe();
    need(full_religions.boundary==runtime::BnwFreshGameBoundary::game_culture_construction_required,
         "Actual selected speed/timer/localization and religion Init must complete");
    const auto speed=scalar(full_religions,"Pregame.s_gameSpeed");
    std::optional<std::int64_t> train;
    const auto report=p->rules->prefetch_info("GameSpeeds",[&](std::uint64_t slot,std::int64_t,const content::RulesRowView& row){
        if(slot==static_cast<std::uint64_t>(speed)) {train=row.integer("TrainPercent");}
        return true;
    });
    need(report.all_succeeded() && train && *train>=std::numeric_limits<std::int32_t>::min() &&
         *train<=std::numeric_limits<std::int32_t>::max(),"Actual selected native TrainPercent source");
    const auto product=std::get<std::int64_t>(faith.value)* *train;
    need(product>=std::numeric_limits<std::int32_t>::min() && product<=std::numeric_limits<std::int32_t>::max() &&
         scalar(full_religions,"GameReligions.m_iMinimumFaithForNextPantheon.multiply")==product &&
         scalar(full_religions,"GameReligions.m_iMinimumFaithForNextPantheon.divide100")==product/100,
         "Actual religion numeric source product before native truncation");
    g->advance_culture_leagues_trade_init();const auto actual_services=g->observe();
    need(actual_services.boundary==runtime::BnwFreshGameBoundary::tactical_map_construction_required &&
         scalar(actual_services,"GameTrade.Init.Reset.popup.type")==0 &&
         actual_services.map_random==full_religions.map_random && actual_services.gameplay_random==full_religions.gameplay_random,
         "Actual SAME selected Game service constructors precede Tactical allocation");
    g->advance_tactical_advisors_reset_tail();const auto actual_tail=g->observe();
    need(actual_tail.boundary==runtime::BnwFreshGameBoundary::city_manager_reset_required &&
         !g->tactical_analysis_map()->observe().plots_allocated && g->advisor_counsel()->size()==50 &&
         actual_tail.map_random.resets==actual_services.map_random.resets+1 &&
         actual_tail.gameplay_random.resets==actual_services.gameplay_random.resets+1,
         "Actual SAME Game allocation tail stops before CityManagerReset, no TacticalInit");
    const auto* realm=g->city_manager().get();g->advance_city_manager_reset();const auto actual_final=g->observe();
    need(actual_final.boundary==runtime::BnwFreshGameBoundary::scoped_reset_complete_owned_cities_required &&
         g->city_manager().get()==realm && scalar(actual_final,"CityManager.scopedResetCalls")==2 &&
         actual_final.map_random==actual_tail.map_random && actual_final.gameplay_random==actual_tail.gameplay_random,
         "Actual selected scoped Reset returns without new city/Init/map readiness");
    g->advance_init_prefix({0,4294967295ULL,runtime::BnwFreshGameType::single_player});const auto actual_init=g->observe();
    need(actual_init.boundary==runtime::BnwFreshGameBoundary::init_mp_options_roster_required &&
         actual_init.map_random==runtime::SimulationRandomState{0,actual_final.map_random.calls,actual_final.map_random.resets+1} &&
         actual_init.gameplay_random==runtime::SimulationRandomState{4294967295ULL%52319761ULL,actual_final.gameplay_random.calls,actual_final.gameplay_random.resets+1} &&
         scalar(actual_init,"Pregame.s_era")==c->metadata().info[4].id && scalar(actual_init,"Pregame.s_gameType")==0 &&
         actual_init.cache_resets==actual_final.cache_resets,"Actual selected current Era setter and fresh input Init prefix; no RNG draw");
    std::cout<<"actual_init_map_seed="<<actual_init.map_random.seed<<" sync_seed="<<actual_init.gameplay_random.seed
             <<" era="<<scalar(actual_init,"Pregame.s_era")<<" next=MP_options_and_roster full_init=0\n";
    g->advance_single_player_options();const auto actual_options=g->observe();
    need(actual_options.boundary==runtime::BnwFreshGameBoundary::init_lock_mods_speed_required &&
         actual_options.map_random==actual_init.map_random && actual_options.gameplay_random==actual_init.gameplay_random &&
         scalar(actual_options,"Pregame.s_gameType")==0 && std::get<std::uint32_t>(actual_options.scalars.at("Pregame.options.count"))==3 &&
         std::get<bool>(actual_options.scalars.at("Pregame.options.complete")),"Actual selected SP options use SAME owner, no RNG draw/type change");
    const auto& actual_mp=*actual_options.arrays.at("Pregame.s_multiplayerOptions").bool_slots;
    need(actual_mp.size()==4 && std::none_of(actual_mp.begin(),actual_mp.end(),[](bool v){return v;}),"Actual all4 MP false");
    const auto& actual_cache=*actual_options.arrays.at("Pregame.s_EnumBasedGameOptions").slots;
    need(actual_cache.size()==22 && actual_cache[16]==0 && std::get<bool>(actual_options.scalars.at("Pregame.s_quickCombat"))==(actual_cache[15]!=0),
         "Actual full enum cache/QuickCombat mirrored native source value");
    std::cout<<"actual_SP_options=3 MP4=false enum_cache=22 next=LOCK_MODS_and_GameSpeed full_init=0\n";
    g->advance_constructor_scoped_lock_mods_turn_prefix();const auto actual_turns=g->observe();
    std::int64_t expected_turns=0;bool row=false;
    const auto sum=p->rules->query_rows({"SELECT SUM(TurnsPerIncrement) FROM GameSpeed_Turns WHERE GameSpeedType=?",
        {c->metadata().info[3].type}},[&](const content::RulesRowView& value){expected_turns=value.integer_at(0);row=true;},1);
    need(sum.all_succeeded() && row && expected_turns>=std::numeric_limits<std::int32_t>::min() && expected_turns<=std::numeric_limits<std::int32_t>::max(),
         "Actual selected turn-table aggregate unavailable");
    const auto draws=actual_cache[8]==1?7ULL:0ULL;
    need(actual_turns.boundary==runtime::BnwFreshGameBoundary::constructor_scoped_init_victory_setup_required &&
         scalar(actual_turns,"Pregame.s_gameTurn")==-1 && scalar(actual_turns,"m_iStartTurn")==-1 &&
         scalar(actual_turns,"m_iDefaultEstimateEndTurn")==expected_turns &&
         actual_turns.gameplay_random.calls==actual_options.gameplay_random.calls+draws &&
         actual_turns.map_random==actual_options.map_random && actual_turns.cache_resets==actual_options.cache_resets,
         "Actual selected LOCK and GameSpeed source prefix; namespace defaults not full newGame setup");
    std::cout<<"actual_LOCK_MODS="<<(actual_cache[8]==1)<<" draws="<<draws<<" actual_Pregame_turn=-1 estimate="<<expected_turns
             <<" next=actual_Pregame_setup_victories full_init=0\n";

    std::cout<<"actual_game_reset_cache=2 handicap="<<chosen<<" units="<<x.arrays.at("m_paiUnitCreatedCount").slots->size()
             <<" next=owned_city_allocation_and_GameInit map_created=0 playable=0\n";
}
} // namespace
int main(int argc,char** argv) {
    try{need(argc==3,"Expected canonical BNW installation and selected INI");cases(argv[2]);owned_pregame_cases(argv[2]);actual(std::filesystem::path(argv[1]),argv[2]);
        std::cout<<"BNW fresh Game constructor/reset prefix passed\n";return 0;
    }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}
