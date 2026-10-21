#pragma once
#include <cstdint>

namespace civ5::content { class RulesDatabase; }
namespace civ5::runtime {

// Resolve from the activated BNW rules database. Keep Water explicit so an
// invalid or modded LAND_TERRAIN cannot be applied as ordinary land.
struct BnwSelectedTerrain {
    std::int32_t id = -1;
    bool water = false;
    bool impassable = false;
};

using BnwSelectedLandTerrain = BnwSelectedTerrain;
BnwSelectedTerrain resolve_bnw_selected_terrain_by_id(
    const content::RulesDatabase &rules, std::int32_t id);
BnwSelectedLandTerrain resolve_bnw_selected_land_terrain(
    const content::RulesDatabase &rules);

} // namespace civ5::runtime
