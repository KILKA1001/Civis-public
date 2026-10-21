#include "civ5/runtime/bnw_unit_visibility_probe.hpp"
#include <algorithm>
#include <stdexcept>
#include <string_view>

namespace civ5::runtime {
namespace {
constexpr auto bnw=saves::GameStateLayout::expansion2_version1;
void require(bool condition,const char *reason) {
    if(!condition) throw std::runtime_error(reason);
}
std::int64_t field(const saves::PlotObservation &plot,std::string_view name) {
    const saves::NamedSaveInteger *found=nullptr;
    for(const auto &entry:plot.fields)
        if(entry.name && std::string_view(entry.name)==name) {
            require(!found,"Duplicate BNW visibility plot field");found=&entry;
        }
    require(found,"Missing BNW visibility plot field");
    return found->value;
}
std::int64_t core(const saves::SavedUnitObservation &unit,std::string_view name) {
    const saves::UnitCoreField *found=nullptr;
    for(const auto &entry:unit.core.fields)
        if(entry.name && std::string_view(entry.name)==name) {
            require(!found,"Duplicate BNW visibility unit field");found=&entry;
        }
    require(found && found->encoding==saves::UnitCoreEncoding::i32 &&
            found->values.size()==1,"Missing BNW visibility unit field");
    return found->values.front();
}
const saves::UnitTailNode *tail_node(const saves::UnitTailNode &node,
                                    std::string_view name) {
    if(node.name && std::string_view(node.name)==name) return &node;
    const saves::UnitTailNode *found=nullptr;
    for(const auto &child:node.children)
        if(const auto *entry=tail_node(child,name)) {
            require(!found,"Duplicate BNW visibility tail field");found=entry;
        }
    return found;
}
std::int64_t tail(const saves::SavedUnitObservation &unit,
                  std::string_view name) {
    const saves::UnitTailNode *found=nullptr;
    for(const auto &node:unit.tail.blocks)
        if(const auto *entry=tail_node(node,name)) {
            require(!found,"Duplicate BNW visibility tail field");found=entry;
        }
    require(found && found->values.size()==1,
            "Missing BNW visibility tail field");
    return found->values.front();
}
const saves::PlotArrayObservation &array(const saves::PlotObservation &plot,
                                        std::string_view name) {
    const saves::PlotArrayObservation *found=nullptr;
    for(const auto &entry:plot.arrays)
        if(entry.name && std::string_view(entry.name)==name) {
            require(!found,"Duplicate BNW visibility plot array");found=&entry;
        }
    require(found,"Missing BNW visibility plot array");return *found;
}
const std::vector<std::int64_t> &team_array(
    const saves::TeamObservation &team,std::string_view name) {
    const saves::PlotArrayObservation *found=nullptr;
    for(const auto &entry:team.arrays)
        if(entry.name && std::string_view(entry.name)==name) {
            require(!found,"Duplicate BNW visibility team array");found=&entry;
        }
    require(found && found->columns==1 && found->values.size()==64,
            "Missing BNW visibility team relation");
    return found->values;
}
const saves::TeamObservation &team(const LiveSessionState &state,std::int64_t id) {
    require(id>=0 && id<64 && state.world.teams &&
            state.world.teams->layout==bnw && state.world.teams->teams.size()==64,
            "BNW visibility team roster unavailable");
    const auto &result=state.world.teams->teams[static_cast<std::size_t>(id)];
    require(result.index==static_cast<std::uint64_t>(id),
            "BNW visibility team identity changed");
    return result;
}
std::int64_t player_team(const LiveSessionState &state,std::int64_t owner) {
    require(owner>=0 && owner<64,"BNW visibility unit owner invalid");
    const auto player=state.setup.players.find(owner);
    require(player!=state.setup.players.end() && player->second.team>=0 &&
            player->second.team<64,"BNW visibility live player team unavailable");
    (void)team(state,player->second.team);
    return player->second.team;
}
const saves::SavedUnitObservation *find_unit(
    const LiveSessionState &state,saves::SavedPlotUnitReference handle) {
    require(state.world.all_players && state.world.all_players->layout==bnw,
            "BNW visibility owned unit roster unavailable");
    const saves::SavedPlayerObservation *owner=nullptr;
    for(const auto &player:state.world.all_players->players)
        if(player.saved_player_id==handle.owner) {
            require(!owner,"Duplicate BNW visibility player ID");owner=&player;
        }
    require(owner,"BNW visibility unit owner missing");
    const saves::SavedUnitObservation *found=nullptr;
    for(const auto &entry:owner->units.units)
        if(core(entry,"m_iID")==handle.id) {
            require(!found,"Duplicate BNW visibility unit ID");found=&entry;
        }
    if(found) require(found->core.layout==bnw && found->tail.layout==bnw &&
                      core(*found,"m_eOwner")==handle.owner,
                      "BNW visibility unit owner or layout changed");
    return found;
}
struct View {
    const LiveSessionState &state;
    const saves::PlotObservation &plot;
    const saves::SavedMapCellProjection &cell;
    std::int64_t owner=-1,x=0,y=0;
    View(const LiveSessionState &source,std::uint64_t index):
        state(source),plot(source.world.map_plots->plots.at(index)),
        cell(source.world.map_projection->cells.at(index)) {
        const auto xy=source.world.map_header.grid.coordinates(index);
        x=xy[0];y=xy[1];owner=field(plot,"owner");
        require(plot.index==index && plot.x==xy[0] && plot.y==xy[1] &&
                cell.index==index && cell.x==xy[0] && cell.y==xy[1] &&
                cell.owner && *cell.owner==owner && cell.units &&
                owner>=-1 && owner<64,
                "BNW visibility raw/live plot identity changed");
        const auto &raw=array(plot,"units");
        require(raw.columns==2 && raw.values.size()==cell.units->size()*2,
                "BNW visibility raw/live unit roster size changed");
        for(std::size_t i=0;i<cell.units->size();++i)
            require(raw.values[2*i]==(*cell.units)[i].owner &&
                    raw.values[2*i+1]==(*cell.units)[i].id,
                    "BNW visibility raw/live unit roster changed");
    }
    const saves::SavedUnitObservation &unit(
        saves::SavedPlotUnitReference handle) const {
        require(std::count(cell.units->begin(),cell.units->end(),handle)==1,
                "BNW visibility plot unit handle unavailable or duplicate");
        const auto *found=find_unit(state,handle);
        require(found && core(*found,"m_iX")==x && core(*found,"m_iY")==y,
                "BNW visibility unit position changed");
        return *found;
    }
};
void validate(const LiveSessionState &state,std::uint64_t index) {
    const auto &world=state.world;
    require(state.setup.profile==content::TechnologyProfile::bnw &&
            world.game_state.layout==bnw && world.map_header.layout==bnw &&
            world.map_plots && world.map_plots->layout==bnw &&
            world.map_projection &&
            world.map_plots->plots.size()==world.map_header.grid.size() &&
            world.map_projection->cells.size()==world.map_header.grid.size() &&
            index<world.map_header.grid.size(),
            "BNW visibility requires a complete owned map");
}
bool invisible(const View &view,saves::SavedPlotUnitReference handle,
               std::int64_t observer) {
    (void)team(view.state,observer);
    const auto &unit=view.unit(handle);
    if(player_team(view.state,handle.owner)==observer) return false;
    // CvUnit::isInvisible checks a valid live cargo transport before type.
    const auto transport_owner=tail(unit,"m_transportUnit.eOwner");
    const auto transport_id=tail(unit,"m_transportUnit.iID");
    if(transport_owner>=0 && transport_owner<64) {
        require(transport_id>=0,"BNW visibility cargo transport ID invalid");
        if(find_unit(view.state,{transport_owner,transport_id})) return true;
    }
    const auto type=core(unit,"m_eInvisibleType");
    if(type==-1) return false;
    const auto &dimensions=view.state.world.map_plots->dimensions;
    // The plot counter wire has 80 fixed MAX_TEAMS slots; live player teams use 0..63.
    require(dimensions.teams==80 && dimensions.invisible_types>0 &&
            type>=0 && static_cast<std::uint64_t>(type)<dimensions.invisible_types &&
            dimensions.invisible_types<=64,
            "BNW invisible type or wire dimensions unresolved");
    const auto &counts=array(view.plot,"invisible_visibility");
    require(counts.columns==1 &&
            counts.values.size()==dimensions.teams*dimensions.invisible_types,
            "BNW invisible detection counters unavailable");
    const auto count=counts.values[static_cast<std::size_t>(observer)*
                                    dimensions.invisible_types+
                                    static_cast<std::size_t>(type)];
    require(count>=0,"BNW invisible detection count is negative");
    return count==0;
}
bool city(const View &view) {
    const auto owner=field(view.plot,"plot_city_owner");
    const auto id=field(view.plot,"plot_city_id");
    require(owner>=-1 && owner<64 &&
            ((owner==-1 && id==-1) || (owner>=0 && id>=0)) &&
            view.cell.plot_city && view.cell.plot_city->owner==owner &&
            view.cell.plot_city->id==id,
            "BNW visibility live city identity unresolved");
    return owner!=-1;
}
bool at_war(const LiveSessionState &state,std::int64_t observer,
            std::int64_t other) {
    const auto &relations=team_array(team(state,observer),"at_war");
    const auto value=relations.at(static_cast<std::size_t>(other));
    require(value==0 || value==1,"BNW visibility war relation is not boolean");
    return value!=0;
}
} // namespace

BnwUnitVisibilityDecision quote_bnw_visible_enemy_unit(
    const LiveSessionState &state,std::uint64_t index,
    saves::SavedPlotUnitReference handle) {
    BnwUnitVisibilityDecision result;
    try {
        validate(state,index);
        const View view(state,index);
        const auto &source=view.unit(handle);
        const auto observer=player_team(state,handle.owner);
        const bool on_city=city(view);
        const auto hostile=core(source,"m_iAlwaysHostileCount");
        require(hostile>=0,"BNW source unit always-hostile count invalid");
        const bool source_hostile=!on_city && hostile>0;
        for(const auto loop:view.cell.units.value()) {
            const auto &unit=view.unit(loop);
            // CvPlot::isVisibleEnemyUnit tests invisibility before isEnemy.
            if(invisible(view,loop,observer)) continue;
            // canCoexistWithEnemyUnit is the same isInvisible query; its
            // result is already proven false on this source branch.
            auto combat_team=player_team(state,loop.owner);
            const auto loop_hostile=core(unit,"m_iAlwaysHostileCount");
            require(loop_hostile>=0,"BNW loop unit always-hostile count invalid");
            if(observer!=combat_team && observer!=63 &&
               loop_hostile>0 && !on_city)
                combat_team=player_team(state,63);
            if(source_hostile ? observer!=combat_team :
               at_war(state,observer,combat_team)) {
                result.value=true;return result;
            }
        }
        result.value=false;
    } catch(const std::exception &error) {result.reason=error.what();}
    return result;
}

BnwUnitVisibilityDecision quote_bnw_unit_invisible(
    const LiveSessionState &state,std::uint64_t index,
    saves::SavedPlotUnitReference handle,std::int64_t observer) {
    BnwUnitVisibilityDecision result;
    try {
        validate(state,index);
        const View view(state,index);
        result.value=invisible(view,handle,observer);
    } catch(const std::exception &error) {result.reason=error.what();}
    return result;
}

BnwUnitValidityProbe make_bnw_visible_enemy_probe() {
    return [](const LiveSessionState &state,std::uint64_t index,
              saves::SavedPlotUnitReference handle) {
        return quote_bnw_visible_enemy_unit(state,index,handle).value;
    };
}
BnwUnitValidityProbe make_bnw_unit_invisible_probe() {
    return [](const LiveSessionState &state,std::uint64_t index,
              saves::SavedPlotUnitReference handle)->std::optional<bool> {
        try {
            validate(state,index);
            const View view(state,index);
            require(view.owner>=0,"BNW visibility second pass needs owned plot");
            return quote_bnw_unit_invisible(
                state,index,handle,player_team(state,view.owner)).value;
        } catch(const std::exception &) {return std::nullopt;}
    };
}

} // namespace civ5::runtime
