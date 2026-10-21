#pragma once
void info_collection_tests() {
    RulesDatabase db;
    db.apply_sql("CREATE TABLE Items(ID,Type); INSERT INTO Items VALUES"
                 "(7,'D'),(0,'A'),(0,'B'),(1,'C'),(-1,'SKIP'),(NULL,'SKIP_NULL'),"
                 "(4294967301,'W'),(9223372036854775807,'M'),(9223372036854775807,'N');");
    std::vector<std::uint64_t> slots;
    std::vector<std::int64_t> ids;
    std::string names;
    const auto report = db.prefetch_info("Items", [&](auto slot, auto id, const RulesRowView &row) {
        slots.push_back(slot);
        ids.push_back(id);
        names += row.text("Type").value_or("?");
        equals(db, "SELECT count(*) FROM Items", std::int64_t{9});
        return id != 0;
    });
    check(slots == std::vector<std::uint64_t>{0, 1, 2, 7, 4294967301ULL, INT64_MAX,
                                             std::uint64_t{INT64_MAX} + 1} &&
              ids == std::vector<std::int64_t>{0, 0, 1, 7, 4294967301LL, INT64_MAX, INT64_MAX} &&
              names == "ABCDWMN", "Collection changed source row order or duplicate append slots");
    check(report.rows == 7 && report.slots == std::uint64_t{INT64_MAX} + 2 &&
              report.holes == report.slots - report.rows && report.overlapping_ids == 3 &&
              report.cache_failures == 2 && report.error.empty() && !report.all_succeeded(),
          "Collection stopped on CacheResults=false or narrowed its logical span");
    check(db.prefetch_info("Items").slots == report.slots,
          "Count-only collection disagrees with typed traversal");
    rejects([&] { db.prefetch_info("Items", {}, {6}); });
    check(db.prefetch_info("Items", {}, {7}).all_succeeded(), "Exact row budget rejected");
    std::size_t visits = 0;
    rejects([&] {
        db.prefetch_info("Items", [&](auto, auto, const auto &) -> bool {
            ++visits;
            db.query({"SELECT * FROM MissingNested", {}}, [](auto) {});
            return true;
        });
    });
    check(visits == 1, "Nested SQL exception was swallowed as a collection read error");
    db.apply_sql("CREATE TABLE Empty(ID); CREATE TABLE MissingId(Type);"
                 "CREATE TABLE Lowercase(id); INSERT INTO Lowercase VALUES(8),(9);"
                 "CREATE TABLE Converted(ID); INSERT INTO Converted VALUES(1.9),('12tail'),('bad');");
    check(db.prefetch_info("Empty", {}, {0}).all_succeeded() && db.prefetch_info("Empty").slots == 0,
          "Empty collection consumes budget or slots");
    for (const auto name : {"Absent", "MissingId"}) {
        const auto error = db.prefetch_info(name);
        check(!error.all_succeeded() && error.sqlite_code != 0 && error.rows == 0,
              "Collection query failure was reported as success");
    }
    check(db.prefetch_info("Lowercase").slots == 2,
          "Named ID getter became case-insensitive with SQL identifier matching");
    slots.clear(); ids.clear();
    db.prefetch_info("Converted", [&](auto slot, auto id, const auto &) {
        slots.push_back(slot); ids.push_back(id); return true;
    });
    check(slots == std::vector<std::uint64_t>{1, 12, 13} && ids == std::vector<std::int64_t>{1, 12, 0},
          "Collection converted before SQLite filtering/sorting or rejected coercible IDs");
    db.apply_sql("CREATE TABLE NegativeText(ID); INSERT INTO NegativeText VALUES('-2tail');");
    check(db.prefetch_info("NegativeText").slots == UINT64_MAX,
          "Coerced negative ID did not widen source size_t conversion");
    db.apply_sql("INSERT INTO NegativeText VALUES('-1tail');");
    rejects([&] { db.prefetch_info("NegativeText"); });
    db.apply_sql("CREATE TABLE Ordered(ID INTEGER PRIMARY KEY); INSERT INTO Ordered VALUES(0),(1),(2);"
                 "CREATE VIEW Partial AS SELECT ID,CASE WHEN ID=2 THEN abs(-9223372036854775808)"
                 " ELSE ID END AS Value FROM Ordered;");
    const auto partial = db.prefetch_info("Partial");
    check(!partial.all_succeeded() && partial.rows == 2 && partial.slots == 2 && partial.sqlite_code != 0,
          "Step error lost the successfully loaded collection prefix");
    // Quote the whole table name; neither dots nor quotes are SQL fragments.
    db.apply_sql("CREATE TABLE \"Odd.\"\"Table\"(ID); INSERT INTO \"Odd.\"\"Table\" VALUES(4294967300);");
    check(db.prefetch_info("Odd.\"Table").slots == 4294967301ULL,
          "Collection table name was interpreted as SQL");
    rejects([&] { db.prefetch_info(std::string("Items\0suffix", 12)); });
    rejects([&] { db.prefetch_info(""); });
    db.apply_sql("CREATE TEMP TABLE Items(ID); INSERT INTO Items VALUES(23);");
    check(db.prefetch_info("Items").slots == 24, "Collection lost source temporary table resolution");

    RulesDatabase getters;
    getters.apply_sql("CREATE TABLE Getters(ID INTEGER PRIMARY KEY,Value,Declared BOOLEAN);"
        "INSERT INTO Getters(Value,Declared) VALUES(0,0),(1,1),(-1,-1),(4294967296,4294967296),"
        "('true','true'),('Tomato','Tomato'),('1','1'),('false','false'),(' t',' t'),('', ''),"
        "(1.5,1.5),(0.0,0.0),(x'74',x'74'),(NULL,NULL),('t'||char(0)||'suffix',NULL);");
    const std::vector<bool> expected{false,true,true,true,true,true,false,false,false,false,
                                   false,false,false,false,true};
    std::size_t index = 0;
    const auto getter_report = getters.prefetch_info("Getters", [&](auto, auto, const RulesRowView &row) {
        check(row.boolean("Value") == expected.at(index) &&
                  row.boolean("Declared") == (index == 6 ? true : index == 14 ? false : expected[index]),
              "GetBool changed storage-type dispatch, text prefix, affinity or wide integer semantics");
        check(row.has_column("Value") && !row.has_column("value") && !row.has_column("Absent") &&
                  !row.boolean("Absent") && row.integer("Absent") == 0 && row.real("Absent") == 0 &&
                  !row.text("Absent"), "Named column matching or missing getter defaults changed");
        if (index == 3)
            check(row.integer("Value") == 4294967296LL && row.real("Value") == 4294967296.0 &&
                      row.text("Value") == "4294967296", "Row getter narrowed a wide integer");
        if (index == 6) check(row.integer("Value") == 1, "GetInt lost SQLite text conversion");
        if (index == 10) check(row.integer("Value") == 1 && row.real("Value") == 1.5,
                               "Numeric getters changed float conversion");
        if (index == 12) check(row.text("Value") == "t", "GetText lost SQLite blob conversion");
        if (index == 13) check(!row.text("Value"), "NULL GetText became an empty string");
        if (index == 14) check(row.text("Value") == "t" &&
                                  row.text(std::string_view("Value\0suffix", 12)) == "t",
                               "GetText or column name lost the native C-string boundary");
        ++index;
        return true;
    });
    check(getter_report.all_succeeded() && index == expected.size(), "Getter rows were skipped");
}
