#pragma once
void vp_player_unit_production_tests(){
    using namespace civ5::runtime;using namespace civ5::saves;constexpr auto vp=TechnologyProfile::vp542;
    constexpr auto layout=GameStateLayout::vp542_corpus;constexpr std::int64_t big=4294967300LL;
    RulesDatabase db;db.apply_sql("CREATE TABLE Defines(Name,Value);INSERT INTO Defines VALUES('MAX_UNIT_SUPPLY_PRODMOD',70),('PRODUCTION_PENALTY_PER_UNIT_OVER_SUPPLY',5);"
        "CREATE TABLE Traits(ID,Type);INSERT INTO Traits VALUES(1,'T1'),(4,'T4');"
        "CREATE TABLE UnitCombatInfos(ID,Type);INSERT INTO UnitCombatInfos VALUES(2,'MELEE');"
        "CREATE TABLE SpecialUnits(ID,Type);INSERT INTO SpecialUnits VALUES(0,'SPECIAL');"
        "CREATE TABLE Units(ID,Type,MilitaryProduction,Found,CombatClass,Special);INSERT INTO Units VALUES(7,'UNIT',1,1,'MELEE','SPECIAL'),(8,'CIVILIAN',0,0,NULL,NULL),(9,'SETTLER',0,1,NULL,NULL);"
        "CREATE TABLE Unit_ProductionTraits(UnitType,TraitType,Trait);INSERT INTO Unit_ProductionTraits VALUES('UNIT','T1',7),('UNIT','T4',12);"
        "CREATE TABLE SpecialUnit_ProductionTraits(SpecialUnitType,TraitType,Trait);INSERT INTO SpecialUnit_ProductionTraits VALUES('SPECIAL','T1',3);");
    const auto types=db.cache_info_types();auto rules=load_vp_player_unit_production_rules(db,types);
    check(rules.trait_slots==5&&rules.combat_slots==3&&rules.units.at(7).trait_modifiers==std::vector<std::int64_t>{0,7,0,0,12}
        &&rules.special_trait_modifiers.at(0)==std::vector<std::int64_t>{0,3,0,0,0},"Unit production loads source-indexed trait arrays with sparse active Info IDs");
    ResearchContext players;players.profile=vp;players.players[5].alive=true;players.players[5].team=9;ensure_vp_unit_trait_profile(players).potential_by_player[5]={1,4};
    players.players[6].alive=true;players.players[6].team=10;
    PlayerResearchContext dip;dip.profile=vp;dip.major_players={5,6};dip.teams[9].at_war={10};
    CityYieldContext world;world.profile=vp;auto &city_player=world.players[5];auto &production_profile=ensure_vp_general_production_profile(world);production_profile.players[5]={};
    auto &profile=ensure_vp_unit_production_profile(world);
    auto &unit_state=profile.players[5];unit_state.military=10;unit_state.military_per_major_war=4;unit_state.settler=6;unit_state.combat[2]=5;
    MaintenanceContext maintenance;maintenance.profile=vp;ensure_vp_unit_supply_profile(maintenance).world_city_supply_modifier=0;maintenance.team_eras[9]=0;
    maintenance.players[5]={};auto &unit_supply=ensure_vp_unit_supply_profile(maintenance).players[5];unit_supply.base_cache=100;unit_supply.war_cache=100;
    VpUnitSupplyRules supply_rules;auto defines=db.cache_global_defines(GlobalDefineProfile::vp542);CustomModOptions options;
    std::vector<TeamTechnologies> techs;techs.emplace_back(vp,9,5);
    VpUnitSupplyContext supply{maintenance,players,dip,world,techs,supply_rules,db,defines,options};
    VpCityFoodRules food_rules;VpCityFoodContext food{world,players,maintenance.team_eras,food_rules,defines};
    VpCityProductionRules production_rules;VpCityProductionContext production{supply,food,production_rules};
    VpUnitTraitRules trait_rules;trait_rules.traits[1]={};trait_rules.traits[4]={};trait_rules.traits[4].prereq_policy=2;
    ResearchTurnState turn;turn.profile=vp;turn.players[5]={};
    SavedMapProjection map{{1,1}};map.cells.resize(1);map.cells[0].index=0;PlotPassability pass;PlotWaterRules water_rules;water_rules.profile=vp;
    PlotWaterState water_state;water_state.layout=layout;PlotWaterContext water{map,water_state,water_rules,types,pass,players,world,defines,&options};
    VpPlotReligionRules religion_rules;VpPlotReligionContext religion{religion_rules,water};TreasuryContext treasury;treasury.profile=vp;treasury.players[5]={};
    VpCityReligionMinorContext minor{religion,treasury};VpUnitTraitContext traits{trait_rules,players,turn,dip,techs,minor};
    VpPlayerUnitProductionContext context{rules,production,traits};
    auto q=vp_player_unit_production(context,5,7);
    check(q.general.total==0&&q.major_wars==1&&q.military==10&&q.war_military==4&&q.settler==6&&q.combat==5&&
        q.unit_traits==0&&q.special_traits==0&&q.total==25,"Inactive second trait omits both positional array bonuses");
    players.players[5].policies.insert(2);q=vp_player_unit_production(context,5,7);
    check(q.unit_traits==7&&q.special_traits==3&&q.total==35,"Active sparse trait ID four reads position one from both arrays");
    players.players[6].alive=false;check(vp_player_unit_production(context,5,7).major_wars==0&&vp_player_unit_production(context,5,7).total==31,
        "Major-war count follows the live alive enemy roster");players.players[6].alive=true;
    players.players[7].alive=true;players.players[7].team=11;dip.teams[9].at_war.insert(11);
    check(vp_player_unit_production(context,5,7).major_wars==1,"Minor war is excluded from major-war production");
    unit_state.military_per_major_war=big;check(vp_player_unit_production(context,5,7).war_military==big,
        "Live production modifiers support values beyond 32 bits");unit_state.military_per_major_war=4;
    check(vp_player_unit_production(context,5,8).total==0,"Civilian unit without traits receives only general production");
    check(vp_player_unit_production(context,5,9).settler==6&&vp_player_unit_production(context,5,9).major_wars==0,
        "Nonmilitary founding unit receives settler bonus without a military-war bonus");
    check(vp_player_unit_production(context,5,999).total==0,
        "Missing Unit Info returns player general production as in the original overload");
    production_profile.players.at(5).conquest=11;production_profile.players.at(5).conquest_turns=1;
    check(vp_player_unit_production(context,5,999).total==11,"Missing Unit Info retains nonzero general production");
    const auto saved_state=profile.players.at(5);profile.players.erase(5);
    check(vp_player_unit_production(context,5,999).total==11,"Missing Unit Info does not require unit-specific state");
    rejects([&]{vp_player_unit_production(context,5,7);});profile.players[5]=saved_state;
    production_profile.players.at(5).conquest=0;production_profile.players.at(5).conquest_turns=0;
    const auto special=rules.special_trait_modifiers.at(0);rules.special_trait_modifiers.erase(0);
    check(vp_player_unit_production(context,5,7).total==32,"Missing special Unit Info skips only the special-trait bonus");
    rules.special_trait_modifiers[0]=special;
    profile.players.at(5).military=INT64_MAX;rejects([&]{vp_player_unit_production(context,5,7);});profile.players.at(5).military=10;
    {
        SavedSessionSnapshot snap;snap.all_players.emplace();snap.all_players->layout=layout;SavedPlayerObservation sp;sp.saved_player_id=big;sp.core.layout=layout;
        const auto scalar_field=[&](const char *name,std::int64_t n){PlayerCoreField f{name,PlayerCoreEncoding::i32};f.values={n};sp.core.fields.push_back(f);};
        scalar_field("m_eID",big);scalar_field("m_iMilitaryProductionModifier",11);scalar_field("m_iMilitaryProductionModPerMajorWar",-3);scalar_field("m_iSettlerProductionModifier",8);
        PlayerCoreField combat{"m_paiUnitCombatProductionModifiers",PlayerCoreEncoding::vector_i32};combat.values={0,0,17};sp.core.fields.push_back(combat);snap.all_players->players={sp};
        CityYieldContext restored;restored.profile=vp;restored.players[big]={};restore_vp_player_unit_production(snap,rules,restored);
        const auto &a=find_vp_unit_production_profile(restored)->players.at(big);check(a.military==11&&a.military_per_major_war==-3&&a.settler==8&&a.combat.at(2)==17,
            "Unit production restore binds signed VP fields to a wide live player ID");
        auto copy=restored;ensure_vp_unit_production_profile(copy).players.at(big).military=99;
        check(find_vp_unit_production_profile(restored)->players.at(big).military==11,
            "GameCore profile extension is copied independently with a runtime snapshot");
        CityYieldContext vanilla;rejects([&]{ensure_vp_unit_production_profile(vanilla);});
        check(!find_vp_unit_production_profile(vanilla),"Vanilla runtime has no required VP production state");
        auto broken=snap;broken.all_players->players[0].core.fields.back().values.pop_back();rejects([&]{restore_vp_player_unit_production(broken,rules,restored);});
        broken=snap;broken.all_players->players[0].core.fields[0].values[0]++;rejects([&]{restore_vp_player_unit_production(broken,rules,restored);});
    }
    db.apply_sql("DROP TABLE Unit_ProductionTraits");rejects([&]{load_vp_player_unit_production_rules(db,types);});
}
