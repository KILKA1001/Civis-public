#pragma once

#include "civ5/runtime/bnw_fresh_religions_uninit.hpp"
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace civ5::runtime {

struct BnwFreshReligionAiServiceState {
    // SDK constructor sets m_pPlayer=NULL but leaves m_pBeliefs undefined.
    bool player_bound = false;
    std::optional<bool> beliefs_bound;
    bool operator==(const BnwFreshReligionAiServiceState &) const = default;
};

struct BnwFreshReligionAiUninitDraft {
    BnwFreshReligionAiServiceState state;
    std::vector<std::string_view> source_order;
    std::string selected_rules_sha256;
    bool member_effect_complete = false;
    bool archive_predecessor_verified = false;
    bool remaining_services_complete = false;
    bool initialized_by_gamecore = false;
    void rollback() noexcept;
};

BnwFreshReligionAiServiceState make_bnw_fresh_religion_ai_constructor_state();

// CvPlayer.cpp:675 calls after CvPlayerReligions. Expansion2
// CvReligionClasses.cpp:4162-4165 has an empty Uninit body. Neither borrowed
// pointer is altered; archive traversal and Espionage/remaining services stay
// blocked.
BnwFreshReligionAiUninitDraft project_bnw_fresh_religion_ai_uninit(
    const BnwFreshFirstPlayerObjects &objects,
    const BnwFreshFullArchiveProjection &archive,
    const BnwFreshPoliciesUninitDraft &policies,
    const BnwFreshEconomicUninitDraft &economic,
    const BnwFreshMilitaryUninitDraft &military,
    const BnwFreshCitySpecializationUninitDraft &city_specialization,
    const BnwFreshWonderUninitDraft &wonder,
    const BnwFreshGrandStrategyUninitDraft &grand_strategy,
    const BnwFreshDiplomacyUninitDraft &diplomacy,
    const BnwFreshReligionsUninitDraft &religions,
    std::string_view selected_rules_sha256,
    BnwFreshReligionAiServiceState initial);

} // namespace civ5::runtime
