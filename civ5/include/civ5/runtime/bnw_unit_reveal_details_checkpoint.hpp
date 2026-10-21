#pragma once
#include "civ5/runtime/session_checkpoint_identity.hpp"
#include "civ5/saves/civis_save.hpp"
#include "civ5/saves/session_snapshot.hpp"

namespace civ5::runtime {

// Proposed private section. The common loader must reject it until the full
// setXY transaction, including AI, Lua, UI and graphics, has an escrow.
inline constexpr std::uint32_t bnw_unit_reveal_details_checkpoint_section_id=43;
inline constexpr bool bnw_unit_reveal_details_checkpoint_can_commit=false;

// One team's known CvPlot::setRevealed details: owner, improvement, route,
// city visibility and found-player-territory. Source/target digests include
// the entire relevant roster, while the wire payload is sparse.
saves::CivisSaveSection make_bnw_unit_reveal_details_checkpoint(
    const saves::SavedSessionSnapshot &world,
    const saves::SavedSessionSnapshot &source,
    const SessionCheckpointIdentity &identity,std::int64_t team);

// Verifies identity, roster, source and target digest before publishing any
// change. This only restores a detached diagnostic slice of setXY.
void restore_bnw_unit_reveal_details_checkpoint(
    const saves::CivisSaveSection &section,
    saves::SavedSessionSnapshot &world,
    const SessionCheckpointIdentity &identity);

} // namespace civ5::runtime
