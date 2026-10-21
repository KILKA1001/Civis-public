#pragma once
#include "civ5/runtime/vp_espionage_turn.hpp"
#include "civ5/runtime/vp_espionage_player_turn.hpp"
#include "civ5/runtime/lua_script_host.hpp"

void vp_espionage_turn_tests() {
    using namespace civ5;
    using namespace civ5::runtime;
    saves::SavedSessionSnapshot snapshot;
    snapshot.all_players.emplace();
    snapshot.all_players->layout=saves::GameStateLayout::vp542_corpus;
    saves::SavedPlayerObservation saved;
    saved.saved_player_id=5000000001LL;
    saved.espionage.layout=saves::GameStateLayout::vp542_corpus;
    saves::PlayerEspionageRecord espionage{};
    espionage.name="player_espionage";
    saves::PlayerEspionageRecord spy{};
    spy.name="spy";
    spy.blobs.push_back({"m_sName",{'S','p','y'}});
    spy.fields={{"m_eRank",2},{"m_iExperience",3000000001LL},{"m_iCityX",-1},
        {"m_iCityY",-1},{"m_eSpyState",0},{"m_eSpyFocus",-1},
        {"m_iReviveCounter",0},{"m_bIsDiplomat",0},
        {"m_eVassalDiplomatPlayer",-1},{"m_bEvaluateReassignment",1},
        {"m_bPassive",0},{"m_iTurnCounterspyMissionChanged",0},
        {"m_iTurnActiveMissionConducted",0}};
    saves::PlayerEspionageRecord notice{};notice.name="spy_notification";
    notice.fields={{"m_iCityX",12},{"m_iCityY",41},{"m_eAttackingPlayer",3},
        {"m_iSpyResult",5},{"m_iAmountStolen",3000000002LL},
        {"m_eStolenTech",7},{"m_iMission",8},{"m_iGWID",9}};
    saves::PlayerEspionageRecord intrigue{};intrigue.name="intrigue";
    intrigue.fields={{"m_eDiscoveringPlayer",1},{"m_eSourcePlayer",2},
        {"m_eTargetPlayer",3},{"m_eDiplomacyPlayer",4},{"m_eBuilding",5},
        {"m_eProject",6},{"m_eUnit",7},{"m_iIntrigueType",8},{"m_iTurnNum",9},
        {"m_iCityX",12},{"m_iCityY",41},{"m_bShared",1},{"iSpyID",5000000003LL}};
    intrigue.blobs.push_back({"m_strSpyName",{'S',0,'p','y'}});
    espionage.children={spy,notice,intrigue};
    saved.espionage.blocks={espionage};
    const auto core=[&](const char *name,std::int64_t value) {
        saves::PlayerCoreField field{};field.name=name;field.values={value};
        saved.core.fields.push_back(std::move(field));
    };
    const auto trait=[&](const char *name,std::int64_t value) {
        saves::PlayerTraitsField field{};field.name=name;field.values={value};
        saved.traits.fields.push_back(std::move(field));
    };
    core("m_iEspionageNetworkPoints",7);core("m_iSpySecurityModifier",3);
    trait("m_iSpyExtraRankBonus",1);trait("m_iSpyOffensiveStrengthModifier",-10);
    saves::PlayerStrategyBlock policies{};policies.name="policies";
    policies.arrays.push_back({"policy_modifiers",1,std::vector<std::int64_t>(60,0)});
    policies.arrays.back().values[48]=25;
    saved.strategy.blocks.push_back(std::move(policies));
    saves::PlayerTradeCultureRecord culture{};culture.name="player_culture";
    culture.arrays.push_back({"m_aiCulturalInfluenceTimes100",1,{12,34}});
    saved.trade_culture.blocks.push_back(std::move(culture));
    snapshot.all_players->players={saved};
    ResearchContext world;
    world.profile=content::TechnologyProfile::vp542;
    world.players.emplace(saved.saved_player_id,ResearchPlayerState{});
    restore_vp_espionage_profile(snapshot,world);
    const auto &profile=*find_vp_espionage_profile(world);
    const auto &imported=profile.players.at(saved.saved_player_id);
    check(imported.spies[0].experience==3000000001LL&&
          imported.extra_rank_bonus==1&&imported.network_points_bonus==7&&
          imported.policy_faster_modifier==25&&imported.security_modifier==3&&
          imported.offensive_strength_modifier==-10&&
          imported.cultural_influence_times100.at(1)==34&&
          imported.spy_messages.size()==1&&
          imported.spy_messages[0].amount_stolen==3000000002LL&&
          imported.intrigue.size()==1&&imported.intrigue[0].spy_id==5000000003LL&&
          imported.intrigue[0].spy_name==std::string("S\0py",4)&&
          profile.players.at(saved.saved_player_id).spies[0].name=="Spy"&&
          vp_espionage_turn_frontier(world,saved.saved_player_id).queued_messages==1,
          "VP spy import must preserve widened state and typed notification queues");
    auto duplicate_world=world;
    duplicate_world.game_turn=5000000000LL;
    auto &duplicate_messages=ensure_vp_espionage_profile(duplicate_world)
        .players.at(saved.saved_player_id).intrigue;
    duplicate_messages.clear();
    VpIntrigue prior;prior.source_player=7;prior.target_player=-1;
    prior.type=1;prior.turn=4999999971LL;
    duplicate_messages.push_back(prior);
    check(vp_recent_building_army_intrigue(duplicate_world,saved.saved_player_id,7,false)&&
          !vp_recent_building_army_intrigue(duplicate_world,saved.saved_player_id,7,true)&&
          !vp_recent_building_army_intrigue(duplicate_world,saved.saved_player_id,8,false),
          "VP building-army duplicate uses source, target, type and a wide game turn");
    duplicate_messages.back().turn=4999999970LL;
    check(!vp_recent_building_army_intrigue(duplicate_world,saved.saved_player_id,7,false),
          "VP building-army intrigue repeats again at the 30-turn boundary");
    auto queue_world=world;
    content::CustomModOptions options;
    options.entries["EVENTS_ESPIONAGE"].value=1;
    VpSpyNotification added=imported.spy_messages.front();
    added.amount_stolen=5000000004LL;
    LuaScriptHost host;
    host.execute("spy",R"lua(
        calls=0
        GameEvents.EspionageNotificationData.Add(function(x,y,attacker,defender,result,tech,amount)
            assert(x==12 and y==41 and attacker==3 and defender==5000000001)
            assert(result==5 and tech==7 and amount==5000000004)
            calls=calls+1
            return true
        end)
    )lua","=vp-espionage-event.lua");
    check(vp_add_spy_message(queue_world,saved.saved_player_id,added,options,&host).hook_errors.empty(),
          "VP spy queue must dispatch the enabled data-driven espionage Lua hook");
    host.execute("spy","assert(calls==1)","=vp-espionage-hook-count.lua");
    auto &messages=ensure_vp_espionage_profile(queue_world).players.at(saved.saved_player_id).spy_messages;
    check(messages.size()==2&&messages.back().amount_stolen==5000000004LL,
          "VP spy hook must precede queue append without dropping the message");
    check(vp_update_spy_message(queue_world,saved.saved_player_id,12,41,3,19,
        6000000005LL,20)&&messages[0].stolen_tech==19&&
        messages[0].amount_stolen==6000000005LL&&messages[0].great_work==20&&
        messages[1].amount_stolen==5000000004LL,
        "VP spy update must change only the first matching queued message");
    check(!vp_update_spy_message(queue_world,saved.saved_player_id,99,41,3,1,2,3)&&
          messages.size()==2,"Unmatched VP spy update must leave the queue alone");
    rejects([&]{(void)vp_add_spy_message(queue_world,saved.saved_player_id,added,options,nullptr);});
    check(messages.size()==2,"Missing enabled VP Lua host must not append a message");
    options.entries["EVENTS_ESPIONAGE"].value=0;
    check(vp_add_spy_message(queue_world,saved.saved_player_id,added,options,nullptr).hook_errors.empty()&&
          messages.size()==3,"Disabled VP Lua event must allow queueing without a host");
    options.entries["EVENTS_ESPIONAGE"].value=1;
    LuaScriptHost faulty;
    faulty.execute("spy","GameEvents.EspionageNotificationData.Add(function() error('spy hook') end)",
                   "=vp-espionage-fault.lua");
    check(vp_add_spy_message(queue_world,saved.saved_player_id,added,options,&faulty).hook_errors.size()==1&&
          messages.size()==4,"Lua listener error must be reported while VP still queues its message");
    auto copy=world;
    auto &copied=ensure_vp_espionage_profile(copy).players.at(saved.saved_player_id);
    copied.spy_messages.clear();copied.intrigue.clear();copied.spies[0].state=3;
    const auto active=vp_espionage_turn_frontier(copy,saved.saved_player_id);
    check(!active.safe_no_gameplay_effect&&active.first_spy_needing_host==0&&
          active.first_spy_state==3&&profile.players.at(saved.saved_player_id).spies[0].state==0,
          "VP active spy must require its world host without changing original copy");
    copied.spy_messages.push_back({});
    const auto queued=vp_espionage_turn_frontier(copy,saved.saved_player_id);
    check(queued.queued_messages==1&&!queued.first_spy_needing_host&&
          queued.unresolved.find("notification")!=std::string::npos,
          "VP notification queue must precede spy processing");
    copied.spy_messages.clear();copied.spies[0].state=9;
    check(vp_espionage_turn_frontier(copy,saved.saved_player_id).safe_no_gameplay_effect,
          "Terminated VP spy must stay idle");
    snapshot.all_players->players[0].espionage.blocks[0].children[0].fields[4].value=10;
    rejects([&]{restore_vp_espionage_profile(snapshot,world);});
    check(find_vp_espionage_profile(world)->players.at(saved.saved_player_id).spies[0].state==0,
          "Failed VP espionage import must preserve previous state");
    ResearchContext vanilla;
    rejects([&]{(void)ensure_vp_espionage_profile(vanilla);});
    rejects([&]{(void)vp_add_spy_message(vanilla,saved.saved_player_id,added,options,&host);});
    check(find_vp_espionage_profile(vanilla)==nullptr,
          "Vanilla must not require VP espionage state");
    saves::SavedSessionSnapshot operation_snapshot;
    operation_snapshot.all_players.emplace();
    operation_snapshot.all_players->layout=saves::GameStateLayout::vp542_corpus;
    saves::SavedPlayerObservation operation_player;
    operation_player.saved_player_id=7;
    operation_player.army_operations.layout=saves::GameStateLayout::vp542_corpus;
    const auto operation_node=[](const char *name,std::int64_t value) {
        saves::ArmyOperationsNode node{};node.name=name;node.values={value};return node;
    };
    const auto attack_operation=[&](std::int64_t type,std::int64_t id,std::int64_t owner) {
        saves::ArmyOperationsNode base{};base.name="CvAIOperation";
        base.children={operation_node("m_iID",id),operation_node("m_eOwner",owner),
            operation_node("m_eEnemy",11),operation_node("m_eCurrentState",2),
            operation_node("m_iTargetX",31),operation_node("m_iTargetY",59)};
        saves::ArmyOperationsNode operation{};operation.name="operation";
        operation.children={operation_node("operation_type",type),base};
        return operation;
    };
    operation_player.army_operations.operations={attack_operation(4,3000000001LL,7),
        attack_operation(2,3000000001LL,7),attack_operation(9,1,7),
        attack_operation(3,3000000002LL,7)};
    operation_snapshot.all_players->players.push_back(operation_player);
    ResearchContext operation_world;operation_world.profile=content::TechnologyProfile::vp542;
    operation_world.players.emplace(7,ResearchPlayerState{});
    restore_vp_attack_operations_profile(operation_snapshot,operation_world);
    const auto &attacks=find_vp_attack_operations_profile(operation_world)->players.at(7);
    check(attacks.size()==3&&attacks[0].type==4&&attacks[0].id==3000000001LL&&
          attacks[1].type==2&&attacks[1].id==3000000001LL&&attacks[2].source_index==3&&
          attacks[2].target_x==31&&attacks[2].target_y==59,
          "VP attack operation import preserves source order, duplicate wide IDs and targets");
    check(vp_first_intrigue_attack_operation(operation_world,7,11)==&attacks[0]&&
          vp_first_intrigue_attack_operation(operation_world,7,12)==nullptr,
          "VP intrigue selects combined attack before earlier land attacks for the same enemy");
    auto lookup_world=operation_world;
    auto &lookup_attacks=ensure_vp_attack_operations_profile(lookup_world).players.at(7);
    lookup_attacks[0].state=0;
    check(vp_first_intrigue_attack_operation(lookup_world,7,11)==&lookup_attacks[2],
          "VP intrigue excludes aborted combined attack and tries naval before land");
    lookup_attacks[2].state=0;
    check(vp_first_intrigue_attack_operation(lookup_world,7,11)==&lookup_attacks[1],
          "VP intrigue falls back to the land attack");
    lookup_attacks[1].state=0;
    check(vp_first_intrigue_attack_operation(lookup_world,7,11)==nullptr,
          "VP intrigue ignores all aborted attack operations");
    auto bad_operation_snapshot=operation_snapshot;
    bad_operation_snapshot.all_players->players[0].army_operations.operations[1]
        .children[1].children[1].values[0]=8;
    rejects([&]{restore_vp_attack_operations_profile(bad_operation_snapshot,operation_world);});
    check(find_vp_attack_operations_profile(operation_world)->players.at(7).size()==3,
          "Failed VP operation import must preserve the live profile");
    rejects([&]{restore_vp_attack_operations_profile(operation_snapshot,vanilla);});
    check(find_vp_attack_operations_profile(vanilla)==nullptr,
          "Vanilla must not require VP attack operations");
    auto army_snapshot=operation_snapshot;
    const auto army_record=[&](std::int64_t type,std::int64_t id,
                              std::int64_t unit,std::int64_t required) {
        saves::ArmyOperationsNode slot{};slot.name="slot";
        slot.children={operation_node("m_iUnitID",unit),
            operation_node("m_bIsRequired",required)};
        saves::ArmyOperationsNode entries{};entries.name="m_FormationEntries";
        entries.children={slot};
        saves::ArmyOperationsNode army{};army.name="army";
        army.children={operation_node("m_iID",id),operation_node("m_eOwner",7),
            operation_node("m_eType",type),entries};
        return army;
    };
    army_snapshot.all_players->players[0].army_operations.armies={
        army_record(0,4000000001LL,-1,1),army_record(1,4000000001LL,-1,0),
        army_record(2,4000000002LL,-2,1)};
    restore_vp_army_construction_profile(army_snapshot,operation_world);
    const auto &armies=find_vp_army_construction_profile(operation_world)->players.at(7);
    check(armies.size()==3&&armies[0].id==4000000001LL&&
          armies[1].id==4000000001LL&&armies[2].source_index==2&&
          vp_is_building_army(operation_world,7,VpArmyType::land)&&
          !vp_is_building_army(operation_world,7,VpArmyType::naval)&&
          !vp_is_building_army(operation_world,7,VpArmyType::combined),
          "VP IsBuildingArmy uses required free slots and preserves wide repeated army IDs");
    auto live_armies=operation_world;
    ensure_vp_army_construction_profile(live_armies).players.at(7)[0].slots[0].unit_id=11;
    check(!vp_is_building_army(live_armies,7,VpArmyType::land)&&
          vp_is_building_army(operation_world,7,VpArmyType::land),
          "VP IsBuildingArmy queries live profile slots rather than frozen save bytes");
    auto bad_army_snapshot=army_snapshot;
    bad_army_snapshot.all_players->players[0].army_operations.armies[0]
        .children[3].children[0].children[1].values[0]=2;
    rejects([&]{restore_vp_army_construction_profile(bad_army_snapshot,operation_world);});
    check(vp_is_building_army(operation_world,7,VpArmyType::land),
          "Failed VP army import must preserve the previous live state");
    rejects([&]{restore_vp_army_construction_profile(army_snapshot,vanilla);});
    check(find_vp_army_construction_profile(vanilla)==nullptr,
          "Vanilla must not require VP army construction state");
    auto segment_world=world;
    CityYieldContext segment_cities;segment_cities.profile=content::TechnologyProfile::vp542;
    PlayerResearchContext diplomacy;diplomacy.profile=segment_cities.profile;
    TreasuryContext treasury;treasury.profile=segment_cities.profile;
    TreasuryRules treasury_rules;treasury_rules.profile=segment_cities.profile;
    content::RulesDatabase database;
    database.apply_sql("CREATE TABLE Defines(Name TEXT COLLATE NOCASE,Value);"
        "INSERT INTO Defines VALUES('ESPIONAGE_XP_PER_TURN_COUNTERSPY',0),"
        "('BALANCE_SPY_RESPAWN_TIMER',5000000000);");
    auto defines=database.cache_global_defines(content::GlobalDefineProfile::vp542);
    content::CustomModOptions segment_options;
    segment_options.entries["BALANCE_VP"].value=1;
    const auto segment=[&]{return process_vp_espionage_player_turn_segment(
        segment_world,segment_cities,diplomacy,{},treasury,treasury_rules,database,
        defines,segment_options,saved.saved_player_id);};
    const auto blocked_messages=segment();
    check(!blocked_messages.complete&&blocked_messages.processed_spies==0&&
          blocked_messages.unresolved.find("notification")!=std::string::npos,
          "Queued VP messages must block before processing any spy");
    auto &segment_spies=ensure_vp_espionage_profile(segment_world)
        .players.at(saved.saved_player_id);
    segment_spies.spy_messages.clear();
    VpSpy counter;counter.state=5;segment_spies.spies.push_back(counter);
    VpSpy dead;dead.state=8;dead.revive_counter=3000000000LL;
    segment_spies.spies.push_back(dead);
    VpSpy terminated;terminated.state=9;segment_spies.spies.push_back(terminated);
    const auto completed=segment();
    check(completed.complete&&completed.processed_spies==4&&
          segment_spies.spies[2].revive_counter==3000000001LL,
          "VP spy phase must process idle, zero-XP counterspy, dead and terminated states in order with wide counters");
    database.apply_sql("UPDATE Defines SET Value=1 WHERE Name='ESPIONAGE_XP_PER_TURN_COUNTERSPY'");
    defines=database.cache_global_defines(content::GlobalDefineProfile::vp542);
    const auto blocked_xp=segment();
    check(!blocked_xp.complete&&blocked_xp.processed_spies==1&&blocked_xp.blocking_spy==1&&
          segment_spies.spies[2].revive_counter==3000000001LL,
          "VP counterspy XP must block before a later dead spy changes");
    database.apply_sql("UPDATE Defines SET Value=0 WHERE Name='ESPIONAGE_XP_PER_TURN_COUNTERSPY';"
        "UPDATE Defines SET Value=3000000002 WHERE Name='BALANCE_SPY_RESPAWN_TIMER'");
    defines=database.cache_global_defines(content::GlobalDefineProfile::vp542);
    const auto blocked_revive=segment();
    check(!blocked_revive.complete&&blocked_revive.processed_spies==2&&
          blocked_revive.blocking_spy==2&&
          segment_spies.spies[2].revive_counter==3000000001LL,
          "VP revival threshold must block before changing the spy without name and notification hosts");
    rejects([&]{(void)process_vp_espionage_player_turn_segment(vanilla,segment_cities,
        diplomacy,{},treasury,treasury_rules,database,defines,segment_options,saved.saved_player_id);});
}
