#pragma once
void vp_nature_yield_tests(){
    using namespace civ5::runtime;using namespace civ5::saves;
    constexpr auto vp=TechnologyProfile::vp542;constexpr auto layout=GameStateLayout::vp542_corpus;
    RulesDatabase db;db.apply_sql("CREATE TABLE Defines(Name,Value);INSERT INTO Defines VALUES('MIN_WATER_SIZE_FOR_OCEAN',1),('SHALLOW_WATER_TERRAIN',5),('DEEP_WATER_TERRAIN',6),('CITY_STRENGTH_THRESHOLD_FOR_BONUSES',40),('BALANCE_PRODUCTION_DESERT_IMPROVEMENT',103);"
        "CREATE TABLE Yields(ID,Type,HillsChange,MinCity,PopulationChangeDivisor);INSERT INTO Yields VALUES(0,'FOOD',7,13,-3);"
        "CREATE TABLE Terrains(ID,Type);INSERT INTO Terrains VALUES(0,'LAND');"
        "CREATE TABLE Features(ID,Type,YieldNotAdditive,NaturalWonder,PseudoNaturalWonder);INSERT INTO Features VALUES(3,'FOREST',1,0,1);"
        "CREATE TABLE Resources(ID,Type);INSERT INTO Resources VALUES(8,'RESOURCE');"
        "CREATE TABLE Improvements(ID,Type,ConnectsAllResources,NoFollowUp);INSERT INTO Improvements VALUES(0,'FORT',1,1);"
        "CREATE TABLE Plots(ID,Type);INSERT INTO Plots VALUES(1,'HILL');");
    for(const auto &[prefix,key]:std::vector<std::pair<std::string,std::string>>{{"Terrain","TerrainType"},{"Feature","FeatureType"}})
        for(const auto &suffix:std::vector<std::string>{prefix=="Terrain"?"Yields":"YieldChanges","RiverYieldChanges","HillsYieldChanges","FreshWaterYields","CoastalLandYields"})
            db.apply_sql("CREATE TABLE "+prefix+"_"+suffix+"("+key+",YieldType,Yield);");
    db.apply_sql("CREATE TABLE Resource_YieldChanges(ResourceType,YieldType,Yield);CREATE TABLE Plot_Yields(PlotType,YieldType,Yield);"
        "INSERT INTO Plot_Yields VALUES('HILL','FOOD',7),('HILL','FOOD',0);"
        "CREATE TABLE Plot_AdjacentFeatureYieldChanges(PlotType,FeatureType,YieldType,Yield,IgnoreNaturalWonderPlots);"
        "INSERT INTO Plot_AdjacentFeatureYieldChanges VALUES('HILL','FOREST','FOOD',3,0),('HILL','FOREST','FOOD',-3,0),('HILL','FOREST','FOOD',0,0),('HILL','FOREST','FOOD',11,1),('HILL','FOREST','FOOD',13,1);");
    const auto loaded=load_vp_nature_yield_rules(db);
    check(loaded.yield_slots==1&&loaded.resource_slots==9&&loaded.yields.at(0).population_divisor==-3&&loaded.yields.at(0).min_city==13&&loaded.features.at(3).pseudo_natural_wonder&&loaded.improvements.at(0).no_followup,
        "VP nature loaders use active yields, pseudo-wonders and improvement flags");
    check(loaded.plots.at(1).base.empty()&&loaded.plots.at(1).adjacent_features.at(3)[0].at(0)==0&&loaded.plots.at(1).adjacent_features.at(3)[1].at(0)==24,
        "Ordinary SetYields replaces rows while plot adjacency adds duplicates and retains cancelled entries");
    const auto types=db.cache_info_types();const auto defines=db.cache_global_defines(GlobalDefineProfile::vp542);
    CustomModOptions options;for(const auto name:{"BALANCE_VP","PLOTS_EXTENSIONS","BALANCE_RESOURCE_MONOPOLIES"})options.entries[name].value=1;
    ResearchContext players;players.profile=vp;players.players[5].team=9;players.players[6].team=10;
    PlayerResearchContext diplomacy;diplomacy.profile=vp;diplomacy.players[5]={};diplomacy.players[6]={};diplomacy.teams[9].members={5};diplomacy.teams[10].members={6};
    PlotMovementState moves;moves.players[5]={};moves.players[6]={};moves.teams[9]={};moves.teams[10]={};
    SavedMapProjection map{{5,5}};PlotPassability pass;pass.layout=layout;
    for(std::uint64_t i=0;i<25;++i){SavedMapCellProjection p;p.index=i;p.x=i%5;p.y=i/5;p.plot_type=2;p.terrain_type=0;p.feature_type=-1;p.owner=5;p.improvement_pillaged=false;p.landmass=0;
        p.route=SavedRoutePlot{};p.rivers=SavedRiverPlot{};p.water_cache=PlotWaterCache{};p.units.emplace();p.revealed_teams.emplace();map.cells.push_back(p);pass.plots.push_back({false});ensure_vp_plot_team_flags(pass,i)={{9,false},{10,false}};}
    auto &plot=map.cells[12];plot.plot_type=1;plot.feature_type=1;plot.rivers->is_river=true;plot.water_cache->lake=true;plot.water_cache->fresh=true;plot.water_cache->adjacent_water=true;
    map.cells[11].feature_type=3;map.cells[17].plot_type=3;map.cells[17].terrain_type=5;map.cells[17].landmass=1;
    PlotWaterRules wr;wr.profile=vp;PlotWaterState ws;ws.layout=layout;ws.landmasses={{0,{24,false}},{1,{10,true}}};
    CityYieldContext cities;cities.profile=vp;cities.yield_slots=1;cities.players[5].capital=44;cities.players[6].capital=99;
    auto &city=cities.players[5].cities[44];city.owner=5;city.id=44;city.x=2;city.y=2;city.population=9;auto &city_plot=ensure_vp_city_plot_profile_city(cities,5,44).yields.emplace();city_plot.strength=5000;city_plot.garrison=10;city_plot.per_strength_times100[0]=201;
    MaintenanceContext units;units.profile=vp;units.players[5]={};units.players[6]={};MaintenanceRules ur;ur.profile=vp;ur.units[0]={};
    MaintenanceUnit plain;check(!plain.profile_extensions.find<VpUnitYieldProfileState>(),"Shared unit does not require VP yield state");
    rejects([&]{ensure_vp_unit_yield_profile(plain,TechnologyProfile::bnw);});
    for(const auto id:{10,11,12,13,14,15,16}){auto &u=units.players[5].units[id];u.type=0;u.civilian=false;
        ensure_vp_unit_yield_profile(u,units.profile).plot_yields.emplace().position={{1,1}};}
    auto &u10=*vp_unit_yield_profile(units.players[5].units[10]).plot_yields;u10.base_combat=10;u10.garrison[0]=3;u10.fortification[0]=3;
    auto &u11=*vp_unit_yield_profile(units.players[5].units[11]).plot_yields;u11.base_combat=7;u11.fortification[0]=1;
    vp_unit_yield_profile(units.players[5].units[12]).plot_yields->position={{99999,99999}};units.players[5].units[14].civilian=true;
    auto &u15=*vp_unit_yield_profile(units.players[5].units[15]).plot_yields;u15.death_delay=true;u15.base_combat=8;u15.fortification[0]=2;
    vp_unit_yield_profile(units.players[5].units[16]).plot_yields->position.reset();
    plot.units=std::vector<SavedPlotUnitReference>{{5,10},{5,11},{5,10},{5,15},{5,999},{99,10}};
    PlotWaterContext water{map,ws,wr,types,pass,players,cities,defines,&options};PlotMovementContext movement{map,pass,players,diplomacy,moves,types,defines};
    VpNatureYieldRules rules;rules.yield_slots=1;rules.resource_slots=9;rules.yields[0]={2,3,5,7,1,2,11,{13,17,19,23,29,31}};
    for(std::size_t i=0;i<5;++i){rules.terrains[0].yields[i][0]=std::array<std::int64_t,5>{37,41,43,47,53}[i];rules.features[1].yields[i][0]=std::array<std::int64_t,5>{59,61,67,71,73}[i];}
    rules.features[2].pseudo_natural_wonder=true;rules.features[3]={};rules.terrains[2].yields[0][0]=101;rules.terrains[5].yields[0][0]=107;rules.terrains[7].yields[0][0]=109;
    for(std::int64_t i=0;i<4;++i){rules.plots[i].base[0]=79;rules.plots[i].adjacent_features[3][0][0]=83;rules.plots[i].adjacent_features[3][1][0]=89;}
    rules.resources[8][0]=97;rules.improvements[0]={false,true};
    VpNatureYieldState state;state.extra_yields={{2,2,{{0,101}}},{2,2,{{0,999}}}};auto &p=state.players[5];cities.players[5].total_population=31;
    const std::array<std::int64_t,12> amounts{67,13,23,67,7,11,19,3,2,11,17,29};for(std::size_t i=0;i<amounts.size();++i)p.yields[i][0]=amounts[i];p.global_monopoly_cache={8,9};
    state.players[6]=p;VpNatureYieldContext c{rules,state,water,movement,cities,units,ur,9};
    const auto query=[&](std::int64_t player=5,std::int64_t feature=1,std::int64_t resource=8,std::int64_t improvement=0,CityYieldState *owning=nullptr,bool display=false){return vp_nature_yield(c,12,0,player,feature,resource,improvement,owning,display);};
    auto q=query();check(q.total==832&&q.extra==101&&q.adjacent==172&&q.fortification==8,"VP nature combines terrain/plot/first game extra, water, features/resources and per-unit rounded fortification yields");
    q=query(5,1,8,0,&city);check(q.total==564&&q.city_replaced&&q.city_geography==19&&q.garrison==3&&q.military==3&&q.monopoly==46&&q.strength==100&&q.city_and_capital==1&&q.capital_population==0&&q.capital_empire==3,
        "Full VP city formula resets terrain then adds garrison, military, coastal, monopoly, strength and separately rounded capital yields");
    check(query(5,1,8,0,&city,true).total==552,"Unrevealed display suppresses only CityChange and population term");plot.revealed_teams->insert(9);
    check(query(5,1,8,0,&city,true).total==564,"Revealed display uses active-team flag");
    options.entries["BALANCE_VP"].value=0;check(query(5,1,8,0,&city).total==1089,"CP-only city keeps existing nature total above minimum");options.entries["BALANCE_VP"].value=1;
    check(query(-1,1,8,0,&city).total==322,"VP city resource reset remains active with NO_PLAYER; player-wide terms are omitted");
    options.entries["BALANCE_NO_RESOURCE_YIELDS_FROM_GP_IMPROVEMENT"].value=1;rules.improvements[0].connects_all_resources=true;
    check(query().resource==0&&query(5,1,8,0,&city).city_resource==97,"GP improvement suppresses normal resource yield but not VP city resource reset");
    rules.improvements[0].connects_all_resources=false;
    plot.feature_type=2;check(query().adjacent==83,"Actual pseudo-natural wonder excludes IgnoreNaturalWonderPlots rows despite hypothetical ordinary feature");plot.feature_type=1;
    rules.features[1].nonadditive=true;check(query().total==608&&query().feature_replaced,"Nonadditive feature removes terrain, plot, game extra, hills and lake subtotal");rules.features[1].nonadditive=false;
    ensure_vp_plot_team_flags(pass,12)[9]=true;plot.owner=6;check(query().terrain_suppressed&&query().terrain==0&&query().feature==59,"Foreign inaccessible plot loses only terrain base");
    plot.owner=5;check(!query().terrain_suppressed,"Own inaccessible plot retains terrain base");
    plot.owner=6;plot.plot_type=3;plot.terrain_type=5;check(!query().terrain_suppressed&&query().terrain==107,"Shallow water retains terrain even without movement rights");
    plot.owner=5;plot.plot_type=1;plot.terrain_type=0;ensure_vp_plot_team_flags(pass,12)[9]=false;
    plot.terrain_type=2;plot.plot_type=2;c.production=0;check(query(5,-1).desert==103&&query(5,1).desert==0&&query(5,-1,-1).desert==0,"Desert production uses hypothetical feature/resource/improvement gate");plot.plot_type=1;plot.terrain_type=0;c.production=1;
    city_plot.garrison=999;check(query(5,1,8,0,&city).garrison==0&&city_plot.garrison==-1,"HasGarrison clears stale saved garrison handle");
    city_plot.garrison=999;city_plot.garrison_override=10;check(query(5,1,8,0,&city).garrison==3&&city_plot.garrison==999,"Valid unsaved override resolves before stale saved handle");
    city_plot.garrison=-1;check(query(5,1,8,0,&city).garrison==0,"Override alone does not make HasGarrison true");city_plot.garrison=10;city_plot.garrison_override=999;
    check(query(5,1,8,0,&city).garrison==3,"Stale override falls back to saved garrison");city_plot.garrison_override=-1;
    city.population=8;rules.yields[0].population_divisor=-2;check(query(5,1,8,0,&city).population==-4,"Authored negative nonzero population divisor truncates toward zero");city.population=9;rules.yields[0].population_divisor=2;
    check(query(6,1,8,0,&city).trait_capital==29,"Capital identity belongs to actual city owner, player bonuses to argument player");
    plot.improvement_pillaged=true;check(query().fortification==0,"Actual pillage suppresses hypothetical fortification bonus");plot.improvement_pillaged=false;
    p.has_global_monopoly={8};refresh_vp_global_monopoly_cache(state,rules,options,5);check(p.global_monopoly_cache==std::vector<std::int64_t>{8}&&query(5,1,8,0,&city).monopoly==23,"Explicit global monopoly refresh reads saved flags");
    p.has_global_monopoly.clear();check(query(5,1,8,0,&city).monopoly==23,"Changing monopoly flag does not silently refresh unsaved vector");
    options.entries["BALANCE_RESOURCE_MONOPOLIES"].value=0;refresh_vp_global_monopoly_cache(state,rules,options,5);check(query(5,1,8,0,&city).monopoly==0,"Disabled monopoly option clears cache");
    rules.features[1].yields[0][0]=-10000;check(query().total==0,"VP nature clamps final negative result");rules.features[1].yields[0][0]=4294967300LL;check(query().total>UINT32_MAX,"VP nature sums stay wider than int32");
    rules.features[1].yields[0][0]=INT64_MAX;rejects([&]{query();});rules.features[1].yields[0][0]=59;
    constexpr std::int64_t wide=4294967300LL;rules.features[wide]=rules.features[1];check(query(5,wide).feature==59,"Wide hypothetical Info ID uses sparse rules");
    plot.revealed_teams->insert(wide);diplomacy.teams[wide]={};c.active_team=wide;check(query(5,1,8,0,&city,true).city_change==7,"Wide live team ID uses sparse city reveal");c.active_team=9;
    plot.units.reset();rejects([&]{query();});plot.units.emplace();plot.revealed_teams.reset();rejects([&]{query(5,1,8,0,&city,true);});
    CityYieldState copy=city;rejects([&]{query(5,1,8,0,&copy);});

    SavedSessionSnapshot snapshot;snapshot.all_players.emplace();snapshot.all_players->layout=snapshot.history.layout=layout;
    SavedPlayerObservation saved;saved.saved_player_id=wide;
    saved.core.layout=saved.traits.layout=saved.tail.layout=saved.cities.layout=saved.units.layout=layout;
    const auto core=[&](const char *name,std::vector<std::int64_t> values){PlayerCoreField f{name,PlayerCoreEncoding::i32};f.values=std::move(values);saved.core.fields.push_back(f);};
    core("m_iTotalPopulation",{wide});
    for(const auto name:{"m_aiCityYieldChange","m_aiCoastalCityYieldChange","m_aiMonopolyCityYieldChange","m_aiCapitalYieldChange","m_aiCapitalYieldPerPopChange","m_aiCapitalYieldPerPopChangeEmpire","m_aiYieldPerCityOverStrengthThreshold","m_aiYieldFromXMilitaryUnits"})core(name,{wide,0});
    core("m_pabHasGlobalMonopoly",{1,0});
    for(const auto name:{"m_iYieldFromXMilitaryUnits","m_iCityYieldChanges","m_iCoastalCityYieldChanges","m_iCapitalYieldChanges"}){
        PlayerTraitsField f{name,PlayerTraitsEncoding::yield_array};f.values={0,wide+1};saved.traits.fields.push_back(f);}
    PlayerTailNode monopoly{"m_pabHasGlobalMonopoly","bool"};monopoly.values={0,1};saved.tail.fields.push_back(monopoly);
    saved.cities.cities.emplace_back();auto &sc=saved.cities.cities.back();sc.core.layout=layout;sc.core.start_offset=wide;sc.core.end_offset=wide+100;
    const auto city_field=[&](const char *name,std::vector<std::int64_t> values){CityCoreField f{name,CityCoreEncoding::i32};f.values=std::move(values);sc.core.fields.push_back(f);};
    city_field("m_iID",{wide+2});city_field("m_eOwner",{wide});city_field("m_iStrengthValue",{wide+3});city_field("m_hGarrison",{wide+4});city_field("m_aiYieldChangesPerCityStrengthTimes100",{0,wide+5});
    saved.units.units.emplace_back();auto &su=saved.units.units.back();su.core.layout=su.tail.layout=layout;su.core.start_offset=wide+200;su.core.end_offset=wide+300;
    const auto unit_field=[&](const char *name,std::vector<std::int64_t> values){UnitCoreField f{name,UnitCoreEncoding::i32};f.values=std::move(values);su.core.fields.push_back(f);};
    unit_field("m_iID",{wide+4});unit_field("m_eOwner",{wide});unit_field("m_iX",{wide});unit_field("m_iY",{wide+1});unit_field("m_iBaseCombat",{wide+6});unit_field("m_bDeathDelay",{1});
    UnitTailNode tail{"state","fields"},gy{"m_iGarrisonYieldChange","array"},fy{"m_iFortificationYieldChange","array"};gy.values={wide+7,0};fy.values={0,wide+8};tail.children={gy,fy};su.tail.blocks={tail};
    snapshot.all_players->players={saved};snapshot.history.extra_yields={{{wide,wide+1},{wide+9,0}},{{wide,wide+1},{0,wide+10}}};
    VpNatureYieldRules import_rules;import_rules.yield_slots=import_rules.resource_slots=2;
    CityYieldContext ic;ic.profile=vp;ic.yield_slots=2;auto &live_city=ic.players[wide].cities[wide+2];live_city.owner=wide;live_city.id=wide+2;live_city.source_core={{wide,wide+100}};
    MaintenanceContext iu;iu.profile=vp;auto &live_unit=iu.players[wide].units[wide+4];live_unit.source_core={{wide+200,wide+300}};
    const auto imported=restore_vp_nature_yields(snapshot,import_rules,ic,iu);const auto &ip=imported.players.at(wide);
    check(ic.players.at(wide).total_population==wide&&ip.yields[0].at(0)==wide&&ip.yields[11].at(1)==wide+1&&ip.has_global_monopoly==std::set<std::int64_t>{1}&&ip.global_monopoly_cache.empty(),
        "VP nature import widens player values, uses later monopoly flags and leaves unsaved cache cold");
    const auto &live_city_plot=*vp_city_plot_profile_city(ic,wide,wide+2).yields;
    check(live_city_plot.strength==wide+3&&live_city_plot.garrison==wide+4&&live_city_plot.garrison_override==-1&&live_city_plot.per_strength_times100.at(1)==wide+5,
        "VP nature binds city state by wide identity and source offsets");
    const auto &live_unit_yields=*vp_unit_yield_profile(live_unit).plot_yields;
    check(live_unit_yields.position==std::optional{std::pair{wide,wide+1}}&&live_unit_yields.base_combat==wide+6&&live_unit_yields.death_delay&&live_unit_yields.garrison.at(0)==wide+7&&live_unit_yields.fortification.at(1)==wide+8,
        "VP nature binds existing unit and preserves wide positions/combat/yields");
    check(imported.extra_yields.size()==2&&imported.extra_yields[0].yields.at(0)==wide+9&&imported.extra_yields[1].yields.at(1)==wide+10,"Duplicate extra-yield positions retain source order");
    for(const auto sentinel:{-1LL,-2147483647LL}){auto invalid=snapshot;invalid.all_players->players[0].units.units[0].core.fields[2].values={sentinel};restore_vp_nature_yields(invalid,import_rules,ic,iu);check(!vp_unit_yield_profile(live_unit).plot_yields->position,"Original off-map wire sentinel becomes absent host position");}
    auto broken=snapshot;broken.all_players->players[0].tail.fields[0].values={1};rejects([&]{restore_vp_nature_yields(broken,import_rules,ic,iu);});
    broken=snapshot;broken.all_players->players[0].traits.fields[0].values={1};rejects([&]{restore_vp_nature_yields(broken,import_rules,ic,iu);});
    broken=snapshot;broken.all_players->players[0].cities.cities[0].core.start_offset++;rejects([&]{restore_vp_nature_yields(broken,import_rules,ic,iu);});
    broken=snapshot;broken.all_players->players[0].units.units[0].core.end_offset++;rejects([&]{restore_vp_nature_yields(broken,import_rules,ic,iu);});
    broken=snapshot;broken.all_players->players[0].units.units[0].tail.blocks[0].children[0].values.pop_back();rejects([&]{restore_vp_nature_yields(broken,import_rules,ic,iu);});
    broken=snapshot;broken.all_players->players[0].tail.fields[0].values[1]=2;rejects([&]{restore_vp_nature_yields(broken,import_rules,ic,iu);});
    broken=snapshot;broken.all_players->players[0].units.units[0].core.fields.back().values={2};rejects([&]{restore_vp_nature_yields(broken,import_rules,ic,iu);});
    broken=snapshot;broken.all_players->players.push_back(saved);rejects([&]{restore_vp_nature_yields(broken,import_rules,ic,iu);});
}
