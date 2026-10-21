#pragma once
#include "civ5/runtime/vp_network_point_binding.hpp"
#include "civ5/runtime/vp_gathering_intel_turn.hpp"
#include "civ5/runtime/vp_schmooze_turn.hpp"
#include "civ5/runtime/vp_espionage_player_turn.hpp"
#include "civ5/runtime/vp_culture_history_turn.hpp"
#include "civ5/runtime/vp_espionage_turn.hpp"
#include "civ5/runtime/vp_treasury_religion_profile.hpp"
#include "civ5/saves/vp_game_trade_profile.hpp"

void vp_network_point_binding_tests() {
    using namespace civ5;
    using namespace civ5::runtime;
    constexpr auto vp=content::TechnologyProfile::vp542;
    constexpr std::int64_t city_id=5000000001LL;
    ResearchContext setup;setup.profile=vp;setup.game_handicap=0;
    setup.players[0].team=0;setup.players[0].slot_status=3;setup.players[0].alive=true;
    setup.players[1].team=1;setup.players[1].handicap=1;
    setup.players[1].slot_status=1;setup.players[1].alive=true;
    auto &spies=ensure_vp_espionage_profile(setup).players;
    spies[0].extra_rank_bonus=1;
    spies[0].network_points_bonus=3;
    spies[0].cultural_influence_times100[1]=5000;
    VpSpy spy;spy.rank=1;spy.state=3;spy.city_x=12;spy.city_y=7;
    spies[0].spies.push_back(spy);spies[1];
    ensure_vp_culture_history_profile(setup).players[1].lifetime_times100=10000;
    PlayerResearchContext diplomacy;diplomacy.profile=vp;
    diplomacy.major_players={0,1};diplomacy.teams[0];
    diplomacy.teams[1].met.insert(0);
    CityYieldContext cities;cities.profile=vp;cities.players[0];
    CityYieldState live;live.owner=1;live.id=city_id;live.x=12;live.y=7;live.population=10;
    cities.players[1].cities.emplace(city_id,live);
    auto &activity=ensure_vp_city_espionage(cities,1,city_id);
    activity.fields["m_aiSpyAssignment"]={{0,0},{1,-1}};
    for(const auto *name:{"m_aiAmount","m_aiMaxAmount","m_aiRate","m_aiGoal"})
        activity.fields[name][0]=0;
    std::vector<TeamTechnologies> teams;
    teams.emplace_back(vp,0,8);teams.emplace_back(vp,1,8);
    MaintenanceWorld world{setup,diplomacy,cities,teams};
    TreasuryContext treasury;treasury.profile=vp;
    ensure_vp_treasury_religion_profile(treasury.players[0]);treasury.players[1];
    TreasuryRules treasury_rules;treasury_rules.profile=vp;
    content::RulesDatabase db;
    db.apply_sql("CREATE TABLE Defines(Name TEXT COLLATE NOCASE,Value);"
        "INSERT INTO Defines VALUES('ESPIONAGE_NP_BASE',50),"
        "('ESPIONAGE_SECURITY_BASE',30),('ESPIONAGE_SECURITY_PER_POPULATION',-2);"
        "CREATE TABLE HandicapInfos(ID INTEGER PRIMARY KEY,SpySecurityModifier INTEGER);"
        "INSERT INTO HandicapInfos VALUES(0,0),(1,0);"
        "CREATE TABLE CityEventChoices(ID INTEGER PRIMARY KEY,CounterspyNPRateReduction INTEGER,"
        "SpySecurityModifier INTEGER);INSERT INTO CityEventChoices VALUES(9,4,5);"
        "CREATE TABLE Beliefs(ID INTEGER PRIMARY KEY,EspionageNetworkPoints INTEGER);"
        "INSERT INTO Beliefs VALUES(1,7);");
    auto defines=db.cache_global_defines(content::GlobalDefineProfile::vp542);
    content::CustomModOptions options;
    options.entries["BALANCE_VP"]={1,1};
    const auto target=vp_spy_city_binding(cities,0,0,12,7);
    check(target&&target->city==city_id,"VP wide city identity must bind before spy rate");
    const auto bind=[&] {return bind_vp_network_point_rate_inputs(world,treasury,
        treasury_rules,db,defines,options,0,0,*target);};
    const auto rate=[&](const VpNetworkPointRateInputs &inputs) {
        return calculate_vp_network_point_rate(inputs,load_vp_network_point_rate_rules(defines)).rate;};
    auto inputs=bind();
    check(inputs.effective_spy_rank==2&&inputs.influence_level==2&&
          inputs.player_bonus==3&&inputs.religion_bonus==0&&inputs.city_security==10&&
          inputs.counterspy_resolved&&!inputs.counterspy&&rate(inputs)==61,
        "VP rate must bind profile-owned rank, culture, player bonus and city security to one wide live city");
    spies[0].policy_faster_modifier=25;
    check(rate(bind())==76,"A live VP policy update must change the next spy rate");
    spies[0].policy_faster_modifier=0;
    activity.security_modifier=5;
    check(bind().city_security==15&&rate(bind())==57,
        "A live VP city-security update must change the next spy rate");
    activity.security_modifier=0;
    VpSpy guard;guard.rank=1;guard.state=5;guard.focus=9;
    spies[1].spies.push_back(guard);activity.fields["m_aiSpyAssignment"][1]=0;
    inputs=bind();
    check(inputs.counterspy&&inputs.counterspy->focus_reduction==4&&
          inputs.city_security==15&&rate(inputs)==51,
        "Selected counterspy mission must change both NP rate and city security");
    auto &faith=ensure_vp_treasury_religion_profile(treasury.players[0]);
    // The saved state religion, not holy-city ownership, selects the religion.
    faith.state_religion=2;
    TreasuryReligion religion;religion.religion=2;religion.belief_religion=2;religion.founder=0;
    religion.holy_plot_owner=0;religion.beliefs={1};religion.belief_lookup={1};
    treasury.religions.push_back(religion);
    treasury_rules.beliefs[1].founder=true;
    inputs=bind();
    check(inputs.religion_bonus==7&&rate(inputs)==56,
        "VP espionage religion bonus must use selected belief and holy-plot owner");
    db.apply_sql("UPDATE Beliefs SET EspionageNetworkPoints=14 WHERE ID=1");
    inputs=bind();
    check(inputs.religion_bonus==14&&rate(inputs)==62,
        "XML/SQL belief changes must reach VP spy rate without a C++ mod port");
    db.apply_sql("INSERT INTO Defines VALUES('ESPIONAGE_SECURITY_PREVIOUS_CITY_MISSIONS',6);"
        "CREATE TABLE GameSpeeds(ID INTEGER PRIMARY KEY,SpyRatePercent INTEGER);"
        "INSERT INTO GameSpeeds VALUES(2,200);");
    defines=db.cache_global_defines(content::GlobalDefineProfile::vp542);
    setup.speed=2;
    activity.previous_spy_missions=2;
    saves::TradeRouteObservation route;
    route.origin_owner=1;route.destination_owner=0;
    auto &route_profile=saves::ensure_vp_trade_route(route);
    route_profile.origin_id=city_id;route_profile.destination_id=0;
    treasury.trade_routes.push_back(std::move(route));
    inputs=bind();
    check(inputs.city_security==16&&rate(inputs)==62,
        "VP city security must use selected spy speed and wide trade-route city IDs");
    check(activity.value("m_aiRate",0)==0,
        "Read-only live rate binding must not advance the city or overwrite saved activity");
    db.apply_sql("INSERT INTO Defines VALUES"
        "('ESPIONAGE_NP_PER_SPY_RANK',0),('ESPIONAGE_NP_CULTURAL_INFLUENCE',0),"
        "('ESPIONAGE_NP_PER_TECHNOLOGY_BEHIND',0),('ESPIONAGE_NP_MAX_NUM_TECH',0),"
        "('ESPIONAGE_NP_REDUCTION_PER_SECURITY_POINT',100),"
        "('ESPIONAGE_XP_PER_TURN_OFFENSIVE',0);"
        "ALTER TABLE GameSpeeds ADD COLUMN TrainPercent INTEGER DEFAULT 100;"
        "ALTER TABLE CityEventChoices ADD COLUMN NetworkPointsNeeded INTEGER DEFAULT 0;"
        "ALTER TABLE CityEventChoices ADD COLUMN NetworkPointsScaling INTEGER DEFAULT 0;"
        "ALTER TABLE CityEventChoices ADD COLUMN IsEspionageMission INTEGER DEFAULT 0;"
        "CREATE TABLE SpyPassiveBonuses(ID INTEGER PRIMARY KEY,NetworkPointsNeeded INTEGER,"
        "NetworkPointsScaling INTEGER,SciencePercentAdded INTEGER,TilesRevealed INTEGER,"
        "RevealCityScreen INTEGER);");
    defines=db.cache_global_defines(content::GlobalDefineProfile::vp542);
    activity.fields["m_aiLastProgress"][0]=0;
    activity.fields["m_abRevealCityScreen"][0]=0;
    activity.next_passive_bonus[0]={true,{}};
    const auto step=process_vp_gathering_intel_without_effect_host(world,cities,treasury,
        treasury_rules,db,defines,options,0,0);
    check(step.city==city_id&&step.target_owner==1&&step.rate.rate>0&&
          step.network_points&&step.network_points->after==step.rate.rate&&
          activity.value("m_aiAmount",0)==step.rate.rate&&
          activity.value("m_aiRate",0)==step.rate.rate&&
          activity.value("m_aiLastProgress",0)==step.rate.rate,
          "VP gathering-intel step must advance the same live city with its selected modded rate");
    auto amount=activity.value("m_aiAmount",0);
    activity.set("m_abRevealCityScreen",0,1);
    rejects([&]{(void)process_vp_gathering_intel_without_effect_host(world,cities,treasury,
        treasury_rules,db,defines,options,0,0);});
    check(activity.value("m_aiAmount",0)==amount,
          "Missing production queue must not be treated as no wonder");
    auto &target_city=cities.players.at(1).cities.at(city_id);
    target_city.production_orders=std::vector<CityYieldState::ProductionOrder>{{1,5}};
    db.apply_sql("CREATE TABLE BuildingClasses(Type TEXT PRIMARY KEY,MaxGlobalInstances INTEGER);"
        "INSERT INTO BuildingClasses VALUES('BUILDINGCLASS_WONDER',1);"
        "CREATE TABLE Buildings(ID INTEGER PRIMARY KEY,BuildingClass TEXT);"
        "INSERT INTO Buildings VALUES(5,'BUILDINGCLASS_WONDER');");
    rejects([&]{(void)process_vp_gathering_intel_without_effect_host(world,cities,treasury,
        treasury_rules,db,defines,options,0,0);});
    check(activity.value("m_aiAmount",0)==amount,
          "Actual world-wonder production must await VP intrigue and notification hosts");
    db.apply_sql("UPDATE BuildingClasses SET MaxGlobalInstances=-1");
    const auto ordinary=process_vp_gathering_intel_without_effect_host(world,cities,treasury,
        treasury_rules,db,defines,options,0,0);
    check(ordinary.network_points&&ordinary.network_points->after>amount,
          "An ordinary building must not block VP spy network progress");
    amount=activity.value("m_aiAmount",0);
    target_city.production_orders=std::vector<CityYieldState::ProductionOrder>{{2,7}};
    db.apply_sql("CREATE TABLE Projects(ID INTEGER PRIMARY KEY,IsRepeatable INTEGER);"
        "INSERT INTO Projects VALUES(7,0);");
    rejects([&]{(void)process_vp_gathering_intel_without_effect_host(world,cities,treasury,
        treasury_rules,db,defines,options,0,0);});
    check(activity.value("m_aiAmount",0)==amount,
          "Nonrepeatable project must await VP intrigue and notification hosts");
    db.apply_sql("UPDATE Projects SET IsRepeatable=1 WHERE ID=7");
    check(process_vp_gathering_intel_without_effect_host(world,cities,treasury,
        treasury_rules,db,defines,options,0,0).network_points.has_value(),
          "Selected SQL must distinguish repeatable projects without a C++ mod port");
    amount=activity.value("m_aiAmount",0);
    const SessionCheckpointIdentity identity{std::string(64,'a'),std::string(64,'b')};
    const auto section=make_vp_espionage_checkpoint(setup,cities,identity);
    auto restored_setup=setup;auto restored_cities=cities;
    ensure_vp_city_espionage(restored_cities,1,city_id).set("m_aiAmount",0,0);
    restore_vp_espionage_checkpoint(section,restored_setup,restored_cities,identity);
    check(find_vp_city_espionage(restored_cities,1,city_id)->value("m_aiAmount",0)==amount&&
          find_vp_city_espionage(restored_cities,1,city_id)->value("m_aiLastProgress",0)==step.rate.rate,
          "Applied VP gathering-intel progress must survive the profile checkpoint");
    auto incomplete_cities=cities;
    auto &incomplete_activity=ensure_vp_city_espionage(incomplete_cities,1,city_id);
    incomplete_activity.fields.erase("m_aiLastProgress");
    const MaintenanceWorld incomplete_world{setup,diplomacy,incomplete_cities,teams};
    rejects([&]{(void)process_vp_gathering_intel_without_effect_host(incomplete_world,
        incomplete_cities,treasury,treasury_rules,db,defines,options,0,0);});
    check(incomplete_activity.value("m_aiAmount",0)==amount,
          "Missing live VP activity field must fail before mutating network points");
    activity.set("m_abRevealCityScreen",0,0);
    db.apply_sql("UPDATE Defines SET Value=1 WHERE Name='ESPIONAGE_XP_PER_TURN_OFFENSIVE'");
    defines=db.cache_global_defines(content::GlobalDefineProfile::vp542);
    rejects([&]{(void)process_vp_gathering_intel_without_effect_host(world,cities,treasury,
        treasury_rules,db,defines,options,0,0);});
    check(activity.value("m_aiAmount",0)==amount,
          "Unbound VP spy promotion must prevent partial network progress");
    db.apply_sql("UPDATE Defines SET Value=0 WHERE Name='ESPIONAGE_XP_PER_TURN_OFFENSIVE'");
    defines=db.cache_global_defines(content::GlobalDefineProfile::vp542);
    db.apply_sql("INSERT INTO SpyPassiveBonuses VALUES(1,"+
        std::to_string(amount+1)+",0,1,0,0)");
    activity.next_passive_bonus[0]={false,{}};
    rejects([&]{(void)process_vp_gathering_intel_without_effect_host(world,cities,treasury,
        treasury_rules,db,defines,options,0,0);});
    check(activity.value("m_aiAmount",0)==amount,
          "Crossing VP passive threshold must await its effect host without partial mutation");
    spies[0].spies[0].state=7;spies[0].spies[0].diplomat=true;
    activity.fields["m_abDiplomatReceiveIntrigues"][0]=0;
    activity.next_passive_bonus[0]={false,{}};
    db.apply_sql("INSERT INTO Defines VALUES('ESPIONAGE_XP_PER_TURN_DIPLOMAT',0);"
        "CREATE TABLE SpyPassiveBonusesDiplomat(ID INTEGER PRIMARY KEY,"
        "NetworkPointsNeeded INTEGER,NetworkPointsScaling INTEGER);"
        "INSERT INTO SpyPassiveBonusesDiplomat VALUES(1,"+
        std::to_string(amount+1000000)+",0)");
    defines=db.cache_global_defines(content::GlobalDefineProfile::vp542);
    const auto diplomat=process_vp_schmooze_without_effect_host(world,cities,treasury,
        treasury_rules,db,defines,options,0,0);
    check(diplomat.network_points&&diplomat.rate.rate>0&&
          diplomat.network_points->before==amount&&
          diplomat.network_points->after==activity.value("m_aiAmount",0),
          "VP diplomat must advance live city through its separate passive rule table");
    amount=activity.value("m_aiAmount",0);
    const auto diplomat_segment=process_vp_espionage_player_turn_segment(setup,cities,
        diplomacy,teams,treasury,treasury_rules,db,defines,options,0);
    check(diplomat_segment.complete&&diplomat_segment.processed_spies==1&&
          diplomat_segment.diplomat_steps.size()==1&&
          activity.value("m_aiAmount",0)>amount,
          "VP ordered espionage phase must execute a safe diplomat step");
    amount=activity.value("m_aiAmount",0);
    activity.set("m_abDiplomatReceiveIntrigues",0,1);
    const auto intrigue_step=process_vp_schmooze_without_effect_host(world,cities,treasury,
        treasury_rules,db,defines,options,0,0);
    check(intrigue_step.network_points&&intrigue_step.network_points->before==amount&&
          intrigue_step.network_points->after==activity.value("m_aiAmount",0)&&
          intrigue_step.network_points->after>amount&&
          intrigue_step.unresolved.find("attack operation profile")!=std::string::npos&&
          !intrigue_step.no_sneak_attack_operations,
          "VP diplomat must earn safe network points before unresolved intrigue");
    amount=activity.value("m_aiAmount",0);
    ensure_vp_attack_operations_profile(setup).players.emplace(0,std::vector<VpAttackOperation>{});
    auto &army_profile=ensure_vp_army_construction_profile(setup).players;
    army_profile[0]={};
    VpArmyConstruction construction;construction.owner=1;construction.type=0;
    construction.slots={{-1,true}};
    army_profile[1].push_back(construction);
    const auto intrigue_segment=process_vp_espionage_player_turn_segment(setup,cities,
        diplomacy,teams,treasury,treasury_rules,db,defines,options,0);
    check(!intrigue_segment.complete&&intrigue_segment.processed_spies==0&&
          intrigue_segment.blocking_spy==0&&intrigue_segment.blocking_state==7&&
          intrigue_segment.diplomat_steps.size()==1&&
          intrigue_segment.diplomat_steps[0].network_points&&
          intrigue_segment.diplomat_steps[0].network_points->before==amount&&
          intrigue_segment.diplomat_steps[0].no_sneak_attack_operations&&
          intrigue_segment.diplomat_steps[0].target_army_count==1&&
          intrigue_segment.diplomat_steps[0].building_land_army==true&&
          intrigue_segment.diplomat_steps[0].building_naval_army==false&&
          intrigue_segment.diplomat_steps[0].target_human_for_units==false&&
          intrigue_segment.diplomat_steps[0].military_might_branch_skipped&&
          intrigue_segment.diplomat_steps[0].building_army_intrigue_candidate=="land"&&
          intrigue_segment.diplomat_steps[0].building_army_intrigue_duplicate==false&&
          intrigue_segment.unresolved.find("AddIntrigueMessage")!=std::string::npos&&
          activity.value("m_aiAmount",0)>amount,
          "Blocked VP diplomat must rule out empty sneak-attack branch and report network progress");
    amount=activity.value("m_aiAmount",0);
    auto duplicate_setup=setup;duplicate_setup.game_turn=100;
    VpIntrigue repeated;repeated.source_player=1;repeated.target_player=-1;
    repeated.type=1;repeated.turn=71;
    ensure_vp_espionage_profile(duplicate_setup).players.at(0).intrigue.push_back(repeated);
    auto duplicate_cities=cities;
    ensure_vp_city_intrigue_order_profile(duplicate_cities).players[0]={};
    ensure_vp_city_intrigue_order_profile(duplicate_cities).players[1]={city_id};
    MaintenanceWorld duplicate_world{duplicate_setup,diplomacy,duplicate_cities,teams};
    const auto duplicate_intrigue=process_vp_schmooze_without_effect_host(
        duplicate_world,duplicate_cities,treasury,treasury_rules,db,defines,options,0,0);
    check(duplicate_intrigue.building_army_intrigue_duplicate==true&&
          duplicate_intrigue.construction_scan&&
          duplicate_intrigue.construction_scan->scanned_cities==1&&
          !duplicate_intrigue.construction_scan->candidate&&
          duplicate_intrigue.unresolved.find("diplomacy")!=std::string::npos&&
          ensure_vp_espionage_profile(duplicate_setup).players.at(0).intrigue.size()==1,
          "Duplicate VP building-army intrigue skips a repeatable project before diplomacy");
    db.apply_sql("UPDATE Projects SET IsRepeatable=0 WHERE ID=7");
    const auto construction_intrigue=process_vp_schmooze_without_effect_host(
        duplicate_world,duplicate_cities,treasury,treasury_rules,db,defines,options,0,0);
    check(construction_intrigue.construction_scan&&
          construction_intrigue.construction_scan->candidate&&
          construction_intrigue.construction_scan->candidate->city==city_id&&
          construction_intrigue.construction_scan->candidate->order_type==2&&
          construction_intrigue.unresolved.find("AddIntrigueMessage")!=std::string::npos,
          "Modded nonrepeatable city project must reach VP construction intrigue host");
    db.apply_sql("UPDATE Projects SET IsRepeatable=1 WHERE ID=7");
    auto human_setup=setup;human_setup.players[1].slot_status=3;
    auto human_cities=cities;
    MaintenanceWorld human_world{human_setup,diplomacy,human_cities,teams};
    const auto human_intrigue=process_vp_schmooze_without_effect_host(human_world,human_cities,
        treasury,treasury_rules,db,defines,options,0,0);
    check(human_intrigue.target_human_for_units==true&&
          human_intrigue.building_army_intrigue_candidate.empty()&&
          human_intrigue.unresolved.find("military history")!=std::string::npos,
          "VP human unit control suppresses building-army intrigue candidate");
    auto ai_control_options=options;
    ai_control_options.entries["AI_CONTROL_UNITS"]={1,1};
    const auto ai_control_intrigue=process_vp_schmooze_without_effect_host(human_world,human_cities,
        treasury,treasury_rules,db,defines,ai_control_options,0,0);
    check(ai_control_intrigue.target_human_for_units==false&&
          ai_control_intrigue.building_army_intrigue_candidate=="land",
          "VP AI_CONTROL_UNITS restores the building-army intrigue branch for a human player");
    activity.set("m_abDiplomatReceiveIntrigues",0,0);
    db.apply_sql("UPDATE Defines SET Value=1 WHERE Name='ESPIONAGE_XP_PER_TURN_DIPLOMAT'");
    defines=db.cache_global_defines(content::GlobalDefineProfile::vp542);
    const auto experience_step=process_vp_schmooze_without_effect_host(world,cities,treasury,
        treasury_rules,db,defines,options,0,0);
    check(experience_step.network_points&&experience_step.network_points->before==amount&&
          activity.value("m_aiAmount",0)>amount&&
          experience_step.unresolved.find("experience")!=std::string::npos,
          "Diplomat XP must block after safe network progress until promotion has a host");
    db.apply_sql("UPDATE Defines SET Value=0 WHERE Name='ESPIONAGE_XP_PER_TURN_DIPLOMAT'");
    defines=db.cache_global_defines(content::GlobalDefineProfile::vp542);
    amount=activity.value("m_aiAmount",0);
    db.apply_sql("INSERT INTO SpyPassiveBonusesDiplomat VALUES(2,"+
        std::to_string(amount+1)+",0)");
    activity.next_passive_bonus[0]={false,{}};
    rejects([&]{(void)process_vp_schmooze_without_effect_host(world,cities,treasury,
        treasury_rules,db,defines,options,0,0);});
    check(activity.value("m_aiAmount",0)==amount,
          "Diplomat passive unlock must await its effects without partial progress");
    cities.profile=content::TechnologyProfile::bnw;
    rejects([&]{(void)bind();});
    rejects([&]{(void)process_vp_gathering_intel_without_effect_host(world,cities,treasury,
        treasury_rules,db,defines,options,0,0);});
}
