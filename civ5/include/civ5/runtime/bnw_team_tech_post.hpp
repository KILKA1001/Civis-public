#pragma once

#include "civ5/runtime/live_session.hpp"
#include <string>
#include <vector>

namespace civ5::runtime {

// CvTeam::setHasTech continuation after CvTeamTechs::SetHasTech. The caller
// supplies a detached state whose owned/raw technology flag and TeamSetHasTech
// hook have already been reconciled by the low-level acquisition service.
enum class BnwTeamTechPostStep {
    city_strength,
    resource_reveal_and_trade,
    process_world_congress,
    process_archaeological_sites,
    process_team_scalar_prefix,
    process_route_and_build_changes,
    process_player_traits,
    process_map_yields_and_city_routes,
    process_lua_team_tech_researched,
    remaining_set_has_tech,
    gameplay_tech_acquired,
};

struct BnwTeamTechPostPrefix {
    std::int64_t team=-1, technology=-1;
    std::string technology_type;
    std::vector<BnwTeamTechPostStep> proven_no_effect;
    BnwTeamTechPostStep first_pending=BnwTeamTechPostStep::city_strength;
    // Required suffix in SDK order. The Lua step is conditional on a script
    // system; without host evidence it remains a mandatory preflight gate.
    std::vector<BnwTeamTechPostStep> pending_effects;
    std::string blocker;
    // A prefix can never commit the acquisition: later callbacks and world
    // effects must be completed in source order by the enclosing transaction.
    bool grant_complete=false;
};

// Execute all provably empty post-low-level branches in source order. The
// supplied state is read-only; a required effect stops before any mutation.
// The active database is mandatory because linked resource rules do not live
// in TechnologyInfo. bFirst=false/bAnnounce=false is the DoBarbarianTech call.
BnwTeamTechPostPrefix run_bnw_barbarian_tech_post_prefix(
    const LiveSessionState &after_low_level,
    const content::TechnologyRules &technologies,
    const content::RulesDatabase &database,
    const content::InfoTypeCatalog &types,
    std::int64_t technology);

} // namespace civ5::runtime
