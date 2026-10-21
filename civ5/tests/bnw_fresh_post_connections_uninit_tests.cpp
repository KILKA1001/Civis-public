#include "civ5/runtime/bnw_fresh_player_uninit_prefix.hpp"
#include "civ5/runtime/bnw_fresh_post_connections_uninit.hpp"
#include <stdexcept>

namespace {
using namespace civ5::runtime;
void check(bool value, const char *message) {
    if (!value) throw std::runtime_error(message);
}
BnwFreshFirstPlayerObjects first_player() {
    BnwFreshFirstPlayerObjects result;
    result.player_id = 0;
    result.selected_rules_sha256 = "selected-sha";
    result.constructors_complete = true;
    result.blockers.push_back({
        BnwFreshFirstPlayerObjectBlockerCode::reset_sync_archive_and_uninit_pending,
        "fixture"});
    return result;
}
BnwFreshMidAiUninitDraft predecessor(const BnwFreshFirstPlayerObjects &objects) {
    const std::vector<civ5::content::GameplayDefine> constants{{"MAX_MAJOR_CIVS",3}};
    auto input = make_bnw_fresh_player_uninit_prefix_input(constants);
    check(input.has_value(),"selected native constants required");
    auto prefix = project_bnw_fresh_player_uninit_prefix(
        objects,"selected-sha",std::move(*input));
    check(prefix.detached_effects_complete &&
          prefix.tactical_ai.member_effect_complete &&
          !prefix.archive_traversal_verified,
          "TacticalAI predecessor required");
    return project_bnw_fresh_mid_ai_uninit(
        objects,prefix.archive,prefix.tactical_ai,"selected-sha",
        make_bnw_fresh_mid_ai_constructor_state());
}
} // namespace

int main() {
    const auto objects = first_player();
    // The standalone service fixture also exercises the complete predecessor.
    const std::vector<civ5::content::GameplayDefine> constants{{"MAX_MAJOR_CIVS",3}};
    auto input = make_bnw_fresh_player_uninit_prefix_input(constants);
    check(input.has_value(),"selected constants required");
    const auto prefix = project_bnw_fresh_player_uninit_prefix(
        objects,"selected-sha",std::move(*input));
    const auto mid = predecessor(objects);
    check(mid.member_effect_complete,"mid AI predecessor required");

    const auto fresh = make_bnw_fresh_post_connections_constructor_state();
    auto draft = project_bnw_fresh_post_connections_uninit(
        objects,prefix.archive,mid,"selected-sha",fresh);
    check(draft.member_effect_complete && draft.state == fresh &&
          draft.source_order.size() == 13 &&
          draft.source_order[0] == "m_pTreasury->Uninit" &&
          draft.source_order[1] == kBnwFreshTraitClearOrder[0] &&
          draft.source_order[8] == kBnwFreshTraitClearOrder[7] &&
          draft.source_order[9] == "DangerPlots::m_ePlayer" &&
          draft.source_order[12] == "DangerPlots::m_bDirty" &&
          !draft.archive_predecessor_verified &&
          !draft.later_containers_complete && !draft.initialized_by_gamecore,
          "null optional services must skip calls and preserve blockers");
    auto absent_danger = fresh;
    absent_danger.danger_plots.reset();
    const auto absent_draft = project_bnw_fresh_post_connections_uninit(
        objects,prefix.archive,mid,"selected-sha",absent_danger);
    check(absent_draft.member_effect_complete &&
          absent_draft.source_order.size() == 9 &&
          !absent_draft.state.danger_plots,
          "conditional null DangerPlots must skip its Uninit");

    BnwFreshPostConnectionsUninitInput populated;
    populated.notifications = BnwFreshNotificationsUninitState{
        0,9,{std::byte{0x11}},2,3};
    populated.diplomacy_requests = BnwFreshDiplomacyRequestsUninitState{
        0,2,{std::byte{0x22}},true,1};
    populated.treasury_opaque_state = {std::byte{0x33}};
    for (std::size_t i = 0; i < populated.trait_containers.size(); ++i)
        populated.trait_containers[i] = {std::byte{static_cast<unsigned char>(i+1)}};
    populated.danger_plots = BnwFreshDangerPlotsUninitState{
        0,{4,8},true,true,{std::byte{0x44}}};
    draft = project_bnw_fresh_post_connections_uninit(
        objects,prefix.archive,mid,"selected-sha",populated);
    check(draft.member_effect_complete && draft.source_order.size() == 23 &&
          draft.source_order[0] == "Notifications::m_ePlayer" &&
          draft.source_order[4] == "Notifications::m_iNotificationsEndIndex" &&
          draft.source_order[5] == "DiplomacyRequests::m_ePlayer" &&
          draft.source_order[9] == "DiplomacyRequests::m_bRequestActiveFromPlayer" &&
          draft.source_order[10] == "m_pTreasury->Uninit" &&
          draft.source_order[11] == kBnwFreshTraitClearOrder[0] &&
          draft.source_order[18] == kBnwFreshTraitClearOrder[7] &&
          draft.source_order[19] == "DangerPlots::m_ePlayer" &&
          draft.source_order[22] == "DangerPlots::m_bDirty",
          "conditional source-order effects changed");
    check(draft.state.notifications && draft.state.notifications->player == -1 &&
          draft.state.notifications->current_lookup_index == 0 &&
          draft.state.notifications->notifications.empty() &&
          draft.state.notifications->begin_index == -1 &&
          draft.state.notifications->end_index == -1 &&
          draft.state.diplomacy_requests &&
          draft.state.diplomacy_requests->player == -1 &&
          draft.state.diplomacy_requests->next_ai_player == -1 &&
          draft.state.diplomacy_requests->requests.empty() &&
          !draft.state.diplomacy_requests->request_active &&
          draft.state.diplomacy_requests->request_active_from_player == -1 &&
          draft.state.treasury_opaque_state == populated.treasury_opaque_state &&
          draft.state.danger_plots && draft.state.danger_plots->player == -1 &&
          draft.state.danger_plots->danger_plots.empty() &&
          !draft.state.danger_plots->array_allocated &&
          !draft.state.danger_plots->dirty &&
          draft.state.danger_plots->untouched_modifiers ==
              populated.danger_plots->untouched_modifiers &&
          populated.notifications->notifications.size() == 1,
          "detached service projection must preserve untouched fields and caller");
    for (const auto &container : draft.state.trait_containers)
        check(container.empty(),"all eight Traits containers must clear");

    auto missing = mid;
    missing.member_effect_complete = false;
    const auto refused = project_bnw_fresh_post_connections_uninit(
        objects,prefix.archive,missing,"selected-sha",populated);
    check(!refused.member_effect_complete && refused.source_order.empty(),
          "missing CityConnections predecessor must fail closed");
    const auto wrong = project_bnw_fresh_post_connections_uninit(
        objects,prefix.archive,mid,"wrong-sha",populated);
    check(!wrong.member_effect_complete && wrong.source_order.empty(),
          "selected rules mismatch must fail closed");
    draft.rollback();
    check(!draft.member_effect_complete && draft.source_order.empty() &&
          draft.selected_rules_sha256.empty(),
          "rollback must discard detached projection");
}
