#pragma once
#include "civ5/runtime/vp_player_plot_yield.hpp"
#include "civ5/saves/vp_plot_projection.hpp"

namespace civ5::runtime {
struct VpPlotYieldContext {
    VpNatureYieldContext &nature;
    VpPlayerPlotContext &player;
    std::int64_t active_player=-1,ice=0;
};
struct VpPlotYieldQuote {
    // 0=normal, 1=no terrain, 2=no-yield ice, 3=not potential work, 4=debug cache.
    std::int64_t early=0,player=-1,improvement=-1,route=-1,feature=-1,resource=-1;
    std::int64_t nature=0,religion_improvement=0,religion_nature=0,improvement_yield=0,player_yield=0;
    std::int64_t pantheon_improvement=0,pantheon_nature=0,extra=0,total=0;
    bool permanent_pantheon=false;
};
CityYieldState *vp_plot_effective_city(CityYieldContext&,const saves::SavedMapCellProjection&);
const CityYieldReligionBeliefs *vp_plot_permanent_pantheon(const VpPlotYieldContext&,const CityYieldState&,VpPlotReligionSelection);
// Full source queries on one shared world. Does not perform updateYield,
// rebuild load caches or change a working city's base terrain income.
VpPlotYieldQuote vp_plot_yield_fast(VpPlotYieldContext&,std::int64_t plot,std::int64_t yield,bool display,
    CityYieldState*,VpPlotReligionSelection,const CityYieldReligionBeliefs *pantheon=nullptr,bool assume_no_improvement=false);
VpPlotYieldQuote vp_plot_yield(VpPlotYieldContext&,std::int64_t plot,std::int64_t yield,bool display=false,
    CityYieldState *city=nullptr,bool assume_no_improvement=false);
} // namespace civ5::runtime
