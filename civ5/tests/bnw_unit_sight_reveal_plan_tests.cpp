#include "civ5/runtime/bnw_unit_sight_reveal_plan.hpp"
#include "civ5/runtime/file_identity.hpp"
#include "civ5/runtime/live_session.hpp"
#include "civ5/ui/selected_bnw_preview_host.hpp"
#include <algorithm>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string_view>

namespace {
using namespace civ5;
using namespace civ5::runtime;
void check(bool ok,const char *why) {if(!ok) throw std::runtime_error(why);}
std::int64_t core(const saves::SavedUnitObservation &unit,std::string_view wanted) {
    for(const auto &field:unit.core.fields)
        if(field.name && wanted==field.name && field.values.size()==1)
            return field.values.front();
    throw std::runtime_error("BNW selected unit field missing");
}
BnwSetXYRequest candidate(const LiveSessionState &state) {
    const auto &grid=state.world.map_header.grid;
    for(const auto &player:state.world.all_players->players)
        for(const auto &unit:player.units.units) {
            const auto x=core(unit,"m_iX"),y=core(unit,"m_iY");
            if(x<0 || y<0 || x>=static_cast<std::int64_t>(grid.width) ||
               y>=static_cast<std::int64_t>(grid.height)) continue;
            for(const auto [dx,dy]:{std::pair{-1,0},std::pair{1,0},
                                    std::pair{0,-1},std::pair{0,1}}) {
                const BnwSetXYRequest request{core(unit,"m_eOwner"),
                    core(unit,"m_iID"),x+dx,y+dy};
                if(preflight_bnw_unit_sight(state,request).candidate) return request;
            }
        }
    throw std::runtime_error("Selected BNW save has no simple sight candidate");
}
saves::PlotArrayObservation &array(saves::PlotObservation &plot,
                                   std::string_view wanted) {
    for(auto &item:plot.arrays)
        if(item.name && wanted==item.name) return item;
    throw std::runtime_error("Selected BNW plot array missing");
}
void selected(const std::filesystem::path &save,
              const std::filesystem::path &game_root) {
    const auto source=identify_file(save);
    auto bootstrap=ui::bootstrap_selected_bnw_preview(save,game_root,0);
    check(static_cast<bool>(bootstrap.preview.host),
          "Selected BNW live preview did not bootstrap");
    auto &host=*bootstrap.preview.host;
    const auto &state=host.session().state();
    const auto input=host.preview_input();
    check(input.frame_sources.database,"Selected BNW rules database is absent");
    const auto &database=*input.frame_sources.database;
    const auto types=database.cache_info_types();
    const auto defines=database.cache_global_defines(content::GlobalDefineProfile::bnw);
    const auto request=candidate(state);
    const auto plan=plan_bnw_unit_sight_reveal(
        state,request,database,types,defines,host.identity());
    check(plan.calculated && !plan.can_commit() &&
          plan.team==plan.counters.team && !plan.newly_visible.empty(),
          plan.reason.c_str());
    std::size_t first=0,meet_units=0,meet_cities=0;
    for(const auto &edge:plan.newly_visible) {
        const auto &delta=*std::find_if(plan.counters.plots.begin(),
            plan.counters.plots.end(),[&](const auto &item){return item.plot==edge.plot;});
        check(delta.after_old==0 && delta.after_new>0 && delta.new_calls==1 &&
              edge.adjacent_owner_refresh.size()<=6,
              "BNW reveal edge does not match SDK visibility transition");
        if(edge.first_reveal) {
            ++first;
            check(edge.area>=-1 &&
                  (edge.area<0 || edge.area_revealed_before>=0),
                  "BNW first reveal omitted area accounting input");
        }
        meet_units+=edge.meet_unit_candidates.size();
        meet_cities+=edge.meet_city_candidate.has_value();
    }
    check(request.owner==0 && request.unit_id==16385 &&
          plan.newly_visible.size()==5 && first==5 &&
          meet_units==0 && meet_cities==0,
          "Selected Gandhi reveal corpus result changed");
    auto bad_reveal=state;
    const auto index=plan.newly_visible.front().plot;
    auto &words=array(bad_reveal.world.map_plots->plots[index],"revealed_words").values;
    words[static_cast<std::size_t>(plan.team/32)]^=
        std::uint32_t{1}<<(plan.team%32);
    check(!plan_bnw_unit_sight_reveal(
        bad_reveal,request,database,types,defines,host.identity()).calculated,
        "BNW reveal plan accepted inconsistent raw and projected bits");
    auto previously_revealed=state;
    auto &old_words=array(previously_revealed.world.map_plots->plots[index],
                          "revealed_words").values;
    old_words[static_cast<std::size_t>(plan.team/32)]|=
        std::uint32_t{1}<<(plan.team%32);
    previously_revealed.world.map_projection->cells[index].revealed_teams->insert(
        plan.team);
    const auto old_plan=plan_bnw_unit_sight_reveal(
        previously_revealed,request,database,types,defines,host.identity());
    check(old_plan.calculated && !old_plan.newly_visible.front().first_reveal &&
          old_plan.newly_visible.front().area_revealed_before==-1,
          "BNW already-revealed visibility edge incorrectly increments area");
    auto no_area=state;
    no_area.world.map_regions.reset();
    check(!plan_bnw_unit_sight_reveal(
        no_area,request,database,types,defines,host.identity()).calculated,
        "BNW first reveal accepted undecoded area counts");
    auto bad_roster=state;
    auto &roster=array(bad_roster.world.map_plots->plots[index],"units").values;
    roster.push_back(0);
    check(!plan_bnw_unit_sight_reveal(
        bad_roster,request,database,types,defines,host.identity()).calculated,
        "BNW reveal plan accepted malformed meet roster");
    auto wrong=host.identity();wrong.rules_sha256=std::string(64,'0');
    check(!plan_bnw_unit_sight_reveal(
        state,request,database,types,defines,wrong).calculated,
        "BNW reveal plan accepted different active rules");
    check(identify_file(save).sha256==source.sha256,
          "Read-only BNW reveal plan changed original save");
    std::cout << "BNW selected reveal edges=" << plan.newly_visible.size() <<
        " first=" << first << " meet units=" << meet_units <<
        " cities=" << meet_cities << '\n';
}
}

int main(int argc,char **argv) {
    try {
        check(argc==3,"Supply selected Gandhi save and game root");
        selected(argv[1],argv[2]);
        std::cout << "PASS BNW read-only reveal/meet inputs\n";
        return 0;
    } catch(const std::exception &error) {
        std::cerr << "FAIL BNW reveal plan: " << error.what() << '\n';
        return 1;
    }
}
