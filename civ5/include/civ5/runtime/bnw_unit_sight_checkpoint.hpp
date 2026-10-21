#pragma once
#include "civ5/runtime/session_checkpoint_identity.hpp"
#include "civ5/saves/civis_save.hpp"
#include "civ5/saves/session_snapshot.hpp"

namespace civ5::runtime {

// Proposed isolated section; the common enum/loader must continue to reject
// this partial setXY overlay until all mandatory effects are checkpointed.
inline constexpr std::uint32_t bnw_unit_sight_checkpoint_section_id=42;

// One team's source-ordered unit sight counter/reveal projection:
// CvPlot visibility[80], invisible_visibility[80], revealed_words[4],
// projected revealed_teams, and CvArea revealed_tiles[80]. Other team slots
// and area reveal columns are required to remain identical. This persists
// the existing detached sight slice, not setRevealed's remaining effects.
saves::CivisSaveSection make_bnw_unit_sight_checkpoint(
    const saves::SavedSessionSnapshot &world,
    const saves::SavedSessionSnapshot &source,
    const SessionCheckpointIdentity &identity,
    std::int64_t team);

void restore_bnw_unit_sight_checkpoint(
    const saves::CivisSaveSection &section,
    saves::SavedSessionSnapshot &world,
    const SessionCheckpointIdentity &identity);

} // namespace civ5::runtime
