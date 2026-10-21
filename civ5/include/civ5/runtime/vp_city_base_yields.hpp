#pragma once
#include "civ5/runtime/vp_city_happiness.hpp"
#include "civ5/runtime/vp_city_building_yields.hpp"
#include "civ5/runtime/vp_city_religion_minor_yields.hpp"
#include "civ5/runtime/vp_city_trait_yields.hpp"
#include "civ5/runtime/vp_city_base_yield_profile.hpp"

namespace civ5::runtime {
struct VpCityBaseYieldRules {
    struct Building {std::int64_t building_class=-1,culture=0;};
    std::map<std::int64_t,Building> buildings;
    std::map<std::int64_t,bool> world_wonder_classes,natural_wonders;
    std::set<std::int64_t> yields;
    std::int64_t culture=-1,tourism=-1,culture_local=-1;
};
VpCityBaseYieldRules load_vp_city_base_yield_rules(const content::RulesDatabase&,const content::InfoTypeCatalog&);
std::optional<std::int64_t> vp_city_ring_plot(const saves::SavedMapProjection&,std::int64_t x,std::int64_t y,std::int64_t ring_index);
void vp_rebuild_city_worked_plots(const saves::SavedMapProjection&,CityYieldContext&,CityYieldState&);
void restore_vp_city_base_yields(const saves::SavedSessionSnapshot&,CityYieldContext&,MaintenanceContext&);
std::int64_t vp_league_world_wonder_yield(const CityYieldContext&,std::int64_t player,std::int64_t yield,std::int64_t culture);
struct VpCityBaseYieldContext {
    const VpCityBaseYieldRules &rules;
    const VpCityBuildingYieldContext &buildings;
    const VpCityHappinessContext &happiness;
    const VpCityReligionMinorContext &religion;
    const VpCityTraitYieldContext &traits;
};
struct VpCityLandmarkTourismQuote {std::int64_t percent=0,wonders=0,natural_wonders=0,terrain=0,total_times100=0;};
VpCityLandmarkTourismQuote vp_city_landmark_tourism(const VpCityBaseYieldContext&,const CityYieldState&);
struct VpCityBaseYieldQuote {
    std::int64_t local_culture=0,great_works=0,terrain=0,buildings=0,policies=0,specialists=0,
        religion=0,minors=0,traits=0,corporations=0,events=0,connection=0,league=0,landmarks=0,units=0,
        state_religion=0,population=0,empire_population=0,industrial=0,non_specialists=0,misc=0,
        previous_golden_ages=0,capital_tiles=0,total_times100=0;
};
// Complete getBaseYieldRateTimes100; later modifiers and city cache updates are separate.
VpCityBaseYieldQuote vp_city_base_yield(const VpCityBaseYieldContext&,CityYieldState&,std::int64_t yield);
} // namespace civ5::runtime
