#pragma once
void lua_save_data_tests(){
    using namespace civ5::runtime;
    sqlite3 *raw=nullptr;
    check(sqlite3_open(":memory:",&raw)==SQLITE_OK,"Open saved database fixture");
    std::unique_ptr<sqlite3,decltype(&sqlite3_close)> fixture(raw,sqlite3_close);
    check(sqlite3_exec(raw,"CREATE TABLE SimpleValues(Name TEXT PRIMARY KEY,Value);"
        "INSERT INTO SimpleValues VALUES('wide',5000000001);"
        "CREATE TABLE ModState(ID INTEGER PRIMARY KEY, value TEXT);"
        "INSERT INTO ModState VALUES(5000000002,'original');",nullptr,nullptr,nullptr)==SQLITE_OK,"Create saved mod schema");
    sqlite3_int64 size=0;
    std::unique_ptr<unsigned char,decltype(&sqlite3_free)> serialized(sqlite3_serialize(raw,"main",&size,0),sqlite3_free);
    check(serialized&&size>100,"Serialize saved mod fixture");
    const std::vector<std::uint8_t> original(serialized.get(),serialized.get()+size);
    auto database=RulesDatabase::from_saved_image(original);
    LuaScriptHost host({},{},{},{},lua_saved_data_services(database));
    host.execute("one",R"lua(
        a=Modding.OpenSaveData();b=Modding.OpenSaveData()
        local function first(query) local row=query();assert(query()==nil);return row end
        assert(type(a)=='userdata' and a~=b and a.Query~=b.Query)
        a.custom='local';assert(a.custom=='local' and b.custom==nil)
        assert(a.GetValue('wide')==5000000001 and a.GetValue('missing')==nil)
        assert(select('#',a.GetValue('missing'))==1)
        assert(a.SetValue('wide',5000000003) and b.GetValue('wide')==5000000003)
        assert(a.SetValue('flag',false) and b.GetValue('flag')==0)
        assert(a.SetValue('text','first\0second') and b.GetValue('text')=='first')
        assert(a.SetValue('null',nil) and b.GetValue('null')==nil)
        assert(a.SetValue('omitted') and b.GetValue('omitted')==0)
        assert(a.SetValue(5000000001,'numeric-key') and b.GetValue(5000000001)=='numeric-key')
        assert(not pcall(a.SetValue,'bad',{}))
        assert(not pcall(a.GetValue,'wide',nil,nil) and not pcall(a.SetValue,'wide',2,3))
        assert(not pcall(function() a:GetValue('wide') end))
        a.Query('BEGIN')();a.Query("UPDATE ModState SET value='uncommitted'")()
        assert(first(a.Query('SELECT value FROM ModState')).value=='uncommitted')
        assert(first(b.Query('SELECT value FROM ModState')).value=='original')
        assert(b.SetValue('locked',9)==false)
        a.Query('ROLLBACK')()
        assert(first(b.Query('SELECT value FROM ModState')).value=='original')
        local write=a.Query("UPDATE ModState SET value='committed'")
        assert(first(b.Query('SELECT value FROM ModState')).value=='original')
        write();assert(first(b.Query('SELECT value FROM ModState')).value=='committed')
        local query=Modding.OpenSaveData().Query
        collectgarbage('collect');assert(first(query('SELECT ID FROM ModState')).ID==5000000002)
        kept=query('SELECT value FROM ModState')
        a=nil;b=nil;query=nil;collectgarbage('collect')
        assert(kept().value=='committed' and kept()==nil)
    )lua","=saved-data-semantics");
    host.execute("two","assert(Modding.OpenSaveData().GetValue('wide')==5000000003)","=shared-saved-database");
    host.complete_loading();check(host.loading_complete(),"Saved database Lua API shares committed data with independent transactions and cursor lifetime");
    const auto updated=database->saved_image();
    auto restored=RulesDatabase::from_saved_image(updated);
    LuaScriptHost reload({},{},{},{},lua_saved_data_services(restored));
    reload.execute("reload",R"lua(
        local db=Modding.OpenSaveData()
        assert(db.GetValue('wide')==5000000003 and db.Query('SELECT value FROM ModState')().value=='committed')
        assert(db.Query('SELECT count(*) AS n FROM SimpleValues WHERE Name=?','null')().n==1)
    )lua","=saved-data-reload");
    reload.complete_loading();check(reload.loading_complete(),"Modified saved database survives image serialization and a fresh Lua host");
    auto unchanged=RulesDatabase::from_saved_image(original);
    LuaScriptHost source({},{},{},{},lua_saved_data_services(unchanged));
    source.execute("source","assert(Modding.OpenSaveData().GetValue('wide')==5000000001)","=original-image");
    source.complete_loading();check(source.loading_complete(),"Saved database writes never modify imported bytes");
    LuaScriptHost absent({},{},{},{},lua_saved_data_services({}));
    absent.execute("absent","assert(select('#',Modding.OpenSaveData())==0)","=absent-saved-database");
    absent.complete_loading();check(absent.loading_complete(),"Absent saved database is not silently synthesized");
    rejects([&]{RulesDatabase::from_saved_image({});});
    auto corrupt=original;corrupt[0]=0;rejects([&]{RulesDatabase::from_saved_image(corrupt);});
    corrupt=original;corrupt[100]=0;rejects([&]{RulesDatabase::from_saved_image(corrupt);});
    auto connection=database->reopen_saved();
    auto cursor=connection->script_query("BEGIN",{});cursor->next();
    rejects([&]{connection->saved_image();});
    rejects([&]{database->saved_image();});
    cursor=connection->script_query("ROLLBACK",{});cursor->next();
    // Native SetValue returns false on schema/constraint failure, GetValue nil.
    LuaScriptHost missing_schema({},{},{},{},lua_saved_data_services(database));
    missing_schema.execute("missing",R"lua(
        local db=Modding.OpenSaveData();db.Query('DROP TABLE SimpleValues')()
        assert(db.SetValue('x',1)==false and db.GetValue('x')==nil)
    )lua","=missing-simple-values");
    missing_schema.complete_loading();check(missing_schema.loading_complete(),"SimpleValues failures preserve native false/nil results");
}
