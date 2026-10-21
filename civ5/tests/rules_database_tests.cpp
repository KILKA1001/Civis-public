#include "civ5/content/game_data_xml.hpp"
#include "civ5/content/rules_database.hpp"
#include "civ5/content/technology_rules.hpp"
#include "civ5/runtime/team_technologies.hpp"
#include "civ5/saves/vp_game_trade_profile.hpp"
#include "civ5/saves/vp_player_strategy_profile.hpp"
#include "civ5/runtime/vp_team_technologies_profile.hpp"
#include "civ5/runtime/research.hpp"
#include "civ5/runtime/vp_research_cost.hpp"
#include "civ5/runtime/vp_research_selection.hpp"
#include "civ5/runtime/vp_research_cost_profile.hpp"
#include "civ5/runtime/research_context.hpp"
#include "civ5/runtime/vp_attack_operations.hpp"
#include "civ5/runtime/vp_army_construction.hpp"
#include "civ5/runtime/vp_city_construction_intrigue.hpp"
#include "civ5/runtime/vp_diplomacy_intrigue.hpp"
#include "civ5/runtime/vp_travelling_turn.hpp"
#include "civ5/runtime/player_research.hpp"
#include "civ5/runtime/vp_player_research_profile.hpp"
#include "civ5/runtime/research_progress.hpp"
#include "civ5/runtime/city_yields.hpp"
#include "civ5/runtime/vp_city_yield_cache_profile.hpp"
#include "civ5/runtime/production_progress.hpp"
#include "civ5/runtime/production_checkpoint.hpp"
#include "civ5/runtime/bnw_unhappiness.hpp"
#include "civ5/runtime/maintenance.hpp"
#include "civ5/runtime/route_economy.hpp"
#include "civ5/runtime/vp_route_economy_profile.hpp"
#include "civ5/runtime/plot_water.hpp"
#include "civ5/runtime/plot_movement.hpp"
#include "civ5/runtime/vp_plot_movement_profile.hpp"
#include "civ5/runtime/vp_plot_passability_profile.hpp"
#include "civ5/runtime/bnw_nature_yield.hpp"
#include "civ5/runtime/vp_nature_yield.hpp"
#include "civ5/runtime/vp_unit_yield_profile.hpp"
#include "civ5/runtime/vp_plot_religion.hpp"
#include "civ5/runtime/vp_religion_beliefs_profile.hpp"
#include "civ5/runtime/vp_player_plot_yield.hpp"
#include "civ5/runtime/vp_player_accomplishments_profile.hpp"
#include "civ5/runtime/vp_plot_yield.hpp"
#include "civ5/runtime/vp_city_food.hpp"
#include "civ5/runtime/vp_city_growth.hpp"
#include "civ5/runtime/vp_unit_supply.hpp"
#include "civ5/runtime/vp_great_works.hpp"
#include "civ5/runtime/vp_city_building_yields.hpp"
#include "civ5/runtime/vp_city_religion_minor_yields.hpp"
#include "civ5/runtime/vp_city_trait_yields.hpp"
#include "civ5/runtime/vp_city_happiness.hpp"
#include "civ5/runtime/vp_city_base_yields.hpp"
#include "civ5/runtime/vp_gathering_intel_turn.hpp"
#include "civ5/runtime/vp_espionage_player_turn.hpp"
#include "civ5/runtime/vp_schmooze_turn.hpp"
#include "civ5/runtime/vp_corporations.hpp"
#include "civ5/saves/vp_player_knowledge_profile.hpp"
#include "civ5/runtime/vp_city_production.hpp"
#include "civ5/runtime/vp_building_production.hpp"
#include "civ5/saves/vp_city_production_profile.hpp"
#include "civ5/runtime/vp_unit_traits.hpp"
#include "civ5/runtime/vp_player_unit_production.hpp"
#include "civ5/runtime/vp_city_unit_production.hpp"
#include <algorithm>
#include <exception>
#include "civ5/runtime/improvement_yields.hpp"
#include "civ5/runtime/improvement_yield_calculation.hpp"
#include "civ5/runtime/vp_improvement_yield_profile.hpp"
#include "civ5/runtime/resource_visibility.hpp"
#include "civ5/runtime/vp_resource_visibility_profile.hpp"
#include "civ5/runtime/treasury.hpp"
#include "civ5/runtime/vp_maintenance_profile.hpp"
#include "civ5/runtime/vp_treasury_income_profile.hpp"
#include "civ5/runtime/vp_treasury_minor_profile.hpp"
#include "civ5/runtime/vp_treasury_religion_profile.hpp"
#include "civ5/runtime/science.hpp"
#include "civ5/runtime/vp_science_profile.hpp"
#include "civ5/runtime/treasury_turn.hpp"
#include "civ5/runtime/lua_treasury_services.hpp"
#include "civ5/runtime/vp_policy_branch_profile.hpp"
#include "civ5/runtime/research_turn.hpp"
#include "civ5/runtime/player_economy_research_stage.hpp"
#include "civ5/runtime/vp_vassal_tax_turn.hpp"
#include "civ5/runtime/vp_culture_history_turn.hpp"
#include "civ5/runtime/vp_culture_income.hpp"
#include "civ5/runtime/vp_policy_cost.hpp"
#include "civ5/runtime/vp_policy_phase.hpp"
#include "civ5/runtime/vp_espionage_turn.hpp"
#include "civ5/runtime/vp_city_espionage.hpp"
#include "civ5/runtime/vp_espionage_checkpoint.hpp"
#include "civ5/runtime/vp_network_points.hpp"
#include "civ5/runtime/vp_network_point_binding.hpp"
#include "civ5/runtime/vp_anarchy_turn.hpp"
#include "civ5/runtime/research_selection.hpp"
#include "civ5/runtime/research_ai_weights.hpp"
#include "civ5/runtime/vp_research_ai_weights.hpp"
#include "civ5/runtime/research_ai_candidates.hpp"
#include "civ5/runtime/vp_research_ai_candidates.hpp"
#include "civ5/runtime/vp_research_ai_choice.hpp"
#include "civ5/runtime/lua_research_services.hpp"
#include "civ5/content/lua_assets.hpp"
#include "civ5/content/mod_script_registry.hpp"
#include "civ5/runtime/lua_gameplay_services.hpp"
#include "civ5/runtime/script_ui_control_graph.hpp"
#include "civ5/runtime/lua_map_services.hpp"
#include "civ5/runtime/pregame_roster.hpp"
#include "civ5/runtime/saved_value_fallback.hpp"
#include "civ5/runtime/lua_content_services.hpp"
#include "civ5/runtime/lua_gamecore_enums.hpp"
#include "civ5/runtime/lua_locale_services.hpp"
#include "../runtime/treasury_report.hpp"
#include "../runtime/vp_unit_supply_report.hpp"
#include "../runtime/research_probe.hpp"
#include "civ5/runtime/wide_arithmetic.hpp"
#include <bit>
#include <chrono>
#include <cmath>
#include <cfenv>
#include <limits>
#include <array>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <source_location>
#include <stdexcept>
#include "../third_party/sqlite/sqlite3.h"

