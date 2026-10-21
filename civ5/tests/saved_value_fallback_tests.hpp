void saved_value_fallback_tests(){
    using namespace civ5::runtime;
    const auto plan=parse_saved_value_fallback_plan(read_xml_metadata(
        "<SavedValueFallback modId='09f11249-de39-4884-975d-9d8f01c241d7' modVersion='5' userData='BE' userDataVersion='1'>"
        "<Key name='existing'/><Key name='wide'/><Key name='absent'/><Key name='null_value'/></SavedValueFallback>"));
    check(plan.keys.size()==4&&plan.mod_version==5,"fallback plan parses its explicit keys");
    rejects([]{parse_saved_value_fallback_plan(read_xml_metadata(
        "<SavedValueFallback modId='09f11249-de39-4884-975d-9d8f01c241d7' modVersion='5' userData='BE' userDataVersion='1'>"
        "<Key name='same'/><Key name='same'/></SavedValueFallback>"));});
    auto saved=RulesDatabase::new_saved_values();
    auto source=RulesDatabase::new_saved_values();
    saved->apply_sql("INSERT INTO SimpleValues VALUES('existing','saved');"
                     "INSERT INTO SimpleValues VALUES('null_value',NULL)");
    source->apply_sql("INSERT INTO SimpleValues VALUES('existing','saved');"
                      "INSERT INTO SimpleValues VALUES('wide',5000000001);"
                      "INSERT INTO SimpleValues VALUES('null_value','source');"
                      "INSERT INTO SimpleValues VALUES('unlisted','private')");
    const auto result=fill_missing_saved_values(*saved,*source,plan);
    check(result.copied==1&&result.present==2&&result.present_null==1&&
          result.matching_source==1&&result.missing_source==1&&
          result.missing_keys==std::vector<std::string>{"absent"},"fallback reports exact changes");
    auto reopened=RulesDatabase::from_saved_image(saved->saved_image());
    std::string existing;
    std::int64_t wide=0;
    reopened->query({"SELECT Value FROM SimpleValues WHERE Name='existing'",{}},
        [&](auto row){existing=std::get<std::string>(row[0]);});
    reopened->query({"SELECT Value FROM SimpleValues WHERE Name='wide'",{}},
        [&](auto row){wide=std::get<std::int64_t>(row[0]);});
    check(existing=="saved"&&wide==5000000001LL,"fallback preserves original value and x64 copied value");
    check(reopened->query({"SELECT 1 FROM SimpleValues WHERE Name='unlisted'",{}},
        [](auto) {})==0,"fallback does not copy unlisted values");
    check(reopened->query({"SELECT 1 FROM SimpleValues WHERE Name='null_value' AND Value IS NULL",{}},
        [](auto) {})==1,"fallback preserves explicit saved NULL values");
    auto conflicting=RulesDatabase::new_saved_values();
    conflicting->apply_sql("INSERT INTO SimpleValues VALUES('existing','different')");
    auto conflict_plan=plan;conflict_plan.keys={"wide","existing"};
    rejects([&]{fill_missing_saved_values(*conflicting,*source,conflict_plan);});
    check(conflicting->query({"SELECT 1 FROM SimpleValues WHERE Name='wide'",{}},
        [](auto) {})==0,"conflicting source rejects all pending copies atomically");

    const auto lua_plan=parse_saved_value_fallback_plan(read_xml_metadata(
        "<SavedValueFallback modId='09f11249-de39-4884-975d-9d8f01c241d7' modVersion='5'>"
        "<LuaPreset call='Write(Modding.OpenSaveData())'><Script path='preset.lua'/>"
        "<Script path='writer.lua'/></LuaPreset><Key name='wide'/><Key name='null_value'/>"
        "</SavedValueFallback>"));
    check(lua_plan.lua_preset&&lua_plan.lua_preset->scripts.size()==2&&lua_plan.user_data_name.empty(),
        "Lua fallback selects authored scripts without requiring ModUserData");
    rejects([]{parse_saved_value_fallback_plan(read_xml_metadata(
        "<SavedValueFallback modId='09f11249-de39-4884-975d-9d8f01c241d7' modVersion='5' userData='BE' userDataVersion='1'>"
        "<LuaPreset call='Write()'><Script path='preset.lua'/></LuaPreset><Key name='wide'/>"
        "</SavedValueFallback>"));});
    const auto root=std::filesystem::temp_directory_path()/
        ("civis-lua-fallback-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    std::filesystem::create_directories(root);
    const auto preset=root/"preset.lua",writer=root/"writer.lua";
    {std::ofstream out(preset,std::ios::binary);out<<"wide=5000000001; absent=false";}
    {std::ofstream out(writer,std::ios::binary);
        out<<"function Write(db) db.SetValue('wide',wide); db.SetValue('null_value',absent) end";}
    PackageManifest package{};package.kind=PackageKind::mod;package.path=root/"Fixture.modinfo";
    package.id=lua_plan.mod_id;package.version=lua_plan.mod_version;
    package.files={{"preset.lua",civ5::runtime::file_md5(preset),true},
                   {"writer.lua",civ5::runtime::file_md5(writer),true}};
    RulesDatabase activated;
    auto lua_source=load_saved_value_fallback_lua_source(package,activated,lua_plan);
    auto lua_saved=RulesDatabase::new_saved_values();
    lua_saved->apply_sql("INSERT INTO SimpleValues VALUES('null_value',NULL)");
    const auto lua_result=fill_missing_saved_values(*lua_saved,*lua_source,lua_plan);
    check(lua_result.copied==1&&lua_result.present_null==1,
          "Authored Lua preset copies only the absent key and preserves saved NULL");
    check(scalar(*lua_saved,"SELECT Value FROM SimpleValues WHERE Name='wide'")==DatabaseValue{5000000001.0},
          "Authored Lua preset retains an exact value beyond the 32-bit range");
    {std::ofstream out(preset,std::ios::app|std::ios::binary);out<<" -- modified";}
    rejects([&]{load_saved_value_fallback_lua_source(package,activated,lua_plan);});
    package.files.pop_back();
    rejects([&]{load_saved_value_fallback_lua_source(package,activated,lua_plan);});
}
