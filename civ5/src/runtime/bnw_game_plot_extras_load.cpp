#include "civ5/runtime/bnw_game_plot_extras_load.hpp"
#include "civ5/runtime/live_session.hpp"
#include "civ5/runtime/file_identity.hpp"
#include <climits>
#include <stdexcept>

namespace civ5::runtime {
namespace {
void require(bool condition,const char *why) {
    if(!condition) throw std::runtime_error(why);
}
bool same(const saves::PlotYieldObservation &a,
          const saves::PlotYieldObservation &b) {
    return a.plot.x==b.plot.x && a.plot.y==b.plot.y &&
           a.yields==b.yields;
}
bool same(const saves::PlotCostObservation &a,
          const saves::PlotCostObservation &b) {
    return a.plot.x==b.plot.x && a.plot.y==b.plot.y &&
           a.cost==b.cost;
}
void validate(const BnwGamePlotExtras &state) {
    // Expansion2 has six fixed YieldTypes. getPlotExtraYield indexes the
    // stored row directly; an unknown row length would make that unsafe.
    for(const auto &entry:state.yields) {
        require(entry.plot.x>=INT32_MIN && entry.plot.x<=INT32_MAX &&
                entry.plot.y>=INT32_MIN && entry.plot.y<=INT32_MAX &&
                entry.yields.size()==6,
                "BNW plot-extra yield row has unsupported coordinates/dimension");
        for(const auto value:entry.yields)
            require(value>=INT32_MIN && value<=INT32_MAX,
                    "BNW plot-extra yield exceeds i32 wire range");
    }
    for(const auto &entry:state.costs)
        require(entry.plot.x>=INT32_MIN && entry.plot.x<=INT32_MAX &&
                entry.plot.y>=INT32_MIN && entry.plot.y<=INT32_MAX &&
                entry.cost>=INT32_MIN && entry.cost<=INT32_MAX,
                "BNW plot-extra cost exceeds source wire range");
}
} // namespace

std::int64_t BnwGamePlotExtras::extra_yield(std::int64_t x,std::int64_t y,
                                             std::int64_t yield) const {
    require(x>=INT32_MIN && x<=INT32_MAX &&
            y>=INT32_MIN && y<=INT32_MAX && yield>=0 && yield<6,
            "BNW getPlotExtraYield requires i32 coordinates and a valid YieldTypes index");
    for(const auto &entry:yields)
        if(entry.plot.x==x && entry.plot.y==y) {
            require(entry.yields.size()==6,
                    "BNW plot-extra yield context became dirty");
            return entry.yields[static_cast<std::size_t>(yield)];
        }
    return 0;
}

std::int64_t BnwGamePlotExtras::extra_cost(std::int64_t x,
                                            std::int64_t y) const {
    require(x>=INT32_MIN && x<=INT32_MAX &&
            y>=INT32_MIN && y<=INT32_MAX,
            "BNW getPlotExtraCost requires i32 coordinates");
    for(const auto &entry:costs)
        if(entry.plot.x==x && entry.plot.y==y) return entry.cost;
    return 0;
}

BnwGamePlotExtras restore_bnw_game_plot_extras(
    const LiveSessionState &state,
    const saves::SavedSession &source,
    const SessionCheckpointIdentity &identity) {
    require(state.setup.profile==content::TechnologyProfile::bnw &&
            state.world.game_state.layout==
                saves::GameStateLayout::expansion2_version1 &&
            state.world.history.layout==
                saves::GameStateLayout::expansion2_version1 &&
            source.snapshot.history.layout==state.world.history.layout &&
            identify_bytes(source.source.bytes).sha256==identity.payload_sha256 &&
            source.snapshot.history.start_offset==state.world.history.start_offset &&
            source.snapshot.history.end_offset==state.world.history.end_offset &&
            source.snapshot.history.extra_yields.size()==
                state.world.history.extra_yields.size() &&
            source.snapshot.history.extra_costs.size()==
                state.world.history.extra_costs.size(),
            "BNW game plot extras source/history identity changed");
    const auto &raw=source.snapshot.history;
    const auto &world=state.world.history;
    for(std::size_t i=0;i<raw.extra_yields.size();++i)
        require(same(raw.extra_yields[i],world.extra_yields[i]),
                "BNW plot extra yield changed without source-bound checkpoint");
    for(std::size_t i=0;i<raw.extra_costs.size();++i)
        require(same(raw.extra_costs[i],world.extra_costs[i]),
                "BNW plot extra cost changed without source-bound checkpoint");
    BnwGamePlotExtras result{world.extra_yields,world.extra_costs};
    validate(result);
    result.current=true;
    return result;
}

} // namespace civ5::runtime
