#pragma once
#include "civ5/runtime/research.hpp"
#include <stdexcept>

namespace civ5::runtime {
// VP's team cost inputs are supplied by its GameCore module. Other profiles
// neither allocate nor interpret this state.
struct VpResearchCostProfile {
    std::int64_t team_handicap_per_era_modifier=0;
    std::int64_t game_ai_handicap_percent=100,game_ai_per_era_modifier=0;
    std::int64_t current_era=0;
    bool human_for_handicap=true,eurekas_enabled=false;
};
inline VpResearchCostProfile &ensure_vp_research_cost_profile(TeamResearchFactors &factors){
    if(auto *state=factors.profile_extensions.find<VpResearchCostProfile>())return *state;
    return factors.profile_extensions.emplace<VpResearchCostProfile>();
}
inline const VpResearchCostProfile &vp_research_cost_profile(const TeamResearchFactors &factors){
    const auto *state=factors.profile_extensions.find<VpResearchCostProfile>();
    if(!state)throw std::invalid_argument("Missing VP research cost profile");
    return *state;
}
} // namespace civ5::runtime
