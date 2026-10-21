#pragma once
void technology_schema(RulesDatabase &db) {
    // Deliberately leave scalar columns absent in the small source fixture.
    db.apply_sql("CREATE TABLE Technologies(ID INTEGER PRIMARY KEY,Type,Text,Description,Quote,AudioIntro,AudioIntroHeader,Era,FirstFreeUnitClass,Cost,Disable);"
        "CREATE TABLE Eras(ID INTEGER PRIMARY KEY,Type); CREATE TABLE UnitClasses(ID INTEGER PRIMARY KEY,Type);"
        "CREATE TABLE Domains(ID INTEGER PRIMARY KEY,Type); CREATE TABLE UnitPromotions(ID INTEGER PRIMARY KEY,Type);"
        "CREATE TABLE Flavors(ID INTEGER PRIMARY KEY,Type); CREATE TABLE Specialists(ID INTEGER PRIMARY KEY,Type);"
        "CREATE TABLE Yields(ID INTEGER PRIMARY KEY,Type);"
        "CREATE TABLE Technology_DomainExtraMoves(TechType,DomainType,Moves);"
        "CREATE TABLE Technology_TradeRouteDomainExtraRange(TechType,DomainType,Range);"
        "CREATE TABLE Technology_FreePromotions(TechType,PromotionType);"
        "CREATE TABLE Technology_Flavors(TechType,FlavorType,Flavor);"
        "CREATE TABLE Technology_ORPrereqTechs(TechType,PrereqTech);"
        "CREATE TABLE Technology_PrereqTechs(TechType,PrereqTech);"
        "CREATE TABLE Tech_SpecialistYieldChanges(TechType,SpecialistType,YieldType,Yield);");
}
void technology_rules_tests() {
    const auto bnw = TechnologyProfile::bnw, vp = TechnologyProfile::vp542;
    check(technology_fields(bnw).size() == 40 && technology_fields(vp).size() == 43,
          "Technology scalar contract incomplete");
    rejects([&] { (void)technology_fields(static_cast<TechnologyProfile>(77)); });
    TechnologyRules retained;
    {
        RulesDatabase db;
        technology_schema(db);
        db.apply_sql("INSERT INTO Technologies(ID,Type,Text,Description,Quote,AudioIntro,AudioIntroHeader,Era,FirstFreeUnitClass,Cost,Disable)"
            " VALUES(0,'A','','DESC','QUOTE','SOUND','MP','ERA','CLASS',4294967300,'true'),"
            "(2,'B','TEXT','OTHER',NULL,NULL,NULL,'MISSING',NULL,'71tail',1.5);"
            "INSERT INTO Eras VALUES(4294967301,'ERA'); INSERT INTO UnitClasses VALUES(8,'CLASS');"
            "INSERT INTO Domains VALUES(0,'LAND'),(4294967302,'SEA');"
            "INSERT INTO UnitPromotions VALUES(4294967303,'PROMO'); INSERT INTO Flavors VALUES(3,'FLAVOR');"
            "INSERT INTO Specialists VALUES(4294967304,'SPEC'); INSERT INTO Yields VALUES(4294967305,'YIELD');"
            "INSERT INTO Technology_DomainExtraMoves VALUES('A','LAND',1),('A','LAND',8),('A','SEA',4294967306),('A','UNKNOWN',999);"
            "INSERT INTO Technology_TradeRouteDomainExtraRange VALUES('A','SEA',-9);"
            "INSERT INTO Technology_FreePromotions VALUES('A','PROMO'),('A','PROMO');"
            "INSERT INTO Technology_Flavors VALUES('A','FLAVOR',3),('A','FLAVOR',0);"
            "INSERT INTO Technology_ORPrereqTechs VALUES('A','B'),('A','B'),('A','UNKNOWN');"
            "INSERT INTO Technology_PrereqTechs VALUES('B','A');"
            "INSERT INTO Tech_SpecialistYieldChanges VALUES('A','SPEC','YIELD',4),('A','SPEC','YIELD',4294967307);"
            "CREATE TABLE CrossTable(ID,Type); INSERT INTO CrossTable VALUES(99,'ERA');");
        std::vector<std::string> localized;
        const auto localizer = [&](std::string_view key) {
            localized.emplace_back(key);
            return "L:" + std::string(key);
        };
        retained = load_technology_rules(db, db.cache_info_types(), vp, 5, localizer);
        const auto &a = *retained.find(0), &b = *retained.find(2);
        check(retained.all_succeeded() && retained.entries.size() == 2 && retained.collection.slots == 3 &&
                  retained.collection.holes == 1 && !retained.find(1) && retained.localization_supplied,
              "Technology rules lost collection slots or failed valid wide relations");
        check(a.base.type == "A" && a.base.authored_id == 0 && a.base.text.key == "DESC" &&
                  a.base.text.value == "L:DESC" && a.base.description.value == "L:DESC" &&
                  a.quote.key == "QUOTE" && a.quote.value == "L:QUOTE" && a.audio_intro == "SOUND" &&
                  a.audio_intro_header == "MP" && localized == std::vector<std::string>{"DESC","QUOTE","TEXT","OTHER",""},
              "Technology metadata, quote localization order or sound keys changed");
        check(a.integers.at("Cost") == 4294967300LL && b.integers.at("Cost") == 71 &&
                  a.booleans.at("Disable") && !b.booleans.at("Disable") &&
                  a.era == 99 && a.first_free_unit_class == 8 && b.era == -1 && b.first_free_unit_class == -1 &&
                  a.integers.at("Happiness") == 0 && !a.booleans.at("CityNoEmbarkCost") &&
                  !a.integers.contains("AIWeight"), "Technology fields changed source getters or global references");
        check(a.domain_extra_moves.extent == 4294967303ULL && a.domain_extra_moves.at(0) == 8 &&
                  a.domain_extra_moves.at(1) == 0 && a.domain_extra_moves.at(4294967302ULL) == 4294967306LL &&
                  a.trade_route_domain_extra_range.at(4294967302ULL) == -9 &&
                  a.free_promotions.at(4294967303ULL) == 1 && a.free_promotions.values.size() == 1 &&
                  a.flavors.at(3) == 0 && a.flavors.values.empty(),
              "Technology arrays narrowed, summed duplicates, or retained an overwritten default");
        check(a.or_prerequisites.extent == 3 && a.or_prerequisites.at(0) == 2 &&
                  a.or_prerequisites.at(1) == 2 && a.or_prerequisites.at(2) == -1 &&
                  b.and_prerequisites.at(0) == 0 && b.and_prerequisites.at(1) == -1 &&
                  a.specialist_yield_changes->at(4294967304ULL,4294967305ULL) == 4294967307LL &&
                  a.specialist_yield_changes->at(0,0) == 0,
              "Technology prerequisites or wide sparse matrix changed source semantics");
        check(retained.relation_rows == 13, "Technology links counted missing joins or lost duplicates");
        rejects([&] { (void)a.flavors.at(4); });
        rejects([&] { (void)a.specialist_yield_changes->at(UINT64_MAX,0); });
        const auto old = load_technology_rules(db, db.cache_info_types(), bnw, 5);
        check(old.all_succeeded() && old.find(0)->integers.contains("AIWeight") &&
                  old.find(0)->booleans.contains("TradeAgreementTradingAllowed") &&
                  !old.find(0)->integers.contains("Happiness") && !old.find(0)->specialist_yield_changes &&
                  !old.find(0)->base.description.value && !old.find(0)->quote.value && !old.localization_supplied,
              "BNW loaded VP-only fields or invented localized text");
        db.apply_sql("UPDATE Technologies SET Cost=1; DELETE FROM Technology_ORPrereqTechs;");
        check(retained.find(0)->integers.at("Cost") == 4294967300LL &&
                  retained.find(0)->or_prerequisites.at(0) == 2, "Technology snapshot borrowed mutable rules");
        TechnologyBudget tiny;
        tiny.technologies = 1;
        rejects([&] { load_technology_rules(db, db.cache_info_types(), vp, 5, {}, tiny); });
        tiny.technologies = 2; tiny.relation_rows = 1;
        rejects([&] { load_technology_rules(db, db.cache_info_types(), vp, 5, {}, tiny); });
        rejects([&] { load_technology_rules(db, db.cache_info_types(), vp, 5,
            [](auto) -> std::string { throw std::runtime_error("Host localization failure"); }); });
    }
    check(retained.find(0)->audio_intro == "SOUND", "Technology snapshot borrowed destroyed database");
    for (const auto profile : {bnw, vp}) {
        RulesDatabase db;
        technology_schema(db);
        const auto fields = technology_fields(profile);
        for (const auto &field : fields)
            if (field.column != "Cost" && field.column != "Disable")
                db.apply_sql("ALTER TABLE Technologies ADD COLUMN \"" + std::string(field.column) + "\"");
        db.apply_sql("INSERT INTO Technologies(ID,Type,Text,Description,Quote,AudioIntro) VALUES"
                     "(9223372036854775807,'WIDE','','','Q'||char(0)||'suffix','S'||char(0)||'suffix');"
                     "INSERT INTO Technology_PrereqTechs VALUES('WIDE','WIDE');");
        std::vector<DatabaseStatement> updates;
        for (std::size_t i = 0; i < fields.size(); ++i)
            updates.push_back({"UPDATE Technologies SET \"" + std::string(fields[i].column) + "\" = ?",
                {fields[i].kind == TechnologyFieldKind::integer ? DatabaseValue(std::int64_t{4294967300LL} + static_cast<std::int64_t>(i))
                                                               : DatabaseValue(std::string(i % 2 ? "True" : "1"))}});
        db.apply_statements(updates);
        const auto techs = load_technology_rules(db, db.cache_info_types(), profile, 5,
                                                [](auto key) { return "L:" + std::string(key); });
        const auto &wide = *techs.find(INT64_MAX);
        check(techs.all_succeeded() && techs.collection.slots == (std::uint64_t{INT64_MAX} + 1) &&
                  wide.and_prerequisites.extent == (std::uint64_t{INT64_MAX} + 1) &&
                  wide.and_prerequisites.at(0) == INT64_MAX && wide.and_prerequisites.at(INT64_MAX) == -1 &&
                  wide.audio_intro == "S" && wide.quote.value == "L:Q" && wide.base.text.value == "L:",
              "Wide technology ID, source text boundary or empty Description changed");
        for (std::size_t i = 0; i < fields.size(); ++i) {
            const auto name = std::string(fields[i].column);
            check(fields[i].kind == TechnologyFieldKind::integer
                      ? wide.integers.at(name) == static_cast<std::int64_t>(4294967300ULL + i)
                      : wide.booleans.at(name) == (i % 2 != 0),
                  "A technology scalar did not load through its source getter");
        }
    }
    {
        RulesDatabase db;
        technology_schema(db);
        db.apply_sql("DROP TABLE Technologies; CREATE TABLE Technologies(ID,Type);"
            "INSERT INTO Technologies VALUES(0,'A'),(0,'B'),(1,'C');"
            "INSERT INTO Technology_PrereqTechs VALUES('B','C');");
        const auto techs = load_technology_rules(db, db.cache_info_types(), bnw, 5);
        check(techs.all_succeeded() && techs.find(1)->base.type == "B" &&
                  techs.find(1)->base.authored_id == 0 && techs.find(2)->base.type == "C" &&
                  techs.find(1)->and_prerequisites.extent == 4 && techs.find(1)->and_prerequisites.at(0) == 1,
              "Technology cache confused authored ID, collection slot and MaxRows");
    }
    {
        RulesDatabase db;
        technology_schema(db);
        db.apply_sql("INSERT INTO Technologies(ID,Type) VALUES(0,'A');");
        const auto empty = load_technology_rules(db, db.cache_info_types(), vp, 4097);
        check(empty.all_succeeded() && empty.find(0)->domain_extra_moves.extent == 4097 &&
                  empty.find(0)->free_promotions.extent == 1 && empty.find(0)->flavors.extent == 1 &&
                  empty.find(0)->specialist_yield_changes->rows == 1,
              "Empty MaxRows lost NULL-to-zero plus one, or domain minimum became a legacy cap");
        db.apply_sql("DROP TABLE Flavors; CREATE TABLE Flavors(ID,Type); INSERT INTO Flavors VALUES(10,'F');"
            "INSERT INTO Technology_Flavors VALUES('A','F',7);"
            "INSERT INTO Technology_PrereqTechs VALUES('A','A'),('A','A');"
            "DROP TABLE Technology_DomainExtraMoves;");
        const auto invalid = load_technology_rules(db, db.cache_info_types(), bnw, 5);
        check(!invalid.all_succeeded() && invalid.entries.size() == 1 && invalid.errors == 3 &&
                  invalid.collection.cache_failures == 1 && invalid.find(0)->flavors.at(10) == 7 &&
                  invalid.find(0)->flavors.extent == 11 && invalid.find(0)->and_prerequisites.extent == 2,
              "Invalid source arrays corrupted memory, lost values, or masqueraded as complete");
        TechnologyBudget no_samples;
        no_samples.error_samples = 0;
        const auto sampled = load_technology_rules(db, db.cache_info_types(), bnw, 5, {}, no_samples);
        check(sampled.errors == 3 && sampled.error_samples.empty(), "Technology diagnostic budget lost error count");
        db.apply_sql("DROP TABLE Technologies;");
        const auto absent = load_technology_rules(db, db.cache_info_types(), bnw, 5);
        check(!absent.all_succeeded() && absent.entries.empty() && !absent.collection.error.empty(),
              "Missing technology collection reported ready");
    }
    // A prepared row can be inspected with native index conversion, and a
    // failing linked statement retains its successfully visited prefix.
    {
        RulesDatabase db;
        db.apply_sql("CREATE TABLE Rows(ID INTEGER PRIMARY KEY); INSERT INTO Rows VALUES(0),(1),(2);");
        std::vector<std::int64_t> values;
        const auto report = db.query_rows({"SELECT ID,CASE WHEN ID=2 THEN abs(-9223372036854775808) ELSE '42tail' END FROM Rows ORDER BY ID",{}}, [&](const auto &row) {
            values.push_back(row.integer_at(1));
            check(row.integer_at(-1) == 0 && row.integer_at(2) == 0, "Invalid column index changed native getter default");
        });
        check(report.rows == 2 && !report.all_succeeded() && values == std::vector<std::int64_t>{42,42},
              "Linked rules query lost prefix or SQLite conversions");
        rejects([&] { db.query_rows({"SELECT * FROM Rows",{}}, [](auto &) {}, 2); });
        rejects([&] { db.query_rows({"",{}}, [](auto &) {}); });
        rejects([&] { db.query_rows({"SELECT * FROM Rows",{}}, [](auto &) { throw std::runtime_error("visitor"); }); });
    }
}
