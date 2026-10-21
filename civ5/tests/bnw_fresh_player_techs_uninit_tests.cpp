#include "civ5/runtime/bnw_fresh_player_uninit_prefix.hpp"
#include "civ5/runtime/bnw_fresh_player_techs_uninit.hpp"
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
          prefix.league_ai.member_effect_complete &&
          !prefix.archive_traversal_verified,
          "source-ordered LeagueAI predecessor required");

    const auto fresh = make_bnw_fresh_player_techs_constructor_state();
    check(!fresh.tech_entries_bound && !fresh.player_bound &&
          !fresh.latest_flavor_values && !fresh.tech_ai_weights,
          "CvPlayerTechs constructor pointers must start null");
    auto draft = project_bnw_fresh_player_techs_uninit(
        objects,prefix.archive,prefix.league_ai,"selected-sha",fresh);
    check(draft.member_effect_complete && draft.state == fresh &&
          draft.source_order.size() == 9 &&
          draft.source_order.front() == "CvFlavorRecipient::Uninit" &&
          draft.source_order.back() == "m_pTechAI" &&
          !draft.archive_predecessor_verified &&
          !draft.remaining_services_complete &&
          !draft.initialized_by_gamecore,
          "fresh Uninit must preserve null state and readiness blockers");

    BnwFreshPlayerTechsServiceState populated;
    populated.is_city = true;
    populated.tech_entries_bound = true;
    populated.player_bound = true;
    populated.latest_flavor_values = std::vector<std::int32_t>{1,2};
    populated.researching_tech = std::vector<bool>{true,false};
    populated.civ_tech_priority = std::vector<std::int32_t>{2,3};
    populated.locale_tech_priority = std::vector<std::int32_t>{4,5};
    populated.locale_tech_resources = std::vector<std::int32_t>{-1,0};
    populated.civ_tech_unique_buildings = std::vector<std::int32_t>{-1,1};
    populated.civ_tech_unique_units = std::vector<std::int32_t>{-1,2};
    populated.civ_tech_unique_improvements = std::vector<std::int32_t>{-1,3};
    populated.tech_ai_weights = std::vector<std::pair<std::int32_t,std::int32_t>>{{0,7}};
    draft = project_bnw_fresh_player_techs_uninit(
        objects,prefix.archive,prefix.league_ai,"selected-sha",populated);
    check(draft.member_effect_complete && draft.state.is_city == populated.is_city &&
          draft.state.tech_entries_bound && draft.state.player_bound &&
          !draft.state.latest_flavor_values && !draft.state.researching_tech &&
          !draft.state.civ_tech_priority && !draft.state.locale_tech_priority &&
          !draft.state.locale_tech_resources &&
          !draft.state.civ_tech_unique_buildings &&
          !draft.state.civ_tech_unique_units &&
          !draft.state.civ_tech_unique_improvements &&
          !draft.state.tech_ai_weights,
          "populated Uninit must delete exactly eight arrays and TechAI");
    check(draft.source_order == std::vector<std::string_view>{
          "CvFlavorRecipient::Uninit","m_pabResearchingTech",
          "m_piCivTechPriority","m_piLocaleTechPriority",
          "m_peLocaleTechResources","m_peCivTechUniqueBuildings",
          "m_peCivTechUniqueUnits","m_peCivTechUniqueImprovements",
          "m_pTechAI"},"SDK delete order changed");

    auto bad_league = prefix.league_ai;
    bad_league.member_effect_complete = false;
    const auto refused = project_bnw_fresh_player_techs_uninit(
        objects,prefix.archive,bad_league,"selected-sha",populated);
    check(!refused.member_effect_complete && refused.source_order.empty(),
          "missing LeagueAI predecessor must fail closed");
    const auto wrong = project_bnw_fresh_player_techs_uninit(
        objects,prefix.archive,prefix.league_ai,"wrong-sha",populated);
    check(!wrong.member_effect_complete && wrong.source_order.empty(),
          "rules mismatch must fail closed");
    draft.rollback();
    check(!draft.member_effect_complete && draft.source_order.empty() &&
          draft.selected_rules_sha256.empty(),
          "rollback must discard detached projection");
}
