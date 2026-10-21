#include "civ5/runtime/bnw_fresh_feature_events.hpp"
#include "civ5/runtime/bnw_fresh_yield.hpp"
#include "civ5/runtime/file_identity.hpp"
#include "civ5/content/rules_database.hpp"
#include <stdexcept>
#include <string>
#include <type_traits>
#include <vector>

namespace {
void check(bool condition, const char *message) {
    if (!condition) throw std::runtime_error(message);
}

void test_prewrite_journal() {
    using namespace civ5::runtime;
    static_assert(!std::is_copy_assignable_v<BnwFreshFeatureChangedEvent>);
    BnwSetupPlotGrid grid;
    grid.reset(2, 1, false, false);
    for (std::size_t i = 0; i < grid.size(); ++i) {
        grid.apply_stock_fresh_plot_type(i, 1, false, false, true,
                                         {0, false, false});
        grid.apply_stock_fresh_terrain_type(i, {0, false, false},
                                            false, false);
    }
    const auto areas = grid.draft_fresh_area_structure(1);
    BnwFreshYieldRules yield_rules;
    yield_rules.terrains[0].base[0] = 2;
    const auto yields = replay_bnw_fresh_yield_passes(grid, areas, yield_rules);
    grid.apply_fresh_area_handles(areas, yields.final_cache,
                                  yields.symbol_events);
    const auto original_grid = grid;

    civ5::content::RulesDatabase rules;
    rules.apply_sql(R"sql(
        CREATE TABLE Features (ID INTEGER PRIMARY KEY, Impassable INTEGER,
                               Rough INTEGER, NaturalWonder INTEGER);
        INSERT INTO Features VALUES (7, 1, 1, 0);
    )sql");
    const auto info = resolve_bnw_fresh_feature_set_info(rules, 7);
    BnwSelectedFeatureEligibility eligible;
    eligible.source_rules = &rules;
    eligible.id = 7;
    eligible.terrain_ids = {0};
    const std::vector<std::int32_t> features(grid.size(), -1);
    const auto first = inspect_bnw_fresh_feature_set(
        grid, areas, features, 0, 7, 42, eligible, info, 1);
    const auto second = inspect_bnw_fresh_feature_set(
        grid, areas, features, 1, 7, -1, eligible, info, 1);
    check(first.blocker == BnwFreshFeatureSetBlocker::required_services_pending &&
              second.blocker == BnwFreshFeatureSetBlocker::required_services_pending,
          "Both selected plot transitions must reach the SDK callback boundary");

    BnwFreshFeatureEventJournal journal(2);
    const std::string rules_sha =
        identify_bytes(rules.gameplay_fingerprint_data()).sha256;
    const std::string script_sha(64, 'b');
    bool invalid_identity_rejected = false;
    try { journal.stage_prewrite(first, grid, features, info,
                                 std::string(63, 'a'), script_sha); }
    catch (const std::invalid_argument &) { invalid_identity_rejected = true; }
    check(invalid_identity_rejected && journal.size() == 0,
          "A detached event cannot be staged without complete source identity");
    journal.stage_prewrite(first, grid, features, info, rules_sha, script_sha);
    bool duplicate_rejected = false;
    try { journal.stage_prewrite(first, grid, features, info,
                                 rules_sha, script_sha); }
    catch (const std::invalid_argument &) { duplicate_rejected = true; }
    check(duplicate_rejected && journal.size() == 1,
          "A repeated pre-write callback for one unchanged plot must be rejected");
    journal.stage_prewrite(second, grid, features, info, rules_sha, script_sha);
    check(journal.size() == 2 && journal.replay_from(0).size() == 2 &&
              journal.replay_from(1).size() == 1 &&
              journal.replay_from(0)[0].sequence() == 0 &&
              journal.replay_from(0)[1].sequence() == 1 &&
              journal.replay_from(0)[0].plot_index() == 0 &&
              journal.replay_from(0)[1].plot_index() == 1 &&
              journal.replay_from(0)[0].old_feature() == -1 &&
              journal.replay_from(0)[0].new_feature() == 7 &&
              journal.replay_from(0)[0].variety() == 0 &&
              journal.replay_from(0)[0].source_rules() == &rules &&
              journal.replay_from(0)[0].rules_sha256() == rules_sha &&
              journal.replay_from(0)[0].map_source_sha256() == script_sha,
          "Pre-write events need selected provenance and stable source order");
    bool budget_rejected = false;
    try { journal.stage_prewrite(first, grid, features, info,
                                 rules_sha, script_sha); }
    catch (const std::length_error &) { budget_rejected = true; }
    check(budget_rejected && journal.size() == 2,
          "Event budget must reject further writes before any publication");
    check(journal.acknowledge_headless(0) ==
              BnwFreshFeatureEventAck::callback_effect_unverified &&
              journal.acknowledge_headless(2) ==
              BnwFreshFeatureEventAck::invalid_sequence &&
              journal.replay_from(0).size() == 2,
          "Headless callback must leave the ordered replay unacknowledged");

    grid.reset(1, 1, false, false);
    check(journal.replay_from(0)[0].before().x == 0 &&
              journal.replay_from(0)[1].before().x == 1 &&
              journal.replay_from(0)[0].before().terrain_type == 0 &&
              journal.replay_from(0)[0].before().yields[0] == 2,
          "Pre-write views must be owned snapshots, not aliases of the map grid");
    journal.rollback();
    check(journal.size() == 0 && journal.replay_from(0).empty() &&
              journal.acknowledge_headless(0) ==
                  BnwFreshFeatureEventAck::invalid_sequence,
          "Failed map transaction must discard events and replay cursor");
    bool stale_view_rejected = false;
    try { journal.stage_prewrite(first, grid, features, info,
                                 rules_sha, script_sha); }
    catch (const std::invalid_argument &) { stale_view_rejected = true; }
    check(stale_view_rejected && journal.size() == 0,
          "An event cannot reuse a preflight after its owned grid changes");
    journal.stage_prewrite(first, original_grid, features, info,
                           rules_sha, script_sha);
    check(journal.replay_from(0)[0].sequence() == 0,
          "A retried detached transaction must restart its event sequence");
}
} // namespace

int main() {
    test_prewrite_journal();
}
