#pragma once

#include "civ5/runtime/bnw_team_tech_domain_yields.hpp"

namespace civ5::runtime {

enum class BnwTechPlayerMapGate {
    prior_effects,
    player_modifiers,
    free_promotions,
    reset_and_init_traits,
    policy_cost_recompute,
    trait_free_units,
    resource_plot_yields,
    city_routes,
    lua_team_tech_researched,
};
struct BnwTechPlayerMapPlan {
    std::vector<std::int64_t> ever_alive_members;
    std::uint64_t nonzero_player_modifiers=0, free_promotions=0;
    std::uint64_t resource_reveal_rules=0, route_build_rules=0;
    BnwTechPlayerMapGate first_pending=BnwTechPlayerMapGate::prior_effects;
    std::string blocker;
    bool grant_complete=false;
};

// Source-order preflight of the next CvTeam::processTech interval. Independent
// map-rule counts are also reported after a player gate, but they never imply
// that the preceding trait callbacks have run. The caller state is unchanged.
BnwTechPlayerMapPlan preflight_bnw_barbarian_player_map_tech(
    const LiveSessionState &after_low_level,
    const content::TechnologyRules &technologies,
    const content::RulesDatabase &database,
    const content::InfoTypeCatalog &types,
    std::int64_t technology);

} // namespace civ5::runtime
