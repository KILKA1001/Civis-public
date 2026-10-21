#include "civ5/runtime/bnw_fresh_trade_uninit.hpp"
#include <utility>

namespace civ5::runtime {

void BnwFreshTradeUninitDraft::rollback() noexcept {
    state = {};
    source_order.clear();
    selected_rules_sha256.clear();
    member_effect_complete = false;
    archive_predecessor_verified = false;
    remaining_services_complete = false;
    initialized_by_gamecore = false;
}

BnwFreshTradeServiceState make_bnw_fresh_trade_constructor_state() {
    return {};
}

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
    BnwFreshTradeServiceState initial) {
    BnwFreshTradeUninitDraft result;
    // CvTradeClasses.h:48-51 and :142-148 fix FStaticVector capacities at
    // 14*12=168 trade connections and 10 plundered records.
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
        initial.recently_expired_connections.size() > 168 ||
        initial.plundered_connections.size() > 10)
        return result;

    result.selected_rules_sha256 = std::string(selected_rules_sha256);
    result.state = std::move(initial);
    // CvTradeClasses.cpp:1724-1728. No global CvGameTrade call is made.
    result.state.player_bound = false;
    result.source_order.push_back("m_pPlayer");
    result.state.recently_expired_connections.clear();
    result.source_order.push_back("m_aRecentlyExpiredConnections");
    result.state.plundered_connections.clear();
    result.source_order.push_back("m_aTradeConnectionWasPlundered");
    result.member_effect_complete = true;
    return result;
}

} // namespace civ5::runtime
