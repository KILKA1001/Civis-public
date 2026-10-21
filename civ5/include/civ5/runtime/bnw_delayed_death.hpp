#pragma once

#include "civ5/runtime/bnw_deficit_selection.hpp"
#include "civ5/runtime/live_action_transaction.hpp"
#include <optional>

namespace civ5::runtime {

struct BnwDelayedDeathLiveChecks {
    // CvUnit::doDelayedDeath queries live combat city/unit and mission state;
    // the saved fields alone cannot establish either predicate after restore.
    std::optional<bool> fighting,busy;
};
struct BnwDelayedDeathDraft {
    std::int64_t unit_id=-1;
    std::uint64_t source_index=0;
};

// Expansion2 CvUnit.cpp:16312 doDelayedDeath and 1166 kill(false), with
// CvPlayer.cpp:19172 deleteUnit. Only a structurally simple delayed unit may
// be removed from a DETACHED owned unit collection and maintenance view.
// This is not a completed kill: source allocator/header, plot, class counts,
// army, cargo, resource, cache, capture, UI and Lua effects are absent.
BnwDelayedDeathDraft stage_bnw_delayed_death_draft(
    LiveSessionState &detached,std::int64_t player,
    const BnwDeficitCandidate &selected,
    const BnwDelayedDeathLiveChecks &checks);

// Always rejects after staging on the transaction's detached copy. A live
// session must never publish this partial state or spend gameplay RNG.
LiveActionResult try_bnw_delayed_death_draft(
    LiveSession &session,std::int64_t player,
    const BnwDeficitCandidate &selected,
    const BnwDelayedDeathLiveChecks &checks);

} // namespace civ5::runtime
