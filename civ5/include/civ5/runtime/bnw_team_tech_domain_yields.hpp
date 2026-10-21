#pragma once

#include "civ5/runtime/bnw_team_tech_route_build.hpp"

namespace civ5::runtime {

struct BnwTechDomainMoveChange {
    std::int64_t domain=0, before=0, delta=0, after=0;
};
struct BnwTechDomainYieldPrefix {
    std::vector<BnwTechDomainMoveChange> domain_calls;
    std::uint64_t building_effects=0, improvement_calls=0,
                  improvement_nonzero=0;
    bool applied=false, grant_complete=false;
    std::string blocker;
};

// Continues the detached CvTeam::processTech draft after route/build loops.
// Domain movement is a plain team-array update. Building matches can run
// city callbacks, and nonzero improvement yields call updateYield(), so those
// branches explicitly reject until their world effects are available.
BnwTechDomainYieldPrefix run_bnw_barbarian_domain_yield_prefix(
    LiveSessionState &detached_after_low_level,
    const content::TechnologyRules &technologies,
    const content::RulesDatabase &database,
    const content::InfoTypeCatalog &types,
    std::int64_t technology);

} // namespace civ5::runtime
