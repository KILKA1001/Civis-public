#include "civ5/runtime/bnw_unit_visibility_probe.hpp"
#include "civ5/runtime/bnw_map_occupied_preflight.hpp"
#include "civ5/ui/selected_bnw_preview_host.hpp"
#include <algorithm>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string_view>

namespace {
using namespace civ5;
using namespace civ5::runtime;
void check(bool value,const char *reason) {
    if(!value) throw std::runtime_error(reason);
}
std::int64_t &core(saves::SavedUnitObservation &unit,std::string_view wanted) {
    for(auto &entry:unit.core.fields)
        if(entry.name && std::string_view(entry.name)==wanted &&
           entry.values.size()==1) return entry.values.front();
    throw std::runtime_error("Missing test unit field");
}
std::int64_t &field(saves::PlotObservation &plot,std::string_view wanted) {
    for(auto &entry:plot.fields)
        if(entry.name && std::string_view(entry.name)==wanted) return entry.value;
    throw std::runtime_error("Missing test plot field");
}
std::vector<std::int64_t> &array(saves::PlotObservation &plot,
                                 std::string_view wanted) {
    for(auto &entry:plot.arrays)
        if(entry.name && std::string_view(entry.name)==wanted)
            return entry.values;
    throw std::runtime_error("Missing test plot array");
}
std::vector<std::int64_t> &team_array(saves::TeamObservation &team,
                                      std::string_view wanted) {
    for(auto &entry:team.arrays)
        if(entry.name && std::string_view(entry.name)==wanted)
            return entry.values;
    throw std::runtime_error("Missing test team array");
}
saves::SavedUnitObservation &unit(saves::SavedSessionSnapshot &world,
                                  std::int64_t owner,std::int64_t id) {
    for(auto &player:world.all_players->players)
        if(player.saved_player_id==owner)
            for(auto &candidate:player.units.units)
                if(core(candidate,"m_iID")==id) return candidate;
    throw std::runtime_error("Missing selected unit");
}
void selected(const std::filesystem::path &save,
              const std::filesystem::path &root) {
    auto bootstrap=ui::bootstrap_selected_bnw_preview(save,root,0);
    if(bootstrap.preview.blocker)
        throw std::runtime_error(bootstrap.preview.blocker->reason);
    check(bootstrap.preview.host!=nullptr,"Selected BNW host unavailable");
    const auto &source=bootstrap.preview.host->session().state();
    const auto occupied=preflight_bnw_map_occupied(source.world);
    check(occupied.shape_valid && occupied.units.size()==7,
          "Selected Gandhi occupied plot roster changed");
    const auto enemy_callback=make_bnw_visible_enemy_probe();
    const auto invisible_callback=make_bnw_unit_invisible_probe();
    std::size_t owned=0;
    for(const auto &item:occupied.units) {
        const auto enemy=quote_bnw_visible_enemy_unit(
            source,item.plot,item.handle);
        check(enemy.value==false &&
              enemy_callback(source,item.plot,item.handle)==false,
              "Selected Gandhi has unresolved visible enemy predicate");
        if(item.plot_owner>=0) {
            ++owned;
            const auto observer=source.setup.players.at(item.plot_owner).team;
            const auto hidden=quote_bnw_unit_invisible(
                source,item.plot,item.handle,observer);
            check(hidden.value==false &&
                  invisible_callback(source,item.plot,item.handle)==false,
                  "Selected Gandhi owned plot invisibility changed");
        } else check(!invisible_callback(source,item.plot,item.handle),
                     "Unowned plot received a guessed observer team");
    }
    check(owned==1,"Selected Gandhi owned occupied plot count changed");

    auto foreign=source;
    const saves::SavedPlotUnitReference visitor{1,8192};
    auto &old_roster=*foreign.world.map_projection->cells[420].units;
    check(old_roster.size()==1 && old_roster.front()==visitor,
          "Selected Gandhi movable visitor changed");
    old_roster.clear();
    array(foreign.world.map_plots->plots[420],"units").clear();
    foreign.world.map_projection->cells[592].units->push_back(visitor);
    auto &raw=array(foreign.world.map_plots->plots[592],"units");
    raw.push_back(visitor.owner);raw.push_back(visitor.id);
    const auto xy=foreign.world.map_header.grid.coordinates(592);
    auto &moved=unit(foreign.world,visitor.owner,visitor.id);
    core(moved,"m_iX")=xy[0];core(moved,"m_iY")=xy[1];
    core(moved,"m_eInvisibleType")=-1;
    const auto observer=foreign.setup.players.at(0).team;
    const auto attacker=foreign.setup.players.at(1).team;
    check(observer!=attacker,"Selected Gandhi teams unexpectedly coincide");
    team_array(foreign.world.teams->teams[observer],"at_war")
        [static_cast<std::size_t>(attacker)]=1;
    const saves::SavedPlotUnitReference gandhi{0,16385};
    const auto enemy=quote_bnw_visible_enemy_unit(foreign,592,gandhi);
    if(enemy.value!=true) std::cerr<<"enemy reason="<<enemy.reason<<'\n';
    check(enemy.value==true,"Foreign visible enemy was not detected");
    team_array(foreign.world.teams->teams[observer],"at_war")
        [static_cast<std::size_t>(attacker)]=0;
    check(quote_bnw_visible_enemy_unit(foreign,592,gandhi).value==false,
          "Peaceful foreign unit was treated as visible enemy");
    const auto native=quote_bnw_unit_invisible(foreign,592,gandhi,attacker);
    check(native.value==false,"Selected Gandhi unit was unexpectedly invisible");
    const auto types=foreign.world.map_plots->dimensions.invisible_types;
    check(types>0,"Selected BNW invisible wire dimensions unavailable");
    core(unit(foreign.world,0,16385),"m_eInvisibleType")=0;
    auto &counts=array(foreign.world.map_plots->plots[592],
                       "invisible_visibility");
    const auto offset=static_cast<std::size_t>(attacker)*types;
    check(offset<counts.size(),"Selected invisible count offset invalid");
    counts[offset]=0;
    const auto undetected=quote_bnw_unit_invisible(foreign,592,gandhi,attacker);
    if(undetected.value!=true) std::cerr<<"undetected reason="<<undetected.reason<<" value="<<undetected.value.value_or(false)<<'\n';
    check(undetected.value==true,
          "Undetected invisible unit was treated as visible");
    counts[offset]=1;
    check(quote_bnw_unit_invisible(foreign,592,gandhi,attacker).value==false,
          "Invisible detection counter was ignored");
    foreign.world.map_projection->cells[592].units->pop_back();
    check(!quote_bnw_visible_enemy_unit(foreign,592,gandhi).value,
          "Divergent raw/live roster silently resolved");
    std::cout<<"selected BNW visibility: 7 visible_enemy=false; "
             <<"1 owned unit invisible=false; synthetic war and detection proved\n";
}
} // namespace

int main(int argc,char **argv) {
    check(argc==3,"Supply selected Gandhi BNW save and game root");
    selected(argv[1],argv[2]);
}
