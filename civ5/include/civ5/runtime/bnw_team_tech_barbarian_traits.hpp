#pragma once

#include "civ5/runtime/bnw_team_tech_player_map.hpp"

namespace civ5::runtime {

// CvPlayerTraits::Reset creates this unsaved per-resource vector. With no
// active leader traits, each entry is the default FreeResourceXCities value.
struct BnwBarbarianTraitResetTransient {
    std::int64_t player=63;
    std::uint64_t free_resource_city_entries=0;
    bool reset_and_init_applied=false;
};

struct BnwBarbarianTraitResetResult {
    bool applied=false, grant_complete=false;
    std::uint64_t checked_trait_fields=0;
    std::int64_t policy_cost_before=0, policy_cost_after=0;
    std::string blocker;
};

// Narrow source-equivalent branch for the selected no-trait barbarian. Checks
// that Reset/InitPlayerTraits leaves every serialized trait value unchanged,
// materializes the unsaved default resource vector, then runs the exact
// recomputePolicyCostModifier scalar write. A rejected preflight leaves the
// caller state untouched. Later Lua/engine effects still block the grant.
BnwBarbarianTraitResetResult run_bnw_barbarian_trait_reset(
    LiveSessionState &detached_after_low_level,
    const content::TechnologyRules &technologies,
    const content::RulesDatabase &database,
    const content::InfoTypeCatalog &types,
    const content::GlobalDefineCache &defines,
    std::int64_t technology);

} // namespace civ5::runtime
