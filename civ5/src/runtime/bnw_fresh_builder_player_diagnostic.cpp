#include "civ5/runtime/bnw_fresh_builder_player_diagnostic.hpp"
#include <algorithm>
#include <utility>

namespace civ5::runtime {
namespace {

BnwFreshBuilderPlayerDiagnostic failed(
    BnwFreshBuilderPlayerDiagnosticBlocker code) {
    BnwFreshBuilderPlayerDiagnostic result;
    result.blocker = code;
    result.actual_source_frontier = "m_syncArchive.reset";
    return result;
}

} // namespace

void BnwFreshBuilderPlayerDiagnostic::rollback() noexcept {
    detached_uninit.rollback();
    selected_rules_sha256.clear();
    blocker = BnwFreshBuilderPlayerDiagnosticBlocker::none;
    actual_source_frontier = {};
    diagnostic_complete = false;
    source_prefix_advanced = false;
    initialized_by_gamecore = false;
}

BnwFreshBuilderPlayerDiagnostic draft_bnw_fresh_builder_player_diagnostic(
    const BnwFreshPregameRosterDraft &roster,
    const BnwFreshFirstPlayerObjects &objects,
    const BnwFreshPlayerUninitPlan &plan,
    const BnwFreshArchiveVectorProjection &first_resource_vector,
    std::string_view selected_rules_sha256,
    std::span<const content::GameplayDefine> native_constants) {
    if (selected_rules_sha256.empty() || !roster.slot_prefix_complete ||
        roster.initialized_by_gamecore || roster.selected_rules_sha256 !=
            selected_rules_sha256 || roster.slots.empty() ||
        roster.teams_reset.empty() ||
        (roster.slots[0].status != BnwFreshSlotStatus::taken &&
         roster.slots[0].status != BnwFreshSlotStatus::computer &&
         roster.slots[0].status != BnwFreshSlotStatus::open) ||
        roster.slots[0].team_id < 0 ||
        static_cast<std::size_t>(roster.slots[0].team_id) >=
            roster.teams_reset.size() ||
        roster.teams_reset[static_cast<std::size_t>(roster.slots[0].team_id)] ||
        static_cast<std::size_t>(roster.slots[0].team_id) >=
            roster.source_order.size() ||
        roster.source_order[
            static_cast<std::size_t>(roster.slots[0].team_id)] !=
            BnwFreshRosterEvent{BnwFreshRosterEventKind::team_reset,
                                roster.slots[0].team_id} ||
        !objects.constructors_complete || objects.initialized_by_gamecore ||
        objects.player_id != 0 || objects.selected_rules_sha256 !=
            selected_rules_sha256 ||
        objects.source_order != std::vector<BnwFreshFirstPlayerObjectEvent>{
            {BnwFreshFirstPlayerObjectStep::notifications_allocated,0},
            {BnwFreshFirstPlayerObjectStep::notifications_constructor_uninit,0},
            {BnwFreshFirstPlayerObjectStep::diplomacy_requests_allocated,0},
            {BnwFreshFirstPlayerObjectStep::diplomacy_requests_constructor_uninit,0},
            {BnwFreshFirstPlayerObjectStep::reset_sync_archive_pending,0}} ||
        objects.notifications.player != -1 ||
        objects.notifications.current_lookup_index != 0 ||
        objects.notifications.begin_index != -1 ||
        objects.notifications.end_index != -1 ||
        !objects.notifications.entries_empty ||
        objects.diplomacy_requests.player != -1 ||
        objects.diplomacy_requests.next_ai_player != -1 ||
        objects.diplomacy_requests.active_from_player != -1 ||
        objects.diplomacy_requests.request_active ||
        !objects.diplomacy_requests.entries_empty)
        return failed(BnwFreshBuilderPlayerDiagnosticBlocker::
                      roster_or_selected_rules_invalid);
    if (!plan.preflight_complete || plan.player_id != 0 ||
        plan.selected_rules_sha256 != selected_rules_sha256 ||
        plan.sync_archive_reset_verified || plan.uninit_complete ||
        plan.initialized_by_gamecore || plan.planned_source_order.empty() ||
        plan.planned_source_order.front() != BnwFreshPlayerUninitEvent{
            BnwFreshPlayerUninitKind::sync_archive_reset,"m_syncArchive"} ||
        !plan.has_blocker(BnwFreshPlayerUninitBlockerCode::
                          sync_archive_reset_semantics_unavailable) ||
        !first_resource_vector.member_effect_complete ||
        first_resource_vector.selected_rules_sha256 != selected_rules_sha256 ||
        first_resource_vector.archive_traversal_verified ||
        first_resource_vector.initialized_by_gamecore ||
        first_resource_vector.state.pending_commands != 1 ||
        !first_resource_vector.state.archive_touched ||
        !first_resource_vector.state.values.empty() ||
        first_resource_vector.source_effects !=
            std::vector<BnwFreshArchiveVectorEvent>{
                {BnwFreshArchiveVectorStep::archive_traversal_pending,
                 "m_syncArchive"},
                {BnwFreshArchiveVectorStep::member_reset_projection,
                 "m_paiNumResourceUsed"},
                {BnwFreshArchiveVectorStep::uninit_clear_projection,
                 "m_paiNumResourceUsed"}})
        return failed(BnwFreshBuilderPlayerDiagnosticBlocker::
                      player_archive_preflight_invalid);

    auto input = make_bnw_fresh_player_uninit_prefix_input(native_constants);
    if (!input)
        return failed(BnwFreshBuilderPlayerDiagnosticBlocker::
                      uninit_projection_invalid);
    // CvPlayer::init has already FNEW'd these two objects before reset calls
    // Uninit. Constructor values alone do not encode the non-null pointers.
    input->post_connections.notifications.emplace();
    input->post_connections.diplomacy_requests.emplace();
    BnwFreshBuilderPlayerDiagnostic result;
    result.detached_uninit = project_bnw_fresh_player_uninit_prefix(
        objects,selected_rules_sha256,std::move(*input));
    const auto first_member = std::find_if(
        result.detached_uninit.archive.defaults.begin(),
        result.detached_uninit.archive.defaults.end(),
        [](const BnwArchiveMemberState &member) {
            return member.name == "m_paiNumResourceUsed";
        });
    if (!result.detached_uninit.detached_effects_complete ||
        result.detached_uninit.archive_traversal_verified ||
        result.detached_uninit.uninit_complete ||
        result.detached_uninit.initialized_by_gamecore ||
        result.detached_uninit.selected_rules_sha256 != selected_rules_sha256 ||
        result.detached_uninit.blocked_at != "archive.clearDelta" ||
        result.detached_uninit.source_order.empty() ||
        result.detached_uninit.source_order.front().sdk_member !=
            "m_syncArchive.reset" ||
        result.detached_uninit.unresolved !=
            std::vector<BnwFreshPlayerUninitPrefixBlocker>{
                BnwFreshPlayerUninitPrefixBlocker::archive_reset_traversal_pending,
                BnwFreshPlayerUninitPrefixBlocker::archive_clear_delta_pending} ||
        first_member == result.detached_uninit.archive.defaults.end() ||
        first_member->pending_vector_commands !=
            first_resource_vector.state.pending_commands ||
        !std::holds_alternative<std::vector<std::int32_t>>(first_member->value) ||
        !std::get<std::vector<std::int32_t>>(first_member->value).empty())
        return failed(BnwFreshBuilderPlayerDiagnosticBlocker::
                      uninit_projection_invalid);
    result.selected_rules_sha256 = std::string(selected_rules_sha256);
    result.blocker = BnwFreshBuilderPlayerDiagnosticBlocker::
        archive_reset_traversal_pending;
    result.actual_source_frontier = "m_syncArchive.reset";
    result.diagnostic_complete = true;
    return result;
}

} // namespace civ5::runtime
