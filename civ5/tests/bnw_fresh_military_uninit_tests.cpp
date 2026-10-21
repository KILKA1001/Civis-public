#include "civ5/runtime/bnw_fresh_military_uninit.hpp"
#include <stdexcept>

namespace {
using namespace civ5::runtime;
void check(bool condition,const char *message) {
    if (!condition) throw std::runtime_error(message);
}
BnwFreshFirstPlayerObjects first_player() {
    BnwFreshFirstPlayerObjects objects;
    objects.player_id = 0;
    objects.selected_rules_sha256 = "selected-sha";
    objects.constructors_complete = true;
    objects.blockers.push_back({
        BnwFreshFirstPlayerObjectBlockerCode::reset_sync_archive_and_uninit_pending,
        "fixture"});
    return objects;
}
}

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
    check(archive.all_registered_member_defaults &&
          policies.member_effect_complete && economic.member_effect_complete &&
          !archive.aggregate_traversal_verified,
          "military service needs the two preceding detached services");

    const auto fresh = make_bnw_fresh_military_constructor_state();
    check(!fresh.using_strategy && !fresh.turn_strategy_adopted &&
          !fresh.temp_flavors && !fresh.last_war_state &&
          !fresh.last_military_threat && !fresh.last_military_strength &&
          !fresh.last_target_value && fresh.total_threat_weight == 1 &&
          fresh.operations_build_skipped == 0 && fresh.army_type_being_built == -1,
          "fresh MilitaryAI constructor state differs from SDK");
    const auto fresh_draft = project_bnw_fresh_military_uninit(
        objects,archive,policies,economic,"selected-sha",fresh);
    check(fresh_draft.member_effect_complete && fresh_draft.state == fresh &&
          !fresh_draft.archive_predecessor_verified &&
          !fresh_draft.remaining_services_complete &&
          !fresh_draft.initialized_by_gamecore,
          "fresh null-array MilitaryAI::Uninit must not advance game readiness");

    BnwFreshMilitaryServiceState allocated;
    allocated.using_strategy = std::vector<bool>{true,false};
    allocated.turn_strategy_adopted = std::vector<std::int32_t>{3,7};
    allocated.temp_flavors =
        std::vector<std::optional<std::int32_t>>{std::nullopt,4};
    allocated.last_war_state =
        std::vector<std::optional<std::int32_t>>{1,std::nullopt};
    allocated.last_military_threat =
        std::vector<std::optional<std::int32_t>>{2,std::nullopt};
    allocated.last_military_strength =
        std::vector<std::optional<std::int32_t>>{3,std::nullopt};
    allocated.last_target_value =
        std::vector<std::optional<std::int32_t>>{4,std::nullopt};
    allocated.total_threat_weight = 19;
    allocated.operations_build_skipped = 5;
    allocated.army_type_being_built = 2;
    auto populated = project_bnw_fresh_military_uninit(
        objects,archive,policies,economic,"selected-sha",allocated);
    check(populated.member_effect_complete &&
          !populated.state.using_strategy &&
          !populated.state.turn_strategy_adopted &&
          !populated.state.temp_flavors &&
          !populated.state.last_war_state &&
          !populated.state.last_military_threat &&
          !populated.state.last_military_strength &&
          !populated.state.last_target_value &&
          populated.state.total_threat_weight == 19 &&
          populated.state.operations_build_skipped == 5 &&
          populated.state.army_type_being_built == 2 &&
          allocated.using_strategy && allocated.last_target_value &&
          populated.source_order == std::vector<std::string_view>{
              "m_pabUsingStrategy", "m_paiTurnStrategyAdopted",
              "m_aiTempFlavors", "m_paeLastTurnWarState",
              "m_paeLastTurnMilitaryThreat", "m_paeLastTurnMilitaryStrength",
              "m_paeLastTurnTargetValue"},
          "MilitaryAI::Uninit must release seven arrays and retain scalars");
    auto wrong = project_bnw_fresh_military_uninit(
        objects,archive,policies,economic,"wrong-sha",allocated);
    check(!wrong.member_effect_complete && wrong.source_order.empty(),
          "selected rules mismatch must block military service");
    auto missing = economic;
    missing.member_effect_complete = false;
    wrong = project_bnw_fresh_military_uninit(
        objects,archive,policies,missing,"selected-sha",allocated);
    check(!wrong.member_effect_complete && wrong.source_order.empty(),
          "missing EconomicAI predecessor must block military service");
    populated.rollback();
    check(!populated.member_effect_complete && populated.source_order.empty() &&
          populated.selected_rules_sha256.empty(),
          "military service rollback must discard detached effects");
}
