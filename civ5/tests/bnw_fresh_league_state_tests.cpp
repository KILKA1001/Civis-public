#include "civ5/runtime/bnw_fresh_league_state.hpp"
#include <stdexcept>

namespace {
void check(bool condition, const char *message) {
    if (!condition) throw std::runtime_error(message);
}

void test_initial_league_and_first_alive_callback() {
    using namespace civ5::runtime;
    auto state = init_bnw_fresh_league_state("selected-rules");
    check(state.constructor_and_init_complete &&
              state.active_league_ids.empty() &&
              state.leagues_ever_founded == 0 &&
              state.diplomatic_victor == -1 &&
              state.generated_id_count == 0 &&
              state.last_era_trigger == -1 &&
              state.votes_needed == 0 &&
              !state.initialized_by_gamecore,
          "Fresh CvGameLeagues constructor and empty Init precede players");
    BnwFreshAliveBeforeLeague first;
    first.selected_rules_sha256 = state.selected_rules_sha256;
    first.first_active_player = 0;
    first.pre_league_prefix_complete = true;
    first.blockers.push_back({
        BnwFreshAliveBlockerCode::league_callback_pending,"fixture"});
    BnwFreshDiploVotesDraft votes;
    votes.selected_rules_sha256 = state.selected_rules_sha256;
    votes.votes_needed = 2;
    votes.zero_ever_alive_votes_computed = true;
    votes.blockers.push_back({
        BnwFreshDiploVotesBlockerCode::league_callback_and_ui_pending,"fixture"});
    auto wrong = first;
    wrong.selected_rules_sha256 = "other";
    check(!stage_bnw_first_alive_league_callback(state,wrong,votes) &&
              state.votes_needed == 0 && state.source_order.empty(),
          "Mismatched selected rules cannot partially write league state");
    check(stage_bnw_first_alive_league_callback(state,first,votes) &&
              state.first_alive_callback_core_complete &&
              !state.league_screen_dirty_emitted &&
              !state.initialized_by_gamecore &&
              state.first_alive_player == 0 && state.votes_needed == 2 &&
              state.active_league_ids.empty() &&
              state.source_order == std::vector<BnwFreshLeagueStep>({
                  BnwFreshLeagueStep::empty_active_league_scan,
                  BnwFreshLeagueStep::votes_setter_called,
                  BnwFreshLeagueStep::league_screen_dirty_pending}) &&
              state.has_blocker(BnwFreshLeagueBlockerCode::league_screen_dirty_pending),
          "First league callback writes vote field before pending UI dirty event");
    check(!stage_bnw_first_alive_league_callback(state,first,votes) &&
              state.source_order.size() == 3 && state.votes_needed == 2,
          "Repeated callback staging cannot duplicate ordered effects");
    state.rollback();
    check(state.votes_needed == 0 && state.source_order.empty() &&
              !state.first_alive_callback_core_complete &&
              !state.initialized_by_gamecore &&
              state.selected_rules_sha256 == "selected-rules" &&
              first.first_active_player == 0,
          "Rollback discards detached league writes without changing inputs");
}
} // namespace

int main() { test_initial_league_and_first_alive_callback(); }
