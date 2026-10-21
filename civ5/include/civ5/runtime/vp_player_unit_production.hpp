#pragma once
#include "civ5/runtime/vp_city_production.hpp"
#include "civ5/runtime/vp_unit_traits.hpp"
#include "civ5/runtime/vp_unit_production_profile.hpp"

namespace civ5::runtime {
struct VpPlayerUnitProductionRules {
    struct Unit {
        bool military=false,found=false;
        std::int64_t combat=-1,special=-1;
        std::vector<std::int64_t> trait_modifiers;
    };
    std::map<std::int64_t,Unit> units;
    std::map<std::int64_t,std::vector<std::int64_t>> special_trait_modifiers;
    std::uint64_t combat_slots=0,trait_slots=0;
};
VpPlayerUnitProductionRules load_vp_player_unit_production_rules(const content::RulesDatabase&,const content::InfoTypeCatalog&);
void restore_vp_player_unit_production(const saves::SavedSessionSnapshot&,const VpPlayerUnitProductionRules&,CityYieldContext&);
struct VpPlayerUnitProductionContext {
    const VpPlayerUnitProductionRules &rules;
    const VpCityProductionContext &production;
    const VpUnitTraitContext &traits;
};
struct VpPlayerUnitProductionQuote {
    VpPlayerProductionQuote general;
    std::int64_t major_wars=0,military=0,war_military=0,settler=0,combat=0,unit_traits=0,special_traits=0,total=0;
};
// Complete CvPlayer unit overload. City modifiers, queue progress and turns are separate.
VpPlayerUnitProductionQuote vp_player_unit_production(const VpPlayerUnitProductionContext&,std::int64_t player,std::int64_t unit);
} // namespace civ5::runtime
