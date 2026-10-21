#include "civ5/runtime/bnw_fresh_mid_ai_uninit.hpp"
#include <utility>

namespace civ5::runtime {

void BnwFreshMidAiUninitDraft::rollback() noexcept {
    state = {};
    source_order.clear();
    selected_rules_sha256.clear();
    member_effect_complete = false;
    archive_predecessor_verified = false;
    remaining_services_complete = false;
    initialized_by_gamecore = false;
}

BnwFreshMidAiUninitInput make_bnw_fresh_mid_ai_constructor_state() {
    return {};
}

BnwFreshMidAiUninitDraft project_bnw_fresh_mid_ai_uninit(
    const BnwFreshFirstPlayerObjects &objects,
    const BnwFreshFullArchiveProjection &archive,
    const BnwFreshTacticalAiUninitDraft &tactical_ai,
    std::string_view selected_rules_sha256,
    BnwFreshMidAiUninitInput initial) {
    BnwFreshMidAiUninitDraft result;
    if (!objects.constructors_complete || objects.initialized_by_gamecore ||
        objects.player_id != 0 || selected_rules_sha256.empty() ||
        objects.selected_rules_sha256 != selected_rules_sha256 ||
        archive.selected_rules_sha256 != selected_rules_sha256 ||
        !archive.all_registered_member_defaults ||
        archive.aggregate_traversal_verified || archive.initialized_by_gamecore ||
        tactical_ai.selected_rules_sha256 != selected_rules_sha256 ||
        !tactical_ai.member_effect_complete ||
        tactical_ai.archive_predecessor_verified ||
        tactical_ai.remaining_services_complete ||
        tactical_ai.initialized_by_gamecore)
        return result;

    result.selected_rules_sha256 = std::string(selected_rules_sha256);
    result.state = std::move(initial);
    result.source_order.push_back("m_pHomelandAI->Uninit");
    result.source_order.push_back("m_pMinorCivAI->Uninit");
    result.source_order.push_back("m_pDealAI->Uninit");
    // CvBuilderTaskingAI.cpp:95-102. Static plot/directive caches and other
    // per-instance members are not cleared by this Uninit.
    result.state.builder_tasking_ai.repair_build = -1;
    result.source_order.push_back("BuilderTaskingAI::m_eRepairBuild");
    result.state.builder_tasking_ai.player_bound = false;
    result.source_order.push_back("BuilderTaskingAI::m_pPlayer");
    result.state.builder_tasking_ai.logging = false;
    result.source_order.push_back("BuilderTaskingAI::m_bLogging");
    result.state.builder_tasking_ai.num_cities = -1;
    result.source_order.push_back("BuilderTaskingAI::m_iNumCities");
    result.state.builder_tasking_ai.target_plot_bound = false;
    result.source_order.push_back("BuilderTaskingAI::m_pTargetPlot");
    // CvCityConnections.cpp:73-81 leaves the dimension and route caches in
    // place. The condition only changes whether deletion has work to do.
    result.state.city_connections.player_bound = false;
    result.source_order.push_back("CityConnections::m_pPlayer");
    if (result.state.city_connections.route_infos)
        result.state.city_connections.route_infos.reset();
    result.source_order.push_back("CityConnections::m_aRouteInfos");
    result.member_effect_complete = true;
    return result;
}

} // namespace civ5::runtime
