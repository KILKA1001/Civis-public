#pragma once
#include "civ5/runtime/maintenance.hpp"
#include "civ5/runtime/plot_water.hpp"
#include "civ5/runtime/plot_movement.hpp"
#include "civ5/runtime/vp_city_plot_profile.hpp"

namespace civ5::runtime {
struct VpNatureYieldRules {
    struct Yield {
        std::int64_t hills=0,mountain=0,lake=0,city=0,population_offset=0,population_divisor=0,min_city=0;
        // Flat fresh/dry, hill fresh/dry, mountain fresh/dry.
        std::array<std::int64_t,6> city_geography{};
    };
    struct Surface {
        std::array<YieldValues,5> yields; // base, river, hills, fresh, coastal
        bool nonadditive=false,natural_wonder=false,pseudo_natural_wonder=false;
    };
    struct Plot {
        YieldValues base;
        // Feature -> include wonders / exclude wonders -> yield. Zero sums
        // remain entries because the source checks whether this map is empty.
        std::map<std::int64_t,std::array<YieldValues,2>> adjacent_features;
    };
    struct Improvement {bool connects_all_resources=false,no_followup=false;};
    std::map<std::int64_t,Yield> yields;
    std::map<std::int64_t,Surface> terrains,features;
    std::map<std::int64_t,YieldValues> resources;
    std::map<std::int64_t,Plot> plots;
    std::map<std::int64_t,Improvement> improvements;
    std::uint64_t yield_slots=0,resource_slots=0;
};
VpNatureYieldRules load_vp_nature_yield_rules(const content::RulesDatabase&);
enum class VpNaturePlayerYield : std::size_t {
    city_times100,coastal,monopoly,capital_times100,capital_per_population,capital_per_empire,
    strength_threshold,military_policy,military_trait,trait_city,trait_coastal,trait_capital,count
};
struct VpNatureYieldState {
    struct Player {
        std::array<YieldValues,static_cast<std::size_t>(VpNaturePlayerYield::count)> yields;
        std::set<std::int64_t> has_global_monopoly;
        std::vector<std::int64_t> global_monopoly_cache; // unsaved, initially empty
    };
    struct ExtraYield {std::int64_t x=0,y=0;YieldValues yields;};
    std::map<std::int64_t,Player> players;
    std::vector<ExtraYield> extra_yields; // first coordinate match wins
};
// Binds city/unit additions to the already restored shared objects and verifies
// source identities. Does not rebuild monopoly or movement/water caches.
VpNatureYieldState restore_vp_nature_yields(const saves::SavedSessionSnapshot&,
    const VpNatureYieldRules&,CityYieldContext&,MaintenanceContext&);
// The global-count portion of UpdateMonopolyCache only; no combat caches.
void refresh_vp_global_monopoly_cache(VpNatureYieldState&,const VpNatureYieldRules&,
    const content::CustomModOptions&,std::int64_t player);
struct VpNatureYieldContext {
    const VpNatureYieldRules &rules;
    const VpNatureYieldState &state;
    PlotWaterContext &water;
    const PlotMovementContext &movement;
    CityYieldContext &cities;
    const MaintenanceContext &units;
    const MaintenanceRules &unit_rules;
    std::int64_t active_team=-1,production=1,desert=2;
};
struct VpNatureYieldQuote {
    std::int64_t terrain=0,desert=0,plot=0,extra=0,hills=0,mountain=0,lake=0,feature=0;
    std::int64_t river=0,surface_hills=0,fresh=0,coastal=0,resource=0,adjacent=0;
    std::int64_t city_base=0,city_resource=0,city_geography=0,garrison=0,city_change=0,population=0;
    std::int64_t military=0,trait_city=0,player_coastal=0,trait_coastal=0,monopoly=0,strength=0,strength_threshold=0;
    std::int64_t trait_capital=0,capital_population=0,capital_empire=0,city_and_capital=0,fortification=0,total=0;
    bool terrain_suppressed=false,feature_replaced=false,city_replaced=false;
};
// HasGarrison semantics: an override needs a saved garrison ID; a stale ID is cleared.
const MaintenanceUnit *vp_city_garrison(CityYieldContext&,const MaintenanceContext&,CityYieldState&);
// Full source helper; hypothetical feature/resource/improvement arguments have
// already been filtered by its caller. HasGarrison may clear a stale live ID.
VpNatureYieldQuote vp_nature_yield(VpNatureYieldContext&,std::int64_t plot,std::int64_t yield,
    std::int64_t player,std::int64_t feature,std::int64_t resource,std::int64_t improvement,
    CityYieldState *owning_city=nullptr,bool display=false);
} // namespace civ5::runtime
