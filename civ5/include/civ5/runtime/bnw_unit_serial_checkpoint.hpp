#pragma once
#include "civ5/runtime/bnw_unit_checkpoint.hpp"

namespace civ5::runtime {
// Sparse persistence of an actual FLTA SetID serial burn followed by abort.
// No new unit, slot, free-list, scalar18, movement, path or Game receipt is
// restored. These detached data functions are not a live continuation grant.
inline constexpr std::uint32_t bnw_unit_serial_checkpoint_section_id=45;
saves::CivisSaveSection make_bnw_unit_serial_checkpoint(
    const saves::SavedSessionSnapshot &world,
    const saves::SavedSessionSnapshot &original,
    const SessionCheckpointIdentity &identity);
void validate_bnw_unit_serial_checkpoint_shape(const saves::CivisSaveSection &section);
void restore_bnw_unit_serial_checkpoint(
    const saves::CivisSaveSection &section,
    saves::SavedSessionSnapshot &world,
    const SessionCheckpointIdentity &identity);
// Existing partial exporters cannot silently discard this allocator mutation.
// A future closed producer/export join must compose section45 explicitly.
void require_bnw_unit_serial_export_unchanged(
    const saves::SavedSessionSnapshot &world,
    const saves::SavedSessionSnapshot &original);
} // namespace civ5::runtime
