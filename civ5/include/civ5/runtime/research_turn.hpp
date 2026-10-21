#pragma once
#include "civ5/runtime/research_progress.hpp"
#include "civ5/runtime/science.hpp"

namespace civ5::runtime {
struct ResearchTurnPlayer {
    bool founded_first_city=false;
    std::int64_t cities_founded=0;
    YieldValues agreement_counters;
    std::set<std::int64_t> researching;
    std::optional<std::string> leader_type;
    std::uint64_t unmapped_researching=0;
};
struct ResearchTurnState {
    content::TechnologyProfile profile=content::TechnologyProfile::bnw;
    std::map<std::int64_t,ResearchTurnPlayer> players;
};
ResearchTurnState restore_research_turn(const saves::SavedSessionSnapshot &snapshot,const ResearchContext &setup,
    const content::TechnologyRules &technologies,const content::InfoTypeCatalog &types,const content::RulesDatabase &database);
enum class ResearchAchievement { catherine_horseback, all_technologies };
// AI choice must update the common queue/researching flags. Technology
// acquisition is still the mandatory full-world service of ResearchProgress.
class ResearchTurnServices {
  public:
    virtual ~ResearchTurnServices() = default;
    virtual void choose_research_ai(std::int64_t player) = 0;
    virtual void choose_research_ui(std::int64_t player) = 0;
    virtual void research_buttons_dirty(std::int64_t team) = 0;
    virtual void unlock_research_achievement(std::int64_t player,ResearchAchievement achievement) = 0;
};
struct ResearchTurnContext {
    ResearchTurnState &state;
    ResearchProgressContext &progress;
    ScienceContext &science;
    const MaintenanceWorld &economy;
    const TreasuryContext &treasury;
    MaintenanceContext &maintenance;
    const TreasuryRules &treasury_rules;
    const CityYieldRules &city_rules;
    const MaintenanceRules &maintenance_rules;
    ResearchTurnServices &services;
    std::int64_t active_player=-1;
    TreasuryServices income_services;
};
struct ResearchTurnResult {
    bool disabled=false, research_enabled=false, forced_choice=false;
    std::int64_t technology=-1, overflow_spent=0, overflow_added=0, science_queries=0;
    std::optional<std::int64_t> beakers_times100, agreement_science;
    std::optional<ResearchProgressChange> progress_change;
};
struct ResearchTurnsLeftQuote {
    std::int64_t research_rate_times100=0, overflow_adjustment=0;
    std::int64_t player_cost=0, team_progress=0, remaining=0;
    // No rate means no finite estimate. The x86 source returned INT_MAX;
    // the host does not inherit that 32-bit sentinel or a 32-bit turn cap.
    std::optional<std::int64_t> turns_times100, turns;
};
struct ResearchTurnUnavailable : std::runtime_error { using std::runtime_error::runtime_error; };
// Source GetResearchTurnsLeft(eTech,bOverflow) with actual science from the
// shared world. In BNW cost/progress/overflow are whole beakers; VP uses
// hundredths. Queries may update expense and BNW max-city caches.
ResearchTurnsLeftQuote research_turns_left(ResearchTurnContext &context,
    std::int64_t player,std::int64_t technology,bool include_overflow);
void update_research_agreements(ResearchTurnContext &context,std::int64_t player,std::int64_t value);
void clear_research_queue(ResearchTurnContext &context,std::int64_t player);
void check_research_achievements(ResearchTurnContext &context,std::int64_t player);
// Uses real science queries (including those made by low-level progress),
// never an injected beaker amount. All state is shared with other subsystems.
// Source-order partial mutations remain on callback failure; do not retry
// without restoring the caller's checkpoint. Does not advance the game clock.
ResearchTurnResult do_research(ResearchTurnContext &context,std::int64_t player);
} // namespace civ5::runtime
