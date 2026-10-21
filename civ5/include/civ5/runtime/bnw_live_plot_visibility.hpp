#pragma once

#include "civ5/saves/map_state.hpp"
#include <cstdint>
#include <vector>

namespace civ5::runtime {

struct LiveSessionState;

// Expansion2 CvPlot::read clamps negative m_aiVisibilityCount values. The
// decoded world and immutable source retain their raw wire values; this owns
// the post-load values used by CvPlot::getVisibilityCount/isVisible.
struct BnwLivePlotVisibility {
    bool current=false;
    saves::MapGrid grid;
    std::uint64_t team_slots=0,wire_slots=0;
    std::vector<std::int64_t> source_raw;
    std::vector<std::int64_t> counts;
};

void rebuild_bnw_live_plot_visibility(LiveSessionState &state);
// Validate once before a batch of visibility queries.
const BnwLivePlotVisibility &bnw_live_plot_visibility_cache(
    const LiveSessionState &state);
std::int64_t bnw_plot_visibility_count(const LiveSessionState &state,
                                       std::uint64_t plot,std::int64_t team);
// CvPlot::isVisible(eTeam), without the separate debug-mode override.
bool bnw_plot_is_visible(const LiveSessionState &state,
                         std::uint64_t plot,std::int64_t team);

} // namespace civ5::runtime
