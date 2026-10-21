#pragma once
#include "civ5/runtime/vp_network_points.hpp"

void vp_network_points_tests() {
    using namespace civ5;
    using namespace civ5::runtime;
    content::RulesDatabase db;
    db.apply_sql("CREATE TABLE GameSpeeds(ID INTEGER PRIMARY KEY,TrainPercent INTEGER);"
        "INSERT INTO GameSpeeds VALUES(7,150);"
        "CREATE TABLE SpyPassiveBonuses(ID INTEGER PRIMARY KEY,NetworkPointsNeeded INTEGER,"
        "NetworkPointsScaling INTEGER,SciencePercentAdded INTEGER,TilesRevealed INTEGER,"
        "RevealCityScreen INTEGER);"
        "INSERT INTO SpyPassiveBonuses VALUES(0,100,0,5,0,1);"
        "INSERT INTO SpyPassiveBonuses VALUES(1,200,1,2,1,0);"
        "CREATE TABLE SpyPassiveBonusesDiplomat(ID INTEGER PRIMARY KEY,"
        "NetworkPointsNeeded INTEGER,NetworkPointsScaling INTEGER);"
        "INSERT INTO SpyPassiveBonusesDiplomat VALUES(0,100,0),(1,200,1);"
        "CREATE TABLE CityEventChoices(ID INTEGER PRIMARY KEY,NetworkPointsNeeded INTEGER,"
        "NetworkPointsScaling INTEGER,IsEspionageMission INTEGER);"
        "INSERT INTO CityEventChoices VALUES(4,400,1,1);");
    // Modded XML extends the same selected SQL table without a C++ port.
    db.apply_mod_xml(xml("<GameData><SpyPassiveBonuses><Row>"
        "<ID>2</ID><NetworkPointsNeeded>900</NetworkPointsNeeded>"
        "<NetworkPointsScaling>0</NetworkPointsScaling>"
        "<SciencePercentAdded>7</SciencePercentAdded>"
        "</Row></SpyPassiveBonuses></GameData>"));
    ResearchContext world;world.profile=content::TechnologyProfile::vp542;world.speed=7;
    const auto rules=load_vp_network_point_rules(db,world);
    check(rules.selected_rules_loaded&&rules.passive.size()==3&&
          rules.passive[1].threshold==300&&rules.passive[2].threshold==900&&
          rules.missions.size()==1&&rules.missions[0].threshold==600,
          "VP network point thresholds must follow selected SQL/XML and game speed");
    VpCityEspionageState activity;
    activity.fields["m_aiAmount"][0]=250;
    activity.fields["m_aiMaxAmount"][0]=250;
    activity.fields["m_aiRate"][0]=0;
    activity.fields["m_aiLastProgress"][0]=0;
    activity.next_passive_bonus[0]={false,{}};
    const auto safe=apply_vp_network_points_without_effect_host(activity,0,10,rules);
    check(safe.safe_without_effect_host&&safe.after==260&&safe.next_passive_bonus==300&&
          activity.value("m_aiAmount",0)==260&&activity.value("m_aiRate",0)==10&&
          activity.next_passive_bonus.at(0).threshold==300,
          "VP network points must apply source progress and initialize next bonus");
    const auto crossing=preview_vp_network_points(activity,0,50,rules);
    check(!crossing.safe_without_effect_host&&crossing.passive_bonuses_unlocked==
          std::vector<std::int64_t>{1}&&crossing.next_passive_bonus==900,
          "VP passive bonus crossing must be identified from selected rules");
    rejects([&]{(void)apply_vp_network_points_without_effect_host(activity,0,50,rules);});
    check(activity.value("m_aiAmount",0)==260,
          "Unbound VP passive world effect must leave city activity unchanged");
    activity.set("m_aiAmount",0,590);activity.set("m_aiMaxAmount",0,590);
    activity.next_passive_bonus.at(0)={true,900};
    const auto mission=preview_vp_network_points(activity,0,20,rules);
    check(!mission.safe_without_effect_host&&mission.missions_unlocked==
          std::vector<std::int64_t>{4}&&mission.passive_bonuses_unlocked.empty(),
          "VP mission notification threshold must be detected independently");
    activity.set("m_aiAmount",0,3000000000LL);
    activity.set("m_aiMaxAmount",0,3000000000LL);
    activity.next_passive_bonus.at(0)={true,{}};
    const auto wide=apply_vp_network_points_without_effect_host(activity,0,2000000000LL,rules);
    check(wide.after==5000000000LL&&activity.value("m_aiMaxAmount",0)==5000000000LL,
          "VP network points must not inherit the original 32-bit ceiling");
    const auto diplomat_rules=load_vp_diplomat_network_point_rules(db,world);
    check(diplomat_rules.passive.size()==2&&diplomat_rules.passive[1].threshold==300,
          "VP diplomat thresholds must use their own selected SQL table and game speed");
    VpCityEspionageState diplomat;
    diplomat.fields["m_aiAmount"][0]=250;
    diplomat.fields["m_aiMaxAmount"][0]=250;
    diplomat.fields["m_aiRate"][0]=0;
    diplomat.fields["m_aiLastProgress"][0]=0;
    diplomat.next_passive_bonus[0]={false,{}};
    const auto diplomat_safe=apply_vp_diplomat_network_points_without_effect_host(
        diplomat,0,10,diplomat_rules);
    check(diplomat_safe.after==260&&diplomat_safe.next_passive_bonus==300&&
          diplomat.value("m_aiLastProgress",0)==10,
          "VP diplomat points must advance activity without offensive mission checks");
    const auto diplomat_crossing=preview_vp_diplomat_network_points(
        diplomat,0,50,diplomat_rules);
    check(!diplomat_crossing.safe_without_effect_host&&
          diplomat_crossing.passive_bonuses_unlocked==std::vector<std::int64_t>{1},
          "VP diplomat bonus crossing must request its world effects");
    rejects([&]{(void)apply_vp_diplomat_network_points_without_effect_host(
        diplomat,0,50,diplomat_rules);});
    check(diplomat.value("m_aiAmount",0)==260,
          "Unbound diplomat passive bonus must not partially advance city activity");
    diplomat.set("m_aiAmount",0,3000000000LL);
    diplomat.set("m_aiMaxAmount",0,3000000000LL);
    diplomat.next_passive_bonus[0]={true,{}};
    check(apply_vp_diplomat_network_points_without_effect_host(
        diplomat,0,2000000000LL,diplomat_rules).after==5000000000LL,
          "VP diplomat points must remain wide beyond the original x86 limit");
    ResearchContext vanilla;vanilla.speed=7;
    rejects([&]{(void)load_vp_network_point_rules(db,vanilla);});
}

