#pragma once

#include "civ5/runtime/bnw_fresh_player_entities_uninit.hpp"
#include <cstdint>
#include <map>
#include <string>
#include <string_view>
#include <vector>

namespace civ5::runtime {

// Detached projection of std::map<int,CvAIOperation*>. A true value means a
// non-null operation pointer, whose virtual destructor is not projected here.
struct BnwFreshPlayerOperationsState {
    std::map<std::int32_t,bool> mapped_pointer_nonnull;
    bool operator==(const BnwFreshPlayerOperationsState &) const = default;
};

enum class BnwFreshPlayerOperationsBlocker {
    none,
    predecessor_or_rules_invalid,
    live_operation_destructor_pending,
};
enum class BnwFreshPlayerOperationsEventKind {
    delete_null_pointer,
    map_clear,
};
struct BnwFreshPlayerOperationsEvent {
    BnwFreshPlayerOperationsEventKind kind;
    std::int32_t key;
    bool operator==(const BnwFreshPlayerOperationsEvent &) const = default;
};
struct BnwFreshPlayerOperationsDraft {
    BnwFreshPlayerOperationsState state;
    std::vector<BnwFreshPlayerOperationsEvent> source_order;
    std::string selected_rules_sha256;
    BnwFreshPlayerOperationsBlocker blocker =
        BnwFreshPlayerOperationsBlocker::none;
    std::string_view blocked_at;
    bool detached_effect_complete = false;
    bool archive_traversal_verified = false;
    bool initialized_by_gamecore = false;
    void rollback() noexcept;
};

// Expansion2 CvPlayer.cpp:719-725 iterates std::map in key order, deletes
// every mapped pointer, then clears the map. The constructor's empty map is
// the first-game case. delete nullptr is also source-exact and harmless;
// non-null pointers fail before any preceding map entry is published.
BnwFreshPlayerOperationsDraft project_bnw_fresh_player_operations_uninit(
    const BnwFreshFirstPlayerObjects &objects,
    const BnwFreshPlayerEntitiesDraft &predecessor,
    std::string_view selected_rules_sha256,
    BnwFreshPlayerOperationsState initial);

} // namespace civ5::runtime
