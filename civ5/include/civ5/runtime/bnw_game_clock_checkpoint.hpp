#pragma once
#include "civ5/runtime/live_session.hpp"
#include "civ5/saves/civis_save.hpp"

namespace civ5::runtime {

// Persists the clock mirrors changed by CvGame::setGameTurn and
// incrementElapsedGameTurns. External UI publication must be finished first.
saves::CivisSaveSection make_bnw_game_clock_checkpoint(
    const LiveSessionState &state, const saves::SavedSessionSnapshot &source,
    const SessionCheckpointIdentity &identity);

// Validates the source and rules identity, then changes a detached decoded
// snapshot. Call before restoring contexts whose inputs depend on game turn.
void restore_bnw_game_clock_checkpoint(
    const saves::CivisSaveSection &section, saves::SavedSessionSnapshot &world,
    BnwGameClockState &clock, const SessionCheckpointIdentity &identity);

} // namespace civ5::runtime
