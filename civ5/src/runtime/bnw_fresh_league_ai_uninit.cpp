#include "civ5/runtime/bnw_fresh_league_ai_uninit.hpp"
#include <utility>

namespace civ5::runtime {

void BnwFreshLeagueAiUninitDraft::rollback() noexcept {
    state = {};
    source_order.clear();
    selected_rules_sha256.clear();
    member_effect_complete = false;
    archive_predecessor_verified = false;
    remaining_services_complete = false;
    initialized_by_gamecore = false;
}

BnwFreshLeagueAiServiceState make_bnw_fresh_league_ai_constructor_state() {
    return {};
}

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
    BnwFreshLeagueAiServiceState initial) {
    BnwFreshLeagueAiUninitDraft result;
    if (!objects.constructors_complete || objects.initialized_by_gamecore ||
        objects.player_id != 0 || selected_rules_sha256.empty() ||
        objects.selected_rules_sha256 != selected_rules_sha256 ||
        archive.selected_rules_sha256 != selected_rules_sha256 ||
        !archive.all_registered_member_defaults ||
        archive.aggregate_traversal_verified || archive.initialized_by_gamecore ||
        policies.selected_rules_sha256 != selected_rules_sha256 ||
        !policies.member_effect_complete || policies.archive_predecessor_verified ||
        policies.initialized_by_gamecore ||
        economic.selected_rules_sha256 != selected_rules_sha256 ||
        !economic.member_effect_complete || economic.archive_predecessor_verified ||
        economic.initialized_by_gamecore ||
        military.selected_rules_sha256 != selected_rules_sha256 ||
        !military.member_effect_complete || military.archive_predecessor_verified ||
        military.initialized_by_gamecore ||
        city_specialization.selected_rules_sha256 != selected_rules_sha256 ||
        !city_specialization.member_effect_complete ||
        city_specialization.archive_predecessor_verified ||
        city_specialization.initialized_by_gamecore ||
        wonder.selected_rules_sha256 != selected_rules_sha256 ||
        !wonder.member_effect_complete || wonder.archive_predecessor_verified ||
        wonder.initialized_by_gamecore ||
        grand_strategy.selected_rules_sha256 != selected_rules_sha256 ||
        !grand_strategy.member_effect_complete ||
        grand_strategy.archive_predecessor_verified ||
        grand_strategy.initialized_by_gamecore ||
        diplomacy.selected_rules_sha256 != selected_rules_sha256 ||
        !diplomacy.member_effect_complete || diplomacy.archive_predecessor_verified ||
        diplomacy.initialized_by_gamecore ||
        religions.selected_rules_sha256 != selected_rules_sha256 ||
        !religions.member_effect_complete || religions.archive_predecessor_verified ||
        religions.initialized_by_gamecore ||
        religion_ai.selected_rules_sha256 != selected_rules_sha256 ||
        !religion_ai.member_effect_complete ||
        religion_ai.archive_predecessor_verified ||
        religion_ai.initialized_by_gamecore ||
        espionage.selected_rules_sha256 != selected_rules_sha256 ||
        !espionage.member_effect_complete || espionage.archive_predecessor_verified ||
        espionage.initialized_by_gamecore ||
        espionage_ai.selected_rules_sha256 != selected_rules_sha256 ||
        !espionage_ai.member_effect_complete ||
        espionage_ai.archive_predecessor_verified ||
        espionage_ai.initialized_by_gamecore ||
        trade.selected_rules_sha256 != selected_rules_sha256 ||
        !trade.member_effect_complete || trade.archive_predecessor_verified ||
        trade.initialized_by_gamecore ||
        trade_ai.selected_rules_sha256 != selected_rules_sha256 ||
        !trade_ai.member_effect_complete || trade_ai.archive_predecessor_verified ||
        trade_ai.initialized_by_gamecore ||
        initial.vote_commitments.size() > 4)
        return result;

    result.selected_rules_sha256 = std::string(selected_rules_sha256);
    result.state = std::move(initial);
    result.source_order.push_back("CvLeagueAI::Uninit");
    result.member_effect_complete = true;
    return result;
}

} // namespace civ5::runtime
