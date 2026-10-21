#include "civ5/runtime/bnw_fresh_player_uninit_prefix.hpp"
#include "civ5/runtime/bnw_fresh_mid_ai_uninit.hpp"
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
    const std::vector<civ5::content::GameplayDefine> constants{{"MAX_MAJOR_CIVS",3}};
    auto input = make_bnw_fresh_player_uninit_prefix_input(constants);
    check(input.has_value(),"native constants required");
    const auto prefix = project_bnw_fresh_player_uninit_prefix(
        objects,"selected-sha",std::move(*input));
    check(prefix.detached_effects_complete &&
          prefix.tactical_ai.member_effect_complete &&
          !prefix.archive_traversal_verified,
          "source-ordered TacticalAI predecessor required");

    const auto fresh = make_bnw_fresh_mid_ai_constructor_state();
    auto draft = project_bnw_fresh_mid_ai_uninit(
        objects,prefix.archive,prefix.tactical_ai,"selected-sha",fresh);
    check(draft.member_effect_complete && draft.state == fresh &&
          draft.source_order.size() == 10 &&
          !draft.archive_predecessor_verified &&
          !draft.remaining_services_complete &&
          !draft.initialized_by_gamecore,
          "fresh no-op/reset calls must keep readiness blocked");

    BnwFreshMidAiUninitInput populated;
    populated.homeland_ai.member_snapshot = {std::byte{0x11}};
    populated.minor_civ_ai.member_snapshot = {std::byte{0x22}};
    populated.deal_ai.member_snapshot = {std::byte{0x33}};
    populated.builder_tasking_ai.repair_build = 7;
    populated.builder_tasking_ai.player_bound = true;
    populated.builder_tasking_ai.logging = true;
    populated.builder_tasking_ai.num_cities = 3;
    populated.builder_tasking_ai.target_plot_bound = true;
    populated.builder_tasking_ai.untouched_member_snapshot = {std::byte{0x44}};
    populated.city_connections.player_bound = true;
    populated.city_connections.route_infos =
        std::vector<std::byte>{std::byte{0x55}};
    populated.city_connections.route_infos_dimension = 64;
    populated.city_connections.buildings_allow_water_routes = {2,4};
    populated.city_connections.plot_route_infos = {std::byte{0x66}};
    draft = project_bnw_fresh_mid_ai_uninit(
        objects,prefix.archive,prefix.tactical_ai,"selected-sha",populated);
    check(draft.member_effect_complete &&
          draft.state.homeland_ai == populated.homeland_ai &&
          draft.state.minor_civ_ai == populated.minor_civ_ai &&
          draft.state.deal_ai == populated.deal_ai &&
          draft.state.builder_tasking_ai.repair_build == -1 &&
          !draft.state.builder_tasking_ai.player_bound &&
          !draft.state.builder_tasking_ai.logging &&
          draft.state.builder_tasking_ai.num_cities == -1 &&
          !draft.state.builder_tasking_ai.target_plot_bound &&
          draft.state.builder_tasking_ai.untouched_member_snapshot ==
              populated.builder_tasking_ai.untouched_member_snapshot &&
          !draft.state.city_connections.player_bound &&
          !draft.state.city_connections.route_infos &&
          draft.state.city_connections.route_infos_dimension == 64 &&
          draft.state.city_connections.buildings_allow_water_routes ==
              std::vector<std::int32_t>{2,4} &&
          draft.state.city_connections.plot_route_infos ==
              populated.city_connections.plot_route_infos &&
          populated.city_connections.route_infos.has_value(),
          "five SDK Uninit calls must alter only named members on detached owner");
    check(draft.source_order == std::vector<std::string_view>{
          "m_pHomelandAI->Uninit","m_pMinorCivAI->Uninit",
          "m_pDealAI->Uninit","BuilderTaskingAI::m_eRepairBuild",
          "BuilderTaskingAI::m_pPlayer","BuilderTaskingAI::m_bLogging",
          "BuilderTaskingAI::m_iNumCities","BuilderTaskingAI::m_pTargetPlot",
          "CityConnections::m_pPlayer","CityConnections::m_aRouteInfos"},
          "CvPlayer::uninit service/member order changed");

    auto without_routes = populated;
    without_routes.city_connections.route_infos.reset();
    const auto no_route_draft = project_bnw_fresh_mid_ai_uninit(
        objects,prefix.archive,prefix.tactical_ai,"selected-sha",without_routes);
    check(no_route_draft.member_effect_complete &&
          !no_route_draft.state.city_connections.route_infos &&
          no_route_draft.state.city_connections.route_infos_dimension == 64,
          "null route pointer must skip delete without changing other fields");
    auto missing = prefix.tactical_ai;
    missing.member_effect_complete = false;
    const auto refused = project_bnw_fresh_mid_ai_uninit(
        objects,prefix.archive,missing,"selected-sha",populated);
    check(!refused.member_effect_complete && refused.source_order.empty(),
          "missing TacticalAI predecessor must fail closed");
    const auto wrong = project_bnw_fresh_mid_ai_uninit(
        objects,prefix.archive,prefix.tactical_ai,"wrong-sha",populated);
    check(!wrong.member_effect_complete && wrong.source_order.empty(),
          "selected rules mismatch must fail closed");
    draft.rollback();
    check(!draft.member_effect_complete && draft.source_order.empty() &&
          draft.selected_rules_sha256.empty(),
          "rollback must discard detached five-service projection");
}
