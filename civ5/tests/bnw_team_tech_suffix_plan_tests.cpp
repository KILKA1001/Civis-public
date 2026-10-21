#include "civ5/runtime/bnw_team_tech_suffix_plan.hpp"
#include "civ5/ui/selected_bnw_preview_host.hpp"
#include "civ5/content/info_types.hpp"
#include <algorithm>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string_view>

using namespace civ5;
using namespace civ5::runtime;
namespace {
void check(bool ok,const char *message) {
    if(!ok) throw std::runtime_error(message);
}
template<class T> T &named(std::vector<T> &items,std::string_view name) {
    for(auto &item:items)
        if(item.name && std::string_view(item.name)==name) return item;
    throw std::runtime_error("Missing test record");
}
void prepare(LiveSessionState &state,const BnwTeamTechLuaAttempt &quote,
             const ui::SelectedBnwPreviewHost &host,
             const content::InfoTypeCatalog &types) {
    state.technologies[63].set_has_tech(0,true);
    auto &team=state.world.teams->teams[63];
    auto &hashes=named(team.arrays,"tech_hashes").values;
    auto &flags=named(team.arrays,"has_tech").values;
    std::size_t matches=0;
    for(std::size_t i=0;i<hashes.size();++i)
        if(hashes[i]==content::saved_info_type_hash("TECH_AGRICULTURE")) {
            flags[i]=1;
            ++matches;
        }
    check(matches==1,"Agriculture raw team hash is not unique");
    // A fixture quote tests this suffix only; the real Lua owner still blocks
    // publishing the selected Agriculture acquisition.
    state.setup.extensions.emplace<BnwTeamTechLuaAttemptMarker>(
        BnwTeamTechLuaAttemptMarker{63,0,true,false,true});
    check(run_bnw_team_tech_best_route(state,quote,host.technologies(),
          host.database(),types,63,0).applied,
          "Best-route predecessor unavailable");
    check(run_bnw_team_tech_player_research(state,quote,host.technologies(),
          host.database(),types,63,0).applied,
          "Player-loop predecessor unavailable");
    check(run_bnw_team_tech_share_culture(state,quote,host.technologies(),
          host.database(),types,63,0).applied,
          "Share/culture predecessor unavailable");
}
bool contains(const BnwTeamTechSuffixPlan &plan,BnwTeamTechSuffixStep step) {
    return std::find(plan.proven_no_effect.begin(),
                     plan.proven_no_effect.end(),step)!=
           plan.proven_no_effect.end();
}
}
int main(int argc,char **argv) {
    check(argc==3,"Expected selected BNW save and game root");
    auto bootstrap=ui::bootstrap_selected_bnw_preview(
        std::filesystem::path(argv[1]),std::filesystem::path(argv[2]),0);
    if(bootstrap.preview.blocker)
        throw std::runtime_error(bootstrap.preview.blocker->reason);
    check(bootstrap.profile.can_activate &&
          bootstrap.profile.enabled_dlc.size()==14 &&
          bootstrap.profile.enabled_mods.empty(),
          "Selected Gandhi profile changed");
    const auto &host=*bootstrap.preview.host;
    const auto types=host.database().cache_info_types();
    BnwTeamTechLuaAttempt quote;
    quote.status=BnwTeamTechLuaStatus::quoted;
    quote.hook_value=true;
    auto state=host.session().state();
    const auto missing=plan_bnw_team_tech_suffix(
        state,quote,host.technologies(),host.database(),types,63,0);
    check(!missing.grant_complete &&
          missing.first_pending==BnwTeamTechSuffixStep::announced_research_choice,
          "Suffix passed missing prior Lua/tech markers");
    prepare(state,quote,host,types);
    const auto selected=plan_bnw_team_tech_suffix(
        state,quote,host.technologies(),host.database(),types,63,0);
    check(!selected.grant_complete &&
          contains(selected,BnwTeamTechSuffixStep::announced_research_choice) &&
          contains(selected,BnwTeamTechSuffixStep::espionage_stealable_list) &&
          contains(selected,BnwTeamTechSuffixStep::major_player_happiness) &&
          contains(selected,BnwTeamTechSuffixStep::active_team_ui_dirty) &&
          selected.first_pending==BnwTeamTechSuffixStep::reward_popup_option &&
          selected.ui_intents.size()==1 &&
          selected.ui_intents[0].kind==
              BnwTeamTechSuffixUiKind::query_no_reward_popups &&
          selected.later_engine_intent &&
          selected.later_engine_intent->gameplay_tech_acquired &&
          selected.additional_checkpoint_sections.empty() &&
          !host.session().state().technologies[63].has_tech(0),
          "Selected Gandhi tech UI/engine suffix boundary changed");
    std::cout << "selected Gandhi suffix blocked at DLLUI::IsOptionNoRewardPopups; "
                 "GameplayTechAcquired remains mandatory\n";

    auto active=state;
    active.world.pregame->archive.integers["active_player"]=63;
    const auto dirty=plan_bnw_team_tech_suffix(
        active,quote,host.technologies(),host.database(),types,63,0);
    check(dirty.first_pending==BnwTeamTechSuffixStep::active_team_ui_dirty &&
          dirty.ui_intents.size()==3 &&
          dirty.ui_intents[0].kind==
              BnwTeamTechSuffixUiKind::dirty_misc_buttons &&
          dirty.ui_intents[1].kind==
              BnwTeamTechSuffixUiKind::dirty_selection_buttons &&
          dirty.ui_intents[2].kind==
              BnwTeamTechSuffixUiKind::dirty_research_buttons &&
          !dirty.grant_complete,
          "Active-team UI dirty callbacks were skipped or reordered");

    auto loading=state;
    auto &final=named(loading.world.game_state.flags,"final_initialized");
    final.value=false;
    const auto engine=plan_bnw_team_tech_suffix(
        loading,quote,host.technologies(),host.database(),types,63,0);
    check(engine.first_pending==BnwTeamTechSuffixStep::gameplay_tech_acquired &&
          contains(engine,BnwTeamTechSuffixStep::reward_popup_option) &&
          engine.ui_intents.empty() && engine.later_engine_intent &&
          !engine.grant_complete,
          "Non-finalized game silently skipped GameplayTechAcquired");

    auto other_leader=state;
    other_leader.setup.players.at(0).team=63;
    const auto espionage=plan_bnw_team_tech_suffix(
        other_leader,quote,host.technologies(),host.database(),types,63,0);
    check(espionage.first_pending==
              BnwTeamTechSuffixStep::espionage_stealable_list &&
          !contains(espionage,
                    BnwTeamTechSuffixStep::espionage_stealable_list) &&
          !espionage.grant_complete,
          "Non-barbarian getLeaderID skipped espionage rebuild");
}
