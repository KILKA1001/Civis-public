#pragma once
#include "civ5/runtime/research_turn.hpp"

namespace civ5::runtime {
struct ResearchSelectionPlayer {
    std::int64_t advanced_start_points=-1;
    std::optional<std::string> civilization;
};
struct ResearchSelectionState {
    content::TechnologyProfile profile=content::TechnologyProfile::bnw;
    std::map<std::int64_t,ResearchSelectionPlayer> players;
    std::map<std::string,std::set<std::int64_t>,std::less<>> disabled_technologies;
};
ResearchSelectionState restore_research_selection(const saves::SavedSessionSnapshot &snapshot,
    const ResearchContext &setup,const content::RulesDatabase &database);
class ResearchSelectionServices {
  public:
    virtual ~ResearchSelectionServices()=default;
    // nullopt means no listener supplied a result. An unavailable script host
    // must throw, not silently permit mod-restricted research.
    virtual std::optional<bool> test_all(std::string_view event,std::int64_t player,std::int64_t tech)=0;
    // Source order: research buttons, game data, score. Runs after mutation.
    virtual void research_queue_dirty(std::int64_t team)=0;
};
struct ResearchSelectionContext {
    ResearchSelectionState &state;
    ResearchTurnState &turn;
    PlayerResearchContext &players;
    const ResearchContext &setup;
    const content::TechnologyRules &technologies;
    const content::CustomModOptions *options;
    std::span<const TeamTechnologies> teams;
    const std::map<std::int64_t,std::int64_t> &team_eras;
    ResearchPrerequisiteLimits limits;
    ResearchSelectionServices &services;
    std::int64_t active_team=-1;
    bool no_science=false;
};
struct ResearchSelectionUnavailable : std::runtime_error { using std::runtime_error::runtime_error; };
bool can_ever_research(ResearchSelectionContext &context,std::int64_t player,std::int64_t tech);
bool can_research(ResearchSelectionContext &context,std::int64_t player,std::int64_t tech,bool trading=false);
bool can_research_for_free(ResearchSelectionContext &context,std::int64_t player,std::int64_t tech);
// The common dispatcher validates the selected GameCore and uses that
// profile's path algorithm. BNW counts shared prerequisites repeatedly and
// treats queued flags as known. BNW and queue walks use heap frames.
std::int64_t research_path_length(ResearchSelectionContext &context,std::int64_t player,std::int64_t tech);
std::int64_t research_queue_position(const PlayerResearchState &player,std::int64_t tech);
bool push_research(ResearchSelectionContext &context,std::int64_t player,std::int64_t tech,bool clear=false);
void pop_research(ResearchSelectionContext &context,std::int64_t player,std::int64_t tech);
void clear_research_queue(ResearchSelectionContext &context,std::int64_t player);
// Failures retain prior mutations, matching source order. Callers must restore
// their checkpoint before retrying. These operations neither choose an AI
// preference nor grant a technology or advance the game clock.
} // namespace civ5::runtime
