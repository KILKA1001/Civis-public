#pragma once

#include "civ5/runtime/bnw_map_turn.hpp"
#include <optional>
#include <string>

namespace civ5::runtime {

struct BnwUnitVisibilityDecision {
    std::optional<bool> value;
    std::string reason;
};

// CvPlot::isVisibleEnemyUnit(pUnit) walks the plot's ordered live unit list.
// This quote uses the saved/owned roster, teams, unit invisibility and war
// state in that order. An unprovable live input returns nullopt, not false.
BnwUnitVisibilityDecision quote_bnw_visible_enemy_unit(
    const LiveSessionState &state, std::uint64_t plot,
    saves::SavedPlotUnitReference handle);

// CvUnit::isInvisible(plot.getTeam(), false), including cargo and the plot's
// per-team invisible-detection counts. Only call with a real team (the source
// second pass calls it after isOwned()).
BnwUnitVisibilityDecision quote_bnw_unit_invisible(
    const LiveSessionState &state, std::uint64_t plot,
    saves::SavedPlotUnitReference handle, std::int64_t observing_team);

// Direct adapters for BnwPlotUnitReadServices. A relocation identified by
// the caller's preflight still requires transactional jump/kill services;
// these read-only callbacks never authorize or perform those effects.
BnwUnitValidityProbe make_bnw_visible_enemy_probe();
BnwUnitValidityProbe make_bnw_unit_invisible_probe();

} // namespace civ5::runtime
