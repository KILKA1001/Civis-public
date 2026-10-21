#pragma once

void lua_map_tests(){
    using namespace civ5::runtime;using namespace civ5::saves;
    for(const auto layout:{GameStateLayout::expansion2_version1,GameStateLayout::vp542_corpus}){
        MapGeometry g{{7,7},false,false,layout};
        // Independent cube-coordinate distance oracle on the bounded map.
        for(std::int64_t a=0;a<49;++a)for(std::int64_t b=0;b<49;++b){
            const auto ax=a%7-a/7/2,ay=a/7,bx=b%7-b/7/2,by=b/7;
            const auto expected=std::max({std::abs(ax-bx),std::abs(ay-by),std::abs(ax+ay-bx-by)});
            check(g.distance(a%7,a/7,b%7,b/7)==expected,"Source hex distance matches independent cube metric without wrapping");
        }
        const std::int64_t even[]={23,17,9,8,15,22},odd[]={32,25,18,17,23,31};
        for(int d=0;d<6;++d){check(g.direction(2,2,d)==even[d],"Even-row direction");check(g.direction(3,3,d)==odd[d],"Odd-row direction");}
        g.wrap_x=true;
        check(g.plot(-1,0)==6&&!g.plot(-7,0)&&g.plot(7,0)==0&&!g.plot(-2147483647LL,0),"Source wrapping and invalid sentinel");
        check(g.direction(0,0,4)==6&&!g.direction(0,0,2),"Direction wraps X and clips Y");
        check(g.distance(0,0,6,0)==1,"Horizontal wrapped distance");
        if(layout==GameStateLayout::vp542_corpus){check(g.direction(3,3,-1)==25&&g.direction(3,3,7)==25,"VP normalizes directions");}
        else {check(g.direction(3,3,-1)==24,"BNW no direction returns centre");rejects([&]{g.direction(3,3,6);});}
        MapGeometry wide_map{{5000000003ULL,2},false,false,layout};
        check(wide_map.plot(5000000001LL,1)==10000000004LL&&wide_map.distance(0,0,5000000001LL,0)==5000000001LL,
              "Map coordinates, indices and distances exceed x86 capacities without allocating a giant map");
        rejects([&]{g.distance(INT64_MIN,0,INT64_MAX,0);});
    }
    // Frozen results from the two source formulas, including their different
    // treatment of large offsets and odd-height toroidal maps.
    const std::int64_t vectors[][10]={
        {7,7,1,0,0,0,6,6,6,5},{7,7,1,1,0,0,6,6,1,3},{8,6,1,1,0,0,4,3,6,4},
        {8,6,1,1,0,0,20,9,6,14},{7,7,0,1,-3,-3,2,8,7,7},{7,7,1,1,2,-4,-8,12,9,11},
        {7,7,1,1,0,0,0,6,1,3},{7,7,1,1,0,6,0,0,1,3}};
    for(const auto &v:vectors)for(int vp=0;vp<2;++vp){
        MapGeometry g{{static_cast<std::uint64_t>(v[0]),static_cast<std::uint64_t>(v[1])},v[2]!=0,v[3]!=0,
            vp?GameStateLayout::vp542_corpus:GameStateLayout::expansion2_version1};
        check(g.distance(v[4],v[5],v[6],v[7])==v[8+vp],"Profile-specific wrapped source distance");
    }
    SavedSessionSnapshot snap;snap.map_header.layout=GameStateLayout::expansion2_version1;snap.map_header.grid={4,2};snap.map_header.wrap_x=true;
    snap.map_plots=MapPlotsObservation{GameStateLayout::expansion2_version1};snap.map_projection=SavedMapProjection{{4,2},true,false};
    auto &map=*snap.map_projection;
    for(std::uint64_t i=0;i<8;++i){SavedMapCellProjection cell;cell.index=i;cell.x=i%4;cell.y=i/4;
        cell.plot_type=2;cell.terrain_type=0;cell.feature_type=-1;map.cells.push_back(cell);
        PlotObservation saved;saved.index=i;saved.x=i%4;saved.y=i/4;snap.map_plots->plots.push_back(saved);}
    RulesDatabase db;db.apply_sql("CREATE TABLE Terrains(ID INTEGER,Type TEXT,Impassable BOOLEAN);"
        "INSERT INTO Terrains VALUES(0,'TERRAIN_TEST',1);"
        "CREATE TABLE Features(ID INTEGER,Type TEXT,Impassable BOOLEAN);"
        "INSERT INTO Features VALUES(0,'FEATURE_PASSABLE',0),(2,'FEATURE_BLOCKED',1);"
        "CREATE TABLE GameOptions(ID INTEGER,Type TEXT,\"Default\" INTEGER);"
        "INSERT INTO GameOptions VALUES(0,'GAMEOPTION_PITBOSS',1);");
    const auto types=db.cache_info_types();
    map.cells[1].feature_type=0;map.cells[2].feature_type=1;map.cells[3].feature_type.reset();
    map.cells[3].feature_type_hash=civ5::content::saved_info_type_hash("FEATURE_BLOCKED");
    map.cells[4].terrain_type=-1;map.cells[4].plot_type=0;map.cells[5].plot_type=3;
    auto pass=restore_plot_passability(snap,db,types);
    check(pass.plots[0].impassable&&!pass.plots[1].impassable&&!pass.plots[2].impassable&&pass.plots[3].impassable&&
        !pass.plots[4].impassable,"BNW rebuild uses feature override, collection holes, hashes and no-terrain default");
    check(pass.extensions.find<VpPlotPassabilityProfileState>()==nullptr,
          "BNW plot passability must not allocate VP-only team flags");
    rejects([&]{ensure_vp_plot_team_flags(pass,0);});
    ResearchContext players;players.game_turn=2;players.players[0].slot_status=3;players.players[1].slot_status=0;
    std::vector<TeamTechnologies> teams;
    auto provider=lua_map_services(map,pass);
    LuaScriptHost host({}, {},{},lua_gameplay_services(db,players,teams,types),{},{},provider);
    host.execute("map",R"lua(
        local w,h=Map.GetGridSize();assert(w==4 and h==2 and select('#',Map.GetGridSize())==2)
        assert(Map.GetNumPlots()==8 and Map.IsWrapX() and not Map.IsWrapY())
        assert(PlotTypes.NO_PLOT==-1 and PlotTypes.PLOT_MOUNTAIN==0 and PlotTypes.PLOT_HILLS==1
            and PlotTypes.PLOT_LAND==2 and PlotTypes.PLOT_OCEAN==3 and PlotTypes.NUM_PLOT_TYPES==4)
        p=Map.GetPlot(0,0);assert(p==Map.GetPlotByIndex(0) and p==Map.PlotDirection(0,0,-1))
        assert(p:GetX()==0 and p:GetY()==0 and p:GetPlotIndex()==0 and p.GetIndex==nil and not p:IsWater() and not p:IsMountain())
        assert(p:IsImpassable() and p:IsImpassable('ignored by BNW'))
        assert(Map.GetPlot(0,1):IsMountain() and Map.GetPlot(1,1):IsWater())
        assert(select('#',Map.GetPlot(0,-1))==0 and select('#',Map.GetPlotByIndex(8))==0)
        assert(select('#',Map.PlotDirection(0,0,2))==1 and Map.PlotDirection(0,0,2)==nil)
        assert(Map.GetPlotByIndex('1.6'):GetPlotIndex()==2 and Map.GetPlotByIndex():GetPlotIndex()==0)
        assert(not pcall(p.GetX) and not pcall(p.GetX,Players[0]))
        assert(not pcall(Map.GetPlot,9007199254740994,0))
        assert(Players[0]:IsHuman() and Players[1]:IsHuman())
        p.custom='first';Plot.marker=7;DirectionTypes.DIRECTION_EAST=99;PlotTypes.PLOT_OCEAN=99
    )lua","=map-api");
    check(players.pitboss==true&&players.game_options.size()==1,"Inactive open player resolves and caches actual PitBoss option");
    map.cells[0].plot_type=3;pass.plots[0].impassable=false;players.players[0].slot_status=1;
    host.execute("map","assert(p:IsWater() and not p:IsImpassable() and not Players[0]:IsHuman())","=live-map");
    host.execute("other","local p=Map.GetPlot(0,0);assert(p.custom==nil and Plot.marker==nil and DirectionTypes.DIRECTION_EAST==1 and PlotTypes.PLOT_OCEAN==3 and p:IsWater())","=context-map");
    host.complete_loading();check(host.loading_complete(),"Read-only Lua map shares live authoritative model");
    auto partial=provider;partial.distance={};rejects([&]{LuaScriptHost bad({}, {},{},{},{},{},partial);});
    LuaScriptHost absent;absent.execute("absent","assert(Map==nil and Plot==nil and DirectionTypes==nil and PlotTypes==nil)","=no-map");
    map.cells[0].plot_type.reset();LuaScriptHost missing({}, {},{},{},{},{},provider);
    missing.execute("missing","assert(not pcall(function() return Map.GetPlot(0,0):IsWater() end))","=unresolved-map");
    rejects([&]{missing.complete_loading();});
    snap.map_header.layout=GameStateLayout::vp542_corpus;snap.map_plots->layout=snap.map_header.layout;snap.map_plots->dimensions.teams=129;
    for(auto &p:snap.map_plots->plots){p.fields.push_back({"packed_flags",16384});p.arrays.push_back({"teams",10,std::vector<std::int64_t>(1290,0)});p.arrays.back().values[1287]=1;}
    pass=restore_plot_passability(snap,db,types);map.cells[0].plot_type=2;
    check(pass.extensions.find<VpPlotPassabilityProfileState>()!=nullptr&&
          !vp_plot_team_impassable(pass,0,0)&&vp_plot_team_impassable(pass,0,128),
          "VP saved team flags remain explicit in the profile extension after restoration");
    LuaScriptHost vp({}, {},{},{},{},{},lua_map_services(map,pass));
    vp.execute("vp",R"lua(
        local p=Map.GetPlot(0,0);assert(p:GetPlotIndex()==0 and p:GetIndex()==0)
        assert(p:IsImpassable() and p:IsImpassable(nil) and p:IsImpassable(-1))
        assert(not p:IsImpassable(0) and p:IsImpassable(128) and p:IsImpassable('128'))
        assert(not pcall(p.IsImpassable,p,'invalid'))
        assert(Map.PlotDirection(0,0,-1)==Map.GetPlot(1,0))
    )lua","=vp-map");vp.complete_loading();
    check(vp.loading_complete(),"VP saved per-team impassability supports more than 64 teams");
    map.cells[0].plot_type=2;map.cells[0].terrain_type=7;
    vp.execute("vp","assert(Map.GetPlot(0,0):IsMountain())","=vp-terrain-mountain");
    auto bnw_pass=pass;bnw_pass.layout=GameStateLayout::expansion2_version1;
    LuaScriptHost bnw_mountain({}, {},{},{},{},{},lua_map_services(map,bnw_pass));
    bnw_mountain.execute("bnw","assert(not Map.GetPlot(0,0):IsMountain())","=bnw-plot-mountain");
    map.cells[0].plot_type=0;map.cells[0].terrain_type=0;
    bnw_mountain.execute("bnw","assert(Map.GetPlot(0,0):IsMountain())","=bnw-mountain-category");
    vp.execute("vp","assert(Map.GetPlot(0,0):IsMountain())","=vp-mountain-category");
    vp.complete_loading();bnw_mountain.complete_loading();
    check(vp.loading_complete()&&bnw_mountain.loading_complete(),"VP terrain mountain and BNW plot mountain follow separate source predicates");
    players.profile=TechnologyProfile::vp542;players.players[0].slot_status=3;
    civ5::content::CustomModOptions options;options.entries["AI_CONTROL_CITY_MANAGEMENT"].value=1;
    options.entries["AI_CONTROL_UNITS"].value=1;options.entries["HUMAN_USES_AI_MECHANICS"].value=1;
    options.entries["HUMAN_USES_AI_HANDICAP"].value=2;
    LuaScriptHost reason({}, {},{},lua_gameplay_services(db,players,teams,types,&options));
    reason.execute("reason",R"lua(
        assert(not Players[0]:IsHuman() and not Players[0]:IsHuman(nil) and not Players[0]:IsHuman('not numeric'))
        for i=-1,22 do assert(Players[0]:IsHuman(i)==(i~=0 and i~=13 and i~=18)) end
        assert(Players[0]:IsHuman(5000000001) and not Players[0]:IsHuman('13'))
    )lua","=human-reasons");
    options.entries["AI_CONTROL_CITY_MANAGEMENT"].value=0;
    reason.execute("reason","assert(Players[0]:IsHuman())","=live-human-options");reason.complete_loading();
    check(reason.loading_complete(),"VP Lua human reason preserves omitted-zero semantics, option equality to one and live state");
    LuaScriptHost unavailable({}, {},{},lua_gameplay_services(db,players,teams,types));
    unavailable.execute("reason","assert(not pcall(function() return Players[0]:IsHuman() end))","=missing-human-options");
    rejects([&]{unavailable.complete_loading();});
    map.cells[1].feature_type=77;snap.map_header.layout=GameStateLayout::expansion2_version1;snap.map_plots->layout=snap.map_header.layout;
    rejects([&]{restore_plot_passability(snap,db,types);});
}

