#pragma once

#include "civ5/runtime/bnw_fresh_diplomacy_uninit.hpp"
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace civ5::runtime {

struct BnwFreshReligionsServiceState {
    bool player_bound = false;
    std::int32_t prophets_spawned = 0;
    // Expansion2 declares m_bFoundingReligion as int, initialized with false.
    std::int32_t founding_religion = 0;
    bool operator==(const BnwFreshReligionsServiceState &) const = default;
};

struct BnwFreshReligionsUninitDraft {
    BnwFreshReligionsServiceState state;
    std::vector<std::string_view> source_order;
    std::string selected_rules_sha256;
    bool member_effect_complete = false;
    bool archive_predecessor_verified = false;
    bool remaining_services_complete = false;
    bool initialized_by_gamecore = false;
    void rollback() noexcept;
};

// CvReligionClasses.cpp:2203-2209: null player and zero scalar fields.
BnwFreshReligionsServiceState make_bnw_fresh_religions_constructor_state();

// CvPlayer.cpp:674 calls this after DiplomacyAI. The SDK Uninit body is
// empty (CvReligionClasses.cpp:2225-2228), so even populated fields remain.
// Archive traversal and ReligionAI/remaining services stay blocked.
BnwFreshReligionsUninitDraft project_bnw_fresh_religions_uninit(
    const BnwFreshFirstPlayerObjects &objects,
    const BnwFreshFullArchiveProjection &archive,
    const BnwFreshPoliciesUninitDraft &policies,
    const BnwFreshEconomicUninitDraft &economic,
    const BnwFreshMilitaryUninitDraft &military,
    const BnwFreshCitySpecializationUninitDraft &city_specialization,
    const BnwFreshWonderUninitDraft &wonder,
    const BnwFreshGrandStrategyUninitDraft &grand_strategy,
    const BnwFreshDiplomacyUninitDraft &diplomacy,
    std::string_view selected_rules_sha256,
    BnwFreshReligionsServiceState initial);

} // namespace civ5::runtime
