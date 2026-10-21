#pragma once
#include "civ5/runtime/maintenance.hpp"
#include "civ5/runtime/profile_extensions.hpp"
#include <functional>

namespace civ5::runtime {
struct TreasuryBelief {
    std::int64_t gold_per_city=0, followers_per_gold=0, city_state_modifier=0, civilization=-1;
    bool founder=false, enhancer=false, follower=false, reformation=false, pantheon=false;
};
struct MinorYieldRules {
    // Yield -> era -> amount in hundredths. Duplicate SQL rows add together.
    std::map<std::int64_t,YieldValues> friends, allies;
    // Type is stable across mod SQL that reshuffles numeric database IDs.
    std::string type;
};
struct TreasuryRules {
    content::TechnologyProfile profile=content::TechnologyProfile::bnw;
    std::map<std::int64_t,TreasuryBelief> beliefs;
    std::map<std::int64_t,MinorYieldRules> minors;
    std::set<std::int64_t> luxury_resources;
};
TreasuryRules load_treasury_rules(const content::RulesDatabase &database,
    content::TechnologyProfile profile,const content::InfoTypeCatalog &types);
struct TreasuryReligion {
    std::int64_t religion=-1, founder=-1, belief_religion=-1;
    bool pantheon=false;
    std::optional<std::int64_t> holy_plot_owner;
    std::optional<std::pair<std::int64_t,std::int64_t>> holy_city;
    std::vector<std::int64_t> beliefs;
    std::set<std::int64_t> belief_lookup;
};
struct TreasuryPlayer {
    std::optional<std::pair<std::uint64_t,std::uint64_t>> source_core, source_administration;
    std::int64_t balance_times100=0, connections_times100=0;
    std::int64_t connection_modifier=0,connection_change=0;
    std::int64_t lifetime_gross_gold=0;
    std::vector<std::int64_t> balance_history_times100, income_history_times100;
    std::int64_t gold_per_partner=0;
    std::int64_t ally=-1;
    bool anarchy=false;
    ProfileExtensions profile_extensions;
};
struct TreasuryContext {
    content::TechnologyProfile profile=content::TechnologyProfile::bnw;
    std::map<std::int64_t,TreasuryPlayer> players;
    std::vector<TreasuryReligion> religions;
    std::vector<saves::TradeRouteObservation> trade_routes;
    // Source loop domains, not host ID/count ceilings.
    std::set<std::int64_t> minor_players;
};
TreasuryContext restore_treasury(const saves::SavedSessionSnapshot &snapshot,
    const MaintenanceWorld &world,const TreasuryRules &rules,const content::InfoTypeCatalog &types);
struct TreasuryUnavailable : std::runtime_error { using std::runtime_error::runtime_error; };
struct TreasuryServices {
    // BNW Lua accumulator. nullopt means no handler supplied a result. An
    // absent service models a disabled script system, not proven mod parity.
    std::function<std::optional<std::int64_t>(std::int64_t)> founder_benefits_religion;
};
struct MinorYieldQuote {
    std::int64_t friends=0, allies=0, modifier=0, scale=100, total_times100=0;
};
MinorYieldQuote minor_yield_times100(const TreasuryContext &state,const MaintenanceWorld &world,
    const MaintenanceContext &maintenance,const TreasuryRules &rules,const content::GlobalDefineCache &defines,
    const content::CustomModOptions &options,std::int64_t minor,std::int64_t player,std::int64_t yield);
struct TreasuryIncomeQuote {
    std::int64_t cities=0, diplomacy=0, connections=0, religion=0, traits=0;
    std::int64_t total_times100=0;
    ProfileExtensions profile_extensions;
};
TreasuryIncomeQuote gross_gold_times100(const TreasuryContext &state,const MaintenanceWorld &world,
    const MaintenanceContext &maintenance,const TreasuryRules &rules,const CityYieldRules &city_rules,
    const content::GlobalDefineCache &defines,const content::CustomModOptions *options,
    std::int64_t player,const TreasuryServices &services={});
struct TreasuryNetQuote {
    bool anarchy=false;
    TreasuryIncomeQuote income;
    std::optional<MaintenanceQuote> expenses;
    std::int64_t net_times100=0;
};
// Query only: updates expense caches through maintenance_costs, but never
// spends/earns gold, advances history or executes a turn. VP anarchy is lazy.
TreasuryNetQuote net_gold_times100(const TreasuryContext &state,const MaintenanceWorld &world,
    MaintenanceContext &maintenance,const TreasuryRules &rules,const CityYieldRules &city_rules,
    const MaintenanceRules &maintenance_rules,const content::GlobalDefineCache &defines,
    const content::CustomModOptions *options,std::int64_t player,const TreasuryServices &services={});
} // namespace civ5::runtime
