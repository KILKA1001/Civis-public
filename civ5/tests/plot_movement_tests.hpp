#pragma once
void plot_movement_tests(){
    using namespace civ5::runtime;using namespace civ5::saves;
    constexpr std::int64_t wide=4294967300LL;
    RulesDatabase db;db.apply_sql("CREATE TABLE Defines(Name,Value);INSERT INTO Defines VALUES('DEEP_WATER_TERRAIN',6),('SHALLOW_WATER_TERRAIN',5);"
        "CREATE TABLE Features(ID,Type);INSERT INTO Features VALUES(0,'ICE');");
    const auto types=db.cache_info_types();const auto defs=db.cache_global_defines(GlobalDefineProfile::vp542);
    SavedMapProjection map{{1,1}};map.cells.emplace_back();auto &p=map.cells[0];p.owner=wide;p.plot_type=2;p.terrain_type=0;p.feature_type=-1;p.route=SavedRoutePlot{};
    PlotPassability pass;pass.layout=GameStateLayout::vp542_corpus;pass.plots.emplace_back();auto &flags=pass.plots[0];auto &team_flags=ensure_vp_plot_team_flags(pass,0);team_flags[wide]=false;
    ResearchContext players;players.profile=TechnologyProfile::vp542;players.players[wide].team=wide;players.players[3].team=3;
    PlayerResearchContext diplomacy;diplomacy.profile=players.profile;diplomacy.players[wide]={};diplomacy.players[3]={};
    diplomacy.teams[wide].members={wide};diplomacy.teams[3].members={3};
    PlotMovementState state;state.players[wide]={};state.players[3]={};state.teams[wide]={};state.teams[3]={};
    PlotMovementContext c{map,pass,players,diplomacy,state,types,defs};
    // Independent boolean model, exercising every geography/capability/route
    // combination and separate terrain/global cache values.
    for(int terrain=0;terrain<6;++terrain)for(int impassable=0;impassable<2;++impassable)
    for(int road=0;road<3;++road)for(int own=0;own<3;++own)for(int city=0;city<2;++city)for(int capability=0;capability<32;++capability){
        const bool ice=terrain==1,mountain=terrain==2||terrain==3,deep=terrain==4,shallow=terrain==5;
        p.plot_type=terrain==2?0:deep||shallow?3:2;p.terrain_type=terrain==3?7:deep?6:shallow?5:0;p.feature_type=ice?0:-1;
        p.owner=own==0?-1:own==1?wide:3;p.is_city=city!=0;p.route->type=road?wide:-1;p.route->pillaged=road==2;
        flags.impassable=!impassable;team_flags[wide]=impassable!=0;
        auto &a=state.players[wide];a.crosses_ice=capability&1;a.mountain_pass=capability&2;a.mountains_after_general=capability&4;a.generals_created=wide;
        a.embarked_all_water=capability&8;state.teams[wide].embark_count=capability&16;state.teams[wide].all_water_count=0;
        const bool trait=(ice&&(capability&1))||(mountain&&(capability&6))||(deep&&(capability&8))||(shallow&&(capability&24));
        const bool can_cross=!impassable||trait||(road==1&&(!city||own==1));
        check(plot_is_valid_move(c,0,wide,false)==can_cross,"VP movement geography/capability truth table with wide player, team and route IDs");
        check(plot_is_valid_move(c,0,wide)==(can_cross&&own!=2),"Movement territory follows terrain eligibility");
        const bool anonymous=(impassable&&!ice)||road==1;
        check(plot_is_valid_move(c,0,-1)==anonymous&&plot_is_valid_move(c,0,-1,false)==anonymous,"NO_PLAYER uses global flags and route override even inside a foreign city");
    }
    p.plot_type=2;p.terrain_type=0;p.feature_type=-1;p.owner=3;p.is_city=false;p.route->type=-1;team_flags[wide]=false;
    for(int mask=0;mask<128;++mask){auto &owner=state.teams[3];auto &a=diplomacy.teams[3];auto &b=diplomacy.teams[wide];
        set_vp_plot_movement_minor(state,3,(mask&1)!=0);owner.open_borders=mask&2?std::set<std::int64_t>{wide}:std::set<std::int64_t>{};
        state.teams[wide].open_borders=mask&4?std::set<std::int64_t>{3}:std::set<std::int64_t>{};
        a.liberated_by=mask&8?wide:-1;a.master=mask&16?wide:-1;
        b.at_war=mask&32?std::set<std::int64_t>{3}:std::set<std::int64_t>{};a.at_war=mask&64?std::set<std::int64_t>{wide}:std::set<std::int64_t>{};
        const bool border=(mask&2)||((mask&8)&&!(mask&64))||(mask&16);
        check(movement_open_borders(c,3,wide)==border&&movement_open_borders(c,wide,wide)&&!movement_open_borders(c,3,-1),"Open borders are directed; liberation checks owner's war; a master bypasses war");
        check(plot_is_valid_move(c,0,wide)==bool((mask&1)||border||(mask&32))&&plot_is_valid_move(c,0,wide,false),"Minor cache, open borders and visitor war determine territory eligibility");
    }
    state.teams[3]={};set_vp_plot_movement_minor(state,3,false);diplomacy.teams[3]={};diplomacy.teams[3].members={3};diplomacy.teams[wide].at_war.clear();
    diplomacy.players[3].pregame_minor=true;players.players[3].alive=false;
    check(!plot_is_valid_move(c,0,wide),"Pregame minor flag does not implicitly refresh source cache");
    refresh_plot_movement_team_status(c,3);check(plot_is_valid_move(c,0,wide)&&
        state.extensions.find<VpPlotMovementProfileState>()!=nullptr,"Team-status refresh stores the VP cache behind its profile boundary and includes dead minor roster members");
    diplomacy.players[3].pregame_minor=false;check(plot_is_valid_move(c,0,wide),"Cached minor status survives until explicit refresh");
    refresh_plot_movement_team_status(c,3);check(!plot_is_valid_move(c,0,wide),"Explicit refresh removes the minor border bypass");
    auto &a=state.players[wide];a={};a.mountains_after_general=true;
    a.generals_created=0;check(!player_can_cross_mountain(c,wide),"Mountain trait requires a previously created general");
    a.generals_created=wide;check(player_can_cross_mountain(c,wide),"Free generals count and counter remains int64");
    a.mountains_after_general=false;a.mountain_pass=true;a.generals_created=-1;check(player_can_cross_mountain(c,wide),"Unconditional mountain pass ignores general count");
    state.teams[wide].embark_count=-1;state.teams[wide].all_water_count=wide;
    check(!player_can_embark(c,wide)&&player_can_cross_ocean(c,wide),"Team embark and ocean counters are distinct positive predicates");
    a.embarked_all_water=true;check(player_can_embark(c,wide)&&player_can_cross_ocean(c,wide),"Embarked-all-water trait supplies both capabilities");
    a={};p.plot_type=0;p.terrain_type=0;p.is_city=true;p.owner=3;p.route->type=wide;p.route->pillaged=false;players.players[3].team=wide;
    team_flags[wide]=true;
    check(!plot_is_valid_move(c,0,wide),"Road in another player's city does not override impassability even on the same team");
    players.players[3].team=3;p.is_city=false;p.route->type=-1;p.owner=-1;p.plot_type=3;
    db.apply_sql("UPDATE Defines SET Value=4294967300 WHERE Name='DEEP_WATER_TERRAIN';UPDATE Defines SET Value=4294967301 WHERE Name='SHALLOW_WATER_TERRAIN';");
    const auto wide_defs=db.cache_global_defines(GlobalDefineProfile::vp542);PlotMovementContext wide_water{map,pass,players,diplomacy,state,types,wide_defs};
    p.terrain_type=wide;check(plot_is_valid_move(wide_water,0,wide),"Ocean identity uses the active wide terrain define");
    p.terrain_type=wide+1;check(!plot_is_valid_move(wide_water,0,wide),"Wide shallow-water identity still requires embarkation rather than ocean passage");
    team_flags[wide]=true;LuaScriptHost host({},{},{},{},{},{},lua_map_services(map,pass));
    host.execute("wide","assert(Map.GetPlotByIndex(0):IsImpassable(4294967300))","=sparse-team-passability");
    team_flags[wide]=false;host.execute("wide","assert(not Map.GetPlotByIndex(0):IsImpassable(4294967300))","=live-team-passability");
    check(host.memory_bytes()>0,"Lua and movement share sparse wide team impassability");
    team_flags.erase(wide);rejects([&]{plot_is_valid_move(c,0,wide);});
    // Exact restoration of the inputs, including raw border booleans and a
    // cold minor cache. Source wire spans do not become runtime capacities.
    SavedSessionSnapshot snap;snap.teams.emplace();snap.all_players.emplace();snap.teams->layout=snap.all_players->layout=GameStateLayout::vp542_corpus;
    ResearchContext restored_players;restored_players.profile=TechnologyProfile::vp542;restored_players.players[wide].team=0;
    PlayerResearchContext restored_diplomacy;restored_diplomacy.profile=restored_players.profile;restored_diplomacy.players[wide]={};restored_diplomacy.teams[0]={};
    SavedPlayerObservation saved;saved.saved_player_id=wide;saved.core.layout=saved.traits.layout=GameStateLayout::vp542_corpus;
    for(const char *name:{"m_bEmbarkedAllWater","m_bMountainPass","m_bCrossesMountainsAfterGreatGeneral","m_bCrossesIce"}){
        PlayerTraitsField f{name,PlayerTraitsEncoding::boolean};f.values={1};saved.traits.fields.push_back(f);}
    PlayerCoreField general{"m_iGreatGeneralsCreated",PlayerCoreEncoding::i32};general.values={wide};saved.core.fields.push_back(general);snap.all_players->players.push_back(saved);
    TeamObservation team;team.fields={{"id",0},{"can_embark",wide},{"embarked_all_water",-1}};team.arrays={{"open_borders",1,{255,73}}};snap.teams->teams.push_back(team);
    const auto imported=restore_plot_movement(snap,restored_players,restored_diplomacy);
    check(imported.players.at(wide).crosses_ice&&imported.players.at(wide).mountain_pass&&imported.players.at(wide).generals_created==wide&&
        imported.teams.at(0).embark_count==wide&&imported.teams.at(0).all_water_count==-1&&imported.teams.at(0).open_borders==std::set<std::int64_t>{0}&&
        imported.extensions.find<VpPlotMovementProfileState>()==nullptr&&!vp_plot_movement_minor(imported,0),
        "Movement import retains wide counters, limits raw border-tail interpretation to the live roster and starts with cold minor status");
    snap.all_players->players[0].traits.fields[0].values={2};rejects([&]{restore_plot_movement(snap,restored_players,restored_diplomacy);});
    snap.all_players->players[0].traits.fields[0].values={1};snap.teams->teams.push_back(team);rejects([&]{restore_plot_movement(snap,restored_players,restored_diplomacy);});
}
