#pragma once
#include <cstdint>
namespace civ5::saves {struct SavedSessionSnapshot;struct MapRegionRecord;}
namespace civ5::runtime {
// Pure borrowed native FLTA lookup. The caller retains the source allocation
// through pointer use. This confers no import, cache or refresh authority.
const saves::MapRegionRecord* resolve_bnw_area_registry(
 const saves::SavedSessionSnapshot&,std::int32_t handle);
// Complete pure registry validation for read models that iterate all records.
void validate_bnw_area_registry(const saves::SavedSessionSnapshot&);
}
