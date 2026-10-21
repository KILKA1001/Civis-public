#pragma once
#include "civ5/runtime/vp_unit_supply.hpp"
#include "civ5/runtime/vp_city_food.hpp"
#include "civ5/runtime/vp_general_production_profile.hpp"

namespace civ5::runtime {
struct VpCityProductionRules {
    std::map<std::int64_t,bool> projects; // IsSpaceship from active Project Info.
    std::set<std::int64_t> processes;
};
VpCityProductionRules load_vp_city_production_rules(const content::RulesDatabase&);
void restore_vp_city_production(const saves::SavedSessionSnapshot&,CityYieldContext&);
struct VpCityProductionContext {
    VpUnitSupplyContext &supply;
    const VpCityFoodContext &food;
    const VpCityProductionRules &rules;
};
struct VpProductionMaintenance {std::int64_t over=0,modifier=0;};
// Complete production maintenance query; updates only the shared supply caches.
VpProductionMaintenance vp_unit_production_maintenance(VpUnitSupplyContext&,std::int64_t player);
struct VpPlayerProductionQuote {std::int64_t supply_over=0,supply=0,conquest=0,space=0,total=0;};
struct VpCityGeneralProductionQuote {std::int64_t industrial=0,puppet=0,occupied=0,specialists=0,total=0;};
VpPlayerProductionQuote vp_player_general_production(const VpCityProductionContext&,std::int64_t player);
VpCityGeneralProductionQuote vp_city_general_production(const VpCityProductionContext&,const CityYieldState&);
VpPlayerProductionQuote vp_player_project_production(const VpCityProductionContext&,std::int64_t player,std::int64_t project);
VpPlayerProductionQuote vp_player_process_production(const VpCityProductionContext&,std::int64_t player,std::int64_t process);
struct VpCityProductionQuote {
    VpCityGeneralProductionQuote city;
    VpPlayerProductionQuote player;
    std::int64_t space=0,second_conquest=0,total=0;
};
// Complete project/process overloads. Unit/building production, current-order
// dispatch, production progress and city yield cache updates are not supplied here.
VpCityProductionQuote vp_city_project_production(const VpCityProductionContext&,const CityYieldState&,std::int64_t project);
VpCityProductionQuote vp_city_process_production(const VpCityProductionContext&,const CityYieldState&,std::int64_t process);
} // namespace civ5::runtime
