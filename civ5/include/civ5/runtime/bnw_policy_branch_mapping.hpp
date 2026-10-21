#pragma once
#include <cstdint>

namespace civ5::runtime::detail {
// Pure CvPlayerPolicies::Reset lookup. The ordered opener map contains only
// actual non-NULL branch Info rows. It issues no policy/branch owner authority.
template<class OrderedOpeners>
std::int64_t bnw_policy_blocked_branch(std::int64_t policy_slot,
                                     std::int64_t explicit_branch,
                                     const OrderedOpeners& free_openers) {
    if(explicit_branch!=-1) return explicit_branch;
    for(const auto& [candidate,opener]:free_openers)
        if(opener==policy_slot) return candidate;
    return -1;
}
} // namespace civ5::runtime::detail
