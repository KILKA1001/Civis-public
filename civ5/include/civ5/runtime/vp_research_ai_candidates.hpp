#pragma once
#include "civ5/runtime/research_ai_candidates.hpp"
#include "civ5/runtime/vp_research_ai_weights.hpp"

namespace civ5::runtime {
ResearchAiCandidates vp_build_research_ai_candidates(ResearchSelectionContext &selection,
    ResearchTurnContext &turn,const ResearchAiWeights &weights,std::int64_t player,bool free_technology);
// VP's free-tech preference, future-era trait and 1..10000 clamp belong to
// its GameCore module, never to a mandatory common candidate field.
std::int64_t vp_reweight_research_ai_candidate(std::int64_t saved_weight,
    std::int64_t turns,float base,float per_turn,bool prefers_expensive,
    bool free_technology,bool permanent_yields_decrease_every_era,
    std::int64_t technology_era,std::int64_t player_era);
} // namespace civ5::runtime
