#include "civ5/runtime/bnw_team_tech_reward_ui.hpp"
#include "civ5/ui/selected_bnw_preview_host.hpp"
#include <filesystem>
#include <stdexcept>
#include <string_view>
#include <utility>

using namespace civ5::runtime;
namespace {
void check(bool value,const char *message) {
    if(!value) throw std::runtime_error(message);
}
}
int main(int argc,char **argv) {
    check(argc==3,"Expected selected BNW save and game root");
    auto bootstrap=civ5::ui::bootstrap_selected_bnw_preview(
        std::filesystem::path(argv[1]),std::filesystem::path(argv[2]),0);
    if(bootstrap.preview.blocker)
        throw std::runtime_error(bootstrap.preview.blocker->reason);
    check(bootstrap.profile.can_activate &&
          bootstrap.profile.enabled_dlc.size()==14 &&
          bootstrap.profile.enabled_mods.empty(),
          "Selected Gandhi profile changed");
    const auto &state=bootstrap.preview.host->session().state();
    BnwTechRewardUiInput selected;
    selected.team=63;
    selected.technology=0;
    const auto active=state.world.pregame->archive.integers.find("active_player");
    check(active!=state.world.pregame->archive.integers.end() &&
          state.setup.players.contains(active->second),
          "Selected active player unavailable");
    selected.active_team=state.setup.players.at(active->second).team;
    for(const auto &flag:state.world.game_state.flags)
        if(flag.name && std::string_view(flag.name)=="final_initialized")
            selected.final_initialized=flag.value;
    check(selected.final_initialized && selected.active_team!=63,
          "Gandhi final-initialized or active-team state changed");
    selected.new_value=true;
    for(std::int64_t player=0;player<22;++player) {
        const auto &slot=state.setup.players.at(player);
        selected.major_players_in_slot_order.push_back(
            {player,slot.team,slot.alive,false});
        check(!slot.alive || slot.team!=63,
              "Gandhi barbarian team acquired a major player");
    }
    // Gandhi barbarian team has no major player recipient. The two UI
    // outcomes are still gated by the mandatory native option query.
    auto missing=plan_bnw_team_tech_reward_ui(selected);
    check(missing.status==BnwTechRewardUiStatus::blocked &&
          missing.native_option_query_required && missing.intents.empty(),
          "Gandhi reward option was silently skipped");
    selected.observed_no_reward_popups=false;
    auto popup_option=plan_bnw_team_tech_reward_ui(selected);
    check(popup_option.status==BnwTechRewardUiStatus::planned &&
          popup_option.intents.empty() && !popup_option.effects_executed &&
          !popup_option.engine_callback_confirmed,
          "Inactive barbarian team fabricated a popup");
    selected.observed_no_reward_popups=true;
    auto notification_option=plan_bnw_team_tech_reward_ui(selected);
    check(notification_option.status==BnwTechRewardUiStatus::planned &&
          notification_option.intents.empty(),
          "Barbarian team fabricated major-player notification");

    BnwTechRewardUiInput synthetic=selected;
    synthetic.team=0;
    synthetic.active_team=0;
    for(auto &slot:synthetic.major_players_in_slot_order)
        slot={slot.id,-1,false,false};
    synthetic.major_players_in_slot_order[0]={0,0,true,true};
    synthetic.major_players_in_slot_order[1]={1,0,false,true};
    synthetic.major_players_in_slot_order[2]={2,1,true,true};
    synthetic.major_players_in_slot_order[3]={3,0,true,false};
    auto notifications=plan_bnw_team_tech_reward_ui(synthetic);
    check(notifications.status==BnwTechRewardUiStatus::planned &&
          notifications.intents.size()==1 &&
          notifications.intents[0].kind==BnwTechRewardUiKind::notification &&
          notifications.intents[0].player==0,
          "Notification recipient loop differs from SDK");
    auto reordered=synthetic;
    std::swap(reordered.major_players_in_slot_order[0],
              reordered.major_players_in_slot_order[1]);
    check(plan_bnw_team_tech_reward_ui(reordered).status==
              BnwTechRewardUiStatus::blocked,
          "Out-of-order major player slots were accepted");
    synthetic.observed_no_reward_popups=false;
    auto unknown_network=plan_bnw_team_tech_reward_ui(synthetic);
    check(unknown_network.status==BnwTechRewardUiStatus::blocked &&
          unknown_network.intents.empty(),
          "Ambiguous network mode was treated as an observed UI effect");
    synthetic.network_multiplayer=false;
    auto popup=plan_bnw_team_tech_reward_ui(synthetic);
    check(popup.status==BnwTechRewardUiStatus::planned &&
          popup.intents.size()==1 &&
          popup.intents[0].kind==BnwTechRewardUiKind::popup,
          "Single-player active team did not stage the award popup");
    synthetic.network_multiplayer=true;
    auto network_notification=plan_bnw_team_tech_reward_ui(synthetic);
    check(network_notification.intents.size()==1 &&
          network_notification.intents[0].kind==
              BnwTechRewardUiKind::notification,
          "Network tech reward did not use notification branch");
}
