#include "civ5/runtime/bnw_fresh_city_specialization_uninit.hpp"
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
    check(archive.all_registered_member_defaults &&
          policies.member_effect_complete && economic.member_effect_complete &&
          military.member_effect_complete &&
          !archive.aggregate_traversal_verified,
          "CitySpecialization Uninit requires preceding detached service order");

    const auto fresh = make_bnw_fresh_city_specialization_constructor_state();
    check(!fresh.specializations_dirty && !fresh.interrupt_wonders &&
          fresh.next_specialization == -1 && fresh.next_wonder == -1 &&
          fresh.wonder_city_id == -1 && fresh.next_wonder_weight == 0 &&
          fresh.last_turn_evaluated == 0 && fresh.yield_weights.empty() &&
          fresh.production_subtype_weights.empty(),
          "fresh CitySpecialization constructor values differ from SDK");
    const auto fresh_draft = project_bnw_fresh_city_specialization_uninit(
        objects,archive,policies,economic,military,"selected-sha",fresh);
    check(fresh_draft.member_effect_complete && fresh_draft.state == fresh &&
          !fresh_draft.archive_predecessor_verified &&
          !fresh_draft.remaining_services_complete &&
          !fresh_draft.initialized_by_gamecore &&
          fresh_draft.source_order == std::vector<std::string_view>{
              "CvCitySpecializationAI::Uninit"},
          "empty SDK Uninit must record only its call and leave readiness blocked");

    BnwFreshCitySpecializationServiceState populated;
    populated.specializations_dirty = true;
    populated.interrupt_wonders = true;
    populated.next_specialization = 4;
    populated.next_wonder = 7;
    populated.wonder_city_id = 11;
    populated.next_wonder_weight = 23;
    populated.last_turn_evaluated = 31;
    populated.yield_weights = {2,3,5};
    populated.production_subtype_weights = {8,13};
    auto draft = project_bnw_fresh_city_specialization_uninit(
        objects,archive,policies,economic,military,"selected-sha",populated);
    check(draft.member_effect_complete && draft.state == populated &&
          draft.state.yield_weights.data() != populated.yield_weights.data(),
          "populated CitySpecialization state must be retained on detached owner");
    auto wrong = project_bnw_fresh_city_specialization_uninit(
        objects,archive,policies,economic,military,"wrong-sha",populated);
    check(!wrong.member_effect_complete && wrong.source_order.empty(),
          "selected rules mismatch must fail closed");
    auto unavailable = military;
    unavailable.member_effect_complete = false;
    wrong = project_bnw_fresh_city_specialization_uninit(
        objects,archive,policies,economic,unavailable,"selected-sha",populated);
    check(!wrong.member_effect_complete && wrong.source_order.empty(),
          "missing MilitaryAI predecessor must fail closed");
    draft.rollback();
    check(!draft.member_effect_complete && draft.source_order.empty() &&
          draft.selected_rules_sha256.empty(),
          "rollback must discard detached no-op observation");
}
