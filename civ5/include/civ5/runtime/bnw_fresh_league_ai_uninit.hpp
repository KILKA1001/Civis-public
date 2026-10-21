#pragma once

#include "civ5/runtime/bnw_fresh_trade_ai_uninit.hpp"
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace civ5::runtime {

struct BnwFreshLeagueVoteCommitment {
    std::int32_t to_player = -1, resolution_id = -1;
    std::int32_t vote_choice = -1, num_votes = 0;
    bool enact = false;
    bool operator==(const BnwFreshLeagueVoteCommitment &) const = default;
};
struct BnwFreshLeagueAiServiceState {
    bool player_bound = false;
    // SDK VoteCommitmentList is an FStaticVector with capacity four.
    std::vector<BnwFreshLeagueVoteCommitment> vote_commitments;
    bool operator==(const BnwFreshLeagueAiServiceState &) const = default;
};
struct BnwFreshLeagueAiUninitDraft {
    BnwFreshLeagueAiServiceState state;
    std::vector<std::string_view> source_order;
    std::string selected_rules_sha256;
    bool member_effect_complete = false;
    bool archive_predecessor_verified = false;
    bool remaining_services_complete = false;
    bool initialized_by_gamecore = false;
    void rollback() noexcept;
};

BnwFreshLeagueAiServiceState make_bnw_fresh_league_ai_constructor_state();

// CvPlayer.cpp:680 calls after TradeAI. Expansion2 CvVotingClasses.cpp:
// 7384-7387 Uninit is empty; it does NOT call Reset or clear commitments.
// Archive traversal and PlayerTechs/later services stay blocked.
BnwFreshLeagueAiUninitDraft project_bnw_fresh_league_ai_uninit(
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
    const BnwFreshTradeAiUninitDraft &trade_ai,
    std::string_view selected_rules_sha256,
    BnwFreshLeagueAiServiceState initial);

} // namespace civ5::runtime
