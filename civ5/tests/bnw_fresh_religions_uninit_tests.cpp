#include "civ5/runtime/bnw_fresh_religions_uninit.hpp"
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
    const auto grand = project_bnw_fresh_grand_strategy_uninit(
        objects,archive,policies,economic,military,city,wonder,"selected-sha",
        make_bnw_fresh_grand_strategy_constructor_state());
    const auto diplomacy = project_bnw_fresh_diplomacy_uninit(
        objects,archive,policies,economic,military,city,wonder,grand,
        "selected-sha",make_bnw_fresh_diplomacy_constructor_state());
    check(diplomacy.member_effect_complete &&
          !archive.aggregate_traversal_verified,
          "Religions Uninit needs detached Diplomacy predecessor");

    const auto fresh = make_bnw_fresh_religions_constructor_state();
    check(!fresh.player_bound && fresh.prophets_spawned == 0 &&
          fresh.founding_religion == 0,
          "fresh CvPlayerReligions constructor fields differ from SDK");
    const auto fresh_draft = project_bnw_fresh_religions_uninit(
        objects,archive,policies,economic,military,city,wonder,grand,
        diplomacy,"selected-sha",fresh);
    check(fresh_draft.member_effect_complete && fresh_draft.state == fresh &&
          fresh_draft.source_order == std::vector<std::string_view>{
              "CvPlayerReligions::Uninit"} &&
          !fresh_draft.archive_predecessor_verified &&
          !fresh_draft.remaining_services_complete &&
          !fresh_draft.initialized_by_gamecore,
          "empty SDK Uninit must preserve fresh state and readiness blockers");

    BnwFreshReligionsServiceState populated;
    populated.player_bound = true;
    populated.prophets_spawned = 4;
    populated.founding_religion = 1;
    auto draft = project_bnw_fresh_religions_uninit(
        objects,archive,policies,economic,military,city,wonder,grand,
        diplomacy,"selected-sha",populated);
    check(draft.member_effect_complete && draft.state == populated,
          "Uninit must leave populated religion fields unchanged");

    auto wrong = project_bnw_fresh_religions_uninit(
        objects,archive,policies,economic,military,city,wonder,grand,
        diplomacy,"wrong-sha",populated);
    check(!wrong.member_effect_complete && wrong.source_order.empty(),
          "selected rules mismatch must fail closed");
    auto missing_diplomacy = diplomacy;
    missing_diplomacy.member_effect_complete = false;
    wrong = project_bnw_fresh_religions_uninit(
        objects,archive,policies,economic,military,city,wonder,grand,
        missing_diplomacy,"selected-sha",populated);
    check(!wrong.member_effect_complete && wrong.source_order.empty(),
          "missing Diplomacy predecessor must fail closed");
    draft.rollback();
    check(!draft.member_effect_complete && draft.source_order.empty() &&
          draft.selected_rules_sha256.empty(),
          "rollback must discard detached religion service observation");
}
