#include "civ5/runtime/bnw_unit_sight_plan.hpp"
#include "civ5/runtime/bnw_live_plot_visibility.hpp"
#include "civ5/runtime/file_identity.hpp"
#include "civ5/runtime/live_session.hpp"
#include "civ5/ui/selected_bnw_preview_host.hpp"
#include <algorithm>
#include <filesystem>
#include <iostream>
#include <set>
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
    throw std::runtime_error("Selected BNW sight plan unit field missing");
}
std::int64_t invisible(const saves::PlotObservation &plot,std::int64_t team) {
    for(const auto &array:plot.arrays)
        if(array.name && std::string_view(array.name)=="invisible_visibility") {
            check(array.columns==1 && array.values.size()==80,
                  "Selected BNW invisible visibility shape changed");
            return array.values.at(static_cast<std::size_t>(team));
        }
    throw std::runtime_error("Selected BNW invisible visibility is absent");
}
void set_core(saves::SavedUnitObservation &unit,std::string_view wanted,
              std::int64_t value) {
    for(auto &field:unit.core.fields)
        if(field.name && wanted==field.name && field.values.size()==1) {
            field.values.front()=value;return;
        }
    throw std::runtime_error("Selected BNW sight plan unit field missing");
}
void set_plot_field(saves::PlotObservation &plot,std::string_view wanted,
                    std::int64_t value) {
    for(auto &field:plot.fields)
        if(field.name && wanted==field.name) {field.value=value;return;}
    throw std::runtime_error("Selected BNW sight plan plot field missing");
}
BnwSetXYRequest find_candidate(const LiveSessionState &state) {
    const auto &world=state.world;
    const auto &grid=world.map_header.grid;
    for(const auto &player:world.all_players->players)
        for(const auto &unit:player.units.units) {
            const auto x=core(unit,"m_iX"),y=core(unit,"m_iY");
            if(x<0 || y<0 || x>=static_cast<std::int64_t>(grid.width) ||
               y>=static_cast<std::int64_t>(grid.height)) continue;
            for(const auto [dx,dy]:{std::pair{-1,0},std::pair{1,0},
                                    std::pair{0,-1},std::pair{0,1}}) {
                const auto nx=x+dx,ny=y+dy;
                if(nx<0 || ny<0 || nx>=static_cast<std::int64_t>(grid.width) ||
                   ny>=static_cast<std::int64_t>(grid.height)) continue;
                const BnwSetXYRequest request{core(unit,"m_eOwner"),
                                              core(unit,"m_iID"),nx,ny};
                if(preflight_bnw_unit_sight(state,request).candidate) return request;
            }
        }
    throw std::runtime_error("Selected Gandhi save lacks simple BNW sight candidate");
}
void selected(const std::filesystem::path &save,
              const std::filesystem::path &game_root) {
    const auto original=identify_file(save);
    auto bootstrap=ui::bootstrap_selected_bnw_preview(save,game_root,0);
    if(!bootstrap.preview.host)
        throw std::runtime_error("Selected BNW rules/live preview did not bootstrap: "+
            (bootstrap.preview.blocker?bootstrap.preview.blocker->reason:
                                       std::string("missing blocker")));
    auto &host=*bootstrap.preview.host;
    const auto &state=host.session().state();
    const auto input=host.preview_input();
    check(input.frame_sources.database,"Selected BNW preview has no active rules");
    const auto &db=*input.frame_sources.database;
    const auto types=db.cache_info_types();
    const auto defines=db.cache_global_defines(content::GlobalDefineProfile::bnw);
    const auto request=find_candidate(state);
    const auto world_before=state.world;
    const auto plan=plan_bnw_unit_sight(state,request,db,types,defines,host.identity());
    check(plan.calculated && !plan.can_commit(),plan.reason.c_str());
    check(plan.team>=0 && plan.team<64 && plan.visibility_range>=0 &&
          !plan.plots.empty() &&
          std::find(plan.blockers.begin(),plan.blockers.end(),
              BnwSightBlocker::reveal_meeting_and_exploration_effects)!=
              plan.blockers.end(),
          "Selected BNW sight plan omitted range, cells or reveal blocker");
    std::set<std::uint64_t> ids;
    std::size_t old_calls=0,new_calls=0,old_invisible=0,new_invisible=0;
    std::size_t old_only=0,new_only=0;
    for(const auto &plot:plan.plots) {
        check(ids.insert(plot.plot).second &&
              plot.before==bnw_plot_visibility_count(state,plot.plot,plan.team) &&
              plot.after_old==std::max<std::int64_t>(0,plot.before-plot.old_calls) &&
              plot.after_new==plot.after_old+plot.new_calls &&
              plot.invisible_before==invisible(
                  state.world.map_plots->plots[plot.plot],plan.team) &&
              plot.invisible_after_old==
                  plot.invisible_before-plot.invisible_old_calls &&
              plot.invisible_after_new==
                  plot.invisible_after_old+plot.invisible_new_calls &&
              plot.old_calls<=1 && plot.new_calls<=1 &&
              plot.invisible_old_calls<=plot.old_calls &&
              plot.invisible_new_calls<=plot.new_calls,
              "BNW sight plot delta disagrees with SDK ordered clamp");
        old_calls+=plot.old_calls;new_calls+=plot.new_calls;
        old_invisible+=plot.invisible_old_calls;
        new_invisible+=plot.invisible_new_calls;
        old_only+=plot.old_calls && !plot.new_calls;
        new_only+=plot.new_calls && !plot.old_calls;
    }
    check(request.owner==0 && request.unit_id==16385 &&
          plan.visibility_range==2 && old_calls==16 && new_calls==16 &&
          plan.plots.size()==22 && old_invisible==7 && new_invisible==7 &&
          old_only>0 && new_only>0 &&
          std::find(plan.blockers.begin(),plan.blockers.end(),
              BnwSightBlocker::invisible_visibility_effects)!=plan.blockers.end(),
          "Selected BNW sight rule/LOS corpus result changed");
    auto wrong=host.identity();wrong.rules_sha256=std::string(64,'0');
    check(!plan_bnw_unit_sight(state,request,db,types,defines,wrong).calculated,
          "BNW sight accepted a different rules fingerprint");
    auto stale=state;stale.plot_visibility.current=false;
    check(!plan_bnw_unit_sight(stale,request,db,types,defines,host.identity()).calculated,
          "BNW sight accepted stale live visibility");
    auto wrong_unit=state;
    for(auto &player:wrong_unit.world.all_players->players)
        for(auto &unit:player.units.units)
            if(core(unit,"m_eOwner")==request.owner &&
               core(unit,"m_iID")==request.unit_id)
                set_core(unit,"idUnitType",0);
    check(!plan_bnw_unit_sight(wrong_unit,request,db,types,defines,host.identity()).calculated,
          "BNW sight accepted mismatched saved unit type hash");
    auto wrong_terrain=state;
    set_plot_field(wrong_terrain.world.map_plots->plots[plan.plots.front().plot],
                   "terrain_type",127);
    check(!plan_bnw_unit_sight(wrong_terrain,request,db,types,defines,host.identity()).calculated,
          "BNW sight accepted missing active terrain rule");
    check(state.world.map_plots->plots[plan.plots.front().plot].fields.size()==
              world_before.map_plots->plots[plan.plots.front().plot].fields.size() &&
          identify_file(save).sha256==original.sha256,
          "Read-only BNW sight plan changed selected save or owned world");
    std::cout << "selected BNW sight plan unit=" << request.owner << ':' <<
        request.unit_id << " range=" << plan.visibility_range <<
        " old/new calls=" << old_calls << '/' << new_calls <<
        " invisible=" << old_invisible << '/' << new_invisible <<
        " plots=" << plan.plots.size() << '\n';
}
} // namespace

int main(int argc,char **argv) {
    try {
        check(argc==3,"Supply selected Gandhi BNW save and install root");
        selected(argv[1],argv[2]);
        std::cout << "PASS BNW sight read-only rule-bound plan\n";
        return 0;
    } catch(const std::exception &error) {
        std::cerr << "FAIL BNW sight plan: " << error.what() << '\n';
        return 1;
    }
}
