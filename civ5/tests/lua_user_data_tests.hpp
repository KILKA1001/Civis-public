#pragma once
void lua_user_data_tests(){
    using namespace civ5::runtime;
    sqlite3 *raw=nullptr;
    check(sqlite3_open(":memory:",&raw)==SQLITE_OK,"Open user-data fixture");
    std::unique_ptr<sqlite3,decltype(&sqlite3_close)> fixture(raw,sqlite3_close);
    check(sqlite3_exec(raw,"CREATE TABLE SimpleValues(Name TEXT PRIMARY KEY,Value);"
        "INSERT INTO SimpleValues VALUES('seed',5000000001);",nullptr,nullptr,nullptr)==SQLITE_OK,
        "Create user-data fixture");
    sqlite3_int64 size=0;
    std::unique_ptr<unsigned char,decltype(&sqlite3_free)> serialized(sqlite3_serialize(raw,"main",&size,0),sqlite3_free);
    check(serialized&&size>100,"Serialize user-data fixture");
    const std::vector<std::uint8_t> original(serialized.get(),serialized.get()+size);
    const auto root=std::filesystem::temp_directory_path()/
        ("civis-user-data-test-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    check(std::filesystem::create_directory(root),"Create isolated user-data fixture directory");
    const auto source=root/"BE-1.db";
    {std::ofstream output(source,std::ios::binary);
     output.write(reinterpret_cast<const char*>(original.data()),static_cast<std::streamsize>(original.size()));
     check(output.good(),"Write user-data fixture");}
    auto provider=lua_user_data_services(root);
    {
        LuaScriptHost host({},{},{},{},{},{},{},{},{},provider);
        host.execute("first",R"lua(
            local a=Modding.OpenUserData('BE',1)
            local b=Modding.OpenUserData('BE',1)
            assert(type(a)=='userdata' and a.GetValue('seed')==5000000001)
            assert(a.SetValue('seed',5000000002) and b.GetValue('seed')==5000000002)
            assert(Modding.OpenUserData('BE',2).GetValue('seed')==nil)
            assert(Modding.OpenUserData('Other',1).GetValue('seed')==nil)
            Modding.DeleteUserData('BE',1)
            assert(Modding.OpenUserData('BE',1).GetValue('seed')==nil)
            assert(a.GetValue('seed')==5000000002)
        )lua","=named-user-data");
        host.execute("second","assert(Modding.OpenUserData('BE',2).SetValue('wide',5000000003))",
            "=another-context");
        host.execute("first","assert(Modding.OpenUserData('BE',2).GetValue('wide')==5000000003)",
            "=shared-user-data");
        host.complete_loading();check(host.loading_complete(),"Named user-data databases share state by name and version");
    }
    std::ifstream input(source,std::ios::binary);
    const std::vector<std::uint8_t> after(std::istreambuf_iterator<char>{input},{});
    input.close();
    check(after==original,"User-data mutations and deletion leave original source bytes unchanged");
    rejects([&]{provider.open("../escape",1);});
    rejects([&]{provider.open("BE",-1);});
    std::filesystem::remove(source);std::filesystem::remove(root);
    LuaScriptHost blank({},{},{},{},{},{},{},{},{},lua_user_data_services());
    blank.execute("vanilla",R"lua(
        assert(Modding.OpenSaveData==nil)
        assert(Modding.OpenUserData('random mod',5000000000).SetValue('x',42))
        assert(Modding.OpenUserData('random mod',5000000000).GetValue('x')==42)
    )lua","=vanilla-user-data");
    blank.complete_loading();check(blank.loading_complete(),"Arbitrary mod names and wide versions need no VP profile");
}
