#pragma once
#include "civ5/runtime/research_ai_weights.hpp"

namespace civ5::runtime {
// VP serializes CvWeightedVector<int> as [element, weight] pairs. Element IDs
// belong to the VP GameCore module; common GetWeight still uses row position.
struct VpResearchAiVector {
    std::map<std::uint64_t, std::int64_t> saved_elements;
    bool permanent_yields_decrease_every_era=false;
};
void restore_vp_research_ai_weights(ResearchAiPlayerWeights &result,
    const saves::PlayerKnowledgeBlock &player_techs,
    const saves::PlayerTraitsObservation &traits);
} // namespace civ5::runtime
