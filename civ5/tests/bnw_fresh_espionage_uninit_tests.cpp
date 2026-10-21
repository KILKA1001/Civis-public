#include "civ5/runtime/bnw_fresh_espionage_uninit.hpp"
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
    check(religion_ai.member_effect_complete &&
          !archive.aggregate_traversal_verified,
          "Espionage Uninit needs detached ReligionAI predecessor");

    const auto fresh = make_bnw_fresh_espionage_constructor_state(constants);
    check(fresh && !fresh->player_bound && fresh->spies.empty() &&
          fresh->spy_name_order_index == -1 &&
          fresh->max_tech_cost == std::vector<std::int32_t>{-1,-1,-1} &&
          fresh->heist_locations.size() == 3 &&
          fresh->spy_notifications.empty(),
          "constructor Reset must use selected major civ count");
    const auto fresh_draft = project_bnw_fresh_espionage_uninit(
        objects,archive,policies,economic,military,city,wonder,grand,
        diplomacy,religions,religion_ai,"selected-sha",constants,*fresh);
    check(fresh_draft.member_effect_complete && fresh_draft.state == *fresh &&
          fresh_draft.source_order.size() == 13 &&
          fresh_draft.source_order.front() ==
              BnwFreshEspionageUninitEvent{"m_pPlayer",-1} &&
          fresh_draft.source_order[6] ==
              BnwFreshEspionageUninitEvent{"m_aaPlayerStealableTechList",-1} &&
          fresh_draft.source_order[7] ==
              BnwFreshEspionageUninitEvent{"m_aiMaxTechCost",0} &&
          fresh_draft.source_order[8] ==
              BnwFreshEspionageUninitEvent{"m_aHeistLocations",0} &&
          fresh_draft.source_order.back() ==
              BnwFreshEspionageUninitEvent{"m_aHeistLocations",2} &&
          !fresh_draft.archive_predecessor_verified &&
          !fresh_draft.remaining_services_complete &&
          !fresh_draft.initialized_by_gamecore,
          "Uninit must retain SDK Reset order and readiness blockers");

    auto populated = *fresh;
    populated.player_bound = true;
    populated.spies.push_back({});
    populated.spy_name_order = {4,5};
    populated.spy_name_order_index = 1;
    populated.techs_to_steal = {2,3};
    populated.intrigue_messages.push_back({"Spy name"});
    populated.stealable_techs = {{7},{8}};
    populated.max_tech_cost = {9,10,11};
    populated.heist_locations[0].push_back({12,13});
    populated.spy_notifications.push_back({});
    const auto original = populated;
    auto draft = project_bnw_fresh_espionage_uninit(
        objects,archive,policies,economic,military,city,wonder,grand,
        diplomacy,religions,religion_ai,"selected-sha",constants,populated);
    check(draft.member_effect_complete && !draft.state.player_bound &&
          draft.state.spies.empty() && draft.state.spy_name_order.empty() &&
          draft.state.spy_name_order_index == -1 &&
          draft.state.techs_to_steal.empty() &&
          draft.state.intrigue_messages.empty() &&
          draft.state.stealable_techs.empty() &&
          draft.state.max_tech_cost == std::vector<std::int32_t>{-1,-1,-1} &&
          draft.state.heist_locations[0].empty() &&
          draft.state.spy_notifications == original.spy_notifications &&
          populated == original,
          "Reset must clear owned lists while retaining spy notification queue");

    auto wrong = project_bnw_fresh_espionage_uninit(
        objects,archive,policies,economic,military,city,wonder,grand,
        diplomacy,religions,religion_ai,"wrong-sha",constants,populated);
    check(!wrong.member_effect_complete && wrong.source_order.empty(),
          "selected rules mismatch must fail closed");
    auto missing = religion_ai;
    missing.member_effect_complete = false;
    wrong = project_bnw_fresh_espionage_uninit(
        objects,archive,policies,economic,military,city,wonder,grand,
        diplomacy,religions,missing,"selected-sha",constants,populated);
    check(!wrong.member_effect_complete && wrong.source_order.empty(),
          "missing ReligionAI predecessor must fail closed");
    const std::vector<civ5::content::GameplayDefine> duplicate{
        {"MAX_MAJOR_CIVS",3},{"MAX_MAJOR_CIVS",3}};
    check(!make_bnw_fresh_espionage_constructor_state(duplicate),
          "ambiguous selected native constant must fail closed");
    populated.heist_locations.pop_back();
    wrong = project_bnw_fresh_espionage_uninit(
        objects,archive,policies,economic,military,city,wonder,grand,
        diplomacy,religions,religion_ai,"selected-sha",constants,populated);
    check(!wrong.member_effect_complete && wrong.source_order.empty(),
          "wrong native array extent must fail before mutation");
    draft.rollback();
    check(!draft.member_effect_complete && draft.source_order.empty() &&
          draft.selected_rules_sha256.empty(),
          "rollback must discard detached Espionage state");
}
