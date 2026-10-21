#include "civ5/runtime/bnw_fresh_flavor_manager_uninit.hpp"
#include <utility>

namespace civ5::runtime {

void BnwFreshFlavorManagerUninitDraft::rollback() noexcept {
    state = {};
    source_order.clear();
    selected_rules_sha256.clear();
    member_effect_complete = false;
    archive_predecessor_verified = false;
    remaining_services_complete = false;
    initialized_by_gamecore = false;
}

BnwFreshFlavorManagerServiceState
make_bnw_fresh_flavor_manager_constructor_state() {
    return {};
}

BnwFreshFlavorManagerUninitDraft project_bnw_fresh_flavor_manager_uninit(
    const BnwFreshFirstPlayerObjects &objects,
    const BnwFreshFullArchiveProjection &archive,
    const BnwFreshPlayerTechsUninitDraft &player_techs,
    std::string_view selected_rules_sha256,
    BnwFreshFlavorManagerServiceState initial) {
    BnwFreshFlavorManagerUninitDraft result;
    if (!objects.constructors_complete || objects.initialized_by_gamecore ||
        objects.player_id != 0 || selected_rules_sha256.empty() ||
        objects.selected_rules_sha256 != selected_rules_sha256 ||
        archive.selected_rules_sha256 != selected_rules_sha256 ||
        !archive.all_registered_member_defaults ||
        archive.aggregate_traversal_verified || archive.initialized_by_gamecore ||
        player_techs.selected_rules_sha256 != selected_rules_sha256 ||
        !player_techs.member_effect_complete ||
        player_techs.archive_predecessor_verified ||
        player_techs.remaining_services_complete ||
        player_techs.initialized_by_gamecore)
        return result;

    result.selected_rules_sha256 = std::string(selected_rules_sha256);
    result.state = std::move(initial);
    result.state.personality_flavor.reset();
    result.source_order.push_back("m_piPersonalityFlavor");
    result.state.active_flavor.reset();
    result.source_order.push_back("m_piActiveFlavor");
    result.state.target_recipients.clear();
    result.source_order.push_back("m_FlavorTargetList");
    result.member_effect_complete = true;
    return result;
}

} // namespace civ5::runtime
