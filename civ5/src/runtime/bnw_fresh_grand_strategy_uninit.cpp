#include "civ5/runtime/bnw_fresh_grand_strategy_uninit.hpp"
#include <utility>

namespace civ5::runtime {

void BnwFreshGrandStrategyUninitDraft::rollback() noexcept {
    state = {};
    source_order.clear();
    selected_rules_sha256.clear();
    member_effect_complete = false;
    archive_predecessor_verified = false;
    remaining_services_complete = false;
    initialized_by_gamecore = false;
}

BnwFreshGrandStrategyServiceState make_bnw_fresh_grand_strategy_constructor_state() {
    return {};
}

BnwFreshGrandStrategyUninitDraft project_bnw_fresh_grand_strategy_uninit(
    const BnwFreshFirstPlayerObjects &objects,
    const BnwFreshFullArchiveProjection &archive,
    const BnwFreshPoliciesUninitDraft &policies,
    const BnwFreshEconomicUninitDraft &economic,
    const BnwFreshMilitaryUninitDraft &military,
    const BnwFreshCitySpecializationUninitDraft &city_specialization,
    const BnwFreshWonderUninitDraft &wonder,
    std::string_view selected_rules_sha256,
    BnwFreshGrandStrategyServiceState initial) {
    BnwFreshGrandStrategyUninitDraft result;
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
        wonder.initialized_by_gamecore)
        return result;

    result.selected_rules_sha256 = std::string(selected_rules_sha256);
    result.state = std::move(initial);
    // Expansion2 CvGrandStrategyAI.cpp:158-163 calls SAFE_DELETE_ARRAY in
    // precisely this order; the macro also nulls each detached pointer.
    result.state.strategy_priorities.reset();
    result.source_order.push_back("CvGrandStrategyAI::m_paiGrandStrategyPriority");
    result.state.other_player_strategy_guesses.reset();
    result.source_order.push_back("CvGrandStrategyAI::m_eGuessOtherPlayerActiveGrandStrategy");
    result.state.other_player_guess_confidence.reset();
    result.source_order.push_back("CvGrandStrategyAI::m_eGuessOtherPlayerActiveGrandStrategyConfidence");
    result.member_effect_complete = true;
    return result;
}

} // namespace civ5::runtime
