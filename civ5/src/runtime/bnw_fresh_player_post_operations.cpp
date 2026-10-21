#include "civ5/runtime/bnw_fresh_player_post_operations.hpp"
#include <algorithm>
#include <utility>

namespace civ5::runtime {

void BnwFreshPlayerPostOperationsDraft::rollback() noexcept {
    state = {};
    source_order.clear();
    selected_rules_sha256.clear();
    blocker = BnwFreshPlayerPostOperationsBlocker::none;
    blocked_at = {};
    detached_effect_complete = false;
    archive_clear_delta_verified = false;
    archive_traversal_verified = false;
    initialized_by_gamecore = false;
}

BnwFreshPlayerPostOperationsDraft project_bnw_fresh_player_post_operations(
    const BnwFreshFirstPlayerObjects &objects,
    const BnwFreshPlayerOperationsDraft &predecessor,
    std::string_view selected_rules_sha256,
    BnwFreshPlayerPostOperationsState initial) {
    BnwFreshPlayerPostOperationsDraft result;
    if (!objects.constructors_complete || objects.initialized_by_gamecore ||
        objects.player_id != 0 || selected_rules_sha256.empty() ||
        objects.selected_rules_sha256 != selected_rules_sha256 ||
        predecessor.selected_rules_sha256 != selected_rules_sha256 ||
        !predecessor.detached_effect_complete ||
        !predecessor.state.mapped_pointer_nonnull.empty() ||
        predecessor.source_order.empty() ||
        predecessor.source_order.back().kind !=
            BnwFreshPlayerOperationsEventKind::map_clear ||
        predecessor.archive_traversal_verified ||
        predecessor.initialized_by_gamecore) {
        result.blocker =
            BnwFreshPlayerPostOperationsBlocker::predecessor_or_rules_invalid;
        result.blocked_at = "m_aiPlots.clear";
        return result;
    }
    if (initial.plot_indices.size() >
        BnwFreshPlayerPostOperationsState::kPlotsInitialCapacity) {
        result.blocker =
            BnwFreshPlayerPostOperationsBlocker::plot_vector_capacity_unverified;
        result.blocked_at = "m_aiPlots.clear";
        return result;
    }
    result.state = std::move(initial);
    result.state.plot_indices.clear();
    result.source_order.push_back({
        BnwFreshPlayerPostOperationsEventKind::plots_size_zero,
        "m_aiPlots.clear"});
    result.state.ever_conquered_by_words.fill(0);
    result.source_order.push_back({
        BnwFreshPlayerPostOperationsEventKind::conquered_by_words_zero,
        "m_bfEverConqueredBy.ClearAll"});
    result.selected_rules_sha256 = std::string(selected_rules_sha256);
    result.detached_effect_complete = true;
    return result;
}

} // namespace civ5::runtime
