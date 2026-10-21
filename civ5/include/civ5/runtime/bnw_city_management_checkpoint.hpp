#pragma once
#include "civ5/runtime/bnw_city_production_checkpoint.hpp"

namespace civ5::runtime {

// Proposed common ID: all six BNW CityManagement blocks for every city.
// allow_unchanged emits version 2 only for an unchanged validated tree.
inline constexpr std::uint32_t bnw_city_management_checkpoint_section_id=27;

saves::CivisSaveSection make_bnw_city_management_checkpoint(
    const saves::SavedSessionSnapshot &world,
    const saves::SavedSessionSnapshot &source,
    const SessionCheckpointIdentity &identity,
    bool allow_unchanged=false);

void restore_bnw_city_management_checkpoint(
    const saves::CivisSaveSection &section,
    saves::SavedSessionSnapshot &world,
    const saves::SavedSessionSnapshot &source,
    const SessionCheckpointIdentity &identity);

struct BnwCityCoreProductionManagementCheckpointTriple {
    saves::CivisSaveSection core,production,management;
};

// Independent domain validation against one baseline plus full-world tail and
// roster verification. Never silently drops a fourth changed city tree.
BnwCityCoreProductionManagementCheckpointTriple
make_bnw_city_core_production_management_checkpoint_triple(
    const saves::SavedSessionSnapshot &world,
    const saves::SavedSessionSnapshot &source,
    const SessionCheckpointIdentity &identity);

// Returns a detached restored snapshot. Live CityYield/Unhappiness caches and
// ID7 production progress must be reconciled before publication.
saves::SavedSessionSnapshot restore_bnw_city_core_production_management_checkpoint_triple(
    const BnwCityCoreProductionManagementCheckpointTriple &triple,
    const saves::SavedSessionSnapshot &source,
    const SessionCheckpointIdentity &identity);

} // namespace civ5::runtime
