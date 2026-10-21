#include "civ5/runtime/bnw_unit_sight_reveal_details.hpp"
#include "civ5/runtime/bnw_live_plot_visibility.hpp"
#include "civ5/runtime/map_geometry.hpp"
#include <climits>
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
            require(!found,"Duplicate BNW sight detail field");found=&item;
        }
    require(found,"Missing BNW sight detail field");return found->value;
}
std::vector<std::int64_t> &array(
    std::vector<saves::PlotArrayObservation> &arrays,
    std::string_view wanted,std::size_t width) {
    saves::PlotArrayObservation *found=nullptr;
    for(auto &item:arrays)
        if(name(item.name)==wanted) {
            require(!found,"Duplicate BNW sight detail array");found=&item;
        }
    require(found && found->columns==1 && found->values.size()==width,
            "BNW sight detail array dimensions changed");
    return found->values;
}
std::int64_t city_field(const saves::CityCoreObservation &core,
                        std::string_view wanted) {
    const saves::CityCoreField *found=nullptr;
    for(const auto &field:core.fields)
        if(name(field.name)==wanted) {
            require(!found,"Duplicate BNW sight city scalar");found=&field;
        }
    require(found && found->encoding==saves::CityCoreEncoding::i32 &&
            found->values.size()==1,"BNW sight city scalar is unavailable");
    return found->values.front();
}
std::vector<std::int64_t> &city_revealed(saves::CityCoreObservation &core) {
    saves::CityCoreField *found=nullptr;
    for(auto &field:core.fields)
        if(name(field.name)=="m_abRevealed") {
            require(!found,"Duplicate BNW city reveal vector");found=&field;
        }
    require(found && found->encoding==saves::CityCoreEncoding::vector_bool &&
            found->values.size()==80,
            "BNW city reveal vector is unavailable or wrong width");
    for(const auto value:found->values)
        require(value==0 || value==1,"BNW city reveal bool is invalid");
    return found->values;
}
void known_type_id(const content::InfoTypeCatalog &types,
                   std::string_view table,std::int64_t id) {
    if(id==-1) return;
    require(id>=0 && id<=INT32_MAX && types.report().all_succeeded(),
            "BNW sight rule catalog or type ID is invalid");
    bool found=false;
    for(const auto &[_,entry]:types.entries())
        if(entry.table==table && entry.id==id) {
            require(!found,"BNW sight rule type ID is ambiguous");found=true;
        }
    require(found,"BNW sight rule type ID absent from active catalog");
}
void known_improvement_hash(const content::InfoTypeCatalog &types,
                            std::int64_t raw) {
    require(raw>=0 && raw<=UINT32_MAX && types.report().all_succeeded(),
            "BNW sight improvement hash is outside wire range");
    if(!raw) return;
    const auto *entry=types.find_hash(static_cast<std::uint32_t>(raw));
    require(entry && entry->table=="Improvements" &&
            entry->hash==static_cast<std::uint32_t>(raw),
            "BNW sight improvement hash absent from active catalog");
}
void found_territory(LiveSessionState &copy,std::int64_t team,
                     std::int64_t owner) {
    if(owner==-1) return;
    require(owner>=0 && owner<64 && copy.world.teams &&
            static_cast<std::size_t>(team)<copy.world.teams->teams.size(),
            "BNW sight territory owner/team ID is invalid");
    auto &saved=copy.world.teams->teams.at(static_cast<std::size_t>(team));
    require(saved.index==static_cast<std::uint64_t>(team),
            "BNW sight territory team identity changed");
    auto &flags=array(saved.arrays,"found_player_territory",64);
    for(const auto value:flags)
        require(value==0 || value==1,"BNW found-territory bool is invalid");
    flags[static_cast<std::size_t>(owner)]=1;
}
void known_owner(LiveSessionState &copy,std::uint64_t plot_index,
                 std::int64_t team,std::int64_t owner) {
    auto &plot=copy.world.map_plots->plots.at(plot_index);
    const auto &cell=copy.world.map_projection->cells.at(plot_index);
    const auto actual_owner=field(plot.fields,"owner");
    require(plot.index==plot_index && cell.index==plot_index &&
            cell.owner && *cell.owner==actual_owner &&
            actual_owner>=-1 && actual_owner<64 &&
            owner>=-1 && owner<64,
            "BNW sight known owner raw/projected identity changed");
    auto &owners=array(plot.arrays,"revealed_owners",80);
    const auto prior=owners.at(static_cast<std::size_t>(team));
    require(prior>=-1 && prior<64,
            "BNW sight known owner byte is invalid");
    owners[static_cast<std::size_t>(team)]=owner;
    // CvPlot::setRevealedOwner calls this only on change. setRevealed itself
    // also calls it unconditionally for an owned plot, so setting true is
    // idempotent and matches the final owned state.
    if(prior!=owner) found_territory(copy,team,owner);
}
void city(LiveSessionState &copy,const BnwSightRevealEdge &edge,
          std::int64_t team) {
    if(!edge.meet_city_candidate) return;
    const auto handle=*edge.meet_city_candidate;
    require(handle.owner>=0 && handle.owner<64 && handle.id>=0 &&
            copy.world.all_players,
            "BNW sight city reference is invalid");
    saves::SavedCityObservation *found=nullptr;
    for(auto &player:copy.world.all_players->players)
        if(player.saved_player_id==handle.owner)
            for(auto &candidate:player.cities.cities)
                if(city_field(candidate.core,"m_iID")==handle.id) {
                    require(!found,"Duplicate BNW sight city handle");
                    found=&candidate;
                }
    require(found && city_field(found->core,"m_eOwner")==handle.owner,
            "BNW sight city handle absent from owned world");
    const auto xy=copy.world.map_header.grid.coordinates(edge.plot);
    require(city_field(found->core,"m_iX")==static_cast<std::int64_t>(xy[0]) &&
            city_field(found->core,"m_iY")==static_cast<std::int64_t>(xy[1]),
            "BNW sight city coordinates disagree with plot");
    city_revealed(found->core).at(static_cast<std::size_t>(team))=1;
}
void revealed_details(LiveSessionState &copy,const BnwSightRevealEdge &edge,
                      std::int64_t team,const content::InfoTypeCatalog &types) {
    auto &plot=copy.world.map_plots->plots.at(edge.plot);
    const auto &cell=copy.world.map_projection->cells.at(edge.plot);
    require(plot.index==edge.plot && cell.index==edge.plot &&
            cell.revealed_teams && cell.revealed_teams->contains(team) &&
            cell.owner && *cell.owner==edge.plot_owner &&
            plot.version,
            "BNW sight revealed plot views or version changed");
    // Expansion2 CvPlot.cpp:8331: even a previously revealed plot refreshes
    // owner, improvement, route and city on a new 0->1 visibility edge.
    found_territory(copy,team,edge.plot_owner);
    known_owner(copy,edge.plot,team,edge.plot_owner);
    if(*plot.version>=6) {
        const auto actual=field(plot.fields,"improvement_type_hash");
        require(cell.improvement_type_hash &&
                *cell.improvement_type_hash==static_cast<std::uint64_t>(actual),
                "BNW sight improvement raw/projected hash disagrees");
        known_improvement_hash(types,actual);
        auto &values=array(plot.arrays,"revealed_improvement_hashes",80);
        known_improvement_hash(types,values.at(static_cast<std::size_t>(team)));
        values[static_cast<std::size_t>(team)]=actual;
    } else {
        const auto actual=field(plot.fields,"improvement_type");
        require(cell.improvement_type && *cell.improvement_type==actual,
                "BNW sight improvement raw/projected ID disagrees");
        known_type_id(types,"Improvements",actual);
        auto &values=array(plot.arrays,"revealed_improvements",80);
        known_type_id(types,"Improvements",values.at(static_cast<std::size_t>(team)));
        values[static_cast<std::size_t>(team)]=actual;
    }
    const auto route=field(plot.fields,"route_type");
    require(cell.route && cell.route->type==route,
            "BNW sight route raw/projected ID disagrees");
    known_type_id(types,"Routes",route);
    auto &routes=array(plot.arrays,"revealed_routes",80);
    known_type_id(types,"Routes",routes.at(static_cast<std::size_t>(team)));
    routes[static_cast<std::size_t>(team)]=route;
    city(copy,edge,team);
}
bool visible(const BnwLivePlotVisibility &cache,std::uint64_t plot,
             std::int64_t team) {
    require(cache.current && plot<cache.grid.size() && team>=0 && team<64,
            "BNW sight adjacent visibility cache is invalid");
    return cache.counts.at(static_cast<std::size_t>(plot*cache.wire_slots+team))>0;
}
void adjacent_owner(LiveSessionState &copy,
                    const BnwSightRevealEdge &edge,std::int64_t team,
                    const MapGeometry &geometry) {
    for(const auto neighbor:edge.adjacent_owner_refresh) {
        require(neighbor<copy.world.map_header.grid.size(),
                "BNW sight adjacent plot ID is invalid");
        bool known=visible(copy.plot_visibility,neighbor,team);
        if(!known) {
            const auto xy=copy.world.map_header.grid.coordinates(neighbor);
            for(std::int64_t direction=0;direction<6;++direction) {
                const auto candidate=geometry.direction(
                    static_cast<std::int64_t>(xy[0]),
                    static_cast<std::int64_t>(xy[1]),direction);
                if(candidate && visible(copy.plot_visibility,
                                        static_cast<std::uint64_t>(*candidate),team)) {
                    known=true;break;
                }
            }
        }
        const auto owner=known?field(
            copy.world.map_plots->plots.at(neighbor).fields,"owner"):-1;
        known_owner(copy,neighbor,team,owner);
    }
}
}

