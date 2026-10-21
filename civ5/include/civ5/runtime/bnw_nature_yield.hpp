#pragma once
#include "civ5/runtime/plot_water.hpp"
#include "civ5/runtime/resource_visibility.hpp"

namespace civ5::runtime {
using NatureYieldMatrix=std::map<std::int64_t,YieldValues>;
struct BnwNatureYieldRules {
    struct Yield {std::int64_t hills=0,mountain=0,lake=0;};
    struct Surface {YieldValues base,river,hills;bool nonadditive=false,natural_wonder=false;};
    struct Belief {
        bool pantheon=false;
        NatureYieldMatrix terrains,features,resources;
        YieldValues natural_wonder,natural_wonder_modifier;
    };
    std::map<std::int64_t,Yield> yields;
    std::map<std::int64_t,Surface> terrains,features;
    NatureYieldMatrix resources;
    std::map<std::int64_t,Belief> beliefs;
    std::uint64_t yield_slots=0,feature_slots=0;
};
BnwNatureYieldRules load_bnw_nature_yield_rules(const content::RulesDatabase&);
struct BnwNatureYieldState {
    struct Player {NatureYieldMatrix unimproved_features;YieldValues natural_wonder;std::int64_t natural_wonder_modifier=0;};
    struct League {std::vector<std::int64_t> members,culture_per_natural_wonder;};
    std::map<std::int64_t,Player> players;
    std::vector<League> leagues;
};
// Also resolves secondary-pantheon counters in the shared player/city world.
BnwNatureYieldState restore_bnw_nature_yields(const saves::SavedSessionSnapshot&,
    const BnwNatureYieldRules&,CityYieldContext&);
struct BnwNatureYieldContext {
    const BnwNatureYieldRules &rules;
    const BnwNatureYieldState &state;
    PlotWaterContext &water;
    const ResourceVisibilityContext &resources;
    std::int64_t culture=4; // source enum; owned worlds may use a wide identity
    // Optional exact current BNW raw/owned registry; no freshness authority.
    const saves::SavedSessionSnapshot* current_city_registry=nullptr;
};
struct BnwNatureYieldQuote {
    std::int64_t terrain=0,religion_terrain=0,plot_hills=0,mountain=0,lake=0;
    std::int64_t feature=0,trait_feature=0,league=0,religion_feature=0,religion_wonder=0,trait_wonder=0,wonder_modifier=0;
    std::int64_t feature_total=0,resource=0,religion_resource=0,river=0,hills=0,total=0;
    bool suppressed=false,replaced=false;
};
// Complete BNW calculateNatureYield helper. Full plot yield/cache update is separate.
BnwNatureYieldQuote bnw_nature_yield(BnwNatureYieldContext&,std::int64_t plot,
    std::int64_t yield,std::int64_t team=-1,bool ignore_feature=false);
std::int64_t bnw_best_nature_yield(BnwNatureYieldContext&,std::int64_t plot,std::int64_t yield,std::int64_t team=-1);
} // namespace civ5::runtime
