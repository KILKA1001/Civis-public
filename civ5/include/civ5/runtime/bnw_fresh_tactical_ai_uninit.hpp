#pragma once

#include "civ5/runtime/bnw_fresh_flavor_manager_uninit.hpp"
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace civ5::runtime {

// The opaque payload is a detached witness for other CvTacticalAI members;
// it is never interpreted as a live native object. The SDK Uninit is empty.
struct BnwFreshTacticalAiServiceState {
    std::int32_t current_zone_index = -1;
    std::vector<std::byte> opaque_member_snapshot;
    bool operator==(const BnwFreshTacticalAiServiceState &) const = default;
};

struct BnwFreshTacticalAiUninitDraft {
    BnwFreshTacticalAiServiceState state;
    std::vector<std::string_view> source_order;
    std::string selected_rules_sha256;
    bool member_effect_complete = false;
    bool archive_predecessor_verified = false;
    bool remaining_services_complete = false;
    bool initialized_by_gamecore = false;
    void rollback() noexcept;
};

BnwFreshTacticalAiServiceState make_bnw_fresh_tactical_ai_constructor_state();

// Expansion2 CvPlayer.cpp:683 and CvTacticalAI.cpp:318-320. Uninit neither
// invokes Reset nor alters the owner. Archive traversal and HomelandAI/later
// services remain blocked.
BnwFreshTacticalAiUninitDraft project_bnw_fresh_tactical_ai_uninit(
    const BnwFreshFirstPlayerObjects &objects,
    const BnwFreshFullArchiveProjection &archive,
    const BnwFreshFlavorManagerUninitDraft &flavor_manager,
    std::string_view selected_rules_sha256,
    BnwFreshTacticalAiServiceState initial);

} // namespace civ5::runtime