using namespace civ5::content;
namespace {
unsigned checks = 0;
void check(bool value, const char *message) {
    ++checks;
    if (!value)
        throw std::runtime_error(message);
}
template <class F> void rejects(F action, std::source_location at = std::source_location::current()) {
    bool rejected = false;
    try {
        action();
    } catch (const std::exception &) {
        rejected = true;
    }
    ++checks;
    if (!rejected)
        throw std::runtime_error("Expected rejection at " + std::string(at.file_name()) + ":" + std::to_string(at.line()));
}
XmlNode xml(const std::string &text) {
    static const auto root = std::filesystem::temp_directory_path() /
                             ("civis-rules-fixtures-" +
                              std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    std::filesystem::create_directories(root);
    static unsigned index = 0;
    const auto path = root / (std::to_string(++index) + ".xml");
    {
        std::ofstream out(path, std::ios::binary);
        out << text;
        if (!out)
            throw std::runtime_error("Fixture write");
    }
    return read_xml(path);
}
DatabaseValue scalar(const RulesDatabase &db, const std::string &sql) {
    DatabaseValue value;
    const auto rows = db.query({sql, {}}, [&](auto row) {
        check(row.size() == 1, "Expected scalar column");
        value = row[0];
    });
    check(rows == 1, "Expected scalar row");
    return value;
}
void equals(const RulesDatabase &db, const std::string &sql, DatabaseValue expected) {
    check(scalar(db, sql) == expected, "Query result mismatch");
}
void compound_select_stack_tests() {
    // Real VP SQL exhausted the Windows default 1 MiB stack in instrumented
    // SQLite's recursive multiSelect/sqlite3Select compiler. Exercise its full
    // supported compound-query depth without rewriting or truncating mod SQL.
    RulesDatabase db;
    std::string sql = "SELECT 1";
    for (int i = 1; i < 500; ++i) sql += " UNION ALL SELECT 1";
    equals(db, "SELECT count(*) FROM (" + sql + ")", std::int64_t{500});
    rejects([&] { db.apply_sql(sql + " UNION ALL SELECT 1"); });
}
void gameplay_comment_tests() {
    const std::string text = "<?xml version='1.0'?><?note <!-- untouched -- PI --> ?>"
                             "<!------- banner -- dashes -------><GameData>"
                             "<Rows><!-- <Delete/> -- not an action -->"
                             "<Row><ID>1</ID><Value><![CDATA[<!--literal--body-->]]></Value></Row>"
                             "<Row ID='2' Value='&lt;!--attribute--literal--&gt;'/></Rows>"
                             "<!--------- tail ---------></GameData>";
    rejects([&] { read_xml_metadata(text); });
    RulesDatabase db;
    db.apply_sql("CREATE TABLE Rows(ID INTEGER PRIMARY KEY, Value TEXT);");
    db.apply_mod_xml(read_game_data_xml_text(text));
    equals(db, "SELECT count(*) FROM Rows", std::int64_t{2});
    equals(db, "SELECT Value FROM Rows WHERE ID=1", std::string("<!--literal--body-->"));
    equals(db, "SELECT Value FROM Rows WHERE ID=2", std::string("<!--attribute--literal-->"));
    // The first terminator wins; a second opener inside the body is not nesting.
    const auto first = read_game_data_xml_text("<GameData><!-- a <!-- b --> <Rows/></GameData>");
    check(first.children.size() == 1 && first.children[0].name == "Rows", "Comment nesting invented");
    for (const auto *broken :
         {"<GameData><!-- unterminated</GameData>", "<GameData><![CDATA[<!-- not a comment --></GameData>",
          "<?note <!-- unfinished><GameData/>", "<GameData><Rows/>",
          "<GameData><Rows x='<!--a--b-->'/></GameData>",
          "<!DOCTYPE GameData [<!ENTITY x SYSTEM 'file:///absent'>]><GameData>&x;</GameData>",
          "<GameData><Rows x='1' x='2'/></GameData>"})
        rejects([&] { read_game_data_xml_text(broken); });
    const std::string nul = std::string("<GameData><!--") + '\0' + "--></GameData>";
    rejects([&] { read_game_data_xml_text(nul); });
    const auto many =
        read_game_data_xml_text("<GameData><!--" + std::string(1024 * 1024, '-') + "--><Rows/></GameData>");
    check(many.children.size() == 1, "Large comment was treated as gameplay or nesting");
    const auto path = std::filesystem::temp_directory_path() /
                      ("civis-comment-" +
                       std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()) + ".xml");
    {
        std::ofstream out(path, std::ios::binary);
        out << text;
    }
    check(read_game_data_xml(path).children.size() == 1, "File gameplay reader differs from memory reader");
    rejects([&] { read_xml(path); });
}
void gameplay_markup_tests() {
    RulesDatabase db;
    db.apply_sql("CREATE TABLE Rows(ID INTEGER PRIMARY KEY, Value TEXT);");
    const std::string source =
        "<?xml version='1.0'?><?note </wrong> ?>"
        "<GameData>root text<Rows>/*<Row><ID>4294967297</Other>"
        "<Value><![CDATA[</literal>]]></Value></WrongRow>*/"
        "<!- - not a field --><Row ID='2' Value='&lt;/literal&gt;'/></WrongTable></WrongRoot>";
    rejects([&] { read_xml_metadata(source); });
    db.apply_mod_xml(read_game_data_xml_text(source));
    equals(db, "SELECT count(*) FROM Rows", std::int64_t{2});
    equals(db, "SELECT Value FROM Rows WHERE ID=4294967297", std::string("</literal>"));
    equals(db, "SELECT Value FROM Rows WHERE ID=2", std::string("</literal>"));
    // A closing token ends the current element, even when it names an ancestor.
    const auto nested = read_game_data_xml_text("<GameData><Rows></GameData></Rows>");
    check(nested.name == "GameData" && nested.children.size() == 1 && nested.children[0].name == "Rows",
          "Closing name was matched to an ancestor");
    // Unknown <! markup ends at the first >, including one inside a quote or
    // an apparent inner tag. It is not a comment, quoted string, or nested XML.
    db.apply_mod_xml(read_game_data_xml_text(
        "<GameData><!discard '<Rows/><Rows><Row ID='3' Value='present'/></Rows></GameData>"));
    equals(db, "SELECT Value FROM Rows WHERE ID=3", std::string("present"));
    db.apply_mod_xml(
        read_game_data_xml_text("<GameData><Rows><Row ID='4'><Value><!x></Value></Row></Rows></GameData>"));
    equals(db, "SELECT Value FROM Rows WHERE ID=4", std::monostate{});
    // Removing container character data must not hide actual unknown actions.
    rejects([&] {
        db.apply_mod_xml(read_game_data_xml_text("<GameData><Rows>/*<Upsert ID='4'/>*/</Rows></GameData>"));
    });
    for (const auto *broken : {"<GameData></Rows extra>", "<GameData></Rows/>", "<GameData></>",
                               "<GameData></42>", "<GameData></Bad<name>", "<GameData><!- - unfinished",
                               "<GameData/><Rows/>", "<GameData></Rows></Extra>"})
        rejects([&] { read_game_data_xml_text(broken); });
    const std::string nul = std::string("<GameData><!unknown ") + '\0' + "></GameData>";
    rejects([&] { read_game_data_xml_text(nul); });
    for (std::size_t length = 0; length < source.size(); ++length)
        rejects([&] { read_game_data_xml_text(source.substr(0, length)); });
    const std::string long_name = "Column" + std::string(2048, 'x');
    const auto long_node = read_game_data_xml_text("<GameData><" + long_name + ">value</X></Other>");
    check(long_node.children[0].name == long_name && long_node.children[0].text == "value",
          "Closing normalization truncated a long element name");
    std::string utf16 = "\xff\xfe";
    for (const char c : std::string("<GameData><Rows/></GameData>")) {
        utf16 += c;
        utf16 += '\0';
    }
    check(read_game_data_xml_text(utf16).children.size() == 1, "UTF-16 strict fallback changed");
}
void id_remap_tests() {
    {
        RulesDatabase db;
        db.apply_sql("CREATE TABLE Remapper(name TEXT);"
                     "CREATE TABLE Units(ID INTEGER PRIMARY KEY AUTOINCREMENT, Type TEXT);"
                     "INSERT INTO Units VALUES(2,'A'),(9,'B'),(4294967298,'C');"
                     "CREATE TABLE Huge(ID INTEGER PRIMARY KEY, Type TEXT);"
                     "INSERT INTO Huge VALUES(9223372036854775807,'H');"
                     "CREATE TABLE Unlisted(ID INTEGER PRIMARY KEY); INSERT INTO Unlisted VALUES(77);"
                     "INSERT INTO Remapper VALUES('Units'),('Huge'),('Units');");
        const auto report = db.remap_vp_ids();
        check(report.all_succeeded && report.tables.size() == 3, "Remapper list order/duplicates");
        check(report.tables[0].completed_updates == 3 && report.tables[1].completed_updates == 1 &&
                  report.tables[2].attempted_updates == 0,
              "Remapper didn't use original ID order or repeat the selected entry");
        equals(db, "SELECT group_concat(ID || ':' || Type, ',') FROM (SELECT * FROM Units ORDER BY ID)",
               std::string("0:A,1:B,2:C"));
        equals(db, "SELECT ID FROM Huge", std::int64_t{0});
        equals(db, "SELECT ID FROM Unlisted", std::int64_t{77});
        db.apply_sql("INSERT INTO Units(Type) VALUES('D');");
        equals(db, "SELECT ID FROM Units WHERE Type='D'", std::int64_t{4294967299LL});
        // UPDATE must not reset sqlite_sequence or invent a new compact insert policy.
        rejects([&] { db.apply_sql("PRAGMA table_info(Units);"); });
        rejects([&] { db.remap_vp_ids({0, 100}); });
        rejects([&] { db.remap_vp_ids({100, 1}); });
        equals(db, "SELECT ID FROM Units WHERE Type='D'", std::int64_t{4294967299LL});
    }
    {
        RulesDatabase db;
        db.apply_sql("CREATE TABLE Remapper(name);"
                     "CREATE TABLE Negative(ID INTEGER PRIMARY KEY); INSERT INTO Negative VALUES(-1),(4);"
                     "CREATE TABLE Mixed(ID PRIMARY KEY); INSERT INTO Mixed VALUES(5),('-1');"
                     "CREATE TABLE Lower(id INTEGER PRIMARY KEY); INSERT INTO Lower VALUES(7);"
                     "CREATE TABLE NonPk(ID INTEGER); INSERT INTO NonPk VALUES(8);"
                     "CREATE TABLE Composite(ID INTEGER, S INTEGER, PRIMARY KEY(ID,S));"
                     "INSERT INTO Composite VALUES(5,1),(5,2);"
                     "CREATE TABLE Second(S INTEGER, ID INTEGER, PRIMARY KEY(S,ID));"
                     "INSERT INTO Second VALUES(1,6);"
                     "CREATE VIEW AView AS SELECT ID FROM Negative;"
                     "CREATE TABLE Empty(X);"
                     "INSERT INTO Remapper VALUES('Negative'),('Mixed'),('Lower'),('NonPk'),"
                     "('Composite'),('Second'),('AView'),('Empty');");
        const auto report = db.remap_vp_ids();
        check(report.all_succeeded && report.tables.size() == 8, "Eligibility fixture failed");
        check(report.tables[0].eligible && report.tables[0].negative_id_seen &&
                  report.tables[0].collected_ids == 0,
              "Negative integer ID didn't stop collection");
        check(report.tables[1].negative_id_seen && report.tables[1].completed_updates == 1,
              "Native collected prefix was discarded after a later negative conversion");
        check(!report.tables[2].eligible && !report.tables[3].eligible && report.tables[4].eligible &&
                  !report.tables[5].eligible && !report.tables[6].eligible && report.tables[7].rows == 0,
              "Remapper ID name/PK position predicate differs");
        equals(db, "SELECT ID FROM Negative WHERE ID=4", std::int64_t{4});
        equals(db, "SELECT ID FROM Mixed WHERE typeof(ID)='integer'", std::int64_t{0});
        equals(db, "SELECT count(*) FROM Composite WHERE ID=0", std::int64_t{2});
        check(report.tables[4].attempted_updates == 2 && report.tables[4].completed_updates == 2,
              "Native duplicate-key UPDATE attempts were collapsed");
    }
    for (const auto *mode : {"ABORT", "FAIL", "ROLLBACK"}) {
        RulesDatabase db;
        db.apply_sql(
            "CREATE TABLE Remapper(name);"
            "CREATE TABLE Items(ID INTEGER PRIMARY KEY); INSERT INTO Items VALUES(1),(5),(9);"
            "CREATE TABLE Later(ID INTEGER PRIMARY KEY); INSERT INTO Later VALUES(7);"
            "CREATE TABLE Attempts(ID);"
            "INSERT INTO Remapper VALUES('Items'),('Missing'),('Later');"
            "CREATE TRIGGER Observe BEFORE UPDATE ON Items BEGIN INSERT INTO Attempts VALUES(OLD.ID);"
            "SELECT CASE WHEN OLD.ID=5 THEN RAISE(" +
            std::string(mode) + ",'fixture failure') END; END;");
        const auto report = db.remap_vp_ids();
        check(!report.all_succeeded && report.tables.size() == 3, "Remap error didn't continue");
        check(report.tables[0].attempted_updates == 3 && report.tables[0].completed_updates == 2 &&
                  report.tables[0].errors.size() == 1 && report.tables[1].errors.size() == 1 &&
                  report.tables[2].completed_updates == 1,
              "Per-update/per-table failure reporting differs");
        equals(db, "SELECT group_concat(ID, ',') FROM (SELECT ID FROM Items ORDER BY ID)",
               std::string("0,2,5"));
        equals(db, "SELECT ID FROM Later", std::int64_t{0});
        equals(db, "SELECT count(*) FROM Attempts", std::int64_t{std::string(mode) == "FAIL" ? 3 : 2});
        db.apply_sql("INSERT INTO Later VALUES(12);"); // No transaction leaked by remapping errors.
    }
    {
        RulesDatabase db;
        check(!db.remap_vp_ids().all_succeeded, "Absent Remapper was treated as completed postprocessing");
        db.apply_sql("CREATE TABLE Remapper(name); INSERT INTO Remapper VALUES(NULL),('');");
        check(db.remap_vp_ids().tables.size() == 2, "Invalid table names stopped the list");
        const auto name = "Long table " + std::string(1024, 'x');
        db.apply_sql("CREATE TABLE \"" + name + "\"(ID INTEGER PRIMARY KEY); INSERT INTO \"" + name +
                     "\" VALUES(4294967297);");
        db.apply_statements(std::vector<DatabaseStatement>{{"DELETE FROM Remapper", {}},
                                                           {"INSERT INTO Remapper VALUES(?)", {name}}});
        check(db.remap_vp_ids().all_succeeded, "Original 512-byte identifier buffer retained");
        equals(db, "SELECT ID FROM \"" + name + "\"", std::int64_t{0});
    }
    {
        RulesDatabase db;
        db.apply_sql("CREATE TABLE Remapper(name); INSERT INTO Remapper VALUES('Items');"
                     "CREATE TABLE Items(ID INTEGER PRIMARY KEY); INSERT INTO Items VALUES(1),(9);"
                     "CREATE TRIGGER AddRow AFTER UPDATE ON Items WHEN OLD.ID=1 BEGIN "
                     "INSERT INTO Items VALUES(5); END;");
        check(db.remap_vp_ids().tables[0].collected_ids == 2, "IDs weren't collected before updating");
        equals(db, "SELECT group_concat(ID, ',') FROM (SELECT ID FROM Items ORDER BY ID)",
               std::string("0,1,5"));
    }
}
void run() {
    id_remap_tests();
    gameplay_comment_tests();
    gameplay_markup_tests();
    check(game_data_value("4294967296") == DatabaseValue(std::int64_t{4294967296LL}),
          "Integer narrowed to 32 bits");
    check(game_data_value("9223372036854775807") == DatabaseValue(INT64_MAX), "Signed64 max");
    rejects([] { game_data_value("9223372036854775808"); });
    check(game_data_value("-7") == DatabaseValue(std::string("-7")), "Native negative lexical branch");
    check(game_data_value("null") == DatabaseValue(std::string("null")), "Native NULL case");
    check(std::holds_alternative<std::monostate>(game_data_value("NULLtail")), "Native NULL prefix");
    check(std::holds_alternative<std::monostate>(game_data_value("")), "Native empty NULL");
    check(game_data_value("TrUetail") == DatabaseValue(std::int64_t{1}), "Native bool prefix");
    check(game_data_value("False") == DatabaseValue(std::int64_t{0}), "Native bool");
    check(game_data_value(".5") == DatabaseValue(0.5), "Native decimal");
    check(game_data_value("5.") == DatabaseValue(std::string("5.")), "Native trailing dot");
    check(game_data_value("1e2") == DatabaseValue(std::string("1e2")), "Native exponent text");
    check(game_data_value("0.1") == DatabaseValue(static_cast<double>(0.1f)), "Native float rounding");
    RulesDatabase db;
    check(RulesDatabase::sqlite_version() == "3.53.4", "Pinned SQLite version");
    db.apply_xml(xml(
        R"(<GameData><Table name="Rows"><Column name="ID" type="integer" primarykey="true"/><Column name="Value" type="text" default="it's base"/><Column name="Flag" type="boolean" default="true"/></Table><Rows><Row ID="4294967296"/><Row><ID>1</ID><Value>initial</Value></Row></Rows></GameData>)"));
    equals(db, "SELECT ID FROM Rows WHERE Value='it''s base'", std::int64_t{4294967296LL});
    equals(db, "SELECT Flag FROM Rows WHERE ID=1", std::int64_t{1});
    db.apply_xml(xml(
        R"(<GameData><Index name="RowsIndex" table="Rows" unique="true"><Column name="Value" sort="ASC"/><Column name="Flag" sort="DESC"/></Index></GameData>)"));
    equals(db,
           "SELECT count(*) FROM sqlite_master WHERE type='index' AND "
           "name='RowsIndex'",
           std::int64_t{1});
    rejects([&] { db.apply_xml(xml(R"(<GameData><Rows><Row ID="4" Value="initial"/></Rows></GameData>)")); });
    db.apply_xml(xml(
        R"(<GameData><Rows><InsertOrIgnore ID="1" Value="ignored"/><Replace ID="4294967296" Value="replacement"/><Update><Where ID="1"/><Set Value="modded"/></Update></Rows></GameData>)"));
    equals(db, "SELECT Value FROM Rows WHERE ID=1", std::string("modded"));
    equals(db, "SELECT Value FROM Rows WHERE ID=4294967296", std::string("replacement"));
    db.apply_xml(xml(
        R"(<GameData><Rows><Update><Set Value="preserved"/><Where ID="1"/></Update><InsertOrAbort ID="1"/></Rows></GameData>)"));
    equals(db, "SELECT Value FROM Rows WHERE ID=1", std::string("preserved"));
    rejects([&] {
        db.apply_xml(xml(
            R"(<GameData><Table name="RolledBack"><Column name="ID" type="integer"/></Table><Rows><Row ID="1"/></Rows></GameData>)"));
    });
    equals(db, "SELECT count(*) FROM sqlite_master WHERE name='RolledBack'", std::int64_t{0});
    db.apply_xml(xml(R"(<GameData><Rows><Delete ID="1"/></Rows></GameData>)"));
    equals(db, "SELECT count(*) FROM Rows", std::int64_t{1});
    // Actual SQLite grammar, including semicolons inside strings and trigger
    // bodies.
    db.apply_sql("CREATE TABLE Audit(Value); CREATE TRIGGER events AFTER UPDATE ON Rows "
                 "BEGIN INSERT INTO Audit VALUES('first;second'); INSERT INTO Audit "
                 "VALUES(new.ID); END; UPDATE Rows SET Value='triggered'; CREATE VIEW "
                 "Visible AS SELECT * FROM Audit;");
    equals(db, "SELECT count(*) FROM Visible", std::int64_t{2});
    equals(db, "SELECT Value FROM Audit WHERE typeof(Value)='integer'", std::int64_t{4294967296LL});
    rejects([&] { db.apply_sql("INSERT INTO Audit VALUES('rollback'); SELECT * FROM Absent;"); });
    equals(db, "SELECT count(*) FROM Audit", std::int64_t{2});
    rejects([&] { db.apply_sql("INSERT INTO Audit VALUES('rollback'); COMMIT;"); });
    equals(db, "SELECT count(*) FROM Audit", std::int64_t{2});
    rejects([&] { db.apply_sql("ATTACH DATABASE 'must-not-create.db' AS extra"); });
    rejects([&] { db.apply_sql("PRAGMA writable_schema=ON"); });
    rejects([&] { db.apply_sql("SELECT load_extension('must-not-load.dll')"); });
    db.apply_sql("CREATE TRIGGER abort_insert BEFORE INSERT ON Audit WHEN "
                 "new.Value='abort' BEGIN SELECT "
                 "RAISE(ROLLBACK,'fixture rollback'); END;");
    rejects([&] {
        db.apply_sql("INSERT INTO Audit VALUES('not committed'); INSERT INTO Audit "
                     "VALUES('abort');");
    });
    equals(db, "SELECT count(*) FROM Audit", std::int64_t{2});
    rejects([&] { db.apply_sql("INSERT INTO Audit VALUES(?)"); });
    rejects([&] { db.query({"DELETE FROM Audit", {}}, [](auto) {}); });
    rejects([&] { db.query({"SELECT 1; SELECT 2", {}}, [](auto) {}); });
    rejects([&] { db.query({"SELECT ?", {}}, [](auto) {}); });
    rejects([&] { db.apply_sql(std::string("SELECT 1;\0SELECT 2;", 19)); });
    db.apply_statements(std::vector<DatabaseStatement>{
        {"INSERT INTO Audit VALUES(?)", {std::string("x'); DROP TABLE Rows;--")}}});
    equals(db, "SELECT count(*) FROM Rows", std::int64_t{1});
    // Queries stream, and do not retain earlier rows inside the database facade.
    const auto streamed = db.query(
        {"WITH RECURSIVE n(x) AS (VALUES(4294967296) UNION ALL SELECT "
         "x+1 FROM n WHERE x<4294977295) SELECT x FROM n",
         {}},
        [&](auto row) { check(std::get<std::int64_t>(row[0]) >= 4294967296LL, "Streamed ID narrowed"); });
    check(streamed == 10000, "Streamed row count");
    rejects([&] { db.apply_xml(xml(R"(<GameData><Rows><Row ID="1"><ID>2</ID></Row></Rows></GameData>)")); });
    rejects([&] { db.apply_xml(xml(R"(<GameData><Rows><Upsert ID="1"/></Rows></GameData>)")); });
    rejects([&] {
        db.apply_xml(
            xml(R"(<GameData><Rows><Update><Set Value="a"/><Set Value="b"/></Update></Rows></GameData>)"));
    });
    rejects([&] { db.apply_xml(xml(R"(<GameData><Index name="NotYetRecovered"/></GameData>)")); });
    rejects([&] { db.apply_xml(xml(R"(<GameData><DeleteMissingReferences table="Rows"/></GameData>)")); });
    rejects([&] {
        db.apply_xml(xml(
            R"(<GameData><Table name="Invalid"><Column name="ID" type="integer" custom="ignored"/></Table></GameData>)"));
    });
    db.apply_xml(xml(R"(<GameData><Rows><Delete/></Rows></GameData>)"));
    equals(db, "SELECT count(*) FROM Rows", std::int64_t{0});
    db.apply_xml(
        xml(R"(<GameData><Rows><Row><ID>7</ID>><Value>preserve &gt; leaf</Value></Row></Rows></GameData>)"));
    equals(db, "SELECT Value FROM Rows WHERE ID=7", std::string("preserve > leaf"));
}
void try_inserts() {
    // Same operation serves vanilla/DLC bootstrap and mod XML. An optional
    // insert failing a constraint must preserve both preceding and later rows.
    for (const bool mod : {false, true}) {
        RulesDatabase db;
        db.apply_sql("CREATE TABLE Attempts(ID INTEGER PRIMARY KEY, Value TEXT NOT NULL UNIQUE "
                     "CHECK(Value <> 'forbidden')); INSERT INTO Attempts VALUES(1,'base');");
        const auto document = xml("<GameData><Attempts><Row ID='4294967296' Value='before'/>"
                                  "<InsertOrAbort><ID>1</ID><Value>duplicate key</Value></InsertOrAbort>"
                                  "<InsertOrAbort ID='2' Value='base'/><InsertOrAbort ID='3'/>"
                                  "<InsertOrAbort ID='4' Value='forbidden'/><Row ID='5' Value='after'/>"
                                  "</Attempts></GameData>");
        if (mod)
            db.apply_mod_xml(document);
        else
            db.apply_xml(document);
        equals(db, "SELECT count(*) FROM Attempts", std::int64_t{3});
        equals(db, "SELECT Value FROM Attempts WHERE ID=1", std::string("base"));
        equals(db, "SELECT ID FROM Attempts WHERE Value='before'", std::int64_t{4294967296LL});
        equals(db, "SELECT Value FROM Attempts WHERE ID=5", std::string("after"));
        // A subsequent mandatory Row still fails and rolls back the XML.
        rejects([&] {
            db.apply_mod_xml(xml("<GameData><Attempts><Row ID='6' Value='rollback'/>"
                                 "<InsertOrAbort ID='1' Value='skip'/><Row ID='1' Value='error'/>"
                                 "</Attempts></GameData>"));
        });
        equals(db, "SELECT count(*) FROM Attempts WHERE ID=6", std::int64_t{0});
        // SQL OR ABORT is not the XML TryExecute path.
        const auto result = db.apply_mod_sql("INSERT OR ABORT INTO Attempts VALUES(1,'raw');"
                                             "INSERT INTO Attempts VALUES(7,'unreached');");
        check(!result.success && result.completed_statements == 0, "Raw SQL ABORT was swallowed");
        equals(db, "SELECT count(*) FROM Attempts WHERE ID=7", std::int64_t{0});
        // Prepare errors remain fatal, including in an optional insert.
        rejects([&] {
            db.apply_mod_xml(xml("<GameData><Attempts><Row ID='8' Value='rollback'/>"
                                 "<InsertOrAbort MissingColumn='1'/></Attempts></GameData>"));
        });
        equals(db, "SELECT count(*) FROM Attempts WHERE ID=8", std::int64_t{0});
    }
    for (const std::string mode : {"ABORT", "FAIL"}) {
        RulesDatabase db;
        db.apply_sql("CREATE TABLE Attempts(ID INTEGER PRIMARY KEY); CREATE TABLE Audit(ID);"
                     "CREATE TRIGGER attempted BEFORE INSERT ON Attempts BEGIN "
                     "INSERT INTO Audit VALUES(new.ID); SELECT CASE WHEN new.ID=2 THEN RAISE(" +
                     mode + ",'optional row') END; END;");
        db.apply_mod_xml(xml(
            "<GameData><Attempts><Row ID='1'/><InsertOrAbort ID='2'/><Row ID='3'/></Attempts></GameData>"));
        equals(db, "SELECT count(*) FROM Attempts", std::int64_t{2});
        equals(db, "SELECT sum(ID) FROM Attempts", std::int64_t{4});
        // RAISE(ABORT) undoes the statement; RAISE(FAIL) preserves its prefix.
        // Rewriting to OR IGNORE, or wrapping every attempt in a rollback,
        // cannot reproduce both behaviors.
        equals(db, "SELECT count(*) FROM Audit WHERE ID=2", std::int64_t{mode == "FAIL" ? 1 : 0});
        rejects([&] {
            db.apply_mod_xml(
                xml("<GameData><Attempts><Row ID='4'/><InsertOrIgnore ID='2'/></Attempts></GameData>"));
        });
        equals(db, "SELECT count(*) FROM Attempts WHERE ID=4", std::int64_t{0});
        equals(db, "SELECT count(*) FROM Audit WHERE ID=4", std::int64_t{0});
    }
    RulesDatabase db;
    db.apply_sql("CREATE TABLE Attempts(ID); CREATE TRIGGER execution_error BEFORE INSERT ON Attempts "
                 "WHEN new.ID=2 BEGIN SELECT abs(-9223372036854775808); END;");
    rejects([&] {
        db.apply_mod_xml(xml(
            "<GameData><Attempts><Row ID='1'/><InsertOrAbort ID='2'/><Row ID='3'/></Attempts></GameData>"));
    });
    equals(db, "SELECT count(*) FROM Attempts", std::int64_t{0});
}
void mod_actions() {
    RulesDatabase db;
    auto result = db.apply_mod_sql("CREATE TABLE Native(ID INTEGER PRIMARY KEY, V);"
                                   "INSERT INTO Native VALUES(4294967297, 'base');");
    check(result.success && result.completed_statements == 2 && result.release_error.empty() &&
              !result.transaction_active_after,
          "Native SQL success");
    result = db.apply_mod_sql("-- comments and empty statements\n ; ; SELECT ';'; "
                              "INSERT INTO Native VALUES(2,'prefix'); SELECT * FROM Absent; "
                              "INSERT INTO Native VALUES(3,'unreachable');");
    check(!result.success && result.completed_statements == 2 && !result.error.empty() &&
              result.error_code != 0 && result.release_error.empty(),
          "Prepare failure result");
    equals(db, "SELECT count(*) FROM Native WHERE ID=2", std::int64_t{1});
    equals(db, "SELECT count(*) FROM Native WHERE ID=3", std::int64_t{0});
    result = db.apply_mod_sql("INSERT INTO Native VALUES(4,'before abort');"
                              "INSERT OR ABORT INTO Native VALUES(5,'undone'),(2,'collision');"
                              "DELETE FROM Native;");
    check(!result.success && result.completed_statements == 1, "ABORT action result");
    equals(db, "SELECT count(*) FROM Native WHERE ID IN(4,5)", std::int64_t{1});
    result = db.apply_mod_sql("INSERT INTO Native VALUES(6,'before fail');"
                              "INSERT OR FAIL INTO Native VALUES(7,'retained'),(2,'collision');");
    check(!result.success && result.completed_statements == 1, "FAIL action result");
    equals(db, "SELECT count(*) FROM Native WHERE ID IN(6,7)", std::int64_t{2});
    result = db.apply_mod_sql("INSERT INTO Native VALUES(8,'rollback');"
                              "INSERT OR ROLLBACK INTO Native VALUES(2,'collision');"
                              "INSERT INTO Native VALUES(9,'unreachable');");
    check(!result.success && result.completed_statements == 1 && !result.release_error.empty() &&
              !result.transaction_active_after,
          "ROLLBACK release error lost or transaction leaked");
    equals(db, "SELECT count(*) FROM Native WHERE ID IN(8,9)", std::int64_t{0});
    result = db.apply_mod_sql("CREATE TRIGGER rollback_mod BEFORE INSERT ON Native WHEN new.ID=11 "
                              "BEGIN SELECT RAISE(ROLLBACK,'native trigger'); END;");
    check(result.success, "Trigger creation");
    result = db.apply_mod_sql("INSERT INTO Native VALUES(10,'rollback'); INSERT "
                              "INTO Native VALUES(11,'x');");
    check(!result.success && result.error.find("native trigger") != std::string::npos &&
              !result.release_error.empty(),
          "Trigger ROLLBACK semantics");
    equals(db, "SELECT count(*) FROM Native WHERE ID IN(10,11)", std::int64_t{0});
    result = db.apply_mod_sql("INSERT INTO Native VALUES(12,?);");
    check(result.success, "Native ExecuteMultiple must leave unbound parameters NULL");
    equals(db, "SELECT V IS NULL FROM Native WHERE ID=12", std::int64_t{1});
    result = db.apply_mod_sql("BEGIN; DELETE FROM Native;");
    check(!result.success && !result.transaction_active_after, "BEGIN inside native savepoint");
    equals(db, "SELECT V FROM Native WHERE ID=4294967297", std::string("base"));
    // A source can end the surrounding savepoint. EndTransaction's failed
    // RELEASE does not overwrite ExecuteMultiple's successful return value.
    result = db.apply_mod_sql("COMMIT; INSERT INTO Native VALUES(13,'committed');");
    check(result.success && !result.release_error.empty() && !result.transaction_active_after,
          "Native unconditional RELEASE after source COMMIT");
    equals(db, "SELECT V FROM Native WHERE ID=13", std::string("committed"));
    // The original uses the name _, including when source SQL uses that name.
    result = db.apply_mod_sql("SAVEPOINT _; INSERT INTO Native VALUES(14,'pending');");
    check(result.success && result.transaction_active_after, "Native nested savepoint name");
    db.apply_mod_xml(xml("<GameData><Native><Row ID='15' V='pending XML'/></Native></GameData>"));
    rejects(
        [&] { db.apply_mod_xml(xml("<GameData><Native><Row ID='16'/><Row ID='2'/></Native></GameData>")); });
    equals(db, "SELECT count(*) FROM Native WHERE ID IN(14,15,16)", std::int64_t{0});
    equals(db, "SELECT count(*) FROM Native WHERE ID=13", std::int64_t{1});
    result = db.apply_mod_sql("SELECT * FROM Absent;");
    check(!result.success && result.completed_statements == 0 && result.release_error.empty(),
          "Failed first statement left a transaction");
    check(db.apply_mod_sql("-- empty source\n;").success, "Empty native SQL");
    result = db.apply_mod_sql("ATTACH DATABASE 'must-not-create.db' AS extra;");
    check(!result.success && result.error_code == 23, "Native API bypassed isolated database authorizer");
    check(!db.apply_mod_sql("PRAGMA writable_schema=ON;").success, "Unported PRAGMA silently accepted");
    check(!db.apply_mod_sql("SELECT load_extension('must-not-load.dll');").success, "Extension loaded");
    rejects([&] { db.apply_mod_sql(std::string("SELECT 1;\0DELETE FROM Native;", 27)); });
    // Atomic API remains atomic, and does not inherit mod-SQL authorization.
    rejects([&] { db.apply_sql("INSERT INTO Native VALUES(17,'atomic'); COMMIT;"); });
    equals(db, "SELECT count(*) FROM Native WHERE ID=17", std::int64_t{0});
    db.apply_mod_xml(xml("<GameData><Native><Row ID='4294967298' V='next "
                         "action'/></Native></GameData>"));
    equals(db, "SELECT ID FROM Native WHERE V='next action'", std::int64_t{4294967298LL});
}
void missing_references() {
    RulesDatabase db;
    db.apply_sql(
        "CREATE TABLE Parent(Code TEXT UNIQUE); INSERT INTO Parent VALUES('keep'),('removed');"
        "CREATE TABLE Child(ID INTEGER PRIMARY KEY, Code TEXT REFERENCES Parent(Code));"
        "INSERT INTO Child VALUES(4294967297,'keep'),(2,'removed'),(3,'orphan'),(4,NULL);"
        "CREATE TABLE Unrelated(Code); INSERT INTO Unrelated VALUES('orphan');"
        "CREATE TABLE Other(Code TEXT UNIQUE);"
        "CREATE TABLE WrongTarget(Code REFERENCES Other(Code)); INSERT INTO WrongTarget VALUES('orphan');"
        "CREATE TABLE Implicit(Code REFERENCES Parent); INSERT INTO Implicit VALUES('orphan');");
    db.apply_xml(
        xml("<GameData><Parent><Delete Code='removed'/></Parent>"
            "<Table name='Early'><Column name='Code' type='text' reference='Parent(Code)'/></Table>"
            "<Early><Row Code='orphan'/></Early><DeleteMissingReferences table='pArEnT' column='cOdE'/>"
            "<Table name='Late'><Column name='Code' type='text' reference='Parent(Code)'/></Table>"
            "<Late><Row Code='orphan'/></Late></GameData>"));
    equals(db, "SELECT count(*) FROM Child", std::int64_t{2});
    equals(db, "SELECT ID FROM Child WHERE Code='keep'", std::int64_t{4294967297LL});
    equals(db, "SELECT count(*) FROM Child WHERE Code IS NULL", std::int64_t{1});
    equals(db, "SELECT count(*) FROM Early", std::int64_t{0});
    equals(db, "SELECT count(*) FROM Late", std::int64_t{1});
    equals(db, "SELECT count(*) FROM Unrelated", std::int64_t{1});
    equals(db, "SELECT count(*) FROM WrongTarget", std::int64_t{1});
    equals(db, "SELECT count(*) FROM Implicit", std::int64_t{1});
    // No matching incoming FK means a no-op, even if the target does not exist.
    db.apply_xml(xml("<GameData><DeleteMissingReferences table='Absent' column='Code'/></GameData>"));
    // A NULL in the target subquery preserves nonmatching children under NOT IN.
    db.apply_sql("INSERT INTO Parent VALUES(NULL); INSERT INTO Child VALUES(5,'orphan');");
    db.apply_mod_xml(xml("<GameData><DeleteMissingReferences table='Parent' column='Code'/></GameData>"));
    equals(db, "SELECT count(*) FROM Child WHERE ID=5", std::int64_t{1});
    db.apply_sql("DELETE FROM Parent WHERE Code IS NULL;");
    // Composite FK rows are matched individually, not as a tuple constraint.
    db.apply_sql("CREATE TABLE Pair(A, B, PRIMARY KEY(A,B)); INSERT INTO Pair VALUES('a','b');"
                 "CREATE TABLE PairChild(X,Y, FOREIGN KEY(X,Y) REFERENCES Pair(A,B));"
                 "INSERT INTO PairChild VALUES('a','wrong'),('missing','b');");
    db.apply_xml(xml("<GameData><DeleteMissingReferences table='Pair' column='A'/></GameData>"));
    equals(db, "SELECT count(*) FROM PairChild", std::int64_t{1});
    equals(db, "SELECT Y FROM PairChild", std::string("wrong"));
    db.apply_sql("CREATE TABLE Tree(ID INTEGER PRIMARY KEY, ParentID REFERENCES Tree(ID));"
                 "INSERT INTO Tree VALUES(1,NULL),(2,1),(3,999);");
    db.apply_xml(xml("<GameData><DeleteMissingReferences table='Tree' column='ID'/></GameData>"));
    equals(db, "SELECT count(*) FROM Tree", std::int64_t{2});
    // A trigger observes cleanup, and a later trigger error rolls back earlier
    // deletes and preceding XML mutations in the outer Load transaction.
    db.apply_sql(
        "CREATE TABLE CleanupLog(ID); CREATE TRIGGER child_deleted AFTER DELETE ON Child "
        "BEGIN INSERT INTO CleanupLog VALUES(old.ID); END;"
        "CREATE TRIGGER late_abort BEFORE DELETE ON Late BEGIN SELECT RAISE(ABORT,'late cleanup'); END;");
    rejects([&] {
        db.apply_mod_xml(xml("<GameData><Parent><Row Code='before failure'/></Parent>"
                             "<DeleteMissingReferences table='Parent' column='Code'/></GameData>"));
    });
    equals(db, "SELECT count(*) FROM CleanupLog", std::int64_t{0});
    equals(db, "SELECT count(*) FROM Parent WHERE Code='before failure'", std::int64_t{0});
    equals(db, "SELECT count(*) FROM Child WHERE ID=5", std::int64_t{1});
    db.apply_sql("DROP TRIGGER late_abort;");
    // The cleanup's nested SAVEPOINT _ must leave the source's outer transaction.
    check(db.apply_mod_sql("SAVEPOINT _; INSERT INTO Child VALUES(6,'pending');").transaction_active_after,
          "Missing outer source transaction");
    db.apply_mod_xml(xml("<GameData><DeleteMissingReferences table='Parent' column='Code'/></GameData>"));
    check(db.apply_mod_sql("SELECT 1;").transaction_active_after, "Cleanup ended outer transaction");
    check(db.apply_mod_sql("ROLLBACK;").success, "Cannot roll back outer source transaction");
    equals(db, "SELECT count(*) FROM Child WHERE ID=5", std::int64_t{1});
    equals(db, "SELECT count(*) FROM Child WHERE ID=6", std::int64_t{0});
    // Dynamic SQL has no original 512-byte formatting buffer; identifiers are
    // quoted, so supported quoted names cannot become executable SQL fragments.
    const std::string long_name = "Long" + std::string(600, 'x');
    db.apply_sql("CREATE TABLE \"" + long_name + "\"(Code REFERENCES Parent(Code)); INSERT INTO \"" +
                 long_name +
                 "\" VALUES('orphan'); CREATE TABLE \"a\"\"b\"(Code REFERENCES Parent(Code)); "
                 "INSERT INTO \"a\"\"b\" VALUES('orphan');");
    db.apply_xml(xml("<GameData><DeleteMissingReferences table='Parent' column='Code'/></GameData>"));
    equals(db, "SELECT count(*) FROM \"" + long_name + "\"", std::int64_t{0});
    equals(db, "SELECT count(*) FROM \"a\"\"b\"", std::int64_t{0});
    rejects([&] { db.apply_xml(xml("<GameData><DeleteMissingReferences column='Code'/></GameData>")); });
    rejects([&] { db.apply_xml(xml("<GameData><DeleteMissingReferences table='Parent'/></GameData>")); });
    rejects([&] {
        db.apply_xml(xml("<GameData><DeleteMissingReferences table='Parent' "
                         "column='Code'><Row/></DeleteMissingReferences></GameData>"));
    });
    rejects([&] { db.apply_sql("PRAGMA foreign_key_list(Child);"); });
    check(!db.apply_mod_sql("PRAGMA writable_schema=ON;").success, "Internal PRAGMA authorization leaked");
}
void localization() {
    RulesDatabase text;
    text.apply_localization_schema(
        "CREATE TABLE Languages(Type, TableName, TagColumnName, TextColumnName, PluralityColumnName, "
        "GenderColumnName);"
        "CREATE TABLE LanguagePriorities(LanguageSet,Language,Priority,PRIMARY KEY(LanguageSet,Language));"
        "CREATE TABLE LocalizedText(Language,Tag,Text,Gender,Plurality,PRIMARY KEY(Language,Tag));"
        "CREATE TABLE ScannedFiles(Path PRIMARY KEY,DateTime);"
        "CREATE TABLE Language_en_US(ID INTEGER PRIMARY KEY,Tag,Text,Gender,Plurality);"
        "CREATE TABLE Language_RU_RU(ID INTEGER PRIMARY KEY,Tag,Text,Gender,Plurality);"
        "INSERT INTO Languages VALUES('en_US','Language_en_US','Tag','Text','Plurality','Gender'),"
        "('ru_RU','Language_RU_RU','Tag','Text','Plurality','Gender');"
        "INSERT INTO LanguagePriorities VALUES('en_US','en_US',7);"
        "INSERT INTO Language_en_US VALUES(4294967297,'SAME',' [ICON_GOLD] {1_Num} ','feminine','plural');"
        "INSERT INTO Language_RU_RU VALUES(4294967298,'SAME','Русский','masculine','singular');"
        "PRAGMA user_version=2;");
    const auto long_table = "Language_" + std::string(600, 'x');
    text.apply_statements(std::vector<DatabaseStatement>{
        {"CREATE TABLE " + long_table + "(Tag,Text,Gender,Plurality)", {}},
        {"INSERT INTO " + long_table + " VALUES('LONG','full length',NULL,NULL)", {}},
        {"INSERT INTO Languages VALUES(?,?,'Tag','Text','Plurality','Gender')", {"q'a", long_table}}});
    check(text.convert_localization_tables() == 3, "Language metadata count");
    equals(text, "SELECT Text FROM " + long_table, std::string("full length"));
    text.apply_sql("UPDATE " + long_table + " SET Text='long view updated' WHERE Tag='LONG';");
    equals(text, "SELECT Text FROM LocalizedText WHERE Language='q''a'", std::string("long view updated"));
    equals(text, "SELECT type FROM sqlite_master WHERE name='Language_en_US'", std::string("view"));
    rejects([&] { scalar(text, "SELECT ID FROM Language_en_US"); });
    equals(text, "SELECT Priority FROM LanguagePriorities WHERE Language='en_US'", std::int64_t{7});
    equals(text, "SELECT Priority FROM LanguagePriorities WHERE Language='ru_RU'", std::int64_t{100});
    equals(text, "SELECT Text FROM Language_en_US", std::string(" [ICON_GOLD] {1_Num} "));
    equals(text, "SELECT Gender || ':' || Plurality FROM Language_en_US", std::string("feminine:plural"));
    text.apply_xml(xml("<GameData><Language_en_US><Update><Where Tag='SAME'/><Set Text='English update'/>"
                       "</Update><Row Tag='ADDED' Text='mod text'/></Language_en_US></GameData>"));
    equals(text, "SELECT Text FROM LocalizedText WHERE Language='en_US' AND Tag='SAME'",
           std::string("English update"));
    equals(text, "SELECT Text FROM Language_RU_RU", std::string("Русский"));
    text.apply_sql("UPDATE Language_en_US SET Tag='RENAMED' WHERE Tag='SAME';");
    equals(text, "SELECT count(*) FROM Language_en_US WHERE Tag='SAME'", std::int64_t{1});
    equals(text, "SELECT count(*) FROM Language_en_US WHERE Tag='RENAMED'", std::int64_t{0});
    // Merged DLC replaces matching language/tag only. Input stays independent.
    RulesDatabase dlc;
    dlc.apply_localization_schema(
        "CREATE TABLE LocalizedText(Language,Tag,Text,Gender,Plurality,PRIMARY KEY(Language,Tag));"
        "INSERT INTO LocalizedText VALUES('en_US','SAME','DLC','neutral',NULL),"
        "('ru_RU','NEW','Новое',NULL,NULL); PRAGMA user_version=2;");
    text.merge_localization(dlc);
    equals(text, "SELECT Text FROM Language_en_US WHERE Tag='SAME'", std::string("DLC"));
    equals(text, "SELECT Text FROM Language_RU_RU WHERE Tag='SAME'", std::string("Русский"));
    equals(text, "SELECT Text FROM Language_RU_RU WHERE Tag='NEW'", std::string("Новое"));
    RulesDatabase wrong;
    wrong.apply_sql("CREATE TABLE LocalizedText(OnlyOne);");
    rejects([&] { text.merge_localization(wrong); });
    // Failed merge detaches its private schema, allowing a subsequent merge.
    text.merge_localization(dlc);
    text.track_localization_mod_changes();
    equals(text, "SELECT count(*) FROM ScannedFiles", std::int64_t{0});
    RulesDatabase game;
    game.apply_sql("CREATE TABLE GameState(ID INTEGER PRIMARY KEY,V); INSERT INTO GameState VALUES(1,10);");
    game.attach_localization(text);
    equals(game, "SELECT Text FROM Language_en_US WHERE Tag='SAME'", std::string("DLC"));
    equals(game, "SELECT Text FROM Localization.Language_en_US WHERE Tag='SAME'", std::string("DLC"));
    equals(game, "SELECT Text FROM " + long_table, std::string("long view updated"));
    auto diagnostic=game.clone_gameplay();
    equals(*diagnostic, "SELECT V FROM GameState", std::int64_t{10});
    equals(*diagnostic, "SELECT Text FROM Localization.Language_en_US WHERE Tag='SAME'", std::string("DLC"));
    diagnostic->apply_sql("UPDATE GameState SET V=4294967299; "
                          "UPDATE Language_en_US SET Text='diagnostic edit' WHERE Tag='SAME';");
    equals(*diagnostic, "SELECT V FROM GameState", std::int64_t{4294967299LL});
    equals(game, "SELECT V FROM GameState", std::int64_t{10});
    equals(*diagnostic, "SELECT Text FROM Language_en_US WHERE Tag='SAME'", std::string("diagnostic edit"));
    equals(game, "SELECT Text FROM Language_en_US WHERE Tag='SAME'", std::string("DLC"));
    auto text_copy=text.clone_gameplay();
    equals(*text_copy, "SELECT Text FROM Language_en_US WHERE Tag='SAME'", std::string("DLC"));
    text_copy->apply_sql("UPDATE Language_en_US SET Text='private' WHERE Tag='SAME';");
    equals(*text_copy, "SELECT Text FROM Language_en_US WHERE Tag='SAME'", std::string("private"));
    equals(text, "SELECT Text FROM Language_en_US WHERE Tag='SAME'", std::string("DLC"));
    rejects([&] { game.attach_localization(text); });
    // Successful source prefix is visible in both main and Localization.
    check(!game.apply_mod_sql("UPDATE GameState SET V=4294967297; "
                              "UPDATE Language_en_US SET Text='changed' WHERE Tag='SAME'; "
                              "INSERT INTO Absent VALUES(1);")
               .success,
          "Expected failing SQL suffix");
    equals(game, "SELECT V FROM GameState", std::int64_t{4294967297LL});
    equals(game, "SELECT Text FROM Language_en_US WHERE Tag='SAME'", std::string("changed"));
    equals(game, "SELECT DateTime FROM Localization.ScannedFiles WHERE Path='Mods'", std::int64_t{0});
    equals(text, "SELECT Text FROM Language_en_US WHERE Tag='SAME'", std::string("DLC"));
    equals(dlc, "SELECT Text FROM LocalizedText WHERE Language='en_US'", std::string("DLC"));
    game.apply_sql("DELETE FROM Localization.ScannedFiles;");
    rejects([&] {
        game.apply_mod_xml(xml("<GameData><GameState><Update><Set V='3'/></Update></GameState>"
                               "<Language_en_US><Delete Tag='ADDED'/></Language_en_US>"
                               "<Absent><Row V='0'/></Absent></GameData>"));
    });
    equals(game, "SELECT V FROM GameState", std::int64_t{4294967297LL});
    equals(game, "SELECT count(*) FROM Language_en_US WHERE Tag='ADDED'", std::int64_t{1});
    equals(game, "SELECT count(*) FROM Localization.ScannedFiles", std::int64_t{0});
    game.apply_sql("DELETE FROM Language_en_US WHERE Tag='ADDED';");
    equals(game, "SELECT count(*) FROM Language_en_US WHERE Tag='ADDED'", std::int64_t{0});
    equals(game, "SELECT count(*) FROM Localization.ScannedFiles WHERE Path='Mods'", std::int64_t{1});
    rejects([&] { game.apply_sql("ATTACH ':memory:' AS Unexpected"); });
    rejects([&] { game.apply_sql("DETACH Localization"); });
    rejects([&] { text.apply_sql("PRAGMA user_version=3;"); });
    rejects([&] { text.apply_localization_schema("PRAGMA writable_schema=ON;"); });
    rejects([&] { text.apply_sql("PRAGMA user_version=3;"); });
    rejects([&] { text.attach_localization(text); });
    RulesDatabase pending;
    check(pending.apply_mod_sql("SAVEPOINT _;").transaction_active_after, "Expected pending source");
    rejects([&] { text.attach_localization(pending); });
    rejects([&] { pending.attach_localization(text); });
    check(pending.apply_mod_sql("ROLLBACK;").success, "Pending rollback");
    pending.attach_localization(text);
    equals(pending, "SELECT Text FROM Language_en_US WHERE Tag='SAME'", std::string("DLC"));
}
void package_tags() {
    const auto tag = dlc_game_data_tag("{6da07636-4123-4018-b643-6575b4ec336b}");
    check(tag.value == "6DA0763641234018B6436575B4EC336B", "Native DLC GUID format");
    rejects([] { (void)dlc_game_data_tag("not-a-guid"); });
    RulesDatabase db;
    db.apply_sql("CREATE TABLE Civilizations(ID INTEGER PRIMARY KEY,Type UNIQUE,PackageID TEXT);"
                 "CREATE TABLE Leaders(ID INTEGER PRIMARY KEY,Type UNIQUE,PackageID TEXT);"
                 "CREATE TABLE Other(Type,PackageID TEXT);");
    db.apply_mod_xml(xml("<GameData><Civilizations>"
                         "<Row><ID>4294967297</ID><Type>CHILD</Type></Row>"
                         "<Row ID='2' Type='ATTRIBUTES'/>"
                         "<Row ID='3'><Type>MIXED</Type></Row>"
                         "<Row><ID>4</ID><Type>EXPLICIT</Type><PackageID>authored</PackageID></Row>"
                         "</Civilizations><Leaders><InsertOrIgnore><Type>LEADER</Type></InsertOrIgnore>"
                         "</Leaders><Other><Row><Type>OTHER</Type></Row></Other></GameData>"),
                     tag);
    equals(db, "SELECT PackageID FROM Civilizations WHERE Type='CHILD'", tag.value);
    equals(db, "SELECT ID FROM Civilizations WHERE Type='CHILD'", std::int64_t{4294967297LL});
    equals(db, "SELECT PackageID FROM Civilizations WHERE Type='ATTRIBUTES'", std::monostate{});
    equals(db, "SELECT PackageID FROM Civilizations WHERE Type='MIXED'", tag.value);
    // The native helper appends a duplicate PackageID column; do not turn it
    // into replacement of the author's earlier field. SQLite takes the first.
    equals(db, "SELECT PackageID FROM Civilizations WHERE Type='EXPLICIT'", std::string("authored"));
    equals(db, "SELECT PackageID FROM Leaders WHERE Type='LEADER'", tag.value);
    equals(db, "SELECT PackageID FROM Other", std::monostate{});
    db.apply_mod_xml(xml("<GameData><civilizations><Row><Type>LOWERCASE</Type></Row></civilizations>"
                         "<Civilizations><Update><Where Type='ATTRIBUTES'/><Set Type='RENAMED'/></Update>"
                         "</Civilizations></GameData>"),
                     tag);
    equals(db, "SELECT PackageID FROM Civilizations WHERE Type='LOWERCASE'", std::monostate{});
    equals(db, "SELECT PackageID FROM Civilizations WHERE Type='RENAMED'", std::monostate{});
    // Context belongs to one XML load, not to subsequent mod actions.
    db.apply_mod_xml(xml("<GameData><Civilizations><Row><Type>MOD</Type></Row></Civilizations></GameData>"));
    equals(db, "SELECT PackageID FROM Civilizations WHERE Type='MOD'", std::monostate{});
    const GameDataPackageTag custom{"CUSTOM", {"Leaders", "Leaders"}};
    db.apply_mod_xml(xml("<GameData><Leaders><Replace><Type>LEADER</Type></Replace></Leaders></GameData>"),
                     custom);
    equals(db, "SELECT PackageID FROM Leaders WHERE Type='LEADER'", std::string("CUSTOM"));
    rejects([&] {
        db.apply_mod_xml(xml("<GameData><Civilizations><Row><Type>ROLLED_BACK</Type></Row></Civilizations>"
                             "<Absent><Row X='1'/></Absent></GameData>"),
                         tag);
    });
    equals(db, "SELECT count(*) FROM Civilizations WHERE Type='ROLLED_BACK'", std::int64_t{0});
}
#include "gameplay_postprocess_tests.hpp"
#include "info_type_tests.hpp"
#include "global_define_tests.hpp"
#include "info_collection_tests.hpp"
#include "technology_rules_tests.hpp"
#include "team_technologies_tests.hpp"
#include "research_tests.hpp"
#include "custom_mod_options_tests.hpp"
#include "research_context_tests.hpp"
#include "player_research_tests.hpp"
#include "research_progress_tests.hpp"
#include "city_yields_tests.hpp"
#include "bnw_unhappiness_tests.hpp"
#include "maintenance_tests.hpp"
#include "route_economy_tests.hpp"
#include "plot_water_tests.hpp"
#include "plot_movement_tests.hpp"
#include "bnw_nature_yield_tests.hpp"
#include "vp_nature_yield_tests.hpp"
#include "vp_plot_religion_tests.hpp"
#include "vp_player_plot_yield_tests.hpp"
#include "vp_plot_yield_tests.hpp"
#include "vp_city_food_tests.hpp"
#include "vp_city_growth_tests.hpp"
#include "vp_unit_supply_tests.hpp"
#include "vp_great_works_tests.hpp"
#include "vp_city_building_yield_tests.hpp"
#include "vp_city_religion_minor_yield_tests.hpp"
#include "vp_city_trait_yield_tests.hpp"
#include "vp_city_happiness_tests.hpp"
#include "vp_city_base_yield_tests.hpp"
#include "vp_corporations_tests.hpp"
#include "vp_city_production_tests.hpp"
#include "vp_building_production_tests.hpp"
#include "vp_unit_trait_tests.hpp"
#include "vp_player_unit_production_tests.hpp"
#include "vp_city_unit_production_tests.hpp"
#include "production_progress_tests.hpp"
#include "improvement_yield_tests.hpp"
#include "improvement_yield_calculation_tests.hpp"
#include "resource_visibility_tests.hpp"
#include "treasury_tests.hpp"
#include "science_tests.hpp"
#include "treasury_turn_tests.hpp"
#include "research_turn_tests.hpp"
#include "vp_espionage_turn_tests.hpp"
#include "vp_city_espionage_tests.hpp"
#include "vp_city_construction_intrigue_tests.hpp"
#include "vp_diplomacy_intrigue_tests.hpp"
#include "vp_travelling_turn_tests.hpp"
#include "vp_espionage_checkpoint_tests.hpp"
#include "vp_network_points_tests.hpp"
#include "vp_network_point_binding_tests.hpp"
#include "research_selection_tests.hpp"
#include "research_ai_weights_tests.hpp"
#include "lua_script_host_tests.hpp"
#include "lua_event_dispatch_tests.hpp"
#include "lua_include_tests.hpp"
#include "lua_modding_tests.hpp"
#include "lua_gameplay_tests.hpp"
#include "lua_gameinfo_tests.hpp"
#include "lua_custom_events_tests.hpp"
#include "lua_ui_events_tests.hpp"
#include "lua_save_data_tests.hpp"
#include "saved_value_fallback_tests.hpp"
#include "lua_user_data_tests.hpp"
#include "lua_gamecore_enum_tests.hpp"
#include "lua_locale_tests.hpp"
#include "lua_pregame_tests.hpp"
#include "lua_map_tests.hpp"
#include "lua_content_tests.hpp"
} // namespace
int main(int argc,char **argv) {
    try {
        run();
        try_inserts();
        mod_actions();
        missing_references();
        localization();
        package_tags();
        gameplay_postprocess_tests();
        info_type_tests();
        global_define_tests();
        info_collection_tests();
        technology_rules_tests();
        team_technologies_tests();
        research_tests();
        custom_mod_options_tests();
        research_context_tests();
        player_research_tests();
        research_progress_tests();
        city_yields_tests();
        bnw_unhappiness_tests();
        maintenance_tests();
        route_economy_tests();
        plot_water_tests();
        plot_movement_tests();
        bnw_nature_yield_tests();
        vp_nature_yield_tests();
        vp_plot_religion_tests();
        vp_player_plot_yield_tests();
        vp_plot_yield_tests();
        vp_city_food_tests();
        vp_city_growth_tests();
        vp_unit_supply_tests();
        vp_great_works_tests();
        vp_city_building_yield_tests();
        vp_city_religion_minor_yield_tests();
        vp_city_trait_yield_tests();
        vp_city_happiness_tests();
        vp_city_base_yield_tests();
        vp_corporations_tests();
        vp_city_production_tests();
        vp_building_production_tests();
        vp_unit_trait_tests();
        vp_player_unit_production_tests();
        vp_city_unit_production_tests();
        production_progress_tests();
        improvement_yield_tests();
        improvement_yield_calculation_tests();
        resource_visibility_tests();
        treasury_tests();
        science_tests();
        treasury_turn_tests();
        research_turn_tests();
        vp_espionage_turn_tests();
        vp_city_espionage_tests();
        vp_city_construction_intrigue_tests();
        vp_diplomacy_intrigue_tests();
        vp_travelling_turn_tests();
        vp_espionage_checkpoint_tests();
        vp_network_points_tests();
        vp_network_point_rate_tests();
        vp_network_point_binding_tests();
        research_selection_tests();
        research_ai_weights_tests();
        lua_script_host_tests();
        lua_event_dispatch_tests();
        lua_include_tests();
        lua_modding_tests();
        lua_gameplay_tests();
        lua_gameinfo_tests();
        lua_custom_events_tests();
        lua_ui_events_tests();
        lua_save_data_tests();
        saved_value_fallback_tests();
        lua_user_data_tests();
        lua_gamecore_enum_tests();
        lua_locale_tests();
        lua_pregame_tests();
        lua_map_tests();
        lua_content_tests();
        if(argc>4)throw std::runtime_error("Optional arguments are original city bombard, Inca and Pontoon Lua sources");
        if(argc>=2)lua_gameplay_asset_tests(std::filesystem::path(argv[1]));
        if(argc>=3)lua_pregame_asset_tests(std::filesystem::path(argv[2]));
        if(argc==4)lua_map_asset_tests(std::filesystem::path(argv[3]));
        compound_select_stack_tests();
        std::cout << "PASS " << checks << " rules checks; runtime_bits=" << sizeof(void *) * 8 << '\n';
        return 0;
    } catch (const std::exception &error) {
        std::cerr << "FAIL after " << checks << ": " << error.what() << '\n';
        return 1;
    }
}
