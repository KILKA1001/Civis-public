#pragma once
#include "civ5/runtime/session_checkpoint_identity.hpp"
#include "civ5/saves/session_snapshot.hpp"

namespace civ5::runtime {

// Proposed common ID; this first city-domain slice covers every decoded core
// value/byte, not production, citizens, religion, espionage or CityAI.
// allow_unchanged emits version 2 only for an unchanged validated tree.
inline constexpr std::uint32_t bnw_city_core_checkpoint_section_id=25;

saves::CivisSaveSection make_bnw_city_core_checkpoint(
    const saves::SavedSessionSnapshot &world,
    const saves::SavedSessionSnapshot &source,
    const SessionCheckpointIdentity &identity,
    bool allow_unchanged=false);

// Strict baseline writer: changed schema3 / unchanged schema4 additionally
// bind every original core value and byte. Legacy schemas1/2 remain readable.
saves::CivisSaveSection make_bnw_city_core_baseline_checkpoint(
    const saves::SavedSessionSnapshot &world,
    const saves::SavedSessionSnapshot &source,
    const SessionCheckpointIdentity &identity,
    bool allow_unchanged=false);

void restore_bnw_city_core_checkpoint(
    const saves::CivisSaveSection &section,
    saves::SavedSessionSnapshot &world,
    const saves::SavedSessionSnapshot &source,
    const SessionCheckpointIdentity &identity);

} // namespace civ5::runtime
