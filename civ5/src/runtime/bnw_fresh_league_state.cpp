#include "civ5/runtime/bnw_fresh_league_state.hpp"
#include <algorithm>
#include <utility>

namespace civ5::runtime {

bool BnwFreshLeagueState::has_blocker(BnwFreshLeagueBlockerCode code) const {
    return std::any_of(blockers.begin(), blockers.end(),
                       [code](const auto &blocker) { return blocker.code == code; });
}

void BnwFreshLeagueState::rollback() noexcept {
    active_league_ids.clear();
    leagues_ever_founded = 0;
    diplomatic_victor = -1;
    generated_id_count = 0;
    last_era_trigger = -1;
    votes_needed = 0;
    first_alive_player = -1;
    source_order.clear();
    blockers.clear();
    first_alive_callback_core_complete = false;
    league_screen_dirty_emitted = false;
    initialized_by_gamecore = false;
}

BnwFreshLeagueState init_bnw_fresh_league_state(
    std::string_view selected_rules_sha256) {
    BnwFreshLeagueState result;
    if (selected_rules_sha256.empty()) {
        result.blockers.push_back({
            BnwFreshLeagueBlockerCode::initial_state_unverified,
            "Fresh league state requires selected GameCore rules identity"});
        return result;
    }
    result.selected_rules_sha256 = selected_rules_sha256;
    result.constructor_and_init_complete = true;
    return result;
}

bool stage_bnw_first_alive_league_callback(
    BnwFreshLeagueState &state,
    const BnwFreshAliveBeforeLeague &before_league,
    const BnwFreshDiploVotesDraft &votes) {
    if (!state.constructor_and_init_complete ||
        state.first_alive_callback_core_complete ||
        !state.active_league_ids.empty() ||
        state.selected_rules_sha256.empty() ||
        state.selected_rules_sha256 != before_league.selected_rules_sha256 ||
        state.selected_rules_sha256 != votes.selected_rules_sha256) {
        state.blockers.push_back({
            BnwFreshLeagueBlockerCode::initial_state_unverified,
            "First callback requires untouched, selected fresh league state"});
        return false;
    }
    if (!before_league.pre_league_prefix_complete ||
        before_league.first_active_player < 0 ||
        !before_league.has_blocker(
            BnwFreshAliveBlockerCode::league_callback_pending) ||
        !votes.zero_ever_alive_votes_computed ||
        votes.league_callback_complete ||
        !votes.has_blocker(
            BnwFreshDiploVotesBlockerCode::league_callback_and_ui_pending)) {
        state.blockers.push_back({
            BnwFreshLeagueBlockerCode::first_alive_or_votes_unverified,
            "The first alive player and selected zero-count vote result are required"});
        return false;
    }
    // SDK: iterate m_vActiveLeagues (empty after constructor/Init), call
    // DoUpdateDiploVictory/SetVotesNeededForDiploVictory, then set UI dirty.
    state.blockers.clear();
    state.source_order.push_back(BnwFreshLeagueStep::empty_active_league_scan);
    state.source_order.push_back(BnwFreshLeagueStep::votes_setter_called);
    state.votes_needed = votes.votes_needed;
    state.first_alive_player = before_league.first_active_player;
    state.first_alive_callback_core_complete = true;
    state.source_order.push_back(BnwFreshLeagueStep::league_screen_dirty_pending);
    state.blockers.push_back({
        BnwFreshLeagueBlockerCode::league_screen_dirty_pending,
        "GC.GetEngineUserInterface()->setDirty(LeagueScreen_DIRTY_BIT, true) has not run"});
    return true;
}
} // namespace civ5::runtime
