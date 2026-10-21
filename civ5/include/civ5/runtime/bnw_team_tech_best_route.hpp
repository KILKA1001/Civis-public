#pragma once

#include "civ5/runtime/bnw_team_tech_after_lua_plan.hpp"
#include <span>

namespace civ5::runtime {

struct BnwBuildRouteRule {
    std::int64_t build=-1, route=-1, prerequisite_technology=-1;
};
struct BnwBestRouteSelection {
    std::int64_t route=-1, value=0, winning_build=-1;
};
// Pure CvTeam::DoUpdateBestRoute evaluation. The last build wins ties.
BnwBestRouteSelection select_bnw_best_route(
    std::span<const BnwBuildRouteRule> builds,
    const TeamTechnologies &owned,
    const content::TechnologyRules &technologies);

struct BnwTeamTechBestRouteMarker {
    std::int64_t team=63, technology=0, before=-1, after=-1;
};
struct BnwTeamTechBestRouteResult {
    bool applied=false, grant_complete=false;
    std::int64_t before=-1, after=-1, winning_build=-1, route_value=0;
    std::size_t inspected_builds=0;
    std::string blocker;
};
// Runs only after a quoted Lua callback and the read-only post-Lua prefix.
// All rules and saved fields are checked before the detached world changes.
BnwTeamTechBestRouteResult run_bnw_team_tech_best_route(
    LiveSessionState &detached,
    const BnwTeamTechLuaAttempt &lua,
    const content::TechnologyRules &technologies,
    const content::RulesDatabase &database,
    const content::InfoTypeCatalog &types,
    std::int64_t team,
    std::int64_t technology);

} // namespace civ5::runtime
