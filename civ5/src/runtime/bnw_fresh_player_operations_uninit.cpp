#include "civ5/runtime/bnw_fresh_player_operations_uninit.hpp"
#include <algorithm>
#include <utility>

namespace civ5::runtime {

void BnwFreshPlayerOperationsDraft::rollback() noexcept {
    state = {};
    source_order.clear();
    selected_rules_sha256.clear();
    blocker = BnwFreshPlayerOperationsBlocker::none;
    blocked_at = {};
    detached_effect_complete = false;
    archive_traversal_verified = false;
    initialized_by_gamecore = false;
}

BnwFreshPlayerOperationsDraft project_bnw_fresh_player_operations_uninit(
    const BnwFreshFirstPlayerObjects &objects,
    const BnwFreshPlayerEntitiesDraft &predecessor,
    std::string_view selected_rules_sha256,
    BnwFreshPlayerOperationsState initial) {
    BnwFreshPlayerOperationsDraft result;
    if (!objects.constructors_complete || objects.initialized_by_gamecore ||
        objects.player_id != 0 || selected_rules_sha256.empty() ||
        objects.selected_rules_sha256 != selected_rules_sha256 ||
        predecessor.selected_rules_sha256 != selected_rules_sha256 ||
        !predecessor.detached_effects_complete ||
        predecessor.source_order.size() != 2 ||
        predecessor.source_order[0].sdk_call != "m_cities.Uninit" ||
        predecessor.source_order[1].sdk_call != "m_units.Uninit" ||
        predecessor.archive_traversal_verified ||
        predecessor.initialized_by_gamecore) {
        result.blocker =
            BnwFreshPlayerOperationsBlocker::predecessor_or_rules_invalid;
        result.blocked_at = "m_AIOperations";
        return result;
    }
    if (std::any_of(initial.mapped_pointer_nonnull.begin(),
                    initial.mapped_pointer_nonnull.end(),
                    [](const auto &entry) { return entry.second; })) {
        result.blocker =
            BnwFreshPlayerOperationsBlocker::live_operation_destructor_pending;
        result.blocked_at = "delete(iter->second)";
        return result;
    }

    result.state = std::move(initial);
    for (const auto &[key,nonnull] : result.state.mapped_pointer_nonnull) {
        (void)nonnull;
        result.source_order.push_back({
            BnwFreshPlayerOperationsEventKind::delete_null_pointer,key});
    }
    result.state.mapped_pointer_nonnull.clear();
    result.source_order.push_back({
        BnwFreshPlayerOperationsEventKind::map_clear,0});
    result.selected_rules_sha256 = std::string(selected_rules_sha256);
    result.detached_effect_complete = true;
    return result;
}

} // namespace civ5::runtime
