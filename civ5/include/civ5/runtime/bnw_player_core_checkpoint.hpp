#pragma once
#include "civ5/runtime/session_checkpoint_identity.hpp"
#include "civ5/saves/session_snapshot.hpp"

namespace civ5::runtime {

// Proposed common section ID. This 64-player core section supersedes the
// proximity-only ID20; import must not apply both to one baseline.
inline constexpr std::uint32_t bnw_player_core_checkpoint_section_id=24;

// Covers every decoded BNW PlayerCore value and fixed-size string byte. The
// source field structure, collection lengths, hash IDs and nested row counts
// must stay fixed. City/unit roster changes and other player subsystems still
// need companion checkpoints; this section alone is not full persistence.
// With allow_unchanged, version 2 records an unchanged validated tree;
// changed trees retain version 1.
saves::CivisSaveSection make_bnw_player_core_checkpoint(
    const saves::SavedSessionSnapshot &world,
    const saves::SavedSessionSnapshot &source,
    const SessionCheckpointIdentity &identity,
    bool allow_unchanged=false);

// Restores only onto the unchanged source core. Parsing and validation finish
// before any destination value is written.
void restore_bnw_player_core_checkpoint(
    const saves::CivisSaveSection &section,
    saves::SavedSessionSnapshot &world,
    const saves::SavedSessionSnapshot &source,
    const SessionCheckpointIdentity &identity);

} // namespace civ5::runtime
