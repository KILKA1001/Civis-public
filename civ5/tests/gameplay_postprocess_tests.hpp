#pragma once
void gameplay_postprocess_tests() {
    const auto schema = [](RulesDatabase &db) {
        db.apply_sql("CREATE TABLE Defines(Name TEXT UNIQUE, Value);"
                     "CREATE TABLE PostDefines(Name,Key,\"Table\");"
                     "CREATE TABLE Resources(Type,Happiness,ResourceUsage);"
                     "CREATE TABLE Unit_ResourceQuantityRequirements(ResourceType);"
                     "CREATE TABLE Building_ResourceQuantityRequirements(ResourceType);"
                     "CREATE TABLE Items(ID,Type);");
    };
    for (const auto profile : {GameplayPostprocessProfile::bnw, GameplayPostprocessProfile::vp542}) {
        RulesDatabase db;
        schema(db);
        db.apply_sql("INSERT INTO Items(ROWID,ID,Type) VALUES(4294967297,12,'WIDE'),(3,99,'DUP'),"
                     "(4,98,'DUP'),(5,97,'quote''key');"
                     "INSERT INTO Defines VALUES('MISSING',777),('VALUE',-1);"
                     "INSERT INTO PostDefines VALUES('VALUE','WIDE','Items'),('FIRST','DUP','Items'),"
                     "('MISSING','absent','Items'),('QUOTED','quote''key','Items'),"
                     "('MAX_PLAYERS','DUP','Items');"
                     "INSERT INTO Resources VALUES('unit',0,77),('building',0,77),('both',4,77),('none',0,77);"
                     "INSERT INTO Unit_ResourceQuantityRequirements VALUES('unit'),('both');"
                     "INSERT INTO Building_ResourceQuantityRequirements VALUES('building');");
        const std::array<GameplayDefine, 2> constants{{{"MAX_PLAYERS", 4294967301LL}, {"NEGATIVE", -1}}};
        const auto report = db.postprocess_gameplay(profile, constants);
        check(report.post_defines.size() == 5 && report.post_defines[0].rowid == 4294967297LL &&
                  report.post_defines[0].applied && report.post_defines[1].rowid == 3 &&
                  !report.post_defines[2].rowid && !report.post_defines[2].applied &&
                  report.runtime_defines_applied == 2 && report.transaction_released,
              "PostDefines changed source order, ROWID lookup, skip behavior or runtime constants");
        check(report.all_succeeded == (profile == GameplayPostprocessProfile::vp542),
              "BNW literal/VP bound lookup semantics changed");
        check(report.post_defines[3].applied == (profile == GameplayPostprocessProfile::vp542),
              "Quoted PostDefine key was repaired or rejected for the wrong source profile");
        equals(db, "SELECT Value FROM Defines WHERE Name='VALUE'", std::int64_t{4294967297LL});
        equals(db, "SELECT Value FROM Defines WHERE Name='MISSING'", std::int64_t{777});
        equals(db, "SELECT Value FROM Defines WHERE Name='MAX_PLAYERS'", std::int64_t{4294967301LL});
        equals(db, "SELECT ResourceUsage FROM Resources WHERE Type='unit'", std::int64_t{1});
        equals(db, "SELECT ResourceUsage FROM Resources WHERE Type='building'",
               std::int64_t{profile == GameplayPostprocessProfile::vp542 ? 1 : 77});
        equals(db, "SELECT ResourceUsage FROM Resources WHERE Type='both'", std::int64_t{2});
        equals(db, "SELECT ResourceUsage FROM Resources WHERE Type='none'", std::int64_t{77});
    }
    RulesDatabase db;
    schema(db);
    db.apply_sql("INSERT INTO Items(ROWID,Type) VALUES(8,'A'),(9,'B');"
                 "INSERT INTO PostDefines VALUES('FIRST','A','Items'),('FAIL','A','Items'),"
                 "('LAST','B','Items'),('BAD','A','Absent');"
                 "CREATE TRIGGER fail_one BEFORE INSERT ON Defines WHEN NEW.Name='FAIL' "
                 "BEGIN SELECT RAISE(ABORT,'authored failure'); END;");
    auto report = db.postprocess_gameplay(GameplayPostprocessProfile::vp542, {});
    check(!report.all_succeeded && report.transaction_released && report.post_defines[0].applied &&
              !report.post_defines[1].applied && report.post_defines[2].applied &&
              !report.post_defines[3].errors.empty(), "Postprocessing hid a failure or stopped later rows");
    equals(db, "SELECT Value FROM Defines WHERE Name='LAST'", std::int64_t{9});
    db.apply_sql("DELETE FROM Defines; DROP TRIGGER fail_one;"
                 "CREATE TRIGGER rollback_one BEFORE INSERT ON Defines WHEN NEW.Name='FAIL' "
                 "BEGIN SELECT RAISE(ROLLBACK,'authored rollback'); END;");
    const std::array<GameplayDefine, 1> after_rollback{{{"AFTER", 99}}};
    report = db.postprocess_gameplay(GameplayPostprocessProfile::vp542, after_rollback);
    check(!report.all_succeeded && !report.transaction_released && report.post_defines[0].applied,
          "Rollback error was hidden or first successful statement was relabeled as committed");
    equals(db, "SELECT count(*) FROM Defines WHERE Name='FIRST'", std::int64_t{0});
    equals(db, "SELECT Value FROM Defines WHERE Name='AFTER'", std::int64_t{99});
    db.apply_sql("DROP TRIGGER rollback_one; DELETE FROM Defines;");
    rejects([&] { (void)db.postprocess_gameplay(GameplayPostprocessProfile::vp542, {}, {1}); });
    equals(db, "SELECT count(*) FROM Defines", std::int64_t{0});

    // The next lookup observes changes caused by the previous insertion.
    db.apply_sql("DELETE FROM PostDefines;"
                 "INSERT INTO PostDefines VALUES('FIRST','A','Items'),('NEXT','B','Items');"
                 "CREATE TRIGGER change_lookup AFTER INSERT ON Defines WHEN NEW.Name='FIRST' "
                 "BEGIN UPDATE Items SET ROWID=4294967399 WHERE Type='B'; END;");
    report = db.postprocess_gameplay(GameplayPostprocessProfile::vp542, {});
    check(report.all_succeeded && report.post_defines[1].rowid == 4294967399LL,
          "PostDefines used a stale precomputed lookup snapshot");
    // Deliberately separate caller-owned runtime values from any source x86 constants.
    const std::string long_name(1025, 'N'), long_key(1500, 'K');
    db.apply_statements(std::array<DatabaseStatement, 2>{
        DatabaseStatement{"INSERT INTO Items(ROWID,Type) VALUES(?,?)", {std::int64_t{INT64_MAX}, long_key}},
        DatabaseStatement{"INSERT INTO PostDefines VALUES(?,?,'Items')", {long_name, long_key}}});
    report = db.postprocess_gameplay(GameplayPostprocessProfile::vp542, {});
    check(report.all_succeeded && report.post_defines.back().rowid == INT64_MAX &&
              report.post_defines.back().name == long_name,
          "PostDefines retained a 512-byte buffer or 32-bit value limit");
    const std::string long_table(1025, 'T');
    db.apply_sql("CREATE TABLE " + long_table + "(Type); INSERT INTO " + long_table + " VALUES('long');");
    db.apply_statements(std::array<DatabaseStatement, 1>{
        DatabaseStatement{"INSERT INTO PostDefines VALUES('LONG_TABLE','long',?)", {long_table}}});
    report = db.postprocess_gameplay(GameplayPostprocessProfile::vp542, {});
    check(report.all_succeeded && report.post_defines.back().rowid == 1,
          "VP lookup retained the fixed table-name SQL buffer");

    db.apply_sql("CREATE TABLE Ordered(ID INTEGER PRIMARY KEY, Type);"
                 "INSERT INTO Ordered VALUES(555,'after_remap');"
                 "CREATE TABLE Remapper(name); INSERT INTO Remapper VALUES('Ordered');"
                 "INSERT INTO PostDefines VALUES('REMAPPED','after_remap','Ordered');");
    check(db.remap_vp_ids().all_succeeded, "Fixture remap failed");
    report = db.postprocess_gameplay(GameplayPostprocessProfile::vp542, {});
    check(report.all_succeeded && report.post_defines.back().rowid == 0,
          "PostDefines used the pre-remap ID instead of the current ROWID");
    db.apply_sql("DROP TABLE PostDefines; DROP TABLE Unit_ResourceQuantityRequirements;"
                 "INSERT INTO Resources VALUES('luxury',3,0);");
    report = db.postprocess_gameplay(GameplayPostprocessProfile::vp542, after_rollback);
    check(!report.all_succeeded && !report.strategic_resources_applied && report.luxury_resources_applied &&
              report.runtime_defines_applied == 1 && report.transaction_released,
          "An unavailable postprocessing stage prevented later source stages");
    equals(db, "SELECT ResourceUsage FROM Resources WHERE Type='luxury'", std::int64_t{2});
    rejects([&] { (void)db.postprocess_gameplay(static_cast<GameplayPostprocessProfile>(9), {}); });
}
