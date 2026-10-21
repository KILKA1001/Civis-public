#include "civ5/runtime/bnw_live_plot_visibility.hpp"
#include "civ5/runtime/live_action_transaction.hpp"
#include "civ5/runtime/live_session.hpp"
#include <algorithm>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string_view>

namespace {
using namespace civ5;
using namespace civ5::runtime;
constexpr auto bnw=saves::GameStateLayout::expansion2_version1;
void check(bool ok,const char *message) {
    if(!ok) throw std::runtime_error(message);
}
template<class F> void rejects(F action) {
    try {action();} catch(const std::exception &) {return;}
    throw std::runtime_error("Missing BNW visibility load blocker");
}
LiveSessionState empty_state() {
    return {saves::SavedSessionSnapshot{},ResearchContext{}, {},
        PlayerResearchContext{},CityYieldContext{},BnwUnhappinessContext{},
        MaintenanceContext{},TreasuryContext{},ScienceContext{},ResearchTurnState{},
        {SimulationRandom(RandomAlgorithm::bnw,{0,0,0}),
         SimulationRandom(RandomAlgorithm::bnw,{0,0,0})},
        ResearchSelectionState{},BnwGameClockState{},std::nullopt,std::nullopt,{}, {}, {}, {}, {}, {}};
}
const saves::PlotArrayObservation &raw_visibility(
    const saves::PlotObservation &plot) {
    for(const auto &array:plot.arrays)
        if(array.name && std::string_view(array.name)=="visibility") return array;
    throw std::runtime_error("Selected BNW plot lacks raw visibility");
}
void synthetic() {
    auto state=empty_state();
    state.world.game_state.layout=state.world.map_header.layout=bnw;
    state.world.map_header.grid={2,1};
    auto &map=state.world.map_projection.emplace();map.grid={2,1};
    auto &plots=state.world.map_plots.emplace();
    plots.layout=bnw;plots.dimensions.teams=80;
    auto &teams=state.world.teams.emplace();
    teams.layout=bnw;teams.teams.resize(64);
    for(std::uint64_t i=0;i<2;++i) {
        saves::SavedMapCellProjection cell;cell.index=i;cell.x=i;cell.y=0;
        map.cells.push_back(cell);
        saves::PlotObservation saved;saved.index=i;saved.x=i;saved.y=0;
        saves::PlotArrayObservation raw{"visibility",1,std::vector<std::int64_t>(80,0)};
        raw.values[0]=i==0?-3:2;
        raw.values[1]=i==0?4:-1;
        raw.values[79]=-7;
        saved.arrays.push_back(std::move(raw));
        plots.plots.push_back(std::move(saved));
    }
    rebuild_bnw_live_plot_visibility(state);
    check(bnw_plot_visibility_count(state,0,0)==0 &&
          bnw_plot_visibility_count(state,0,1)==4 &&
          bnw_plot_visibility_count(state,1,0)==2 &&
          bnw_plot_visibility_count(state,1,1)==0 &&
          bnw_plot_is_visible(state,0,1) &&
          !bnw_plot_is_visible(state,0,0) &&
          !bnw_plot_is_visible(state,0,-1),
          "CvPlot::read clamp/isVisible semantics changed");
    check(raw_visibility(state.world.map_plots->plots[0]).values[0]==-3 &&
          raw_visibility(state.world.map_plots->plots[0]).values[79]==-7,
          "Raw BNW visibility source was mutated by load hook");
    rejects([&]{(void)bnw_plot_visibility_count(state,0,64);});
    rejects([&]{(void)bnw_plot_visibility_count(state,0,79);});
    auto changed=state;
    changed.world.map_plots->plots[0].arrays.front().values[0]=-2;
    rejects([&]{(void)bnw_live_plot_visibility_cache(changed);});
    auto bad=state;
    bad.world.map_plots->plots[0].arrays.front().values.pop_back();
    rejects([&]{rebuild_bnw_live_plot_visibility(bad);});
    check(!bad.plot_visibility.current,
          "Failed visibility rebuild retained a current cache");
    auto vp=state;vp.world.game_state.layout=saves::GameStateLayout::vp542_corpus;
    rejects([&]{rebuild_bnw_live_plot_visibility(vp);});
    saves::SavedSession source;source.snapshot=state.world;
    auto session=LiveSession::from_restored_bnw(std::move(source),std::move(state));
    check(raw_visibility(session.source().snapshot.map_plots->plots[0]).values[0]==-3,
          "Immutable BNW source lost raw negative visibility");
    const auto checkpoint=session.checkpoint();
    const auto declined=apply_live_action(session,[](LiveSessionState &) {
        return LiveActionResult{LiveActionOutcome::rejected,"declined"};
    });
    check(declined.outcome==LiveActionOutcome::rejected &&
          session.state().plot_visibility.current,
          "Rejected action invalidated unchanged visibility");
    (void)apply_live_action(session,[](LiveSessionState &) {
        return LiveActionResult{LiveActionOutcome::committed,{}};
    });
    check(!session.state().plot_visibility.current,
          "Committed action retained stale visibility");
    session.restore(checkpoint);
    check(bnw_plot_visibility_count(session.state(),0,0)==0 &&
          bnw_plot_visibility_count(session.state(),0,1)==4,
          "Checkpoint failed to restore clamped visibility");
    (void)session.mutable_state();
    check(!session.state().plot_visibility.current,
          "Arbitrary world edit retained visibility");
}
void selected(const std::filesystem::path &path) {
    saves::SavedSessionDecodeContext context;
    context.decode_map_regions=true;context.decode_teams=true;
    context.decode_all_players=true;context.decode_supporting_classes=true;
    context.all_players=saves::AllPlayersContext{};
    auto source=saves::read_saved_session(path,context);
    auto state=empty_state();state.world=std::move(source.snapshot);
    check(state.world.game_state.layout==bnw && state.world.map_plots &&
          state.world.teams,
          "Selected Gandhi save lacks BNW map/team source");
    rebuild_bnw_live_plot_visibility(state);
    const auto &cache=bnw_live_plot_visibility_cache(state);
    std::size_t positive=0,clamped=0;
    for(std::size_t plot=0;plot<state.world.map_plots->plots.size();++plot) {
        const auto &raw=raw_visibility(state.world.map_plots->plots[plot]);
        for(std::size_t team=0;team<80;++team) {
            const auto index=plot*80+team;
            check(cache.source_raw[index]==raw.values[team] &&
                  cache.counts[index]==std::max<std::int64_t>(0,raw.values[team]),
                  "Selected BNW post-load visibility disagrees with SDK clamp");
            if(team<64) positive+=cache.counts[index]>0;
            clamped+=raw.values[team]<0;
        }
    }
    check(positive>0 && cache.counts.size()==960*80,
          "Selected save did not exercise live visibility across the map");
    std::cout << "selected BNW visible plot/team values=" << positive
              << " raw negatives=" << clamped << '\n';
}
} // namespace

int main(int argc,char **argv) {
    try {
        check(argc==2,"Supply selected Gandhi BNW save path");
        synthetic();
        selected(argv[1]);
        std::cout << "PASS BNW CvPlot visibility read hook\n";
        return 0;
    } catch(const std::exception &error) {
        std::cerr << "FAIL BNW CvPlot visibility read hook: "
                  << error.what() << '\n';
        return 1;
    }
}
