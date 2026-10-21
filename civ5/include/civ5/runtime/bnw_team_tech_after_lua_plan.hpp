#pragma once

#include "civ5/runtime/bnw_team_tech_lua_gate.hpp"
#include "civ5/content/info_types.hpp"

namespace civ5::runtime {

// Source order: CvTeam::processTech after TeamTechResearched, then the
// continuation of CvTeam::setHasTech. A plan never commits a technology.
enum class BnwTeamTechAfterLuaStep {
    lua_quote,
    ends_game,
    artifact_notifications,
    era_evaluation,
    map_reveal,
    best_route,
};

struct BnwTeamTechAfterLuaPlan {
    std::vector<BnwTeamTechAfterLuaStep> proven_no_effect;
    BnwTeamTechAfterLuaStep first_pending=BnwTeamTechAfterLuaStep::lua_quote;
    std::string blocker;
    bool grant_complete=false;
};

// Reads the active BNW rules and a detached, already-acquired world. The
// quoted Lua attempt is an obligatory predecessor, not a substitute for a
// selected stock gameplay context owner or for later world effects.
BnwTeamTechAfterLuaPlan plan_bnw_team_tech_after_lua(
    const LiveSessionState &detached,
    const BnwTeamTechLuaAttempt &lua,
    const content::TechnologyRules &technologies,
    const content::RulesDatabase &database,
    const content::InfoTypeCatalog &types,
    std::int64_t team,
    std::int64_t technology);

} // namespace civ5::runtime
