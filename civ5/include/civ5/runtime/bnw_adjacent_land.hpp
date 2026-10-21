#pragma once

#include "civ5/saves/map_state.hpp"
#include <cstdint>
#include <vector>

namespace civ5::runtime {

struct LiveSessionState;

// CvMap::Read rebuilds CvPlot::m_bIsAdjacentToLand after all plots load.
// CvAStar consumes this value for sea movement. Values are indexed by the
// owned map's row-major plot index, never by a native CvPlot pointer.
struct BnwAdjacentLandCache {
    bool current=false;
    saves::MapGrid grid;
    bool wrap_x=false,wrap_y=false;
    std::vector<std::int64_t> plot_types;
    std::vector<std::uint8_t> adjacent_land;
};

void rebuild_bnw_adjacent_land(LiveSessionState &state);
// Validate once before a path search, then read adjacent_land by plot index.
const BnwAdjacentLandCache &bnw_adjacent_land_cache(const LiveSessionState &state);
bool bnw_adjacent_land(const LiveSessionState &state,std::uint64_t plot);

} // namespace civ5::runtime
