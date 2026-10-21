#pragma once

#include "civ5/runtime/bnw_fresh_player_tail_containers.hpp"
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace civ5::runtime {

// FFreeListTrashArray<CvCityAI/CvUnit> state relevant to the first fresh
// CvPlayer::uninit. An occupied slot requires the object's destructor and is
// deliberately outside this empty-container projection.
struct BnwFreshFreeListNode {
    std::int32_t next_free_index = -1;
    bool occupied = false;
    bool operator==(const BnwFreshFreeListNode &) const = default;
};
struct BnwFreshFreeListState {
    std::int32_t free_list_head = -2;
    std::int32_t free_list_count = 0;
    std::int32_t last_index = -1;
    std::int32_t num_slots = 0;
    std::int32_t current_id = 8192;
    bool array_allocated = false;
    std::vector<BnwFreshFreeListNode> nodes;
    bool operator==(const BnwFreshFreeListState &) const = default;
};

// FFreeListArrayBase ctor and FFreeListTrashArray ctor, or Init() with its
// default eight slots. These are the two source-proven first-game states.
BnwFreshFreeListState make_bnw_fresh_unallocated_free_list();
BnwFreshFreeListState make_bnw_fresh_initialized_free_list();

enum class BnwFreshPlayerEntitiesBlocker {
    none,
    predecessor_or_rules_invalid,
    city_container_state_unverified,
    city_destructor_effects_pending,
    unit_container_state_unverified,
    unit_destructor_effects_pending,
};
struct BnwFreshPlayerEntitiesEvent {
    std::string_view sdk_call;
    bool array_deleted;
    bool operator==(const BnwFreshPlayerEntitiesEvent &) const = default;
};
struct BnwFreshPlayerEntitiesDraft {
    BnwFreshFreeListState cities;
    BnwFreshFreeListState units;
    std::vector<BnwFreshPlayerEntitiesEvent> source_order;
    std::string selected_rules_sha256;
    BnwFreshPlayerEntitiesBlocker blocker = BnwFreshPlayerEntitiesBlocker::none;
    std::string_view blocked_at;
    bool detached_effects_complete = false;
    bool archive_traversal_verified = false;
    bool initialized_by_gamecore = false;
    void rollback() noexcept;
};

// Expansion2 CvPlayer.cpp:715,717. Uninit on an unallocated list is a no-op;
// on an initialized empty list RemoveAll resets counters/slots before
// SAFE_DELETE_ARRAY. A populated list fails atomically before any deletion.
BnwFreshPlayerEntitiesDraft project_bnw_fresh_player_entities_uninit(
    const BnwFreshFirstPlayerObjects &objects,
    const BnwFreshPlayerTailContainersDraft &predecessor,
    std::string_view selected_rules_sha256,
    BnwFreshFreeListState cities,
    BnwFreshFreeListState units);

} // namespace civ5::runtime
