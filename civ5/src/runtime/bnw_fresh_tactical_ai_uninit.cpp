#include "civ5/runtime/bnw_fresh_tactical_ai_uninit.hpp"
#include <utility>

namespace civ5::runtime {

void BnwFreshTacticalAiUninitDraft::rollback() noexcept {
    state = {};
    source_order.clear();
    selected_rules_sha256.clear();
    member_effect_complete = false;
    archive_predecessor_verified = false;
    remaining_services_complete = false;
    initialized_by_gamecore = false;
}

BnwFreshTacticalAiServiceState make_bnw_fresh_tactical_ai_constructor_state() {
    return {};
}

BnwFreshTacticalAiUninitDraft project_bnw_fresh_tactical_ai_uninit(
    const BnwFreshFirstPlayerObjects &objects,
    const BnwFreshFullArchiveProjection &archive,
    const BnwFreshFlavorManagerUninitDraft &flavor_manager,
    std::string_view selected_rules_sha256,
    BnwFreshTacticalAiServiceState initial) {
    BnwFreshTacticalAiUninitDraft result;
    if (!objects.constructors_complete || objects.initialized_by_gamecore ||
        objects.player_id != 0 || selected_rules_sha256.empty() ||
        objects.selected_rules_sha256 != selected_rules_sha256 ||
        archive.selected_rules_sha256 != selected_rules_sha256 ||
        !archive.all_registered_member_defaults ||
        archive.aggregate_traversal_verified || archive.initialized_by_gamecore ||
        flavor_manager.selected_rules_sha256 != selected_rules_sha256 ||
        !flavor_manager.member_effect_complete ||
        flavor_manager.archive_predecessor_verified ||
        flavor_manager.remaining_services_complete ||
        flavor_manager.initialized_by_gamecore)
        return result;

    result.selected_rules_sha256 = std::string(selected_rules_sha256);
    result.state = std::move(initial);
    result.source_order.push_back("CvTacticalAI::Uninit");
    result.member_effect_complete = true;
    return result;
}

} // namespace civ5::runtime
