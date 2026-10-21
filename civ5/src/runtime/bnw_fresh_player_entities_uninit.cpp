#include "civ5/runtime/bnw_fresh_player_entities_uninit.hpp"
#include <algorithm>
#include <utility>

namespace civ5::runtime {
namespace {

constexpr std::int32_t kInvalidIndex = -1;
constexpr std::int32_t kFreeListIndex = -2;
constexpr std::int32_t kMaxBuckets = 8192;
constexpr std::int32_t kInitialSlots = 8;

bool verified_fresh_shape(const BnwFreshFreeListState &state) {
    if (state.current_id != kMaxBuckets || state.last_index != kInvalidIndex ||
        state.free_list_count != 0) return false;
    if (!state.array_allocated)
        return state.nodes.empty() &&
            ((state.num_slots == 0 && state.free_list_head == kFreeListIndex) ||
             (state.num_slots == kInitialSlots &&
              state.free_list_head == kInvalidIndex));
    return state.num_slots == kInitialSlots &&
        state.free_list_head == kInvalidIndex &&
        state.nodes.size() == static_cast<std::size_t>(kInitialSlots) &&
        std::all_of(state.nodes.begin(),state.nodes.end(),
            [](const BnwFreshFreeListNode &node) {
                return node.next_free_index == kInvalidIndex;
            });
}

bool occupied(const BnwFreshFreeListState &state) {
    return std::any_of(state.nodes.begin(),state.nodes.end(),
        [](const BnwFreshFreeListNode &node) { return node.occupied; });
}

BnwFreshPlayerEntitiesDraft failed(BnwFreshPlayerEntitiesBlocker blocker,
                                   std::string_view at) {
    BnwFreshPlayerEntitiesDraft result;
    result.blocker = blocker;
    result.blocked_at = at;
    return result;
}

void uninit_empty(BnwFreshFreeListState &state) {
    if (!state.array_allocated) return;
    // FFreeListTrashArray::RemoveAll, then SAFE_DELETE_ARRAY. Neither
    // m_iNumSlots nor m_iCurrentID is reset by Uninit.
    state.last_index = kInvalidIndex;
    state.free_list_head = kInvalidIndex;
    state.free_list_count = 0;
    for (auto &node : state.nodes) node = {};
    state.nodes.clear();
    state.array_allocated = false;
}

} // namespace

BnwFreshFreeListState make_bnw_fresh_unallocated_free_list() {
    return {};
}

BnwFreshFreeListState make_bnw_fresh_initialized_free_list() {
    BnwFreshFreeListState result;
    result.free_list_head = kInvalidIndex;
    result.num_slots = kInitialSlots;
    result.array_allocated = true;
    result.nodes.resize(kInitialSlots);
    return result;
}

void BnwFreshPlayerEntitiesDraft::rollback() noexcept {
    cities = {};
    units = {};
    source_order.clear();
    selected_rules_sha256.clear();
    blocker = BnwFreshPlayerEntitiesBlocker::none;
    blocked_at = {};
    detached_effects_complete = false;
    archive_traversal_verified = false;
    initialized_by_gamecore = false;
}

BnwFreshPlayerEntitiesDraft project_bnw_fresh_player_entities_uninit(
    const BnwFreshFirstPlayerObjects &objects,
    const BnwFreshPlayerTailContainersDraft &predecessor,
    std::string_view selected_rules_sha256,
    BnwFreshFreeListState cities,
    BnwFreshFreeListState units) {
    if (!objects.constructors_complete || objects.initialized_by_gamecore ||
        objects.player_id != 0 || selected_rules_sha256.empty() ||
        objects.selected_rules_sha256 != selected_rules_sha256 ||
        predecessor.selected_rules_sha256 != selected_rules_sha256 ||
        predecessor.archive.selected_rules_sha256 != selected_rules_sha256 ||
        !predecessor.archive.all_registered_member_defaults ||
        predecessor.archive.aggregate_traversal_verified ||
        !predecessor.detached_container_effects_complete ||
        predecessor.archive_touch_intents_materialized ||
        predecessor.archive_traversal_verified ||
        predecessor.cities_uninit_complete ||
        predecessor.initialized_by_gamecore)
        return failed(BnwFreshPlayerEntitiesBlocker::predecessor_or_rules_invalid,
                      "m_cities.Uninit");
    if (!verified_fresh_shape(cities))
        return failed(BnwFreshPlayerEntitiesBlocker::city_container_state_unverified,
                      "m_cities.Uninit");
    if (occupied(cities))
        return failed(BnwFreshPlayerEntitiesBlocker::city_destructor_effects_pending,
                      "m_cities.Uninit");
    if (!verified_fresh_shape(units))
        return failed(BnwFreshPlayerEntitiesBlocker::unit_container_state_unverified,
                      "m_units.Uninit");
    if (occupied(units))
        return failed(BnwFreshPlayerEntitiesBlocker::unit_destructor_effects_pending,
                      "m_units.Uninit");

    BnwFreshPlayerEntitiesDraft result;
    const bool city_array_deleted = cities.array_allocated;
    const bool unit_array_deleted = units.array_allocated;
    uninit_empty(cities);
    result.source_order.push_back({"m_cities.Uninit",city_array_deleted});
    uninit_empty(units);
    result.source_order.push_back({"m_units.Uninit",unit_array_deleted});
    result.cities = std::move(cities);
    result.units = std::move(units);
    result.selected_rules_sha256 = std::string(selected_rules_sha256);
    result.detached_effects_complete = true;
    return result;
}

} // namespace civ5::runtime
