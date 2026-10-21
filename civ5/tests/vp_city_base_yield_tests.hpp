#pragma once
void vp_city_base_yield_tests(){
    using namespace civ5::runtime;using namespace civ5::saves;constexpr auto vp=TechnologyProfile::vp542;
    RulesDatabase db;db.apply_sql("CREATE TABLE Defines(Name,Value);INSERT INTO Defines VALUES('DEFAULT_SPECIALIST',0);CREATE TABLE Yields(ID,Type);INSERT INTO Yields VALUES(0,'YIELD_FOOD'),(4,'YIELD_CULTURE'),(6,'YIELD_TOURISM'),(8,'YIELD_CULTURE_LOCAL');CREATE TABLE BuildingClasses(ID,Type,MaxGlobalInstances);INSERT INTO BuildingClasses VALUES(0,'NORMAL',-1),(1,'WONDER',1),(2,'ZERO_LIMIT',0);CREATE TABLE Buildings(ID,Type,BuildingClass);INSERT INTO Buildings VALUES(0,'A','NORMAL'),(1,'B','WONDER'),(2,'C','ZERO_LIMIT');CREATE TABLE Building_YieldChanges(BuildingType,YieldType,Yield);INSERT INTO Building_YieldChanges VALUES('B','YIELD_CULTURE',7),('B','YIELD_CULTURE',3),('C','YIELD_CULTURE',5),('C','YIELD_CULTURE',0);CREATE TABLE Features(ID,Type,NaturalWonder,PseudoNaturalWonder);INSERT INTO Features VALUES(0,'NORMAL',0,0),(1,'NATURAL',1,0),(2,'PSEUDO',0,1);");
    const auto types=db.cache_info_types();const auto defines=db.cache_global_defines(GlobalDefineProfile::vp542);auto rules=load_vp_city_base_yield_rules(db,types);
    check(!rules.world_wonder_classes.at(0)&&rules.world_wonder_classes.at(1)&&rules.world_wonder_classes.at(2),"World wonder class accepts every global limit except minus one");
    check(rules.buildings.at(1).culture==3&&rules.buildings.at(2).culture==0&&rules.natural_wonders.at(2),"Culture SQL keeps last duplicate including zero; pseudo wonders count");
    SavedMapProjection map{{25,25}};map.cells.resize(625);for(std::size_t i=0;i<625;++i){auto &v=map.cells[i];auto xy=map.grid.coordinates(i);v.index=i;v.x=xy[0];v.y=xy[1];v.feature_type=-1;v.trade_route_units=0;v.cached_yields.emplace();for(std::int64_t y=0;y<9;++y)(*v.cached_yields)[y]=0;v.units.emplace();}
    const auto origin=map.grid.index(12,12);check(vp_city_ring_plot(map,12,12,0)==origin,"Ring zero selects city itself");
    // Independent walk starts north and follows the six hex edge directions.
    const std::array<std::array<int,2>,6> directions={{{1,-1},{0,-1},{-1,0},{-1,1},{0,1},{1,0}}};std::int64_t idx=1;
    for(int radius=1;radius<=8;++radius){int hx=6,hy=12+radius;for(auto d:directions)for(int k=0;k<radius;++k){check(vp_city_ring_plot(map,12,12,idx++)==map.grid.index(hx+hy/2,hy),"City ring matches independent hex edge walk beyond original 91 cells");hx+=d[0];hy+=d[1];}}
    rejects([&]{vp_city_ring_plot(map,12,12,-1);});check(!vp_city_ring_plot(map,12,12,INT64_MAX),"Largest signed ring index computes without 32-bit truncation or overflow");
    map.wrap_x=map.wrap_y=true;check(vp_city_ring_plot(map,0,-1,0)==map.grid.index(0,24),"Negative odd row uses shared wrapping");
    check(!vp_city_ring_plot(map,0,-25,0),"Original negative exact-multiple wrapping remains out of bounds");map.wrap_x=map.wrap_y=false;
    CityYieldContext world;world.profile=vp;world.yield_slots=9;auto &p=world.players[5];p.capital=42;p.total_population=31;vp_player_state_religion(p)=2;
    ensure_vp_player_accomplishments_profile(world).players[5]={};
    auto &city=p.cities[42];city.id=42;city.owner=5;city.x=city.y=12;city.population=10;vp_city_majority_religion(city)=2;city.building_counts_resolved=true;
    auto &great_work_profile=ensure_vp_great_work_profile(world);great_work_profile.players[5]={};auto &great_work_city=great_work_profile.cities[{5,42}];
    auto &trait_profile=ensure_vp_city_trait_yield_profile(world);trait_profile.players[5]={};trait_profile.eras[5]={};trait_profile.cities[{5,42}]={};
    auto &happiness_profile=ensure_vp_city_happiness_profile(world);happiness_profile.players[5]={};happiness_profile.cities[{5,42}]={};
    auto &building_yield_profile=ensure_vp_city_building_yield_profile(world);building_yield_profile.players[5]={};building_yield_profile.cities[{5,42}]={};
    auto &religion_minor_profile=ensure_vp_city_religion_minor_yield_profile(world);religion_minor_profile.players[5]={};religion_minor_profile.cities[{5,42}]={};
    auto &base_profile=ensure_vp_city_base_yield_profile(world);auto &a=base_profile.cities[{5,42}];auto &b=base_profile.players[5];
    ResearchContext players;players.profile=vp;players.players[5].team=9;CustomModOptions options;
    PlotPassability pass;PlotWaterRules wr;wr.profile=vp;PlotWaterState ws;ws.layout=GameStateLayout::vp542_corpus;PlotWaterContext water{map,ws,wr,types,pass,players,world,defines,&options};
    VpPlotReligionRules rr;rr.yields=rules.yields;VpPlotReligionContext religion{rr,water};CityYieldRules cr;cr.profile=vp;VpGreatWorkRules gr;gr.yields=rules.yields;gr.tourism=6;
    struct Services:VpGreatWorkServices{void unlock_capital_theming_achievement(std::int64_t)override{throw std::runtime_error("Unexpected base yield achievement");}} services;
    VpGreatWorkContext great{world,cr,gr,religion,services};MaintenanceContext units;units.profile=vp;units.team_eras[9]=3;
    VpCityFoodRules fr;fr.specialist_slots=3;VpCityFoodContext food{world,players,units.team_eras,fr,defines};VpCityHappinessRules hr;hr.yields=rules.yields;hr.culture=4;
    VpCityHappinessContext happiness{hr,great,food,units,players,db};VpCityBuildingYieldRules br;br.yields=rules.yields;VpCityBuildingYieldContext buildings{br,great,units.team_eras};
    TreasuryContext treasury;treasury.profile=vp;ensure_vp_treasury_religion_profile(treasury.players[5]).state_religion=2;ensure_vp_treasury_minor_profile(treasury.players[5]);VpCityReligionMinorContext rm{religion,treasury};VpCityTraitYieldRules tr;tr.yields=rules.yields;tr.culture=4;tr.tourism=6;
    VpCityTraitYieldContext traits{tr,players,world,treasury,units.team_eras,defines,options};VpCityBaseYieldContext c{rules,buildings,happiness,rm,traits};
    for(auto y:rules.yields)great_work_city.yield_cache[y]=0;
    const auto query=[&](std::int64_t y=0){return vp_city_base_yield(c,city,y);};check(query().total_times100==0,"Resolved empty base yield world gives zero");
    world.leagues={{{5,5},{},{2,-1}},{{6},{},{99}},{{5},{},{3}}};
    check(vp_league_world_wonder_yield(world,5,4,4)==4&&vp_league_world_wonder_yield(world,5,0,4)==0,"Congress sums ordered resolution effects once per member league and only for culture");
    great_work_city.existing_buildings={0,1,2,999};city.real_buildings[1]=5;a.landmarks_tourism=20;b.landmarks_tourism=5;city.yields[4].terrain=11;
    a.working.resize(218);a.working[0]=a.working[1]=a.working[217]=true;vp_rebuild_city_worked_plots(map,world,city);
    check(a.worked_plots.size()==3&&a.worked_plots.back()==vp_city_ring_plot(map,12,12,217),"Working flags reconstruct ordered plots without original array capacity");
    map.cells[origin].feature_type=2;(*map.cells[origin].cached_yields)[4]=13;
    const auto l=vp_city_landmark_tourism(c,city);check(l.wonders==11&&l.natural_wonders==13&&l.terrain==11&&l.total_times100==875,"Landmarks count each wonder type once, include cached pseudo-wonder culture and multiply directly into hundredths");
    a.worked_plots.push_back(origin);check(vp_city_landmark_tourism(c,city).natural_wonders==26,"Repeated worked plots retain source iteration semantics");a.worked_plots.pop_back();
    check(query(6).landmarks==875,"Tourism base includes landmark contribution");a.world_wonders=7;a.league[4]=2;check(query(4).league==3000,"Congress uses saved world-wonder count rather than building-list length");
    vp_city_cached_yields(world,5,42)[4]=12345;check(query(8).local_culture==12345,"Local culture base uses ordinary culture cache including hundredths");
    great_work_city.yield_cache[0]=17;city.yields[0].terrain=2;city.yields[0].buildings=3;happiness_profile.cities[{5,42}].policies[0]=4;city.yields[0].specialists=5;city.yields[0].religion=6;
    religion_minor_profile.cities[{5,42}].from_minors[0]=7;trait_profile.cities[{5,42}].added[0]=8;a.corporations[0]=9;a.events[0]=10;trait_profile.cities[{5,42}].connected=true;trait_profile.players[5].connection[0]=11;a.league[0]=12;b.state_religion[0]=13;
    city.yields[0].per_population=14;a.empire_population[0]=15;b.non_specialists[0]=16;
    auto &food_profile=ensure_vp_city_food_profile(world);food_profile.players[5]={};food_profile.cities[{5,42}].specialists={{0,99},{1,3},{2,2}};
    city.yields[0].misc=17;a.happiness[0]=18;a.health[0]=19;happiness_profile.cities[{5,42}].crime[0]=20;happiness_profile.cities[{5,42}].development[0]=21;a.previous_golden_ages[0]=22;b.capital_tiles[0]=23;
    auto q=query();check(q.great_works==17&&q.terrain==200&&q.buildings==300&&q.policies==400&&q.specialists==500&&q.religion==600&&q.minors==700&&q.traits==800&&q.connection==1100,"Base calculation composes existing complete helpers and retained caches");
    check(q.state_religion==1300&&q.population==140&&q.empire_population==465&&q.non_specialists==80&&q.misc==9500&&q.previous_golden_ages==2200&&q.capital_tiles==2300&&q.total_times100==23702,"Remaining source contributions retain units, raw empire population and ungated crime/development");
    city.population=2;check(query().non_specialists==-48,"Non-specialist yield preserves negative population-minus-specialists");city.population=10;
    vp_city_majority_religion(city)=0;check(query().state_religion==0,"Pantheon does not trigger state-religion bonus");vp_city_majority_religion(city)=2;
    p.capital=-1;check(query().capital_tiles==0,"Tile expenditure yield is capital only");p.capital=42;
    auto &unit_yield=ensure_vp_unit_yield_profile(units.players[6].units[55],units.profile);
    unit_yield.city_yield_change=YieldValues{{0,3}};map.cells[origin].units->push_back({6,55});map.cells[origin].units->push_back({6,55});options.entries["YIELD_MODIFIER_FROM_UNITS"].value=1;
    check(query().units==600,"Unit city yield includes foreign owner and duplicate plot handles");unit_yield.city_yield_change.reset();rejects([&]{query();});options.entries["YIELD_MODIFIER_FROM_UNITS"].value=0;check(query().units==0,"Disabled unit option never reads unresolved unit yield");
    b.state_religion[0]=-1000;check(query().total_times100<0,"Complete base yield has no zero clamp");constexpr std::int64_t big=4294967300LL;p.total_population=big;check(query().empire_population==big*15,"Empire population arithmetic exceeds 32 bits");
    p.total_population=INT64_MAX;rejects([&]{query();});p.total_population=31;auto detached=city;rejects([&]{vp_city_base_yield(c,detached,0);});rejects([&]{query(999);});
    SavedMapProjection tiny{{1,1}};tiny.wrap_x=tiny.wrap_y=true;a.working.assign(19,true);city.x=city.y=0;vp_rebuild_city_worked_plots(tiny,world,city);check(a.worked_plots.size()>1&&std::all_of(a.worked_plots.begin(),a.worked_plots.end(),[](auto n){return n==0;}),"Rebuild retains duplicate wrapped plots");
    auto copied=world;ensure_vp_city_base_yield_profile(copied).cities.at({5,42}).world_wonders=17;
    check(find_vp_city_base_yield_profile(world)->cities.at({5,42}).world_wonders==7,"VP base-yield profile copies independently");
    CityYieldContext vanilla;check(!find_vp_city_base_yield_profile(vanilla),"Vanilla runtime has no VP base-yield state");rejects([&]{ensure_vp_city_base_yield_profile(vanilla);});
}
