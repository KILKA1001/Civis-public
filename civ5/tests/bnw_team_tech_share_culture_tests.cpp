#include "civ5/runtime/bnw_team_tech_share_culture.hpp"
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
void quote_fixture(LiveSessionState &state) {
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
    check(matches==1,"Agriculture team raw hash is not unique");
    // Fixture evidence for this interval only, never a retail Lua dispatch.
    state.setup.extensions.emplace<BnwTeamTechLuaAttemptMarker>(
        BnwTeamTechLuaAttemptMarker{63,0,true,false,true});
}
void prepare_previous(LiveSessionState &state,const BnwTeamTechLuaAttempt &quote,
                      const ui::SelectedBnwPreviewHost &host,
                      const content::InfoTypeCatalog &types) {
    quote_fixture(state);
    check(run_bnw_team_tech_best_route(state,quote,host.technologies(),
          host.database(),types,63,0).applied,
          "Best-route predecessor unavailable");
    check(run_bnw_team_tech_player_research(state,quote,host.technologies(),
          host.database(),types,63,0).applied,
          "Player research predecessor unavailable");
}
void raw_tech(LiveSessionState &state,std::int64_t team,bool owned) {
    state.technologies[static_cast<std::size_t>(team)].set_has_tech(0,owned);
    auto &raw=state.world.teams->teams[static_cast<std::size_t>(team)];
    const auto &hashes=named(raw.arrays,"tech_hashes").values;
    auto &flags=named(raw.arrays,"has_tech").values;
    std::size_t matches=0;
    for(std::size_t i=0;i<hashes.size();++i)
        if(hashes[i]==content::saved_info_type_hash("TECH_AGRICULTURE")) {
            flags[i]=owned ? 1 : 0;
            ++matches;
        }
    check(matches==1,"Synthetic team raw Agriculture hash ambiguous");
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
    auto detached=host.session().state();
    quote_fixture(detached);
    const auto early=run_bnw_team_tech_share_culture(
        detached,quote,host.technologies(),host.database(),types,63,0);
    check(!early.applied && !early.grant_complete,
          "Tech-share interval passed absent player-loop marker");
    detached=host.session().state();
    prepare_previous(detached,quote,host,types);
    const auto safe=run_bnw_team_tech_share_culture(
        detached,quote,host.technologies(),host.database(),types,63,0);
    check(safe.applied && !safe.grant_complete &&
          safe.proven_no_effect==std::vector<BnwTeamTechShareCultureStep>{
              BnwTeamTechShareCultureStep::first_to_research,
              BnwTeamTechShareCultureStep::announce,
              BnwTeamTechShareCultureStep::tech_share,
              BnwTeamTechShareCultureStep::owed_culture_buildings} &&
          safe.first_pending==BnwTeamTechShareCultureStep::later_set_has_tech &&
          safe.ui_intents.empty() && safe.additional_checkpoint_sections.empty() &&
          detached.setup.extensions.find<BnwTeamTechShareCultureMarker>() &&
          !host.session().state().technologies[63].has_tech(0),
          "Selected Gandhi share/culture no-effect interval changed");
    std::cout << "selected Gandhi checked " << safe.checked_teams.size()
              << " living non-barbarian teams for Agriculture tech share\n";
    check(!run_bnw_team_tech_share_culture(detached,quote,host.technologies(),
          host.database(),types,63,0).applied,
          "Tech-share/culture interval replayed");

    auto grant=host.session().state();
    prepare_previous(grant,quote,host,types);
    std::int64_t donor=-1,recipient=-1;
    for(std::int64_t team=0;team<63;++team)
        if(grant.world.teams->teams[static_cast<std::size_t>(team)].fields.size() &&
           named(grant.world.teams->teams[static_cast<std::size_t>(team)].fields,
                 "alive_count").value>0) {
            if(donor<0 && grant.technologies[static_cast<std::size_t>(team)].has_tech(0))
                donor=team;
        }
    for(std::int64_t team=0;team<63;++team)
        if(team!=donor &&
           named(grant.world.teams->teams[static_cast<std::size_t>(team)].fields,
                 "alive_count").value>0) {
            recipient=team;
            break;
        }
    check(donor>=0 && recipient>=0,
          "Selected save lacks two living civilization teams for synthetic share");
    raw_tech(grant,recipient,false);
    auto &recipient_team=grant.world.teams->teams[static_cast<std::size_t>(recipient)];
    auto &share=named(recipient_team.arrays,"tech_share").values;
    auto &met=named(recipient_team.arrays,"has_met").values;
    share[0]=1;
    met[static_cast<std::size_t>(donor)]=1;
    const auto would_grant=run_bnw_team_tech_share_culture(
        grant,quote,host.technologies(),host.database(),types,63,0);
    check(!would_grant.applied && !would_grant.grant_complete &&
          would_grant.first_pending==BnwTeamTechShareCultureStep::tech_share &&
          !grant.setup.extensions.find<BnwTeamTechShareCultureMarker>() &&
          !grant.technologies[static_cast<std::size_t>(recipient)].has_tech(0),
          "Recursive tech-share grant was silently skipped or partially applied");

    auto owed=host.session().state();
    prepare_previous(owed,quote,host,types);
    owed.setup.players.at(0).team=63;
    auto &cities=owed.world.all_players->players[0].cities.cities;
    check(!cities.empty(),"Selected Gandhi has no city for owed-building fixture");
    auto &before=named(cities.front().production.blocks,"before_strategy");
    named(before.fields,"m_bOwedCultureBuilding").value=1;
    const auto blocked_owed=run_bnw_team_tech_share_culture(
        owed,quote,host.technologies(),host.database(),types,63,0);
    check(!blocked_owed.applied && !blocked_owed.grant_complete &&
          blocked_owed.first_pending==
              BnwTeamTechShareCultureStep::owed_culture_buildings &&
          !owed.setup.extensions.find<BnwTeamTechShareCultureMarker>(),
          "Owed culture building was silently skipped");
}
