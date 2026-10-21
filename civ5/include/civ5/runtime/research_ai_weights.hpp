#pragma once
#include "civ5/runtime/profile_extensions.hpp"
#include "civ5/runtime/research_context.hpp"
#include "civ5/content/technology_rules.hpp"

namespace civ5::runtime {
// TechAI's GetWeight indexes a vector, even when a VP saved row carries a
// different element ID. Sparse storage avoids allocating through MAX(Tech ID).
struct ResearchAiPlayerWeights {
    std::uint64_t weight_slots = 0;
    std::map<std::uint64_t, std::int64_t> weights;
    std::uint64_t unresolved_rows = 0;
    ProfileExtensions profile_extensions;
    std::optional<std::int64_t> weight_at(std::uint64_t slot) const;
};
struct ResearchAiWeights {
    content::TechnologyProfile profile = content::TechnologyProfile::bnw;
    std::map<std::int64_t, ResearchAiPlayerWeights> players;
};
// Saved weights are input to AI selection, not an AI choice or a completed
// turn. Missing mod hashes remain unresolved instead of being guessed.
ResearchAiWeights restore_research_ai_weights(const saves::SavedSessionSnapshot &snapshot,
    const ResearchContext &setup, const content::TechnologyRules &technologies,
    const content::InfoTypeCatalog &types);
} // namespace civ5::runtime
