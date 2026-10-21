#pragma once
#include "civ5/runtime/vp_city_production.hpp"
#include "civ5/runtime/vp_great_works.hpp"
#include "civ5/runtime/vp_building_production_profile.hpp"

namespace civ5::runtime {
struct VpBuildingProductionRules {
    struct Building {std::int64_t building_class=-1,technology=-1;};
    struct Class {std::int64_t world=-1,team=-1,national=-1;};
    struct Bonus {std::int64_t modifier=0,obsolete=-1;};
    struct Policy {std::int64_t branch=-1;YieldValues classes;};
    std::map<std::int64_t,Building> buildings;
    std::map<std::int64_t,Class> classes;
    YieldValues technology_eras,improvements,free_policies;
    std::map<std::int64_t,Bonus> resources,beliefs;
    std::map<std::int64_t,Policy> policies;
    std::uint64_t resource_slots=0,building_slots=0,policy_slots=0,branch_slots=0;
};
VpBuildingProductionRules load_vp_building_production_rules(const content::RulesDatabase&,const content::InfoTypeCatalog&);
void restore_vp_building_production(const saves::SavedSessionSnapshot&,const VpBuildingProductionRules&,CityYieldContext&,ResearchContext&,MaintenanceContext&);
struct VpBuildingProductionContext {
    const VpBuildingProductionRules &rules;
    const VpCityProductionContext &production;
    const VpGreatWorkContext &buildings;
};
bool vp_production_policy_blocked(const VpBuildingProductionContext&,std::int64_t player,std::int64_t policy);
std::int64_t vp_building_class_policy_production(const VpBuildingProductionContext&,std::int64_t player,std::int64_t building_class);
std::int64_t vp_capital_building_discount(const VpBuildingProductionContext&,std::int64_t player,std::int64_t building);
std::int64_t vp_wonder_to_building_discount(const VpBuildingProductionContext&,std::int64_t player,std::int64_t building);
struct VpWonderResourceQuote {bool incoming=false,outgoing=false;std::int64_t total=0;};
VpWonderResourceQuote vp_wonder_resource_production(const VpBuildingProductionContext&,const CityYieldState&,std::int64_t era);
std::int64_t vp_building_resource_production(const VpBuildingProductionContext&,const CityYieldState&,std::int64_t building);
struct VpWonderReligionQuote {std::int64_t majority=0,secondary=0,pantheon=0,total=0;};
VpWonderReligionQuote vp_wonder_religion_production(const VpBuildingProductionContext&,const CityYieldState&,std::int64_t era);
std::int64_t vp_wonder_unit_production(const VpBuildingProductionContext&,const CityYieldState&);
std::int64_t vp_wonder_improvement_production(const VpBuildingProductionContext&,const CityYieldState&);
struct VpPlayerBuildingProductionQuote {
    VpPlayerProductionQuote general;
    std::int64_t policy=0,wonder=0,limited=0,converted_wonder=0,converted_policy=0,building_class=0,capital=0,total=0;
};
VpPlayerBuildingProductionQuote vp_player_building_production(const VpBuildingProductionContext&,std::int64_t player,std::int64_t building);
struct VpCityBuildingProductionQuote {
    VpCityGeneralProductionQuote general;
    VpPlayerBuildingProductionQuote player;
    std::int64_t era=-1,conversion=0,wonder=0,resources=0,religion=0,units=0,improvements=0,building=0,past_eras=0,total=0;
};
// Complete building overload, including wonder helpers. Does not advance a queue.
VpCityBuildingProductionQuote vp_city_building_production(const VpBuildingProductionContext&,const CityYieldState&,std::int64_t building);
} // namespace civ5::runtime
