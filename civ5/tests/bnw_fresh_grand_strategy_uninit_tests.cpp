#include "civ5/runtime/bnw_fresh_grand_strategy_uninit.hpp"
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
} // namespace

int main() {
    const auto objects = first_player();
    const auto archive = project_bnw_fresh_full_archive_reset(
        objects,"selected-sha",make_bnw_fresh_archive_defaults());
    const auto policies = project_bnw_fresh_policies_uninit(
        objects,archive,"selected-sha",
        make_bnw_fresh_policies_constructor_state());
    const auto economic = project_bnw_fresh_economic_uninit(
        objects,archive,policies,"selected-sha",
        make_bnw_fresh_economic_constructor_state());
    const auto military = project_bnw_fresh_military_uninit(
        objects,archive,policies,economic,"selected-sha",
        make_bnw_fresh_military_constructor_state());
    const auto city = project_bnw_fresh_city_specialization_uninit(
        objects,archive,policies,economic,military,"selected-sha",
        make_bnw_fresh_city_specialization_constructor_state());
    const auto wonder = project_bnw_fresh_wonder_uninit(
        objects,archive,policies,economic,military,city,"selected-sha",
        make_bnw_fresh_wonder_constructor_state());
    check(wonder.member_effect_complete && !archive.aggregate_traversal_verified,
          "GrandStrategy Uninit needs detached Wonder predecessor");

    const auto fresh = make_bnw_fresh_grand_strategy_constructor_state();
    check(!fresh.strategy_priorities && !fresh.other_player_strategy_guesses &&
          !fresh.other_player_guess_confidence && !fresh.turns_since_active_set &&
          !fresh.active_strategy && !fresh.player_bound &&
          !fresh.strategies_info_bound,
          "fresh GrandStrategy constructor initializes only owned pointers");
    const auto fresh_draft = project_bnw_fresh_grand_strategy_uninit(
        objects,archive,policies,economic,military,city,wonder,
        "selected-sha",fresh);
    check(fresh_draft.member_effect_complete && fresh_draft.state == fresh &&
          fresh_draft.source_order == std::vector<std::string_view>{
              "CvGrandStrategyAI::m_paiGrandStrategyPriority",
              "CvGrandStrategyAI::m_eGuessOtherPlayerActiveGrandStrategy",
              "CvGrandStrategyAI::m_eGuessOtherPlayerActiveGrandStrategyConfidence"} &&
          !fresh_draft.archive_predecessor_verified &&
          !fresh_draft.remaining_services_complete &&
          !fresh_draft.initialized_by_gamecore,
          "fresh Uninit must preserve source order and readiness blockers");

    auto populated = fresh;
    populated.strategy_priorities = std::vector<std::int32_t>{1,2};
    populated.other_player_strategy_guesses = std::vector<std::int32_t>{3,4};
    populated.other_player_guess_confidence = std::vector<std::int32_t>{5,6};
    populated.turns_since_active_set = 7;
    populated.active_strategy = 8;
    populated.player_bound = true;
    populated.strategies_info_bound = true;
    auto draft = project_bnw_fresh_grand_strategy_uninit(
        objects,archive,policies,economic,military,city,wonder,
        "selected-sha",populated);
    check(draft.member_effect_complete && !draft.state.strategy_priorities &&
          !draft.state.other_player_strategy_guesses &&
          !draft.state.other_player_guess_confidence &&
          draft.state.turns_since_active_set == 7 &&
          draft.state.active_strategy == 8 && draft.state.player_bound &&
          draft.state.strategies_info_bound &&
          populated.strategy_priorities.has_value(),
          "Uninit releases only the three detached arrays");

    auto wrong = project_bnw_fresh_grand_strategy_uninit(
        objects,archive,policies,economic,military,city,wonder,
        "wrong-sha",populated);
    check(!wrong.member_effect_complete && wrong.source_order.empty(),
          "selected rules mismatch must fail closed");
    auto missing_wonder = wonder;
    missing_wonder.member_effect_complete = false;
    wrong = project_bnw_fresh_grand_strategy_uninit(
        objects,archive,policies,economic,military,city,missing_wonder,
        "selected-sha",populated);
    check(!wrong.member_effect_complete && wrong.source_order.empty(),
          "missing Wonder predecessor must fail closed");
    draft.rollback();
    check(!draft.member_effect_complete && draft.source_order.empty() &&
          draft.selected_rules_sha256.empty() &&
          !draft.state.strategy_priorities,
          "rollback must discard detached GrandStrategy effect");
}
