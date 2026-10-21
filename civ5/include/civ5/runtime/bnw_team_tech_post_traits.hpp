#pragma once

#include "civ5/runtime/bnw_team_tech_barbarian_traits.hpp"

namespace civ5::runtime {

// The source GetFirstFreeUnit call resets this unsaved iterator even when the
// no-trait barbarian has no free unit. The later map loops are inert only when
// the selected technology has no matching reveal or route-build rule.
struct BnwBarbarianPostTraitsTransient {
    std::int64_t team=63, player=63;
    std::int64_t free_unit_prereq_tech=-1;
    std::uint64_t free_unit_index=0;
    bool free_unit_query_done=false, map_loops_done=false;
};

struct BnwBarbarianPostTraitsResult {
    bool applied=false, grant_complete=false;
    std::int64_t first_free_unit_class=-1;
    std::uint64_t resource_reveal_rules=0, route_build_rules=0;
    std::string blocker;
};

// Completes the selected Gandhi CvTeam::processTech interval after the
// barbarian trait reset through its two map loops. The mandatory Lua hook and
// later CvTeam::setHasTech effects are separate, still-pending boundaries.
BnwBarbarianPostTraitsResult run_bnw_barbarian_post_traits(
    LiveSessionState &detached_after_traits,
    const content::TechnologyRules &technologies,
    const content::RulesDatabase &database,
    const content::InfoTypeCatalog &types,
    std::int64_t technology);

} // namespace civ5::runtime
