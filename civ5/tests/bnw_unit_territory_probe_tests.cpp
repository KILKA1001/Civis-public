#include "civ5/runtime/bnw_unit_territory_probe.hpp"
#include "civ5/runtime/bnw_map_occupied_preflight.hpp"
#include "civ5/ui/selected_bnw_preview_host.hpp"
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
std::int64_t &field(saves::PlotObservation &plot,std::string_view name) {
    for(auto &entry:plot.fields)
        if(entry.name && std::string_view(entry.name)==name) return entry.value;
    throw std::runtime_error("Missing test plot field");
}
std::vector<std::int64_t> &team_array(saves::TeamObservation &team,
                                      std::string_view name) {
    for(auto &entry:team.arrays)
        if(entry.name && std::string_view(entry.name)==name) return entry.values;
    throw std::runtime_error("Missing test team array");
}
void selected(const std::filesystem::path &save,const std::filesystem::path &root) {
    auto bootstrap=ui::bootstrap_selected_bnw_preview(save,root,0);
    if(bootstrap.preview.blocker)
        throw std::runtime_error(bootstrap.preview.blocker->reason);
    check(bootstrap.preview.host!=nullptr,"Selected BNW host unavailable");
    const auto &source=bootstrap.preview.host->session().state();
    const auto occupied=preflight_bnw_map_occupied(source.world);
    check(occupied.shape_valid && occupied.units.size()==7,
          "Selected Gandhi occupied unit roster changed");
    const auto callback=make_bnw_can_enter_territory_probe(
        &bootstrap.preview.host->database());
    std::size_t unowned=0,friendly=0;
    for(const auto &item:occupied.units) {
        const auto quote=quote_bnw_unit_can_enter_territory(
            source,item.plot,item.handle,&bootstrap.preview.host->database());
        if(quote.allowed!=true)
            std::cerr<<"territory failure plot="<<item.plot<<" reason="
                     <<quote.reason<<'\n';
        check(quote.allowed==true &&
              callback(source,item.plot,item.handle)==true &&
              quote.plot_owner==item.plot_owner,
              "Selected Gandhi territory predicate lacks live proof");
        if(quote.branch==BnwTerritoryBranch::no_team) ++unowned;
        else if(quote.branch==BnwTerritoryBranch::friendly) ++friendly;
        else throw std::runtime_error("Selected Gandhi territory source branch changed");
        std::cout<<"territory plot="<<item.plot<<" unit="<<item.handle.owner
                 <<':'<<item.handle.id<<" branch="
                 <<(quote.branch==BnwTerritoryBranch::no_team?"NO_TEAM":"friendly")
                 <<'\n';
    }
    check(unowned==6 && friendly==1,
          "Selected Gandhi territory short circuits changed");

    auto bad=source;
    bad.world.map_projection->cells[333].owner=0;
    check(!quote_bnw_unit_can_enter_territory(
              bad,333,occupied.units[0].handle).allowed,
          "BNW territory accepted divergent raw and live plot owner");
    // A foreign major plot reaches the active Units rule and target-team
    // open-borders relation. The selected same-team result above needed none.
    auto foreign=source;
    auto &plot=foreign.world.map_plots->plots[592];
    field(plot,"owner")=1;
    foreign.world.map_projection->cells[592].owner=1;
    const saves::SavedPlotUnitReference gandhi{0,16385};
    auto without_rules=quote_bnw_unit_can_enter_territory(foreign,592,gandhi);
    check(!without_rules.allowed && !without_rules.reason.empty(),
          "Foreign territory silently defaulted a missing unit rule");
    const auto team=foreign.setup.players.at(0).team;
    const auto target=foreign.setup.players.at(1).team;
    check(team!=target,"Selected Gandhi major teams changed");
    team_array(foreign.world.teams->teams[target],"open_borders")
        [static_cast<std::size_t>(team)]=1;
    const auto open=quote_bnw_unit_can_enter_territory(
        foreign,592,gandhi,&bootstrap.preview.host->database());
    check(open.allowed==true && open.branch==BnwTerritoryBranch::open_borders,
          "BNW territory open-borders relation or unit rule was not applied");
    team_array(foreign.world.teams->teams[target],"open_borders")
        [static_cast<std::size_t>(team)]=0;
    const auto blocked=quote_bnw_unit_can_enter_territory(
        foreign,592,gandhi,&bootstrap.preview.host->database());
    check(blocked.allowed==false && blocked.branch==BnwTerritoryBranch::blocked,
          "BNW foreign territory without war or passage was not rejected");
    team_array(foreign.world.teams->teams[team],"at_war")
        [static_cast<std::size_t>(target)]=1;
    const auto enemy=quote_bnw_unit_can_enter_territory(
        foreign,592,gandhi,&bootstrap.preview.host->database());
    check(enemy.allowed==true && enemy.branch==BnwTerritoryBranch::enemy,
          "BNW foreign territory war branch was not applied first");
    std::cout<<"selected BNW territory: 6 NO_TEAM, 1 friendly; foreign open borders proved\n";
}
} // namespace

int main(int argc,char **argv) {
    check(argc==3,"Supply selected Gandhi BNW save and game root");
    selected(argv[1],argv[2]);
}
