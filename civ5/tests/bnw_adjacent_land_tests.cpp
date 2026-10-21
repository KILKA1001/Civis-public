#include "civ5/runtime/bnw_adjacent_land.hpp"
#include "civ5/runtime/live_action_transaction.hpp"
#include "civ5/runtime/live_session.hpp"
#include <filesystem>
#include <iostream>
#include <stdexcept>

namespace {
using namespace civ5;
using namespace civ5::runtime;
constexpr auto bnw=saves::GameStateLayout::expansion2_version1;
void check(bool ok,const char *message) {
    if(!ok) throw std::runtime_error(message);
}
template<class F> void rejects(F action) {
    try {action();} catch(const std::exception &) {return;}
    throw std::runtime_error("Missing BNW adjacent-land load blocker");
}
LiveSessionState empty_state() {
    return {saves::SavedSessionSnapshot{},ResearchContext{}, {},
        PlayerResearchContext{},CityYieldContext{},BnwUnhappinessContext{},
        MaintenanceContext{},TreasuryContext{},ScienceContext{},ResearchTurnState{},
        {SimulationRandom(RandomAlgorithm::bnw,{0,0,0}),
         SimulationRandom(RandomAlgorithm::bnw,{0,0,0})},
        ResearchSelectionState{},BnwGameClockState{},std::nullopt,std::nullopt,{}, {}, {}, {}, {}, {}};
}
LiveSessionState small_map() {
    auto state=empty_state();
    state.world.game_state.layout=state.world.map_header.layout=bnw;
    state.world.map_header.grid={4,3};
    state.world.map_header.wrap_x=true;
    auto &plots=state.world.map_plots.emplace();plots.layout=bnw;
    auto &map=state.world.map_projection.emplace();
    map.grid={4,3};map.wrap_x=true;
    for(std::uint64_t y=0;y<3;++y)
        for(std::uint64_t x=0;x<4;++x) {
            const auto i=y*4+x;
            saves::SavedMapCellProjection cell;
            cell.index=i;cell.x=x;cell.y=y;
            cell.plot_type=(x==0 && y==1)?2:3;
            map.cells.push_back(cell);
            saves::PlotObservation saved;
            saved.index=i;saved.x=x;saved.y=y;
            plots.plots.push_back(saved);
        }
    return state;
}
void synthetic() {
    auto state=small_map();
    rebuild_bnw_adjacent_land(state);
    check(!bnw_adjacent_land(state,4) &&
          bnw_adjacent_land(state,7) &&
          bnw_adjacent_land(state,0) &&
          !bnw_adjacent_land(state,2),
          "CvMap::updateAdjacency wrap/land rule changed");
    auto changed=state;
    changed.world.map_projection->cells[4].plot_type=3;
    rejects([&]{(void)bnw_adjacent_land(changed,7);});
    auto missing=state;
    missing.world.map_projection->cells[4].plot_type.reset();
    rejects([&]{rebuild_bnw_adjacent_land(missing);});
    check(!missing.adjacent_land.current,
          "Failed adjacency rebuild retained current data");
    auto vp=state;vp.world.game_state.layout=saves::GameStateLayout::vp542_corpus;
    rejects([&]{rebuild_bnw_adjacent_land(vp);});
    rejects([&]{(void)bnw_adjacent_land(state,12);});
    saves::SavedSession source;source.snapshot=state.world;
    auto session=LiveSession::from_restored_bnw(std::move(source),std::move(state));
    const auto checkpoint=session.checkpoint();
    const auto declined=apply_live_action(session,[](LiveSessionState &) {
        return LiveActionResult{LiveActionOutcome::rejected,"declined"};
    });
    check(declined.outcome==LiveActionOutcome::rejected &&
          session.state().adjacent_land.current,
          "Rejected action invalidated unchanged adjacency");
    (void)apply_live_action(session,[](LiveSessionState &) {
        return LiveActionResult{LiveActionOutcome::committed,{}};
    });
    check(!session.state().adjacent_land.current,
          "Committed action retained stale adjacency");
    session.restore(checkpoint);
    check(bnw_adjacent_land(session.state(),7),
          "Checkpoint failed to restore adjacency");
    (void)session.mutable_state();
    check(!session.state().adjacent_land.current,
          "Arbitrary world edit retained adjacency");
}
bool oracle(const saves::SavedMapProjection &map,std::uint64_t x,
            std::uint64_t y) {
    constexpr int even_dx[6]={0,1,0,-1,-1,-1};
    constexpr int odd_dx[6]={1,1,1,0,-1,0};
    constexpr int dy[6]={1,0,-1,-1,0,1};
    const auto width=static_cast<std::int64_t>(map.grid.width);
    const auto height=static_cast<std::int64_t>(map.grid.height);
    for(int d=0;d<6;++d) {
        auto nx=static_cast<std::int64_t>(x)+(y%2?odd_dx[d]:even_dx[d]);
        auto ny=static_cast<std::int64_t>(y)+dy[d];
        if(map.wrap_x) nx=(nx%width+width)%width;
        if(map.wrap_y) ny=(ny%height+height)%height;
        if(nx<0||ny<0||nx>=width||ny>=height) continue;
        const auto &other=map.cells.at(map.grid.index(nx,ny));
        check(other.plot_type.has_value(),"Selected map plot type unavailable");
        if(*other.plot_type!=3) return true;
    }
    return false;
}
void selected(const std::filesystem::path &path) {
    saves::SavedSessionDecodeContext context;
    context.decode_map_regions=true;context.decode_teams=true;
    context.decode_all_players=true;context.decode_supporting_classes=true;
    context.all_players=saves::AllPlayersContext{};
    auto source=saves::read_saved_session(path,context);
    auto state=empty_state();state.world=std::move(source.snapshot);
    check(state.world.game_state.layout==bnw && state.world.map_projection,
          "Selected Gandhi save does not contain a BNW map");
    rebuild_bnw_adjacent_land(state);
    const auto &map=*state.world.map_projection;
    const auto &cache=bnw_adjacent_land_cache(state);
    std::size_t adjacent=0;
    for(std::size_t i=0;i<map.cells.size();++i) {
        const auto xy=map.grid.coordinates(i);
        const auto actual=cache.adjacent_land.at(i)!=0;
        check(actual==oracle(map,xy[0],xy[1]),
              "Selected BNW adjacency differs from independent hex oracle");
        adjacent+=actual;
    }
    check(adjacent>0 && adjacent<map.cells.size(),
          "Selected save did not exercise both adjacency states");
    std::cout << "selected BNW adjacent-land=" << adjacent
              << "/" << map.cells.size() << '\n';
}
} // namespace

int main(int argc,char **argv) {
    try {
        check(argc==2,"Supply selected Gandhi BNW save path");
        synthetic();
        selected(argv[1]);
        std::cout << "PASS BNW CvMap::updateAdjacency load hook\n";
        return 0;
    } catch(const std::exception &error) {
        std::cerr << "FAIL BNW CvMap::updateAdjacency load hook: "
                  << error.what() << '\n';
        return 1;
    }
}
