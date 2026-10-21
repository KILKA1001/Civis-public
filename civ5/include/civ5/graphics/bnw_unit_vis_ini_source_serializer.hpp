#pragma once
#include <string>
#include <stdexcept>
namespace civ5::graphics::unit_art_scale_detail {
// Closed source826EF0-missing-file branch: GetFloat inserts %f literal100; GetInt inserts %d defaults in
// multiplierРІвЂ вЂ™scale order. This is not a general user INI serializer.
inline std::string missing_unit_vis_scale_ini(unsigned completed_getters){
    if(completed_getters<1||completed_getters>4)throw std::invalid_argument("Missing unit INI source getter count");
    std::string out="[Debugging]\r\n\r\n; Global unit member count multiplier\r\nGlobalUnitMemberMultiplier = 100.000000\r\n";
    if(completed_getters>=2)out+="\r\n; Global unit scale\r\nGlobalUnitScale = 100.000000\r\n";
    if(completed_getters>=3)out+="\r\n; Global unit member placement random seed (use -1 to let the game decide)\r\nGlobalUnitPlacementSeed = -1\r\n";
    if(completed_getters>=4)out+="\r\n; Use Threaded render path for unit vis\r\nUnitRenderMultiThreaded = 1\r\n";
    return out+"\r\n";
}
} // namespace civ5::graphics::unit_art_scale_detail
