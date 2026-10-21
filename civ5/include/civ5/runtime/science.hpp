#pragma once
#include "civ5/runtime/treasury.hpp"

namespace civ5::runtime {
struct SciencePlayer {
    std::int64_t happiness_to_science = 0, median_tech_percent = 0;
    bool minor_science_allies = false;
    std::set<std::int64_t> science_from_players;
    ProfileExtensions profile_extensions;
};
struct ScienceContext {
    content::TechnologyProfile profile = content::TechnologyProfile::bnw;
    bool no_science = false;
    std::map<std::int64_t,SciencePlayer> players;
    std::map<std::int64_t,std::set<std::int64_t>> research_agreements;
    // Selected source loop membership; these are not host ID capacities.
    std::set<std::int64_t> minor_players, barbarian_players;
};
ScienceContext restore_science(const saves::SavedSessionSnapshot &snapshot, ResearchContext &setup,
    const MaintenanceWorld &world, const content::RulesDatabase &database);
struct ScienceUnavailable : std::runtime_error { using std::runtime_error::runtime_error; };
struct ScienceQuote {
    bool anarchy = false, research_agreements_excluded = false;
    std::int64_t cities = 0, religion = 0, traits = 0, other_players = 0, happiness = 0;
    std::int64_t agreements = 0, budget_deficit = 0;
    std::int64_t total_times100 = 0;
    std::optional<std::int64_t> gold_rate_times100;
    ProfileExtensions profile_extensions;
};
// Full GetScienceTimes100 query, including nested minor/vassal queries in
// source order. Uses a heap work stack, diagnoses cycles, and preserves the
// treasury expense-cache side effects. No research/turn mutation or Lua VM.
// VP's exclude flag affects only this player's RA; nested queries use false.
ScienceQuote science_times100(const ScienceContext &state, const MaintenanceWorld &world,
    const TreasuryContext &treasury, MaintenanceContext &maintenance,
    const TreasuryRules &treasury_rules, const CityYieldRules &city_rules,
    const MaintenanceRules &maintenance_rules, const content::GlobalDefineCache &defines,
    const content::CustomModOptions *options, std::int64_t player,
    bool exclude_research_agreements = false, const TreasuryServices &services = {});
} // namespace civ5::runtime
