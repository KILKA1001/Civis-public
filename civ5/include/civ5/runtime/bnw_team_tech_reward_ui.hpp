#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace civ5::runtime {

// CvTeam::setHasTech, Expansion2 CvTeam.cpp:5676-5708. This plan starts
// after the three active-team dirty bits and does not attest that the native
// DLLUI option query or the subsequent GameplayTechAcquired call happened.
struct BnwTechRewardUiInput {
    std::int64_t team=-1, technology=-1, active_team=-1;
    bool final_initialized=false, new_value=false;
    // Must come from a live DLLUI-equivalent owner, not OptionsManager's
    // cached Lua setting or an assumed default.
    std::optional<bool> observed_no_reward_popups;
    // GC.getGame().isNetworkMultiPlayer(); unknown is allowed only when both
    // branches have the same observable effects.
    std::optional<bool> network_multiplayer;
    struct MajorPlayer {
        std::int64_t id=-1, team=-1;
        bool alive=false, has_notification_owner=false;
    };
    std::vector<MajorPlayer> major_players_in_slot_order;
};

enum class BnwTechRewardUiStatus { blocked, planned };
enum class BnwTechRewardUiKind { notification, popup };
struct BnwTechRewardUiIntent {
    BnwTechRewardUiKind kind=BnwTechRewardUiKind::notification;
    std::int64_t player=-1, team=-1, technology=-1;
    // The notification owner supplies the localized text before execution.
    // A popup uses BUTTONPOPUP_TECH_AWARD and the active player.
};
struct BnwTechRewardUiPlan {
    BnwTechRewardUiStatus status=BnwTechRewardUiStatus::blocked;
    bool native_option_query_required=false;
    bool native_option_query_observed=false;
    bool network_mode_observed=false;
    std::vector<BnwTechRewardUiIntent> intents;
    std::string blocker;
    // Execution and durable callback receipts are owned by later stages.
    bool effects_executed=false, engine_callback_confirmed=false;
};

BnwTechRewardUiPlan plan_bnw_team_tech_reward_ui(
    const BnwTechRewardUiInput &input);

} // namespace civ5::runtime
