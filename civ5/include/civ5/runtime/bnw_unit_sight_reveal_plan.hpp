#pragma once
#include "civ5/runtime/bnw_unit_sight_plan.hpp"
#include "civ5/saves/map_state.hpp"

namespace civ5::runtime {

// A zero-to-positive visibility edge in Expansion2 CvPlot::changeVisibilityCount.
// The entries are grouped by plot ID, not in the SDK's ring execution order.
// They describe mandatory calls and their decoded inputs; they are not commands.
struct BnwSightRevealEdge {
    std::uint64_t plot=0;
    bool first_reveal=false;
    std::int64_t area=-1,area_revealed_before=-1;
    std::int64_t plot_owner=-1;
    std::vector<std::uint64_t> adjacent_owner_refresh;
    std::vector<saves::SavedPlotUnitReference> meet_unit_candidates;
    std::optional<saves::SavedPlotCityReference> meet_city_candidate;
};

struct BnwUnitSightRevealPlan {
    bool calculated=false;
    std::string reason;
    std::int64_t team=-1;
    BnwUnitSightPlan counters;
    std::vector<BnwSightRevealEdge> newly_visible;
    // Even when no cell is first revealed, setRevealed updates known owner,
    // improvement, route and city state on every zero-to-positive edge.
    // Area, exploration, natural-wonder, diplomacy, Lua and UI services have
    // not been applied to the owned world or checkpointed.
    std::vector<BnwSightBlocker> blockers;
    bool can_commit() const noexcept {return blockers.empty();}
};

// Read-only classification of Expansion2 SDK CvPlot.cpp:7655
// changeVisibilityCount and CvPlot.cpp:8043 setRevealed(true), using a fresh
// rule-bound sight counter plan. This never
// mutates map reveal, area counts, diplomacy or exploration state.
BnwUnitSightRevealPlan plan_bnw_unit_sight_reveal(
    const LiveSessionState &state,const BnwSetXYRequest &request,
    const content::RulesDatabase &database,const content::InfoTypeCatalog &types,
    const content::GlobalDefineCache &defines,
    const SessionCheckpointIdentity &identity);
} // namespace civ5::runtime
