#pragma once
#include "civ5/runtime/city_yields.hpp"
#include "civ5/runtime/vp_city_food_profile.hpp"

namespace civ5::runtime {
struct VpCityFoodRules {
    struct Unit {bool food=false,military=false;};
    std::map<std::int64_t,Unit> units;
    YieldValues specialist_extra_by_era;
    std::uint64_t specialist_slots=0,promotion_slots=0;
};
VpCityFoodRules load_vp_city_food_rules(const content::RulesDatabase&);
void restore_vp_city_food(const saves::SavedSessionSnapshot&,const VpCityFoodRules&,CityYieldContext&);
struct VpCityFoodContext {
    CityYieldContext &cities;
    const ResearchContext &players;
    const std::map<std::int64_t,std::int64_t> &team_eras;
    const VpCityFoodRules &rules;
    const content::GlobalDefineCache &defines;
};
struct VpCityFoodRates {
    std::int64_t specialists=0,non_specialists=0,era_extra=0,
        per_non_specialist_times100=0,per_specialist_times100=0;
};
VpCityFoodRates vp_city_food_rates(const VpCityFoodContext&,const CityYieldState&);
// Excludes the default specialist and preserves a sum above city population.
std::int64_t vp_total_specialists(const VpCityFoodContext&,const CityYieldState&);
struct VpCityFoodConsumption {
    VpCityFoodRates rates;
    std::int64_t non_specialists_times100=0,specialists_times100=0,total_times100=0;
    bool protected_non_specialists=false,protected_size_one=false;
};
// The full yield caller supplies its freshly calculated food BEFORE consumption
// and growth (with its own ignore-process mode). Never substitute the saved net-food cache.
VpCityFoodConsumption vp_city_food_consumption(const VpCityFoodContext&,const CityYieldState&,
    std::int64_t food_before_consumption_times100,bool assume_no_reduction=false);
bool vp_unit_food_production(const VpCityFoodContext&,std::int64_t owner,std::int64_t unit);
bool vp_city_food_production(const VpCityFoodContext&,const CityYieldState&);
std::int64_t vp_food_to_production_times100(std::int64_t excess_food_times100);
} // namespace civ5::runtime
