#pragma once
void vp_city_building_yield_tests(){
    using namespace civ5::runtime;using namespace civ5::saves;
    constexpr auto vp=TechnologyProfile::vp542;
    RulesDatabase db;db.apply_sql("CREATE TABLE Defines(Name,Value);INSERT INTO Defines VALUES('CITY_MAX_NUM_BUILDINGS',1);CREATE TABLE Yields(ID,Type);INSERT INTO Yields VALUES(0,'YIELD_FOOD'),(6,'YIELD_TOURISM');CREATE TABLE Policies(ID,Type);INSERT INTO Policies VALUES(7,'POLICY');CREATE TABLE Buildings(ID,Type,FaithCost,Cost,UnlockedByBelief,PolicyType);INSERT INTO Buildings VALUES(10,'A',100,-1,1,NULL),(11,'B',50,-2,0,'POLICY'),(12,'C',0,-1,1,NULL),(13,'D',100,0,1,NULL),(14,'E',100,-1,0,'MISSING');CREATE TABLE Accomplishments(ID,Type);INSERT INTO Accomplishments VALUES(2,'EVENT');CREATE TABLE Beliefs(ID,Type,FaithBuildingTourism);INSERT INTO Beliefs VALUES(1,'BELIEF',3),(2,'NEGATIVE',-1);CREATE TABLE Building_YieldChangesFromAccomplishments(BuildingType,AccomplishmentType,YieldType,Yield);INSERT INTO Building_YieldChangesFromAccomplishments VALUES('A','EVENT','YIELD_FOOD',7),('A','EVENT','YIELD_FOOD',-2),('B','EVENT','YIELD_FOOD',1),('B','EVENT','YIELD_FOOD',-1),('A','MISSING','YIELD_TOURISM',2),('A','EVENT','UNKNOWN',99);");
    const auto types=db.cache_info_types();auto defines=db.cache_global_defines(GlobalDefineProfile::vp542);auto rules=load_vp_city_building_yield_rules(db,types);
    check(rules.building_slots==15&&rules.buildings.at(10).faith_only&&rules.buildings.at(11).faith_only&&!rules.buildings.at(12).faith_only&&!rules.buildings.at(13).faith_only&&!rules.buildings.at(14).faith_only,"Faith-only buildings require positive faith cost, negative production cost and belief or resolved policy");
    check(rules.buildings.at(10).accomplishments.at(2).at(0)==5&&rules.buildings.at(11).accomplishments.at(2).at(0)==0&&rules.buildings.at(10).accomplishments.at(0).at(6)==2,"Building accomplishment SQL adds duplicate rows, retains zero and preserves NULL-to-zero LEFT JOIN");
    CustomModOptions options;ResearchContext players;players.profile=vp;players.players[5].team=9;
    CityYieldContext vanilla;vanilla.players[5]={};
    check(!vanilla.extensions.find<VpPlayerAccomplishmentsProfileState>(),
          "Vanilla city context must not require VP accomplishment counters");
    rejects([&]{ensure_vp_player_accomplishments_profile(vanilla);});
    rejects([&]{vp_player_accomplishments(vanilla,5);});
    CityYieldContext cities;cities.profile=vp;cities.yield_slots=17;auto &p=cities.players[5];auto &accomplishments=ensure_vp_player_accomplishments_profile(cities).players[5];auto &building_profile=ensure_vp_city_building_yield_profile(cities);building_profile.players[5]={};auto &great_work_profile=ensure_vp_great_work_profile(cities);
    auto copied=cities;ensure_vp_player_accomplishments_profile(copied).players[5][2]=7;
    check(vp_player_accomplishments(copied,5).at(2)==7&&accomplishments.empty(),
          "Copied VP accomplishment state must be independent of the source world");
    auto &city=p.cities[42];city.owner=5;city.id=42;city.x=city.y=0;vp_city_majority_religion(city)=-1;city.building_counts_resolved=true;auto &a=building_profile.cities[{5,42}];auto &great_work_city=great_work_profile.cities[{5,42}];great_work_profile.players[5]={};
    SavedMapProjection map{{1,1}};map.cells.resize(1);map.cells[0].trade_route_units=0;PlotPassability pass;PlotWaterRules wr;wr.profile=vp;PlotWaterState ws;ws.layout=GameStateLayout::vp542_corpus;
    PlotWaterContext water{map,ws,wr,types,pass,players,cities,defines,&options};VpPlotReligionRules rr;rr.beliefs[1]={};rr.beliefs[2]={};VpPlotReligionContext religion{rr,water};
    CityYieldRules cr;cr.profile=vp;VpGreatWorkRules gr;gr.tourism=6;gr.buildings[10]={};gr.buildings[11]={};
    struct Services:VpGreatWorkServices{void unlock_capital_theming_achievement(std::int64_t)override{throw std::runtime_error("Unexpected achievement in building income");}} services;
    VpGreatWorkContext great{cities,cr,gr,religion,services};std::map<std::int64_t,std::int64_t> eras{{9,3}};VpCityBuildingYieldContext context{rules,great,eras};
    const auto query=[&](std::int64_t y=0){return vp_city_building_yield(context,city,y);};
    city.yields[0].buildings=7;a.era_scaling_times100[0]=11;a.buildings=3;a.plots={2,4};a.fractions[0][0]={1,3};a.fractions[1][0]={-1,3};a.fractions[2][0]={2,3};building_profile.players[5].strategic_from_minors=4;
    a.passing_trade[0]=9;a.per_x_terrain[0]={{0,13},{0,17},{1,-1}};a.per_x_feature[0]={{0,19},{0,-23}};
    auto q=query();check(q.base==700&&q.era==33&&q.per_building==100&&q.per_tile==-66&&q.minor_resources==266&&q.terrain==29&&q.features==-4&&q.passing_trade==0&&q.total_times100==1058,"Building contribution retains raw building and plot counts, duplicate rows and one fractional truncation after scaling");
    map.cells[0].trade_route_units=1;check(query().passing_trade==900&&query().total_times100==1958,"Passing trade yields require a positive live trade route count on the city tile");
    map.cells[0].trade_route_units=-1;check(query().passing_trade==0,"Nonpositive trade route count does not activate passing income");map.cells[0].trade_route_units=0;
    eras[9]=-2;check(query().era==11,"Era scaling uses a minimum team era of one");eras[9]=3;
    city.real_buildings[10]=2;city.free_buildings[10]=3;accomplishments[2]=4;check(query().accomplishments==6000,"Accomplishment buildings use live max(real,free) and repeated completion count");
    check(query().per_building==100,"Saved total building count is independent of the per-type building maps");
    db.apply_sql("UPDATE Defines SET Value=2 WHERE Name='CITY_MAX_NUM_BUILDINGS'");defines=db.cache_global_defines(GlobalDefineProfile::vp542);check(query().accomplishments==10000,"Multiple-building mode adds real and free copies for accomplishment income");
    accomplishments[2]=-1;check(query().accomplishments==0,"Nonpositive accomplishment completions do not grant yields");accomplishments.clear();
    city.yields[0].per_religion=7;building_profile.players[5].per_religion_times100[0]=11;city.religions={{0,3,1},{1,1,1},{1,2,1},{2,0,1},{3,-1,1}};
    check(query().religions==36,"Religion income counts each positive follower record above pantheon, preserving duplicate records");
    great_work_city.existing_buildings={10,11};great_work_city.theming_indices[10]=0;great_work_city.theming_indices[11]=3;
    great_work_city.per_local_theme[0]=2;check(query().local_themes==400,"Local theme building income counts selected indices without requiring works or positive theme bonus");
    great_work_city.per_local_theme[0]=-2;check(query().local_themes==0,"Negative local theme yield is ignored in the buildings contribution");
    CityYieldReligionBeliefs r;r.religion=2;r.founder=5;r.beliefs={1,1,2};ensure_vp_religion_beliefs(r,vp);vp_religion_beliefs(r).belief_religion=2;vp_religion_beliefs(r).holy_x=vp_religion_beliefs(r).holy_y=-1;cities.religions={r};vp_city_majority_religion(city)=2;
    check(query(6).faith==1000,"Faith building tourism preserves duplicate beliefs and counts each eligible type once regardless of copies");
    rr.beliefs[1].founder=true;check(query(6).faith==-200,"Faith building tourism applies live founder validity and keeps negative income");rr.beliefs[1].founder=false;
    rr.beliefs[1].follower=true;vp_city_majority_religion(city)=0;cities.religions[0].religion=0;vp_religion_beliefs(cities.religions[0]).belief_religion=0;vp_religion_beliefs(cities.religions[0]).pantheon=true;
    check(query(6).faith==1000,"Faith building tourism uses direct owner pantheon lookup without a major-religion gate");
    cities.religions[0].founder=6;check(query(6).faith==0,"Owner-specific pantheon lookup rejects another player's pantheon");
    check(YieldFraction(1,3).scaled_truncate(3,100)==100&&YieldFraction(-1,3).scaled_truncate(2,100)==-66,"Scaled fraction truncates only after both factors");
    check(YieldFraction(INT64_MAX,INT64_MAX-1).scaled_truncate(INT64_MAX-1)==INT64_MAX,"Wide fractional product can exceed int64 while its quotient fits");
    check(YieldFraction(INT64_MIN).scaled_truncate(1)==INT64_MIN&&YieldFraction(INT64_MIN).scaled_truncate(INT64_MIN,0)==0,"Scaled fraction handles signed minimum and zero without signed overflow");
    check(YieldFraction(1,3).scaled_truncate(-2,-100)==66,"Scaled fraction preserves signs of both factors");
    rejects([&]{YieldFraction(INT64_MIN).scaled_truncate(-1);});rejects([&]{YieldFraction(INT64_MAX).scaled_truncate(INT64_MAX,INT64_MAX);});
    constexpr std::int64_t big=4294967300LL;a.buildings=big;a.fractions[0][0]={1,1};check(query().per_building==big*100,"Owned building count and yield arithmetic exceed uint32");
    city.yields[0].buildings=INT64_MAX;rejects([&]{query();});city.yields[0].buildings=0;
    auto detached=cities;VpGreatWorkContext wrong{detached,cr,gr,religion,services};VpCityBuildingYieldContext wrong_context{rules,wrong,eras};rejects([&]{vp_city_building_yield(wrong_context,detached.players.at(5).cities.at(42),0);});
    rejects([&]{query(99);});map.cells[0].trade_route_units.reset();rejects([&]{query();});map.cells[0].trade_route_units=0;ensure_vp_player_accomplishments_profile(cities).players.erase(5);rejects([&]{query();});
}
