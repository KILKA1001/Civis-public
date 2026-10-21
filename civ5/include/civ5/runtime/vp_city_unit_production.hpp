#pragma once
#include "civ5/runtime/vp_player_unit_production.hpp"
#include "civ5/runtime/vp_great_works.hpp"
#include "civ5/runtime/vp_city_growth.hpp"

namespace civ5::runtime {
struct VpCityUnitProductionRules {
    struct Unit {
        std::int64_t unit_class=-1,domain=-1,combat=-1,spaceship=-1;
        std::int64_t strength=0,ranged=0,nuke=0;
        bool military=false,found=false,found_mid=false,found_late=false,found_abroad=false;
        YieldValues required_resources,total_resources,buildings;
    };
    struct Belief {std::int64_t unit=0;YieldValues combats;};
    std::map<std::int64_t,Unit> units;
    std::map<std::int64_t,Belief> beliefs;
    std::map<std::int64_t,YieldValues> corporations;
    std::set<std::int64_t> buildings;
    std::uint64_t class_slots=0,combat_slots=0,resource_slots=0,building_slots=0;
};
VpCityUnitProductionRules load_vp_city_unit_production_rules(const content::RulesDatabase&,const content::InfoTypeCatalog&);
void restore_vp_city_unit_production(const saves::SavedSessionSnapshot&,const VpCityUnitProductionRules&,
    CityYieldContext&,MaintenanceContext&);
struct VpCityUnitProductionContext {
    const VpCityUnitProductionRules &rules;
    const VpPlayerUnitProductionContext &player;
    const VpGreatWorkContext &buildings;
    const VpCityGrowthContext &growth;
};
struct VpCityUnitProductionQuote {
    VpCityGeneralProductionQuote city;
    VpPlayerUnitProductionQuote player;
    std::int64_t capital_settler=0,unit_class=0,domain=0,pledges=0,combat=0,corporation=0,
        unhappiness=0,military_city=0,military_units=0,space_city=0,space_player=0,
        space_leagues=0,religion_unit=0,religion_combat=0,buildings=0,total=0;
};
// CvCity::getProductionModifier(UnitTypes) for the active VP profile. The
// happiness branch reads the first queued order, as in the original source.
VpCityUnitProductionQuote vp_city_unit_production(const VpCityUnitProductionContext&,
    const CityYieldState&,std::int64_t unit,bool ignore_happiness=false);
} // namespace civ5::runtime