void vp_network_point_rate_tests() {
    using namespace civ5;
    using namespace civ5::runtime;
    content::RulesDatabase db;
    db.apply_sql("CREATE TABLE Defines(Name TEXT COLLATE NOCASE,Value);"
        "INSERT INTO Defines VALUES"
        "('ESPIONAGE_NP_BASE',50),"
        "('ESPIONAGE_NP_PER_SPY_RANK',10),"
        "('ESPIONAGE_NP_CULTURAL_INFLUENCE',15),"
        "('ESPIONAGE_NP_PER_TECHNOLOGY_BEHIND',2),"
        "('ESPIONAGE_NP_MAX_NUM_TECH',10),"
        "('ESPIONAGE_NP_REDUCTION_PER_SECURITY_POINT',100);");
    auto defines=db.cache_global_defines(content::GlobalDefineProfile::vp542);
    auto rules=load_vp_network_point_rate_rules(defines);
    VpNetworkPointRateInputs input;
    input.balance_vp=true;input.city_present=true;input.spy_state=3;
    input.effective_spy_rank=2;input.influence_level=3;
    input.target_known_techs=15;input.own_known_techs=3;
    input.player_bonus=5;input.religion_bonus=7;
    input.counterspy_resolved=true;input.counterspy=VpCounterspyNetworkReduction{4,1};
    input.policy_faster_modifier=25;input.trait_strength_modifier=-20;
    input.city_security=20;
    const auto active=calculate_vp_network_point_rate(input,rules);
    check(active.before_security==124&&active.security_reduction_percent==20&&active.rate==99,
        "VP NP rate must apply ranked spy, capped tech gap, counterspy, policy, trait and security in source order");
    input.spy_state=7;input.city_security.reset();input.counterspy_resolved=false;
    const auto diplomat=calculate_vp_network_point_rate(input,rules);
    check(diplomat.before_security==132&&diplomat.rate==132&&
          diplomat.security_reduction_percent==0,
        "VP schmooze must omit both counterspy and city security");
    input.balance_vp=false;input.religion_bonus.reset();
    check(calculate_vp_network_point_rate(input,{}).rate==0,
        "Disabled VP logic must not demand VP-only inputs or rules");
    input.balance_vp=true;
    rejects([&]{(void)calculate_vp_network_point_rate(input,rules);});
    input.religion_bonus=7;input.spy_state=3;input.city_security=20;
    rejects([&]{(void)calculate_vp_network_point_rate(input,rules);});
    input.counterspy_resolved=true;input.counterspy.reset();
    input.effective_spy_rank.reset(); // AI's theoretical city-selection query.
    check(calculate_vp_network_point_rate(input,rules).rate==89,
        "VP city selection without a spy index must omit rank and counterspy terms");
    db.apply_sql("UPDATE Defines SET Value=80 WHERE Name='ESPIONAGE_NP_BASE'");
    defines=db.cache_global_defines(content::GlobalDefineProfile::vp542);
    rules=load_vp_network_point_rate_rules(defines);
    input.effective_spy_rank=2;input.counterspy=VpCounterspyNetworkReduction{4,1};
    check(calculate_vp_network_point_rate(input,rules).rate==122,
        "Selected SQL/XML mod define changes must affect VP rate without a C++ port");
    input.spy_state=7;input.player_bonus=3000000000LL;
    input.religion_bonus=1000000000LL;
    check(calculate_vp_network_point_rate(input,rules).rate>INT32_MAX,
        "VP rate must not inherit x86 int limits");
    const auto bnw=db.cache_global_defines(content::GlobalDefineProfile::bnw);
    rejects([&]{(void)load_vp_network_point_rate_rules(bnw);});
}
