#pragma once
#include "civ5/runtime/vp_city_food.hpp"
#include "civ5/runtime/vp_great_works.hpp"
#include "civ5/runtime/vp_nature_yield.hpp"
#include "civ5/runtime/vp_city_happiness_profile.hpp"

namespace civ5::runtime {
struct VpCityHappinessRules {
    struct Handicap {std::int64_t capital=0,ai_capital=0;};
    std::map<std::int64_t,Handicap> handicaps;
    std::map<std::int64_t,bool> dummy_policies;
    std::set<std::int64_t> yields;
    std::uint64_t yield_slots=0;
    std::int64_t culture=-1,crime=-1;
};
VpCityHappinessRules load_vp_city_happiness_rules(const content::RulesDatabase&,const content::InfoTypeCatalog&);
void restore_vp_city_happiness(const saves::SavedSessionSnapshot&,CityYieldContext&);
// Complete native UpdateTradeStats, including its per-slice cache and reset.
void vp_reset_trade_stats(CityYieldContext&,std::int64_t player);
void vp_update_trade_stats(CityYieldContext&,std::int64_t player);
std::int64_t vp_trade_routes_from_city(CityYieldContext&,std::int64_t player,const CityYieldState*);
struct VpCityHappinessContext {
    const VpCityHappinessRules &rules;
    const VpGreatWorkContext &great_works;
    const VpCityFoodContext &food;
    const MaintenanceContext &units;
    ResearchContext &players;
    const content::RulesDatabase &database;
};
struct VpCityPolicyHappinessQuote {
    std::int64_t garrison=0,strength=0,great_works=0,population_global=0,capital=0,
        policies_from_policies=0,policies=0,specialists=0,per_city=0,population=0,trade=0,total=0;
};
VpCityPolicyHappinessQuote vp_city_policy_happiness(const VpCityHappinessContext&,CityYieldState&,std::int64_t population_change=0);
struct VpCityHappinessQuote {
    VpCityPolicyHappinessQuote policies;
    std::int64_t empire=0,buildings=0,building_classes=0,religion=0,unmodded_buildings=0,handicap=0,crime=0,development=0,total=0;
    bool suppressed=false;
};
VpCityHappinessQuote vp_city_local_happiness(const VpCityHappinessContext&,CityYieldState&,std::int64_t population_change=0,bool exclude_empire=false);
struct VpCityPolicyYieldQuote {std::int64_t base=0,happiness=0,total_times100=0;};
VpCityPolicyYieldQuote vp_city_policy_yield(const VpCityHappinessContext&,CityYieldState&,std::int64_t yield);
} // namespace civ5::runtime
