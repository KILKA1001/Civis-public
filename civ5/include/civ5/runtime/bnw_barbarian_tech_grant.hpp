#pragma once

#include "civ5/runtime/live_action_transaction.hpp"
#include <cstdint>
#include <string>
#include <vector>

namespace civ5::runtime {

// The effects below follow CvTeam::setHasTech in Expansion2 CvTeam.cpp.
// Listing an effect does not imply that Civis can currently execute it.
enum class BnwBarbarianTechEffect {
    repeat_count_and_research_reset,
    repeat_future_technology_score,
    team_technology_write,
    team_set_has_tech_lua,
    team_city_strength,
    resource_reveal_and_connection,
    process_technology,
    team_tech_researched_lua,
    artifact_notifications,
    era_evaluation,
    map_reveal,
    best_route,
    team_player_research_and_yields,
    technology_sharing,
    owed_culture_buildings,
    team_player_happiness,
    interface_notification,
    gameplay_technology_event,
};

struct BnwBarbarianTechGrant {
    std::int64_t technology=-1;
    std::string type;
    std::int64_t major_owners=0;
    std::int64_t era=-1;
    std::vector<BnwBarbarianTechEffect> effects;
};

struct BnwBarbarianTechGate {
    std::int64_t alive_major_teams=0;
    std::int64_t percent=0;
    std::int64_t threshold=0;
    std::vector<BnwBarbarianTechGrant> grants;
    // No call to CvTeam::setHasTech is safe merely because its bit write is
    // available. This preflight rejects before any world or RNG mutation.
    bool safe=false;
    std::string blocker;
};

// Exact DoBarbarianTech gate and ascending technology order. Requires the
// selected BNW rules and complete 64-slot imported player/team roster.
BnwBarbarianTechGate plan_bnw_barbarian_technology_gate(
    const LiveSessionState &state,
    const content::TechnologyRules &technologies,
    const content::GlobalDefineCache &defines);

// Executable no-grant branch. A due technology rejects before mutation; the
// missing TeamSetHasTech / TeamTechResearched callbacks cannot be skipped.
LiveActionResult run_bnw_barbarian_technology_gate(
    LiveSessionState &state,
    const content::TechnologyRules &technologies,
    const content::GlobalDefineCache &defines);

} // namespace civ5::runtime
