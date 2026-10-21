#pragma once
#include "civ5/runtime/improvement_yield_calculation.hpp"

namespace civ5::runtime {
// These saved capital-connection vectors exist in VP's player planning state.
// Keep their source order: VP searches them without sorting or deduplication.
struct VpImprovementYieldProfileState {
    struct Player {
        std::vector<std::int64_t> connections,industrial_connections;
    };
    std::map<std::int64_t,Player> players;
};
inline VpImprovementYieldProfileState &ensure_vp_improvement_yield_profile(ImprovementYieldState &state){
    if(state.profile!=content::TechnologyProfile::vp542)
        throw std::runtime_error("VP improvement connections require VP GameCore profile");
    if(auto *profile=state.profile_extensions.find<VpImprovementYieldProfileState>())return *profile;
    return state.profile_extensions.emplace<VpImprovementYieldProfileState>();
}
inline const VpImprovementYieldProfileState::Player &vp_improvement_yield_player(const ImprovementYieldState &state,std::int64_t player){
    if(state.profile!=content::TechnologyProfile::vp542)
        throw std::runtime_error("VP improvement connections require VP GameCore profile");
    const auto *profile=state.profile_extensions.find<VpImprovementYieldProfileState>();
    if(!profile)throw std::runtime_error("Missing VP improvement connection profile");
    const auto found=profile->players.find(player);
    if(found==profile->players.end())throw std::runtime_error("Missing VP improvement player connections");
    return found->second;
}
} // namespace civ5::runtime
