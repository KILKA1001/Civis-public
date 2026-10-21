#pragma once
void custom_mod_options_tests() {
    RulesDatabase db;
    db.apply_sql("CREATE TABLE CustomModOptions(Name,Value,DbUpdates);"
                 "CREATE TABLE CustomModPostDefines(Name,Type,\"Table\");"
                 "CREATE TABLE CustomModDbUpdates(Name,Value);"
                 "CREATE TABLE Techs(ID,Type);"
                 "INSERT INTO Techs(rowid,ID,Type) VALUES(4294967297,99,'T');"
                 "INSERT INTO CustomModPostDefines VALUES('TECH','T','Techs');"
                 "INSERT INTO CustomModOptions VALUES('ONE',1,0),('TWO',2,0),('NEG',-1,0),"
                 "('DIRECT',1,1),('BOTH',1,1),('SQL_ONLY',1,1),('XML_ONLY',1,1),"
                 "('ZERO',0,1),('DUP',1,0),('DUP',0,0),('MISSING',1,1);"
                 "INSERT INTO CustomModDbUpdates VALUES('DIRECT',1),('BOTH_SQL',1),('BOTH_XML',1),"
                 "('SQL_ONLY_SQL',1),('XML_ONLY_XML',1),('MISSING',2);");
    auto options = load_custom_mod_options(db);
    check(options.all_succeeded() && options.prefetch.transaction_released && options.query.rows == 12 &&
          options.entries.size() == 11 && options.rejected_updates == 3, "CustomMods cache traversal/update gating");
    check(options.value("TECH") == 4294967297LL && options.prefetch.post_defines[0].applied &&
          !options.prefetch.strategic_resources_applied && !options.prefetch.luxury_resources_applied,
          "CustomMods postdefine must use wide ROWID and never update resources");
    check(options.enabled("ONE") && !options.enabled("TWO") && !options.enabled("NEG") &&
          !options.enabled("DUP") && !options.enabled("ABSENT") && options.value("NEG") == -1,
          "CustomMods bool cache requires exactly one, with last duplicate winning");
    check(options.enabled("DIRECT") && options.enabled("BOTH") && !options.enabled("SQL_ONLY") &&
          !options.enabled("XML_ONLY") && !options.enabled("MISSING") && !options.enabled("ZERO"),
          "CustomMods evidence must be direct or both SQL and XML");
    check(options.entries.at("MISSING").authored_value == 1 && options.entries.at("MISSING").value == 0,
          "CustomMods retains authored value for diagnosis");
    db.apply_sql("INSERT INTO CustomModOptions VALUES('BAD''NAME',1,1),(NULL,1,0);"
                 "INSERT INTO CustomModPostDefines VALUES('BAD','T','Absent'),('AFTER','T','Techs');");
    options = load_custom_mod_options(db);
    check(!options.all_succeeded() && !options.prefetch.all_succeeded && options.value("AFTER") == 4294967297LL &&
          options.errors == 2 && !options.enabled("BAD'NAME"), "CustomMods SQL/name failures remain diagnosed; later rows continue");
    rejects([&] { load_custom_mod_options(db, {1,16,{}}); });
    RulesDatabase absent;
    options = load_custom_mod_options(absent);
    check(!options.all_succeeded() && !options.query.error.empty() && !options.prefetch.all_succeeded,
          "Missing CustomMods schema masqueraded as all disabled");
    RulesDatabase minimal;
    minimal.apply_sql("CREATE TABLE CustomModOptions(Name,Value); CREATE TABLE CustomModPostDefines(Name,Type,\"Table\");"
                      "INSERT INTO CustomModOptions VALUES('NO_COLUMN',1),('MISSING_UPDATES',0);");
    options = load_custom_mod_options(minimal);
    check(options.all_succeeded() && options.enabled("NO_COLUMN"), "Absent DbUpdates column has native GetInt zero");
    minimal.apply_sql("ALTER TABLE CustomModOptions ADD COLUMN DbUpdates; UPDATE CustomModOptions SET DbUpdates=1;");
    options = load_custom_mod_options(minimal);
    check(!options.all_succeeded() && options.errors == 1 && options.rejected_updates == 1 && !options.enabled("NO_COLUMN"),
          "Missing evidence table must disable nonzero option without querying zero option");
    // Postdefines use the same source transaction failure semantics as gameplay
    // postprocessing (covered separately), including trigger-driven rollback.
    RulesDatabase rollback;
    rollback.apply_sql("CREATE TABLE CustomModOptions(Name PRIMARY KEY,Value,DbUpdates DEFAULT 0);"
                       "CREATE TABLE CustomModPostDefines(Name,Type,\"Table\"); CREATE TABLE T(Type);"
                       "INSERT INTO T VALUES('X'); INSERT INTO CustomModPostDefines VALUES('A','X','T'),('B','X','T');"
                       "CREATE TRIGGER Fail BEFORE INSERT ON CustomModOptions WHEN NEW.Name='B' BEGIN SELECT RAISE(ROLLBACK,'stop'); END;");
    options = load_custom_mod_options(rollback);
    check(!options.all_succeeded() && options.entries.empty() && !options.prefetch.transaction_released,
          "CustomMods trigger rollback must not retain an earlier option");
}
