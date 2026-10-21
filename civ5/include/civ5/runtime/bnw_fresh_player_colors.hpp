#pragma once

#include "civ5/runtime/bnw_fresh_pregame_roster.hpp"
#include <cstdint>
#include <string>
#include <vector>

namespace civ5::runtime {

struct BnwFreshMajorColorSlot {
    BnwFreshSlotStatus status = BnwFreshSlotStatus::closed;
    std::int32_t civilization_id = -1;
    std::int32_t chosen_color_id = -1; // NO_PLAYERCOLOR
};
enum class BnwFreshColorsBlockerCode {
    selected_rules_unverified,
    initial_colors_incomplete,
    selected_color_info_missing,
    remaining_init_players_pending,
};
struct BnwFreshColorsBlocker {
    BnwFreshColorsBlockerCode code;
    std::string message;
};
struct BnwFreshPlayerColorsDraft {
    std::vector<std::int32_t> major_colors;
    std::int32_t barbarian_color = -1;
    std::string selected_rules_sha256;
    std::vector<BnwFreshColorsBlocker> blockers;
    bool major_colors_complete = false;
    bool has_blocker(BnwFreshColorsBlockerCode code) const;
    void rollback() noexcept;
};

// CvGame::InitPlayers major color loops before the pregame slot writes.
// The barbarian civilization ID must come from the selected GameCore info
// cache, and all default colors resolve against selected Civilizations and
// PlayerColors rows. No player is initialized or published by this stage.
BnwFreshPlayerColorsDraft draft_bnw_fresh_player_colors(
    const BnwFreshGameContentResult &selected,
    std::string_view selected_rules_sha256,
    std::span<const BnwFreshMajorColorSlot> major_slots,
    std::int32_t barbarian_civilization_id,
    std::uint64_t maximum_player_colors = 4096);

} // namespace civ5::runtime
