#pragma once
#include "civ5/runtime/session_checkpoint_identity.hpp"
#include "civ5/saves/session_snapshot.hpp"
#include <span>

namespace civ5::runtime {

// Proposed ID36: complete decoded BNW PlayerAdministration for all 64 player
// slots. This is a fixed-shape raw checkpoint, not a gameplay notification,
// treasury or research-queue service. IDs 8, 10, 11, 15, 30, 33 and 34 own
// overlapping values or must agree with its research queue; no composition
// is proven for these sections yet.
inline constexpr std::uint32_t bnw_player_administration_checkpoint_section_id=36;

void validate_bnw_player_administration_composition(
    std::span<const saves::CivisSaveSection> sections);

saves::CivisSaveSection make_bnw_player_administration_checkpoint(
    const saves::SavedSessionSnapshot &world,
    const saves::SavedSessionSnapshot &source,
    const SessionCheckpointIdentity &identity);

// Returns a detached copy. A failed decode never changes world_before.
saves::SavedSessionSnapshot restore_bnw_player_administration_checkpoint(
    const saves::CivisSaveSection &section,
    const saves::SavedSessionSnapshot &world_before,
    const saves::SavedSessionSnapshot &source,
    const SessionCheckpointIdentity &identity);

} // namespace civ5::runtime
