#pragma once
void lua_gameinfo_tests(){
    using namespace civ5::runtime;
    RulesDatabase db;
    db.apply_sql("CREATE TABLE Defines(Name,Value);"
        "CREATE TABLE Units(ID INTEGER,Type TEXT,Flag boolean,Cost INTEGER);"
        "INSERT INTO Units VALUES(5000000001,'FIRST',0,10),(5000000002,'SECOND',1,20);"
        "CREATE TABLE Alias(ID INTEGER PRIMARY KEY,Type TEXT); INSERT INTO Alias VALUES(2,'TWO'),(5000000001,'WIDE');"
        "CREATE TABLE Plain(Value); INSERT INTO Plain VALUES(7),(8);"
        "CREATE TABLE OnlyType(Type TEXT,Value); INSERT INTO OnlyType VALUES('A',1),('A',2);"
        "CREATE TABLE Converted(ID,Type,Value); INSERT INTO Converted VALUES('42tail',17,'converted'),(NULL,NULL,'null');"
        "CREATE VIEW UnitView AS SELECT * FROM Units;"
        "CREATE TABLE \"123\"(V); CREATE TABLE \"bad name\"(V);");
    const auto types=db.cache_info_types();
    ResearchContext players;std::vector<TeamTechnologies> teams;
    const auto services=lua_gameplay_services(db,players,teams,types);
    LuaScriptHost host({}, {},{},services);
    host.execute("A",R"lua(
        assert(type(GameInfo)=='table' and type(GameInfoTypes)=='table' and getmetatable(GameInfoTypes)==nil)
        assert(GameInfoTypes.FIRST==5000000001 and GameInfoTypes.SECOND==5000000002 and GameInfoTypes.absent==nil)
        assert(GameInfo.UnitView==nil and GameInfo.sqlite_master==nil and GameInfo.Missing==nil and GameInfo[{}]==nil)
        assert(rawget(GameInfo,'Units')==nil)
        local units=GameInfo.Units
        assert(type(units)=='userdata' and GameInfo.Units==units)
        assert(getmetatable(units).__tablename=='Units' and type(getmetatable(units).__db)=='userdata')
        assert(getmetatable(units).__db==getmetatable(GameInfo).__db)
        assert(GameInfo[123]==GameInfo['123'] and rawget(GameInfo,123)==nil)
        assert(not pcall(function() units[1]={} end))
        assert(not pcall(function() return GameInfo['bad name'][1] end))
        assert(not pcall(function() return units[{}] end))
        assert(units[5000000001]==nil) -- Numeric lookup is ROWID, not a non-alias ID.
        local a=units[1]
        assert(a.ID==5000000001 and a.Type=='FIRST' and a.Flag==false)
        assert(units[5000000001]==a and units.FIRST==a)
        assert(units[1]~=a) -- Requested ROWID is not itself a cache key.
        assert(GameInfo.Alias[2.5].Type=='TWO' and GameInfo.Alias[5000000001].Type=='WIDE')
        assert(GameInfo.Alias['2']==nil and GameInfo.Alias['TWO\0tail'].ID==2)
        assert(#units==2 and #GameInfo.Plain==2)
        assert(GameInfo.Plain[1].Value==7)
        assert(not pcall(function() return GameInfo.Plain.STRING end))
        local iter=GameInfo.Plain()
        assert(iter().Value==7 and iter().Value==8 and iter()==nil and iter()==nil)
        local x,y=GameInfo.Plain(),GameInfo.Plain()
        assert(x().Value==7 and y().Value==7 and x().Value==8)
        assert(not pcall(function() units(nil) end))
        assert(not pcall(function() units(false) end))
        local a=units.FIRST
        a.Cost=999
        assert(units('Cost = 10')()==a) -- WHERE uses SQL; returned row uses shared cache.
        local b=units{Flag=true}() -- Native boolean lua_tointeger converts true to zero.
        assert(b==a and units{Flag=false}()==a and units{Cost=19.7}().Type=='SECOND')
        assert(units{Type='SECOND\0suffix'}().ID==5000000002)
        -- Array entry comes before hash fields in Lua traversal. It consumes
        -- bind position one despite generating no SQL condition.
        assert(units{false,Type='FIRST'}()==nil)
        local duplicate=GameInfo.OnlyType()
        assert(duplicate()==duplicate())
        local converted=GameInfo.Converted[1]
        assert(converted.ID=='42tail' and converted.Type==17)
        assert(GameInfo.Converted[42]==converted and GameInfo.Converted['17']==converted)
        assert(GameInfo.Converted{Value='converted'}()==converted)
        getmetatable(units).__rowdata.FIRST=false
        assert(units.FIRST==false)
        assert(units{Type='FIRST'}()==a) -- ID cache has priority over Type.
        getmetatable(units).__rowdata[5000000001]=nil
        assert(units{Type='FIRST'}()==false)
        DB.Query("UPDATE Units SET Cost=70 WHERE Type='SECOND'")()
        assert(units.SECOND.Cost==20)
        getmetatable(units).__rowdata[5000000002]=nil
        getmetatable(units).__rowdata.SECOND=nil
        assert(units.SECOND.Cost==70)
        DB.Query("INSERT INTO Units VALUES(5000000003,'THIRD',0,30)")()
        assert(#units==3 and units.THIRD.ID==5000000003)
        assert(GameInfoTypes.THIRD==nil) -- Immutable source catalog, writable Lua snapshot.
        GameInfoTypes.FIRST=9
        DB.Query('CREATE TABLE Missing(Value)')()
        assert(GameInfo.Missing~=nil and #GameInfo.Missing==0)
        DB.Query('CREATE TEMP TABLE TempOnly(V)')()
        assert(GameInfo.TempOnly==nil)
        GameEvents.CommonDatabase.Add(function(db) return db==getmetatable(GameInfo).__db end)
    )lua","=gameinfo.lua");
    host.execute("B",R"lua(
        assert(GameInfoTypes.FIRST==5000000001 and GameInfoTypes.THIRD==nil)
        assert(GameInfo.Units.FIRST.Cost==10 and GameInfo.Units.SECOND.Cost==70)
        assert(#GameInfo.Units==3)
        assert(GameEvents.CommonDatabase.TestAll(getmetatable(GameInfo).__db))
    )lua","=independent-gameinfo.lua");
    host.complete_loading();check(host.loading_complete(),"GameInfo must preserve source lookup, cache, filter, iterator and snapshot semantics with wide IDs");
    check(types.find("FIRST")->id==5000000001LL,"Lua GameInfoTypes edits must not alter the authoritative GC catalog");
    LuaScriptHost invalid({}, {},{},services);
    invalid.execute("bad","local q=GameInfo.Units({}); assert(type(q)=='function'); assert(not pcall(q))","=invalid-gameinfo.lua");
    rejects([&]{invalid.complete_loading();});
    LuaScriptHost null_type({}, {},{},services);
    null_type.execute("null",R"lua(
        assert(not pcall(function() return GameInfo.Converted[2] end))
        assert(GameInfo.Converted[0].Value=='null') -- ID cache write precedes the NULL Type error.
        assert(GameInfo.Converted[1]==nil) -- Busy Bind is ignored; the previous query reaches EOF.
        assert(GameInfo.Converted[1].Value=='converted') -- Now Reset allows a fresh binding.
    )lua","=null-type.lua");
    null_type.complete_loading();check(null_type.loading_complete(),"Native NULL Type cache error must preserve the earlier ID cache write");
    const auto long_name="T"+std::string(600,'x');
    db.apply_sql("CREATE TABLE "+long_name+"(ID INTEGER PRIMARY KEY,Type); INSERT INTO "+long_name+" VALUES(5000000001,'LONG')");
    host.execute("wide","local t=GameInfo['"+long_name+"']; assert(#t==1 and t[5000000001].Type=='LONG' and t{Type='LONG'}().ID==5000000001)","=long-name.lua");
    host.complete_loading();check(host.loading_complete(),"GameInfo SQL assembly must not inherit fixed 256/512-byte native buffers");
    auto partial=services;partial.info_types={};rejects([&]{LuaScriptHost h({}, {},{},partial);});
}
