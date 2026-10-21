#include "civ5/runtime/bnw_unit_sight_reveal_plan.hpp"
#include "civ5/runtime/live_session.hpp"
#include "civ5/runtime/map_geometry.hpp"
#include <algorithm>
#include <climits>
#include <map>
#include <stdexcept>
#include <string_view>

namespace civ5::runtime {
namespace {
void require(bool ok,const char *why) {if(!ok) throw std::runtime_error(why);}
std::string_view name(const char *value) {
    return value?std::string_view(value):std::string_view();
}
std::int64_t field(const std::vector<saves::NamedSaveInteger> &fields,
                   std::string_view wanted) {
    const saves::NamedSaveInteger *found=nullptr;
    for(const auto &item:fields)
        if(name(item.name)==wanted) {
            require(!found,"Duplicate BNW reveal field");found=&item;
        }
    require(found,"Missing BNW reveal field");return found->value;
}
const saves::PlotArrayObservation &array(
    const std::vector<saves::PlotArrayObservation> &arrays,
    std::string_view wanted,std::size_t width,std::uint64_t columns=1) {
    const saves::PlotArrayObservation *found=nullptr;
    for(const auto &item:arrays)
        if(name(item.name)==wanted) {
            require(!found,"Duplicate BNW reveal array");found=&item;
        }
    require(found && found->columns==columns && found->values.size()==width,
            "BNW reveal array dimensions or content are unavailable");
    return *found;
}
bool revealed(const saves::PlotObservation &plot,
              const saves::SavedMapCellProjection &cell,std::int64_t team) {
    require(team>=0 && team<64 && cell.revealed_teams,
            "BNW reveal team or projected bitset is unavailable");
    const auto &words=array(plot.arrays,"revealed_words",4).values;
    const auto word=static_cast<std::size_t>(team/32);
    require(words[word]>=0 && words[word]<=UINT32_MAX,
            "BNW reveal word is outside u32 range");
    const bool raw=(static_cast<std::uint32_t>(words[word]) &
                    (std::uint32_t{1}<<(team%32)))!=0;
    require(raw==cell.revealed_teams->contains(team),
            "BNW raw/projected reveal bits disagree");
    return raw;
}
const saves::MapRegionRecord &area(const saves::SavedSessionSnapshot &world,
                                   std::int64_t id) {
    require(world.map_regions &&
            world.map_regions->layout==saves::GameStateLayout::expansion2_version1,
            "BNW area reveal state is not decoded");
    const saves::MapRegionContainer *areas=nullptr;
    for(const auto &container:world.map_regions->containers)
        if(name(container.name)=="areas") {
            require(!areas,"Duplicate BNW area container");areas=&container;
        }
    require(areas,"BNW area container is unavailable");
    const saves::MapRegionRecord *found=nullptr;
    for(const auto &record:areas->records)
        if(field(record.fields,"id")==id) {
            require(!found,"Duplicate BNW area ID");found=&record;
        }
    require(found,"BNW plot area ID is not present in owned area records");
    return *found;
}
std::vector<saves::SavedPlotUnitReference> units(
    const saves::PlotObservation &plot,const saves::SavedMapCellProjection &cell) {
    const saves::PlotArrayObservation *found=nullptr;
    for(const auto &item:plot.arrays)
        if(name(item.name)=="units") {
            require(!found,"Duplicate BNW plot unit roster");found=&item;
        }
    require(found && found->columns==2 && found->values.size()%2==0 && cell.units,
            "BNW raw/projected plot unit roster is unavailable");
    std::vector<saves::SavedPlotUnitReference> result;
    for(std::size_t i=0;i<found->values.size();i+=2)
        result.push_back({found->values[i],found->values[i+1]});
    require(result==*cell.units,"BNW raw/projected unit roster disagrees");
    return result;
}
}

BnwUnitSightRevealPlan plan_bnw_unit_sight_reveal(
    const LiveSessionState &state,const BnwSetXYRequest &request,
    const content::RulesDatabase &database,const content::InfoTypeCatalog &types,
    const content::GlobalDefineCache &defines,
    const SessionCheckpointIdentity &identity) {
    BnwUnitSightRevealPlan result;
    try {
        result.counters=plan_bnw_unit_sight(
            state,request,database,types,defines,identity);
        require(result.counters.calculated,
                "BNW reveal needs a valid rule-bound sight counter plan");
        const auto &world=state.world;
        require(world.map_plots && world.map_projection &&
                world.map_plots->plots.size()==world.map_header.grid.size() &&
                world.map_projection->cells.size()==world.map_header.grid.size(),
                "BNW reveal map views are not synchronized");
        const auto team=result.counters.team;
        MapGeometry geometry{world.map_header.grid,world.map_header.wrap_x,
            world.map_header.wrap_y,saves::GameStateLayout::expansion2_version1};
        geometry.validate();
        std::map<std::int64_t,std::pair<std::int64_t,std::int64_t>> area_increments;
        for(const auto &delta:result.counters.plots) {
            // CvPlot::changeVisibilityCount calls setRevealed and meet only on
            // zero-to-positive transitions. The old sight phase only removes.
            if(!(delta.after_old==0 && delta.after_new>0)) continue;
            require(delta.new_calls==1 && delta.plot<world.map_plots->plots.size(),
                    "BNW reveal visibility edge is inconsistent");
            const auto &plot=world.map_plots->plots.at(delta.plot);
            const auto &cell=world.map_projection->cells.at(delta.plot);
            require(plot.index==delta.plot && cell.index==delta.plot &&
                    plot.x==cell.x && plot.y==cell.y,
                    "BNW reveal plot identity disagrees across views");
            BnwSightRevealEdge edge;
            edge.plot=delta.plot;
            edge.first_reveal=!revealed(plot,cell,team);
            edge.plot_owner=field(plot.fields,"owner");
            require(cell.owner && *cell.owner==edge.plot_owner,
                    "BNW reveal raw/projected plot owner disagrees");
            edge.meet_unit_candidates=units(plot,cell);
            const auto city_owner=field(plot.fields,"plot_city_owner");
            const auto city_id=field(plot.fields,"plot_city_id");
            require((city_owner==-1)==(city_id==-1) && cell.plot_city &&
                    cell.plot_city->owner==city_owner &&
                    cell.plot_city->id==city_id,
                    "BNW reveal raw/projected city reference disagrees");
            if(city_owner!=-1)
                edge.meet_city_candidate=saves::SavedPlotCityReference{city_owner,city_id};
            if(edge.first_reveal) {
                edge.area=field(plot.fields,"area");
                require(edge.area>=-1,"BNW reveal plot area ID is invalid");
                if(edge.area>=0) {
                    const auto &record=area(world,edge.area);
                    const auto &counts=array(record.arrays,"revealed_tiles",80).values;
                    edge.area_revealed_before=counts.at(static_cast<std::size_t>(team));
                    require(edge.area_revealed_before>=0 &&
                            edge.area_revealed_before<INT32_MAX,
                            "BNW area reveal count cannot be incremented");
                    auto [it,inserted]=area_increments.try_emplace(
                        edge.area,edge.area_revealed_before,0);
                    require(inserted || it->second.first==edge.area_revealed_before,
                            "BNW area reveal baseline changed during planning");
                    require(it->second.second<INT32_MAX-it->second.first,
                            "BNW cumulative area reveal count overflows i32");
                    ++it->second.second;
                }
            }
            // SDK refreshes six neighboring owners after setRevealed, even
            // when this team had revealed the central plot earlier.
            for(std::int64_t direction=0;direction<6;++direction) {
                const auto neighbor=geometry.direction(
                    static_cast<std::int64_t>(plot.x),
                    static_cast<std::int64_t>(plot.y),direction);
                if(neighbor) edge.adjacent_owner_refresh.push_back(
                    static_cast<std::uint64_t>(*neighbor));
            }
            result.newly_visible.push_back(std::move(edge));
        }
        result.calculated=true;result.team=team;
        result.blockers=result.counters.blockers;
        result.reason="BNW reveal/meet inputs classified; world, diplomacy, Lua and UI effects remain";
    } catch(const std::exception &error) {
        result.calculated=false;result.newly_visible.clear();
        result.blockers={BnwSightBlocker::unsupported_candidate};
        result.reason=error.what();
    }
    return result;
}
} // namespace civ5::runtime
