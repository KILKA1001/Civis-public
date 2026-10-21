#pragma once

#include "civ5/runtime/bnw_fresh_economic_uninit.hpp"
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace civ5::runtime {

struct BnwFreshMilitaryServiceState {
    // Seven nullable arrays owned by CvMilitaryAI::Uninit. Int arrays may
    // contain uninitialized cells after Init, represented as typed absence.
    std::optional<std::vector<bool>> using_strategy;
    std::optional<std::vector<std::int32_t>> turn_strategy_adopted;
    std::optional<std::vector<std::optional<std::int32_t>>> temp_flavors;
    std::optional<std::vector<std::optional<std::int32_t>>> last_war_state;
    std::optional<std::vector<std::optional<std::int32_t>>> last_military_threat;
    std::optional<std::vector<std::optional<std::int32_t>>> last_military_strength;
    std::optional<std::vector<std::optional<std::int32_t>>> last_target_value;
    // Constructor-defined values; Uninit leaves these scalars untouched.
    std::int32_t total_threat_weight = 1;
    std::int32_t operations_build_skipped = 0;
    std::int32_t army_type_being_built = -1;
    bool operator==(const BnwFreshMilitaryServiceState &) const = default;
};
struct BnwFreshMilitaryUninitDraft {
    BnwFreshMilitaryServiceState state;
    std::vector<std::string_view> source_order;
    std::string selected_rules_sha256;
    bool member_effect_complete = false;
    bool archive_predecessor_verified = false;
    bool remaining_services_complete = false;
    bool initialized_by_gamecore = false;
    void rollback() noexcept;
};

// Expansion2 CvMilitaryAI.cpp:252-265 constructor initializes seven array
// pointers NULL, total threat 1, skipped operation count 0, army type none.
// Other uninitialized members are deliberately not given host defaults.
BnwFreshMilitaryServiceState make_bnw_fresh_military_constructor_state();

// CvPlayer::uninit calls CvMilitaryAI::Uninit after EconomicAI. Source
// CvMilitaryAI.cpp:306-315 deletes exactly seven arrays. The aggregate
// archive predecessor and the later CitySpecializationAI callbacks remain
// blockers even when this detached member effect is complete.
BnwFreshMilitaryUninitDraft project_bnw_fresh_military_uninit(
    const BnwFreshFirstPlayerObjects &objects,
    const BnwFreshFullArchiveProjection &archive,
    const BnwFreshPoliciesUninitDraft &policies,
    const BnwFreshEconomicUninitDraft &economic,
    std::string_view selected_rules_sha256,
    BnwFreshMilitaryServiceState initial);

} // namespace civ5::runtime
