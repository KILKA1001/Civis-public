#pragma once

#include "civ5/runtime/bnw_team_tech_post.hpp"

namespace civ5::runtime {

enum class BnwTechRouteBuildKind { route_movement, build_time };
struct BnwTechRouteBuildChange {
    BnwTechRouteBuildKind kind;
    std::int64_t id=0, before=0, delta=0, after=0;
    std::string type;
};
struct BnwTechRouteBuildPrefix {
    // Includes zero-delta calls in CvTeam::processTech order: all routes,
    // then all builds. Values are the active cache's default-filled lookup.
    std::vector<BnwTechRouteBuildChange> calls;
    bool applied=false, grant_complete=false;
    std::string blocker;
};

// Pure source cache/default-array quotation shared by the detached legacy
// caller and genuine major pending owner. It never mutates/grants admission.
std::vector<BnwTechRouteBuildChange> prepare_bnw_route_build_calls(
    const saves::TeamObservation&,const content::RulesDatabase&,
    const content::InfoTypeCatalog&,std::int64_t technology,
    const std::string& technology_type);

// Continues after run_bnw_barbarian_tech_post_prefix. On a detached draft,
// preflights all source route/build rows and saved hash mappings, then applies
// CvTeam::changeRouteChange / changeBuildTimeChange. Caller must not publish
// the draft until the entire technology acquisition has completed.
BnwTechRouteBuildPrefix run_bnw_barbarian_route_build_prefix(
    LiveSessionState &detached_after_low_level,
    const content::TechnologyRules &technologies,
    const content::RulesDatabase &database,
    const content::InfoTypeCatalog &types,
    std::int64_t technology);

} // namespace civ5::runtime
