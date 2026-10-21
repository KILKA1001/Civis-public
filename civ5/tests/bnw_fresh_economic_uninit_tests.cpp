#include "civ5/runtime/bnw_fresh_economic_uninit.hpp"
#include <stdexcept>

namespace {
using namespace civ5::runtime;
void check(bool condition, const char *message) {
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
} // namespace

int main() {
    const auto objects = first_player();
    const auto archive = project_bnw_fresh_full_archive_reset(
        objects,"selected-sha",make_bnw_fresh_archive_defaults());
    const auto policies = project_bnw_fresh_policies_uninit(
        objects,archive,"selected-sha",
        make_bnw_fresh_policies_constructor_state());
    check(archive.all_registered_member_defaults &&
          policies.member_effect_complete &&
          !archive.aggregate_traversal_verified,
          "economic callback needs preceding detached policy projection");

    const auto fresh = make_bnw_fresh_economic_constructor_state();
    check(!fresh.using_strategy && !fresh.turn_strategy_adopted &&
          !fresh.temp_flavors && fresh.early_city_number_target == 1 &&
          fresh.cached_yields.empty() && fresh.exploration_plots.empty(),
          "fresh EconomicAI constructor state differs from SDK");
    const auto fresh_draft = project_bnw_fresh_economic_uninit(
        objects,archive,policies,"selected-sha",fresh);
    check(fresh_draft.member_effect_complete &&
          fresh_draft.state == fresh &&
          !fresh_draft.archive_predecessor_verified &&
          !fresh_draft.remaining_services_complete &&
          !fresh_draft.initialized_by_gamecore,
          "fresh null-array EconomicAI::Uninit should be a source no-op");

    BnwFreshEconomicServiceState allocated;
    allocated.using_strategy = std::vector<bool>{true,false};
    allocated.turn_strategy_adopted = std::vector<std::int32_t>{3,7};
    allocated.temp_flavors =
        std::vector<std::optional<std::int32_t>>{std::nullopt,4};
    allocated.early_city_number_target = 9;
    allocated.cached_yields = {5,6};
    allocated.exploration_plots = {11,12};
    auto populated = project_bnw_fresh_economic_uninit(
        objects,archive,policies,"selected-sha",allocated);
    check(populated.member_effect_complete &&
          !populated.state.using_strategy &&
          !populated.state.turn_strategy_adopted &&
          !populated.state.temp_flavors &&
          populated.state.early_city_number_target == 9 &&
          populated.state.cached_yields == allocated.cached_yields &&
          populated.state.exploration_plots == allocated.exploration_plots &&
          allocated.using_strategy && allocated.turn_strategy_adopted &&
          allocated.temp_flavors &&
          populated.source_order == std::vector<std::string_view>{
              "m_pabUsingStrategy", "m_paiTurnStrategyAdopted",
              "m_aiTempFlavors"},
          "EconomicAI::Uninit must delete only three arrays in source order");
    auto wrong = project_bnw_fresh_economic_uninit(
        objects,archive,policies,"wrong-sha",allocated);
    check(!wrong.member_effect_complete && wrong.source_order.empty(),
          "selected rules mismatch must fail closed before EconomicAI effect");
    populated.rollback();
    check(!populated.member_effect_complete && populated.source_order.empty() &&
          populated.selected_rules_sha256.empty(),
          "economic service rollback must discard detached effects");
}
