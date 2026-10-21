#pragma once

#include "civ5/runtime/bnw_fresh_player_colors.hpp"
#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace civ5::runtime {

enum class BnwFreshSlotWriteField : std::uint8_t {
    team, status, net_id, handicap, civilization, leader,
    player_color, minor_civ,
};
struct BnwFreshSlotWriteEvent {
    std::int32_t player_id;
    BnwFreshSlotWriteField field;
    std::int32_t value;
    bool operator==(const BnwFreshSlotWriteEvent &) const = default;
};
enum class BnwFreshSlotInfoBlockerCode {
    roster_colors_or_rules_unverified,
    selected_gamecore_constants_unverified,
    selected_info_unverified,
    minor_pregame_types_missing,
    player_init_pending,
};
struct BnwFreshSlotInfoBlocker {
    BnwFreshSlotInfoBlockerCode code;
    std::string message;
};
struct BnwFreshSlotInfoState {
    BnwFreshPregameSlot slot;
    std::optional<std::int32_t> net_id, handicap, civilization;
    std::optional<std::int32_t> leader, player_color;
    std::optional<bool> minor_civ;
};
struct BnwFreshSlotInfoDraft {
    std::vector<BnwFreshSlotInfoState> slots;
    std::vector<BnwFreshSlotWriteEvent> source_order;
    std::string selected_rules_sha256;
    std::vector<BnwFreshSlotInfoBlocker> blockers;
    bool pregame_writes_complete = false;
    bool initialized_by_gamecore = false;
    bool has_blocker(BnwFreshSlotInfoBlockerCode code) const;
    void rollback() noexcept;
};

// Detached CvGame::InitPlayers per-player CvPreGame setter loop. IDs passed
// through native_constants are the selected GameCore cached constants, not
// inferred from table ordering. The active minor IDs are the selected
// CvPreGame::minorCivType values in ascending active-minor slot order.
BnwFreshSlotInfoDraft draft_bnw_fresh_slot_info(
    const BnwFreshGameContentResult &selected,
    const BnwFreshPregameRosterDraft &roster,
    const BnwFreshPlayerColorsDraft &colors,
    std::string_view selected_rules_sha256,
    std::span<const content::GameplayDefine> native_constants,
    std::span<const std::int32_t> active_minor_civ_ids);

} // namespace civ5::runtime
