#include "civ5/runtime/bnw_fresh_economic_uninit.hpp"
#include <utility>

namespace civ5::runtime {

void BnwFreshEconomicUninitDraft::rollback() noexcept {
    state = {};
    source_order.clear();
    selected_rules_sha256.clear();
    member_effect_complete = false;
    archive_predecessor_verified = false;
    remaining_services_complete = false;
    initialized_by_gamecore = false;
}

BnwFreshEconomicServiceState make_bnw_fresh_economic_constructor_state() {
    // CvEconomicAI.cpp:244-250. Its three SAFE_DELETE_ARRAY targets start
    // NULL. m_iEarlyCityNumberTarget starts at 1. FFastVector members start
    // empty by their constructors, before any EconomicAI::Init.
    return {};
}

BnwFreshEconomicUninitDraft project_bnw_fresh_economic_uninit(
    const BnwFreshFirstPlayerObjects &objects,
    const BnwFreshFullArchiveProjection &archive,
    const BnwFreshPoliciesUninitDraft &policies,
    std::string_view selected_rules_sha256,
    BnwFreshEconomicServiceState initial) {
    BnwFreshEconomicUninitDraft result;
    if (!objects.constructors_complete || objects.initialized_by_gamecore ||
        objects.player_id != 0 || selected_rules_sha256.empty() ||
        objects.selected_rules_sha256 != selected_rules_sha256 ||
        archive.selected_rules_sha256 != selected_rules_sha256 ||
        !archive.all_registered_member_defaults ||
        archive.aggregate_traversal_verified || archive.initialized_by_gamecore ||
        policies.selected_rules_sha256 != selected_rules_sha256 ||
        !policies.member_effect_complete ||
        policies.archive_predecessor_verified ||
        policies.initialized_by_gamecore)
        return result;

    result.selected_rules_sha256 = std::string(selected_rules_sha256);
    result.state = std::move(initial);
    // SAFE_DELETE_ARRAY in FFireTypes.h nulls each owner. The source Uninit
    // body has no calls, cache writes or writes through m_pPlayer.
    result.state.using_strategy.reset();
    result.source_order.push_back("m_pabUsingStrategy");
    result.state.turn_strategy_adopted.reset();
    result.source_order.push_back("m_paiTurnStrategyAdopted");
    result.state.temp_flavors.reset();
    result.source_order.push_back("m_aiTempFlavors");
    result.member_effect_complete = true;
    return result;
}

} // namespace civ5::runtime
