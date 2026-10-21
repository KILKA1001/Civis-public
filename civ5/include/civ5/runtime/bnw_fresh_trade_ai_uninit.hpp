#pragma once

#include "civ5/runtime/bnw_fresh_trade_uninit.hpp"
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace civ5::runtime {

struct BnwFreshTradeAiServiceState {
    bool player_bound = false;
    // CvTradeAI constructor leaves this scalar undefined; Init sets zero.
    // Uninit/Reset does not touch it.
    std::optional<std::int32_t> removable_value;
    bool operator==(const BnwFreshTradeAiServiceState &) const = default;
};
struct BnwFreshTradeAiUninitDraft {
    BnwFreshTradeAiServiceState state;
    std::vector<std::string_view> source_order;
    std::string selected_rules_sha256;
    bool member_effect_complete = false;
    bool archive_predecessor_verified = false;
    bool remaining_services_complete = false;
    bool initialized_by_gamecore = false;
    void rollback() noexcept;
};

BnwFreshTradeAiServiceState make_bnw_fresh_trade_ai_constructor_state();

// CvPlayer.cpp:679 calls after CvPlayerTrade. Expansion2
// CvTradeClasses.cpp:3885-3894: Uninit invokes Reset, which only sets
// m_pPlayer=NULL. Archive traversal and LeagueAI/later services stay blocked.
BnwFreshTradeAiUninitDraft project_bnw_fresh_trade_ai_uninit(
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
    const BnwFreshTradeUninitDraft &trade,
    std::string_view selected_rules_sha256,
    BnwFreshTradeAiServiceState initial);

} // namespace civ5::runtime
