#pragma once

#include "civ5/runtime/bnw_deficit_selection.hpp"
#include "civ5/runtime/live_action_transaction.hpp"
#include <map>
#include <optional>

namespace civ5::runtime {

struct BnwDeficitScrapRules {
    // These values must be bound to the selected BNW rules database before use.
    std::map<std::uint32_t,std::int64_t> production_cost_by_unit_hash;
    std::int64_t refund_percent=-1,train_percent=-1,max_hit_points=-1;
    bool unit_delete_disabled=true;
};
struct BnwDeficitScrapPlot {
    // Both values require a live plot binding; the saved map projection alone
    // does not establish current ownership or combat state after load hooks.
    std::optional<std::int64_t> x,y,owner;
    std::optional<bool> unit_fighting;
};
struct BnwDeficitScrapDraft {
    std::int64_t refund_gold=0;
    std::int64_t unit_id=-1;
};

// Expansion2 CvUnit.cpp:3231 scrap() and 3252 GetScrapGold. This mutates ONLY
// a detached LiveSessionState copy: the authoritative treasury gets the refund
// and the selected unit's owned m_bDeathDelay becomes true. It supports a unit
// with no transported cargo. The caller must discard this draft until the
// preceding DoDeficit notification and kill(true)'s CanSaveUnit/UnitPrekill Lua
// callbacks are implemented. Later doDelayedDeath/kill(false) and its army,
// plot, resource, maintenance and UI effects are entirely absent here.
BnwDeficitScrapDraft stage_bnw_deficit_scrap_draft(
    LiveSessionState &detached,std::int64_t player,
    const BnwDeficitCandidate &selected,const BnwDeficitScrapRules &rules,
    const BnwDeficitScrapPlot &plot);

// Diagnostic transaction: validates and stages the draft, then rejects it.
// No gold, death flag or RNG can escape to the live session yet.
LiveActionResult try_bnw_deficit_scrap_draft(
    LiveSession &session,std::int64_t player,
    const BnwDeficitCandidate &selected,const BnwDeficitScrapRules &rules,
    const BnwDeficitScrapPlot &plot);

} // namespace civ5::runtime
