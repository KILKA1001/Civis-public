#pragma once

#include "civ5/runtime/bnw_fresh_archive_full.hpp"
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace civ5::runtime {

struct BnwFreshPoliciesServiceState {
    // Nullable SDK array/object owners. Null and allocated empty are distinct.
    std::optional<std::vector<std::int32_t>> latest_flavors;
    std::optional<std::vector<bool>> has_policy;
    std::optional<std::vector<bool>> one_shot_policy_fired;
    std::optional<std::vector<bool>> one_shot_free_units_fired;
    std::optional<std::vector<bool>> branch_unlocked;
    std::optional<std::vector<bool>> branch_blocked;
    std::optional<std::vector<bool>> branch_finished;
    std::optional<std::vector<std::int32_t>> branches_chosen;
    std::optional<std::vector<std::int32_t>> policy_ai_weights;
    std::optional<std::vector<std::int32_t>> blocked_branch_check;
    // CvPlayerPolicies::Uninit does not null these borrowed pointers.
    bool policies_info_bound = false;
    bool player_bound = false;
    bool operator==(const BnwFreshPoliciesServiceState &) const = default;
};
struct BnwFreshPoliciesUninitDraft {
    BnwFreshPoliciesServiceState state;
    std::vector<std::string_view> source_order;
    std::string selected_rules_sha256;
    bool member_effect_complete = false;
    bool archive_predecessor_verified = false;
    bool uninit_chain_complete = false;
    bool initialized_by_gamecore = false;
    void rollback() noexcept;
};

// SDK CvPlayer::uninit calls CvPlayerPolicies::Uninit first among services.
// CvFlavorRecipient::Uninit deletes its flavor array; eight policy arrays
// and CvPolicyAI follow in their source order. CvPolicyAI destructor is empty.
// This applies all owned deletion effects to a detached snapshot, including
// allocated and constructor-null cases. The earlier aggregate archive.reset
// and following EconomicAI/other services remain mandatory blockers.
BnwFreshPoliciesServiceState make_bnw_fresh_policies_constructor_state();
BnwFreshPoliciesUninitDraft project_bnw_fresh_policies_uninit(
    const BnwFreshFirstPlayerObjects &objects,
    const BnwFreshFullArchiveProjection &archive,
    std::string_view selected_rules_sha256,
    BnwFreshPoliciesServiceState initial);

} // namespace civ5::runtime
