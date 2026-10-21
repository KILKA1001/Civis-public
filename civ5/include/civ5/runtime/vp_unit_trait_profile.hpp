#pragma once
#include "civ5/runtime/research_context.hpp"

namespace civ5::runtime {
// CvPlayerTraits rebuilds this ascending leader list on Init/Read. Live
// belief, policy and technology gates are checked separately by HasTrait.
// Arbitrary GameCore profiles need no VP trait cache in the shared roster.
struct VpUnitTraitProfileState {
    std::map<std::int64_t,std::vector<std::int64_t>> potential_by_player;
};
inline VpUnitTraitProfileState &ensure_vp_unit_trait_profile(ResearchContext &world){
    if(world.profile!=content::TechnologyProfile::vp542)
        throw std::runtime_error("VP unit traits require VP GameCore profile");
    if(auto *state=world.extensions.find<VpUnitTraitProfileState>())return *state;
    return world.extensions.emplace<VpUnitTraitProfileState>();
}
inline const VpUnitTraitProfileState *find_vp_unit_trait_profile(const ResearchContext &world){
    return world.profile==content::TechnologyProfile::vp542
        ?world.extensions.find<VpUnitTraitProfileState>():nullptr;
}
inline const std::vector<std::int64_t> &vp_potential_traits(const ResearchContext &world,std::int64_t player){
    const auto *state=find_vp_unit_trait_profile(world);
    if(!state)throw std::runtime_error("Unresolved VP trait cache");
    const auto found=state->potential_by_player.find(player);
    if(found==state->potential_by_player.end())throw std::runtime_error("Unresolved VP player trait cache");
    return found->second;
}
} // namespace civ5::runtime
