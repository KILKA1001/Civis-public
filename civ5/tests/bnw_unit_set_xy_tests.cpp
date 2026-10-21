#include "civ5/runtime/bnw_unit_set_xy.hpp"
#include "civ5/saves/loaded_save.hpp"
#include <algorithm>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string_view>

namespace {
using namespace civ5;
void check(bool condition,const char *why) {if(!condition) throw std::runtime_error(why);}
std::int64_t &core(saves::SavedUnitObservation &unit,std::string_view wanted) {
    for(auto &field:unit.core.fields)
        if(field.name && wanted==field.name && field.values.size()==1)
            return field.values.front();
    throw std::runtime_error("Selected BNW unit core field unavailable");
}
std::vector<std::int64_t> &units(saves::PlotObservation &plot) {
    for(auto &array:plot.arrays)
        if(array.name && std::string_view(array.name)=="units" && array.columns==2)
            return array.values;
    throw std::runtime_error("Selected BNW plot roster unavailable");
}
struct Fixture {runtime::BnwSetXYRequest request;std::size_t player=0,unit=0;};
Fixture find_fixture(saves::SavedSessionSnapshot &world) {
    const auto &grid=world.map_header.grid;
    for(std::size_t p=0;p<world.all_players->players.size();++p)
        for(std::size_t u=0;u<world.all_players->players[p].units.units.size();++u) {
            auto &unit=world.all_players->players[p].units.units[u];
            const auto owner=core(unit,"m_eOwner"),id=core(unit,"m_iID");
            const auto x=core(unit,"m_iX"),y=core(unit,"m_iY");
            if(x<0 || y<0 || x>=static_cast<std::int64_t>(grid.width) ||
               y>=static_cast<std::int64_t>(grid.height)) continue;
            for(const auto [dx,dy]:{std::pair{-1,0},std::pair{1,0},
                                    std::pair{0,-1},std::pair{0,1}}) {
                const auto nx=x+dx,ny=y+dy;
                if(nx<0 || ny<0 || nx>=static_cast<std::int64_t>(grid.width) ||
                   ny>=static_cast<std::int64_t>(grid.height)) continue;
                runtime::BnwSetXYRequest request{owner,id,nx,ny};
                if(runtime::preflight_bnw_set_xy(world,request).simple_candidate)
                    return {request,p,u};
            }
        }
    throw std::runtime_error("Selected Gandhi save has no simple BNW setXY candidate");
}
void test_selected(const std::filesystem::path &path) {
    saves::SavedSessionDecodeContext context;
    context.decode_map_regions=true;context.decode_teams=true;
    context.decode_all_players=true;context.decode_supporting_classes=true;
    context.all_players=saves::AllPlayersContext{};
    auto imported=saves::read_saved_session(path,context);
    auto &world=imported.snapshot;
    const auto fixture=find_fixture(world);
    auto &unit=world.all_players->players[fixture.player].units.units[fixture.unit];
    const auto original_x=core(unit,"m_iX"),original_y=core(unit,"m_iY");
    const auto old_plot=world.map_header.grid.index(original_x,original_y);
    const auto source_roster=units(world.map_plots->plots[old_plot]);
    const auto result=runtime::preflight_bnw_set_xy(world,fixture.request);
    check(result.simple_candidate && !result.can_commit() &&
          result.from_x==original_x && result.from_y==original_y &&
          result.to_x==fixture.request.to_x && result.to_y==fixture.request.to_y &&
          result.next_last_move_turn==
              std::get<saves::Expansion2PrefixObservation>(world.game_state.prefix).scalars[7],
          "BNW setXY preflight lost decoded candidate or SDK turn slice");
    const std::vector<runtime::BnwSetXYPhase> order{
        runtime::BnwSetXYPhase::popup_guard_and_ranged_setup,
        runtime::BnwSetXYPhase::destination_collision,
        runtime::BnwSetXYPhase::old_plot_remove,
        runtime::BnwSetXYPhase::old_plot_sight_area_and_move_clock,
        runtime::BnwSetXYPhase::coordinates_and_facing,
        runtime::BnwSetXYPhase::new_plot_fortify_sight_roster_and_area,
        runtime::BnwSetXYPhase::city_transport_meeting_and_adjacent_effects,
        runtime::BnwSetXYPhase::graphics_and_cargo_follow,
        runtime::BnwSetXYPhase::goody_camp_and_territory,
        runtime::BnwSetXYPhase::capture_and_ui_dirty,
        runtime::BnwSetXYPhase::lua_unit_set_xy,
        runtime::BnwSetXYPhase::popup_release_and_garrison_ui};
    check(result.sdk_order==order &&
          std::find(result.blockers.begin(),result.blockers.end(),
              runtime::BnwSetXYBlocker::plot_sight_and_revelation)!=result.blockers.end() &&
          std::find(result.blockers.begin(),result.blockers.end(),
              runtime::BnwSetXYBlocker::lua_unit_set_xy)!=result.blockers.end() &&
          std::find(result.blockers.begin(),result.blockers.end(),
              runtime::BnwSetXYBlocker::complete_unit_persistence)!=result.blockers.end(),
          "BNW setXY preflight omitted mandatory SDK phases or services");
    check(core(unit,"m_iX")==original_x && core(unit,"m_iY")==original_y &&
          units(world.map_plots->plots[old_plot])==source_roster,
          "Read-only BNW setXY preflight changed owned world");
    auto invalid=fixture.request;invalid.to_x=original_x;invalid.to_y=original_y;
    const auto no_move=runtime::preflight_bnw_set_xy(world,invalid);
    check(!no_move.simple_candidate && !no_move.can_commit() &&
          no_move.blockers==std::vector<runtime::BnwSetXYBlocker>{
              runtime::BnwSetXYBlocker::unsupported_candidate},
          "BNW setXY accepted a same-plot move");
    auto damaged=world;
    core(damaged.all_players->players[fixture.player].units.units[fixture.unit],
         "m_bDeathDelay")=1;
    check(!runtime::preflight_bnw_set_xy(damaged,fixture.request).simple_candidate,
          "BNW setXY accepted delayed-death unit");
    damaged=world;
    units(damaged.map_plots->plots[old_plot]).clear();
    check(!runtime::preflight_bnw_set_xy(damaged,fixture.request).simple_candidate,
          "BNW setXY accepted missing source plot handle");
    damaged=world;
    damaged.game_state.layout=saves::GameStateLayout::vp542_corpus;
    check(!runtime::preflight_bnw_set_xy(damaged,fixture.request).simple_candidate,
          "BNW setXY accepted mismatched GameCore profile");
    std::cout << "selected BNW setXY candidate=" << fixture.request.owner << ':' <<
        fixture.request.unit_id << " from " << original_x << ',' << original_y <<
        " to " << fixture.request.to_x << ',' << fixture.request.to_y << '\n';
}
} // namespace

int main(int argc,char **argv) {
    try {
        check(argc==2,"Supply selected Gandhi BNW save path");
        test_selected(argv[1]);
        std::cout << "PASS BNW setXY fail-closed preflight\n";
        return 0;
    } catch(const std::exception &error) {
        std::cerr << "FAIL BNW setXY preflight: " << error.what() << '\n';
        return 1;
    }
}
