#include "civ5/runtime/bnw_fresh_start_coast.hpp"
#include "civ5/runtime/bnw_map_script.hpp"
#include "civ5/runtime/file_identity.hpp"
#include "civ5/content/rules_database.hpp"
#include <stdexcept>

namespace {
void check(bool condition, const char *message) {
    if (!condition) throw std::runtime_error(message);
}

void test_start_coast_tables() {
    using namespace civ5::runtime;
    BnwFreshGameContentResult selected;
    selected.profile.kind = ContentProfileKind::dlc;
    selected.rules = std::make_unique<civ5::content::RulesDatabase>();
    selected.activation.emplace();
    selected.activation->profile_kind = ContentProfileKind::dlc;
    selected.activation->rules_database_ready = true;
    const auto rules_sha = identify_bytes(
        selected.rules->gameplay_fingerprint_data()).sha256;
    const std::vector<BnwMapIncludedSource> stock_includes{{
        "MapmakerUtilities",
        "ea463458f9fde6571ce2269c2ebf994f58d8f5b9859201bc506703d176c03f47",
        {civ5::assets::SourceKind::loose, "Expansion2",
         "Assets/DLC/Expansion2/Gameplay/Lua/MapmakerUtilities.lua", std::nullopt}}};
    SimulationRandom random(RandomAlgorithm::bnw, {73, 12, 1});
    BnwSetupPlotGrid grid;
    grid.reset(4, 2, false, false);
    for (std::size_t i = 0; i < grid.size(); ++i) {
        const bool water = i == 0 || i == 4;
        grid.apply_stock_fresh_plot_type(i, water ? 3 : 2,
                                         false, false, true, {0, false, false});
        grid.apply_stock_fresh_terrain_type(i,
                                            {water ? 1 : 0, water, false},
                                            false, false);
    }
    const auto areas = grid.draft_fresh_area_structure(1);
    std::vector<std::array<std::int16_t, 6>> yields(grid.size());
    grid.apply_fresh_area_handles(areas, yields, {});
    const auto original_grid = grid;
    const auto original_rng = random.state();

    auto pending = draft_bnw_fresh_start_coast_tables(
        selected, grid, areas, random, stock_includes, rules_sha, false);
    check(pending.has_blocker(BnwStartCoastBlockerCode::post_feature_areas_pending) &&
              !pending.coastal_tables_complete && pending.coastal.empty(),
          "Second post-feature area pass must precede starting plot data");
    auto mismatch = draft_bnw_fresh_start_coast_tables(
        selected, grid, areas, random, stock_includes, "other-rules", true);
    check(mismatch.has_blocker(BnwStartCoastBlockerCode::selected_rules_unverified) &&
              mismatch.coastal.empty(),
          "Starting plot data must reject another selected rules identity");
    auto incomplete = areas;
    incomplete.landmass_ids.pop_back();
    auto invalid = draft_bnw_fresh_start_coast_tables(
        selected, grid, incomplete, random, stock_includes, rules_sha, true);
    check(invalid.has_blocker(BnwStartCoastBlockerCode::owned_map_incomplete) &&
              invalid.coastal.empty(),
          "Partial landmass handles cannot publish coastal data");

    auto tables = draft_bnw_fresh_start_coast_tables(
        selected, grid, areas, random, stock_includes, rules_sha, true);
    check(tables.coastal_tables_complete &&
              tables.has_blocker(BnwStartCoastBlockerCode::start_placement_pending) &&
              tables.selected_rules_sha256 == rules_sha &&
              tables.map_random_before == original_rng &&
              tables.map_random_after == original_rng && random.state() == original_rng,
          "Coastal tables are detached and consume no map RNG");
    check(tables.coastal == std::vector<bool>({false, true, false, false,
                                               false, true, false, false}) &&
              tables.next_to_coast == std::vector<bool>({false, false, true, false,
                                                         false, false, true, false}),
          "Selected two-tile salt-water landmass produces coastal and second-ring land");
    check(!tables.queries.empty() &&
              tables.queries.front().stage ==
                  BnwStartCoastQueryStage::adjacent_to_salt_water &&
              tables.queries.front().plot_index == 1 &&
              tables.queries.front().direction == 0 &&
              tables.queries.back().stage ==
                  BnwStartCoastQueryStage::next_to_coastal_land,
          "Queries retain MapmakerUtilities x/y and DirectionTypes order");
    const auto altered_helper = draft_bnw_fresh_start_coast_tables(
        selected, grid, areas, random, {}, rules_sha, true);
    check(altered_helper.has_blocker(BnwStartCoastBlockerCode::stock_helper_unverified) &&
              altered_helper.coastal.empty(),
          "Hardcoded stock coastal semantics require exact selected helper bytes");
    for (std::size_t i = 0; i < grid.size(); ++i)
        check(grid.at(i) == original_grid.at(i), "Read-only stage changed plot");
    tables.rollback();
    check(!tables.coastal_tables_complete && tables.coastal.empty() &&
              tables.next_to_coast.empty() && tables.queries.empty() &&
              random.state() == original_rng,
          "Rollback discards only the detached starting data");
}
} // namespace

int main() { test_start_coast_tables(); }
