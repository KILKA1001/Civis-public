#include "civ5/runtime/bnw_fresh_feature_effects.hpp"
#include "civ5/runtime/file_identity.hpp"
#include "civ5/content/rules_database.hpp"
#include <stdexcept>
#include <string>
#include <vector>

namespace {
void check(bool condition, const char *message) {
    if (!condition) throw std::runtime_error(message);
}

void test_detached_post_callback_effects() {
    using namespace civ5::runtime;
    civ5::content::RulesDatabase selected;
    selected.apply_sql(R"sql(
        CREATE TABLE Features (
            ID INTEGER PRIMARY KEY, Type TEXT, YieldNotAdditive INTEGER,
            Impassable INTEGER, Rough INTEGER, NaturalWonder INTEGER);
        INSERT INTO Features VALUES (7,'FEATURE_TEST',0,1,1,0);
        INSERT INTO Features VALUES (8,'FEATURE_WONDER',1,1,0,1);
        CREATE TABLE Yields (ID INTEGER PRIMARY KEY, Type TEXT, MountainChange INTEGER);
        INSERT INTO Yields VALUES (0,'YIELD_FOOD',0);
        INSERT INTO Yields VALUES (1,'YIELD_PRODUCTION',0);
        INSERT INTO Yields VALUES (2,'YIELD_GOLD',0);
        INSERT INTO Yields VALUES (3,'YIELD_SCIENCE',0);
        INSERT INTO Yields VALUES (4,'YIELD_CULTURE',0);
        INSERT INTO Yields VALUES (5,'YIELD_FAITH',0);
        CREATE TABLE Feature_YieldChanges (FeatureType TEXT, YieldType TEXT, Yield INTEGER);
        CREATE TABLE Feature_RiverYieldChanges (FeatureType TEXT, YieldType TEXT, Yield INTEGER);
        CREATE TABLE Feature_HillsYieldChanges (FeatureType TEXT, YieldType TEXT, Yield INTEGER);
        INSERT INTO Feature_YieldChanges VALUES ('FEATURE_TEST','YIELD_FOOD',1);
        INSERT INTO Feature_HillsYieldChanges VALUES ('FEATURE_TEST','YIELD_FOOD',2);
        INSERT INTO Feature_YieldChanges VALUES ('FEATURE_WONDER','YIELD_FOOD',5);
    )sql");
    BnwSetupPlotGrid grid;
    grid.reset(2, 1, false, false);
    for (std::size_t i = 0; i < grid.size(); ++i) {
        grid.apply_stock_fresh_plot_type(i, i ? 1 : 2, false, false, true,
                                         {0, false, false});
        grid.apply_stock_fresh_terrain_type(i, {0, false, false},
                                            false, false);
    }
    const auto areas = grid.draft_fresh_area_structure(1);
    BnwFreshYieldRules terrain_yields;
    terrain_yields.source_rules = &selected;
    terrain_yields.terrains[0].base[0] = 2;
    terrain_yields.yields[0].hills = 1;
    const auto replay = replay_bnw_fresh_yield_passes(
        grid, areas, terrain_yields);
    grid.apply_fresh_area_handles(areas, replay.final_cache,
                                  replay.symbol_events);
    const auto source_grid = grid;
    const auto source_areas = areas;
    const auto grid_unchanged = [&] {
        if (grid.size() != source_grid.size() ||
            grid.land_plot_count() != source_grid.land_plot_count() ||
            grid.symbol_events().size() != source_grid.symbol_events().size())
            return false;
        for (std::size_t i = 0; i < grid.size(); ++i)
            if (grid.at(i) != source_grid.at(i)) return false;
        return true;
    };
    const std::vector<std::int32_t> no_features(grid.size(), -1);
    const auto sha = identify_bytes(selected.gameplay_fingerprint_data()).sha256;
    const std::string map_sha(64, 'b');

    const auto normal = resolve_bnw_fresh_feature_set_info(selected, 7);
    const auto normal_yields = load_bnw_fresh_feature_yield_info(selected, normal);
    BnwSelectedFeatureEligibility eligibility;
    eligibility.source_rules = &selected;
    eligibility.id = 7;
    eligibility.terrain_ids = {0};
    const auto normal_plan = inspect_bnw_fresh_feature_set(
        grid, areas, no_features, 1, 7, 3, eligibility, normal, 1);
    BnwFreshFeatureEventJournal normal_events(1);
    const auto &normal_event = normal_events.stage_prewrite(
        normal_plan, grid, no_features, normal, sha, map_sha);
    auto normal_draft = draft_bnw_fresh_feature_after_callback(
        normal_event, grid, areas, terrain_yields, normal,
        normal_yields, true, 0, 0, 10);
    check(normal_draft.after &&
              normal_draft.after->feature_type == 7 &&
              normal_draft.after->variety == 0 &&
              normal_draft.after->rough &&
              normal_draft.after->plot.impassable &&
              normal_draft.after->plot.yields[0] == 6 &&
              normal_draft.before.plot.yields[0] == 3 &&
              normal_draft.yield_symbol_after &&
              (*normal_draft.yield_symbol_after)[0] == 6 &&
              normal_draft.source_order ==
                  std::vector<BnwFreshFeatureEffectStep>{
                      BnwFreshFeatureEffectStep::feature_written,
                      BnwFreshFeatureEffectStep::yield_recomputed,
                      BnwFreshFeatureEffectStep::yield_symbol_staged,
                      BnwFreshFeatureEffectStep::impassability_updated,
                      BnwFreshFeatureEffectStep::empty_sight_restored,
                      BnwFreshFeatureEffectStep::variety_forced_zero,
                      BnwFreshFeatureEffectStep::rough_feature_checked,
                      BnwFreshFeatureEffectStep::old_feature_removal_skipped} &&
              !normal_draft.can_publish() &&
              grid_unchanged(),
          "Normal feature draft must use old impassability and SDK effect order");
    bool unverified_world_rejected = false;
    try { (void)draft_bnw_fresh_feature_after_callback(
        normal_event, grid, areas, terrain_yields, normal,
        normal_yields, false, 0, 0, 10); }
    catch (const std::invalid_argument &) { unverified_world_rejected = true; }
    check(unverified_world_rejected,
          "Feature suffix must reject unverified owner/city/unit/recon state");
    normal_draft.rollback();
    check(!normal_draft.after && !normal_draft.yield_symbol_after &&
              grid_unchanged(),
          "Discarding the detached suffix must leave owned grid unchanged");

    const auto wonder = resolve_bnw_fresh_feature_set_info(selected, 8);
    const auto wonder_yields = load_bnw_fresh_feature_yield_info(selected, wonder);
    eligibility.id = 8;
    const auto wonder_plan = inspect_bnw_fresh_feature_set(
        grid, areas, no_features, 0, 8, -1, eligibility, wonder, 1);
    BnwFreshFeatureEventJournal wonder_events(1);
    const auto &wonder_event = wonder_events.stage_prewrite(
        wonder_plan, grid, no_features, wonder, sha, map_sha);
    const auto wonder_draft = draft_bnw_fresh_feature_after_callback(
        wonder_event, grid, areas, terrain_yields, wonder,
        wonder_yields, true, 2, 1, 10);
    check(wonder_draft.after &&
              wonder_draft.after->plot.yields[0] == 5 &&
              wonder_draft.after->map_natural_wonders == 3 &&
              wonder_draft.after->area_natural_wonders == 2 &&
              wonder_draft.source_order[7] ==
                  BnwFreshFeatureEffectStep::map_natural_wonders_incremented &&
              wonder_draft.source_order[8] ==
                  BnwFreshFeatureEffectStep::area_natural_wonders_incremented &&
              areas.area_ids == source_areas.area_ids &&
              grid_unchanged() && !wonder_draft.can_publish(),
          "Natural wonder counters and nonadditive yield stay detached");
}
} // namespace

int main() {
    test_detached_post_callback_effects();
}
