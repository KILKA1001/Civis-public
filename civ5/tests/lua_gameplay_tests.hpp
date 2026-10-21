#pragma once
void lua_gameplay_tests(){
    using namespace civ5::runtime;
    RulesDatabase db;
    db.apply_sql("CREATE TABLE Defines(Name TEXT PRIMARY KEY,Value); INSERT INTO Defines VALUES('N',5000000001),('F',1.5),('S','text'),('EMPTY',NULL);"
                 "CREATE TABLE Rows(ID INTEGER, Flag boolean, Other BOOL, Value TEXT); INSERT INTO Rows VALUES(5000000001,1,1,'first'),(5000000002,0,0,'second');");
    ResearchContext players;players.players[5000000001LL].team=5000000002LL;
    players.players[5000000001LL].alive=true;players.players[5000000001LL].ever_alive=true;
    players.game_turn=5000000004LL;players.speed=5000000008LL;
    std::vector<TeamTechnologies> teams;teams.emplace_back(TechnologyProfile::vp542,5000000002LL,5000000010ULL);
    teams.front().set_has_tech(5000000003LL,true);
    const auto types=db.cache_info_types();
    PregameRoster roster;roster.slots[5000000001LL]={3,5000000012LL,5000000006LL,true,5000000011LL};
    auto services=lua_gameplay_services(db,players,teams,types,nullptr,&roster);
    LuaScriptHost host({}, {},{},services);
    host.execute("A",R"lua(
        assert(Game.GetGameTurn()==5000000004)
        assert(Game.GetGameSpeedType()==5000000008)
        local clock=os.clock();assert(type(clock)=='number' and clock>=0 and os.clock()>=clock)
        local iter=DB.Query('SELECT * FROM Rows ORDER BY ID')
        assert(type(iter)=='function')
        local a,b=iter(),iter()
        assert(a.ID==5000000001 and a.Flag==true and a.Other==1 and b.ID==5000000002 and b.Flag==false)
        assert(select('#',iter())==1 and iter().Value=='first')
        a.Value='local'; assert(DB.Query('SELECT * FROM Rows')().Value=='first')
        local duplicate=DB.Query('SELECT 1 AS ID, 5000000002 AS ID, NULL AS Missing, X\'abc0\' AS Blob')()
        assert(duplicate.ID==5000000002 and duplicate.Missing==nil and duplicate.Blob==nil)
        assert(not pcall(function() duplicate.Missing=1 end))
        local row=DB.Query('SELECT ? AS N, typeof(?) AS Kind, ? AS Flag, ? AS Missing, ? AS Text',5000000001,5,true,nil,'a\0b')()
        assert(row.N==5000000001 and row.Kind=='real' and row.Flag==1 and row.Missing==nil and row.Text=='a')
        assert(DB.Query('SELECT ? AS V')().V==nil)
        assert(DB.Query('SELECT ? AS V',7,8)().V==7)
        local bound=DB.Query('SELECT ? AS V',7)
        assert(bound().V==7 and bound()==nil and bound().V==nil) -- Reset clears parameters.
        assert(DB.Query('SELECT 1 AS V; DROP TABLE Rows')().V==1)
        assert(DB.Query('SELECT char(97,0,98) AS V')().V=='a')
        local write=DB.Query("UPDATE Rows SET Value='changed' WHERE ID=?",5000000001)
        assert(DB.Query('SELECT Value FROM Rows WHERE ID=5000000001')().Value=='first') -- lazy
        assert(write()==nil and DB.Query('SELECT Value FROM Rows WHERE ID=5000000001')().Value=='changed')
        assert(DB.Query('BEGIN')()==nil)
        DB.Query("UPDATE Rows SET Value='rolled back'")()
        assert(DB.Query('ROLLBACK')()==nil and DB.Query('SELECT Value FROM Rows WHERE ID=5000000001')().Value=='changed')
        assert(GameDefines.N==5000000001 and GameDefines.F==1.5 and GameDefines.S=='text')
        assert(GameDefines.EMPTY==nil and GameDefines.LATER==nil)
        DB.Query("UPDATE Defines SET Value=7 WHERE Name='N'")()
        DB.Query("INSERT INTO Defines VALUES('LATER',9)")()
        assert(GameDefines.N==5000000001 and GameDefines.LATER==9) -- positive cache, no negative cache
        GameDefines.N=nil; assert(GameDefines.N==7)
        assert(Teams[5000000002]:IsHasTech(5000000003))
        assert(Teams[5000000002]:IsHasTech(-1))
        assert(not Teams[5000000002]:IsHasTech('not a number')) -- native lua_tointeger default 0
        assert(Players[5000000001]:GetTeam()==5000000002)
        assert(Players[5000000001]:IsAlive())
        assert(Players[5000000001]:IsEverAlive())
        assert(Players[5000000001]:IsMinorCiv())
        assert(Players[5000000001]:GetMinorCivType()==5000000011)
        assert(Players[5000000001]:GetCivilizationType()==5000000012)
        assert(Players[5000000001]:GetLeaderType()==5000000006)
        assert(type(Teams[5000000002].__instance)=='userdata' and Teams[0]==nil)
        assert(not pcall(Teams[5000000002].IsHasTech,Players[5000000001],0))
        assert(not pcall(Players[5000000001].GetTeam,{}))
        print('world',Players[5000000001]:GetTeam(),false)
    )lua","=gameplay.lua");
    host.complete_loading();check(host.loading_complete(),"Gameplay Lua must query and mutate the same database, preserve typed rows and read wide object IDs");
    check(host.messages().size()==1&&host.messages()[0].context=="A"&&host.messages()[0].text=="world\t5000000002\tfalse","Original Lua print must reach structured context output");
    teams.front().set_has_tech(5000000003LL,false);players.players.at(5000000001LL).team=5000000007LL;
    players.players.at(5000000001LL).alive=false;players.game_turn=5000000005LL;players.speed=5000000009LL;
    roster.slots.at(5000000001LL).leader=5000000007LL;
    roster.slots.at(5000000001LL).minor=false;
    roster.slots.at(5000000001LL).minor_type=-1;
    roster.slots.at(5000000001LL).civilization=5000000013LL;
    host.execute("A","assert(Game.GetGameTurn()==5000000005 and Game.GetGameSpeedType()==5000000009)","=live-game-state");
    host.execute("A","assert(not Teams[5000000002]:IsHasTech(5000000003)); assert(Players[5000000001]:GetTeam()==5000000007 and not Players[5000000001]:IsAlive() and Players[5000000001]:IsEverAlive() and Players[5000000001]:GetLeaderType()==5000000007)","=live-state.lua");
    players.players.at(5000000001LL).ever_alive=false;
    host.execute("A","assert(not Players[5000000001]:IsEverAlive() and not Players[5000000001]:IsMinorCiv() and Players[5000000001]:GetMinorCivType()==-1 and Players[5000000001]:GetCivilizationType()==5000000013)","=live-player-flags.lua");
    host.execute("B","assert(GameDefines.N==7); GameDefines.N=80","=other-context.lua");
    host.execute("A","assert(GameDefines.N==7)","=cache-scope.lua");
    host.complete_loading();check(host.loading_complete(),"Team/player getters must see shared mutations; define caches stay context-local");
    auto no_speed=services;no_speed.game_speed={};
    LuaScriptHost without_speed({},{},{},no_speed);
    without_speed.execute("base","assert(Game.GetGameTurn()==5000000005 and Game.GetGameSpeedType==nil)",
        "=no-speed-capability");
    without_speed.complete_loading();check(without_speed.loading_complete(),
        "Game speed is exposed only when the shared session provides it");
    auto cursor=db.script_query("SELECT ID FROM Rows ORDER BY ID",{});
    check(std::get<std::int64_t>(cursor->next()->at(0).second)==5000000001LL,"Script cursor must return wide SQL integers");
    check(cursor->next().has_value()&&!cursor->next()&&cursor->next().has_value(),"Script cursor resets at exhaustion");
    std::unique_ptr<RulesScriptCursor> retained;
    {RulesDatabase temporary;temporary.apply_sql("CREATE TABLE Own(V); INSERT INTO Own VALUES(5000000001)");retained=temporary.script_query("SELECT V FROM Own",{});}
    check(std::get<std::int64_t>(retained->next()->at(0).second)==5000000001LL,"Prepared Lua queries must retain their owned in-memory connection");
    RulesDatabase reset_db;reset_db.apply_sql("CREATE TABLE T(V INTEGER UNIQUE); INSERT INTO T VALUES(7)");
    const std::array<DatabaseScriptValue,1> reset_args{std::int64_t{7}};
    auto reset_cursor=reset_db.script_query("INSERT INTO T VALUES(?)",reset_args);
    check(!reset_cursor->next()&&!reset_cursor->error().empty(),"Constraint failure must remain visible after cursor reset");
    reset_db.apply_sql("DELETE FROM T");
    check(!reset_cursor->next()&&reset_cursor->error().empty(),"Failed Results::Reset must retain bindings for a later retry");
    auto reset_result=reset_db.script_query("SELECT V FROM T",{});
    check(std::get<std::int64_t>(reset_result->next()->at(0).second)==7,"Retried query must use the parameter retained after a step error");
    LuaScriptHost invalid({}, {},{},services);
    invalid.execute("bad","local q=DB.Query('SELECT * FROM absent'); assert(type(q)=='function'); assert(not pcall(q))","=invalid-query.lua");
    rejects([&]{invalid.complete_loading();});
    LuaScriptHost syntax({}, {},{},services);
    syntax.execute("unused","assert(type(DB.Query('bad sql'))=='function')","=unused-query.lua");
    syntax.complete_loading();check(syntax.loading_complete(),"SQL preparation failure creates an invalid iterator; it must not throw until invoked");
    LuaScriptHost write_error({}, {},{},services);
    write_error.execute("bad","local q=DB.Query(\"INSERT INTO Defines VALUES('N',2)\"); assert(q()==nil)","=constraint.lua");
    check(write_error.source_errors().size()==1,"Native Step errors return nil but must remain visible to content readiness");
    rejects([&]{write_error.complete_loading();});
    LuaScriptHost argument({}, {},{},services);
    argument.execute("bad","assert(not pcall(DB.Query,'SELECT 1',{}))","=argument.lua");
    rejects([&]{argument.complete_loading();});
    check(!db.script_query("ATTACH 'unwanted.db' AS unwanted",{}),"DB.Query cannot open ambient files");
    check(!db.script_query("SELECT load_extension('unwanted')",{}),"DB.Query cannot load native extensions");
    auto partial=services;partial.player_team={};rejects([&]{LuaScriptHost h({}, {},{},partial);});
    auto bad_ids=services;bad_ids.ids=[](bool){return std::vector<std::int64_t>{3,3};};
    LuaScriptHost bad_roster({}, {},{},bad_ids);rejects([&]{bad_roster.execute("bad","assert(false)","=roster.lua");});
}
// Optional read-only original asset supplied by the integration verifier.
// Controlled rules exercise the nonempty branch even when the real saved
// profile leaves every BombardRange at its authored default zero.
void lua_gameplay_asset_tests(const std::filesystem::path &source_path){
    using namespace civ5::runtime;
    std::ifstream file(source_path,std::ios::binary);
    if(!file)throw std::runtime_error("Cannot read supplied original Lua asset");
    const std::string source{std::istreambuf_iterator<char>(file),{}};
    if(file.bad())throw std::runtime_error("Original Lua asset read failed");
    RulesDatabase db;
    db.apply_sql("CREATE TABLE Defines(Name TEXT PRIMARY KEY, Value);"
                 "INSERT INTO Defines VALUES('MAX_CITY_ATTACK_RANGE',5),('CAN_CITY_USE_INDIRECT_FIRE',1),('MAX_CIV_TEAMS',1);"
                 "CREATE TABLE Technologies(ID INTEGER,BombardRange INTEGER,BombardIndirect INTEGER);"
                 "INSERT INTO Technologies VALUES(5000000001,2,0),(5000000002,4,1);");
    ResearchContext players;players.players[0].team=0;players.players[5000000001LL].team=1;
    std::vector<TeamTechnologies> teams;
    teams.emplace_back(TechnologyProfile::vp542,0,5000000010ULL);teams.emplace_back(TechnologyProfile::vp542,1,5000000010ULL);
    teams[0].set_has_tech(5000000001LL,true);
    const auto types=db.cache_info_types();
    LuaScriptHost host({}, {},{},lua_gameplay_services(db,players,teams,types));
    host.execute("asset",source,"@original-city-bombard.lua");
    host.execute("asset","assert(OnGetBombardRange(0,0)==2 and OnGetBombardRange(5000000001,0)==-5); assert(GameEvents.GetBombardRange.Count()==1)","=initial-ranges.lua");
    host.complete_loading();check(host.loading_complete(),"Unmodified city bombard asset must initialize from controlled SQL and wide technology/player IDs");
    teams[1].set_has_tech(5000000002LL,true);
    const std::array<ScriptValue,3> notification{std::int64_t{1},std::int64_t{5000000002LL},std::int64_t{1}};
    const auto result=host.call_hook("TeamTechResearched",notification);
    check(result.errors.empty(),"Original research listener must execute without a provider or Lua error");
    host.execute("asset","assert(OnGetBombardRange(5000000001,0)==-4 and Teams[1]:IsHasTech(5000000002))","=changed-range.lua");
    const std::array<ScriptValue,2> bombard{std::int64_t{5000000001LL},std::int64_t{5000000003LL}};
    const auto range=host.accumulate_int("GetBombardRange",bombard);
    check(range.value==-4&&range.errors.empty(),"Native Value accumulator must return the original bombard listener result");
    const std::array<ScriptValue,3> ignored{std::int64_t{1},std::int64_t{5000000001LL},std::int64_t{2}};
    check(host.call_hook("TeamTechResearched",ignored).errors.empty(),"Repeat change of two must reach the original listener");
    host.execute("asset","assert(OnGetBombardRange(5000000001,0)==-4)","=ignored-change.lua");
    host.complete_loading();check(host.loading_complete()&&host.messages().size()==1,"Original asset must preserve its event condition and output");
}
