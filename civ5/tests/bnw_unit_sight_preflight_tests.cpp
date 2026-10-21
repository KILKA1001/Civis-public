#include "civ5/runtime/bnw_unit_sight_preflight.hpp"
#include "civ5/runtime/bnw_live_plot_visibility.hpp"
#include "civ5/runtime/live_session.hpp"
#include "civ5/saves/loaded_save.hpp"
#include <algorithm>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string_view>

namespace {
using namespace civ5;
using namespace civ5::runtime;
void check(bool ok,const char *why) {if(!ok) throw std::runtime_error(why);}
LiveSessionState empty_state() {
    return {saves::SavedSessionSnapshot{},ResearchContext{}, {},
        PlayerResearchContext{},CityYieldContext{},BnwUnhappinessContext{},
        MaintenanceContext{},TreasuryContext{},ScienceContext{},ResearchTurnState{},
        {SimulationRandom(RandomAlgorithm::bnw,{0,0,0}),
         SimulationRandom(RandomAlgorithm::bnw,{0,0,0})},
        ResearchSelectionState{},BnwGameClockState{},std::nullopt,std::nullopt,
        {}, {}, {}, {}, {}, {}};
}
std::int64_t core(const saves::SavedUnitObservation &unit,std::string_view wanted) {
    for(const auto &field:unit.core.fields)
        if(field.name && wanted==field.name && field.values.size()==1)
            return field.values.front();
    throw std::runtime_error("Selected BNW sight unit field unavailable");
}
void set_core(saves::SavedUnitObservation &unit,std::string_view wanted,
              std::int64_t value) {
    for(auto &field:unit.core.fields)
        if(field.name && wanted==field.name && field.values.size()==1) {
            field.values.front()=value;return;
        }
    throw std::runtime_error("Selected BNW sight unit field unavailable");
}
const std::vector<std::int64_t> &raw_array(
    const saves::PlotObservation &plot,std::string_view wanted) {
    for(const auto &array:plot.arrays)
        if(array.name && wanted==array.name) return array.values;
    throw std::runtime_error("Selected BNW sight plot array unavailable");
}
BnwSetXYRequest find_candidate(const LiveSessionState &state) {
    const auto &world=state.world;
    const auto &grid=world.map_header.grid;
    for(const auto &player:world.all_players->players)
        for(const auto &unit:player.units.units) {
            const auto owner=core(unit,"m_eOwner"),id=core(unit,"m_iID");
            const auto x=core(unit,"m_iX"),y=core(unit,"m_iY");
            if(x<0 || y<0 || x>=static_cast<std::int64_t>(grid.width) ||
               y>=static_cast<std::int64_t>(grid.height)) continue;
            for(const auto [dx,dy]:{std::pair{-1,0},std::pair{1,0},
                                    std::pair{0,-1},std::pair{0,1}}) {
                const auto nx=x+dx,ny=y+dy;
                if(nx<0 || ny<0 || nx>=static_cast<std::int64_t>(grid.width) ||
                   ny>=static_cast<std::int64_t>(grid.height)) continue;
                const BnwSetXYRequest request{owner,id,nx,ny};
                if(!preflight_bnw_set_xy(world,request).simple_candidate) continue;
                if(preflight_bnw_unit_sight(state,request).candidate) return request;
            }
        }
    throw std::runtime_error("Selected Gandhi save lacks simple nonembarked sight fixture");
}
void selected(const std::filesystem::path &path) {
    saves::SavedSessionDecodeContext context;
    context.decode_map_regions=true;context.decode_teams=true;
    context.decode_all_players=true;context.decode_supporting_classes=true;
    context.all_players=saves::AllPlayersContext{};
    auto imported=saves::read_saved_session(path,context);
    auto state=empty_state();state.world=std::move(imported.snapshot);
    state.setup.profile=content::TechnologyProfile::bnw;
    rebuild_bnw_live_plot_visibility(state);
    const auto request=find_candidate(state);
    const auto before=state.world;
    const auto sight=preflight_bnw_unit_sight(state,request);
    check(sight.candidate && !sight.can_commit() &&
          sight.owner==request.owner && sight.unit_id==request.unit_id &&
          sight.team>=0 && sight.team<64 && sight.unit_type>=0 &&
          sight.from_plot!=sight.to_plot &&
          sight.from_center_visibility==
              bnw_plot_visibility_count(state,sight.from_plot,sight.team) &&
          sight.to_center_visibility==
              bnw_plot_visibility_count(state,sight.to_plot,sight.team) &&
          sight.from_center_visibility>1 && sight.to_center_visibility>1,
          "BNW sight preflight did not bind decoded unit/team/visibility");
    for(const auto required:{BnwSightBlocker::unit_sight_range_rules,
                             BnwSightBlocker::terrain_and_feature_line_of_sight,
                             BnwSightBlocker::reveal_meeting_and_exploration_effects,
                             BnwSightBlocker::ui_and_derived_cache_effects,
                             BnwSightBlocker::visibility_checkpoint})
        check(std::find(sight.blockers.begin(),sight.blockers.end(),required)!=
                  sight.blockers.end(),"BNW sight preflight omitted a required blocker");
    auto stale=state;stale.plot_visibility.current=false;
    check(!preflight_bnw_unit_sight(stale,request).candidate,
          "BNW sight accepted stale visibility cache");
    stale=state;
    for(auto &array:stale.world.map_plots->plots[sight.from_plot].arrays)
        if(array.name && std::string_view(array.name)=="visibility")
            ++array.values[static_cast<std::size_t>(sight.team)];
    check(!preflight_bnw_unit_sight(stale,request).candidate,
          "BNW sight accepted changed raw visibility with stale cache");
    auto wrong_team=state;
    wrong_team.world.pregame->archive.integer_arrays.at("team_types")
        [static_cast<std::size_t>(request.owner)]=64;
    check(!preflight_bnw_unit_sight(wrong_team,request).candidate,
          "BNW sight accepted inactive team mapping");
    auto wrong_reveal=state;
    auto &cell=wrong_reveal.world.map_projection->cells[sight.to_plot];
    if(cell.revealed_teams->contains(sight.team))
        cell.revealed_teams->erase(sight.team);
    else cell.revealed_teams->insert(sight.team);
    check(!preflight_bnw_unit_sight(wrong_reveal,request).candidate,
          "BNW sight accepted raw/projected reveal disagreement");
    if(sight.see_invisible==-1) {
        auto invisible=state;
        for(auto &player:invisible.world.all_players->players)
            for(auto &unit:player.units.units)
                if(core(unit,"m_eOwner")==request.owner &&
                   core(unit,"m_iID")==request.unit_id)
                    set_core(unit,"m_eSeeInvisibleType",0);
        const auto special=preflight_bnw_unit_sight(invisible,request);
        check(special.candidate &&
              std::find(special.blockers.begin(),special.blockers.end(),
                  BnwSightBlocker::invisible_visibility_effects)!=
                      special.blockers.end(),
              "BNW sight omitted invisible visibility service");
    }
    const BnwSetXYRequest no_move{request.owner,request.unit_id,
        static_cast<std::int64_t>(state.world.map_header.grid.coordinates(sight.from_plot)[0]),
        static_cast<std::int64_t>(state.world.map_header.grid.coordinates(sight.from_plot)[1])};
    check(!preflight_bnw_unit_sight(state,no_move).candidate,
          "BNW sight accepted unsupported setXY candidate");
    check(raw_array(state.world.map_plots->plots[sight.from_plot],"visibility")==
              raw_array(before.map_plots->plots[sight.from_plot],"visibility") &&
          raw_array(state.world.map_plots->plots[sight.to_plot],"revealed_words")==
              raw_array(before.map_plots->plots[sight.to_plot],"revealed_words"),
          "Read-only BNW sight preflight mutated map visibility");
    std::cout << "selected BNW sight candidate " << sight.owner << ':' <<
        sight.unit_id << " team=" << sight.team << " centers=" <<
        sight.from_center_visibility << ',' << sight.to_center_visibility << '\n';
}
} // namespace

int main(int argc,char **argv) {
    try {
        check(argc==2,"Supply selected Gandhi BNW save path");
        selected(argv[1]);
        std::cout << "PASS BNW sight fail-closed preflight\n";
        return 0;
    } catch(const std::exception &error) {
        std::cerr << "FAIL BNW sight preflight: " << error.what() << '\n';
        return 1;
    }
}
