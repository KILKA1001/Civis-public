#include "civ5/runtime/bnw_fresh_game_builder.hpp"
#include "civ5/content/rules_database.hpp"
#include "civ5/runtime/file_identity.hpp"
#include <exception>
#include <string_view>
#include <utility>

namespace civ5::runtime {
namespace {
BnwFreshGameBuilderResult blocked(BnwFreshGameBuilderBlockerCode code,
                                  std::string message) {
    BnwFreshGameBuilderResult result;
    result.frontier = BnwFreshGameBuilderBlocker{code, std::move(message)};
    return result;
}

void append_source_prefix(BnwFreshGameBuilderResult &draft) {
    for (const auto &event : draft.roster.source_order) {
        switch (event.kind) {
        case BnwFreshRosterEventKind::team_reset:
            draft.source_order_prefix.push_back(
                {BnwFreshGameBuilderStep::team_init, event.index});
            break;
        case BnwFreshRosterEventKind::major_color_pass_pending:
            draft.source_order_prefix.push_back(
                {BnwFreshGameBuilderStep::major_color_pass, event.index});
            break;
        case BnwFreshRosterEventKind::barbarian_team:
        case BnwFreshRosterEventKind::barbarian_computer_slot:
        case BnwFreshRosterEventKind::observer_team:
        case BnwFreshRosterEventKind::minor_computer_slot:
            break;
        }
    }
    for (const auto &event : draft.slot_info.source_order)
        draft.source_order_prefix.push_back(
            {BnwFreshGameBuilderStep::pregame_slot_write,event.player_id});
    for (const auto &event : draft.first_player_objects.source_order) {
        BnwFreshGameBuilderStep step;
        switch (event.step) {
        case BnwFreshFirstPlayerObjectStep::notifications_allocated:
            step = BnwFreshGameBuilderStep::notifications_allocated; break;
        case BnwFreshFirstPlayerObjectStep::notifications_constructor_uninit:
            step = BnwFreshGameBuilderStep::notifications_constructor_uninit; break;
        case BnwFreshFirstPlayerObjectStep::diplomacy_requests_allocated:
            step = BnwFreshGameBuilderStep::diplomacy_requests_allocated; break;
        case BnwFreshFirstPlayerObjectStep::diplomacy_requests_constructor_uninit:
            step = BnwFreshGameBuilderStep::diplomacy_requests_constructor_uninit; break;
        case BnwFreshFirstPlayerObjectStep::reset_sync_archive_pending:
            step = BnwFreshGameBuilderStep::reset_sync_archive_pending; break;
        }
        draft.source_order_prefix.push_back({step,event.player_id});
    }
}
} // namespace

void BnwFreshGameBuilderResult::rollback() noexcept {
    *this = BnwFreshGameBuilderResult{};
}

BnwFreshGameBuilderResult draft_bnw_fresh_gamecore(
    const BnwFreshGameContentResult &selected,
    const BnwFreshGameBuilderInputs &inputs) {
    if (!selected.content_ready() || !selected.rules || !selected.activation ||
        selected.profile.kind == ContentProfileKind::vanilla ||
        selected.activation->profile_kind != selected.profile.kind) {
        return blocked(BnwFreshGameBuilderBlockerCode::selected_content,
                       "Selected BNW content and activated rules are required");
    }
    try {
        BnwFreshGameBuilderResult draft;
        draft.selected_rules_sha256 = identify_bytes(
            selected.rules->gameplay_fingerprint_data()).sha256;
        const std::string_view sha = draft.selected_rules_sha256;
        draft.roster = draft_bnw_fresh_pregame_roster(
            selected, sha, inputs.native_constants, inputs.pregame_slots,
            inputs.selected_minor_count, inputs.observer_team);
        if (!draft.roster.slot_prefix_complete)
            return blocked(BnwFreshGameBuilderBlockerCode::pregame_roster,
                           "Selected pregame roster cannot be prepared atomically");
        draft.team_fixed = draft_bnw_fresh_team_init_fixed_prefix(
            draft.roster, sha, inputs.native_constants);
        if (!draft.team_fixed.fixed_reset_complete)
            return blocked(BnwFreshGameBuilderBlockerCode::team_init,
                           "Fresh Team fixed reset is incomplete");
        draft.team_info = draft_bnw_fresh_team_info_init(
            selected, draft.team_fixed, sha);
        if (!draft.team_info.info_and_tech_arrays_complete)
            return blocked(BnwFreshGameBuilderBlockerCode::team_init,
                           "Selected Team Info or tech arrays are incomplete");
        draft.colors = draft_bnw_fresh_player_colors(
            selected, sha, inputs.major_color_slots,
            inputs.barbarian_civilization_id);
        if (!draft.colors.major_colors_complete)
            return blocked(BnwFreshGameBuilderBlockerCode::major_colors,
                           "Selected major or barbarian colors are unresolved");
        draft.slot_info = draft_bnw_fresh_slot_info(
            selected,draft.roster,draft.colors,sha,inputs.native_constants,
            inputs.active_minor_civ_ids);
        if (!draft.slot_info.pregame_writes_complete)
            return blocked(BnwFreshGameBuilderBlockerCode::selected_slot_info_pending,
                           "Selected CvPreGame slot writes are incomplete");
        draft.first_player_objects = draft_bnw_fresh_first_player_objects(
            selected,draft.slot_info,sha,inputs.native_constants);
        if (!draft.first_player_objects.constructors_complete)
            return blocked(BnwFreshGameBuilderBlockerCode::reset_sync_archive_pending,
                           "First major player object constructors are incomplete");
        draft.first_player_uninit = plan_bnw_fresh_first_player_uninit(
            selected,draft.first_player_objects,sha);
        if (!draft.first_player_uninit.preflight_complete)
            return blocked(BnwFreshGameBuilderBlockerCode::reset_sync_archive_pending,
                           "First Player reset/uninit source-order preflight is incomplete");
        draft.first_resource_vector = project_bnw_fresh_resource_vector_reset(
            draft.first_player_objects,sha,{});
        if (!draft.first_resource_vector.member_effect_complete ||
            draft.first_resource_vector.archive_traversal_verified)
            return blocked(BnwFreshGameBuilderBlockerCode::reset_sync_archive_pending,
                           "First registered archive vector projection is incomplete");
        draft.first_player_diagnostic = draft_bnw_fresh_builder_player_diagnostic(
            draft.roster,draft.first_player_objects,draft.first_player_uninit,
            draft.first_resource_vector,sha,inputs.native_constants);
        if (!draft.first_player_diagnostic.diagnostic_complete ||
            draft.first_player_diagnostic.source_prefix_advanced ||
            draft.first_player_diagnostic.initialized_by_gamecore)
            return blocked(BnwFreshGameBuilderBlockerCode::reset_sync_archive_pending,
                           "First Player detached Uninit projection is inconsistent");
        draft.player_early = draft_bnw_fresh_player_early_reset(
            selected, draft.roster, sha, inputs.native_constants,
            inputs.pregame_leader_ids);
        if (!draft.player_early.early_arrays_complete)
            return blocked(BnwFreshGameBuilderBlockerCode::player_reset,
                           "Fresh Player early reset is incomplete");
        draft.player_info = draft_bnw_fresh_player_info_reset(
            selected, draft.player_early, sha, inputs.native_constants);
        if (!draft.player_info.selected_arrays_complete)
            return blocked(BnwFreshGameBuilderBlockerCode::player_reset,
                           "Selected Player Info arrays are incomplete");
        draft.economic = draft_bnw_fresh_economic_ai_init(
            selected, draft.player_info, sha);
        if (!draft.economic.economic_ai_init_complete)
            return blocked(BnwFreshGameBuilderBlockerCode::economic_ai,
                           "Fresh EconomicAI reset is incomplete");
        draft.military = draft_bnw_fresh_military_ai_init(
            selected, draft.player_info, draft.economic, sha);
        if (!draft.military.arrays_and_reset_complete)
            return blocked(BnwFreshGameBuilderBlockerCode::military_ai,
                           "Fresh MilitaryAI reset is incomplete");
        draft.city_specialization = draft_bnw_fresh_city_specialization_init(
            selected, draft.military, sha);
        if (!draft.city_specialization.city_specialization_init_complete)
            return blocked(BnwFreshGameBuilderBlockerCode::city_specialization_ai,
                           "Fresh CitySpecializationAI reset is incomplete");
        draft.player_init = draft_bnw_fresh_player_init_prefix(
            draft.roster, draft.colors);
        if (!draft.player_init.member_prefix_complete)
            return blocked(BnwFreshGameBuilderBlockerCode::player_init,
                           "Fresh Player init prefix is incomplete");
        draft.first_alive = draft_bnw_fresh_first_alive_before_league(
            draft.roster, draft.player_init);
        if (!draft.first_alive.pre_league_prefix_complete)
            return blocked(BnwFreshGameBuilderBlockerCode::first_alive,
                           "First setAlive prefix is incomplete");
        draft.diplomatic_votes = draft_bnw_fresh_zero_ever_alive_diplo_votes(
            selected, sha);
        if (!draft.diplomatic_votes.zero_ever_alive_votes_computed)
            return blocked(BnwFreshGameBuilderBlockerCode::diplomatic_votes,
                           "Selected first-alive diplomatic vote rules are unresolved");
        draft.league = init_bnw_fresh_league_state(sha);
        if (!draft.league.constructor_and_init_complete ||
            !stage_bnw_first_alive_league_callback(
                draft.league, draft.first_alive, draft.diplomatic_votes) ||
            !draft.league.first_alive_callback_core_complete)
            return blocked(BnwFreshGameBuilderBlockerCode::league_callback,
                           "First-alive league callback core is incomplete");
        draft.first_player_state = BnwFreshFirstPlayerState::bind(draft,selected,inputs);
        append_source_prefix(draft);
        // Replace the pending reset occurrence with the executed owned prefix.
        draft.source_order_prefix.back().step =
            BnwFreshGameBuilderStep::owned_player_reset_through_tactical_reset;
        draft.frontier = BnwFreshGameBuilderBlocker{
            BnwFreshGameBuilderBlockerCode::tactical_game_analysis_map_source_pending,
            "CvTacticalAI::Init next requires actual CvGame::GetTacticalAnalysisMap owner; then GAME handicap, cached Defines/Info and HomelandAI remain mandatory"};
        draft.detached_diagnostics_complete = true;
        return draft;
    } catch (const std::exception &error) {
        return blocked(BnwFreshGameBuilderBlockerCode::diagnostic_exception,
                       error.what());
    }
}
} // namespace civ5::runtime
