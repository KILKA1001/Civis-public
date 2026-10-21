#pragma once

#include "civ5/runtime/session_checkpoint_identity.hpp"
#include "civ5/saves/civis_save.hpp"
#include "civ5/saves/session_snapshot.hpp"

namespace civ5::runtime {

inline constexpr std::uint32_t bnw_game_score_checkpoint_section_id=38;

// Persists the six CvGame score/rank arrays and the two BNW CvPlayer replay
// collections against the immutable Civ5Save payload and selected rules.
// The source and live collection shapes are validated before any bytes escape.
saves::CivisSaveSection make_bnw_game_score_checkpoint(
    const saves::SavedSessionSnapshot &world,
    const saves::SavedSessionSnapshot &source,
    const SessionCheckpointIdentity &identity);

// Requires an unchanged source baseline in world. Parsing and validation are
// transactional: failure never publishes a partially restored snapshot.
void restore_bnw_game_score_checkpoint(
    const saves::CivisSaveSection &section,
    saves::SavedSessionSnapshot &world,
    const saves::SavedSessionSnapshot &source,
    const SessionCheckpointIdentity &identity);

} // namespace civ5::runtime
