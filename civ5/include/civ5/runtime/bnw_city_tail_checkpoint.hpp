#pragma once
#include "civ5/runtime/bnw_city_management_checkpoint.hpp"

namespace civ5::runtime {

// Proposed common ID for all five BNW CityTail trees of every city.
// allow_unchanged emits version 2 only for an unchanged validated tree.
inline constexpr std::uint32_t bnw_city_tail_checkpoint_section_id=28;

saves::CivisSaveSection make_bnw_city_tail_checkpoint(
    const saves::SavedSessionSnapshot &world,
    const saves::SavedSessionSnapshot &source,
    const SessionCheckpointIdentity &identity,
    bool allow_unchanged=false);

void restore_bnw_city_tail_checkpoint(
    const saves::CivisSaveSection &section,
    saves::SavedSessionSnapshot &world,
    const saves::SavedSessionSnapshot &source,
    const SessionCheckpointIdentity &identity);

struct BnwCityCheckpointQuadruple {
    saves::CivisSaveSection core,production,management,tail;
};

BnwCityCheckpointQuadruple make_bnw_city_checkpoint_quadruple(
    const saves::SavedSessionSnapshot &world,
    const saves::SavedSessionSnapshot &source,
    const SessionCheckpointIdentity &identity,
    bool allow_unchanged=false);

// Returns a detached result after all four restores. Live city, religion,
// espionage and AI contexts still need SDK load effects and reconciliation.
saves::SavedSessionSnapshot restore_bnw_city_checkpoint_quadruple(
    const BnwCityCheckpointQuadruple &sections,
    const saves::SavedSessionSnapshot &source,
    const SessionCheckpointIdentity &identity);

} // namespace civ5::runtime
