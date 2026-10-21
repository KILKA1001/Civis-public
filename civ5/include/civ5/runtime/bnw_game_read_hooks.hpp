#pragma once
#include "civ5/saves/game_state.hpp"
#include "civ5/saves/session_snapshot.hpp"

namespace civ5::runtime {
struct ResearchContext;
// CvGame::Read scalar effects. Apply to the owned world after checkpoint
// restoration; the immutable source snapshot retains the serialized values.
void apply_bnw_game_read_scalar_hooks(saves::GameStateObservation &game);
// CvGame::Read increments m_iNumSessions after reading the saved value for
// every non-network game. The caller still has to add the reload replay event.
void apply_bnw_game_read_session_hook(saves::SavedSessionSnapshot &world);
// For a single-player load, CvGame::Read selects the first human civilization
// after the saved player objects exist. This updates the owned PreGame state;
// replay text, graphics invalidation and reseeding remain separate hooks.
void apply_bnw_game_read_active_player_hook(saves::SavedSessionSnapshot &world,
                                            const ResearchContext &setup);
}
