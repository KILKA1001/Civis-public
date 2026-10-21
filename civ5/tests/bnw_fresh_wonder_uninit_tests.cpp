#include "civ5/runtime/bnw_fresh_wonder_uninit.hpp"
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
    check(city.member_effect_complete && !archive.aggregate_traversal_verified,
          "Wonder Uninit needs detached CitySpecialization predecessor");

    const auto fresh = make_bnw_fresh_wonder_constructor_state();
    check(!fresh.latest_flavors && fresh.wonder_weights.empty() &&
          fresh.buildables.empty() && fresh.player_bound &&
          fresh.buildings_info_bound,
          "fresh Wonder constructor state differs from SDK");
    const auto fresh_draft = project_bnw_fresh_wonder_uninit(
        objects,archive,policies,economic,military,city,"selected-sha",fresh);
    check(fresh_draft.member_effect_complete && fresh_draft.state == fresh &&
          fresh_draft.source_order == std::vector<std::string_view>{
              "CvFlavorRecipient::m_piLatestFlavorValues"} &&
          !fresh_draft.archive_predecessor_verified &&
          !fresh_draft.remaining_services_complete &&
          !fresh_draft.initialized_by_gamecore,
          "inherited fresh Uninit must retain readiness blockers");

    auto populated = fresh;
    populated.latest_flavors = std::vector<std::int32_t>{1,2,3};
    populated.wonder_weights = {{4,5}};
    populated.buildables = {{6,7}};
    auto draft = project_bnw_fresh_wonder_uninit(
        objects,archive,policies,economic,military,city,
        "selected-sha",populated);
    check(draft.member_effect_complete && !draft.state.latest_flavors &&
          draft.state.wonder_weights == populated.wonder_weights &&
          draft.state.buildables == populated.buildables &&
          populated.latest_flavors.has_value(),
          "inherited Uninit releases only detached flavor array");

    auto wrong = project_bnw_fresh_wonder_uninit(
        objects,archive,policies,economic,military,city,
        "wrong-sha",populated);
    check(!wrong.member_effect_complete && wrong.source_order.empty(),
          "selected rules mismatch must fail closed");
    auto missing_city = city;
    missing_city.member_effect_complete = false;
    wrong = project_bnw_fresh_wonder_uninit(
        objects,archive,policies,economic,military,missing_city,
        "selected-sha",populated);
    check(!wrong.member_effect_complete && wrong.source_order.empty(),
          "missing CitySpecialization predecessor must fail closed");
    populated.buildings_info_bound = false;
    wrong = project_bnw_fresh_wonder_uninit(
        objects,archive,policies,economic,military,city,
        "selected-sha",populated);
    check(!wrong.member_effect_complete && wrong.source_order.empty(),
          "missing selected Buildings Info must fail closed");
    draft.rollback();
    check(!draft.member_effect_complete && draft.source_order.empty() &&
          draft.selected_rules_sha256.empty(),
          "rollback must discard detached Wonder service effect");
}
