#include "civ5/runtime/bnw_fresh_player_uninit_prefix.hpp"
#include <algorithm>
#include <array>
#include <limits>
#include <utility>

namespace civ5::runtime {
namespace {

BnwFreshPlayerUninitPrefixDraft failed(
    BnwFreshPlayerUninitPrefixBlocker code, std::string_view at) {
    BnwFreshPlayerUninitPrefixDraft result;
    result.blocker = code;
    result.blocked_at = at;
    return result;
}

bool clear_detached_vector(std::vector<BnwArchiveMemberState> &members,
                           std::string_view name) {
    const auto it = std::find_if(members.begin(), members.end(),
        [name](const BnwArchiveMemberState &member) {
            return member.name == name;
        });
    if (it == members.end() || it->pending_vector_commands ==
        std::numeric_limits<std::uint32_t>::max()) return false;
    switch (it->kind) {
    case BnwArchiveMemberKind::int_vector:
        if (auto *value = std::get_if<std::vector<std::int32_t>>(&it->value)) {
            ++it->pending_vector_commands;
            value->clear();
            return true;
        }
        break;
    case BnwArchiveMemberKind::bool_vector:
        if (auto *value = std::get_if<std::vector<bool>>(&it->value)) {
            ++it->pending_vector_commands;
            value->clear();
            return true;
        }
        break;
    case BnwArchiveMemberKind::options_vector:
        if (auto *value = std::get_if<
                std::vector<std::pair<std::uint32_t,std::int32_t>>>(&it->value)) {
            ++it->pending_vector_commands;
            value->clear();
            return true;
        }
        break;
    case BnwArchiveMemberKind::yield_array_vector:
        if (auto *value = std::get_if<
                std::vector<std::array<std::int32_t,6>>>(&it->value)) {
            ++it->pending_vector_commands;
            value->clear();
            return true;
        }
        break;
    default: break;
    }
    return false;
}

} // namespace

void BnwFreshPlayerUninitPrefixDraft::rollback() noexcept {
    archive.rollback();
    policies.rollback();
    economic.rollback();
    military.rollback();
    city_specialization.rollback();
    wonder.rollback();
    grand_strategy.rollback();
    diplomacy.rollback();
    religions.rollback();
    religion_ai.rollback();
    espionage.rollback();
    espionage_ai.rollback();
    trade.rollback();
    trade_ai.rollback();
    league_ai.rollback();
    player_techs.rollback();
    flavor_manager.rollback();
    tactical_ai.rollback();
    mid_ai.rollback();
    post_connections.rollback();
    tail_containers.rollback();
    entities.rollback();
    operations.rollback();
    post_operations.rollback();
    source_order.clear();
    selected_rules_sha256.clear();
    blocked_at = {};
    unresolved.clear();
    blocker = BnwFreshPlayerUninitPrefixBlocker::none;
    detached_effects_complete = false;
    archive_traversal_verified = false;
    uninit_complete = false;
    initialized_by_gamecore = false;
}

std::optional<BnwFreshPlayerUninitPrefixInput>
make_bnw_fresh_player_uninit_prefix_input(
    std::span<const content::GameplayDefine> native_constants) {
    auto espionage = make_bnw_fresh_espionage_constructor_state(native_constants);
    if (!espionage) return std::nullopt;
    BnwFreshPlayerUninitPrefixInput result;
    result.native_constants.assign(native_constants.begin(),native_constants.end());
    result.archive_members = make_bnw_fresh_archive_defaults();
    result.policies = make_bnw_fresh_policies_constructor_state();
    result.economic = make_bnw_fresh_economic_constructor_state();
    result.military = make_bnw_fresh_military_constructor_state();
    result.city_specialization =
        make_bnw_fresh_city_specialization_constructor_state();
    result.wonder = make_bnw_fresh_wonder_constructor_state();
    result.grand_strategy = make_bnw_fresh_grand_strategy_constructor_state();
    result.diplomacy = make_bnw_fresh_diplomacy_constructor_state();
    result.religions = make_bnw_fresh_religions_constructor_state();
    result.religion_ai = make_bnw_fresh_religion_ai_constructor_state();
    result.espionage = std::move(*espionage);
    result.espionage_ai = make_bnw_fresh_espionage_ai_constructor_state();
    result.trade = make_bnw_fresh_trade_constructor_state();
    result.trade_ai = make_bnw_fresh_trade_ai_constructor_state();
    result.league_ai = make_bnw_fresh_league_ai_constructor_state();
    result.player_techs = make_bnw_fresh_player_techs_constructor_state();
    result.flavor_manager = make_bnw_fresh_flavor_manager_constructor_state();
    result.tactical_ai = make_bnw_fresh_tactical_ai_constructor_state();
    result.mid_ai = make_bnw_fresh_mid_ai_constructor_state();
    result.post_connections =
        make_bnw_fresh_post_connections_constructor_state();
    result.tail_containers = {};
    result.cities = make_bnw_fresh_unallocated_free_list();
    result.units = make_bnw_fresh_unallocated_free_list();
    result.operations = {};
    result.post_operations = {};
    return result;
}

BnwFreshPlayerUninitPrefixDraft project_bnw_fresh_player_uninit_prefix(
    const BnwFreshFirstPlayerObjects &objects,
    std::string_view selected_rules_sha256,
    BnwFreshPlayerUninitPrefixInput initial) {
    BnwFreshPlayerUninitPrefixDraft result;
    result.archive = project_bnw_fresh_full_archive_reset(
        objects,selected_rules_sha256,std::move(initial.archive_members));
    if (!result.archive.all_registered_member_defaults ||
        result.archive.aggregate_traversal_verified)
        return failed(BnwFreshPlayerUninitPrefixBlocker::archive_projection_invalid,
                      "m_syncArchive.reset");
    result.selected_rules_sha256 = std::string(selected_rules_sha256);
    result.source_order.push_back({
        BnwFreshPlayerUninitPrefixEventKind::archive_reset_projection,
        "m_syncArchive.reset",false});

    // Expansion2 CvPlayer.cpp:644-665, sourced from the generated catalog.
    // The full-archive value reset has already made these vectors empty, but
    // each explicit clear is still applied to the detached owner in order.
    std::size_t early_count = 0;
    for (const auto &write : kBnwPlayerUninitVectorClears) {
        if (write.after_service_callbacks) break;
        if (!clear_detached_vector(result.archive.defaults,write.name))
            return failed(BnwFreshPlayerUninitPrefixBlocker::early_vector_clear_invalid,
                          write.name);
        result.source_order.push_back({
            BnwFreshPlayerUninitPrefixEventKind::vector_clear,
            write.name,true});
        ++early_count;
    }
    if (early_count != 21)
        return failed(BnwFreshPlayerUninitPrefixBlocker::early_vector_clear_invalid,
                      "CvPlayer::uninit early vectors");

    // Every projector mutates only its own detached service owner. Return an
    // empty failure draft on any missing predecessor; no partial owner escapes.
    result.policies = project_bnw_fresh_policies_uninit(
        objects,result.archive,selected_rules_sha256,std::move(initial.policies));
    if (!result.policies.member_effect_complete)
        return failed(BnwFreshPlayerUninitPrefixBlocker::service_projection_invalid,
                      "m_pPlayerPolicies->Uninit");
    result.source_order.push_back({
        BnwFreshPlayerUninitPrefixEventKind::service_uninit,
        "m_pPlayerPolicies->Uninit",true});

    result.economic = project_bnw_fresh_economic_uninit(
        objects,result.archive,result.policies,selected_rules_sha256,
        std::move(initial.economic));
    if (!result.economic.member_effect_complete)
        return failed(BnwFreshPlayerUninitPrefixBlocker::service_projection_invalid,
                      "m_pEconomicAI->Uninit");
    result.source_order.push_back({
        BnwFreshPlayerUninitPrefixEventKind::service_uninit,
        "m_pEconomicAI->Uninit",true});

    result.military = project_bnw_fresh_military_uninit(
        objects,result.archive,result.policies,result.economic,
        selected_rules_sha256,std::move(initial.military));
    if (!result.military.member_effect_complete)
        return failed(BnwFreshPlayerUninitPrefixBlocker::service_projection_invalid,
                      "m_pMilitaryAI->Uninit");
    result.source_order.push_back({
        BnwFreshPlayerUninitPrefixEventKind::service_uninit,
        "m_pMilitaryAI->Uninit",true});

    result.city_specialization = project_bnw_fresh_city_specialization_uninit(
        objects,result.archive,result.policies,result.economic,result.military,
        selected_rules_sha256,std::move(initial.city_specialization));
    if (!result.city_specialization.member_effect_complete)
        return failed(BnwFreshPlayerUninitPrefixBlocker::service_projection_invalid,
                      "m_pCitySpecializationAI->Uninit");
    result.source_order.push_back({
        BnwFreshPlayerUninitPrefixEventKind::service_uninit,
        "m_pCitySpecializationAI->Uninit",true});

    result.wonder = project_bnw_fresh_wonder_uninit(
        objects,result.archive,result.policies,result.economic,result.military,
        result.city_specialization,selected_rules_sha256,std::move(initial.wonder));
    if (!result.wonder.member_effect_complete)
        return failed(BnwFreshPlayerUninitPrefixBlocker::service_projection_invalid,
                      "m_pWonderProductionAI->Uninit");
    result.source_order.push_back({
        BnwFreshPlayerUninitPrefixEventKind::service_uninit,
        "m_pWonderProductionAI->Uninit",true});

    result.grand_strategy = project_bnw_fresh_grand_strategy_uninit(
        objects,result.archive,result.policies,result.economic,result.military,
        result.city_specialization,result.wonder,selected_rules_sha256,
        std::move(initial.grand_strategy));
    if (!result.grand_strategy.member_effect_complete)
        return failed(BnwFreshPlayerUninitPrefixBlocker::service_projection_invalid,
                      "m_pGrandStrategyAI->Uninit");
    result.source_order.push_back({
        BnwFreshPlayerUninitPrefixEventKind::service_uninit,
        "m_pGrandStrategyAI->Uninit",true});

    result.diplomacy = project_bnw_fresh_diplomacy_uninit(
        objects,result.archive,result.policies,result.economic,result.military,
        result.city_specialization,result.wonder,result.grand_strategy,
        selected_rules_sha256,std::move(initial.diplomacy));
    if (!result.diplomacy.member_effect_complete)
        return failed(BnwFreshPlayerUninitPrefixBlocker::service_projection_invalid,
                      "m_pDiplomacyAI->Uninit");
    result.source_order.push_back({
        BnwFreshPlayerUninitPrefixEventKind::service_uninit,
        "m_pDiplomacyAI->Uninit",true});

    result.religions = project_bnw_fresh_religions_uninit(
        objects,result.archive,result.policies,result.economic,result.military,
        result.city_specialization,result.wonder,result.grand_strategy,
        result.diplomacy,selected_rules_sha256,std::move(initial.religions));
    if (!result.religions.member_effect_complete)
        return failed(BnwFreshPlayerUninitPrefixBlocker::service_projection_invalid,
                      "m_pReligions->Uninit");
    result.source_order.push_back({
        BnwFreshPlayerUninitPrefixEventKind::service_uninit,
        "m_pReligions->Uninit",true});

    result.religion_ai = project_bnw_fresh_religion_ai_uninit(
        objects,result.archive,result.policies,result.economic,result.military,
        result.city_specialization,result.wonder,result.grand_strategy,
        result.diplomacy,result.religions,selected_rules_sha256,
        std::move(initial.religion_ai));
    if (!result.religion_ai.member_effect_complete)
        return failed(BnwFreshPlayerUninitPrefixBlocker::service_projection_invalid,
                      "m_pReligionAI->Uninit");
    result.source_order.push_back({
        BnwFreshPlayerUninitPrefixEventKind::service_uninit,
        "m_pReligionAI->Uninit",true});

    result.espionage = project_bnw_fresh_espionage_uninit(
        objects,result.archive,result.policies,result.economic,result.military,
        result.city_specialization,result.wonder,result.grand_strategy,
        result.diplomacy,result.religions,result.religion_ai,
        selected_rules_sha256,initial.native_constants,
        std::move(initial.espionage));
    if (!result.espionage.member_effect_complete)
        return failed(BnwFreshPlayerUninitPrefixBlocker::service_projection_invalid,
                      "m_pEspionage->Uninit");
    result.source_order.push_back({
        BnwFreshPlayerUninitPrefixEventKind::service_uninit,
        "m_pEspionage->Uninit",true});

    result.espionage_ai = project_bnw_fresh_espionage_ai_uninit(
        objects,result.archive,result.policies,result.economic,result.military,
        result.city_specialization,result.wonder,result.grand_strategy,
        result.diplomacy,result.religions,result.religion_ai,
        result.espionage,selected_rules_sha256,initial.native_constants,
        std::move(initial.espionage_ai));
    if (!result.espionage_ai.member_effect_complete)
        return failed(BnwFreshPlayerUninitPrefixBlocker::service_projection_invalid,
                      "m_pEspionageAI->Uninit");
    result.source_order.push_back({
        BnwFreshPlayerUninitPrefixEventKind::service_uninit,
        "m_pEspionageAI->Uninit",true});

    result.trade = project_bnw_fresh_trade_uninit(
        objects,result.archive,result.policies,result.economic,result.military,
        result.city_specialization,result.wonder,result.grand_strategy,
        result.diplomacy,result.religions,result.religion_ai,
        result.espionage,result.espionage_ai,selected_rules_sha256,
        std::move(initial.trade));
    if (!result.trade.member_effect_complete)
        return failed(BnwFreshPlayerUninitPrefixBlocker::service_projection_invalid,
                      "m_pTrade->Uninit");
    result.source_order.push_back({
        BnwFreshPlayerUninitPrefixEventKind::service_uninit,
        "m_pTrade->Uninit",true});

    result.trade_ai = project_bnw_fresh_trade_ai_uninit(
        objects,result.archive,result.policies,result.economic,result.military,
        result.city_specialization,result.wonder,result.grand_strategy,
        result.diplomacy,result.religions,result.religion_ai,
        result.espionage,result.espionage_ai,result.trade,
        selected_rules_sha256,std::move(initial.trade_ai));
    if (!result.trade_ai.member_effect_complete)
        return failed(BnwFreshPlayerUninitPrefixBlocker::service_projection_invalid,
                      "m_pTradeAI->Uninit");
    result.source_order.push_back({
        BnwFreshPlayerUninitPrefixEventKind::service_uninit,
        "m_pTradeAI->Uninit",true});

    result.league_ai = project_bnw_fresh_league_ai_uninit(
        objects,result.archive,result.policies,result.economic,result.military,
        result.city_specialization,result.wonder,result.grand_strategy,
        result.diplomacy,result.religions,result.religion_ai,
        result.espionage,result.espionage_ai,result.trade,result.trade_ai,
        selected_rules_sha256,std::move(initial.league_ai));
    if (!result.league_ai.member_effect_complete)
        return failed(BnwFreshPlayerUninitPrefixBlocker::service_projection_invalid,
                      "m_pLeagueAI->Uninit");
    result.source_order.push_back({
        BnwFreshPlayerUninitPrefixEventKind::service_uninit,
        "m_pLeagueAI->Uninit",true});

    result.player_techs = project_bnw_fresh_player_techs_uninit(
        objects,result.archive,result.league_ai,selected_rules_sha256,
        std::move(initial.player_techs));
    if (!result.player_techs.member_effect_complete)
        return failed(BnwFreshPlayerUninitPrefixBlocker::service_projection_invalid,
                      "m_pPlayerTechs->Uninit");
    result.source_order.push_back({
        BnwFreshPlayerUninitPrefixEventKind::service_uninit,
        "m_pPlayerTechs->Uninit",true});

    result.flavor_manager = project_bnw_fresh_flavor_manager_uninit(
        objects,result.archive,result.player_techs,selected_rules_sha256,
        std::move(initial.flavor_manager));
    if (!result.flavor_manager.member_effect_complete)
        return failed(BnwFreshPlayerUninitPrefixBlocker::service_projection_invalid,
                      "m_pFlavorManager->Uninit");
    result.source_order.push_back({
        BnwFreshPlayerUninitPrefixEventKind::service_uninit,
        "m_pFlavorManager->Uninit",true});

    result.tactical_ai = project_bnw_fresh_tactical_ai_uninit(
        objects,result.archive,result.flavor_manager,
        selected_rules_sha256,std::move(initial.tactical_ai));
    if (!result.tactical_ai.member_effect_complete)
        return failed(BnwFreshPlayerUninitPrefixBlocker::service_projection_invalid,
                      "m_pTacticalAI->Uninit");
    result.source_order.push_back({
        BnwFreshPlayerUninitPrefixEventKind::service_uninit,
        "m_pTacticalAI->Uninit",true});

    result.mid_ai = project_bnw_fresh_mid_ai_uninit(
        objects,result.archive,result.tactical_ai,selected_rules_sha256,
        std::move(initial.mid_ai));
    if (!result.mid_ai.member_effect_complete)
        return failed(BnwFreshPlayerUninitPrefixBlocker::service_projection_invalid,
                      "m_pHomelandAI->Uninit through m_pCityConnections->Uninit");
    for (const auto name : {"m_pHomelandAI->Uninit",
                            "m_pMinorCivAI->Uninit",
                            "m_pDealAI->Uninit",
                            "m_pBuilderTaskingAI->Uninit",
                            "m_pCityConnections->Uninit"})
        result.source_order.push_back({
            BnwFreshPlayerUninitPrefixEventKind::service_uninit,name,true});

    result.post_connections = project_bnw_fresh_post_connections_uninit(
        objects,result.archive,result.mid_ai,selected_rules_sha256,
        std::move(initial.post_connections));
    if (!result.post_connections.member_effect_complete)
        return failed(BnwFreshPlayerUninitPrefixBlocker::service_projection_invalid,
                      "m_pNotifications through m_pDangerPlots->Uninit");
    if (result.post_connections.state.notifications)
        result.source_order.push_back({
            BnwFreshPlayerUninitPrefixEventKind::service_uninit,
            "m_pNotifications->Uninit",true});
    if (result.post_connections.state.diplomacy_requests)
        result.source_order.push_back({
            BnwFreshPlayerUninitPrefixEventKind::service_uninit,
            "m_pDiplomacyRequests->Uninit",true});
    result.source_order.push_back({
        BnwFreshPlayerUninitPrefixEventKind::service_uninit,
        "m_pTreasury->Uninit",true});
    result.source_order.push_back({
        BnwFreshPlayerUninitPrefixEventKind::service_uninit,
        "m_pTraits->Uninit",true});
    if (result.post_connections.state.danger_plots)
        result.source_order.push_back({
            BnwFreshPlayerUninitPrefixEventKind::service_uninit,
            "m_pDangerPlots->Uninit",true});

    result.tail_containers = project_bnw_fresh_player_tail_containers(
        objects,result.archive,result.post_connections,
        selected_rules_sha256,std::move(initial.tail_containers));
    if (!result.tail_containers.detached_container_effects_complete)
        return failed(BnwFreshPlayerUninitPrefixBlocker::service_projection_invalid,
                      "m_ppaaiSpecialistExtraYield.clear through m_cityNames.clear");
    result.archive = result.tail_containers.archive;
    for (const auto name : {"m_ppaaiSpecialistExtraYield",
                            "m_ppaaiImprovementYieldChange",
                            "m_ppaaiBuildingClassYieldMod"})
        result.source_order.push_back({
            BnwFreshPlayerUninitPrefixEventKind::vector_clear,name,false});
    for (const auto name : {"m_UnitCycle.Clear",
                            "m_researchQueue.clear",
                            "m_cityNames.clear"})
        result.source_order.push_back({
            BnwFreshPlayerUninitPrefixEventKind::container_clear,name,true});

    result.entities = project_bnw_fresh_player_entities_uninit(
        objects,result.tail_containers,selected_rules_sha256,
        std::move(initial.cities),std::move(initial.units));
    if (!result.entities.detached_effects_complete)
        return failed(BnwFreshPlayerUninitPrefixBlocker::entities_container_uninit_invalid,
                      result.entities.blocked_at);
    for (const auto &event : result.entities.source_order)
        result.source_order.push_back({
            BnwFreshPlayerUninitPrefixEventKind::container_clear,
            event.sdk_call,true});

    result.operations = project_bnw_fresh_player_operations_uninit(
        objects,result.entities,selected_rules_sha256,
        std::move(initial.operations));
    if (!result.operations.detached_effect_complete)
        return failed(BnwFreshPlayerUninitPrefixBlocker::ai_operations_uninit_invalid,
                      result.operations.blocked_at);
    for (const auto &event : result.operations.source_order)
        result.source_order.push_back({
            BnwFreshPlayerUninitPrefixEventKind::container_clear,
            event.kind == BnwFreshPlayerOperationsEventKind::map_clear
                ? "m_AIOperations.clear" : "delete(iter->second)",true});

    result.post_operations = project_bnw_fresh_player_post_operations(
        objects,result.operations,selected_rules_sha256,
        std::move(initial.post_operations));
    if (!result.post_operations.detached_effect_complete)
        return failed(BnwFreshPlayerUninitPrefixBlocker::post_operations_invalid,
                      result.post_operations.blocked_at);
    for (const auto &event : result.post_operations.source_order)
        result.source_order.push_back({
            BnwFreshPlayerUninitPrefixEventKind::container_clear,
            event.sdk_call,true});

    result.detached_effects_complete = true;
    result.blocked_at = "archive.clearDelta";
    result.blocker = BnwFreshPlayerUninitPrefixBlocker::archive_reset_traversal_pending;
    result.unresolved = {
        BnwFreshPlayerUninitPrefixBlocker::archive_reset_traversal_pending,
        BnwFreshPlayerUninitPrefixBlocker::archive_clear_delta_pending};
    return result;
}

} // namespace civ5::runtime
