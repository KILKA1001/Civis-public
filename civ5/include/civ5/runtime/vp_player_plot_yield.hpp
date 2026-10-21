#pragma once
#include "civ5/runtime/improvement_yield_calculation.hpp"
#include "civ5/runtime/vp_nature_yield.hpp"
#include "civ5/runtime/vp_plot_religion.hpp"
#include "civ5/runtime/vp_city_trait_yield_profile.hpp"
#include "civ5/runtime/vp_city_plot_profile.hpp"

namespace civ5::runtime {
enum class VpPlayerPlotVector : std::size_t {
    threshold,sea,lake,connection,wonder,trait_mountain,trait_wonder,trait_sea,trait_luxury,trait_strategic,count
};
enum class VpPlayerPlotMatrix : std::size_t {
    plot,terrain,feature,resource,unimproved,extra_improvement,
    trait_plot,trait_terrain,trait_feature,trait_resource,trait_unimproved,trait_resource_ga,trait_resource_ga_cap,count
};
struct VpPlayerPlotRules {
    struct Yield {std::int64_t golden_threshold=0,golden=0;};
    struct Resource {std::int64_t usage=0;YieldValues monopoly;};
    struct Policy {std::int64_t branch=-1;YieldValues improvement_culture;};
    std::map<std::int64_t,Yield> yields;
    std::map<std::int64_t,Resource> resources;
    std::map<std::int64_t,ImprovementYieldMatrix> accomplishments;
    std::map<std::int64_t,Policy> policies;
    std::map<std::int64_t,YieldValues> belief_wonder_modifier;
    std::uint64_t yield_slots=0,plot_slots=0,terrain_slots=0,feature_slots=0,resource_slots=0,
        improvement_slots=0,accomplishment_slots=0,policy_slots=0,branch_slots=0;
};
VpPlayerPlotRules load_vp_player_plot_rules(const content::RulesDatabase&,const content::InfoTypeCatalog&);
struct VpPlayerPlotState {
    struct Player {
        std::array<YieldValues,static_cast<std::size_t>(VpPlayerPlotVector::count)> yields;
        std::array<ImprovementYieldMatrix,static_cast<std::size_t>(VpPlayerPlotMatrix::count)> matrices;
        std::set<std::int64_t> blocked_branches;
        std::int64_t golden_ages=0,monopoly_flat=0,wonder_modifier=0,wltkd_gp_modifier=0,
            policy_culture=0;
        bool trade_route_only=false;
    };
    std::map<std::int64_t,Player> players;
    std::map<std::int64_t,std::array<ImprovementYieldMatrix,2>> teams; // terrain, feature
};
VpPlayerPlotState restore_vp_player_plot_yields(const saves::SavedSessionSnapshot&,const VpPlayerPlotRules&,CityYieldContext&);
struct VpPlayerPlotContext {
    const VpPlayerPlotRules &rules;
    const VpPlayerPlotState &state;
    ImprovementYieldContext &improvements;
    const VpNatureYieldRules &nature_rules;
    const VpNatureYieldState &nature_state;
    const PlotMovementContext &movement;
    const VpPlotReligionRules &religion_rules;
    std::int64_t active_team=-1,culture=4,hill_terrain=8,mountain_terrain=7;
    std::set<std::int64_t> league_balance_yields{0,1,2,3,5};
};
std::int64_t vp_trait_era_modifier(const VpCityTraitYieldProfileState::TraitEraState&,std::int64_t era);
// Full calculatePlayerYield helper. Returns an additional signed amount;
// current_yield participates only in the final golden-age threshold.
std::int64_t vp_player_plot_yield(VpPlayerPlotContext&,std::int64_t plot,std::int64_t yield,
    std::int64_t current_yield,std::int64_t player,std::int64_t improvement,std::int64_t feature,
    std::int64_t resource,const CityYieldState *city,VpPlotReligionSelection,
    const CityYieldReligionBeliefs *permanent_pantheon=nullptr,bool display=false,
    std::optional<std::int64_t> forced_connection={});
} // namespace civ5::runtime
