#pragma once
#include "civ5/runtime/bnw_unit_set_xy.hpp"
#include <cstdint>
#include <string>
#include <vector>

namespace civ5::runtime {
struct LiveSessionState;

enum class BnwSightBlocker : std::uint8_t {
    unsupported_candidate,
    unit_sight_range_rules,
    terrain_and_feature_line_of_sight,
    reveal_meeting_and_exploration_effects,
    invisible_visibility_effects,
    ui_and_derived_cache_effects,
    visibility_checkpoint,
};

struct BnwUnitSightPreflight {
    bool candidate=false;
    std::string reason;
    std::int64_t owner=-1,unit_id=-1,team=-1,unit_type=-1;
    std::int64_t extra_visibility=0,see_invisible=-1;
    std::uint64_t from_plot=0,to_plot=0;
    std::int64_t from_center_visibility=0,to_center_visibility=0;
    bool from_center_revealed=false,to_center_revealed=false;
    std::vector<BnwSightBlocker> blockers;
    bool can_commit() const noexcept {return blockers.empty();}
};

// Expansion2 CvUnit.cpp:13129,13188 calls CvPlot::changeAdjacentSight before
// and after coordinate change. This only validates decoded inputs for that
// path; it does not approximate the remaining rings or mutate visibility.
BnwUnitSightPreflight preflight_bnw_unit_sight(
    const LiveSessionState &state,const BnwSetXYRequest &request);
} // namespace civ5::runtime
