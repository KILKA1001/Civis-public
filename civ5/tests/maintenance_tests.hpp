#pragma once
void maintenance_tests() {
    using namespace civ5::runtime;
    RulesDatabase db;
    db.apply_sql("CREATE TABLE Defines(Name,Value);INSERT INTO Defines VALUES"
        "('UNIT_MAINTENANCE_GAME_MULTIPLIER',8),('UNIT_MAINTENANCE_GAME_EXPONENT_DIVISOR',7),"
        "('INITIAL_FREE_OUTSIDE_UNITS',2),('INITIAL_OUTSIDE_UNIT_GOLD_PERCENT',50),"
        "('VASSALAGE_VASSAL_UNIT_MAINT_COST_PERCENT',10),('VASSALAGE_VASSAL_CITY_POP_EXPONENT',0.8),('AI_HANDICAP',1);");
    const auto bnw_defs=db.cache_global_defines(GlobalDefineProfile::bnw),vp_defs=db.cache_global_defines(GlobalDefineProfile::vp542);
    ResearchContext setup;setup.game_handicap=1;setup.start_era=0;setup.players[0]={0,0,3,0,true};
    PlayerResearchContext dip;dip.players[0]={};dip.teams[0].members={0};dip.teams[0].alive_count=1;dip.major_players={0};dip.barbarian_team=63;
    CityYieldContext cities;cities.players[0].cities[10].population=10;
    std::vector<TeamTechnologies> techs;MaintenanceWorld w{setup,dip,cities,techs};
    MaintenanceRules rules;rules.handicaps[0]={1,0,75,0,80,0,70,0,80,0,0};
    rules.handicaps[1]={0,2,100,50,100,60,100,50,100,60,-10};rules.era_building_percent[0]=90;rules.combat_classes={0};
    MaintenanceContext s;s.default_end_turn=s.estimate_end_turn=500;s.team_eras[0]=2;
    rejects([&]{ensure_vp_maintenance_rules_profile(rules);});
    rejects([&]{ensure_vp_maintenance_context_profile(s);});
    auto &p=s.players[0];p.gold_per_unit_times100=150;p.extra_unit_cost=2;p.outside_units=7;
    p.unit_modifier=-20;p.supply_modifier=50;p.building_modifier=10;p.improvement_modifier=-20;
    p.land_modifier=-20;p.naval_modifier=25;p.base_building=25;p.base_improvement=13;p.free_garrison=true;p.combat_modifiers[0]=-10;
    for(std::int64_t id=0;id<8;++id){auto &u=p.units[id];u.domain=2;u.combat=0;u.military=true;u.civilian=false;u.garrison=false;}
    p.units[0].no_maintenance=true;p.units[3].domain=0;p.units[3].garrison=true;p.units[7].domain=0;
    p.units[4].domain=1;p.units[4].combat=-1;p.units[4].military=false;p.units[4].no_maintenance=true;
    p.units[5].military=false;p.units[5].civilian=true;p.units[5].combat=-1;
    auto q=maintenance_costs(s,w,rules,bnw_defs,nullptr,0);
    check(q.units.free_units==4&&q.units.paid_units==4&&q.units.base_times100==457&&q.units.extra_times100==200&&q.units.total==3,
        "BNW unit maintenance discounts, fractional truncation or flat cost placement differ");
    check(q.supply==1&&q.buildings==18&&q.improvements==7&&q.total==29&&p.cached_unit_expense==3&&p.cached_supply_expense==1&&
        !q.profile_extensions.find<VpMaintenanceQuote>(),
        "BNW supply/building/route cost or query cache mutation differs");
    setup.players[0].slot_status=1;
    check(unit_maintenance(s,w,rules,bnw_defs,nullptr,0).total==2,"BNW AI unit discount must use game handicap");
    setup.players[9]={0,0,3,0,true};
    check(unit_maintenance(s,w,rules,bnw_defs,nullptr,0).total==5,"BNW human teammate must exempt AI discount");
    setup.players.erase(9);setup.players[0].slot_status=3;
    p.units[3].garrison.reset();rejects([&]{unit_maintenance(s,w,rules,bnw_defs,nullptr,0);});
    p.free_garrison=false;check(unit_maintenance(s,w,rules,bnw_defs,nullptr,0).paid_units==5,"Unused garrison query must remain lazy");
    p.free_garrison=true;p.units[3].garrison=true;
    rejects([&]{unit_maintenance(s,w,rules,bnw_defs,nullptr,0,20);});
    s.default_end_turn=0;rejects([&]{unit_maintenance(s,w,rules,bnw_defs,nullptr,0);});
    cities.players[0].cities.clear();check(unit_maintenance(s,w,rules,bnw_defs,nullptr,0).total==0,"Cityless unit cost must return before progress division");
    cities.players[0].cities[10].population=10;s.default_end_turn=500;
    setup.profile=dip.profile=cities.profile=rules.profile=s.profile=TechnologyProfile::vp542;
    check(!p.profile_extensions.find<VpMaintenancePlayerProfile>(),"BNW maintenance player must not require VP expense state");
    rejects([&]{ensure_vp_maintenance_profile(p,TechnologyProfile::bnw);});
    ensure_vp_maintenance_profile(p,s.profile).contract_units.insert(4);
    ensure_vp_maintenance_rules_profile(rules);
    ensure_vp_maintenance_context_profile(s);
    CustomModOptions options;options.prefetch.all_succeeded=true;options.entries["BALANCE_CORE_JFD"].value=1;
    q=maintenance_costs(s,w,rules,vp_defs,&options,0);
    check(q.units.free_units==6&&q.units.paid_units==2&&q.units.base_times100==150&&q.units.total==2,
        "VP repeated handicap allowance or additive contract free-unit rule differs");
    check(unit_maintenance(s,w,rules,vp_defs,&options,0,20).total==18,"VP count override must keep domain and combat discounts");
    p.units[4].domain=2;
    // Free-count military filter uses !IsCivilianUnit, domain count uses IsCombatUnit.
    check(unit_maintenance(s,w,rules,vp_defs,&options,0).base_times100==210,"VP ranged civilian exclusion must differ between the two domain getters");
    p.units[4].domain=1;
    options.entries["BALANCE_VP"].value=1;techs.emplace_back(TechnologyProfile::vp542,0,100);vp_team_change_known_count(techs[0],20);
    w.technologies=techs;
    const auto progress=unit_maintenance(s,w,rules,vp_defs,&options,0);
    check(progress.multiplier==static_cast<double>(2.6f)&&progress.exponent==static_cast<double>(1.0285714864730835f),
        "VP tech progress must preserve float operation precision");
    options.entries["BALANCE_VP"].value=0;
    setup.players[1]={1,0,1,0,true};setup.players[2]={0,0,3,0,true};dip.players[1]={};dip.players[2]={};
    dip.teams[0].members={0,2};dip.teams[0].alive_count=2;dip.teams[1].master=0;dip.teams[1].members={1};dip.teams[1].alive_count=1;
    dip.major_players={0,1,2};cities.players[1].cities[20].population=16;cities.players[2]={};s.players[1].cached_unit_expense=21;s.players[2]={};
    auto &vp_player=ensure_vp_maintenance_profile(p,s.profile);
    vp_player.vassal_modifier=10;vp_player.vassal_taxes_times100=199;p.diplomacy_gold=-3;vp_player.outgoing_espionage_gold=2;
    vp_player.extra_building_percent[10]=15;options.entries["BALANCE_RESOURCE_SHORTAGE_BUILDING_REFUND"].value=1;
    ensure_vp_maintenance_rules_profile(rules).contracts={3};vp_player.active_contracts={3};
    ensure_vp_maintenance_context_profile(s).contracts={{3,1,0,7,0},{3,0,0,90,0}};
    q=maintenance_costs(s,w,rules,vp_defs,&options,0);
    const auto &vp_quote=vp_maintenance_quote(q);
    check(q.buildings==21&&q.improvements==7&&vp_quote.vassals==6&&vp_quote.taxes==1&&vp_quote.contracts==7&&vp_quote.espionage==2&&vp_quote.diplomacy==3&&q.total==49,
        "VP total costs must use ordered contract lookup, vassal cached expense, leader remainder and component rounding");
    check(p.cached_unit_expense==2&&s.players[1].cached_unit_expense==21,"Vassal expense must not recalculate its unit cache");
    dip.teams[0].members={2,0};
    check(vp_maintenance_quote(maintenance_costs(s,w,rules,vp_defs,&options,0)).vassals==5,"Vassal maintenance remainder belongs to first alive stored member");
    dip.teams[0].alive_count=0;
    check(vp_maintenance_quote(maintenance_costs(s,w,rules,vp_defs,&options,0)).vassals==0,"Zero team alive count must bypass vassal division");
    // A vassal maintenance discount is separate from the master's costs.
    dip.teams[0].master=1;
    check(unit_maintenance(s,w,rules,vp_defs,&options,0,20).total==16,"VP vassal unit discount ordering differs");
    dip.teams[0].master=-1;
    options.entries["HUMAN_USES_AI_HANDICAP"].value=1;
    check(unit_maintenance(s,w,rules,vp_defs,&options,0,20).total==12,"Human AI-handicap option must change both selected Info and percent branch");
    options.entries["HUMAN_USES_AI_HANDICAP"].value=0;
    p={};p.gold_per_unit_times100=5000000000LL;rules.handicaps[0].unit=100;
    p.units[4294967300LL]={};
    check(unit_maintenance(s,w,rules,vp_defs,&options,0,100).total==5000000000LL,"Maintenance retained x86 result capacity");
    p.gold_per_unit_times100=INT64_MAX;rejects([&]{unit_maintenance(s,w,rules,vp_defs,&options,0,100);});
    s.profile=TechnologyProfile::bnw;rejects([&]{maintenance_costs(s,w,rules,vp_defs,&options,0);});
    db.apply_sql("CREATE TABLE HandicapInfos(ID,Type,GoldFreeUnits,MaintenanceFreeUnits,RouteCostPercent,ImprovementCostPercent);"
        "INSERT INTO HandicapInfos VALUES(0,'H',2,3,70,80);"
        "CREATE TABLE Eras(ID,Type,BuildingMaintenancePercent);INSERT INTO Eras VALUES(0,'E',90);"
        "CREATE TABLE Domains(ID,Type);INSERT INTO Domains VALUES(4,'HOVER');"
        "CREATE TABLE UnitCombatInfos(ID,Type);INSERT INTO UnitCombatInfos VALUES(0,'C'),(4294967300,'WIDE_C');"
        "CREATE TABLE Units(ID,Type,Domain,CombatClass,Range,NukeDamageLevel,NoMaintenance);"
        "INSERT INTO Units VALUES(0,'U','HOVER','WIDE_C',2,0,0);"
        "CREATE TABLE Contracts(ID,Type);INSERT INTO Contracts VALUES(4294967300,'WIDE_CONTRACT');");
    const auto types=db.cache_info_types();
    for(bool vp:{false,true}) {
        using namespace civ5::saves;
        const auto profile=vp?TechnologyProfile::vp542:TechnologyProfile::bnw;
        const auto layout=vp?GameStateLayout::vp542_corpus:GameStateLayout::expansion2_version1;
        const auto loaded_rules=load_maintenance_rules(db,profile,types);
        check(vp?loaded_rules.extensions.find<VpMaintenanceRulesProfile>()!=nullptr:
                 loaded_rules.extensions.find<VpMaintenanceRulesProfile>()==nullptr,
            "Only VP maintenance rules import contract Info");
        check(loaded_rules.handicaps.at(0).free_units==(vp?5:2)&&loaded_rules.handicaps.at(0).improvement==(vp?56:70)&&
            loaded_rules.combat_classes.contains(4294967300LL),"Maintenance Info must apply VP legacy columns and preserve sparse wide slots");
        SavedSessionSnapshot snap{};snap.game_state.layout=layout;
        if(vp)snap.game_state.prefix=VpPrefixObservation{};else snap.game_state.prefix=Expansion2PrefixObservation{};
        std::visit([](auto &pre){pre.scalars[1]=10;pre.scalars[5]=400;pre.scalars[6]=500;},snap.game_state.prefix);
        snap.teams.emplace();snap.teams->layout=layout;TeamObservation team{};team.fields={{"id",0},{"era",2}};snap.teams->teams={team};
        snap.map_plots.emplace();snap.map_plots->layout=layout;snap.map_header.grid={1,1};
        PlotObservation plot{};plot.fields={{"owner",0},{"plot_city_id",7},{"plot_type",3}};snap.map_plots->plots={plot};
        snap.final_systems.layout=layout;if(vp)snap.final_systems.vp.emplace();
        snap.all_players.emplace();snap.all_players->layout=layout;SavedPlayerObservation sp{};sp.saved_player_id=0;
        sp.core.layout=sp.traits.layout=sp.administration.layout=sp.units.layout=sp.cities.layout=sp.tail.layout=layout;sp.traits.version=9;
        const auto field=[&](const char *name,std::vector<std::int64_t> values){PlayerCoreField f{};f.name=name;f.values=std::move(values);sp.core.fields.push_back(f);};
        for(const auto *name:{"m_iBaseFreeUnits","m_iGoldPerUnit","m_iExtraUnitCost","m_iNumOutsideUnits","m_iUnitGoldMaintenanceMod",
            "m_iUnitSupplyMod","m_iBuildingGoldMaintenanceMod","m_iRouteGoldMaintenanceMod","m_iImprovementGoldMaintenanceMod",
            "m_iGarrisonFreeMaintenanceCount","m_iVassalGoldMaintenanceMod"})field(name,{0});
        PlayerTailNode outgoing{};outgoing.name="m_aiOutgoingEspionageYields";outgoing.values={0,0,7};sp.tail.fields={outgoing};field("m_abActiveContract",{0,1});
        for(const auto *name:{"m_iLandUnitMaintenanceModifier","m_iNavalUnitMaintenanceModifier","m_paiMaintenanceModifierUnitCombat"}) {
            PlayerTraitsField f{};f.name=name;f.values={17};sp.traits.fields.push_back(f);
        }
        PlayerAdministrationRecord tr{};tr.name="treasury";
        tr.fields={{"m_iBaseBuildingGoldMaintenance",25},{"m_iBaseImprovementGoldMaintenance",13},{"m_iExpensePerTurnUnitMaintenance",42},
            {"m_iGoldPerTurnFromDiplomacy",-3},{"m_iExpensePerTurnFromVassalTax",99},{"m_iExpensePerTurnUnitSupply",4}};sp.administration.blocks={tr};
        SavedCityObservation sc{};sc.core.layout=layout;
        for(const auto &[name,n]:std::vector<std::pair<const char *,std::int64_t>>{{"m_iID",7},{"m_eOwner",0},{"m_iExtraBuildingMaintenance",15}}) {
            CityCoreField f{};f.name=name;f.values={n};sc.core.fields.push_back(f);
        }
        sp.cities.cities={sc};SavedUnitObservation su{};su.core.layout=su.tail.layout=layout;
        for(const auto &[name,n]:std::vector<std::pair<const char *,std::int64_t>>{{"m_iID",4294967300LL},{"m_eOwner",0},
            {"m_eUnitType",vp?0:999},{"idUnitType",saved_info_type_hash("U")},{"m_eCombatType",12},{"m_iBaseCombat",0},
            {"m_iBaseRangedCombat",5},{"m_iExtraRange",0},{"m_bIsNoMaintenance",0},{"m_iGarrisonCityID",7},
            {"m_eUnitContract",-1},{"m_iX",0},{"m_iY",0}}) {
            UnitCoreField f{};f.name=name;f.values={n};su.core.fields.push_back(f);
        }
        UnitTailNode layer{};layer.name="m_iMapLayer";layer.values={0};UnitTailNode tail{};tail.name="state";tail.children={layer};su.tail.blocks={tail};
        sp.units.units={su};snap.all_players->players={sp};
        ResearchContext rs;rs.profile=profile;rs.players[0]={0,0,3,0,true};
        PlayerResearchContext rd;rd.profile=profile;rd.teams[0]={};rd.players[0]={};
        CityYieldContext cy;cy.profile=profile;cy.players[0].cities[7]={};std::vector<TeamTechnologies> rt;
        MaintenanceWorld view{rs,rd,cy,rt};
        auto restored=restore_maintenance(snap,view,loaded_rules,types);const auto &rp=restored.players.at(0);const auto &u=rp.units.at(4294967300LL);
        check(restored.elapsed_turns==10&&restored.estimate_end_turn==400&&restored.default_end_turn==500&&
            rp.cached_unit_expense==42&&rp.base_building==25&&u.type==0&&u.domain==(vp?4:0)&&u.combat==(vp?12:4294967300LL),
            "Restored maintenance inputs must use BNW type hash/hover plot and VP live combat type");
        check(vp?(u.garrison==true&&!u.civilian&&vp_maintenance_profile(rp).outgoing_espionage_gold==7&&
                  restored.extensions.find<VpMaintenanceContextProfile>()!=nullptr):
                 (!u.garrison&&u.civilian&&rp.combat_modifiers.empty()&&!rp.profile_extensions.find<VpMaintenancePlayerProfile>()),
            "Garrison, ranged civilian or pre-v10 BNW trait restoration differs");
        snap.all_players->players[0].units.units.push_back(su);
        rejects([&]{restore_maintenance(snap,view,loaded_rules,types);});snap.all_players->players[0].units.units.pop_back();
        snap.all_players->players[0].units.units[0].core.fields[1].values={1};
        rejects([&]{restore_maintenance(snap,view,loaded_rules,types);});
    }
}
