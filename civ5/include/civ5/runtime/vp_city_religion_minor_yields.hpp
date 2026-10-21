#pragma once
#include "civ5/runtime/vp_plot_religion.hpp"
#include "civ5/runtime/vp_city_religion_minor_yield_profile.hpp"
#include "civ5/runtime/treasury.hpp"

namespace civ5::runtime {
void restore_vp_city_religion_minor_yields(const saves::SavedSessionSnapshot&,CityYieldContext&,const TreasuryContext&);
struct VpCityReligionMinorContext {
    const VpPlotReligionContext &religion;
    const TreasuryContext &treasury;
};
// GetOwnedReligion's default bIgnoreLocal=false path.
std::int64_t vp_owned_religion(const VpCityReligionMinorContext&,std::int64_t player);
struct VpCityReligionYieldQuote {
    std::int64_t base=0,terrain=0,features=0,own_pantheon=0,special=0,total_times100=0;
};
struct VpCityMinorYieldQuote {
    std::int64_t modifier=0,city_scaling=0,alliance_base=0,friendship_base=0,
        per_ally=0,per_friend=0,from_minors=0,total_times100=0;
};
VpCityReligionYieldQuote vp_city_religion_yield(const VpCityReligionMinorContext&,const CityYieldState&,std::int64_t yield);
VpCityMinorYieldQuote vp_city_minor_yield(const VpCityReligionMinorContext&,const CityYieldState&,std::int64_t yield);
} // namespace civ5::runtime
