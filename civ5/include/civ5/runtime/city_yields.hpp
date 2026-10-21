#pragma once
#include "civ5/runtime/research_context.hpp"
#include "civ5/runtime/profile_extensions.hpp"
#include "civ5/runtime/yield_fraction.hpp"
#include <array>
#include <set>
#include <stdexcept>

namespace civ5::runtime {
using YieldValues = std::map<std::int64_t, std::int64_t>;
using YieldModifierRows = std::map<std::int64_t,std::vector<std::pair<std::int64_t,std::int64_t>>>;
struct CityYieldRules {
    content::TechnologyProfile profile = content::TechnologyProfile::bnw;
    YieldValues golden_age_modifiers, building_trade_modifiers;
    std::map<std::int64_t, YieldValues> belief_follower_maximum;
    // Civilization -> source BuildingClass index -> Building index, -1 absent.
    std::map<std::int64_t, YieldValues> civilization_buildings;
    std::uint64_t building_class_slots = 0, building_slots = 0, belief_slots = 0;
};
CityYieldRules load_city_yield_rules(const content::RulesDatabase &database,
    content::TechnologyProfile profile);
void load_city_building_replacements(const content::RulesDatabase&,CityYieldRules&);
struct CityYieldReligion { std::int64_t religion = -1, followers = 0, pressure = 0; };
struct CityYieldReligionBeliefs {
    std::int64_t religion = -1, founder = -1;
    // Keep source order and duplicates: secondary pantheon uses the first match.
    std::vector<std::int64_t> beliefs;
    ProfileExtensions extensions;
};
struct CityYieldComponents {
    std::int64_t terrain = 0, buildings = 0, specialists = 0, misc = 0, religion = 0;
    std::int64_t per_population = 0, per_religion = 0, rate_modifier = 0, resource_modifier = 0, process_modifier = 0;
};
struct CityYieldState {
    std::optional<std::pair<std::uint64_t,std::uint64_t>> source_core;
    std::int64_t owner = -1, id = -1, x = 0, y = 0, area = -1, population = 0;
    bool resistance = false, razing = false, puppet = false;
    std::map<std::int64_t, CityYieldComponents> yields;
    YieldValues real_buildings, free_buildings;
    std::vector<CityYieldReligion> religions;
    bool building_counts_resolved=false;
    std::optional<std::int64_t> we_love_king_turns;
    ProfileExtensions extensions;
    struct ProductionOrder {
        std::int64_t type=-1,item=-1,secondary=-1;
        bool save=false,rush=false;
        // BNW v5+ writes an Info hash; retain it even after resolving the ID.
        std::optional<std::uint32_t> source_hash;
        bool operator==(const ProductionOrder&)const=default;
    };
    std::optional<std::vector<ProductionOrder>> production_orders;
    struct ProductionProgress {
        content::TechnologyProfile source_profile=content::TechnologyProfile::bnw;
        std::int64_t overflow_times100=0,feature_times100=0;
        YieldValues units,unit_times,buildings,building_times,projects;
        // Missing mod Info remains available for a later profile reload.
        using HashedRows=std::vector<std::pair<std::uint32_t,std::int64_t>>;
        HashedRows unresolved_units,unresolved_unit_times,unresolved_buildings,unresolved_building_times;
        bool fully_resolved=true;
    };
    std::optional<ProductionProgress> production_progress;
};
struct CityYieldPlayer {
    std::int64_t total_population=0; // Saved empire total, not a sum of live cities.
    std::int64_t civilization = -1, capital = -1;
    bool minor = false, golden_age = false;
    YieldValues modifiers, capital_modifiers;
    // BNW computes unhappiness from the whole empire. A saved happiness value
    // alone cannot supply this difference. The live happiness subsystem must
    // resolve it before gold/production queries; absence is never zero.
    std::optional<std::int64_t> excess_happiness;
    std::map<std::int64_t, CityYieldState> cities;
    std::optional<std::int64_t> second_pantheon_count;
    ProfileExtensions extensions;
};
struct CityYieldContext {
    content::TechnologyProfile profile = content::TechnologyProfile::bnw;
    ProfileExtensions extensions;
    std::uint64_t yield_slots = 0;
    std::optional<std::int64_t> turn_slice;
    // Default source enum identities. Owned wider worlds may assign wide IDs.
    std::int64_t production = 1, gold = 2, science = 3, pantheon = 0;
    bool no_happiness = false;
    std::map<std::int64_t, CityYieldPlayer> players;
    std::map<std::pair<std::int64_t, std::int64_t>, YieldValues> area_modifiers;
    std::vector<CityYieldReligionBeliefs> religions;
    std::vector<saves::TradeRouteObservation> trade_routes;
    struct League {std::vector<std::int64_t> members,culture_per_natural_wonder,culture_per_wonder;};
    std::vector<League> leagues;
    // Global work identities are independent of city slots and can be sparse/wide.
    std::optional<std::map<std::int64_t,saves::GreatWorkObservation>> great_works;
};
struct CityYieldUnavailable : std::runtime_error { using std::runtime_error::runtime_error; };
CityYieldContext restore_city_yields(const saves::SavedSessionSnapshot &snapshot,
    ResearchContext &setup, const content::RulesDatabase &database,
    const CityYieldRules &rules, const content::InfoTypeCatalog &types);
struct CityYieldQuote {
    std::int64_t base_times100 = 0, modifier = 0, process_times100 = 0, trade_times100 = 0, total_times100 = 0;
    bool cached = false, suppressed = false;
};
// BNW's actual getYieldRateTimes100; VP's ordinary cached overload. VP's
// ignore-trade/recompute path requires the full cache updater and is rejected.
// Production cycles in malformed mod rules are diagnosed instead of recursing.
CityYieldQuote city_yield_times100(const CityYieldContext &state, const CityYieldRules &rules,
    const content::GlobalDefineCache &defines, std::int64_t player, std::int64_t city,
    std::int64_t yield, bool ignore_trade = false);
namespace detail {
// Pure arithmetic, NOT an import/refresh capability. Source-bound production
// FOOD callers use BnwCityCurrentFoodRate; its process recursion explicitly refuses.
std::int32_t bnw_native_food_rate_math(const CityYieldContext&,const CityYieldRules&,
 const content::GlobalDefineCache&,std::int64_t player,std::int64_t city);
// Detached checked arithmetic only; genuine readers bind actual source operands.
// Checks the SAME shared base/modifier prefix before a genuine happiness read.
std::int32_t bnw_native_gold_rate_prefix_math(const CityYieldContext&,std::int64_t player,std::int64_t city);
std::int32_t bnw_native_gold_rate_math(const CityYieldContext&,const CityYieldRules&,const content::GlobalDefineCache&,std::int64_t player,std::int64_t city);
std::int32_t bnw_native_science_rate_prefix_math(const CityYieldContext&,std::int64_t player,std::int64_t city);
// Pure Reset/hashedRead reconstruction. A DTO result cannot issue a current owner.
std::vector<std::int32_t> bnw_native_building_counts_reset_read_math(
 const saves::CityProductionRecord&,std::uint64_t slots,const content::InfoTypeCatalog&);
std::int32_t bnw_native_production_rate_prefix_math(const CityYieldContext&,std::int64_t player,std::int64_t city);
// Pure native arithmetic; supplied contexts cannot issue a city command.
std::int32_t bnw_native_current_production_difference_prefix_math(const CityYieldContext&,std::int64_t,std::int64_t);
std::int32_t bnw_native_current_production_difference_math(const CityYieldContext&,const CityYieldRules&,const content::GlobalDefineCache&,std::int64_t,std::int64_t,std::int32_t,std::int32_t);
std::int32_t bnw_native_production_rate_math(const CityYieldContext&,const CityYieldRules&,const content::GlobalDefineCache&,std::int64_t player,std::int64_t city);
std::int32_t bnw_native_science_rate_math(const CityYieldContext&,const CityYieldRules&,
 const content::GlobalDefineCache&,std::int64_t player,std::int64_t city);
}
std::int64_t science_from_cities_times100(const CityYieldContext &state, const CityYieldRules &rules,
    const content::GlobalDefineCache &defines, std::int64_t player);
} // namespace civ5::runtime
