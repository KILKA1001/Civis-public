#pragma once
#include "civ5/runtime/vp_plot_religion.hpp"
#include "civ5/runtime/science.hpp"
#include "civ5/runtime/vp_city_growth_profile.hpp"

namespace civ5::runtime {
struct VpCityGrowthRules {
    struct Belief {std::int64_t modifier=0;bool requires_peace=false;};
    std::map<std::int64_t,Belief> beliefs;
    std::int64_t food=0;
};
VpCityGrowthRules load_vp_city_growth_rules(const content::RulesDatabase&);
void restore_vp_city_growth(const saves::SavedSessionSnapshot&,CityYieldContext&);
struct VpCityGrowthContext {
    VpPlotReligionContext &religion;
    const PlayerResearchContext &diplomacy;
    // Reuse the shared player happiness cache and civilization domains.
    const ScienceContext &science;
    const VpCityGrowthRules &rules;
};
struct VpUnitSupplyContext;
struct VpCityGrowthQuote {
    std::int64_t capital=0,player=0,golden_city=0,golden_player=0,golden_trait=0,
        supply=0,event=0,tourism=0,puppet=0,beliefs=0,followers=0,secondary=0,
        pantheon=0,happiness=0,celebration=0,total=0;
    std::int64_t excess_happiness=0;
    bool at_peace=true,unhappy=false;
};
// Fresh GetNumUnitsOutOfSupply is an explicit required dependency. Never pass
// the saved m_iUnitGrowthMaintenanceMod in its place. For the shared world's
// current supply, use vp_city_growth below. The full city-yield updater is pending.
VpCityGrowthQuote vp_city_growth_modifiers(const VpCityGrowthContext&,const CityYieldState&,
    std::int64_t units_out_of_supply,std::int64_t assumed_local_happiness_change=0);
VpCityGrowthQuote vp_city_growth(const VpCityGrowthContext&,VpUnitSupplyContext&,const CityYieldState&,
    std::int64_t assumed_local_happiness_change=0);
} // namespace civ5::runtime
