#include "civ5/runtime/bnw_fresh_city_specialization_uninit.hpp"
#include <utility>

namespace civ5::runtime {

void BnwFreshCitySpecializationUninitDraft::rollback() noexcept {
    state = {};
    source_order.clear();
    selected_rules_sha256.clear();
    member_effect_complete = false;
    archive_predecessor_verified = false;
    remaining_services_complete = false;
    initialized_by_gamecore = false;
}

BnwFreshCitySpecializationServiceState
make_bnw_fresh_city_specialization_constructor_state() {
    // CvCitySpecializationAI.cpp:216-225; constructor-initialized members
    // only. NO_CITY_SPECIALIZATION and NO_BUILDING are -1 in CvEnums.h.
    return {};
}

BnwFreshCitySpecializationUninitDraft
project_bnw_fresh_city_specialization_uninit(
    const BnwFreshFirstPlayerObjects &objects,
    const BnwFreshFullArchiveProjection &archive,
    const BnwFreshPoliciesUninitDraft &policies,
    const BnwFreshEconomicUninitDraft &economic,
    const BnwFreshMilitaryUninitDraft &military,
    std::string_view selected_rules_sha256,
    BnwFreshCitySpecializationServiceState initial) {
    BnwFreshCitySpecializationUninitDraft result;
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
        military.initialized_by_gamecore)
        return result;

    result.selected_rules_sha256 = std::string(selected_rules_sha256);
    result.state = std::move(initial);
    result.source_order.push_back("CvCitySpecializationAI::Uninit");
    // The exact SDK method body is empty. Retain every represented value.
    result.member_effect_complete = true;
    return result;
}

} // namespace civ5::runtime
