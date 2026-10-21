#pragma once
#include "civ5/runtime/improvement_yields.hpp"
#include "civ5/runtime/plot_water.hpp"
#include "civ5/runtime/resource_visibility.hpp"
#include "civ5/runtime/profile_extensions.hpp"

namespace civ5::runtime {
struct ImprovementYieldState {
    content::TechnologyProfile profile=content::TechnologyProfile::bnw;
    struct Player {
        ImprovementYieldMatrix own,traits;
    };
    // Base, dry, fresh; these are saved live bonuses, not inferred from techs.
    std::map<std::int64_t,std::array<ImprovementYieldMatrix,3>> teams;
    std::map<std::int64_t,Player> players;
    ProfileExtensions profile_extensions;
};
struct ImprovementReligionRules {
    struct Belief {bool pantheon=false;ImprovementYieldMatrix improvements;};
    std::map<std::int64_t,Belief> beliefs;
};
ImprovementReligionRules load_improvement_religion_rules(const content::RulesDatabase&);
// Also restores WLTKD and secondary-pantheon counters into the shared cities/players.
ImprovementYieldState restore_improvement_yield_state(const saves::SavedSessionSnapshot&,
    const ImprovementYieldRules&,const content::InfoTypeCatalog&,CityYieldContext&);
struct ImprovementYieldContext {
    const ImprovementYieldRules &rules;
    const ImprovementYieldState &state;
    PlotWaterContext &water;
    const ResourceVisibilityContext &resources;
    // Same live era map already used by maintenance/research selection.
    const std::map<std::int64_t,std::int64_t> &team_eras;
    const ImprovementReligionRules *religion=nullptr; // required by BNW city religion branch
    // Source enum identities; wide owned worlds may supply different IDs.
    std::int64_t road=0,railroad=1,automatic_route=rules.profile==content::TechnologyProfile::vp542?3:2;
    // BNW native FLTA lookup only; VP retains its profile lookup.
    const saves::SavedSessionSnapshot* current_city_registry=nullptr;
};
struct ImprovementYieldQuote {
    std::int64_t base=0,era=0,adjacent_features=0,adjacent_resources=0,resource=0,golden_age=0,wltkd=0;
    std::int64_t river=0,coast=0,hills=0,feature=0,adjacent_city=0,adjacent_mountain=0,fresh=0;
    std::int64_t route=0,trade_route=0,connection=0,route_intrinsic=0,technologies=0,policies=0;
    std::int64_t player=0,traits=0,team=0,team_water=0,religion=0,fractional=0;
    std::int64_t total() const;
};
// Complete source helper formulas, not the full plot yield or cache update.
// Pillage/visibility filters on VP arguments belong to the caller, as in source.
ImprovementYieldQuote bnw_improvement_yield(ImprovementYieldContext&,std::int64_t plot,
    std::int64_t improvement,std::int64_t yield,std::int64_t player=-1,bool optimal=false,
    std::optional<std::int64_t> assumed_route={}); // absent = source NUM_ROUTE_TYPES
ImprovementYieldQuote vp_improvement_yield(ImprovementYieldContext&,std::int64_t plot,
    std::int64_t improvement,std::int64_t yield,std::int64_t player,std::int64_t route,
    std::int64_t feature,std::int64_t resource,const CityYieldState *owning_city=nullptr,
    bool optimal=false,std::optional<std::int64_t> forced_connection={});
} // namespace civ5::runtime
