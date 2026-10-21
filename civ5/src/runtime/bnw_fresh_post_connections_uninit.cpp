#include "civ5/runtime/bnw_fresh_post_connections_uninit.hpp"
#include <utility>

namespace civ5::runtime {

void BnwFreshPostConnectionsUninitDraft::rollback() noexcept {
    state = {};
    source_order.clear();
    selected_rules_sha256.clear();
    member_effect_complete = false;
    archive_predecessor_verified = false;
    later_containers_complete = false;
    initialized_by_gamecore = false;
}

BnwFreshPostConnectionsUninitInput
make_bnw_fresh_post_connections_constructor_state() {
    BnwFreshPostConnectionsUninitInput result;
    // CvPlayer.cpp:423 allocates CvDangerPlots before the first reset.
    result.danger_plots.emplace();
    return result;
}

BnwFreshPostConnectionsUninitDraft project_bnw_fresh_post_connections_uninit(
    const BnwFreshFirstPlayerObjects &objects,
    const BnwFreshFullArchiveProjection &archive,
    const BnwFreshMidAiUninitDraft &mid_ai,
    std::string_view selected_rules_sha256,
    BnwFreshPostConnectionsUninitInput initial) {
    BnwFreshPostConnectionsUninitDraft result;
    if (!objects.constructors_complete || objects.initialized_by_gamecore ||
        objects.player_id != 0 || selected_rules_sha256.empty() ||
        objects.selected_rules_sha256 != selected_rules_sha256 ||
        archive.selected_rules_sha256 != selected_rules_sha256 ||
        !archive.all_registered_member_defaults ||
        archive.aggregate_traversal_verified || archive.initialized_by_gamecore ||
        mid_ai.selected_rules_sha256 != selected_rules_sha256 ||
        !mid_ai.member_effect_complete || mid_ai.archive_predecessor_verified ||
        mid_ai.remaining_services_complete || mid_ai.initialized_by_gamecore)
        return result;

    result.selected_rules_sha256 = std::string(selected_rules_sha256);
    result.state = std::move(initial);
    // CvNotifications.cpp:253-261, only when m_pNotifications is non-null.
    if (auto &n = result.state.notifications) {
        n->player = -1;
        result.source_order.push_back("Notifications::m_ePlayer");
        n->current_lookup_index = 0;
        result.source_order.push_back("Notifications::m_iCurrentLookupIndex");
        n->notifications.clear();
        result.source_order.push_back("Notifications::m_aNotifications");
        n->begin_index = -1;
        result.source_order.push_back("Notifications::m_iNotificationsBeginIndex");
        n->end_index = -1;
        result.source_order.push_back("Notifications::m_iNotificationsEndIndex");
    }
    // CvDiplomacyRequests.cpp:80-87, only when its pointer is non-null.
    if (auto &d = result.state.diplomacy_requests) {
        d->player = -1;
        result.source_order.push_back("DiplomacyRequests::m_ePlayer");
        d->next_ai_player = -1;
        result.source_order.push_back("DiplomacyRequests::m_eNextAIPlayer");
        d->requests.clear();
        result.source_order.push_back("DiplomacyRequests::m_aRequests");
        d->request_active = false;
        result.source_order.push_back("DiplomacyRequests::m_bRequestActive");
        d->request_active_from_player = -1;
        result.source_order.push_back("DiplomacyRequests::m_bRequestActiveFromPlayer");
    }
    // CvTreasury.cpp:64-67 has an empty Uninit body.
    result.source_order.push_back("m_pTreasury->Uninit");
    // CvTraitClasses.cpp:1556-1566 clears only these eight containers.
    for (std::size_t i = 0; i < kBnwFreshTraitClearOrder.size(); ++i) {
        result.state.trait_containers[i].clear();
        result.source_order.push_back(kBnwFreshTraitClearOrder[i]);
    }
    // CvDangerPlots.cpp:71-77, only when m_pDangerPlots is non-null.
    if (auto &danger = result.state.danger_plots) {
        danger->player = -1;
        result.source_order.push_back("DangerPlots::m_ePlayer");
        danger->danger_plots.clear();
        result.source_order.push_back("DangerPlots::m_DangerPlots");
        danger->array_allocated = false;
        result.source_order.push_back("DangerPlots::m_bArrayAllocated");
        danger->dirty = false;
        result.source_order.push_back("DangerPlots::m_bDirty");
    }
    result.member_effect_complete = true;
    return result;
}

} // namespace civ5::runtime
