#pragma once

#include "civ5/content/global_defines.hpp"
#include "civ5/runtime/bnw_turn_scheduler.hpp"

namespace civ5::runtime {

// GC.getBARBARIAN_CAMP_IMPROVEMENT from the activated Expansion2 rules.
std::int64_t resolve_bnw_barbarian_camp_improvement(
    const content::GlobalDefineCache &defines);

// Exact serializable CvBarbarians::BeginTurn counter transitions. This does
// not include DoCamps or DoUnits, which occur later in CvGame::doTurn.
LiveActionResult run_bnw_barbarians_begin_phase(
    LiveSessionState &state,std::int64_t camp_improvement);
BnwTurnHandler make_bnw_barbarians_begin_handler(
    std::int64_t camp_improvement);

} // namespace civ5::runtime
