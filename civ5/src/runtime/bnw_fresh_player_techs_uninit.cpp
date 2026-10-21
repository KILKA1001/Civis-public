#include "civ5/runtime/bnw_fresh_player_techs_uninit.hpp"
#include <utility>

namespace civ5::runtime {

void BnwFreshPlayerTechsUninitDraft::rollback() noexcept {
    state = {};
    source_order.clear();
    selected_rules_sha256.clear();
    member_effect_complete = false;
    archive_predecessor_verified = false;
    remaining_services_complete = false;
    initialized_by_gamecore = false;
}

BnwFreshPlayerTechsServiceState make_bnw_fresh_player_techs_constructor_state() {
    return {};
}

BnwFreshPlayerTechsUninitDraft project_bnw_fresh_player_techs_uninit(
    const BnwFreshFirstPlayerObjects &objects,
    const BnwFreshFullArchiveProjection &archive,
    const BnwFreshLeagueAiUninitDraft &league_ai,
    std::string_view selected_rules_sha256,
    BnwFreshPlayerTechsServiceState initial) {
    BnwFreshPlayerTechsUninitDraft result;
    if (!objects.constructors_complete || objects.initialized_by_gamecore ||
        objects.player_id != 0 || selected_rules_sha256.empty() ||
        objects.selected_rules_sha256 != selected_rules_sha256 ||
        archive.selected_rules_sha256 != selected_rules_sha256 ||
        !archive.all_registered_member_defaults ||
        archive.aggregate_traversal_verified || archive.initialized_by_gamecore ||
        league_ai.selected_rules_sha256 != selected_rules_sha256 ||
        !league_ai.member_effect_complete ||
        league_ai.archive_predecessor_verified ||
        league_ai.remaining_services_complete ||
        league_ai.initialized_by_gamecore)
        return result;

    result.selected_rules_sha256 = std::string(selected_rules_sha256);
    result.state = std::move(initial);
    // CvFlavorRecipient::Uninit first, then seven SAFE_DELETE_ARRAY calls and
    // finally SAFE_DELETE(m_pTechAI). None clears m_pTechs or m_pPlayer.
    result.state.latest_flavor_values.reset();
    result.source_order.push_back("CvFlavorRecipient::Uninit");
    result.state.researching_tech.reset();
    result.source_order.push_back("m_pabResearchingTech");
    result.state.civ_tech_priority.reset();
    result.source_order.push_back("m_piCivTechPriority");
    result.state.locale_tech_priority.reset();
    result.source_order.push_back("m_piLocaleTechPriority");
    result.state.locale_tech_resources.reset();
    result.source_order.push_back("m_peLocaleTechResources");
    result.state.civ_tech_unique_buildings.reset();
    result.source_order.push_back("m_peCivTechUniqueBuildings");
    result.state.civ_tech_unique_units.reset();
    result.source_order.push_back("m_peCivTechUniqueUnits");
    result.state.civ_tech_unique_improvements.reset();
    result.source_order.push_back("m_peCivTechUniqueImprovements");
    result.state.tech_ai_weights.reset();
    result.source_order.push_back("m_pTechAI");
    result.member_effect_complete = true;
    return result;
}

} // namespace civ5::runtime
