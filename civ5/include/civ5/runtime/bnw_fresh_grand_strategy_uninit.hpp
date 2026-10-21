#pragma once

#include "civ5/runtime/bnw_fresh_wonder_uninit.hpp"
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace civ5::runtime {

struct BnwFreshGrandStrategyServiceState {
    // Only these three pointers are initialized by the SDK constructor.
    std::optional<std::vector<std::int32_t>> strategy_priorities;
    std::optional<std::vector<std::int32_t>> other_player_strategy_guesses;
    std::optional<std::vector<std::int32_t>> other_player_guess_confidence;
    // Constructor leaves these fields indeterminate until Init/Reset.
    std::optional<std::int32_t> turns_since_active_set;
    std::optional<std::int32_t> active_strategy;
    // These borrowed pointers are likewise uninitialized until Init.
    std::optional<bool> player_bound;
    std::optional<bool> strategies_info_bound;
    bool operator==(const BnwFreshGrandStrategyServiceState &) const = default;
};

struct BnwFreshGrandStrategyUninitDraft {
    BnwFreshGrandStrategyServiceState state;
    std::vector<std::string_view> source_order;
    std::string selected_rules_sha256;
    bool member_effect_complete = false;
    bool archive_predecessor_verified = false;
    bool remaining_services_complete = false;
    bool initialized_by_gamecore = false;
    void rollback() noexcept;
};

// CvGrandStrategyAI.cpp:124-129 initializes only three owned pointers to NULL.
BnwFreshGrandStrategyServiceState make_bnw_fresh_grand_strategy_constructor_state();

// CvPlayer.cpp:672 dispatches after WonderProductionAI. Uninit releases the
// three arrays in SDK order. It does not Reset scalar state or borrowed links.
// Archive traversal and DiplomacyAI/remaining services stay blocked.
BnwFreshGrandStrategyUninitDraft project_bnw_fresh_grand_strategy_uninit(
    const BnwFreshFirstPlayerObjects &objects,
    const BnwFreshFullArchiveProjection &archive,
    const BnwFreshPoliciesUninitDraft &policies,
    const BnwFreshEconomicUninitDraft &economic,
    const BnwFreshMilitaryUninitDraft &military,
    const BnwFreshCitySpecializationUninitDraft &city_specialization,
    const BnwFreshWonderUninitDraft &wonder,
    std::string_view selected_rules_sha256,
    BnwFreshGrandStrategyServiceState initial);

} // namespace civ5::runtime
