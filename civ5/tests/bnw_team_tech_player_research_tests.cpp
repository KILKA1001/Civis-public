#include "civ5/runtime/bnw_team_tech_player_research.hpp"
#include "civ5/ui/selected_bnw_preview_host.hpp"
#include "civ5/content/info_types.hpp"
#include <algorithm>
#include <filesystem>
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
    throw std::runtime_error("Missing named test record");
}
template<class T> const T &named(const std::vector<T> &items,std::string_view name) {
    for(const auto &item:items)
        if(item.name && std::string_view(item.name)==name) return item;
    throw std::runtime_error("Missing named test record");
}
void quote_fixture(LiveSessionState &state) {
    // Contract fixture only: the actual selected gameplay Lua owner remains
    // mandatory for publication of the enclosing Agriculture grant.
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
    check(found==1,"Selected Agriculture team hash ambiguous");
    state.setup.extensions.emplace<BnwTeamTechLuaAttemptMarker>(
        BnwTeamTechLuaAttemptMarker{63,0,true,false,true});
}
void research_fixture(LiveSessionState &state) {
    auto &player=state.world.all_players->players[63];
    auto &pre=named(player.administration.blocks,"pre_city");
    auto &raw_queue=named(pre.arrays,"m_researchQueue").values;
    auto &raw_count=named(pre.fields,"m_researchQueue_wire_count");
    check(raw_queue.empty() && state.player_research.players.at(63).queue.empty(),
          "Selected barbarian queue is not empty");
    raw_queue.push_back(0);
    raw_count.value=1;
    state.player_research.players.at(63).queue.push_back(0);
    state.research_turn.players.at(63).researching.insert(0);
    auto &techs=named(player.knowledge.blocks,"player_techs");
    auto &rows=named(techs.hashes,"researching_tech").rows;
    std::size_t found=0;
    for(auto &row:rows)
        if(row.type_hash==content::saved_info_type_hash("TECH_AGRICULTURE")) {
            check(row.values.size()==1,"Agriculture flag row malformed");
            row.values[0]=1;
            ++found;
        }
    check(found==1,"Selected Agriculture researching flag is not unique");
    // Synthetic active-player switch exercises the source popup and the
    // three active-team dirty bits without calling the real UI.
    state.world.pregame->archive.integers["active_player"]=63;
}
void city_fixture(LiveSessionState &state) {
    auto &source=state.world.all_players->players[0].cities.cities;
    check(!source.empty(),"Selected Gandhi has no city fixture");
    auto city=source.front();
    auto &owner=named(city.core.fields,"m_eOwner");
    check(owner.values.size()==1,"City owner fixture malformed");
    owner.values[0]=63;
    auto &before=named(city.production.blocks,"before_strategy");
    for(const auto name:{"m_abBaseYieldRankValid","m_abYieldRankValid"}) {
        auto &values=named(before.arrays,name).values;
        check(values.size()==6,"City rank flags do not have BNW yield span");
        std::fill(values.begin(),values.end(),1);
    }
    auto &barbarian=state.world.all_players->players[63].cities;
    check(barbarian.cities.empty(),"Selected barbarian unexpectedly owns a city");
    city.index=0;
    barbarian.cities.push_back(std::move(city));
    barbarian.header.wire_city_count=1;
}
void check_cleared(const LiveSessionState &state) {
    const auto &player=state.world.all_players->players[63];
    check(state.player_research.players.at(63).queue.empty() &&
          !state.research_turn.players.at(63).researching.contains(0),
          "Owned research queue or flag was not cleared");
    const auto &pre=named(player.administration.blocks,"pre_city");
    check(named(pre.arrays,"m_researchQueue").values.empty() &&
          named(pre.fields,"m_researchQueue_wire_count").value==0,
          "Raw research queue was not cleared");
    const auto &before=named(player.cities.cities[0].production.blocks,
                             "before_strategy");
    for(const auto name:{"m_abBaseYieldRankValid","m_abYieldRankValid"})
        for(const auto value:named(before.arrays,name).values)
            check(value==0,"City yield-rank validity was not cleared");
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
    BnwTeamTechLuaAttempt quote;
    quote.status=BnwTeamTechLuaStatus::quoted;
    quote.hook_value=true;
    auto state=host.session().state();
    quote_fixture(state);
    const auto missing_best=run_bnw_team_tech_player_research(
        state,quote,host.technologies(),host.database(),types,63,0);
    check(!missing_best.applied && !missing_best.grant_complete,
          "Player loop passed absent best-route marker");
    const auto route=run_bnw_team_tech_best_route(
        state,quote,host.technologies(),host.database(),types,63,0);
    check(route.applied,"Selected best-route predecessor unavailable");
    const auto unchanged=run_bnw_team_tech_player_research(
        state,quote,host.technologies(),host.database(),types,63,0);
    check(unchanged.applied && !unchanged.grant_complete &&
          unchanged.effects.size()==1 && unchanged.effects[0].player==63 &&
          !unchanged.effects[0].popped &&
          unchanged.effects[0].cities_invalidated==0 &&
          unchanged.checkpoint_sections.empty() &&
          !host.session().state().technologies[63].has_tech(0),
          "Selected Gandhi barbarian player loop changed");

    auto changed=host.session().state();
    quote_fixture(changed);
    check(run_bnw_team_tech_best_route(changed,quote,host.technologies(),
          host.database(),types,63,0).applied,
          "Synthetic predecessor unavailable");
    research_fixture(changed);
    city_fixture(changed);
    const auto applied=run_bnw_team_tech_player_research(
        changed,quote,host.technologies(),host.database(),types,63,0);
    check(applied.applied && !applied.grant_complete &&
          applied.effects.size()==1 && applied.effects[0].popped &&
          applied.effects[0].cities_invalidated==1 &&
          applied.effects[0].rank_flags_changed &&
          applied.effects[0].dirty_order==std::vector<BnwTeamTechResearchDirtyBit>{
              BnwTeamTechResearchDirtyBit::popup,
              BnwTeamTechResearchDirtyBit::research_buttons,
              BnwTeamTechResearchDirtyBit::game_data,
              BnwTeamTechResearchDirtyBit::score} &&
          applied.checkpoint_sections.size()==5,
          "Synthetic queue/city/UI effect plan changed");
    check_cleared(changed);
    check(!run_bnw_team_tech_player_research(changed,quote,host.technologies(),
              host.database(),types,63,0).applied,
          "Player loop effect replayed");

    auto divergent=host.session().state();
    quote_fixture(divergent);
    check(run_bnw_team_tech_best_route(divergent,quote,host.technologies(),
          host.database(),types,63,0).applied,
          "Divergent predecessor unavailable");
    research_fixture(divergent);
    auto &pre=named(divergent.world.all_players->players[63].administration.blocks,
                    "pre_city");
    named(pre.fields,"m_researchQueue_wire_count").value=2;
    const auto rejected=run_bnw_team_tech_player_research(
        divergent,quote,host.technologies(),host.database(),types,63,0);
    check(!rejected.applied && !divergent.setup.extensions.find<
              BnwTeamTechPlayerResearchMarker>() &&
          divergent.player_research.players.at(63).queue.size()==1,
          "Divergent raw queue partially changed detached world");
}
