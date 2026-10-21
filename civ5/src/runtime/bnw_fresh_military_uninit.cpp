#include "civ5/runtime/bnw_fresh_military_uninit.hpp"
#include <utility>

namespace civ5::runtime {

void BnwFreshMilitaryUninitDraft::rollback() noexcept {
    state = {};
    source_order.clear();
    selected_rules_sha256.clear();
    member_effect_complete = false;
    archive_predecessor_verified = false;
    remaining_services_complete = false;
    initialized_by_gamecore = false;
}

BnwFreshMilitaryServiceState make_bnw_fresh_military_constructor_state() {
    return {};
}

BnwFreshMilitaryUninitDraft project_bnw_fresh_military_uninit(
    const BnwFreshFirstPlayerObjects &objects,
    const BnwFreshFullArchiveProjection &archive,
    const BnwFreshPoliciesUninitDraft &policies,
    const BnwFreshEconomicUninitDraft &economic,
    std::string_view selected_rules_sha256,
    BnwFreshMilitaryServiceState initial) {
    BnwFreshMilitaryUninitDraft result;
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
        economic.initialized_by_gamecore)
        return result;

    result.selected_rules_sha256 = std::string(selected_rules_sha256);
    result.state = std::move(initial);
    // CvMilitaryAI.cpp:306-315; SAFE_DELETE_ARRAY nulls its owner. The
    // callback does not clear cached scalars or dereference borrowed handles.
    result.state.using_strategy.reset();
    result.source_order.push_back("m_pabUsingStrategy");
    result.state.turn_strategy_adopted.reset();
    result.source_order.push_back("m_paiTurnStrategyAdopted");
    result.state.temp_flavors.reset();
    result.source_order.push_back("m_aiTempFlavors");
    result.state.last_war_state.reset();
    result.source_order.push_back("m_paeLastTurnWarState");
    result.state.last_military_threat.reset();
    result.source_order.push_back("m_paeLastTurnMilitaryThreat");
    result.state.last_military_strength.reset();
    result.source_order.push_back("m_paeLastTurnMilitaryStrength");
    result.state.last_target_value.reset();
    result.source_order.push_back("m_paeLastTurnTargetValue");
    result.member_effect_complete = true;
    return result;
}

} // namespace civ5::runtime