BnwSightRevealDetailsResult apply_bnw_unit_sight_reveal_details(
    const LiveSessionState &state,const BnwSetXYRequest &request,
    const content::RulesDatabase &database,const content::InfoTypeCatalog &types,
    const content::GlobalDefineCache &defines,
    const SessionCheckpointIdentity &identity) {
    BnwSightRevealDetailsResult result;
    try {
        result.sight=apply_bnw_unit_sight_detached(
            state,request,database,types,defines,identity);
        require(result.sight.calculated && result.sight.detached,
                "BNW sight details require a valid detached counter/reveal state");
        auto &copy=*result.sight.detached;
        const auto team=result.sight.plan.team;
        require(team>=0 && team<64,"BNW sight detail team is invalid");
        (void)bnw_live_plot_visibility_cache(copy);
        MapGeometry geometry{copy.world.map_header.grid,
            copy.world.map_header.wrap_x,copy.world.map_header.wrap_y,
            saves::GameStateLayout::expansion2_version1};
        geometry.validate();
        for(const auto &edge:result.sight.plan.newly_visible) {
            revealed_details(copy,edge,team,types);
            adjacent_owner(copy,edge,team,geometry);
        }
        result.calculated=true;
        result.blockers={BnwSightDetailBlocker::sdk_ordered_meet_and_diplomacy,
                         BnwSightDetailBlocker::exploration_and_natural_wonder,
                         BnwSightDetailBlocker::lua_notifications_and_ui,
                         BnwSightDetailBlocker::full_set_xy_effects,
                         BnwSightDetailBlocker::visibility_and_world_checkpoint};
        result.reason="Detached BNW known-reveal state; remaining SDK effects block setXY";
    } catch(const std::exception &error) {
        result.calculated=false;
        result.sight.detached.reset();
        result.blockers={BnwSightDetailBlocker::full_set_xy_effects};
        result.reason=error.what();
    }
    return result;
}
} // namespace civ5::runtime
