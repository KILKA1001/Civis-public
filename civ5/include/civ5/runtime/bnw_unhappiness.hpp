#pragma once
#include "civ5/runtime/city_yields.hpp"

namespace civ5::runtime {
struct LiveSessionState;
class BnwCitySpecialistCounts;
struct BnwUnhappinessHandicap {
    std::int64_t cities = 0, population = 0, ai = 0;
};
struct BnwUnhappinessRules {
    std::map<std::int64_t,BnwUnhappinessHandicap> handicaps;
    std::uint64_t specialist_slots = 0;
};
BnwUnhappinessRules load_bnw_unhappiness_rules(const content::RulesDatabase &database);
struct BnwCityUnhappiness {
    bool occupied = false, no_occupied_unhappiness = false, ignore = false;
    YieldValues specialists;
};
struct BnwPlayerUnhappiness {
    std::int64_t happiness = 0, units = 0, units_modifier = 0, population_modifier = 0;
    std::int64_t cities_modifier = 0, occupied_modifier = 0, capital_modifier = 0;
    std::int64_t trait_cities_modifier = 0, trait_population_modifier = 0, public_opinion = 0;
    bool half_specialists = false;
    std::map<std::int64_t,BnwCityUnhappiness> cities;
};
struct BnwUnhappinessContext {
    std::int64_t world_cities_percent = 0;
    std::map<std::int64_t,BnwPlayerUnhappiness> players;
};
// Additional saved inputs; population, puppet, capital and identity remain in
// the shared city-yield state, while human/alive/team/handicap remain in setup.
BnwUnhappinessContext restore_bnw_unhappiness(const saves::SavedSessionSnapshot &snapshot,
    const ResearchContext &setup,const CityYieldContext &cities,
    const BnwUnhappinessRules &rules,const content::InfoTypeCatalog &types);
using UnhappinessCityKey = std::pair<std::int64_t,std::int64_t>; // owner, city ID
struct BnwUnhappinessAssumptions {
    std::optional<UnhappinessCityKey> annexed, puppeted;
};
struct BnwUnhappinessQuote {
    std::int64_t normal_cities_times100 = 0, captured_cities_times100 = 0;
    std::int64_t normal_population_times100 = 0, occupied_population_times100 = 0, units_times100 = 0;
    std::int64_t public_opinion = 0, before_ai = 0, total = 0, excess_happiness = 0;
    bool ai_discount_applied = false;
};
// Full BNW CvPlayer::GetUnhappiness and GetExcessHappiness for the current
// owned state, including source annex/puppet preview arguments. Does not
// recalculate the separate positive happiness cache or issue world mutations.
BnwUnhappinessQuote bnw_unhappiness(const BnwUnhappinessContext &state,const ResearchContext &setup,
    const CityYieldContext &cities,const BnwUnhappinessRules &rules,
    const content::GlobalDefineCache &defines,std::int64_t player,
    BnwUnhappinessAssumptions assumptions = {});
// Evaluate first, then publish all resolved excess values. No partial publish
// if some player's required input is missing. Re-run after relevant mutations.
std::map<std::int64_t,BnwUnhappinessQuote> bind_bnw_unhappiness(const BnwUnhappinessContext &state,
    const ResearchContext &setup,CityYieldContext &cities,const BnwUnhappinessRules &rules,
    const content::GlobalDefineCache &defines);
namespace detail {
// Pure read-model/math helpers; callers guard actual rules/source before use.
void require_bnw_current_unhappiness_inputs(const LiveSessionState&,const BnwUnhappinessRules&,
 const content::InfoTypeCatalog&,std::int64_t player);
BnwUnhappinessQuote bnw_native_unhappiness_math(const BnwUnhappinessContext&,const ResearchContext&,
 const CityYieldContext&,const BnwUnhappinessRules&,const content::GlobalDefineCache&,std::int64_t player);
BnwUnhappinessQuote bnw_native_unhappiness_math_with_specialists(const BnwUnhappinessContext&,const ResearchContext&,
 const CityYieldContext&,const BnwUnhappinessRules&,const content::GlobalDefineCache&,std::int64_t player,const BnwCitySpecialistCounts&);
}
} // namespace civ5::runtime
