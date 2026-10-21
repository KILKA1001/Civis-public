#pragma once

#include "civ5/runtime/bnw_map_turn.hpp"
#include "civ5/runtime/bnw_plot_domain_binding.hpp"
#include "civ5/runtime/bnw_plot_unit_validity_preflight.hpp"
#include "civ5/runtime/bnw_world_anchor_clear.hpp"

namespace civ5::runtime {

// Existing deliberately narrow branch for synthetic unitless, anchorless
// maps. It has no engine service and rejects occupied/anchored saves.
BnwTurnHandler make_bnw_game_map_phase_handler(
    const content::GlobalDefineCache &defines);

// Read-only GameCore predicates prove the no-effect verifyUnitValidPlot path.
// A required jump/kill, missing predicate, or anchor without an emitter
// rejects the detached map phase. The anchor callback is only published by
// after_commit after the enclosing game turn commits; its journal retains
// uncertain delivery and blocks another run until reconciled.
BnwTurnHandler make_bnw_game_map_phase_handler(
    const content::GlobalDefineCache &defines,
    BnwPlotDomainRules improvement_rules,
    BnwPlotUnitReadServices unit_reads,
    BnwWorldAnchorEmitter anchor_emitter);

} // namespace civ5::runtime