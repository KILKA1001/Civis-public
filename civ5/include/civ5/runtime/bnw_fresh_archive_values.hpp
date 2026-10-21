#pragma once

#include "civ5/runtime/bnw_fresh_player_objects.hpp"
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace civ5::runtime {

// Four different registered CvPlayer auto-variable types, before
// CvPlayer::uninit writes its own values. This is an owned projection; it is
// never an assertion that the unavailable FAutoArchive::reset implementation
// traversed its m_contents or cleared archive-level deltas.
struct BnwFreshArchiveObservableValues {
    std::int32_t starting_x = 0;       // FAutoVariable<int>
    bool alive = false;                 // FAutoVariable<bool>
    std::int32_t player_id = 0;        // FAutoVariable<PlayerTypes>
    std::vector<std::int32_t> city_yield_change; // FAutoVariable<vector<int>>
    std::vector<std::int32_t> resource_used;     // FAutoVector<int>
    std::uint32_t resource_pending_commands = 0;
    bool operator==(const BnwFreshArchiveObservableValues &) const = default;
};
struct BnwFreshArchiveObservableReset {
    BnwFreshArchiveObservableValues values;
    std::vector<std::string_view> registered_members;
    std::string selected_rules_sha256;
    bool member_defaults_complete = false;
    bool archive_traversal_verified = false;
    bool uninit_complete = false;
    bool initialized_by_gamecore = false;
    void rollback() noexcept;
};

// Source contracts: CvPlayer.cpp constructor registrations at 129, 316,
// 333, 335, 352; FAutoVariable.h:310-313 value initialization and
// FAutoVector.h:385-390 command/value clear. These five independent member
// effects are exact. CvPlayer::reset calls aggregate archive.reset before
// uninit; no partial projection is published as that aggregate call.
BnwFreshArchiveObservableReset project_bnw_fresh_archive_observable_reset(
    const BnwFreshFirstPlayerObjects &objects,
    std::string_view selected_rules_sha256,
    BnwFreshArchiveObservableValues initial);

} // namespace civ5::runtime
