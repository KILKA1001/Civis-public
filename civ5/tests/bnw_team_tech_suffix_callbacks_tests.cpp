#include "civ5/runtime/bnw_team_tech_suffix_callbacks.hpp"
#include "civ5/ui/selected_bnw_preview_host.hpp"
#include "civ5/content/info_types.hpp"
#include <filesystem>
#include <stdexcept>
#include <string_view>
#include <vector>

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
    std::size_t found=0;
    for(std::size_t i=0;i<hashes.size();++i)
        if(hashes[i]==content::saved_info_type_hash("TECH_AGRICULTURE")) {
            flags[i]=1;
            ++found;
        }
    check(found==1,"Agriculture raw hash ambiguous");
    // Contract fixture only; no selected gameplay Lua callback is claimed.
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
BnwTeamTechSuffixCallbackOwner fixture_owner(
    LiveSessionState &state,const SessionCheckpointIdentity &identity,
    std::vector<std::string> &order,bool option) {
    BnwTeamTechSuffixCallbackOwner owner;
    owner.getter_world=&state;
    owner.identity=identity;
    owner.selected_ui_option_owner=true;
    owner.selected_engine_owner=true;
    owner.external_writes_escrowed=true;
    owner.query_no_reward_popups=[&order,option](const LiveSessionState &) {
        order.push_back("ui_query");
        return std::optional<bool>{option};
    };
    owner.gameplay_tech_acquired=[&order](std::int64_t team,
                                           std::int64_t technology,
                                           const LiveSessionState &) {
        check(team==63 && technology==0,
              "Wrong GameplayTechAcquired fixture arguments");
        order.push_back("engine_callback");
        return true;
    };
    return owner;
}
}
int main(int argc,char **argv) {
    check(argc==3,"Expected selected BNW save and game root");
    auto bootstrap=ui::bootstrap_selected_bnw_preview(
        std::filesystem::path(argv[1]),std::filesystem::path(argv[2]),0);
    if(bootstrap.preview.blocker)
        throw std::runtime_error(bootstrap.preview.blocker->reason);
    const auto &host=*bootstrap.preview.host;
    const auto types=host.database().cache_info_types();
    const auto identity=host.identity();
    BnwTeamTechLuaAttempt quote;
    quote.status=BnwTeamTechLuaStatus::quoted;
    quote.hook_value=true;
    auto selected=host.session().state();
    prepare(selected,quote,host,types);

    // SelectedBnwPreviewHost/BnwNotificationPopupOwner expose saved
    // notification activation, not the DLLUI reward option or gDLL callback.
    const auto absent=quote_bnw_team_tech_suffix_callbacks(
        selected,quote,host.technologies(),host.database(),types,
        identity,nullptr,63,0);
    check(absent.status==BnwTeamTechSuffixCallbackStatus::blocked &&
          !absent.ui_query_confirmed && !absent.engine_confirmed &&
          !selected.setup.extensions.find<BnwTeamTechSuffixCallbackMarker>() &&
          !absent.grant_complete,
          "Selected Gandhi fabricated missing UI/engine owners");

    std::vector<std::string> order;
    auto owner=fixture_owner(selected,identity,order,false);
    owner.selected_engine_owner=false;
    const auto partial=quote_bnw_team_tech_suffix_callbacks(
        selected,quote,host.technologies(),host.database(),types,
        identity,&owner,63,0);
    check(partial.status==BnwTeamTechSuffixCallbackStatus::blocked &&
          order.empty() &&
          !selected.setup.extensions.find<BnwTeamTechSuffixCallbackMarker>(),
          "Partial callback owner performed UI query before engine preflight");
    owner.selected_engine_owner=true;
    owner.getter_world=&host.session().state();
    const auto foreign=quote_bnw_team_tech_suffix_callbacks(
        selected,quote,host.technologies(),host.database(),types,
        identity,&owner,63,0);
    check(foreign.status==BnwTeamTechSuffixCallbackStatus::blocked &&
          order.empty(),"Foreign-world callback owner was accepted");
    owner.getter_world=&selected;
    const auto quoted=quote_bnw_team_tech_suffix_callbacks(
        selected,quote,host.technologies(),host.database(),types,
        identity,&owner,63,0);
    const auto *marker=selected.setup.extensions.find<
        BnwTeamTechSuffixCallbackMarker>();
    check(quoted.status==BnwTeamTechSuffixCallbackStatus::quoted &&
          quoted.ui_query_confirmed && quoted.engine_confirmed &&
          quoted.no_reward_popups==false &&
          quoted.notifications==0 && quoted.popups==0 &&
          order==std::vector<std::string>{"ui_query","engine_callback"} &&
          marker && marker->ui_query_confirmed &&
          marker->engine_confirmed && !marker->uncertain &&
          !quoted.grant_complete,
          "Fixture callbacks did not produce ordered verified receipts");
    check(quote_bnw_team_tech_suffix_callbacks(
              selected,quote,host.technologies(),host.database(),types,
              identity,&owner,63,0).status==
              BnwTeamTechSuffixCallbackStatus::blocked &&
          order.size()==2,
          "Verified external callbacks were replayed");

    auto notify_option=host.session().state();
    prepare(notify_option,quote,host,types);
    std::vector<std::string> second_order;
    auto second_owner=fixture_owner(notify_option,identity,second_order,true);
    const auto true_option=quote_bnw_team_tech_suffix_callbacks(
        notify_option,quote,host.technologies(),host.database(),types,
        identity,&second_owner,63,0);
    check(true_option.status==BnwTeamTechSuffixCallbackStatus::quoted &&
          true_option.no_reward_popups==true &&
          true_option.notifications==0 && true_option.popups==0 &&
          second_order.size()==2,
          "NoRewardPopups true invented recipients for team 63");

    auto uncertain=host.session().state();
    prepare(uncertain,quote,host,types);
    std::vector<std::string> failed_order;
    auto failed_owner=fixture_owner(uncertain,identity,failed_order,false);
    failed_owner.gameplay_tech_acquired=[&failed_order](auto,auto,const auto &) -> bool {
        failed_order.push_back("engine_callback");
        throw std::runtime_error("fixture engine failure");
    };
    const auto failed=quote_bnw_team_tech_suffix_callbacks(
        uncertain,quote,host.technologies(),host.database(),types,
        identity,&failed_owner,63,0);
    const auto *failed_marker=uncertain.setup.extensions.find<
        BnwTeamTechSuffixCallbackMarker>();
    check(failed.status==BnwTeamTechSuffixCallbackStatus::uncertain &&
          failed.ui_query_confirmed && !failed.engine_confirmed &&
          failed_marker && failed_marker->uncertain &&
          failed_marker->engine_attempted &&
          failed_order.size()==2 &&
          quote_bnw_team_tech_suffix_callbacks(
              uncertain,quote,host.technologies(),host.database(),types,
              identity,&failed_owner,63,0).status==
              BnwTeamTechSuffixCallbackStatus::blocked &&
          failed_order.size()==2,
          "Throwing engine callback was retried or treated as success");
}
