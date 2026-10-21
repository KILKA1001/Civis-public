#include "civ5/runtime/bnw_fresh_espionage_ai_uninit.hpp"
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
    const std::vector<civ5::content::GameplayDefine> constants{
        {"MAX_MAJOR_CIVS",3}};
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
    const auto grand = project_bnw_fresh_grand_strategy_uninit(
        objects,archive,policies,economic,military,city,wonder,"selected-sha",
        make_bnw_fresh_grand_strategy_constructor_state());
    const auto diplomacy = project_bnw_fresh_diplomacy_uninit(
        objects,archive,policies,economic,military,city,wonder,grand,
        "selected-sha",make_bnw_fresh_diplomacy_constructor_state());
    const auto religions = project_bnw_fresh_religions_uninit(
        objects,archive,policies,economic,military,city,wonder,grand,
        diplomacy,"selected-sha",make_bnw_fresh_religions_constructor_state());
    const auto religion_ai = project_bnw_fresh_religion_ai_uninit(
        objects,archive,policies,economic,military,city,wonder,grand,
        diplomacy,religions,"selected-sha",
        make_bnw_fresh_religion_ai_constructor_state());
    const auto espionage_initial =
        make_bnw_fresh_espionage_constructor_state(constants);
    check(espionage_initial.has_value(),"selected major constant required");
    const auto espionage = project_bnw_fresh_espionage_uninit(
        objects,archive,policies,economic,military,city,wonder,grand,
        diplomacy,religions,religion_ai,"selected-sha",constants,
        *espionage_initial);
    check(espionage.member_effect_complete &&
          !archive.aggregate_traversal_verified,
          "EspionageAI Uninit needs PlayerEspionage predecessor");

    const auto fresh = make_bnw_fresh_espionage_ai_constructor_state();
    check(!fresh.player_bound && fresh.civ_out_of_tech_turn.empty() &&
          !fresh.un_countdown_started && fresh.turn_espionage_started == -1,
          "constructor must leave seven vectors empty");
    const auto fresh_draft = project_bnw_fresh_espionage_ai_uninit(
        objects,archive,policies,economic,military,city,wonder,grand,
        diplomacy,religions,religion_ai,espionage,"selected-sha",constants,
        fresh);
    check(fresh_draft.member_effect_complete &&
          fresh_draft.state.civ_out_of_tech_turn ==
              std::vector<std::int32_t>{-1,-1,-1} &&
          fresh_draft.state.spies_caught == std::vector<std::int32_t>{0,0,0} &&
          fresh_draft.state.spies_killed == std::vector<std::int32_t>{0,0,0} &&
          fresh_draft.state.spies_died == std::vector<std::int32_t>{0,0,0} &&
          fresh_draft.state.last_spy_caught ==
              std::vector<std::int32_t>{-1,-1,-1} &&
          fresh_draft.state.last_spy_killed ==
              std::vector<std::int32_t>{-1,-1,-1} &&
          fresh_draft.state.last_spy_died ==
              std::vector<std::int32_t>{-1,-1,-1} &&
          fresh_draft.source_order.size() == 30 &&
          fresh_draft.source_order[0] == BnwFreshEspionageAiUninitEvent{
              "m_aiCivOutOfTechTurn",BnwFreshEspionageAiUninitOperation::clear,-1} &&
          fresh_draft.source_order[1] == BnwFreshEspionageAiUninitEvent{
              "m_aiCivOutOfTechTurn",BnwFreshEspionageAiUninitOperation::push_back,0} &&
          fresh_draft.source_order[4] == BnwFreshEspionageAiUninitEvent{
              "m_aiNumSpiesCaught",BnwFreshEspionageAiUninitOperation::clear,-1} &&
          fresh_draft.source_order[28] == BnwFreshEspionageAiUninitEvent{
              "m_bUNCountdownStarted",BnwFreshEspionageAiUninitOperation::scalar_write,-1} &&
          fresh_draft.source_order[29] == BnwFreshEspionageAiUninitEvent{
              "m_iTurnEspionageStarted",BnwFreshEspionageAiUninitOperation::scalar_write,-1} &&
          !fresh_draft.archive_predecessor_verified &&
          !fresh_draft.remaining_services_complete &&
          !fresh_draft.initialized_by_gamecore,
          "fresh Uninit must fill seven vectors in SDK order without readiness");

    auto populated = fresh;
    populated.player_bound = true;
    populated.civ_out_of_tech_turn = {4,5};
    populated.spies_caught = {6};
    populated.last_spy_died = {7,8,9};
    populated.un_countdown_started = true;
    populated.turn_espionage_started = 10;
    const auto original = populated;
    auto draft = project_bnw_fresh_espionage_ai_uninit(
        objects,archive,policies,economic,military,city,wonder,grand,
        diplomacy,religions,religion_ai,espionage,"selected-sha",constants,
        populated);
    check(draft.member_effect_complete && draft.state.player_bound &&
          draft.state.civ_out_of_tech_turn ==
              std::vector<std::int32_t>{-1,-1,-1} &&
          draft.state.spies_caught == std::vector<std::int32_t>{0,0,0} &&
          !draft.state.un_countdown_started &&
          draft.state.turn_espionage_started == -1 && populated == original,
          "Uninit must replace populated arrays and retain player binding");

    auto wrong = project_bnw_fresh_espionage_ai_uninit(
        objects,archive,policies,economic,military,city,wonder,grand,
        diplomacy,religions,religion_ai,espionage,"wrong-sha",constants,
        populated);
    check(!wrong.member_effect_complete && wrong.source_order.empty(),
          "selected rules mismatch must fail closed");
    auto missing = espionage;
    missing.member_effect_complete = false;
    wrong = project_bnw_fresh_espionage_ai_uninit(
        objects,archive,policies,economic,military,city,wonder,grand,
        diplomacy,religions,religion_ai,missing,"selected-sha",constants,
        populated);
    check(!wrong.member_effect_complete && wrong.source_order.empty(),
          "missing PlayerEspionage predecessor must fail closed");
    populated.spies_caught = {1,2,3,4};
    wrong = project_bnw_fresh_espionage_ai_uninit(
        objects,archive,policies,economic,military,city,wonder,grand,
        diplomacy,religions,religion_ai,espionage,"selected-sha",constants,
        populated);
    check(!wrong.member_effect_complete && wrong.source_order.empty(),
          "oversize static vector must fail before mutation");
    draft.rollback();
    check(!draft.member_effect_complete && draft.source_order.empty() &&
          draft.selected_rules_sha256.empty(),
          "rollback must discard detached EspionageAI state");
}
