#pragma once
#include "civ5/runtime/plot_water.hpp"
#include "civ5/runtime/vp_city_religion_profile.hpp"
#include "civ5/runtime/vp_player_religion_profile.hpp"

namespace civ5::runtime {
struct VpPlotReligionRules {
    using Matrix=std::map<std::int64_t,YieldValues>;
    struct Belief {
        bool pantheon=false,founder=false,follower=false,enhancer=false,reformation=false;
        bool improvement=false,no_improvement=false,resource=false,no_feature=false;
        std::int64_t civilization=-1;
        Matrix terrain,plot,feature,unimproved_feature,resources,improvements;
        YieldValues lake,wonder;
    };
    struct Improvement {
        bool connects_all=false,great_person=false,cityside=false,ignore_ownership=false;
        std::int64_t created_resource=-1;
        std::set<std::int64_t> trades,valid_resources;
    };
    std::map<std::int64_t,Belief> beliefs;
    std::map<std::int64_t,Improvement> improvements;
    std::set<std::int64_t> natural_wonders,yields;
    std::uint64_t belief_slots=0,resource_slots=0;
};
VpPlotReligionRules load_vp_plot_religion_rules(const content::RulesDatabase&,const content::InfoTypeCatalog&);
// Preserve saved majority/state religion and ordered status/list separately from lookup.
void restore_vp_plot_religion(const saves::SavedSessionSnapshot&,const VpPlotReligionRules&,CityYieldContext&);
const CityYieldReligionBeliefs *vp_find_religion(const CityYieldContext&,std::int64_t religion,std::int64_t player=-1);
std::int64_t vp_created_religion(const PlotWaterContext&,std::int64_t player,bool include_pantheon=false);
std::int64_t vp_created_pantheon(const CityYieldContext&,std::int64_t player);
bool vp_pantheon_belief_in_religion(const CityYieldContext&,std::int64_t belief,std::int64_t religion,std::int64_t player);
struct VpPlotReligionSelection {const CityYieldReligionBeliefs *majority=nullptr;std::int64_t secondary=-1;};
VpPlotReligionSelection vp_plot_religion_selection(const CityYieldContext&,const VpPlotReligionRules&,const CityYieldState&);
struct VpPlotReligionContext {const VpPlotReligionRules &rules;PlotWaterContext &water;};
struct VpPermanentPantheonSelection {
    const CityYieldReligionBeliefs *religion=nullptr;
    // GetBeliefInPantheon can select the first belief of the state religion.
    std::int64_t belief=-1;
};
VpPermanentPantheonSelection vp_permanent_pantheon(const PlotWaterContext&,const CityYieldState&,VpPlotReligionSelection);
// IsBeliefValid with bHolyCityOnly=false, the mode used by the plot helpers.
bool vp_plot_belief_valid(const VpPlotReligionContext&,std::int64_t belief,std::int64_t religion,
    std::int64_t player,const CityYieldState *city=nullptr);
struct VpReligionNatureQuote {std::int64_t terrain=0,plot=0,lake=0,unimproved_feature=0,feature=0,wonder=0,resource=0;std::int64_t total()const;};
// Full religious helpers. Arguments are supplied/filtered by the caller; no final clamp.
VpReligionNatureQuote vp_religion_nature_yield(const VpPlotReligionContext&,std::int64_t plot,std::int64_t yield,
    std::int64_t player,std::int64_t improvement,std::int64_t feature,std::int64_t resource,
    const CityYieldState *city,VpPlotReligionSelection);
std::int64_t vp_religion_improvement_yield(const VpPlotReligionContext&,std::int64_t yield,std::int64_t player,
    std::int64_t improvement,std::int64_t resource,const CityYieldState *city,VpPlotReligionSelection);
} // namespace civ5::runtime
