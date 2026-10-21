#pragma once
void civis_saved_database_tests(const fs::path &root){
    const auto section_ids=[](const saves::CivisSaveDocument &document){
        std::vector<std::uint32_t> ids;
        for(const auto &section:document.sections)ids.push_back(section.id);
        std::sort(ids.begin(),ids.end());return ids;
    };
    // Independent minimal SQLite page, then let the actual x64 DB add content.
    std::vector<std::uint8_t> empty(512);
    const std::string magic("SQLite format 3\0",16);std::copy(magic.begin(),magic.end(),empty.begin());
    const std::array<std::uint8_t,8> fields{2,0,1,1,0,64,32,32};std::copy(fields.begin(),fields.end(),empty.begin()+16);
    const auto be32=[&](std::size_t at,std::uint32_t value){for(unsigned i=0;i<4;++i)empty[at+i]=static_cast<std::uint8_t>(value>>(24-8*i));};
    for(const auto at:{24u,28u,40u,56u,92u})be32(at,1);
    be32(44,4);be32(96,3053004);empty[100]=13;empty[105]=2;
    saves::validate_saved_database_image(empty);
    auto database=content::RulesDatabase::from_saved_image(empty);
    database->apply_sql("CREATE TABLE SimpleValues(Name TEXT PRIMARY KEY,Value VARIANT);"
                        "INSERT INTO SimpleValues VALUES('wide',5000000001);");
    const auto first_image=database->saved_image();
    saves::LoadedSave original;
    original.header=parse(header(1,42));original.bytes={4,5,6};
    const auto prefix=header(1,42)+std::string("\0opaque\xff",8);
    original.source_prefix=std::vector<std::uint8_t>(prefix.begin(),prefix.end());
    const std::string origin_hash(64,'a');
    auto first=saves::make_civis_import_document(original,origin_hash,std::span(first_image));
    const auto first_path=root/"changed-database.CivisSave";
    saves::write_civis_save(first_path,first);
    const auto loaded=saves::read_loaded_save(first_path);
    check(loaded.saved_database_override==first_image&&loaded.bytes==original.bytes&&loaded.header==original.header&&
          loaded.source_prefix==original.source_prefix&&loaded.source_sha256==origin_hash&&
          loaded.standalone_services==std::optional{saves::StandaloneServicesConfig{}},
          "Civis database section must preserve current mod data and retained source provenance");
    auto reversed=first;std::reverse(reversed.sections.begin(),reversed.sections.end());
    saves::write_civis_save(root/"changed-database-reversed.CivisSave",reversed);
    check(saves::read_loaded_save(root/"changed-database-reversed.CivisSave").saved_database_override==first_image,
          "Database section reader must not depend on section order");
    const auto kept=saves::make_civis_import_document(loaded,runtime::identify_file(first_path).sha256);
    saves::write_civis_save(root/"changed-database-kept.CivisSave",kept);
    check(saves::read_loaded_save(root/"changed-database-kept.CivisSave").saved_database_override==first_image,
          "Repacking must retain an existing current database, not fall back to imported bytes");
    database->apply_sql("UPDATE SimpleValues SET Value=5000000002 WHERE Name='wide'");
    const auto second_image=database->saved_image();
    const auto second=saves::make_civis_import_document(loaded,runtime::identify_file(first_path).sha256,std::span(second_image));
    saves::write_civis_save(root/"changed-database-second.CivisSave",second);
    const auto latest=saves::read_loaded_save(root/"changed-database-second.CivisSave");
    check(latest.saved_database_override==second_image&&latest.source_sha256==origin_hash&&
          latest.bytes==original.bytes&&latest.header==original.header&&
          latest.source_prefix==original.source_prefix&&latest.standalone_services==loaded.standalone_services&&
          section_ids(second)==std::vector<std::uint32_t>({1,4,5,6,42}),
          "Repeated mod-data saves must replace their section and retain original import identity");
    auto reopened=content::RulesDatabase::from_saved_image(*latest.saved_database_override);
    std::int64_t value=0;
    reopened->query({"SELECT Value FROM SimpleValues WHERE Name='wide'",{}},[&](auto row){value=std::get<std::int64_t>(row[0]);});
    check(value==5000000002LL,"Wide mod state must survive an actual Civis file round-trip");
    auto legacy=original;legacy.source_prefix.reset();
    const auto without_prefix=saves::make_civis_import_document(legacy,origin_hash,std::span(first_image));
    saves::write_civis_save(root/"changed-database-legacy.CivisSave",without_prefix);
    const auto legacy_read=saves::read_loaded_save(root/"changed-database-legacy.CivisSave");
    check(!legacy_read.source_prefix&&legacy_read.saved_database_override==first_image,
          "Saved database state must not synthesize missing legacy setup");
    auto with_production=first;
    with_production.sections.push_back({static_cast<std::uint32_t>(saves::CivisSaveSectionId::live_city_production),0,{1,2,3}});
    const auto production_path=root/"changed-production.CivisSave";
    saves::write_civis_save(production_path,with_production);
    const auto loaded_production=saves::read_loaded_save(production_path);
    check(loaded_production.live_city_production==std::vector<std::uint8_t>({1,2,3})&&
          loaded_production.saved_database_override==first_image&&loaded_production.bytes==original.bytes,
          "Civis container transports live production beside original payload and current mod data");
    bool overlay_guard=false;
    try{saves::observe_loaded_session(loaded_production,{});}
    catch(const std::exception &error){overlay_guard=std::string(error.what()).find("overlay-aware")!=std::string::npos;}
    check(overlay_guard,"Source-only session readers reject live state instead of silently showing stale production");
    const auto repacked=saves::make_civis_import_document(loaded_production,runtime::identify_file(production_path).sha256);
    saves::write_civis_save(root/"changed-production-repacked.CivisSave",repacked);
    check(saves::read_loaded_save(root/"changed-production-repacked.CivisSave").live_city_production==
          loaded_production.live_city_production,"Repacking a Civis save retains live production state");
    auto all_supported=first;
    for(const auto id:{saves::CivisSaveSectionId::live_city_production,
                       saves::CivisSaveSectionId::live_team_research,
                       saves::CivisSaveSectionId::vp_research_profile,
                       saves::CivisSaveSectionId::live_treasury,
                       saves::CivisSaveSectionId::live_research_turn,
                       saves::CivisSaveSectionId::vp_science_profile,
                       saves::CivisSaveSectionId::vp_espionage_profile,
                       saves::CivisSaveSectionId::live_simulation_random})
        all_supported.sections.push_back({static_cast<std::uint32_t>(id),0,{1}});
    check(section_ids(all_supported)==std::vector<std::uint32_t>({1,4,5,6,7,8,9,10,11,12,13,14,42}),
          "Full supported import sections must retain the original standalone services profile");
    const auto all_path=root/"all-supported-sections.CivisSave";
    saves::write_civis_save(all_path,all_supported);
    const auto all_loaded=saves::read_loaded_save(all_path);
    check(all_loaded.live_city_production==std::vector<std::uint8_t>({1})&&
          all_loaded.live_simulation_random==std::vector<std::uint8_t>({1})&&
          all_loaded.profile_sections.size()==3,
          "Full supported import section set was rejected or discarded");
    unsigned bad_index=0;
    const auto invalid=[&](saves::CivisSaveDocument document){
        const auto path=root/("bad-database-section-"+std::to_string(bad_index++)+".CivisSave");
        rejects([&]{saves::write_civis_save(path,document);});
        check(!fs::exists(path),"Malformed import was published before refusal");
        // Deliberately corrupt a generic container's flags to preserve the
        // independent loader negative test despite stricter production writes.
        document.flags=0;saves::write_civis_save(path,document);
        {std::fstream wire(path,std::ios::binary|std::ios::in|std::ios::out);
         wire.seekp(12);const char import_flags[4]={1,0,0,0};
         wire.write(import_flags,4);wire.flush();check(static_cast<bool>(wire),"Cannot corrupt fixture import flags");}
        rejects([&]{saves::read_loaded_save(path);});
    };
    auto bad=first;bad.sections.back().flags=1;invalid(bad);
    bad=first;bad.sections.back().bytes.clear();invalid(bad);
    bad=first;bad.sections.back().bytes.resize(99);invalid(bad);
    bad=first;bad.sections.back().bytes[0]='X';invalid(bad);
    bad=first;bad.sections.back().bytes[100]=0;invalid(bad);
    for(const auto id:{saves::CivisSaveSectionId::session_state,saves::CivisSaveSectionId::rules_snapshot}){
        bad=first;bad.sections.back().id=static_cast<std::uint32_t>(id);invalid(bad);
    }
    bad=first;bad.sections.back().id=77;invalid(bad);
    bad=with_production;bad.sections.back().flags=1;invalid(bad);
    bad=with_production;bad.sections.back().bytes.clear();invalid(bad);
    bad=first;std::erase_if(bad.sections,[](const auto &section){
        return section.id==static_cast<std::uint32_t>(saves::CivisSaveSectionId::imported_payload);
    });invalid(bad);
    bad=first;bad.sections.push_back(first.sections.back());rejects([&]{saves::write_civis_save(root/"duplicate-database.CivisSave",bad);});
    rejects([&]{saves::make_civis_import_document(original,origin_hash,std::span<const std::uint8_t>{});});
    rejects([&]{saves::write_civis_save(first_path,second);});
    check(saves::read_loaded_save(first_path).saved_database_override==first_image,"Exclusive creation must preserve an existing save");
    rejects([&]{saves::read_loaded_save(first_path,{}, {.maximum_section_bytes=first_image.size()-1});});
}
