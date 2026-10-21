#pragma once
#include "civ5/runtime/city_yields.hpp"
#include "civ5/runtime/lua_map_services.hpp"

namespace civ5::runtime {
struct PlotWaterRules {
    content::TechnologyProfile profile=content::TechnologyProfile::bnw;
    std::map<std::int64_t,bool> features, improvements, buildings;
    std::uint64_t building_slots=0;
};
PlotWaterRules load_plot_water_rules(const content::RulesDatabase &database,content::TechnologyProfile profile);
struct PlotWaterLandmass { std::int64_t tiles=0; bool water=false; };
struct PlotWaterState {
    saves::GameStateLayout layout=saves::GameStateLayout::expansion2_version1;
    std::map<std::int64_t,PlotWaterLandmass> landmasses;
};
PlotWaterState restore_plot_water(const saves::SavedSessionSnapshot &snapshot);
// Reuse the city's live building counts. VP's ordinary cached yield getter
// did not need these counts; its water-supplying buildings do.
void restore_vp_water_city_buildings(const saves::SavedSessionSnapshot &snapshot,
    CityYieldContext &cities,const PlotWaterRules &rules);
struct PlotWaterContext {
    saves::SavedMapProjection &map;
    const PlotWaterState &state;
    const PlotWaterRules &rules;
    const content::InfoTypeCatalog &types;
    const PlotPassability &passability;
    const ResearchContext &players;
    const CityYieldContext &cities;
    const content::GlobalDefineCache &defines;
    const content::CustomModOptions *options=nullptr;
};
// Source CvPlot::reset initializes VP's unsaved water caches to false. This
// must be an explicit load step, never a silently invented saved value.
void reset_plot_water_cache(PlotWaterContext &context);
void update_plot_water_flags(PlotWaterContext &context,std::int64_t plot);
bool plot_is_lake(PlotWaterContext &context,std::int64_t plot,bool use_cache=true);
bool plot_is_fresh_water(PlotWaterContext &context,std::int64_t plot,bool use_cache=true);
bool plot_is_adjacent_to_land(PlotWaterContext &context,std::int64_t plot,bool use_cache=true);
bool plot_is_adjacent_to_water(PlotWaterContext &context,std::int64_t plot);
bool plot_is_coastal_land(PlotWaterContext &context,std::int64_t plot,
    std::int64_t minimum_water_size=-1,bool use_cache=true,bool check_canals=false);
} // namespace civ5::runtime
