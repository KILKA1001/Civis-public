#include "civ5/runtime/bnw_nearby_cities.hpp"
#include "civ5/runtime/live_session.hpp"
#include "civ5/runtime/map_geometry.hpp"
#include <algorithm>
#include <set>
#include <stdexcept>
#include <string_view>
#include <type_traits>

namespace civ5::runtime {
namespace {
static_assert(std::is_nothrow_move_assignable_v<BnwNearbyCityCache>);
void require(bool ok,const char *message) {
    if (!ok) throw std::runtime_error(message);
}
std::int64_t scalar(const saves::CityCoreObservation &core,
                    std::string_view name) {
    const saves::CityCoreField *found=nullptr;
    for (const auto &field:core.fields)
        if (field.name && std::string_view(field.name)==name) {
            require(!found,"Duplicate BNW nearby-city core field");
            found=&field;
        }
    require(found && found->values.size()==1,
            "BNW nearby-city core field unavailable");
    return found->values.front();
}
struct CitySource {
    MapGeometry geometry;
    std::map<BnwCityHandle,BnwCityLocation> locations;
    std::vector<bool> live_is_city;
};
CitySource collect(const LiveSessionState &state) {
    constexpr auto bnw=saves::GameStateLayout::expansion2_version1;
    require(state.world.game_state.layout==bnw &&
            state.world.map_header.layout==bnw &&
            state.world.all_players && state.world.all_players->layout==bnw &&
            state.world.map_projection &&
            state.setup.profile==content::TechnologyProfile::bnw &&
            state.cities.profile==content::TechnologyProfile::bnw,
            "BNW nearby cities require complete owned map/city profiles");
    const auto &map=*state.world.map_projection;
    MapGeometry geometry{map.grid,map.wrap_x,map.wrap_y,bnw};
    geometry.validate();
    require(map.grid.width==state.world.map_header.grid.width &&
            map.grid.height==state.world.map_header.grid.height &&
            map.wrap_x==state.world.map_header.wrap_x &&
            map.wrap_y==state.world.map_header.wrap_y &&
            map.cells.size()==map.grid.size(),
            "BNW nearby-city map projection disagrees with header");
    CitySource result{geometry,{},std::vector<bool>(map.cells.size(),false)};
    std::set<std::int64_t> players;
    for (const auto &saved:state.world.all_players->players) {
        const auto owner=saved.saved_player_id;
        require(owner>=0 && players.insert(owner).second &&
                saved.cities.layout==bnw,
                "BNW nearby-city player identity/profile mismatch");
        const auto live=state.cities.players.find(owner);
        require(live!=state.cities.players.end() &&
                live->second.cities.size()==saved.cities.cities.size(),
                "BNW nearby-city live city roster differs from saved owner");
        for (const auto &city:saved.cities.cities) {
            require(city.core.layout==bnw,
                    "BNW nearby-city city profile mismatch");
            const auto id=scalar(city.core,"m_iID");
            const auto city_owner=scalar(city.core,"m_eOwner");
            const auto x=scalar(city.core,"m_iX");
            const auto y=scalar(city.core,"m_iY");
            require(id>=0 && city_owner==owner && x>=0 && y>=0 &&
                    static_cast<std::uint64_t>(x)<map.grid.width &&
                    static_cast<std::uint64_t>(y)<map.grid.height,
                    "BNW nearby-city city location/owner invalid");
            const BnwCityHandle handle{owner,id};
            const auto found=live->second.cities.find(id);
            require(found!=live->second.cities.end() &&
                    found->second.owner==owner && found->second.id==id &&
                    found->second.x==x && found->second.y==y &&
                    found->second.source_core==std::optional{std::pair{
                        city.core.start_offset,city.core.end_offset}} &&
                    result.locations.emplace(handle,BnwCityLocation{x,y}).second,
                    "BNW nearby-city restored city identity disagrees with live owner");
            const auto index=map.grid.index(static_cast<std::uint64_t>(x),
                                            static_cast<std::uint64_t>(y));
            const auto &cell=map.cells.at(static_cast<std::size_t>(index));
            require(cell.index==index && cell.x==static_cast<std::uint64_t>(x) &&
                    cell.y==static_cast<std::uint64_t>(y) &&
                    cell.plot_city && cell.plot_city->owner==owner &&
                    cell.plot_city->id==id,
                    "BNW nearby-city city plot link is unresolved");
            result.live_is_city[static_cast<std::size_t>(index)]=true;
        }
    }
    for (const auto &[owner,live]:state.cities.players)
        if (!live.cities.empty())
            require(players.contains(owner),
                    "BNW nearby-city live owner absent from decoded world");
    for (std::size_t i=0;i<map.cells.size();++i) {
        const auto &cell=map.cells[i];
        const auto xy=map.grid.coordinates(i);
        require(cell.index==i && cell.x==xy[0] && cell.y==xy[1],
                "BNW nearby-city plot grid identity mismatch");
        if (cell.plot_city) {
            const auto found=result.locations.find(BnwCityHandle{
                cell.plot_city->owner,cell.plot_city->id});
            if (found!=result.locations.end())
                require(found->second.x==static_cast<std::int64_t>(cell.x) &&
                        found->second.y==static_cast<std::int64_t>(cell.y) &&
                        result.live_is_city[i],
                        "BNW city handle occurs on a different plot");
        }
    }
    return result;
}
} // namespace

void rebuild_bnw_nearby_cities(LiveSessionState &state,
                               bool city_roster_current) {
    state.nearby_cities.current=false;
    state.nearby_cities.order_gap.reset();
    require(city_roster_current,
            "BNW nearby-city load requires current city roster");
    const auto source=collect(state);
    BnwNearbyCityCache replacement;
    replacement.grid=source.geometry.grid;
    replacement.wrap_x=source.geometry.wrap_x;
    replacement.wrap_y=source.geometry.wrap_y;
    replacement.locations=source.locations;
    for (const auto &[origin,point]:source.locations) {
        std::vector<std::pair<std::int64_t,BnwCityHandle>> ordered;
        for (const auto &[other,position]:source.locations) {
            if (other==origin) continue;
            ordered.emplace_back(source.geometry.distance(
                point.x,point.y,position.x,position.y),other);
        }
        std::sort(ordered.begin(),ordered.end());
        for (std::size_t i=1;i<ordered.size();++i) {
            if (ordered[i-1].first!=ordered[i].first) continue;
            BnwNearbyCityOrderGap gap{origin,ordered[i].first,{}};
            std::size_t first=i-1,last=i+1;
            while (first>0 && ordered[first-1].first==gap.distance) --first;
            while (last<ordered.size() && ordered[last].first==gap.distance) ++last;
            for (auto k=first;k<last;++k) gap.tied.push_back(ordered[k].second);
            // Diagnostic handles are unordered source operands, not a tie
            // break. No partial proximity lists or plot flags are published.
            replacement.nearby.clear();
            replacement.order_gap=std::move(gap);
            state.nearby_cities=std::move(replacement);
            throw BnwNearbyCityOrderUnavailable(*state.nearby_cities.order_gap);
        }
        auto &list=replacement.nearby[origin];
        for (const auto &[distance,other]:ordered) {
            (void)distance;
            list.push_back(other);
        }
    }
    replacement.current=true;
    state.nearby_cities=std::move(replacement);
    // CvPlot::isCity resolves m_plotCity against the now-loaded player city
    // objects. BNW does not serialize an is_city flag; the projection's
    // previous false values were observations before this load hook.
    auto &cells=state.world.map_projection->cells;
    for (std::size_t i=0;i<cells.size();++i)
        cells[i].is_city=source.live_is_city[i];
}

void resolve_bnw_city_plot_links(LiveSessionState &state,
                                 bool city_roster_current) {
    require(city_roster_current,
            "BNW city plot load requires current city roster");
    const auto source=collect(state);
    auto &cells=state.world.map_projection->cells;
    // collect validates every reached raw/owned link before these no-throw
    // scalar stores. Nearby distances/order and RNG are independent inputs.
    for (std::size_t i=0;i<cells.size();++i)
        cells[i].is_city=source.live_is_city[i];
}

const std::vector<BnwCityHandle> &bnw_nearby_cities(
    const LiveSessionState &state,BnwCityHandle city) {
    const auto &cache=state.nearby_cities;
    if (!cache.current && cache.order_gap)
        throw BnwNearbyCityOrderUnavailable(*cache.order_gap);
    require(cache.current,"BNW nearby-city cache is not current");
    const auto source=collect(state);
    require(cache.grid.width==source.geometry.grid.width &&
            cache.grid.height==source.geometry.grid.height &&
            cache.wrap_x==source.geometry.wrap_x &&
            cache.wrap_y==source.geometry.wrap_y &&
            cache.locations==source.locations,
            "BNW nearby-city cache became stale after world change");
    const auto &cells=state.world.map_projection->cells;
    for (std::size_t i=0;i<cells.size();++i)
        require(cells[i].is_city==source.live_is_city[i],
                "BNW city plot load cache became stale");
    const auto found=cache.nearby.find(city);
    require(found!=cache.nearby.end(),
            "BNW nearby-city owner/ID unavailable");
    return found->second;
}

} // namespace civ5::runtime
