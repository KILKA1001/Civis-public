#pragma once
#include "civ5/runtime/session_checkpoint_identity.hpp"
#include "civ5/saves/civis_save.hpp"
#include "civ5/saves/session_snapshot.hpp"

namespace civ5::runtime {

// Proposed section ID for an atomic single-unit state and two-plot roster
// overlay. The common CivisSaveSectionId/loader is intentionally owned by
// integration; this codec is usable directly until it is registered.
inline constexpr std::uint32_t bnw_unit_jump_checkpoint_section_id=41;

struct BnwUnitJumpCheckpointTarget {
    std::uint64_t player_index=0,unit_index=0,from_plot=0,to_plot=0;
};

// Persists the selected unit's complete decoded core/tail values and raw +
// projected unit rosters on the two affected plots. Other unit/plot roster
// changes are rejected. Area, sight, meetings, Lua and UI remain separate
// required checkpoint/effect owners, so this section alone is not a setXY.
saves::CivisSaveSection make_bnw_unit_jump_checkpoint(
    const saves::SavedSessionSnapshot &world,
    const saves::SavedSessionSnapshot &source,
    const SessionCheckpointIdentity &identity,
    BnwUnitJumpCheckpointTarget target);

// Verifies source identity, roster, map dimensions and digest before changing
// a detached copy; verifies target digest before assigning it to world.
void restore_bnw_unit_jump_checkpoint(
    const saves::CivisSaveSection &section,
    saves::SavedSessionSnapshot &world,
    const SessionCheckpointIdentity &identity);

} // namespace civ5::runtime
