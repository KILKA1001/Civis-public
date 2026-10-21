#pragma once

#include "civ5/runtime/lua_map_services.hpp"
#include "civ5/runtime/session_checkpoint_identity.hpp"
#include <optional>
#include <string>
#include <vector>

namespace civ5::runtime {

struct LiveSessionState;

struct BnwPlotImpassabilitySource {
    std::uint64_t index=0,x=0,y=0;
    std::optional<std::int64_t> terrain,feature;
    std::optional<std::uint64_t> feature_hash;
    bool operator==(const BnwPlotImpassabilitySource &) const=default;
};

struct BnwLivePlotImpassability {
    bool current=false;
    PlotPassability values;
    std::vector<BnwPlotImpassabilitySource> source;
    std::string rules_sha256;
};

// Expansion2 CvPlot.cpp:9594 calls updateImpassable after each plot Read.
// Bind the existing verified terrain/feature calculation to the owned world.
// A missing rule, unresolved feature or mismatched activated rules rejects.
void rebuild_bnw_live_plot_impassability(
    LiveSessionState &state,const content::RulesDatabase &rules,
    const content::InfoTypeCatalog &types,
    const SessionCheckpointIdentity &identity);

// Rejects a stale cache even when a caller has modified the public state
// directly without using the LiveSession invalidation boundary.
const PlotPassability &bnw_live_plot_impassability(const LiveSessionState &state);

} // namespace civ5::runtime
