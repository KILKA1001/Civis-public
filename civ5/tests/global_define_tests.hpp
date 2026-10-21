#pragma once
void global_define_tests() {
    RulesDatabase db;
    using Kind = GlobalDefineKind;
    check(!db.global_define("ABSENT", Kind::integer).error.empty(),
          "Missing Defines table was confused with missing value");
    db.apply_sql("CREATE TABLE Defines(Name TEXT COLLATE NOCASE,Value);"
                 "INSERT INTO Defines VALUES('mixed',9),('mixed',2),('wide',4294967301),"
                 "('null',NULL),('text','42tail'),('round',16777217),('real',0.1),"
                 "('nultext','first'||char(0)||'last');");
    const auto integer = [&](const char *name) { return std::get<std::int64_t>(db.global_define(name, Kind::integer).value); };
    check(integer("MiXeD") == 9 && integer("wide") == 4294967301LL && integer("text") == 42,
          "Global lookup lost LIMIT 1, column collation, or SQLite int64 conversion");
    const auto missing = db.global_define("absent", Kind::integer), null_value = db.global_define("null", Kind::integer);
    check(!missing.found && missing.error.empty() && std::get<std::int64_t>(missing.value) == 0 &&
                  null_value.found && std::get<std::int64_t>(null_value.value) == 0,
          "Missing define was confused with a present NULL");
    check(std::get<double>(db.global_define("round", Kind::source_float).value) == 16777216.0 &&
                  std::get<double>(db.global_define("round", Kind::real).value) == 16777217.0,
          "Source float rounding leaked into wide real lookup or was discarded");
    check(std::get<std::string>(db.global_define("nultext", Kind::text).value) == "first" &&
                  std::get<std::string>(db.global_define("wide", Kind::text).value) == "4294967301" &&
                  std::get<std::string>(db.global_define("null", Kind::text).value).empty(),
          "Text global conversion changed source C-string/NULL behavior");
    check(std::get<std::int64_t>(db.global_define(std::string_view("wide\0ignored", 12), Kind::integer).value) == 4294967301LL,
          "Global lookup name did not use source C-string prefix");
    db.apply_sql("CREATE INDEX define_order ON Defines(Name,Value);");
    check(integer("mixed") == 2, "Global lookup substituted a dictionary for the source query plan");
    db.apply_sql("UPDATE Defines SET Value=4294967399 WHERE Name='wide';");
    check(integer("wide") == 4294967399LL, "Prepared define statement retained stale data");
    const auto maximum = static_cast<double>(std::numeric_limits<float>::max());
    const std::array<DatabaseStatement, 3> boundaries{{
        {"INSERT INTO Defines VALUES('edge',?)", {std::nextafter(maximum, std::numeric_limits<double>::infinity())}},
        {"INSERT INTO Defines VALUES('overflow',?)", {maximum + std::ldexp(1.0, 103)}},
        {"INSERT INTO Defines VALUES('negative_overflow',?)", {-1e40}}}};
    db.apply_statements(boundaries);
    check(std::get<double>(db.global_define("edge", Kind::source_float).value) == maximum &&
                  std::get<double>(db.global_define("overflow", Kind::source_float).value) == std::numeric_limits<double>::infinity() &&
                  std::get<double>(db.global_define("negative_overflow", Kind::source_float).value) == -std::numeric_limits<double>::infinity() &&
                  std::get<double>(db.global_define("negative_overflow", Kind::real).value) == -1e40,
          "Source float overflow rounding or wide-real range changed");
    db.apply_sql("DROP TABLE Defines;");
    check(!db.global_define("mixed", Kind::integer).error.empty(), "Invalidated lookup statement hid schema failure");
    db.apply_sql("CREATE TABLE Defines(Name,Value); INSERT INTO Defines VALUES('after',81);");
    check(integer("after") == 81, "Global lookup could not recover after schema replacement");
    rejects([&] { (void)db.global_define("after", static_cast<Kind>(99)); });
    rejects([&] { (void)global_define_specs(static_cast<GlobalDefineProfile>(99)); });

    for (const auto profile : {GlobalDefineProfile::bnw, GlobalDefineProfile::vp542}) {
        RulesDatabase rules;
        rules.apply_sql("CREATE TABLE Defines(Name TEXT UNIQUE,Value);");
        const auto specs = global_define_specs(profile);
        const auto initial = rules.cache_global_defines(profile);
        check(initial.entries().size() == (profile == GlobalDefineProfile::bnw ? 1689 : 1879) &&
                      initial.missing() == specs.size() && initial.errors() == 0,
              "Global cache contract is incomplete or defaults were reported as SQL errors");
        check(std::get<std::int64_t>(initial.find("AI_ATTEMPT_RUSH_OVER_X_TURNS_TO_BUILD")->lookup.value) ==
                      (profile == GlobalDefineProfile::bnw ? 0 : 15),
              "BNW zero getter/VP constructor default semantics changed");
        std::vector<DatabaseStatement> inserts;
        for (std::size_t i = 0; i < specs.size(); ++i)
            inserts.push_back({"INSERT INTO Defines VALUES(?,?)", {std::string(specs[i].name),
                specs[i].kind == Kind::integer ? DatabaseValue(std::int64_t{4294967301LL} + static_cast<std::int64_t>(i))
                                               : DatabaseValue(7.25 + i)}});
        rules.apply_statements(inserts);
        const auto loaded = rules.cache_global_defines(profile, &initial);
        check(loaded.missing() == 0 && loaded.errors() == 0, "Not every source global was read");
        for (std::size_t i = 0; i < specs.size(); ++i) {
            const auto *entry = loaded.find(specs[i].name);
            check(entry && entry->lookup.found && !entry->retained_previous && entry->kind == specs[i].kind &&
                          (specs[i].kind == Kind::integer
                              ? std::get<std::int64_t>(entry->lookup.value) == std::int64_t{4294967301LL} + i
                              : std::get<double>(entry->lookup.value) == 7.25 + i),
                  "Source global type dispatch or wide integer value changed");
        }
        rules.apply_sql("DELETE FROM Defines;");
        const auto reloaded = rules.cache_global_defines(profile, &loaded);
        const auto *retained = reloaded.find("AI_ATTEMPT_RUSH_OVER_X_TURNS_TO_BUILD");
        check(!retained->lookup.found && retained->retained_previous == (profile == GlobalDefineProfile::vp542) &&
                      std::get<std::int64_t>(retained->lookup.value) == (profile == GlobalDefineProfile::bnw ? 0 : 4294967301LL),
              "Reload did not preserve the source's missing-value behavior");
        rules.apply_sql("INSERT INTO Defines VALUES('POLICY_BRANCH_FREEDOM',0);");
        const auto ideology = rules.cache_global_defines(profile);
        check(std::get<std::int64_t>(ideology.find("POLICY_BRANCH_FREEDOM")->lookup.value) ==
                      (profile == GlobalDefineProfile::bnw ? -1 : 0),
              "BNW ideology zero-to-NONE fixup leaked into VP or disappeared");
        rules.apply_sql("DROP TABLE Defines;");
        const auto failed = rules.cache_global_defines(profile, &loaded);
        check(failed.errors() == specs.size() && failed.missing() == 0 &&
                      !failed.find("AI_ATTEMPT_RUSH_OVER_X_TURNS_TO_BUILD")->lookup.error.empty(),
              "Global cache SQL errors were hidden behind defaults");
        rejects([&] { (void)rules.cache_global_defines(profile == GlobalDefineProfile::bnw
                         ? GlobalDefineProfile::vp542 : GlobalDefineProfile::bnw, &loaded); });
    }
}
