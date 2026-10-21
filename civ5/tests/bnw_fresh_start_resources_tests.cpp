#include "civ5/runtime/bnw_fresh_start_resources.hpp"
#include "civ5/runtime/bnw_map_script.hpp"
#include "civ5/runtime/file_identity.hpp"
#include "civ5/content/rules_database.hpp"
#include <stdexcept>

namespace {
void check(bool condition, const char *message) {
    if (!condition) throw std::runtime_error(message);
}

void test_source_order_resource_scan() {
    using namespace civ5::runtime;
    BnwFreshGameContentResult selected;
    selected.profile.kind = ContentProfileKind::dlc;
    selected.rules = std::make_unique<civ5::content::RulesDatabase>();
    selected.rules->apply_sql(R"sql(
        CREATE TABLE Resources(ID INTEGER, Type TEXT);
        INSERT INTO Resources VALUES(7,'RESOURCE_COAL');
        INSERT INTO Resources VALUES(2,'RESOURCE_UNKNOWN_MODDED');
        INSERT INTO Resources VALUES(9,'RESOURCE_WHEAT');
    )sql");
    selected.activation.emplace();
    selected.activation->profile_kind = ContentProfileKind::dlc;
    selected.activation->rules_database_ready = true;
    SimulationRandom map_random(RandomAlgorithm::bnw, {99, 14, 2});
    BnwFreshStartCoastTables coastal;
    coastal.coastal_tables_complete = true;
    coastal.coastal = {true, false};
    coastal.next_to_coast = {false, true};
    coastal.map_random_before = coastal.map_random_after = map_random.state();
    coastal.selected_rules_sha256 = identify_bytes(
        selected.rules->gameplay_fingerprint_data()).sha256;
    coastal.blockers.push_back({BnwStartCoastBlockerCode::start_placement_pending,
                                "fixture"});
    const std::vector<BnwMapIncludedSource> stock_includes{{
        "AssignStartingPlots",
        "a957db6a15138a09aef4221217c654ef146dca65aee17aea701e13b3db3b1b32",
        {civ5::assets::SourceKind::loose, "Expansion2",
         "Assets/DLC/Expansion2/Gameplay/Lua/AssignStartingPlots.lua", std::nullopt}}};

    const auto before = map_random.state();
    auto scan = draft_bnw_fresh_start_resources(
        selected, coastal, map_random, stock_includes);
    check(scan.resource_scan_complete &&
              scan.has_blocker(BnwStartResourcesBlockerCode::luxury_weights_pending) &&
              scan.resources == std::vector<BnwStartResourceRow>({
                  {7, "RESOURCE_COAL"}, {2, "RESOURCE_UNKNOWN_MODDED"},
                  {9, "RESOURCE_WHEAT"}}) &&
              scan.stock_shortcuts == std::vector<BnwStartResourceRow>({
                  {7, "RESOURCE_COAL"}, {9, "RESOURCE_WHEAT"}}),
          "GameInfo.Resources visit order is SQLite SELECT order, not ID order");
    check(scan.map_random_before == before && scan.map_random_after == before &&
              map_random.state() == before,
          "Resource-ID setup consumes no map RNG");

    auto wrong_rules = coastal;
    wrong_rules.selected_rules_sha256 = "other-rules";
    const auto refused = draft_bnw_fresh_start_resources(
        selected, wrong_rules, map_random, stock_includes);
    check(refused.has_blocker(BnwStartResourcesBlockerCode::selected_rules_unverified) &&
              refused.resources.empty(),
          "Resource rows cannot be mixed with another selected map profile");
    auto unready = coastal;
    unready.coastal_tables_complete = false;
    const auto pending = draft_bnw_fresh_start_resources(
        selected, unready, map_random, stock_includes);
    check(pending.has_blocker(BnwStartResourcesBlockerCode::coastal_tables_pending) &&
              pending.resources.empty(),
          "Resource scan cannot overtake source-order coastal setup");
    const auto budget = draft_bnw_fresh_start_resources(
        selected, coastal, map_random, stock_includes, 2);
    check(budget.has_blocker(BnwStartResourcesBlockerCode::resource_rows_unavailable) &&
              budget.resources.empty(),
          "A truncated GameInfo resource iterator cannot publish a partial scan");
    const auto wrong_helper = draft_bnw_fresh_start_resources(
        selected, coastal, map_random, {});
    check(wrong_helper.has_blocker(BnwStartResourcesBlockerCode::stock_helper_unverified) &&
              wrong_helper.resources.empty(),
          "Stock shortcut branches require exact selected AssignStartingPlots bytes");
    scan.rollback();
    check(!scan.resource_scan_complete && scan.resources.empty() &&
              scan.stock_shortcuts.empty() && map_random.state() == before,
          "Rollback discards only detached resource shortcuts");
}
} // namespace

int main() { test_source_order_resource_scan(); }