void lua_map_asset_tests(const std::filesystem::path &path){
    using namespace civ5::runtime;using namespace civ5::saves;
    std::ifstream file(path,std::ios::binary);if(!file)throw std::runtime_error("Cannot read original Pontoon script");
    const std::string source{std::istreambuf_iterator<char>(file),{}};if(file.bad())throw std::runtime_error("Cannot read Pontoon source");
    RulesDatabase db;db.apply_sql("CREATE TABLE Builds(ID INTEGER,Type TEXT);INSERT INTO Builds VALUES(5000000001,'BUILD_PONTOON_BRIDGE'),(5000000002,'BUILD_PONTOON_BRIDGE2');"
        "CREATE TABLE Improvements(ID INTEGER,Type TEXT);INSERT INTO Improvements VALUES(5000000003,'IMPROVEMENT_PONTOON_BRIDGE');"
        "CREATE TABLE Routes(ID INTEGER,Type TEXT);INSERT INTO Routes VALUES(0,'ROUTE_ROAD'),(1,'ROUTE_RAILROAD');"
        "CREATE TABLE Technologies(ID INTEGER,Type TEXT);INSERT INTO Technologies VALUES(0,'TECH_RAILROAD');");
    const auto types=db.cache_info_types();ResearchContext players;players.profile=TechnologyProfile::vp542;players.players[0].slot_status=3;players.players[1].slot_status=1;
    civ5::content::CustomModOptions options;
    std::vector<TeamTechnologies> teams;SavedMapProjection map{{7,7}};PlotPassability pass;pass.layout=GameStateLayout::vp542_corpus;
    for(std::uint64_t i=0;i<49;++i){SavedMapCellProjection cell;cell.index=i;cell.x=i%7;cell.y=i/7;cell.plot_type=3;cell.terrain_type=0;map.cells.push_back(cell);pass.plots.push_back({});}
    LuaScriptHost host({}, {},{},lua_gameplay_services(db,players,teams,types,&options),{},{},lua_map_services(map,pass));
    host.execute("pontoon",source,"@ImprovementPontoonBridge.lua");
    host.execute("pontoon","assert(GameEvents.PlayerCanBuild.Count()==1 and GameEvents.PlotCanImprove.Count()==1 and GameEvents.TeamTechResearched.Count()==1 and GameEvents.CityProjectComplete.Count()==1)","=pontoon-registration");
    // Exhaust all four states on each of six neighbours. Oracle uses ring
    // positions only: adjacent ring positions are distance 1, all others 2.
    const std::int64_t ring[]={32,25,18,17,23,31};
    const auto ring_distance=[](int a,int b){const auto gap=std::abs(a-b);return gap==1||gap==5?1:2;};
    for(int pattern=0;pattern<4096;++pattern){
        int bits=pattern;std::vector<int> land,water;
        for(int d=0;d<6;++d){const auto value=bits%4;bits/=4;
            // Alternate canonical mountains and VP terrain-only mountains.
            map.cells[ring[d]].plot_type=value==0?3:value==2&&d%2==0?0:2;
            map.cells[ring[d]].terrain_type=value==2&&d%2!=0?7:0;pass.plots[ring[d]].impassable=value==3;
            if(value==0)water.push_back(d);else if(value==1)land.push_back(d);
        }
        const bool can_improve=land.size()>3||(land.size()==2&&ring_distance(land[0],land[1])>1)||
            (land.size()==3&&ring_distance(land[0],land[1])+ring_distance(land[1],land[2])+ring_distance(land[0],land[2])>4);
        const bool can_ai=land.size()!=6&&!(water.size()==2&&ring_distance(water[0],water[1])==1);
        const auto improve=host.test_all("PlotCanImprove",std::array<ScriptValue,3>{std::int64_t{3},std::int64_t{3},std::int64_t{5000000003LL}});
        check(improve.errors.empty()&&improve.value==can_improve,"Unmodified Pontoon improvement predicate matches exhaustive ring oracle");
        for(const auto build:{5000000001LL,5000000002LL,77LL})for(const auto id:{0LL,1LL}){
            const auto result=host.test_all("PlayerCanBuild",std::array<ScriptValue,5>{std::int64_t{id},std::int64_t{-1},std::int64_t{3},std::int64_t{3},std::int64_t{build}});
            check(result.errors.empty()&&result.value==(id==0||build==77||can_ai),"Unmodified Pontoon build predicate preserves human/AI and unrelated-build behavior");
        }
    }
    host.complete_loading();check(host.loading_complete(),"Original Pontoon placement events execute without source edits");
}
