#pragma once
#include "civ5/runtime/research_context.hpp"

namespace civ5::runtime {
// VP's blocked PolicyBranchTypes are GameCore state. Shared policy ownership
// stays in ResearchPlayerState; BNW and other profiles need no branch ledger.
struct VpPolicyBranchProfileState {
    std::map<std::int64_t, std::set<std::int64_t>> players;
};

inline VpPolicyBranchProfileState &ensure_vp_policy_branch_profile(ResearchContext &world) {
    if (world.profile != content::TechnologyProfile::vp542)
        throw std::runtime_error("VP policy branches require VP GameCore profile");
    if (auto *state = world.extensions.find<VpPolicyBranchProfileState>()) return *state;
    return world.extensions.emplace<VpPolicyBranchProfileState>();
}

inline const VpPolicyBranchProfileState *find_vp_policy_branch_profile(const ResearchContext &world) {
    return world.profile == content::TechnologyProfile::vp542
        ? world.extensions.find<VpPolicyBranchProfileState>() : nullptr;
}

inline const std::set<std::int64_t> &vp_blocked_policy_branches(const ResearchContext &world,
                                                                 std::int64_t player) {
    if (!world.players.contains(player)) throw std::runtime_error("Unresolved VP policy branch player");
    const auto *state = find_vp_policy_branch_profile(world);
    if (!state || !state->players.contains(player))
        throw std::runtime_error("Unresolved VP player policy branches");
    return state->players.at(player);
}
} // namespace civ5::runtime
