#include "civ5/runtime/bnw_fresh_wonder_uninit.hpp"
#include <utility>

namespace civ5::runtime {

void BnwFreshWonderUninitDraft::rollback() noexcept {
    state = {};
    source_order.clear();
    selected_rules_sha256.clear();
    member_effect_complete = false;
    archive_predecessor_verified = false;
    remaining_services_complete = false;
    initialized_by_gamecore = false;
}

BnwFreshWonderServiceState make_bnw_fresh_wonder_constructor_state() {
    // CvPlayer.cpp:411 binds `this` and GC.GetGameBuildings();
    // CvFlavorManager.cpp:16-21 starts latest flavors NULL.
    return {};
}

BnwFreshWonderUninitDraft project_bnw_fresh_wonder_uninit(
    const BnwFreshFirstPlayerObjects &objects,
    const BnwFreshFullArchiveProjection &archive,
    const BnwFreshPoliciesUninitDraft &policies,
    const BnwFreshEconomicUninitDraft &economic,
    const BnwFreshMilitaryUninitDraft &military,
    const BnwFreshCitySpecializationUninitDraft &city_specialization,
    std::string_view selected_rules_sha256,
    BnwFreshWonderServiceState initial) {
    BnwFreshWonderUninitDraft result;
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
        !initial.player_bound || !initial.buildings_info_bound)
        return result;

    result.selected_rules_sha256 = std::string(selected_rules_sha256);
    result.state = std::move(initial);
    // No CvWonderProductionAI::Uninit override exists in the Expansion2 SDK.
    // CvFlavorRecipient::Uninit at CvFlavorManager.cpp:37-40 is the only
    // dispatched method; SAFE_DELETE_ARRAY nulls the owned pointer.
    result.state.latest_flavors.reset();
    result.source_order.push_back("CvFlavorRecipient::m_piLatestFlavorValues");
    result.member_effect_complete = true;
    return result;
}

} // namespace civ5::runtime
