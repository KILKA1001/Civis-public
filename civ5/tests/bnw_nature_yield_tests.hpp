#pragma once
void bnw_nature_yield_tests(){
    using namespace civ5::runtime;using namespace civ5::saves;
    RulesDatabase db;
    db.apply_sql("CREATE TABLE Defines(Name,Value);INSERT INTO Defines VALUES('LAKE_MAX_AREA_SIZE',2),('BARBARIAN_CAMP_IMPROVEMENT',99);"
        "CREATE TABLE Yields(ID,Type,HillsChange,MountainChange,LakeChange);INSERT INTO Yields VALUES(0,'FOOD',2,5,3),(4,'CULTURE',2,5,3);"
        "CREATE TABLE Terrains(ID,Type);INSERT INTO Terrains VALUES(0,'LAND');"
        "CREATE TABLE Features(ID,Type,YieldNotAdditive,NaturalWonder);INSERT INTO Features VALUES(0,'WONDER',0,1),(1,'FOREST',0,0);"
        "CREATE TABLE Resources(ID,Type);INSERT INTO Resources VALUES(8,'RESOURCE');"
        "CREATE TABLE Beliefs(ID,Type,Pantheon);INSERT INTO Beliefs VALUES(7,'MAIN',0),(8,'OTHER',1),(9,'SECOND',1);");
    for(const auto &[table,key]:std::vector<std::pair<std::string,std::string>>{
        {"Terrain_Yields","TerrainType"},{"Terrain_RiverYieldChanges","TerrainType"},{"Terrain_HillsYieldChanges","TerrainType"},
        {"Feature_YieldChanges","FeatureType"},{"Feature_RiverYieldChanges","FeatureType"},{"Feature_HillsYieldChanges","FeatureType"},
        {"Resource_YieldChanges","ResourceType"},{"Belief_YieldChangeNaturalWonder","BeliefType"},{"Belief_YieldModifierNaturalWonder","BeliefType"}})
        db.apply_sql("CREATE TABLE "+table+"("+key+",YieldType,Yield);");
    for(const auto &target:std::vector<std::string>{"Terrain","Feature","Resource"})
        db.apply_sql("CREATE TABLE Belief_"+target+"YieldChanges(BeliefType,"+target+"Type,YieldType,Yield);");
    for(const auto &y:std::vector<std::string>{"FOOD","CULTURE"}){
        const auto row=[&](const std::string &table,const std::string &key,std::int64_t n){
            db.apply_sql("INSERT INTO "+table+" VALUES('"+key+"','"+y+"',"+std::to_string(n)+");");};
        row("Terrain_Yields","LAND",11);row("Terrain_RiverYieldChanges","LAND",47);row("Terrain_HillsYieldChanges","LAND",53);
        row("Feature_YieldChanges","WONDER",7);row("Feature_RiverYieldChanges","WONDER",41);row("Feature_HillsYieldChanges","WONDER",43);
        row("Resource_YieldChanges","RESOURCE",29);row("Belief_YieldChangeNaturalWonder","MAIN",17);row("Belief_YieldChangeNaturalWonder","SECOND",19);
        row("Belief_YieldModifierNaturalWonder","MAIN",50);row("Belief_YieldModifierNaturalWonder","SECOND",25);
        const auto relation=[&](const std::string &target,const std::string &key,std::int64_t a,std::int64_t b){
            db.apply_sql("INSERT INTO Belief_"+target+"YieldChanges VALUES('MAIN','"+key+"','"+y+"',"+std::to_string(a)+"),('SECOND','"+key+"','"+y+"',"+std::to_string(b)+");");};
        relation("Terrain","LAND",3,5);relation("Feature","WONDER",11,13);relation("Resource","RESOURCE",31,37);
    }
    db.apply_sql("INSERT INTO Feature_YieldChanges VALUES('FOREST','FOOD',11),('FOREST','FOOD',0),('FOREST','UNKNOWN',999);");
    auto rules=load_bnw_nature_yield_rules(db);
    check(rules.yield_slots==5&&rules.feature_slots==2&&rules.features.at(1).base.empty()&&rules.beliefs.at(9).pantheon,
        "Nature cache uses Info slots, joined relation IDs and last-row zero replacement");
    const auto types=db.cache_info_types();const auto defs=db.cache_global_defines(GlobalDefineProfile::bnw);
    ResearchContext players;players.players[5]={9,-1,2,0,true,{},true};players.players[6]={9,-1,2,0,true,{},true};
    PlayerResearchContext roster;roster.teams[9].members={5,6};
    std::vector<TeamTechnologies> technologies;technologies.emplace_back(TechnologyProfile::bnw,9,2);
    ResourceVisibilityRules rr;rr.resources[8]={-1,-1};ResourceVisibilityState rs;rs.teams[9]={};
    SavedMapProjection map{{1,1}};SavedMapCellProjection p;p.index=0;p.x=0;p.y=0;p.plot_type=1;p.terrain_type=0;p.feature_type=0;p.improvement_type=-1;
    p.owner=5;p.resource_type=8;p.force_resource_reveal.emplace();p.landmass=0;p.rivers=SavedRiverPlot{};p.rivers->is_river=true;
    p.yield_city=SavedPlotCityReference{6,44};p.improvement_pillaged=true;map.cells.push_back(p);auto &plot=map.cells[0];
    PlotPassability pass;pass.plots.push_back({});PlotWaterState ws;ws.landmasses[0]={1,false};PlotWaterRules wr;
    CityYieldContext cities;cities.yield_slots=5;cities.players[5].second_pantheon_count=0;cities.players[6].second_pantheon_count=1;
    auto &city=cities.players[6].cities[44];city.id=44;city.owner=6;city.religions={{1,6,20},{2,4,30}};
    cities.religions={{1,6,{7,7}},{2,6,{9,8}}};
    PlotWaterContext water{map,ws,wr,types,pass,players,cities,defs};
    ResourceVisibilityContext resources{map,rs,rr,players,roster,technologies,types,defs,false};
    BnwNatureYieldState state;auto &traits=state.players[5];traits.unimproved_features[0]={{0,2},{4,2}};traits.natural_wonder={{0,23},{4,23}};traits.natural_wonder_modifier=-5;
    state.leagues={{{6,6},{13,7}},{{5},{999}},{{6},{-3}}};
    BnwNatureYieldContext c{rules,state,water,resources};
    const auto query=[&](std::int64_t y=4,std::int64_t team=9,bool ignore=false){return bnw_nature_yield(c,0,y,team,ignore);};
    auto q=query();
    check(q.total==387&&q.feature_total==185&&q.league==17&&q.religion_terrain==8&&q.religion_resource==68,
        "Complete BNW nature formula uses working-city religion/congress and actual owner's traits");
    check(query(0).total==358&&query(0).league==0,"Congress natural-wonder bonus applies only to culture");
    check(query(4,-1).total==290,"NO_TEAM omits both base and religious resource yields");
    check(query(4,9,true).total==218&&bnw_best_nature_yield(c,0,4,9)==387,"Ignoring feature selects terrain river/hills while keeping religion terrain and visible resource");
    rules.features[0].nonadditive=true;q=query();check(q.total==366&&q.replaced,"Nonadditive feature replaces all earlier terrain/hill/lake/religion yield");
    ws.landmasses[0].water=true;check(query().total==366&&query(4,9,true).total==221,"Replacement removes lake change too; ignore-feature retains it");ws.landmasses[0].water=false;
    rules.features[0].nonadditive=false;
    plot.improvement_type=17;check(query().trait_feature==0&&query().feature_total==181,"An existing pillaged improvement still suppresses unimproved-feature trait");plot.improvement_type=-1;
    plot.owner=-1;q=query();check(q.trait_feature==0&&q.trait_wonder==0&&q.religion_wonder==0&&q.wonder_modifier==0&&q.feature_total==48,
        "Unowned natural wonder retains city feature/congress bonuses but skips the entire owner-gated wonder branch");plot.owner=5;
    traits.natural_wonder_modifier=-100;q=query();check(q.wonder_modifier==-25&&q.feature_total==109,"Nonpositive combined wonder modifier does not reduce yields");traits.natural_wonder_modifier=-5;
    rules.features[0].base[4]=-113;check(query().feature_total==-18&&query().total==184,"Negative feature scaling truncates toward zero before resource/river/hill additions");rules.features[0].base[4]=7;
    cities.religions[1].beliefs={8,9};check(query().religion_feature==11,"Secondary pantheon uses first saved pantheon even when its bonus is zero");cities.religions[1].beliefs={9,8};
    city.religions={{1,5,20},{2,5,20}};check(query().religion_feature==24,"Exact half and tied pressure retain first religion row");
    city.religions={{1,4,20},{2,3,30},{3,3,40}};check(query().religion_feature==0&&query().religion_resource==0,"Plurality below half has no religious yield");
    city.religions={{0,6,20},{2,4,30}};cities.religions.insert(cities.religions.begin(),{0,5,{8}});cities.religions.insert(cities.religions.begin()+1,{0,6,{7}});
    check(query().religion_feature==24,"Pantheon majority resolves the working-city founder");
    city.religions={{1,6,20},{2,4,30}};
    plot.yield_city=SavedPlotCityReference{6,999};q=query();check(q.league==0&&q.religion_feature==0&&q.trait_wonder==23,"Stale working-city handle resolves null without borrowing another city");plot.yield_city=SavedPlotCityReference{6,44};
    pass.plots[0].impassable=true;plot.feature_type=1;check(query().suppressed&&query(4,9,true).total==0,"Impassable ordinary feature returns zero even with ignore-feature");
    plot.feature_type=0;check(!query().suppressed&&query(4,9,true).total==218,"Actual natural wonder permits an impassable plot even when ignored");
    pass.plots[0].impassable=false;plot.plot_type=0;plot.feature_type=-1;check(query().suppressed,"Passable mountain without a natural wonder is suppressed");
    plot.feature_type=0;check(query().mountain==5&&query().plot_hills==0&&query().hills==0,"Mountain natural wonder receives YieldInfo mountain change");plot.plot_type=1;
    rules.terrains[0].base[4]=-10000;check(query().total==0&&bnw_best_nature_yield(c,0,4,9)==0,"Final nature result clamps negative sums to zero");
    rules.terrains[0].base[4]=4294967300LL;check(query().total>UINT32_MAX,"Nature sum retains values wider than int32");
    rules.terrains[0].base[4]=INT64_MAX;rejects([&]{query();});rules.terrains[0].base[4]=11;
    constexpr std::int64_t wide=4294967301LL;
    rules.features[wide]=rules.features[0];plot.feature_type=wide;traits.unimproved_features[wide][4]=2;
    check(query().trait_feature==2&&query().feature==7,"Owned nature Info identifiers remain wider than int32 without dense allocation");plot.feature_type=0;
    rules.yields[wide]={};rules.terrains[0].base[wide]=wide;rules.features[0].base[wide]=9;c.culture=wide;
    check(query(wide).total==wide+26&&query(wide).league==17,"Wide yield identity also selects the congress culture bonus without narrowing");c.culture=4;
    plot.feature_type_hash=saved_info_type_hash("WONDER");rejects([&]{query();});plot.feature_type.reset();check(query().total==387,"BNW nature resolves original hashed feature identity");plot.feature_type_hash.reset();plot.feature_type=0;
    plot.rivers.reset();rejects([&]{query();});plot.rivers=SavedRiverPlot{};plot.yield_city.reset();rejects([&]{query();});
    auto vp_water_rules=wr;vp_water_rules.profile=TechnologyProfile::vp542;PlotWaterContext wrong_water{map,ws,vp_water_rules,types,pass,players,cities,defs};
    BnwNatureYieldContext wrong{rules,state,wrong_water,resources};rejects([&]{bnw_nature_yield(wrong,0,4);});

    SavedSessionSnapshot snapshot;snapshot.all_players.emplace();snapshot.all_players->layout=snapshot.leagues.layout=GameStateLayout::expansion2_version1;
    SavedPlayerObservation saved;saved.saved_player_id=wide;saved.traits.layout=saved.core.layout=GameStateLayout::expansion2_version1;
    PlayerTraitsField f{"m_ppaaiUnimprovedFeatureYieldChange",PlayerTraitsEncoding::yield_rows};f.columns=2;f.values={0,wide,7,8};saved.traits.fields.push_back(f);
    f={"m_iYieldChangeNaturalWonder",PlayerTraitsEncoding::yield_array};f.values={wide,0};saved.traits.fields.push_back(f);
    f={"m_iNaturalWonderYieldModifier",PlayerTraitsEncoding::i32};f.values={wide+1};saved.traits.fields.push_back(f);
    PlayerCoreField count{"m_iSecondReligionPantheonCount",PlayerCoreEncoding::i32};count.values={wide+2};saved.core.fields.push_back(count);snapshot.all_players->players.push_back(saved);
    LeagueObservation league;league.members={{wide},{wide}};ResolutionObservation resolution;resolution.effects.integers={{"culture_per_natural_wonder",wide}};
    league.active_resolutions={resolution,resolution};resolution.effects.integers[0].value=999;league.enact_proposals={resolution};league.repeal_proposals={resolution};snapshot.leagues.leagues={league};
    BnwNatureYieldRules import_rules;import_rules.feature_slots=2;import_rules.yield_slots=2;CityYieldContext import_cities;import_cities.yield_slots=2;import_cities.players[wide]={};
    const auto restored=restore_bnw_nature_yields(snapshot,import_rules,import_cities);
    check(restored.players.at(wide).unimproved_features.at(0).at(1)==wide&&restored.players.at(wide).natural_wonder.at(0)==wide&&restored.players.at(wide).natural_wonder_modifier==wide+1&&import_cities.players.at(wide).second_pantheon_count==wide+2,
        "Nature import retains wide IDs/counters and trait matrix axes");
    check(restored.leagues[0].members==std::vector<std::int64_t>{wide,wide}&&restored.leagues[0].culture_per_natural_wonder==std::vector<std::int64_t>{wide,wide},
        "Nature import preserves league/member/resolution order and ignores pending enact/repeal proposals");
    auto broken=snapshot;broken.all_players->players.push_back(saved);rejects([&]{restore_bnw_nature_yields(broken,import_rules,import_cities);});
    broken=snapshot;broken.all_players->players[0].traits.fields[0].columns=1;rejects([&]{restore_bnw_nature_yields(broken,import_rules,import_cities);});
    broken=snapshot;broken.all_players->players[0].traits.fields[1].values.pop_back();rejects([&]{restore_bnw_nature_yields(broken,import_rules,import_cities);});
    broken=snapshot;broken.leagues.leagues[0].active_resolutions[0].effects.integers.clear();rejects([&]{restore_bnw_nature_yields(broken,import_rules,import_cities);});
    broken=snapshot;broken.leagues.layout=GameStateLayout::vp542_corpus;rejects([&]{restore_bnw_nature_yields(broken,import_rules,import_cities);});
}
