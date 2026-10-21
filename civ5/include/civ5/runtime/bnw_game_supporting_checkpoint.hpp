#pragma once

#include "civ5/runtime/live_session.hpp"
#include "civ5/saves/civis_save.hpp"

namespace civ5::runtime {

// Persists CvBarbarians counters, CvGoodyHuts memory and the GameCore cached
// world-religion tech progress against the immutable BNW payload baseline.
saves::CivisSaveSection make_bnw_game_supporting_checkpoint(
    const saves::SavedSessionSnapshot &world,
    const saves::SavedSessionSnapshot &source,
    const SessionCheckpointIdentity &identity);

// Validates the complete section and source baseline before changing world.
void restore_bnw_game_supporting_checkpoint(
    const saves::CivisSaveSection &section,
    saves::SavedSessionSnapshot &world,
    const SessionCheckpointIdentity &identity);

} // namespace civ5::runtime
