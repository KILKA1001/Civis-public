#include "civ5/runtime/bnw_fresh_game_builder.hpp"
#include "civ5/content/rules_database.hpp"
#include "civ5/runtime/file_identity.hpp"
#include <stdexcept>
#include <type_traits>
#include <algorithm>
#include <cstddef>
#include <iterator>

namespace {
void check(bool condition, const char *message) {
    if (!condition) throw std::runtime_error(message);
}

void test_atomic_selected_fresh_diagnostics() {
    using namespace civ5::runtime;
    BnwFreshGameContentResult selected;
    selected.profile.kind = ContentProfileKind::dlc;
    selected.rules = std::make_unique<civ5::content::RulesDatabase>();
    constexpr const char *info_tables[] = {
        "VoteSources", "Victories", "SmallAwards", "Routes", "Builds",
        "Projects", "UnitClasses", "BuildingClasses", "Buildings", "Units",
        "Terrains", "Improvements", "Technologies", "Resources",
        "UnitCombatInfos", "UnitPromotions", "HurryInfos", "Leaders",
        "AIEconomicStrategies", "AIMilitaryStrategies", "Flavors",
        "CitySpecializations", "HandicapInfos", "AIGrandStrategies", "Beliefs"};
    for (const auto *table : info_tables)
        selected.rules->apply_sql(std::string("CREATE TABLE ") + table +
            "(ID INTEGER PRIMARY KEY, Type TEXT); INSERT INTO " + table +
            " VALUES(0,'ROW_0');");
    selected.rules->apply_sql(R"sql(
        CREATE TABLE PlayerColors(ID INTEGER PRIMARY KEY, Type TEXT);
        INSERT INTO PlayerColors VALUES(0,'PLAYERCOLOR_BARBARIAN');
        INSERT INTO PlayerColors VALUES(1,'PLAYERCOLOR_BLUE');
        CREATE TABLE Civilizations(ID INTEGER PRIMARY KEY, DefaultPlayerColor TEXT);
        INSERT INTO Civilizations VALUES(10,'PLAYERCOLOR_BLUE');
        INSERT INTO Civilizations VALUES(99,'PLAYERCOLOR_BARBARIAN');
        CREATE TABLE Defines(Name TEXT PRIMARY KEY, Value TEXT);
        INSERT INTO Defines VALUES('AI_STRATEGY_MINIMUM_SETTLE_FERTILITY','6');
        INSERT INTO Defines VALUES('AI_STRATEGY_EARLY_EXPANSION_NUM_CITIES_LIMIT','10');
        INSERT INTO Defines VALUES('DIPLO_VICTORY_CIV_DELEGATES_COEFFICIENT','1.443');
        INSERT INTO Defines VALUES('DIPLO_VICTORY_CIV_DELEGATES_CONSTANT','7');
        INSERT INTO Defines VALUES('DIPLO_VICTORY_CS_DELEGATES_COEFFICIENT','16.023');
        INSERT INTO Defines VALUES('DIPLO_VICTORY_CS_DELEGATES_CONSTANT','-13.758');
        CREATE TABLE LeagueSpecialSessions(
            ID INTEGER PRIMARY KEY, UnitedNations INTEGER,
            HostDelegates INTEGER, CivDelegates INTEGER,
            CityStateDelegates INTEGER);
        INSERT INTO LeagueSpecialSessions VALUES(0,1,2,4,2);
    )sql");
    // Flavor MaxRows(rowid)+1 and Building Info spans are distinct native sizes.
    selected.rules->apply_sql("INSERT INTO Buildings VALUES(2,'NON_WONDER_BUILDING'); INSERT INTO Flavors VALUES(2,'FLAVOR_SECOND'); INSERT INTO AIGrandStrategies VALUES(2,'STRATEGY_SECOND'); INSERT INTO AIGrandStrategies VALUES(-1,'EXCLUDED_NEGATIVE_ID'); INSERT INTO Beliefs VALUES(4,'BELIEF_SECOND');");
    selected.rules->apply_sql(R"sql(
        ALTER TABLE Civilizations ADD COLUMN Type TEXT;
        UPDATE Civilizations SET Type=CASE ID WHEN 10 THEN 'CIV_TEN' ELSE 'CIV_BARBARIAN' END;
        ALTER TABLE Buildings ADD COLUMN PrereqTech TEXT;
        UPDATE Buildings SET Type=CASE ID WHEN 0 THEN 'BUILDING_ZERO' ELSE 'BUILDING_SECOND' END;
        ALTER TABLE Units ADD COLUMN PrereqTech TEXT;
        UPDATE Units SET Type='UNIT_ZERO';
        ALTER TABLE BuildingClasses ADD COLUMN DefaultBuilding TEXT;
        UPDATE BuildingClasses SET Type='BCLASS_ZERO',DefaultBuilding='BUILDING_ZERO';
        ALTER TABLE UnitClasses ADD COLUMN DefaultUnit TEXT;
        UPDATE UnitClasses SET Type='UCLASS_ZERO',DefaultUnit='UNIT_ZERO';
        CREATE TABLE Civilization_BuildingClassOverrides(CivilizationType TEXT,BuildingClassType TEXT,BuildingType TEXT);
        CREATE TABLE Civilization_UnitClassOverrides(CivilizationType TEXT,UnitClassType TEXT,UnitType TEXT);
        ALTER TABLE Improvements ADD COLUMN SpecificCivRequired INTEGER;
        ALTER TABLE Improvements ADD COLUMN CivilizationType TEXT;
        UPDATE Improvements SET Type='IMPROVEMENT_ZERO';
        ALTER TABLE Builds ADD COLUMN ImprovementType TEXT;
        ALTER TABLE Builds ADD COLUMN PrereqTech TEXT;
        UPDATE Builds SET Type='BUILD_ZERO';
        UPDATE Technologies SET Type='TECH_ZERO'; INSERT INTO Technologies VALUES(2,'TECH_SECOND');
        UPDATE Leaders SET Type='LEADER_ZERO';
        CREATE TABLE Traits(ID INTEGER PRIMARY KEY,Type TEXT,MayaCalendarBonuses INTEGER,PrereqTech TEXT);
        CREATE TABLE Leader_Traits(LeaderType TEXT,TraitType TEXT);
        INSERT INTO Defines VALUES('TECH_PRIORITY_UNIQUE_ITEM','5');
        INSERT INTO Defines VALUES('TECH_PRIORITY_MAYA_CALENDAR_BONUS','50');
    )sql");
    selected.rules->apply_sql(R"sql(
        CREATE TABLE Policies(ID INTEGER PRIMARY KEY,Type TEXT,PolicyBranchType TEXT);
        INSERT INTO Policies VALUES(0,'POLICY_ZERO',NULL);
        INSERT INTO Policies VALUES(2,'POLICY_SECOND','BRANCH_SECOND');
        INSERT INTO Policies VALUES(4,'POLICY_FINISHER',NULL);
        CREATE TABLE PolicyBranchTypes(ID INTEGER PRIMARY KEY,Type TEXT,FreePolicy TEXT,FreeFinishingPolicy TEXT);
        INSERT INTO PolicyBranchTypes VALUES(0,'BRANCH_FIRST','POLICY_ZERO','POLICY_FINISHER');
        INSERT INTO PolicyBranchTypes VALUES(2,'BRANCH_SECOND','POLICY_ZERO',NULL);
        INSERT INTO PolicyBranchTypes VALUES(6,'BRANCH_THIRD','POLICY_SECOND',NULL);
        INSERT INTO Defines VALUES('POLICY_WEIGHT_PROPAGATION_LEVELS','-3');
        INSERT INTO Defines VALUES('POLICY_WEIGHT_PERCENT_DROP_NEW_BRANCH','101');
    )sql");
    selected.activation.emplace();
    selected.activation->profile_kind = ContentProfileKind::dlc;
    selected.activation->rules_database_ready = true;
    const std::vector<civ5::content::GameplayDefine> constants{
        {"MAX_TEAMS",3}, {"MAX_PLAYERS",3}, {"MAX_CIV_PLAYERS",2},
        {"MAX_MAJOR_CIVS",1}, {"BARBARIAN_PLAYER",2},
        {"BARBARIAN_TEAM",2}, {"NUM_DOMAIN_TYPES",5},
        {"NUM_YIELD_TYPES",6}, {"BARBARIAN_CIVILIZATION",99},
        {"BARBARIAN_HANDICAP",0}, {"BARBARIAN_LEADER",0},
        {"MINOR_CIVILIZATION",10}, {"MINOR_CIV_HANDICAP",0},
        {"AI_HANDICAP",0}};
    const std::vector<BnwFreshPregameSlot> slots{
        {BnwFreshSlotStatus::taken,0},
        {BnwFreshSlotStatus::closed,1},
        {BnwFreshSlotStatus::closed,2}};
    const std::vector<BnwFreshMajorColorSlot> colors{
        {BnwFreshSlotStatus::taken,10,-1}};
    const std::vector<std::int32_t> leaders{0,-1,-1};
    const BnwFreshGameBuilderInputs inputs{
        constants,slots,colors,leaders,{},0,-1,99};
    auto draft = draft_bnw_fresh_gamecore(selected,inputs);
    check(draft.detached_diagnostics_complete &&
              !draft.initialized_by_gamecore &&
              draft.frontier && draft.frontier->code ==
                  BnwFreshGameBuilderBlockerCode::tactical_game_analysis_map_source_pending &&
              draft.selected_rules_sha256 == identify_bytes(
                  selected.rules->gameplay_fingerprint_data()).sha256 &&
              draft.team_info.teams.size() == 3 &&
              draft.player_info.players.size() == 3 &&
              draft.city_specialization.players.size() == 3 &&
              draft.league.first_alive_callback_core_complete &&
              !draft.league.league_screen_dirty_emitted,
          "All selected detached diagnostics share provenance and one frontier");
    check(!draft.source_order_prefix.empty() &&
              draft.source_order_prefix[0] == BnwFreshGameBuilderEvent{
                  BnwFreshGameBuilderStep::team_init,0} &&
              draft.source_order_prefix[1] == BnwFreshGameBuilderEvent{
                  BnwFreshGameBuilderStep::team_init,1} &&
              draft.source_order_prefix[2] == BnwFreshGameBuilderEvent{
                  BnwFreshGameBuilderStep::team_init,2} &&
              draft.source_order_prefix[3] == BnwFreshGameBuilderEvent{
                  BnwFreshGameBuilderStep::major_color_pass,-1} &&
              draft.slot_info.pregame_writes_complete &&
              draft.slot_info.source_order.size() == 9 &&
              draft.first_player_objects.constructors_complete &&
              draft.first_player_uninit.preflight_complete &&
              !draft.first_player_uninit.sync_archive_reset_verified &&
              draft.first_resource_vector.member_effect_complete &&
              !draft.first_resource_vector.archive_traversal_verified &&
              draft.first_player_diagnostic.diagnostic_complete &&
              !draft.first_player_diagnostic.source_prefix_advanced &&
              !draft.first_player_diagnostic.initialized_by_gamecore &&
              draft.first_player_diagnostic.actual_source_frontier ==
                  "m_syncArchive.reset" &&
              draft.first_player_diagnostic.detached_uninit.
                  detached_effects_complete &&
              draft.first_player_diagnostic.detached_uninit.
                  blocked_at == "archive.clearDelta" &&
              draft.first_player_diagnostic.detached_uninit.
                  source_order.size() == 60 &&
              draft.source_order_prefix.size() == 18 &&
              draft.source_order_prefix.back() == BnwFreshGameBuilderEvent{
                  BnwFreshGameBuilderStep::owned_player_reset_through_tactical_reset,0},
          "Owned gameplay reset advances while original archive diagnostics stay explicit");

    static_assert(!std::is_default_constructible_v<BnwFreshFirstPlayerState>);
    check(bool(draft.first_player_state),"Real builder must issue owned first-player state");
    const auto owner=draft.first_player_state;
    const auto scalar=[&](std::string_view name){return std::get<std::int32_t>(owner->registered_member(name).value);};
    check(scalar("m_iStartingX")==-2147483647 && scalar("m_iStartingY")==-2147483647 &&
          scalar("m_iCapitalCityID")==-1 && scalar("m_iHighestUnitLevel")==1 &&
          scalar("m_eID")==0 && scalar("m_ePersonalityType")==0,
          "Native Uninit sentinels precede actual reset player identity");
    check(std::get<std::int32_t>(owner->unregistered_scalar("m_iFaith"))==0 &&
          std::get<std::int32_t>(owner->unregistered_scalar("m_iMedianTechPercentage"))==50 &&
          std::get<std::int32_t>(owner->unregistered_scalar("m_iMaxEffectiveCities"))==1 &&
          std::get<bool>(owner->unregistered_scalar("m_bDynamicTurnsSimultMode")),
          "Unregistered gameplay values are owned, not omitted as bookkeeping");
    const auto& resource=owner->registered_member("m_paiNumResourceUsed");
    check(std::get<std::vector<std::int32_t>>(resource.value)==std::vector<std::int32_t>{0} &&
          resource.pending_vector_commands==2 &&
          std::get<std::vector<bool>>(owner->registered_member("m_pabLoyalMember").value)==std::vector<bool>{true} &&
          owner->registered_member("m_aOptions").pending_vector_commands==1 &&
          owner->registered_member("m_iStartingX").pending_vector_commands==0,
          "clearDelta preserves gameplay values and subsequent clear/resize produces new commands");
    check(owner->great_work_yield_change().size()==6 && owner->siphon_luxury_count().size()==3 &&
          owner->embarked_graphic_override().empty() && owner->economic().minimum_settle_fertility==6 &&
          owner->military().total_threat_weight==1 && !owner->military().temp_flavors.front().has_value() &&
          owner->city_specialization().next_specialization==-1,
          "Existing selected reset and AI states become actual owned first-player components");
    const auto trace=owner->source_order();
    const auto delta=std::find(trace.begin(),trace.end(),"archive.clearDelta owned command tracking");
    const auto starting=std::find(trace.begin(),trace.end(),"m_iStartingX");
    check(delta!=trace.end() && starting!=trace.end() && delta<starting && trace.back()=="CvTacticalAI::Reset MapNULL" &&
          std::find(trace.begin(),trace.end(),"m_pTacticalAI->Init")==trace.end(),
          "Native writes occur after tracking clear; absent next service is not executed");
    check(owner->tactical_reset_value("current_zone_index")==-1 &&
          owner->tactical_reset_value("move_priority_turn")==-1 &&
          owner->tactical_reset_map_is_null(),
          "Actual constructor zone and reached Reset fields retain source sentinels");
    const auto policies_end=std::find(trace.begin(),trace.end(),"m_pPlayerPolicies->Init");
    const auto tactical_bind=std::find(trace.begin(),trace.end(),"CvTacticalAI::Init SAME player");
    check(policies_end<tactical_bind && tactical_bind+1<trace.end() &&
          *(tactical_bind+1)=="CvTacticalAI::Reset MovePriorityTurn-1" &&
          tactical_bind+2==trace.end()-1 &&
          std::find(trace.begin(),trace.end(),"m_pHomelandAI->Init")==trace.end(),
          "Tactical binding and two Reset stores precede the absent actual Game pointer read");
    bool unreached_tactical=false;
    try{(void)owner->tactical_reset_value("recruit_range");}
    catch(const std::out_of_range&){unreached_tactical=true;}
    check(unreached_tactical,"No later cached Define field is zero-inferred");
    // Controlled selected Rules variation, never an actual Game issuer. An
    // out-of-range FUTURE getter must not be consumed before the map frontier.
    selected.rules->apply_sql("INSERT INTO Defines VALUES('AI_TACTICAL_RECRUIT_RANGE','9223372036854775807')");
    const auto future_tactical=draft_bnw_fresh_gamecore(selected,inputs);
    check(future_tactical.first_player_state && future_tactical.frontier &&
          future_tactical.frontier->code==BnwFreshGameBuilderBlockerCode::tactical_game_analysis_map_source_pending &&
          future_tactical.first_player_state->tactical_reset_value("move_priority_turn")==-1 &&
          std::ranges::equal(future_tactical.first_player_state->policy_weights(),owner->policy_weights()),
          "Unreached Game/Define/Info cannot eagerly fail or alter reached Policies/Tactical Reset");
    selected.rules->apply_sql("DELETE FROM Defines WHERE Name='AI_TACTICAL_RECRUIT_RANGE'");
    check(!owner->wonder_is_city() && owner->wonder_latest_flavors().size()==3 &&
          std::all_of(owner->wonder_latest_flavors().begin(),owner->wonder_latest_flavors().end(),[](auto v){return v==0;}) &&
          owner->wonder_weights().size()==3 && owner->wonder_buildables().empty(),
          "Wonder Init owns native zero flavors, all building slots and distinct constructor buildables");
    for(std::int32_t i=0;i<3;++i)
        check(owner->wonder_weights()[static_cast<std::size_t>(i)]==std::pair<std::int32_t,std::int32_t>{i,0},
              "Wonder Reset retains sparse and non-wonder building indices in native order");
    const auto specialization=std::find(trace.begin(),trace.end(),"m_pCitySpecializationAI->Init");
    const auto flavor_init=std::find(trace.begin(),trace.end(),"CvWonderProductionAI::CvFlavorRecipient::Init");
    const auto bindings=std::find(trace.begin(),trace.end(),"CvWonderProductionAI::selected_buildings_player_isCity_bound");
    const auto weights=std::find(trace.begin(),trace.end(),"CvWonderProductionAI::Reset weights");
    check(specialization<flavor_init && flavor_init<bindings && bindings<weights && weights<trace.end()-1,
          "Native Wonder Init follows specialization, then flavor allocation, bindings and Reset");
    check(owner->grand_strategy_priorities().size()==3 &&
          std::all_of(owner->grand_strategy_priorities().begin(),owner->grand_strategy_priorities().end(),[](auto v){return v==-1;}) &&
          owner->grand_strategy_guesses().size()==22 && owner->grand_strategy_guess_confidence().size()==22 &&
          std::all_of(owner->grand_strategy_guesses().begin(),owner->grand_strategy_guesses().end(),[](auto v){return v==-1;}) &&
          std::all_of(owner->grand_strategy_guess_confidence().begin(),owner->grand_strategy_guess_confidence().end(),[](auto v){return v==-1;}) &&
          owner->grand_strategy_turns_since_active_set()==0 && owner->grand_strategy_active()==-1,
          "Grand Reset owns every Info slot and all 22 native major guesses, independent of smaller scaffold roster");
    const std::vector<std::string_view> grand_order{
        "CvGrandStrategyAI::selected_strategies_player_bound",
        "CvGrandStrategyAI::allocate priorities", "CvGrandStrategyAI::allocate guesses",
        "CvGrandStrategyAI::allocate confidence", "CvGrandStrategyAI::m_iNumTurnsSinceActiveSet",
        "CvGrandStrategyAI::m_eActiveGrandStrategy", "CvGrandStrategyAI::Reset priorities",
        "CvGrandStrategyAI::Reset guesses and confidence", "m_pGrandStrategyAI->Init"};
    const auto grand_begin=std::find(trace.begin(),trace.end(),grand_order.front());
    check(grand_begin!=trace.end() && std::distance(grand_begin,trace.end())>=static_cast<std::ptrdiff_t>(grand_order.size()) &&
          std::equal(grand_order.begin(),grand_order.end(),grand_begin) &&
          std::find(trace.begin(),trace.end(),"m_pWonderProductionAI->Init")<grand_begin,
          "Grand native allocation and Reset source order follows actual Wonder Init");
    check(owner->diplomacy_scalar("m_eDemandTargetPlayer")==-1 && owner->diplomacy_scalar("m_bDemandReady")==0 &&
          owner->diplomacy_scalar("m_iBoldness")==-1 && owner->diplomacy_scalar("m_eStateAllWars")==0 &&
          owner->diplomacy_scalar("m_eTargetPlayer")==-1 && owner->diplomacy_reset_write_count()==36818,
          "Diplomacy Reset executes native scalars and real array stores after Grand Init");
    for(std::size_t i=0;i<63;++i) {
        check(owner->diplomacy_array_value("m_paeApproachScratchPad",0,i)==-1 &&
              owner->diplomacy_array_value("m_paeLastWarProjection",0,i)==-1,
              "Native civ-player Reset writes all63 source values");
        for(std::size_t j=0;j<63;++j)
            check(owner->diplomacy_array_value("m_ppaaeOtherPlayerVictoryDisputeLevel",i,j)==-1 &&
                  owner->diplomacy_array_value("m_ppaaiOtherPlayerWarValueLost",i,j)==0,
                  "Native safe logical matrix preserves all63x63 source Reset values");
    }
    check(owner->diplomacy_array_value("m_paeApproachScratchPad",0,63)==0 &&
          !owner->diplomacy_array_value("m_paeLastWarProjection",0,63) &&
          owner->diplomacy_array_value("m_ppaaeOtherPlayerVictoryDisputeLevel",0,63)==0 &&
          !owner->diplomacy_array_value("m_ppaaeOtherPlayerVictoryDisputeLevel",1,63) &&
          !owner->diplomacy_array_value("m_ppaaeOtherPlayerVictoryDisputeLevel",63,0) &&
          owner->diplomacy_array_value("m_pabWantToRouteToMinor",0,40)==1 &&
          owner->diplomacy_array_value("m_pabWantToRouteToMinor",0,41)==0,
          "Constructor zeros, omitted uninitialized padding, NULL row padding and minor41 differ honestly");
    check(owner->diplomacy_array_value("m_paDeclarationsLog.m_eDeclaration",0,59)==-1 &&
          owner->diplomacy_array_value("m_paDeclarationsLog.m_iTurn",0,59)==-1 &&
          owner->diplomacy_array_value("m_paDeclarationsLog.m_iData1",0,59)==0 &&
          owner->diplomacy_array_value("m_paDeclarationsLog.m_eMustHaveMetPlayer",0,59)==0 &&
          owner->diplomacy_array_value("m_ppaDiploStatementsLog.m_iTurn",21,59)==-1,
          "Declarations Reset changes only source-selected members; both full diplo log domains initialized");
    const std::vector<std::string_view> diplomacy_order{
        "CvDiplomacyAI::m_pPlayer SAME allocation",
        "CvDiplomacyAI::DiplomacyAIData constructor allocation",
        "CvDiplomacyAI::Init native array aliases", "CvDiplomacyAI::Reset gameplay stores",
        "m_pDiplomacyAI->Init"};
    const auto diplomacy_begin=std::find(trace.begin(),trace.end(),diplomacy_order.front());
    check(diplomacy_begin!=trace.end() &&
          std::distance(diplomacy_begin,trace.end())>=static_cast<std::ptrdiff_t>(diplomacy_order.size()) &&
          std::equal(diplomacy_order.begin(),diplomacy_order.end(),diplomacy_begin) &&
          grand_begin+static_cast<std::ptrdiff_t>(grand_order.size())==diplomacy_begin,
          "Actual diplomacy player/data/aliases/Reset continuation follows Grand without later service execution");
    const std::vector<std::string_view> religions_order{
        "CvPlayerReligions::m_pPlayer SAME allocation",
        "CvPlayerReligions::m_bFoundingReligion", "CvPlayerReligions::m_iNumProphetsSpawned",
        "m_pReligions->Init"};
    const auto religions_begin=std::find(trace.begin(),trace.end(),religions_order.front());
    check(owner->religions_founding_religion()==0 && owner->religions_prophets_spawned()==0 &&
          religions_begin!=trace.end() &&
          std::distance(religions_begin,trace.end())>=static_cast<std::ptrdiff_t>(religions_order.size()) &&
          std::equal(religions_order.begin(),religions_order.end(),religions_begin) &&
          diplomacy_begin+static_cast<std::ptrdiff_t>(diplomacy_order.size())==religions_begin,
          "Real PlayerReligions owner binds then Reset stores int founding before prophets after Diplomacy Init");
    const std::vector<std::string_view> religion_ai_order{
        "Rules parent::Beliefs source collection materialization",
        "CvReligionAI::m_pBeliefs SAME source collection", "CvReligionAI::m_pPlayer SAME allocation",
        "CvReligionAI::Reset empty body", "m_pReligionAI->Init"};
    const auto religion_ai_begin=std::find(trace.begin(),trace.end(),religion_ai_order.front());
    check(owner->religion_ai_belief_slots()==5 && religion_ai_begin!=trace.end() &&
          std::distance(religion_ai_begin,trace.end())>=static_cast<std::ptrdiff_t>(religion_ai_order.size()) &&
          std::equal(religion_ai_order.begin(),religion_ai_order.end(),religion_ai_begin) &&
          religions_begin+static_cast<std::ptrdiff_t>(religions_order.size())==religion_ai_begin,
          "Selected Beliefs caller materialization precedes actual native beliefs/player binding and empty Reset");
    check(owner->tech_latest_flavors().size()==3 &&
          std::all_of(owner->tech_latest_flavors().begin(),owner->tech_latest_flavors().end(),[](auto x){return x==0;}) &&
          owner->tech_weights().size()==3 && owner->tech_weights()[1]==std::pair<std::int32_t,std::int32_t>{1,0} &&
          !owner->tech_researching(0) && owner->tech_reset_value("civ_priority",1)==1 &&
          owner->tech_reset_value("locale_priority",2)==1 && owner->tech_reset_value("resource",0)==-1 &&
          owner->tech_reset_value("unit",0)==-1 && owner->tech_reset_value("building",0)==-1 &&
          owner->tech_reset_value("improvement",0)==-1 && !owner->tech_pregame_minor() &&
          owner->tech_pregame_civilization()==10 && owner->tech_pregame_leader()==0,
          "Real native Flavor Init, seven reset arrays including tech hole, and SAME Pregame source values");
    const std::vector<std::string_view> tech_init_order{
        "CvPlayerTechs::CvFlavorRecipient::Init zeros", "CvPlayerTechs::m_bIsCity false",
        "CvPlayerTechs::m_pTechs SAME source collection", "CvPlayerTechs::m_pPlayer SAME allocation",
        "CvPlayerTechs::allocate researching", "CvPlayerTechs::allocate civ priority",
        "CvPlayerTechs::allocate locale priority", "CvPlayerTechs::allocate resources",
        "CvPlayerTechs::allocate unique units", "CvPlayerTechs::allocate unique buildings",
        "CvPlayerTechs::allocate unique improvements", "CvTechAI::constructor SAME PlayerTechs",
        "CvPlayerTechs::Reset GameBuildings pointer"};
    const auto tech_begin=std::find(trace.begin(),trace.end(),tech_init_order.front());
    check(tech_begin!=trace.end() && std::distance(tech_begin,trace.end())>=static_cast<std::ptrdiff_t>(tech_init_order.size()) &&
          std::equal(tech_init_order.begin(),tech_init_order.end(),tech_begin) &&
          std::count(trace.begin(),trace.end(),"CvPlayerTechs::Reset tech slot false/1/1/-1/-1/-1/-1")==3 &&
          std::count(trace.begin(),trace.end(),"CvTechAI::Reset push native slot zero")==3 &&
          std::find(trace.begin(),trace.end(),"m_pPlayerTechs->Init")<std::find(trace.begin(),trace.end(),"CvPlayerPolicies::CvFlavorRecipient::Init zeros"),
          "Actual Init allocation order then per-slot Reset and all-slot TechAI Reset");
    check(owner->policy_latest_flavors().size()==3 && owner->policy_weights().size()==5 &&
          owner->policy_scalar("is_city")==0 && owner->policy_scalar("extra_branches")==0 &&
          owner->policy_scalar("branch_picked1")==-1 && owner->policy_scalar("branch_picked2")==-1 &&
          owner->policy_scalar("branch_picked3")==-1 && owner->policy_scalar("propagation_levels")==-3 &&
          owner->policy_scalar("percent_drop_new_branch")==101,
          "PlayerPolicies owns SAME native flavors, Reset scalars and actual selected cached Defines");
    for(std::size_t i=0;i<5;++i) {
        check(!owner->policy_flag("has_policy",i) && !owner->policy_flag("one_shot",i) &&
              !owner->policy_flag("one_shot_free_units",i) &&
              owner->policy_weights()[i]==std::pair<std::int32_t,std::int32_t>{static_cast<std::int32_t>(i),0},
              "All native policy slots including NULL holes Reset and receive zero AI weights");
    }
    for(std::size_t i=0;i<7;++i)
        check(!owner->policy_flag("branch_unlocked",i) && !owner->policy_flag("branch_blocked",i) &&
              !owner->policy_flag("branch_finished",i) && owner->policy_reset_value("branch_chosen",i)==-1,
              "All native branch slots including NULL holes Reset false/false/false/-1");
    check(owner->policy_reset_value("blocked_branch",0)==0 && owner->policy_reset_value("blocked_branch",1)==-1 &&
          owner->policy_reset_value("blocked_branch",2)==2 && owner->policy_reset_value("blocked_branch",3)==-1 &&
          owner->policy_reset_value("blocked_branch",4)==-1,
          "First matching FreePolicy wins, explicit branch wins, holes and finishing-only policy remain-1");
    bool policy_extent_refused=false;
    try {static_cast<void>(owner->policy_reset_value("blocked_branch",5));}
    catch(const std::out_of_range&) {policy_extent_refused=true;}
    check(policy_extent_refused && owner->policy_reset_value("branch_chosen",6)==-1,
          "BlockedBranchCheck allocation is Policies5, not Branches7");
    const std::vector<std::string_view> policy_init_order{
        "CvPlayerPolicies::CvFlavorRecipient::Init zeros", "CvPlayerPolicies::m_bIsCity false",
        "CvPlayerPolicies::m_pPolicies SAME collection", "CvPlayerPolicies::m_pPlayer SAME allocation",
        "CvPlayerPolicies::allocate HasPolicy", "CvPlayerPolicies::allocate OneShotPolicyFired",
        "CvPlayerPolicies::allocate OneShotFreeUnitsFired", "CvPlayerPolicies::allocate BranchUnlocked",
        "CvPlayerPolicies::allocate BranchBlocked", "CvPlayerPolicies::allocate BranchFinished",
        "CvPlayerPolicies::allocate BranchesChosen", "CvPlayerPolicies::allocate BlockedBranchCheck",
        "CvPolicyAI::constructor SAME PlayerPolicies"};
    const auto policy_begin=std::find(trace.begin(),trace.end(),policy_init_order.front());
    check(policy_begin!=trace.end() && std::distance(policy_begin,trace.end())>=static_cast<std::ptrdiff_t>(policy_init_order.size()) &&
          std::equal(policy_init_order.begin(),policy_init_order.end(),policy_begin),
          "Native PlayerPolicies eight allocations and PolicyAI constructor preserve source order");
    const auto levels=std::find(trace.begin(),trace.end(),"CvPolicyAI::Reset cached PropagationLevels");
    const auto percent=std::find(trace.begin(),trace.end(),"CvPolicyAI::Reset cached PercentDropNewBranch");
    const auto weights_begin=std::find(trace.begin(),trace.end(),"CvPolicyAI::Reset push native slot zero");
    const auto mapping_begin=std::find(trace.begin(),trace.end(),"CvPlayerPolicies::Reset final native blocked branch");
    check(levels<percent && percent<weights_begin && weights_begin<mapping_begin &&
          std::count(trace.begin(),trace.end(),"CvPlayerPolicies::Reset policy slot false/false/false/-2")==5 &&
          std::count(trace.begin(),trace.end(),"CvPlayerPolicies::Reset branch slot false/false/false/-1")==7 &&
          std::count(trace.begin(),trace.end(),"CvPolicyAI::Reset push native slot zero")==5 &&
          std::count(trace.begin(),trace.end(),"CvPlayerPolicies::Reset final native blocked branch")==5 &&
          trace.back()=="CvTacticalAI::Reset MapNULL",
          "Transient-2 Reset then scalars, AI clear/Defines/weights precede final mapping");
    selected.rules->apply_sql("DELETE FROM Defines WHERE Name IN('POLICY_WEIGHT_PROPAGATION_LEVELS','POLICY_WEIGHT_PERCENT_DROP_NEW_BRANCH');");
    const auto zero_policy_defines=draft_bnw_fresh_gamecore(selected,inputs);
    check(zero_policy_defines.first_player_state && zero_policy_defines.first_player_state->policy_scalar("propagation_levels")==0 &&
          zero_policy_defines.first_player_state->policy_scalar("percent_drop_new_branch")==0,
          "Missing cached Defines retain native BNW GetDefineINT0, not constructor2/75");
    selected.rules->apply_sql("INSERT INTO Defines VALUES('POLICY_WEIGHT_PROPAGATION_LEVELS','-3'); INSERT INTO Defines VALUES('POLICY_WEIGHT_PERCENT_DROP_NEW_BRANCH','101');");
    selected.rules->apply_sql("UPDATE Policies SET PolicyBranchType='UNKNOWN_BRANCH' WHERE ID=2;");
    const auto unknown_policy_branch=draft_bnw_fresh_gamecore(selected,inputs);
    check(unknown_policy_branch.first_player_state && unknown_policy_branch.first_player_state->policy_reset_value("blocked_branch",2)==6,
          "Unknown global enum resolves-1 then actual first FreePolicy opener lookup executes");
    selected.rules->apply_sql("UPDATE Policies SET PolicyBranchType='BRANCH_SECOND' WHERE ID=2; DELETE FROM PolicyBranchTypes WHERE ID=0;");
    const auto next_opener=draft_bnw_fresh_gamecore(selected,inputs);
    check(next_opener.first_player_state && next_opener.first_player_state->policy_reset_value("blocked_branch",0)==2,
          "NULL first branch slot skips and next real opener wins");
    selected.rules->apply_sql("INSERT INTO PolicyBranchTypes VALUES(0,'BRANCH_FIRST','POLICY_ZERO','POLICY_FINISHER'); DELETE FROM Policies; DELETE FROM PolicyBranchTypes;");
    const auto empty_policies=draft_bnw_fresh_gamecore(selected,inputs);
    check(empty_policies.first_player_state && empty_policies.first_player_state->policy_weights().empty() &&
          empty_policies.first_player_state->policy_scalar("extra_branches")==0 &&
          empty_policies.first_player_state->policy_latest_flavors().size()==3,
          "Present allocated-empty policy and branch arrays retain scalars and real Flavor Init");
    selected.rules->apply_sql("INSERT INTO Policies VALUES(1048576,'OVER_POLICY_BUDGET',NULL);");
    const auto large_policy=draft_bnw_fresh_gamecore(selected,inputs);
    check(large_policy.frontier && !large_policy.first_player_state,
          "Oversized native policy span refuses before initialized owner publication");
    selected.rules->apply_sql("DELETE FROM Policies; INSERT INTO Policies VALUES(0,'POLICY_ZERO',NULL); INSERT INTO Policies VALUES(2,'POLICY_SECOND','BRANCH_SECOND'); INSERT INTO Policies VALUES(4,'POLICY_FINISHER',NULL); DROP TABLE PolicyBranchTypes;");
    const auto missing_branches=draft_bnw_fresh_gamecore(selected,inputs);
    check(missing_branches.frontier && missing_branches.frontier->message=="PlayerPolicies selected source collections unavailable" &&
          !missing_branches.first_player_state,"Missing branch table cannot issue empty native GamePolicies");
    selected.rules->apply_sql("CREATE TABLE PolicyBranchTypes(ID INTEGER PRIMARY KEY,Type TEXT,FreePolicy TEXT,FreeFinishingPolicy TEXT); INSERT INTO PolicyBranchTypes VALUES(0,'BRANCH_FIRST','POLICY_ZERO','POLICY_FINISHER'); INSERT INTO PolicyBranchTypes VALUES(2,'BRANCH_SECOND','POLICY_ZERO',NULL); INSERT INTO PolicyBranchTypes VALUES(6,'BRANCH_THIRD','POLICY_SECOND',NULL);");

    selected.rules->apply_sql(R"sql(
        INSERT INTO Civilization_BuildingClassOverrides VALUES('CIV_TEN','BCLASS_ZERO','BUILDING_SECOND');
        INSERT INTO Civilization_UnitClassOverrides VALUES('CIV_TEN','UCLASS_ZERO','UNIT_ZERO');
        UPDATE Buildings SET PrereqTech='TECH_ZERO' WHERE ID=2;
        UPDATE Units SET PrereqTech='TECH_ZERO' WHERE ID=0;
        UPDATE Improvements SET SpecificCivRequired=1,CivilizationType='CIV_TEN' WHERE ID=0;
        UPDATE Builds SET ImprovementType='IMPROVEMENT_ZERO',PrereqTech='TECH_ZERO' WHERE ID=0;
        INSERT INTO Builds VALUES(2,'BUILD_SECOND','IMPROVEMENT_ZERO','TECH_ZERO');
        INSERT INTO Traits VALUES(0,'TRAIT_MAYA',1,'TECH_ZERO');
        INSERT INTO Leader_Traits VALUES('LEADER_ZERO','TRAIT_MAYA');
    )sql");
    const auto unique=draft_bnw_fresh_gamecore(selected,inputs);
    check(unique.first_player_state && unique.first_player_state->tech_reset_value("civ_priority",0)==31250 &&
          unique.first_player_state->tech_reset_value("building",0)==2 &&
          unique.first_player_state->tech_reset_value("unit",0)==0 &&
          unique.first_player_state->tech_reset_value("improvement",0)==0 &&
          unique.first_player_state->tech_reset_value("civ_priority",1)==1 &&
          unique.first_player_state->tech_weights()[2]==std::pair<std::int32_t,std::int32_t>{2,0},
          "Building then unit then BOTH matching Builds multiply5 four times then Maya50; holes stay1");
    const auto unique_trace=unique.first_player_state->source_order();
    check(std::count(unique_trace.begin(),unique_trace.end(),"CvPlayerTechs::Reset reached native priority multiply")==5,
          "Repeated matching build iteration is not replaced with one multiply");
    selected.rules->apply_sql("UPDATE Defines SET Value='2147483647' WHERE Name='TECH_PRIORITY_UNIQUE_ITEM';");
    const auto overflow=draft_bnw_fresh_gamecore(selected,inputs);
    check(overflow.frontier && overflow.frontier->message=="PlayerTechs native int32 unavailable" &&
          !overflow.first_player_state && overflow.source_order_prefix.empty(),
          "Second reached native priority multiplication overflow cannot publish a partial owner");
    selected.rules->apply_sql("UPDATE Defines SET Value='0' WHERE Name='TECH_PRIORITY_UNIQUE_ITEM';");
    const auto zero_factor=draft_bnw_fresh_gamecore(selected,inputs);
    check(zero_factor.first_player_state && zero_factor.first_player_state->tech_reset_value("civ_priority",0)==0 &&
          zero_factor.first_player_state->tech_reset_value("improvement",0)==0,
          "A real zero cached Define multiplies to0 and does not skip enum assignment");
    selected.rules->apply_sql(R"sql(
        UPDATE Defines SET Value='5' WHERE Name='TECH_PRIORITY_UNIQUE_ITEM';
        DELETE FROM Civilization_BuildingClassOverrides; DELETE FROM Civilization_UnitClassOverrides;
        UPDATE Buildings SET PrereqTech=NULL; UPDATE Units SET PrereqTech=NULL;
        UPDATE Improvements SET SpecificCivRequired=0,CivilizationType=NULL;
        UPDATE Builds SET ImprovementType=NULL,PrereqTech=NULL; DELETE FROM Builds WHERE ID=2;
        DELETE FROM Traits; DELETE FROM Leader_Traits;
        INSERT INTO Civilization_BuildingClassOverrides VALUES('CIV_TEN','BCLASS_ZERO',NULL);
    )sql");
    const auto null_override=draft_bnw_fresh_gamecore(selected,inputs);
    check(null_override.first_player_state && null_override.first_player_state->tech_reset_value("civ_priority",0)==1 &&
          null_override.first_player_state->tech_reset_value("building",0)==-1,
          "Actual NULL civilization override resolves NO_BUILDING and skips prereq lookup");
    selected.rules->apply_sql("DELETE FROM Civilization_BuildingClassOverrides; INSERT INTO Technologies VALUES(1048576,'TOO_WIDE');");
    const auto too_wide_tech=draft_bnw_fresh_gamecore(selected,inputs);
    check(too_wide_tech.frontier && !too_wide_tech.first_player_state,
          "Unsafe native technology slot extent refuses without fabricated arrays");
    selected.rules->apply_sql("DELETE FROM Technologies WHERE ID=1048576;");

    selected.rules->apply_sql("DROP TABLE Beliefs;");
    const auto missing_beliefs=draft_bnw_fresh_gamecore(selected,inputs);
    check(missing_beliefs.frontier && missing_beliefs.frontier->code==BnwFreshGameBuilderBlockerCode::diagnostic_exception &&
          missing_beliefs.frontier->message=="ReligionAI Init selected Beliefs unavailable" &&
          !missing_beliefs.first_player_state && missing_beliefs.source_order_prefix.empty(),
          "Missing selected Beliefs cannot fabricate a native collection binding");
    selected.rules->apply_sql("CREATE TABLE Beliefs(ID INTEGER PRIMARY KEY, Type TEXT);");
    const auto empty_beliefs=draft_bnw_fresh_gamecore(selected,inputs);
    check(empty_beliefs.first_player_state && empty_beliefs.first_player_state->religion_ai_belief_slots()==0 &&
          empty_beliefs.frontier && empty_beliefs.frontier->code==BnwFreshGameBuilderBlockerCode::tactical_game_analysis_map_source_pending,
          "Present empty Beliefs collection permits source Init with no invented entries");
    selected.rules->apply_sql("INSERT INTO Beliefs VALUES(1048576,'OVER_BUDGET');");
    const auto large_beliefs=draft_bnw_fresh_gamecore(selected,inputs);
    check(large_beliefs.frontier && large_beliefs.frontier->message=="ReligionAI Init selected Beliefs unavailable" &&
          !large_beliefs.first_player_state,"Oversized selected collection refuses before native pointer publication");
    selected.rules->apply_sql("DELETE FROM Beliefs; INSERT INTO Beliefs VALUES(0,'ROW_0'); INSERT INTO Beliefs VALUES(4,'BELIEF_SECOND');");
    selected.rules->apply_sql("DROP TABLE AIGrandStrategies;");
    const auto missing_grand=draft_bnw_fresh_gamecore(selected,inputs);
    check(missing_grand.frontier && missing_grand.frontier->code==BnwFreshGameBuilderBlockerCode::diagnostic_exception &&
          missing_grand.frontier->message=="GrandStrategy Init selected Info unavailable" &&
          !missing_grand.first_player_state && missing_grand.source_order_prefix.empty(),
          "Missing actual Grand Info discards unpublished owned prefix without a fabricated empty Init");
    selected.rules->apply_sql("CREATE TABLE AIGrandStrategies(ID INTEGER PRIMARY KEY, Type TEXT);");
    const auto empty_grand=draft_bnw_fresh_gamecore(selected,inputs);
    check(empty_grand.first_player_state && empty_grand.first_player_state->grand_strategy_priorities().empty() &&
          empty_grand.first_player_state->grand_strategy_guesses().size()==22 &&
          empty_grand.first_player_state->grand_strategy_active()==-1,
          "Present empty Info collection permits zero priority array, retains native major arrays");
    selected.rules->apply_sql("INSERT INTO AIGrandStrategies VALUES(1048576,'OVER_BUDGET');");
    const auto oversized_grand=draft_bnw_fresh_gamecore(selected,inputs);
    check(oversized_grand.frontier && oversized_grand.frontier->code==BnwFreshGameBuilderBlockerCode::diagnostic_exception &&
          oversized_grand.frontier->message=="GrandStrategy Init selected Info unavailable" && !oversized_grand.first_player_state,
          "Oversized native Info span refuses before Grand allocation and publication");
    selected.rules->apply_sql("DELETE FROM AIGrandStrategies; INSERT INTO AIGrandStrategies VALUES(0,'ROW_0'); INSERT INTO AIGrandStrategies VALUES(2,'STRATEGY_SECOND');");
    // Native rowid count differs from Prefetch slots for EMPTY and non-alias ID.
    selected.rules->apply_sql("DELETE FROM Flavors;");
    const auto empty_flavors=draft_bnw_fresh_gamecore(selected,inputs);
    check(empty_flavors.first_player_state && empty_flavors.first_player_state->wonder_latest_flavors().size()==1 &&
          empty_flavors.economic.players.front().temp_flavors.size()==1 &&
          empty_flavors.military.players.front().temp_flavors.size()==1,
          "Native max(rowid) aggregate NULL/GetInt0/add1 allocates one flavor cell");
    selected.rules->apply_sql("DROP TABLE Flavors; CREATE TABLE Flavors(ID INT PRIMARY KEY,Type TEXT); INSERT INTO Flavors VALUES(0,'FLAVOR_ZERO'); INSERT INTO Flavors VALUES(1,'FLAVOR_ONE');");
    const auto nonalias_flavors=draft_bnw_fresh_gamecore(selected,inputs);
    check(nonalias_flavors.first_player_state && nonalias_flavors.first_player_state->wonder_latest_flavors().size()==3 &&
          nonalias_flavors.economic.players.front().temp_flavors.size()==3 &&
          nonalias_flavors.military.players.front().temp_flavors.size()==3 &&
          selected.rules->prefetch_info("Flavors").slots==2,
          "INT PRIMARY KEY does not alias rowid: native rows1,2 allocate3 versus Info slots2");
    selected.rules->apply_sql("DROP TABLE Flavors; CREATE TABLE Flavors(ID INTEGER PRIMARY KEY,Type TEXT); INSERT INTO Flavors VALUES(0,'FLAVOR_ZERO'); INSERT INTO Flavors VALUES(4,'FLAVOR_FOUR');");
    const auto sparse_flavors=draft_bnw_fresh_gamecore(selected,inputs);
    check(sparse_flavors.first_player_state && sparse_flavors.first_player_state->wonder_latest_flavors().size()==5 &&
          selected.rules->prefetch_info("Flavors").slots==5,
          "Nonempty standard INTEGER PRIMARY KEY schema aliases rowid and sparse extent is5");
    selected.rules->apply_sql("INSERT INTO Flavors VALUES(2147483647,'NATIVE_ADD_OVERFLOW');");
    const auto overflow_flavors=draft_bnw_fresh_gamecore(selected,inputs);
    check(!overflow_flavors.first_player_state && overflow_flavors.frontier,
          "Reached GetInt(maxrowid)+1 overflow cannot initialize an owned native array");
    selected.rules->apply_sql("DELETE FROM Flavors; INSERT INTO Flavors VALUES(0,'ROW_0'); INSERT INTO Flavors VALUES(2,'FLAVOR_SECOND');");

    // The new PlayerTechs flavor recipient shares the actual SAME GC count.
    selected.rules->apply_sql("DELETE FROM Flavors;");
    const auto tech_empty_flavors=draft_bnw_fresh_gamecore(selected,inputs);
    check(tech_empty_flavors.first_player_state && tech_empty_flavors.first_player_state->tech_latest_flavors().size()==1,
          "PlayerTechs native empty Flavor MaxRows is1, not zero Info slots");
    selected.rules->apply_sql("DROP TABLE Flavors; CREATE TABLE Flavors(ID INT PRIMARY KEY,Type TEXT); INSERT INTO Flavors VALUES(0,'FLAVOR_ZERO'); INSERT INTO Flavors VALUES(1,'FLAVOR_ONE');");
    const auto tech_nonalias_flavors=draft_bnw_fresh_gamecore(selected,inputs);
    check(tech_nonalias_flavors.first_player_state && tech_nonalias_flavors.first_player_state->tech_latest_flavors().size()==3,
          "PlayerTechs reads native rowid count3 although authored Flavor slots are2");
    selected.rules->apply_sql("DROP TABLE Flavors; CREATE TABLE Flavors(ID INTEGER PRIMARY KEY,Type TEXT); INSERT INTO Flavors VALUES(0,'ROW_0'); INSERT INTO Flavors VALUES(2,'FLAVOR_SECOND');");
    selected.rules->apply_sql("DROP TABLE BuildingClasses; CREATE TABLE BuildingClasses(ID INT PRIMARY KEY,Type TEXT,DefaultBuilding TEXT); INSERT INTO BuildingClasses VALUES(0,'BCLASS_ZERO','BUILDING_ZERO'); INSERT INTO BuildingClasses VALUES(3,'BCLASS_THREE',NULL);");
    const auto unsafe_class=draft_bnw_fresh_gamecore(selected,inputs);
    check(unsafe_class.frontier && unsafe_class.frontier->message=="PlayerTechs reached building class read unavailable" &&
          !unsafe_class.first_player_state,
          "Actual rowids1,2 give class allocation3; native Info loop reaches slot3 and must refuse");
    selected.rules->apply_sql("DROP TABLE BuildingClasses; CREATE TABLE BuildingClasses(ID INTEGER PRIMARY KEY,Type TEXT,DefaultBuilding TEXT); INSERT INTO BuildingClasses VALUES(0,'BCLASS_ZERO','BUILDING_ZERO');");
    selected.rules->apply_sql("DROP TABLE UnitClasses; CREATE TABLE UnitClasses(ID INT PRIMARY KEY,Type TEXT,DefaultUnit TEXT); INSERT INTO UnitClasses VALUES(0,'UCLASS_ZERO','UNIT_ZERO'); INSERT INTO UnitClasses VALUES(3,'UCLASS_THREE',NULL);");
    const auto unsafe_unitclass=draft_bnw_fresh_gamecore(selected,inputs);
    check(unsafe_unitclass.frontier && unsafe_unitclass.frontier->message=="PlayerTechs reached unit class read unavailable" &&
          !unsafe_unitclass.first_player_state,"Unit class allocation uses rowid and checks native Info slot before flag read");
    selected.rules->apply_sql("DROP TABLE UnitClasses; CREATE TABLE UnitClasses(ID INTEGER PRIMARY KEY,Type TEXT,DefaultUnit TEXT); INSERT INTO UnitClasses VALUES(0,'UCLASS_ZERO','UNIT_ZERO');");
    selected.rules->apply_sql("DROP TABLE Traits; CREATE TABLE Traits(ID INT PRIMARY KEY,Type TEXT,MayaCalendarBonuses INTEGER,PrereqTech TEXT); INSERT INTO Traits VALUES(0,'TRAIT_ZERO',0,NULL); INSERT INTO Traits VALUES(3,'TRAIT_THREE',0,NULL);");
    const auto unsafe_traits=draft_bnw_fresh_gamecore(selected,inputs);
    check(unsafe_traits.frontier && unsafe_traits.frontier->message=="PlayerTechs reached Leader hasTrait read unavailable" &&
          !unsafe_traits.first_player_state,"Leader trait flags use native rowid allocation and cannot guess absent out-of-range flag false");
    selected.rules->apply_sql("DROP TABLE Traits; CREATE TABLE Traits(ID INTEGER PRIMARY KEY,Type TEXT,MayaCalendarBonuses INTEGER,PrereqTech TEXT);");
    auto wrong_roster = draft.roster;
    wrong_roster.slots[0].status = BnwFreshSlotStatus::closed;
    auto rejected_roster = draft_bnw_fresh_builder_player_diagnostic(
        wrong_roster,draft.first_player_objects,draft.first_player_uninit,
        draft.first_resource_vector,draft.selected_rules_sha256,constants);
    check(!rejected_roster.diagnostic_complete &&
          rejected_roster.blocker ==
              BnwFreshBuilderPlayerDiagnosticBlocker::
                  roster_or_selected_rules_invalid &&
          !rejected_roster.source_prefix_advanced,
          "mismatched first-major roster must reject detached handoff");
    auto wrong_objects = draft.first_player_objects;
    wrong_objects.source_order[2].step =
        BnwFreshFirstPlayerObjectStep::reset_sync_archive_pending;
    auto rejected_allocations = draft_bnw_fresh_builder_player_diagnostic(
        draft.roster,wrong_objects,draft.first_player_uninit,
        draft.first_resource_vector,draft.selected_rules_sha256,constants);
    check(!rejected_allocations.diagnostic_complete &&
          rejected_allocations.blocker ==
              BnwFreshBuilderPlayerDiagnosticBlocker::
                  roster_or_selected_rules_invalid,
          "both constructor allocations must precede first player Uninit");
    wrong_roster = draft.roster;
    wrong_roster.source_order[0].index = 2;
    auto rejected_team_order = draft_bnw_fresh_builder_player_diagnostic(
        wrong_roster,draft.first_player_objects,draft.first_player_uninit,
        draft.first_resource_vector,draft.selected_rules_sha256,constants);
    check(!rejected_team_order.diagnostic_complete &&
          rejected_team_order.blocker ==
              BnwFreshBuilderPlayerDiagnosticBlocker::
                  roster_or_selected_rules_invalid,
          "first team reset event must match the selected roster team");
    wrong_roster = draft.roster;
    wrong_roster.selected_rules_sha256 = "different-rules";
    auto rejected_rules = draft_bnw_fresh_builder_player_diagnostic(
        wrong_roster,draft.first_player_objects,draft.first_player_uninit,
        draft.first_resource_vector,draft.selected_rules_sha256,constants);
    check(!rejected_rules.diagnostic_complete &&
          rejected_rules.blocker ==
              BnwFreshBuilderPlayerDiagnosticBlocker::
                  roster_or_selected_rules_invalid,
          "selected RulesDatabase identity must bind the handoff");
    auto wrong_resource = draft.first_resource_vector;
    wrong_resource.state.pending_commands = 0;
    auto rejected_archive = draft_bnw_fresh_builder_player_diagnostic(
        draft.roster,draft.first_player_objects,draft.first_player_uninit,
        wrong_resource,draft.selected_rules_sha256,constants);
    check(!rejected_archive.diagnostic_complete &&
          rejected_archive.blocker ==
              BnwFreshBuilderPlayerDiagnosticBlocker::
                  player_archive_preflight_invalid,
          "first archive member command count must match before diagnostics");
    selected.rules->apply_sql("DROP TABLE CitySpecializations;");
    auto refused = draft_bnw_fresh_gamecore(selected,inputs);
    check(refused.frontier && refused.frontier->code ==
              BnwFreshGameBuilderBlockerCode::city_specialization_ai &&
              !refused.detached_diagnostics_complete && !refused.first_player_state &&
              refused.roster.slots.empty() && refused.team_info.teams.empty() &&
              refused.slot_info.slots.empty() &&
              !refused.first_player_objects.constructors_complete &&
              !refused.first_player_uninit.preflight_complete &&
              !refused.first_resource_vector.member_effect_complete &&
              !refused.first_player_diagnostic.diagnostic_complete &&
              refused.player_info.players.empty() &&
              refused.source_order_prefix.empty(),
          "A failed diagnostic stage exposes no partial builder state");
    draft.rollback();
    check(draft.roster.slots.empty() && draft.team_info.teams.empty() &&
              draft.source_order_prefix.empty() && !draft.frontier &&
              !draft.detached_diagnostics_complete && !draft.first_player_state &&
              scalar("m_eID")==0 && owner->wonder_weights().size()==3 &&
              owner->wonder_latest_flavors().size()==3 &&
              owner->grand_strategy_priorities().size()==3 && owner->grand_strategy_guesses().size()==22 &&
              owner->grand_strategy_active()==-1 && owner->diplomacy_scalar("m_iBoldness")==-1 &&
              owner->diplomacy_array_value("m_ppaaeOtherPlayerVictoryDisputeLevel",62,62)==-1 &&
              owner->religions_prophets_spawned()==0 && owner->religions_founding_religion()==0 &&
              owner->religion_ai_belief_slots()==5 && owner->tech_reset_value("civ_priority",1)==1 &&
              owner->tech_weights()[2]==std::pair<std::int32_t,std::int32_t>{2,0} &&
              owner->policy_weights()[4]==std::pair<std::int32_t,std::int32_t>{4,0} &&
              owner->policy_reset_value("blocked_branch",0)==0 &&
              owner->tactical_reset_value("current_zone_index")==-1 &&
              owner->tactical_reset_value("move_priority_turn")==-1 &&
              owner->tactical_reset_map_is_null() &&
              slots.front().status == BnwFreshSlotStatus::taken,
          "Rollback discards owned diagnostics without changing inputs");
}
} // namespace

int main() { test_atomic_selected_fresh_diagnostics(); }
