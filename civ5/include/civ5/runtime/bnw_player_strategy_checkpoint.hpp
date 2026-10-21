#pragma once
#include "civ5/runtime/session_checkpoint_identity.hpp"
#include "civ5/saves/session_snapshot.hpp"

namespace civ5::runtime {

// Proposed common section ID. This full six-block value section supersedes
// section 22; an importer must reject documents containing both IDs.
inline constexpr std::uint32_t bnw_player_strategy_checkpoint_section_id=23;

// Saves every scalar, array element, and hashed-row value in all six BNW
// PlayerStrategy blocks for all 64 players. Source wire structure, row counts,
// type hashes, object roster and field order must remain unchanged. AI changes
// to variable collection structure are intentionally rejected until encoded.
// With allow_unchanged, version 2 records an unchanged validated tree;
// changed trees retain version 1.
saves::CivisSaveSection make_bnw_player_strategy_checkpoint(
    const saves::SavedSessionSnapshot &world,
    const saves::SavedSessionSnapshot &source,
    const SessionCheckpointIdentity &identity,
    bool allow_unchanged=false);

// Applies all values only after complete validation. The destination must be
// the unmodified source strategy; composition with section 22 is forbidden.
void restore_bnw_player_strategy_checkpoint(
    const saves::CivisSaveSection &section,
    saves::SavedSessionSnapshot &world,
    const saves::SavedSessionSnapshot &source,
    const SessionCheckpointIdentity &identity);

} // namespace civ5::runtime
