#pragma once

#include "civ5/runtime/bnw_turn_scheduler.hpp"
#include "civ5/runtime/treasury_turn.hpp"
#include <functional>
#include <optional>

namespace civ5::runtime {

struct BnwPlayerGoldRules {
    const TreasuryRules &treasury;
    const CityYieldRules &cities;
    const MaintenanceRules &maintenance;
    const content::GlobalDefineCache &defines;
    std::int64_t active_player=-1;
};

// The Lua income query must be observational. Only DoDeficit's proven
// no-disband branch is available; unit selection, gameplay RNG, scrap and
// notifications still require live services.
struct BnwPlayerGoldServices {
    std::function<bool()> lua_ready;
    std::function<std::optional<std::int64_t>(
        std::int64_t,const LiveSessionState &)> founder_benefits_read_only;
    std::function<void(std::int64_t,TreasuryDirty)> dirty_after_commit;
    // Attest that unit and city collections reflect the live world at the
    // DoDeficit call. With both current, the no-disband branch can be proved
    // directly from CvPlayer::DoDeficit without military AI or RNG.
    bool deficit_unit_roster_current=false;
    bool deficit_city_roster_current=false;
};

// CvPlayer::doTurnPostDiplomacy, Expansion2 CvPlayer.cpp:4186, calls
// CvTreasury::DoGold (CvTreasury.cpp:70). Call this phase ONLY on detached
// LiveSessionState through apply_live_action or the turn scheduler. A rejected
// DoDeficit can leave that detached copy partly changed; its caller must
// discard it. Active UI dirty intents are published by the scheduler handler
// only after commit. Steam achievement unlock is outside this BNW milestone.
LiveActionResult run_bnw_player_gold_phase(
    LiveSessionState &state,std::int64_t player,
    const BnwPlayerGoldRules &rules,const BnwPlayerGoldServices &services);
BnwTurnHandler make_bnw_player_gold_handler(
    BnwPlayerGoldRules rules,BnwPlayerGoldServices services);

LiveActionResult publish_bnw_player_gold(
    LiveSession &session,const BnwPlayerGoldServices &services);

// Standalone diagnostic execution; this does not complete a player turn.
LiveActionResult apply_bnw_player_gold(
    LiveSession &session,std::int64_t player,
    const BnwPlayerGoldRules &rules,const BnwPlayerGoldServices &services);

// Requires caller handlers for every other player phase; never supplies a
// successful placeholder. Publishes DoGold UI intents only after full commit.
LiveActionResult run_bnw_player_turn_with_gold(
    LiveSession &session,std::int64_t player,
    std::span<const BnwTurnHandler> other_handlers,
    const BnwPlayerGoldRules &rules,const BnwPlayerGoldServices &services);

} // namespace civ5::runtime
