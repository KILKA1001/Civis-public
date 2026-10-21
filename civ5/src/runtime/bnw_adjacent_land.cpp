#include "civ5/runtime/bnw_adjacent_land.hpp"
#include "civ5/runtime/live_session.hpp"
#include "civ5/runtime/map_geometry.hpp"
#include <stdexcept>
#include <type_traits>

namespace civ5::runtime {
namespace {
constexpr auto bnw=saves::GameStateLayout::expansion2_version1;
constexpr std::int64_t ocean=3; // CvPlot::isWater: PLOT_OCEAN
static_assert(std::is_nothrow_move_assignable_v<BnwAdjacentLandCache>);
void require(bool ok,const char *message) {
    if(!ok) throw std::runtime_error(message);
}
struct Source {
    MapGeometry geometry;
    std::vector<std::int64_t> types;
};
Source collect(const LiveSessionState &state) {
    const auto &world=state.world;
    require(world.game_state.layout==bnw && world.map_header.layout==bnw &&
            world.map_plots && world.map_plots->layout==bnw &&
            world.map_projection &&
            state.setup.profile==content::TechnologyProfile::bnw,
            "BNW adjacent-land cache requires a complete owned BNW map");
    const auto &map=*world.map_projection;
    MapGeometry geometry{map.grid,map.wrap_x,map.wrap_y,bnw};
    geometry.validate();
    require(map.grid.width==world.map_header.grid.width &&
            map.grid.height==world.map_header.grid.height &&
            map.wrap_x==world.map_header.wrap_x &&
            map.wrap_y==world.map_header.wrap_y &&
            map.cells.size()==map.grid.size() &&
            world.map_plots->plots.size()==map.cells.size(),
            "BNW adjacent-land map shape disagrees with saved header");
    Source result{geometry,{}};
    result.types.reserve(map.cells.size());
    for(std::size_t i=0;i<map.cells.size();++i) {
        const auto &cell=map.cells[i];
        const auto &saved=world.map_plots->plots[i];
        const auto xy=map.grid.coordinates(i);
        require(cell.index==i && cell.x==xy[0] && cell.y==xy[1] &&
                saved.index==i && saved.x==xy[0] && saved.y==xy[1] &&
                cell.plot_type && *cell.plot_type>=0 && *cell.plot_type<=3,
                "BNW adjacent-land plot identity/category unresolved");
        result.types.push_back(*cell.plot_type);
    }
    return result;
}
} // namespace

void rebuild_bnw_adjacent_land(LiveSessionState &state) {
    state.adjacent_land.current=false;
    const auto source=collect(state);
    BnwAdjacentLandCache replacement;
    replacement.grid=source.geometry.grid;
    replacement.wrap_x=source.geometry.wrap_x;
    replacement.wrap_y=source.geometry.wrap_y;
    replacement.plot_types=source.types;
    replacement.adjacent_land.reserve(source.types.size());
    for(std::size_t i=0;i<source.types.size();++i) {
        const auto xy=source.geometry.grid.coordinates(i);
        bool adjacent=false;
        for(std::int64_t direction=0;direction<6;++direction) {
            const auto neighbor=source.geometry.direction(
                static_cast<std::int64_t>(xy[0]),
                static_cast<std::int64_t>(xy[1]),direction);
            if(neighbor && source.types.at(static_cast<std::size_t>(*neighbor))!=ocean) {
                adjacent=true;
                break;
            }
        }
        replacement.adjacent_land.push_back(adjacent);
    }
    replacement.current=true;
    state.adjacent_land=std::move(replacement);
}

const BnwAdjacentLandCache &bnw_adjacent_land_cache(const LiveSessionState &state) {
    const auto &cache=state.adjacent_land;
    require(cache.current,"BNW adjacent-land cache is not current");
    const auto source=collect(state);
    require(cache.grid.width==source.geometry.grid.width &&
            cache.grid.height==source.geometry.grid.height &&
            cache.wrap_x==source.geometry.wrap_x &&
            cache.wrap_y==source.geometry.wrap_y &&
            cache.plot_types==source.types &&
            cache.adjacent_land.size()==source.types.size(),
            "BNW adjacent-land cache became stale");
    return cache;
}

bool bnw_adjacent_land(const LiveSessionState &state,std::uint64_t plot) {
    const auto &cache=bnw_adjacent_land_cache(state);
    if(plot>=cache.adjacent_land.size())
        throw std::out_of_range("BNW adjacent-land plot outside map");
    return cache.adjacent_land[static_cast<std::size_t>(plot)]!=0;
}

} // namespace civ5::runtime
