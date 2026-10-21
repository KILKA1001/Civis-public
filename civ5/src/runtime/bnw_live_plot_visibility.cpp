#include "civ5/runtime/bnw_live_plot_visibility.hpp"
#include "civ5/runtime/live_session.hpp"
#include "civ5/runtime/map_geometry.hpp"
#include <stdexcept>
#include <string_view>
#include <type_traits>

namespace civ5::runtime {
namespace {
constexpr auto bnw=saves::GameStateLayout::expansion2_version1;
// CvDLLUtilDefines.h: MAX_TEAMS=64, REALLY_MAX_TEAMS=80.
constexpr std::uint64_t active_teams=64,wire_teams=80;
static_assert(std::is_nothrow_move_assignable_v<BnwLivePlotVisibility>);
void require(bool ok,const char *message) {
    if(!ok) throw std::runtime_error(message);
}
struct Source {
    saves::MapGrid grid;
    std::vector<std::int64_t> raw;
};
Source collect(const LiveSessionState &state) {
    const auto &world=state.world;
    require(world.game_state.layout==bnw && world.map_header.layout==bnw &&
            world.map_plots && world.map_plots->layout==bnw &&
            world.map_projection && world.teams && world.teams->layout==bnw &&
            state.setup.profile==content::TechnologyProfile::bnw,
            "BNW plot visibility requires a complete owned map/team profile");
    const auto &map=*world.map_projection;
    MapGeometry geometry{map.grid,map.wrap_x,map.wrap_y,bnw};
    geometry.validate();
    require(world.map_plots->dimensions.teams==wire_teams &&
            world.teams->dimensions.teams==active_teams &&
            world.teams->teams.size()==active_teams &&
            map.grid.width==world.map_header.grid.width &&
            map.grid.height==world.map_header.grid.height &&
            map.wrap_x==world.map_header.wrap_x &&
            map.wrap_y==world.map_header.wrap_y &&
            map.cells.size()==map.grid.size() &&
            world.map_plots->plots.size()==map.cells.size() &&
            map.cells.size()<=SIZE_MAX/wire_teams,
            "BNW plot visibility source dimensions disagree");
    Source result{map.grid,{}};
    result.raw.reserve(map.cells.size()*wire_teams);
    for(std::size_t i=0;i<map.cells.size();++i) {
        const auto &cell=map.cells[i];
        const auto &saved=world.map_plots->plots[i];
        const auto xy=map.grid.coordinates(i);
        require(cell.index==i && cell.x==xy[0] && cell.y==xy[1] &&
                saved.index==i && saved.x==xy[0] && saved.y==xy[1],
                "BNW plot visibility plot identity mismatch");
        const saves::PlotArrayObservation *visibility=nullptr;
        for(const auto &array:saved.arrays)
            if(array.name && std::string_view(array.name)=="visibility") {
                require(!visibility,"Duplicate BNW plot visibility array");
                visibility=&array;
            }
        require(visibility && visibility->columns==1 &&
                visibility->values.size()==wire_teams,
                "BNW plot visibility source array unavailable");
        result.raw.insert(result.raw.end(),visibility->values.begin(),
                          visibility->values.end());
    }
    return result;
}
std::size_t position(const BnwLivePlotVisibility &cache,
                     std::uint64_t plot,std::int64_t team) {
    if(plot>=cache.grid.size() || team<0 ||
       static_cast<std::uint64_t>(team)>=cache.team_slots)
        throw std::out_of_range("BNW plot visibility plot/team outside live range");
    return static_cast<std::size_t>(plot*cache.wire_slots+
                                    static_cast<std::uint64_t>(team));
}
} // namespace

void rebuild_bnw_live_plot_visibility(LiveSessionState &state) {
    state.plot_visibility.current=false;
    const auto source=collect(state);
    BnwLivePlotVisibility replacement;
    replacement.grid=source.grid;
    replacement.team_slots=active_teams;
    replacement.wire_slots=wire_teams;
    replacement.source_raw=source.raw;
    replacement.counts.reserve(source.raw.size());
    for(const auto raw:source.raw)
        replacement.counts.push_back(raw<0?0:raw);
    replacement.current=true;
    state.plot_visibility=std::move(replacement);
}

const BnwLivePlotVisibility &bnw_live_plot_visibility_cache(
    const LiveSessionState &state) {
    const auto &cache=state.plot_visibility;
    require(cache.current && cache.team_slots==active_teams &&
            cache.wire_slots==wire_teams,
            "BNW plot visibility cache is not current");
    const auto source=collect(state);
    require(cache.grid.width==source.grid.width &&
            cache.grid.height==source.grid.height &&
            cache.source_raw==source.raw &&
            cache.counts.size()==source.raw.size(),
            "BNW plot visibility source changed");
    return cache;
}

std::int64_t bnw_plot_visibility_count(const LiveSessionState &state,
                                       std::uint64_t plot,std::int64_t team) {
    const auto &cache=bnw_live_plot_visibility_cache(state);
    return cache.counts.at(position(cache,plot,team));
}

bool bnw_plot_is_visible(const LiveSessionState &state,
                         std::uint64_t plot,std::int64_t team) {
    if(team==-1) return false; // CvPlot::isVisible(NO_TEAM)
    return bnw_plot_visibility_count(state,plot,team)>0;
}

} // namespace civ5::runtime
