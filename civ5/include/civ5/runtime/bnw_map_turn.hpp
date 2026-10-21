#pragma once

#include "civ5/runtime/bnw_turn_scheduler.hpp"
#include <cstdint>
#include <functional>
#include <optional>

namespace civ5::runtime {

// CvMap::doTurn visits plots in their saved index order. These callbacks must
// implement unit validity and, when an old anchor exists, stage its engine
// removal in the detached LiveSessionState. Neither may publish an external
// effect before the enclosing game-turn transaction commits.
struct BnwMapTurnServices {
    std::function<LiveActionResult(LiveSessionState &, std::uint64_t)> verify_units;
    std::function<LiveActionResult(LiveSessionState &, std::uint64_t,
                                   std::int64_t)> stage_anchor_removal;
    bool complete() const noexcept {
        return static_cast<bool>(verify_units) && static_cast<bool>(stage_anchor_removal);
    }
};

// CvPlot::verifyUnitValidPlot copies the plot's ordered unit handles before
// either pass. The caller supplies current GameCore predicates and the
// transactional jump/kill operations; an absent answer rejects the plot turn.
// Every operation must use only the detached LiveSessionState. In particular,
// jump and kill must update both owned units and the live plot roster.
using BnwUnitValidityProbe = std::function<std::optional<bool>(
    const LiveSessionState &, std::uint64_t, saves::SavedPlotUnitReference)>;
using BnwUnitValidityNumber = std::function<std::optional<std::int64_t>(
    const LiveSessionState &, std::uint64_t, saves::SavedPlotUnitReference)>;
struct BnwUnitValidityServices {
    BnwUnitValidityProbe exists, delayed_death, at_plot, cargo, in_combat;
    BnwUnitValidityProbe valid_domain, can_enter_territory;
    BnwUnitValidityProbe visible_enemy, invisible;
    BnwUnitValidityNumber friendly_units_of_type, unit_team, plot_team;
    std::function<std::optional<bool>(LiveSessionState &, std::uint64_t,
                                      saves::SavedPlotUnitReference)> jump;
    std::function<LiveActionResult(LiveSessionState &, std::uint64_t,
                                   saves::SavedPlotUnitReference)> kill;
    bool complete() const noexcept;
};

LiveActionResult verify_bnw_plot_units(
    LiveSessionState &state, std::uint64_t plot_index,
    std::int64_t plot_unit_limit, const BnwUnitValidityServices &services);

struct BnwMapTurnRules {
    std::int64_t ownership_score_duration_threshold = 0;
    std::int64_t plot_unit_limit = 0;
};
// Read both source CvGlobals values from one activated BNW rules cache.
// Missing or invalid values cannot be replaced with the SDK constructor
// defaults when continuing a saved game.
BnwMapTurnRules resolve_bnw_map_turn_rules(const content::GlobalDefineCache &defines);

// Implements the serializable CvPlot::doTurn fields and land-score transition
// in source order. ownership_score_duration_threshold must come from the
// activated BNW GlobalDefines. This is a handler boundary, not a complete
// game turn or an implementation of verifyUnitValidPlot.
LiveActionResult run_bnw_map_turn_phase(
    LiveSessionState &state, std::int64_t ownership_score_duration_threshold,
    const BnwMapTurnServices &services);
BnwTurnHandler make_bnw_map_turn_handler(
    std::int64_t ownership_score_duration_threshold,
    BnwMapTurnServices services);

} // namespace civ5::runtime
