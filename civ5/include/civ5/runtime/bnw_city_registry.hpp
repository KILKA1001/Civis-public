#pragma once
#include <cstdint>
namespace civ5::saves { struct SavedSessionSnapshot; struct SavedCityObservation; }
namespace civ5::runtime {
struct CityYieldContext;
struct CityYieldState;
// Pure borrowed lookup. The caller holds the current source/owned world and
// its read scope for the whole query and returned pointer use. No capability
// or freshness follows from this exact native FLTA lookup.
const saves::SavedCityObservation* resolve_bnw_city_registry(
    const saves::SavedSessionSnapshot&,const CityYieldContext&,
    std::int32_t owner,std::int32_t handle);
const CityYieldState* resolve_bnw_city_yield_registry(
    const saves::SavedSessionSnapshot&,const CityYieldContext&,
    std::int32_t owner,std::int32_t handle);
}
