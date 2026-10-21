#pragma once

#include "civ5/runtime/bnw_fresh_policies_uninit.hpp"
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace civ5::runtime {

struct BnwFreshEconomicServiceState {
    // These are the only three owned pointers touched by
    // CvEconomicAI::Uninit. Null differs from an allocated empty array.
    std::optional<std::vector<bool>> using_strategy;
    std::optional<std::vector<std::int32_t>> turn_strategy_adopted;
    std::optional<std::vector<std::optional<std::int32_t>>> temp_flavors;
    // Representative retained owned members: Uninit leaves them untouched.
    std::int32_t early_city_number_target = 1;
    std::vector<std::uint32_t> cached_yields;
    std::vector<std::int32_t> exploration_plots;
    bool operator==(const BnwFreshEconomicServiceState &) const = default;
};
struct BnwFreshEconomicUninitDraft {
    BnwFreshEconomicServiceState state;
    std::vector<std::string_view> source_order;
    std::string selected_rules_sha256;
    bool member_effect_complete = false;
    bool archive_predecessor_verified = false;
    bool remaining_services_complete = false;
    bool initialized_by_gamecore = false;
    void rollback() noexcept;
};

// First fresh CvEconomicAI constructor initializes the three array pointers
// null and early-city target to 1. Other scalars/pointers without constructor
// initialization are deliberately absent from this projection.
BnwFreshEconomicServiceState make_bnw_fresh_economic_constructor_state();

// Expansion2 CvPlayer::uninit calls this immediately after Policies::Uninit.
// SDK CvEconomicAI.cpp:284-289 contains exactly three SAFE_DELETE_ARRAY calls;
// no AI reset, selected rules query, or retained cache clear occurs here.
BnwFreshEconomicUninitDraft project_bnw_fresh_economic_uninit(
    const BnwFreshFirstPlayerObjects &objects,
    const BnwFreshFullArchiveProjection &archive,
    const BnwFreshPoliciesUninitDraft &policies,
    std::string_view selected_rules_sha256,
    BnwFreshEconomicServiceState initial);

} // namespace civ5::runtime
