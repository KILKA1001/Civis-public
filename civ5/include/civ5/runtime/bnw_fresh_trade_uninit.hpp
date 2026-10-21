#pragma once

#include "civ5/runtime/bnw_fresh_espionage_ai_uninit.hpp"
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace civ5::runtime {

struct BnwFreshTradeServiceState {
    bool player_bound = false;
    // Reset never reads connection fields. Each owned opaque payload stands
    // for one TradeConnection or TradeConnectionWasPlundered element; clear
    // destroys the entire bounded FStaticVector, including its nested path.
    std::vector<std::vector<std::byte>> recently_expired_connections;
    std::vector<std::vector<std::byte>> plundered_connections;
    bool operator==(const BnwFreshTradeServiceState &) const = default;
};
struct BnwFreshTradeUninitDraft {
    BnwFreshTradeServiceState state;
    std::vector<std::string_view> source_order;
    std::string selected_rules_sha256;
    bool member_effect_complete = false;
    bool archive_predecessor_verified = false;
    bool remaining_services_complete = false;
    bool initialized_by_gamecore = false;
    void rollback() noexcept;
};

// CvTradeClasses.cpp:1695-1698: constructor calls Reset, producing this state.
BnwFreshTradeServiceState make_bnw_fresh_trade_constructor_state();

// CvPlayer.cpp:678 calls after EspionageAI. Expansion2 CvTradeClasses.cpp:
// 1717-1728: Uninit invokes Reset, which nulls player and clears exactly two
// owned FStaticVectors. Archive traversal and TradeAI/later services remain
// blocked.
BnwFreshTradeUninitDraft project_bnw_fresh_trade_uninit(
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
    const BnwFreshReligionAiUninitDraft &religion_ai,
    const BnwFreshEspionageUninitDraft &espionage,
    const BnwFreshEspionageAiUninitDraft &espionage_ai,
    std::string_view selected_rules_sha256,
    BnwFreshTradeServiceState initial);

} // namespace civ5::runtime
