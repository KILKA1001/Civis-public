#pragma once
#include "civ5/runtime/bnw_city_core_checkpoint.hpp"

namespace civ5::runtime {

// Proposed common ID. Raw CityProduction is distinct from live ID7 progress.
// allow_unchanged emits version 2 only for an unchanged validated tree.
inline constexpr std::uint32_t bnw_city_production_checkpoint_section_id=26;

saves::CivisSaveSection make_bnw_city_production_checkpoint(
    const saves::SavedSessionSnapshot &world,
    const saves::SavedSessionSnapshot &source,
    const SessionCheckpointIdentity &identity,
    bool allow_unchanged=false);

void restore_bnw_city_production_checkpoint(
    const saves::CivisSaveSection &section,
    saves::SavedSessionSnapshot &world,
    const saves::SavedSessionSnapshot &source,
    const SessionCheckpointIdentity &identity);

struct BnwCityCoreProductionCheckpointPair {
    saves::CivisSaveSection core;
    saves::CivisSaveSection production;
};

// Exports both changed raw domains from the same detached world. ID25's
// companion guard is checked against a core-only copy; ID26 verifies the
// unchanged header, management and tail in the complete world.
BnwCityCoreProductionCheckpointPair make_bnw_city_core_production_checkpoint_pair(
    const saves::SavedSessionSnapshot &world,
    const saves::SavedSessionSnapshot &source,
    const SessionCheckpointIdentity &identity);

// Produces a new world after both restores; the caller publishes only after
// validating live CityYieldContext/ID7 and all other companion sections.
saves::SavedSessionSnapshot restore_bnw_city_core_production_checkpoint_pair(
    const BnwCityCoreProductionCheckpointPair &pair,
    const saves::SavedSessionSnapshot &source,
    const SessionCheckpointIdentity &identity);

} // namespace civ5::runtime
