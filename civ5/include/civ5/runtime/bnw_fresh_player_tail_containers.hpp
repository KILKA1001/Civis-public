#pragma once

#include "civ5/runtime/bnw_fresh_post_connections_uninit.hpp"
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace civ5::runtime {

enum class BnwFreshTailEventKind {
    archive_touch_intent,
    vector_clear_command,
    vector_value_clear,
    unit_cycle_list_clear,
    research_queue_clear,
    city_names_clear,
};
struct BnwFreshTailEvent {
    BnwFreshTailEventKind kind;
    std::string_view sdk_member;
    bool operator==(const BnwFreshTailEvent &) const = default;
};
struct BnwFreshPlayerTailContainersState {
    std::vector<std::int32_t> unit_cycle_ids;
    std::vector<std::int32_t> research_queue;
    std::vector<std::string> city_names;
    bool operator==(const BnwFreshPlayerTailContainersState &) const = default;
};
struct BnwFreshPlayerTailContainersDraft {
    BnwFreshFullArchiveProjection archive;
    BnwFreshPlayerTailContainersState state;
    std::vector<BnwFreshTailEvent> source_order;
    std::string selected_rules_sha256;
    bool detached_container_effects_complete = false;
    bool archive_touch_intents_materialized = false;
    bool archive_traversal_verified = false;
    bool cities_uninit_complete = false;
    bool initialized_by_gamecore = false;
    void rollback() noexcept;
};

// Expansion2 CvPlayer.cpp:705-713. FAutoVector::clear touches archive, adds
// one CLEAR command, then clears value, even if already empty. The three
// archive touches remain staged intents because archive-level reset/delta
// traversal is opaque. CvUnitCycler::Clear and CLinkList::clear free nodes.
// m_cities.Uninit at line 715 is the next source-order blocker.
BnwFreshPlayerTailContainersDraft project_bnw_fresh_player_tail_containers(
    const BnwFreshFirstPlayerObjects &objects,
    const BnwFreshFullArchiveProjection &archive,
    const BnwFreshPostConnectionsUninitDraft &post_connections,
    std::string_view selected_rules_sha256,
    BnwFreshPlayerTailContainersState initial);

} // namespace civ5::runtime
