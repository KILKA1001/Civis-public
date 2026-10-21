#pragma once
#include "civ5/runtime/city_yields.hpp"
#include "civ5/runtime/player_research.hpp"
#include "civ5/runtime/profile_extensions.hpp"

namespace civ5::runtime {
struct MaintenanceHandicap {
    std::int64_t free_units=0, ai_free_units=0, unit=0, ai_unit=0;
    std::int64_t building=0, ai_building=0, improvement=0, ai_improvement=0;
    std::int64_t inflation=0, ai_inflation=0, ai_per_era=0;
};
struct MaintenanceUnitInfo {
    std::int64_t domain=-1, combat=-1, range=0, nuke=0;
    bool no_maintenance=false;
};
struct MaintenanceRules {
    content::TechnologyProfile profile=content::TechnologyProfile::bnw;
    ProfileExtensions extensions;
    std::map<std::int64_t,MaintenanceHandicap> handicaps;
    std::map<std::int64_t,std::int64_t> era_building_percent;
    std::map<std::int64_t,MaintenanceUnitInfo> units;
    std::set<std::int64_t> combat_classes;
};
MaintenanceRules load_maintenance_rules(const content::RulesDatabase &database,
    content::TechnologyProfile profile,const content::InfoTypeCatalog &types);
struct MaintenanceUnit {
    std::int64_t type=-1, domain=-1, combat=-1;
    bool military=false, civilian=true, no_maintenance=false;
    // BNW asks the live plot's best defender, not the saved garrison ID.
    // nullopt requires that world service when the free-garrison branch runs.
    std::optional<bool> garrison;
    std::optional<std::pair<std::uint64_t,std::uint64_t>> source_core;
    ProfileExtensions profile_extensions;
};
struct MaintenancePlayer {
    std::int64_t base_free=0, gold_per_unit_times100=0, extra_unit_cost=0, outside_units=0;
    std::int64_t unit_modifier=0, supply_modifier=0, building_modifier=0, improvement_modifier=0;
    std::int64_t land_modifier=0, naval_modifier=0, base_building=0, base_improvement=0;
    std::int64_t cached_unit_expense=0, cached_supply_expense=0;
    std::int64_t diplomacy_gold=0;
    bool free_garrison=false, barbarian=false;
    std::map<std::int64_t,MaintenanceUnit> units;
    YieldValues combat_modifiers;
    ProfileExtensions profile_extensions;
};
struct MaintenanceContext {
    content::TechnologyProfile profile=content::TechnologyProfile::bnw;
    ProfileExtensions extensions;
    std::int64_t elapsed_turns=0, estimate_end_turn=0, default_end_turn=0;
    std::map<std::int64_t,MaintenancePlayer> players;
    std::map<std::int64_t,std::int64_t> team_eras;
};
struct MaintenanceWorld {
    const ResearchContext &setup;
    const PlayerResearchContext &diplomacy;
    const CityYieldContext &cities;
    std::span<const TeamTechnologies> technologies;
};
MaintenanceContext restore_maintenance(const saves::SavedSessionSnapshot &snapshot,
    const MaintenanceWorld &world,const MaintenanceRules &rules,const content::InfoTypeCatalog &types);
struct MaintenanceUnavailable : std::runtime_error { using std::runtime_error::runtime_error; };
struct UnitMaintenanceQuote {
    std::int64_t free_units=0, paid_units=0, base_times100=0, extra_times100=0, total=0;
    double multiplier=0, exponent=0;
};
UnitMaintenanceQuote unit_maintenance(const MaintenanceContext &state,const MaintenanceWorld &world,
    const MaintenanceRules &rules,const content::GlobalDefineCache &defines,
    const content::CustomModOptions *options,std::int64_t player,std::int64_t paid_units_override=-1);
struct MaintenanceQuote {
    UnitMaintenanceQuote units;
    std::int64_t supply=0, buildings=0, improvements=0, total=0;
    ProfileExtensions profile_extensions;
};
// Source CalculatePreInflatedCosts / CalculateTotalCosts. Updates expense
// caches in source order; vassal queries read other players' current caches.
// This is expenditure, not gross/net treasury income or a completed turn.
MaintenanceQuote maintenance_costs(MaintenanceContext &state,const MaintenanceWorld &world,
    const MaintenanceRules &rules,const content::GlobalDefineCache &defines,
    const content::CustomModOptions *options,std::int64_t player);
} // namespace civ5::runtime
