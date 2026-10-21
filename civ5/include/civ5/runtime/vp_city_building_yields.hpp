#pragma once
#include "civ5/runtime/vp_great_works.hpp"
#include "civ5/runtime/vp_city_building_yield_profile.hpp"

namespace civ5::runtime {
struct VpCityBuildingYieldRules {
    struct Building {bool faith_only=false;std::map<std::int64_t,YieldValues> accomplishments;};
    std::map<std::int64_t,Building> buildings;
    YieldValues faith_building_tourism;
    std::set<std::int64_t> yields;
    std::uint64_t building_slots=0;
};
VpCityBuildingYieldRules load_vp_city_building_yield_rules(const content::RulesDatabase&,const content::InfoTypeCatalog&);
void restore_vp_city_building_yields(const saves::SavedSessionSnapshot&,const VpCityBuildingYieldRules&,CityYieldContext&);
struct VpCityBuildingYieldContext {
    const VpCityBuildingYieldRules &rules;
    const VpGreatWorkContext &great_works;
    const std::map<std::int64_t,std::int64_t> &team_eras;
};
struct VpCityBuildingYieldQuote {
    std::int64_t base=0,era=0,per_building=0,per_tile=0,passing_trade=0,terrain=0,features=0,
        minor_resources=0,accomplishments=0,faith=0,religions=0,local_themes=0,total_times100=0;
};
// The complete buildings contribution in CvCity::getBaseYieldRateTimes100.
// Fresh great-work income is a separate contribution in that outer function.
VpCityBuildingYieldQuote vp_city_building_yield(const VpCityBuildingYieldContext&,const CityYieldState&,std::int64_t yield);
} // namespace civ5::runtime
