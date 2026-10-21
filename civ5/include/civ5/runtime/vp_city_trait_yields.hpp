#pragma once
#include "civ5/runtime/vp_player_plot_yield.hpp"
#include "civ5/runtime/vp_city_trait_yield_profile.hpp"
#include "civ5/runtime/treasury.hpp"

namespace civ5::runtime {
struct VpCityTraitYieldRules {
    std::uint64_t improvement_slots=0,yield_slots=0;
    std::set<std::int64_t> yields;
    std::int64_t culture=-1,tourism=-1;
};
VpCityTraitYieldRules load_vp_city_trait_yield_rules(const content::RulesDatabase&,const content::InfoTypeCatalog&);
void restore_vp_city_trait_yields(const saves::SavedSessionSnapshot&,const VpCityTraitYieldRules&,CityYieldContext&,const TreasuryContext&);
void rebuild_vp_trait_improvement_types(CityYieldContext&,std::int64_t player);
std::int64_t vp_different_trade_partners(const CityYieldContext&,std::int64_t player);
struct VpCityTraitYieldContext {
    const VpCityTraitYieldRules &rules;
    const ResearchContext &players;
    const CityYieldContext &cities;
    const TreasuryContext &treasury;
    const std::map<std::int64_t,std::int64_t> &team_eras;
    const content::GlobalDefineCache &defines;
    const content::CustomModOptions &options;
};
struct VpCityTraitPerTurnQuote {std::int64_t improvements=0,historic=0,partners=0,added=0,total=0;};
struct VpCityTraitYieldQuote {
    VpCityTraitPerTurnQuote per_turn;
    std::int64_t traits=0,unimproved=0,permanent_wltkd=0,culture=0,land_connections=0,golden_age_tourism=0,total_times100=0;
};
VpCityTraitPerTurnQuote vp_city_trait_per_turn_yield(const VpCityTraitYieldContext&,const CityYieldState&,std::int64_t yield);
VpCityTraitYieldQuote vp_city_trait_yield(const VpCityTraitYieldContext&,const CityYieldState&,std::int64_t yield);
std::int64_t vp_city_connection_gold_times100(const VpCityTraitYieldContext&,const CityYieldState*);
struct VpCityConnectionYieldQuote {std::int64_t route=0,industrial=0,total_times100=0;};
VpCityConnectionYieldQuote vp_city_connection_yield(const VpCityTraitYieldContext&,const CityYieldState&,std::int64_t yield);
} // namespace civ5::runtime
