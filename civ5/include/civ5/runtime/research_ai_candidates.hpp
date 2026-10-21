#pragma once
#include "civ5/runtime/research_ai_weights.hpp"
#include "civ5/runtime/research_selection.hpp"
#include "civ5/runtime/simulation_random.hpp"
#include "civ5/runtime/lua_script_host.hpp"

namespace civ5::runtime {
struct ResearchAiCandidate {
    std::int64_t technology=-1, saved_weight=0, adjusted_weight=0, turns=0;
};
struct ResearchAiCandidates {
    bool prefers_expensive=false;
    std::vector<ResearchAiCandidate> choices; // source technology-slot order
};
struct ResearchAiUnavailable : std::runtime_error { using std::runtime_error::runtime_error; };
// Shared selection, science and source-float inputs used by GameCore scorers.
// Eligibility is supplied by the selected profile; no profile-specific state
// is stored in these common values.
void validate_research_ai_world(const ResearchSelectionContext &selection,
    const ResearchTurnContext &turn,const ResearchAiWeights &weights,std::int64_t player);
ResearchAiCandidates collect_research_ai_candidates(ResearchSelectionContext &selection,
    ResearchTurnContext &turn,const ResearchAiWeights &weights,std::int64_t player,
    bool free_technology,bool eligible);
struct ResearchAiScoringInputs {
    float base=0,per_turn=0;
    bool prefers_expensive=false;
};
ResearchAiScoringInputs research_ai_scoring_inputs(ResearchSelectionContext &selection,
    ResearchTurnContext &turn,std::int64_t player,bool force_expensive);
// Build the source TechAI candidate vector. CanResearch invokes the selected
// Lua handlers. This does not sort, roll RNG, change the queue or grant a tech.
ResearchAiCandidates build_research_ai_candidates(ResearchSelectionContext &selection,
    ResearchTurnContext &turn,const ResearchAiWeights &weights,std::int64_t player,bool free_technology=false);
// Source AI_chooseResearch: clear the common queue, adopt a living teammate's
// project, then use the selected GameCore's scripted override and weighted
// choice. The saved pregame mapRandomSeed comes from the shared ResearchContext.
// The BNW gameplay stream advances only if BNW actually rolls a candidate.
struct ResearchAiChoiceContext {
    ResearchSelectionContext &selection;
    ResearchTurnContext &turn;
    const ResearchAiWeights &weights;
    const content::RulesDatabase &rules;
    LuaScriptHost &scripts;
    SimulationRandom &gameplay_random;
};
struct ResearchAiChoiceResult {
    std::int64_t technology=-1;
    bool teammate=false, script_override=false, weighted=false;
};
ResearchAiChoiceResult choose_research_ai(ResearchAiChoiceContext &context,std::int64_t player);
// Run source-order research accrual with this same world's real AI chooser.
// UI and world-effect callbacks remain supplied by ResearchTurnServices.
// As with do_research, a failed callback may leave partial turn mutations.
ResearchTurnResult do_research_with_ai(ResearchAiChoiceContext &context,std::int64_t player);
// Shared active-handicap lookup used by each GameCore's choice policy.
std::int64_t research_ai_handicap_setting(const ResearchAiChoiceContext &context,std::string_view column);
} // namespace civ5::runtime
