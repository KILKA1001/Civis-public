#pragma once
#include "civ5/runtime/research_ai_candidates.hpp"

namespace civ5::runtime {
// VP GameCore-only deterministic weighted choice. This consumes no shared
// gameplay RNG state; its seed is mixed from the saved map seed and turn.
std::int64_t vp_choose_research_ai_candidate(const ResearchAiCandidates &candidates,
    std::int64_t cutoff_percent,std::uint64_t map_seed,std::int64_t game_turn,
    std::int64_t player,std::int64_t team_known_count);
std::optional<std::int64_t> vp_research_ai_script_override(LuaScriptHost &scripts,
    const ResearchSelectionContext &selection,std::int64_t player);
struct VpResearchAiChoiceResult {
    std::int64_t technology=-1;
    bool script_override=false,weighted=false;
};
// The VP module owns its Lua override and seeded weighted fallback. The
// caller has already handled the shared queue and teammate adoption.
VpResearchAiChoiceResult vp_choose_research_ai(ResearchAiChoiceContext &context,std::int64_t player);
} // namespace civ5::runtime
