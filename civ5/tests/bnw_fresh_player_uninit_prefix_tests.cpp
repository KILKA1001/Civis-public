#include "civ5/runtime/bnw_fresh_player_uninit_prefix.hpp"
#include "civ5/runtime/bnw_fresh_archive_vector.hpp"
#include <algorithm>
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
    const std::vector<civ5::content::GameplayDefine> constants{
        {"MAX_MAJOR_CIVS",3}};
    auto input = make_bnw_fresh_player_uninit_prefix_input(constants);
    check(input.has_value(),"selected native constants must initialize Espionage");
    auto initial = std::move(*input);
    const auto member = std::find_if(
        initial.archive_members.begin(),initial.archive_members.end(),
        [](const BnwArchiveMemberState &value) {
            return value.name == "m_paiNumResourceUsed";
        });
    check(member != initial.archive_members.end(),
          "selected archive catalog must include first early vector");
    member->value = std::vector<std::int32_t>{2,3,5};
    initial.policies.latest_flavors = std::vector<std::int32_t>{7};
    initial.economic.using_strategy = std::vector<bool>{true};
    initial.military.using_strategy = std::vector<bool>{true};
    initial.wonder.latest_flavors = std::vector<std::int32_t>{11};
    initial.grand_strategy.strategy_priorities =
        std::vector<std::int32_t>{13};
    initial.diplomacy.owned_diplo_data =
        std::vector<std::byte>{std::byte{0x17}};
    std::fill(initial.diplomacy.null_written_aliases.begin(),
              initial.diplomacy.null_written_aliases.end(),
              BnwFreshDiplomacyAliasState::bound_to_diplo_data);
    initial.diplomacy.other_player_major_opinion_alias =
        BnwFreshDiplomacyAliasState::bound_to_diplo_data;
    initial.religions.player_bound = true;
    initial.religions.prophets_spawned = 19;
    initial.religion_ai.player_bound = true;
    initial.religion_ai.beliefs_bound = true;
    initial.espionage.player_bound = true;
    initial.espionage.spies.push_back({});
    initial.espionage.max_tech_cost = {5,8,13};
    initial.espionage.heist_locations[1].push_back({2,3});
    initial.espionage.spy_notifications.push_back({});
    initial.espionage_ai.player_bound = true;
    initial.espionage_ai.spies_caught = {3,5};
    initial.espionage_ai.un_countdown_started = true;
    initial.espionage_ai.turn_espionage_started = 21;
    initial.trade.player_bound = true;
    initial.trade.recently_expired_connections = {{std::byte{0x01}}};
    initial.trade.plundered_connections = {{std::byte{0x02}}};
    initial.trade_ai.player_bound = true;
    initial.trade_ai.removable_value = 23;
    initial.league_ai.player_bound = true;
    initial.league_ai.vote_commitments.push_back({1,2,3,4,true});
    initial.player_techs.tech_entries_bound = true;
    initial.player_techs.player_bound = true;
    initial.player_techs.latest_flavor_values = std::vector<std::int32_t>{17};
    initial.player_techs.researching_tech = std::vector<bool>{true};
    initial.player_techs.tech_ai_weights =
        std::vector<std::pair<std::int32_t,std::int32_t>>{{0,7}};
    initial.flavor_manager.player_bound = true;
    initial.flavor_manager.personality_flavor =
        std::vector<std::int32_t>{19};
    initial.flavor_manager.active_flavor = std::vector<std::int32_t>{23};
    initial.flavor_manager.target_recipients = {1,2,1};
    initial.tactical_ai.current_zone_index = 7;
    initial.tactical_ai.opaque_member_snapshot = {std::byte{0x5a}};
    initial.mid_ai.homeland_ai.member_snapshot = {std::byte{0x71}};
    initial.mid_ai.builder_tasking_ai.repair_build = 4;
    initial.mid_ai.builder_tasking_ai.player_bound = true;
    initial.mid_ai.city_connections.player_bound = true;
    initial.mid_ai.city_connections.route_infos =
        std::vector<std::byte>{std::byte{0x72}};
    initial.post_connections =
        make_bnw_fresh_post_connections_constructor_state();
    initial.post_connections.notifications = BnwFreshNotificationsUninitState{
        0,5,{std::byte{0x73}},0,1};
    initial.post_connections.diplomacy_requests =
        BnwFreshDiplomacyRequestsUninitState{
            0,1,{std::byte{0x74}},true,1};
    initial.post_connections.trait_containers[0] = {std::byte{0x75}};
    initial.post_connections.danger_plots->danger_plots = {2,3};
    initial.post_connections.danger_plots->array_allocated = true;
    initial.tail_containers.unit_cycle_ids = {2,4};
    initial.tail_containers.research_queue = {1,3};
    initial.tail_containers.city_names = {"Alpha"};
    const auto original = initial;

    auto draft = project_bnw_fresh_player_uninit_prefix(
        objects,"selected-sha",initial);
    check(draft.detached_effects_complete && !draft.archive_traversal_verified &&
          !draft.uninit_complete && !draft.initialized_by_gamecore &&
          draft.blocked_at == "archive.clearDelta" &&
          draft.unresolved == std::vector<BnwFreshPlayerUninitPrefixBlocker>{
              BnwFreshPlayerUninitPrefixBlocker::archive_reset_traversal_pending,
              BnwFreshPlayerUninitPrefixBlocker::archive_clear_delta_pending},
          "detached prefix must expose both exact unresolved frontiers");
    check(draft.source_order.size() == 60 &&
          draft.source_order[0] == BnwFreshPlayerUninitPrefixEvent{
              BnwFreshPlayerUninitPrefixEventKind::archive_reset_projection,
              "m_syncArchive.reset",false} &&
          draft.source_order[1] == BnwFreshPlayerUninitPrefixEvent{
              BnwFreshPlayerUninitPrefixEventKind::vector_clear,
              "m_paiNumResourceUsed",true} &&
          draft.source_order[21] == BnwFreshPlayerUninitPrefixEvent{
              BnwFreshPlayerUninitPrefixEventKind::vector_clear,
              "m_pabGetsScienceFromPlayer",true} &&
          draft.source_order[22] == BnwFreshPlayerUninitPrefixEvent{
              BnwFreshPlayerUninitPrefixEventKind::service_uninit,
              "m_pPlayerPolicies->Uninit",true} &&
          draft.source_order[29] == BnwFreshPlayerUninitPrefixEvent{
              BnwFreshPlayerUninitPrefixEventKind::service_uninit,
              "m_pReligions->Uninit",true} &&
          draft.source_order[30] == BnwFreshPlayerUninitPrefixEvent{
              BnwFreshPlayerUninitPrefixEventKind::service_uninit,
              "m_pReligionAI->Uninit",true} &&
          draft.source_order[31] == BnwFreshPlayerUninitPrefixEvent{
              BnwFreshPlayerUninitPrefixEventKind::service_uninit,
              "m_pEspionage->Uninit",true} &&
          draft.source_order[32] == BnwFreshPlayerUninitPrefixEvent{
              BnwFreshPlayerUninitPrefixEventKind::service_uninit,
              "m_pEspionageAI->Uninit",true} &&
          draft.source_order[33] == BnwFreshPlayerUninitPrefixEvent{
              BnwFreshPlayerUninitPrefixEventKind::service_uninit,
              "m_pTrade->Uninit",true} &&
          draft.source_order[34] == BnwFreshPlayerUninitPrefixEvent{
              BnwFreshPlayerUninitPrefixEventKind::service_uninit,
              "m_pTradeAI->Uninit",true} &&
          draft.source_order[35] == BnwFreshPlayerUninitPrefixEvent{
              BnwFreshPlayerUninitPrefixEventKind::service_uninit,
              "m_pLeagueAI->Uninit",true} &&
          draft.source_order[36] == BnwFreshPlayerUninitPrefixEvent{
              BnwFreshPlayerUninitPrefixEventKind::service_uninit,
              "m_pPlayerTechs->Uninit",true} &&
          draft.source_order[37] == BnwFreshPlayerUninitPrefixEvent{
              BnwFreshPlayerUninitPrefixEventKind::service_uninit,
              "m_pFlavorManager->Uninit",true} &&
          draft.source_order[38] == BnwFreshPlayerUninitPrefixEvent{
              BnwFreshPlayerUninitPrefixEventKind::service_uninit,
              "m_pTacticalAI->Uninit",true} &&
          draft.source_order[39].sdk_member == "m_pHomelandAI->Uninit" &&
          draft.source_order[43].sdk_member == "m_pCityConnections->Uninit" &&
          draft.source_order[44].sdk_member == "m_pNotifications->Uninit" &&
          draft.source_order[45].sdk_member == "m_pDiplomacyRequests->Uninit" &&
          draft.source_order[46].sdk_member == "m_pTreasury->Uninit" &&
          draft.source_order[47].sdk_member == "m_pTraits->Uninit" &&
          draft.source_order[48].sdk_member == "m_pDangerPlots->Uninit" &&
          draft.source_order[49] == BnwFreshPlayerUninitPrefixEvent{
              BnwFreshPlayerUninitPrefixEventKind::vector_clear,
              "m_ppaaiSpecialistExtraYield",false} &&
          draft.source_order[51].sdk_member ==
              "m_ppaaiBuildingClassYieldMod" &&
          draft.source_order[52] == BnwFreshPlayerUninitPrefixEvent{
              BnwFreshPlayerUninitPrefixEventKind::container_clear,
              "m_UnitCycle.Clear",true} &&
          draft.source_order[54].sdk_member == "m_cityNames.clear" &&
          draft.source_order[55].sdk_member == "m_cities.Uninit" &&
          draft.source_order[56].sdk_member == "m_units.Uninit" &&
          draft.source_order[57].sdk_member == "m_AIOperations.clear" &&
          draft.source_order[58].sdk_member == "m_aiPlots.clear" &&
          draft.source_order[59].sdk_member == "m_bfEverConqueredBy.ClearAll",
          "aggregate projection must preserve CvPlayer::uninit call order");
    const auto reset_member = std::find_if(
        draft.archive.defaults.begin(),draft.archive.defaults.end(),
        [](const BnwArchiveMemberState &value) {
            return value.name == "m_paiNumResourceUsed";
        });
    const auto first_vector = project_bnw_fresh_resource_vector_reset(
        objects,"selected-sha",{});
    check(first_vector.member_effect_complete &&
          reset_member != draft.archive.defaults.end() &&
          reset_member->pending_vector_commands ==
              first_vector.state.pending_commands &&
          reset_member->pending_vector_commands == 1,
          "aggregate first vector must preserve standalone CLEAR command");
    for (const auto &clear : kBnwPlayerUninitVectorClears) {
        if (clear.after_service_callbacks) break;
        const auto item = std::find_if(
            draft.archive.defaults.begin(),draft.archive.defaults.end(),
            [&clear](const BnwArchiveMemberState &member) {
                return member.name == clear.name;
            });
        check(item != draft.archive.defaults.end() &&
              item->pending_vector_commands == 1,
              "all early FAutoVector clears must stage one archive command");
    }
    check(reset_member != draft.archive.defaults.end() &&
          std::get<std::vector<std::int32_t>>(reset_member->value).empty() &&
          !draft.policies.state.latest_flavors &&
          !draft.economic.state.using_strategy &&
          !draft.military.state.using_strategy &&
          !draft.wonder.state.latest_flavors &&
          !draft.grand_strategy.state.strategy_priorities &&
          !draft.diplomacy.state.owned_diplo_data &&
          draft.diplomacy.state.other_player_major_opinion_alias ==
              BnwFreshDiplomacyAliasState::dangling_after_delete &&
          draft.religions.state == original.religions &&
          draft.religion_ai.state == original.religion_ai &&
          !draft.espionage.state.player_bound &&
          draft.espionage.state.spies.empty() &&
          draft.espionage.state.max_tech_cost ==
              std::vector<std::int32_t>{-1,-1,-1} &&
          draft.espionage.state.heist_locations[1].empty() &&
          draft.espionage.state.spy_notifications ==
              original.espionage.spy_notifications &&
          draft.espionage_ai.state.player_bound &&
          draft.espionage_ai.state.spies_caught ==
              std::vector<std::int32_t>{0,0,0} &&
          !draft.espionage_ai.state.un_countdown_started &&
          draft.espionage_ai.state.turn_espionage_started == -1 &&
          !draft.trade.state.player_bound &&
          draft.trade.state.recently_expired_connections.empty() &&
          draft.trade.state.plundered_connections.empty() &&
          !draft.trade_ai.state.player_bound &&
          draft.trade_ai.state.removable_value == 23 &&
          draft.league_ai.state == original.league_ai &&
          draft.player_techs.state.tech_entries_bound &&
          draft.player_techs.state.player_bound &&
          !draft.player_techs.state.latest_flavor_values &&
          !draft.player_techs.state.researching_tech &&
          !draft.player_techs.state.tech_ai_weights &&
          draft.flavor_manager.state.player_bound == true &&
          !draft.flavor_manager.state.personality_flavor &&
          !draft.flavor_manager.state.active_flavor &&
          draft.flavor_manager.state.target_recipients.empty() &&
          draft.tactical_ai.state == original.tactical_ai &&
          draft.mid_ai.state.homeland_ai == original.mid_ai.homeland_ai &&
          draft.mid_ai.state.builder_tasking_ai.repair_build == -1 &&
          !draft.mid_ai.state.builder_tasking_ai.player_bound &&
          !draft.mid_ai.state.city_connections.player_bound &&
          !draft.mid_ai.state.city_connections.route_infos &&
          draft.post_connections.state.notifications &&
          draft.post_connections.state.notifications->notifications.empty() &&
          draft.post_connections.state.diplomacy_requests &&
          draft.post_connections.state.diplomacy_requests->requests.empty() &&
          draft.post_connections.state.trait_containers[0].empty() &&
          draft.post_connections.state.danger_plots &&
          draft.post_connections.state.danger_plots->danger_plots.empty() &&
          draft.tail_containers.detached_container_effects_complete &&
          !draft.tail_containers.archive_touch_intents_materialized &&
          draft.tail_containers.state.unit_cycle_ids.empty() &&
          draft.tail_containers.state.research_queue.empty() &&
          draft.tail_containers.state.city_names.empty() &&
          draft.entities.detached_effects_complete &&
          !draft.entities.cities.array_allocated &&
          !draft.entities.units.array_allocated &&
          draft.operations.detached_effect_complete &&
          draft.operations.state.mapped_pointer_nonnull.empty() &&
          draft.post_operations.detached_effect_complete &&
          draft.post_operations.state.plot_indices.empty() &&
          draft.post_operations.state.ever_conquered_by_words ==
              std::array<std::uint32_t,4>{0,0,0,0} &&
          !draft.post_operations.archive_clear_delta_verified &&
          std::get<std::vector<std::int32_t>>(member->value) ==
              std::vector<std::int32_t>{2,3,5} &&
          initial.policies == original.policies,
          "eight service effects must apply only to detached owner");

    auto malformed = original;
    malformed.diplomacy.null_written_aliases.pop_back();
    const auto rejected = project_bnw_fresh_player_uninit_prefix(
        objects,"selected-sha",std::move(malformed));
    check(!rejected.detached_effects_complete &&
          rejected.blocker ==
              BnwFreshPlayerUninitPrefixBlocker::service_projection_invalid &&
          rejected.blocked_at == "m_pDiplomacyAI->Uninit" &&
          rejected.source_order.empty() &&
          !rejected.policies.member_effect_complete,
          "failed downstream projection must not publish partial prefix");
    malformed = original;
    malformed.espionage.max_tech_cost.pop_back();
    const auto rejected_espionage = project_bnw_fresh_player_uninit_prefix(
        objects,"selected-sha",std::move(malformed));
    check(!rejected_espionage.detached_effects_complete &&
          rejected_espionage.blocker ==
              BnwFreshPlayerUninitPrefixBlocker::service_projection_invalid &&
          rejected_espionage.blocked_at == "m_pEspionage->Uninit" &&
          rejected_espionage.source_order.empty(),
          "invalid Espionage extent must not publish earlier service effects");
    malformed = original;
    malformed.espionage_ai.spies_killed = {1,2,3,4};
    const auto rejected_espionage_ai = project_bnw_fresh_player_uninit_prefix(
        objects,"selected-sha",std::move(malformed));
    check(!rejected_espionage_ai.detached_effects_complete &&
          rejected_espionage_ai.blocker ==
              BnwFreshPlayerUninitPrefixBlocker::service_projection_invalid &&
          rejected_espionage_ai.blocked_at == "m_pEspionageAI->Uninit" &&
          rejected_espionage_ai.source_order.empty(),
          "invalid EspionageAI extent must not publish earlier service effects");
    malformed = original;
    malformed.trade.plundered_connections.resize(11);
    const auto rejected_trade = project_bnw_fresh_player_uninit_prefix(
        objects,"selected-sha",std::move(malformed));
    check(!rejected_trade.detached_effects_complete &&
          rejected_trade.blocker ==
              BnwFreshPlayerUninitPrefixBlocker::service_projection_invalid &&
          rejected_trade.blocked_at == "m_pTrade->Uninit" &&
          rejected_trade.source_order.empty(),
          "invalid Trade capacity must not publish earlier service effects");
    malformed = original;
    malformed.league_ai.vote_commitments.resize(5);
    const auto rejected_league = project_bnw_fresh_player_uninit_prefix(
        objects,"selected-sha",std::move(malformed));
    check(!rejected_league.detached_effects_complete &&
          rejected_league.blocker ==
              BnwFreshPlayerUninitPrefixBlocker::service_projection_invalid &&
          rejected_league.blocked_at == "m_pLeagueAI->Uninit" &&
          rejected_league.source_order.empty(),
          "invalid LeagueAI capacity must not publish earlier service effects");
    malformed = original;
    malformed.cities = make_bnw_fresh_initialized_free_list();
    malformed.cities.nodes[0].occupied = true;
    const auto rejected_city = project_bnw_fresh_player_uninit_prefix(
        objects,"selected-sha",std::move(malformed));
    check(!rejected_city.detached_effects_complete &&
          rejected_city.blocker ==
              BnwFreshPlayerUninitPrefixBlocker::entities_container_uninit_invalid &&
          rejected_city.blocked_at == "m_cities.Uninit" &&
          rejected_city.source_order.empty(),
          "occupied city must fail before publishing aggregate effects");
    malformed = original;
    malformed.units = make_bnw_fresh_initialized_free_list();
    malformed.units.nodes[0].occupied = true;
    const auto rejected_unit = project_bnw_fresh_player_uninit_prefix(
        objects,"selected-sha",std::move(malformed));
    check(!rejected_unit.detached_effects_complete &&
          rejected_unit.blocker ==
              BnwFreshPlayerUninitPrefixBlocker::entities_container_uninit_invalid &&
          rejected_unit.blocked_at == "m_units.Uninit" &&
          rejected_unit.source_order.empty(),
          "occupied unit must fail before publishing earlier city effects");
    malformed = original;
    malformed.operations.mapped_pointer_nonnull.emplace(4,true);
    const auto rejected_operation = project_bnw_fresh_player_uninit_prefix(
        objects,"selected-sha",std::move(malformed));
    check(!rejected_operation.detached_effects_complete &&
          rejected_operation.blocker ==
              BnwFreshPlayerUninitPrefixBlocker::ai_operations_uninit_invalid &&
          rejected_operation.blocked_at == "delete(iter->second)" &&
          rejected_operation.source_order.empty(),
          "live operation must block whole detached aggregate");
    malformed = original;
    malformed.post_operations.plot_indices.resize(
        BnwFreshPlayerPostOperationsState::kPlotsInitialCapacity + 1);
    const auto rejected_plots = project_bnw_fresh_player_uninit_prefix(
        objects,"selected-sha",std::move(malformed));
    check(!rejected_plots.detached_effects_complete &&
          rejected_plots.blocker ==
              BnwFreshPlayerUninitPrefixBlocker::post_operations_invalid &&
          rejected_plots.blocked_at == "m_aiPlots.clear" &&
          rejected_plots.source_order.empty(),
          "oversized plot vector must not publish earlier effects");
    const auto wrong_sha = project_bnw_fresh_player_uninit_prefix(
        objects,"wrong-sha",original);
    check(!wrong_sha.detached_effects_complete &&
          wrong_sha.blocker ==
              BnwFreshPlayerUninitPrefixBlocker::archive_projection_invalid &&
          wrong_sha.source_order.empty(),
          "selected rules mismatch must fail before service effects");
    draft.rollback();
    check(!draft.detached_effects_complete && draft.source_order.empty() &&
          draft.selected_rules_sha256.empty() &&
          !draft.policies.member_effect_complete &&
          !draft.religion_ai.member_effect_complete &&
          !draft.espionage.member_effect_complete &&
          !draft.espionage_ai.member_effect_complete &&
          !draft.trade.member_effect_complete &&
          !draft.trade_ai.member_effect_complete &&
          !draft.league_ai.member_effect_complete &&
          !draft.player_techs.member_effect_complete &&
          !draft.flavor_manager.member_effect_complete &&
          !draft.tactical_ai.member_effect_complete &&
          !draft.mid_ai.member_effect_complete &&
          !draft.post_connections.member_effect_complete &&
          !draft.tail_containers.detached_container_effects_complete &&
          !draft.entities.detached_effects_complete &&
          !draft.operations.detached_effect_complete &&
          !draft.post_operations.detached_effect_complete &&
          !draft.archive.all_registered_member_defaults,
          "rollback must discard entire detached chain");
}
