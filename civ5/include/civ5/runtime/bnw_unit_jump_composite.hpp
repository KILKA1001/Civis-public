#pragma once
#include "civ5/runtime/bnw_area_unit_checkpoint.hpp"
#include "civ5/runtime/bnw_unit_jump_checkpoint.hpp"
#include "civ5/runtime/bnw_unit_sight_checkpoint.hpp"

namespace civ5::runtime {

struct BnwUnitJumpCompositeSections {
    saves::CivisSaveSection unit,area,sight;
    // This is an atomic persistence slice, not a playable setXY. Mandatory
    // meeting, owner/improvement/route, AI, Lua, UI and graphics effects still
    // reject a real jump before the enclosing game turn can commit.
    bool can_commit() const noexcept {return false;}
};

// Narrow cross-area, single-unit, single-team composition. ID19's region
// writer is given a view with ID42 revealed_tiles masked back to source;
// otherwise it correctly rejects the unrelated area reveal changes.
BnwUnitJumpCompositeSections make_bnw_unit_jump_composite(
    const saves::SavedSessionSnapshot &world,
    const saves::SavedSessionSnapshot &source,
    const SessionCheckpointIdentity &identity,
    BnwUnitJumpCheckpointTarget target,
    const BnwAreaUnitTransfer &transfer,
    std::int64_t team);

// Applies ID41, ID19 and ID42 to one private copy, then re-encodes the three
// sections as a cross-check. No partial state is published on failure.
// The caller must rebuild BnwLivePlotVisibility after assigning the result.
void restore_bnw_unit_jump_composite(
    const BnwUnitJumpCompositeSections &sections,
    saves::SavedSessionSnapshot &world,
    const SessionCheckpointIdentity &identity,
    BnwUnitJumpCheckpointTarget target,
    const BnwAreaUnitTransfer &transfer,
    std::int64_t team);

} // namespace civ5::runtime
