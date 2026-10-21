#pragma once
void info_type_tests() {
    InfoTypeCatalog retained;
    {
        RulesDatabase db;
        db.apply_sql("CREATE TABLE ZFirst(ID,Type);"
                     "INSERT INTO ZFirst VALUES(4294967301,'DUP'),(-7,'NEGATIVE'),(99,NULL),"
                     "(NULL,'NULL_ID'),('42tail','TEXT_ID'),(10.75,'REAL_ID'),(5,''),"
                     "(6,'Case'),(7,'CASE'),(8,'TYPE_4978D320478D8056');"
                     "CREATE TABLE ASecond(ID,Type);"
                     "INSERT INTO ASecond VALUES(12,'DUP'),(11,'DUP'),"
                     "(9223372036854775807,'WIDE'),(13,'TYPE_5348478E677AAFE1'),(-1,'NONE');"
                     "CREATE TABLE Empty(ID,Type); CREATE TABLE NoId(Type); INSERT INTO NoId VALUES('IGNORE');"
                     "CREATE TABLE Lowercase(id,type); INSERT INTO Lowercase VALUES(123,'LOWER');"
                     "CREATE VIEW ViewOnly AS SELECT 200 AS ID,'VIEW' AS Type;"
                     "CREATE TEMP TABLE TempOnly(ID,Type); INSERT INTO TempOnly VALUES(300,'TEMP');");
        const auto catalog = db.cache_info_types();
        const auto &report = catalog.report();
        check(report.all_succeeded() && report.tables == 5 && report.eligible_tables == 2 &&
                  report.rows == 15 && report.null_types == 1,
              "Info type inventory included a view/temp-only table or skipped eligible rows");
        check(catalog.find("DUP") && catalog.find("DUP")->id == 11 &&
                  catalog.find_hash(saved_info_type_hash("DUP"))->id == 11 &&
                  catalog.find("DUP")->table == "ASecond" && report.name_overwrites == 2,
              "Info type loading sorted table names/IDs or changed last-row overwrite");
        check(catalog.find("WIDE")->id == INT64_MAX && catalog.find("NEGATIVE")->id == -7 &&
                  catalog.find("NONE")->id == -1 && !catalog.find("ABSENT") && !catalog.find(nullptr),
              "Info types narrowed IDs or lost distinction between missing and ID -1");
        check(catalog.find("NULL_ID")->id == 0 && catalog.find("TEXT_ID")->id == 42 &&
                  catalog.find("REAL_ID")->id == 10,
              "Info type ID conversion changed SQLite integer semantics");
        check(catalog.find("")->id == 5 && catalog.find("Case")->id == 6 && catalog.find("CASE")->id == 7 &&
                  !catalog.find("case") && !catalog.find("LOWER") && !catalog.find("VIEW") && !catalog.find("TEMP"),
              "Info types changed exact case, empty names, or source table filtering");
        constexpr auto first = "TYPE_4978D320478D8056", second = "TYPE_5348478E677AAFE1";
        check(saved_info_type_hash(first) == 0x2b0c0da6u && saved_info_type_hash(second) == 0x2b0c0da6u &&
                  catalog.find_hash(0x2b0c0da6u)->id == 13 && catalog.find(first)->id == 8 &&
                  catalog.find(second)->id == 13 && report.hash_collisions == 1 &&
                  !catalog.find_hash(0x12b0c0da6ULL) &&
                  catalog.hashes() + 1 == catalog.names() && report.collision_samples.size() == 3,
              "CRC collision changed independent name lookups or did not keep last hash entry");
        db.apply_sql("DELETE FROM ASecond; DELETE FROM ZFirst WHERE Type LIKE 'TYPE_%';"
                     "INSERT INTO ZFirst VALUES(77,'prefix'||char(0)||'suffix');");
        auto rebuilt = db.cache_info_types();
        check(rebuilt.find("DUP")->id == 4294967301LL && !rebuilt.find("WIDE") &&
                  !rebuilt.find_hash(0x2b0c0da6u) && catalog.find("WIDE")->id == INT64_MAX,
              "Rebuilding retained stale info types or changed an existing snapshot");
        check(rebuilt.find("prefix")->id == 77 &&
                  rebuilt.find(std::string_view("prefix\0other", 12))->id == 77 &&
                  rebuilt.find_hash(saved_info_type_hash("prefix"))->id == 77,
              "Info type C-string terminator semantics changed");
        rejects([&] { (void)db.cache_info_types({1, 100, 0}); });
        rejects([&] { (void)db.cache_info_types({100, 1, 0}); });
        equals(db, "SELECT count(*) FROM ASecond", std::int64_t{0});
        retained = std::move(rebuilt);
    }
    check(retained.find("DUP")->id == 4294967301LL, "Info type snapshot retained database pointers");
    RulesDatabase db;
    db.apply_sql("CREATE TABLE Broken(ID,Type); INSERT INTO Broken VALUES(1,'WRONG');"
                 "CREATE TEMP VIEW Broken AS SELECT * FROM Absent;"
                 "CREATE TABLE Good(ID,Type); INSERT INTO Good VALUES(2,'GOOD');");
    const auto partial = db.cache_info_types();
    check(!partial.report().all_succeeded() && partial.report().errors.size() == 1 &&
              partial.report().errors[0].table == "Broken" && partial.find("GOOD")->id == 2 &&
              !partial.find("WRONG"), "Failed table read stopped the source scan or hid an error");
    db.apply_sql("DROP VIEW temp.Broken; CREATE TEMP TABLE Broken(ID,Type);"
                 "INSERT INTO temp.Broken VALUES(3,'SHADOW');");
    check(db.cache_info_types().find("SHADOW")->id == 3,
          "Info type scan changed native temporary-object name resolution");
    const std::string long_table(1600, 'T'), long_type(2000, 'X');
    db.apply_sql("CREATE TABLE " + long_table + "(ID,Type);");
    db.apply_statements(std::array<DatabaseStatement, 1>{DatabaseStatement{
        "INSERT INTO " + long_table + " VALUES(?,?)", {std::int64_t{INT64_MAX}, long_type}}});
    const auto wide = db.cache_info_types();
    check(wide.find(long_type)->id == INT64_MAX && wide.find(long_type)->table == long_table,
          "Info type loading retained fixed native SQL/name buffers");
    db.apply_sql("CREATE TABLE Ordered(ID INTEGER PRIMARY KEY, Type);"
                 "INSERT INTO Ordered VALUES(555,'REMAPPED');"
                 "CREATE TABLE Remapper(name); INSERT INTO Remapper VALUES('Ordered');");
    check(db.remap_vp_ids().all_succeeded && db.cache_info_types().find("REMAPPED")->id == 0,
          "Info types ignored the activated VP ID remapper");
    db.apply_sql("CREATE TABLE OrderedRows(K TEXT PRIMARY KEY,ID,Type) WITHOUT ROWID;"
                 "INSERT INTO OrderedRows VALUES('z',-5,'ORDERED'),('a',9,'ORDERED');");
    check(db.cache_info_types().find("ORDERED")->id == -5,
          "Info type scan invented ROWID/insertion/ID order for a WITHOUT ROWID table");
}
