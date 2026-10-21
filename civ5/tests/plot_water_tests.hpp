#pragma once
void plot_water_tests(){
    using namespace civ5::runtime;using namespace civ5::saves;
    RulesDatabase db;db.apply_sql("CREATE TABLE Defines(Name,Value);INSERT INTO Defines VALUES('LAKE_MAX_AREA_SIZE',9),('MIN_WATER_SIZE_FOR_OCEAN',10);"
        "CREATE TABLE Features(ID,Type,AddsFreshWater);INSERT INTO Features VALUES(0,'ICE',0),(1,'OASIS',1);"
        "CREATE TABLE Improvements(ID,Type,AddsFreshWater);INSERT INTO Improvements VALUES(4294967300,'WELL',1);"
        "CREATE TABLE Buildings(ID,Type,AddsFreshWater);INSERT INTO Buildings VALUES(4294967300,'CISTERN',1);");
    const auto types=db.cache_info_types();auto rules=load_plot_water_rules(db,TechnologyProfile::vp542);
    auto defs=db.cache_global_defines(GlobalDefineProfile::vp542);
    check(rules.building_slots==4294967301ULL&&rules.improvements.at(4294967300LL)&&rules.buildings.at(4294967300LL),
        "Freshwater rules must accept sparse IDs above UINT32_MAX");
    SavedMapProjection map{{5,5}};PlotPassability pass;pass.layout=GameStateLayout::vp542_corpus;
    for(std::uint64_t id=0;id<25;++id){SavedMapCellProjection p;p.index=id;p.x=id%5;p.y=id/5;p.owner=-1;
        p.plot_type=2;p.terrain_type=0;p.feature_type=-1;p.improvement_type=-1;p.landmass=0;
        p.improvement_pillaged=false;p.improvement_passable=false;p.rivers=SavedRiverPlot{};map.cells.push_back(p);pass.plots.push_back({});}
    PlotWaterState s;s.layout=pass.layout;s.landmasses[0]={21,false};s.landmasses[1]={1,true};s.landmasses[2]={10,true};
    ResearchContext players;players.profile=TechnologyProfile::vp542;players.players[0].team=100;players.players[4294967300LL].team=100;
    CityYieldContext cities;cities.profile=players.profile;cities.players[0]={};cities.players[4294967300LL]={};
    CustomModOptions options;options.entries["GLOBAL_PASSABLE_FORTS"].value=1;
    PlotWaterContext c{map,s,rules,types,pass,players,cities,defs,&options};
    rejects([&]{plot_is_lake(c,12);});reset_plot_water_cache(c);
    // East neighbor has a later row-major index; its lake cache starts false.
    map.cells[13].plot_type=3;map.cells[13].terrain_type=5;map.cells[13].landmass=1;
    update_plot_water_flags(c,12);
    check(!plot_is_fresh_water(c,12)&&plot_is_coastal_land(c,12)&&!plot_is_lake(c,13),
        "VP cold water-cache pass must not eagerly recompute the later lake");
    update_plot_water_flags(c,13);
    check(plot_is_lake(c,13)&&plot_is_adjacent_to_land(c,13)&&!plot_is_fresh_water(c,12),
        "VP updating lake must not silently update neighbouring freshwater caches");
    check(plot_is_fresh_water(c,12,false)&&!plot_is_coastal_land(c,12,10),"Explicit VP refresh uses current adjacent lake cache");
    map.cells[13].feature_type=0;update_plot_water_flags(c,12);
    check(!plot_is_coastal_land(c,12)&&plot_is_adjacent_to_water(c,12),"VP cached coast excludes ice but direct water adjacency does not");
    map.cells[13].feature_type=-1;map.cells[13].landmass=2;update_plot_water_flags(c,13);update_plot_water_flags(c,12);
    check(!plot_is_fresh_water(c,12)&&!plot_is_lake(c,13)&&plot_is_coastal_land(c,12,10),"VP lake threshold is strictly below ocean size");
    map.cells[12].plot_type=0;map.cells[12].terrain_type=7;map.cells[12].rivers->is_river=true;pass.plots[12].impassable=true;
    check(plot_is_fresh_water(c,12,false),"VP rivers supply freshwater even on impassable mountains");
    map.cells[12].plot_type=3;check(plot_is_fresh_water(c,12,false),"VP water retains its river freshwater bit");
    map.cells[12].plot_type=2;map.cells[12].terrain_type=0;map.cells[12].rivers->is_river=false;pass.plots[12].impassable=false;
    map.cells[11].improvement_type=4294967300LL;map.cells[11].improvement_pillaged=true;
    check(plot_is_fresh_water(c,12,false),"Source freshwater improvements supply water even when pillaged");
    map.cells[11].improvement_type=-1;map.cells[12].feature_type=1;
    check(plot_is_fresh_water(c,12,false),"Water refresh must include the central plot itself");map.cells[12].feature_type=-1;
    map.cells[11].is_city=true;map.cells[11].owner=0;
    auto &city=cities.players[0].cities[4294967300LL];city.x=1;city.y=2;city.owner=0;city.id=4294967300LL;
    rejects([&]{plot_is_fresh_water(c,12,false);});
    city.building_counts_resolved=true;city.real_buildings[4294967300LL]=-4;city.free_buildings[4294967300LL]=1;
    check(plot_is_fresh_water(c,12,false),"City freshwater uses positive real OR free count, not their sum");
    city.free_buildings.clear();check(!plot_is_fresh_water(c,12,false),"Live building removal must affect the next water refresh");
    map.cells[11].is_city=false;map.cells[11].owner=-1;
    // One lake next to a city/canal joins another water body. The source BFS
    // counts the intervening land canal too, not just water cells.
    map.cells[13].landmass=1;map.cells[12].owner=0;map.cells[14].owner=4294967300LL;
    map.cells[14].improvement_passable=true;map.cells[14].plot_type=2;
    map.cells[19].plot_type=3;map.cells[19].landmass=1;map.cells[19].feature_type=-1;
    for(int i=0;i<25;++i)update_plot_water_flags(c,i);
    check(!plot_is_coastal_land(c,12,3,true,false)&&plot_is_coastal_land(c,12,3,true,true),
        "VP canal search must cross friendly passable land and count it in the connected region");
    map.cells[14].improvement_pillaged=true;
    check(!plot_is_coastal_land(c,12,3,true,true),"Pillaged canal must not connect the bodies of water");
    map.cells[14].improvement_pillaged=false;players.players[4294967300LL].team=4294967300LL;
    check(!plot_is_coastal_land(c,12,3,true,true),"Foreign canal must not be traversed");players.players[4294967300LL].team=100;
    options.entries["GLOBAL_PASSABLE_FORTS"].value=0;
    check(!plot_is_coastal_land(c,12,3,true,true),"Disabled passable forts must disable canal search");
    check(!plot_is_coastal_land(c,12,4294967300LL,true,true),"Large requested water size must not narrow to int32");
    // BNW queries ignore VP caches and derive water directly each time.
    s.layout=pass.layout=GameStateLayout::expansion2_version1;
    rules=load_plot_water_rules(db,TechnologyProfile::bnw);players.profile=cities.profile=TechnologyProfile::bnw;
    defs=db.cache_global_defines(GlobalDefineProfile::bnw);c.options=nullptr;
    check(plot_is_fresh_water(c,12)&&!plot_is_coastal_land(c,12),"BNW freshwater and default coast threshold differ from VP");
    check(plot_is_coastal_land(c,12,0)&&plot_is_coastal_land(c,12,1)&&plot_is_adjacent_to_land(c,12),"BNW direct adjacency includes neighbouring land on land plots");
    map.cells[12].plot_type=0;map.cells[12].rivers->is_river=true;
    check(!plot_is_fresh_water(c,12),"BNW mountains reject river freshwater");map.cells[12].plot_type=2;
    pass.plots[12].impassable=true;check(!plot_is_fresh_water(c,12),"BNW impassability precedes the river branch");pass.plots[12].impassable=false;
    check(plot_is_fresh_water(c,12),"BNW passable river provides freshwater");map.cells[12].rivers->is_river=false;
    s.landmasses[1].tiles=10;check(!plot_is_lake(c,13)&&!plot_is_fresh_water(c,12)&&plot_is_coastal_land(c,12),"BNW lake maximum is inclusive and distinct from coast minimum");
    map.cells[13].feature_type.reset();map.cells[13].feature_type_hash=saved_info_type_hash("OASIS");
    check(plot_is_fresh_water(c,12),"BNW feature water must resolve saved hashes through active content");
    map.cells[13].feature_type_hash=0;map.cells[11].improvement_type=4294967300LL;
    check(!plot_is_fresh_water(c,12),"BNW does not inherit VP improvement freshwater rules");
    map.cells[11].improvement_type=-1;
    rejects([&]{update_plot_water_flags(c,12);});
    // Restore original landmass IDs and VP real/free building vectors.
    SavedSessionSnapshot snapshot{};snapshot.map_header.layout=GameStateLayout::vp542_corpus;snapshot.map_regions.emplace();
    snapshot.map_regions->layout=snapshot.map_header.layout;MapRegionContainer masses{};masses.name="landmasses";
    MapRegionRecord m{};m.fields={{"id",4294967300LL},{"tiles",4294967301LL},{"water",1}};masses.records.push_back(m);snapshot.map_regions->containers.push_back(masses);
    const auto restored=restore_plot_water(snapshot);
    check(restored.landmasses.at(4294967300LL).tiles==4294967301LL,"Landmass import must preserve wide identity and tile count");
    rules.profile=cities.profile=TechnologyProfile::vp542;rules.building_slots=2;
    snapshot.all_players.emplace();snapshot.all_players->layout=snapshot.map_header.layout;
    SavedPlayerObservation sp{};sp.saved_player_id=0;SavedCityObservation sc{};sc.production.layout=snapshot.map_header.layout;
    CityCoreField id{};id.name="m_iID";id.values={4294967300LL};sc.core.fields.push_back(id);
    CityProductionRecord buildings{};buildings.name="buildings";
    buildings.arrays={{"m_paiNumRealBuilding",1,{0,4294967300LL}},{"m_paiNumFreeBuilding",1,{1,0}}};
    sc.production.blocks.push_back(buildings);sp.cities.cities.push_back(sc);snapshot.all_players->players.push_back(sp);
    restore_vp_water_city_buildings(snapshot,cities,rules);
    check(city.building_counts_resolved&&city.real_buildings.at(1)==4294967300LL&&city.free_buildings.at(0)==1,
        "VP water providers must restore into the shared city's live building counts");
    snapshot.all_players->players[0].cities.cities[0].production.blocks[0].arrays[0].values.pop_back();
    rejects([&]{restore_vp_water_city_buildings(snapshot,cities,rules);});
}
