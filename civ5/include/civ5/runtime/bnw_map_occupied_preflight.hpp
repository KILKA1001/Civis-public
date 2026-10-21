#pragma once

#include "civ5/saves/session_snapshot.hpp"
#include <cstdint>
#include <string>
#include <vector>

namespace civ5::runtime {

struct BnwOccupiedPlotUnit {
    std::uint64_t plot=0;
    std::int64_t x=0,y=0,plot_owner=-1;
    saves::SavedPlotUnitReference handle;
    // These are saved inputs to CvPlot::verifyUnitValidPlot. They do not
    // replace the live GameCore predicates or jump/kill effects.
    bool saved_unit_found=false,at_saved_plot=false,delayed_death=false;
    bool transport_link=false,combat_link=false;
};
struct BnwWorldAnchorRemoval {
    std::uint64_t plot=0;
    std::int64_t x=0,y=0,old_anchor=-1,old_data=-1;
};
struct BnwMapOccupiedPreflight {
    bool shape_valid=false;
    std::string reason;
    std::vector<BnwOccupiedPlotUnit> units;
    std::vector<BnwWorldAnchorRemoval> anchors;
    // True means the source doTurn needs GameCore unit predicates/actions or
    // an engine anchor removal event before the map phase may commit.
    bool requires_unit_services=false,requires_anchor_service=false;
};

// Read-only audit of every saved plot. Requires raw/projected plot rosters and
// unique owned CvUnit IDs to agree; errors are fail-closed diagnostics.
BnwMapOccupiedPreflight preflight_bnw_map_occupied(
    const saves::SavedSessionSnapshot &world);

} // namespace civ5::runtime
