#include "civ5/runtime/bnw_unit_territory_probe.hpp"
#include <algorithm>
#include <stdexcept>
#include <string_view>

namespace civ5::runtime {
namespace {
constexpr auto bnw=saves::GameStateLayout::expansion2_version1;
void require(bool valid,const char *reason) {
    if(!valid) throw std::runtime_error(reason);
}
std::int64_t field(const saves::PlotObservation &plot,std::string_view name) {
    const saves::NamedSaveInteger *found=nullptr;
    for(const auto &entry:plot.fields)
        if(entry.name && std::string_view(entry.name)==name) {
            require(!found,"Duplicate BNW territory plot field");found=&entry;
        }
    require(found,"Missing BNW territory plot field");
    return found->value;
}
std::int64_t field(const saves::TeamObservation &team,std::string_view name) {
    const saves::NamedSaveInteger *found=nullptr;
    for(const auto &entry:team.fields)
        if(entry.name && std::string_view(entry.name)==name) {
            require(!found,"Duplicate BNW territory team field");found=&entry;
        }
    require(found,"Missing BNW territory team field");
    return found->value;
}
std::int64_t core(const saves::SavedUnitObservation &unit,std::string_view name) {
    const saves::UnitCoreField *found=nullptr;
    for(const auto &entry:unit.core.fields)
        if(entry.name && std::string_view(entry.name)==name) {
            require(!found,"Duplicate BNW territory unit field");found=&entry;
        }
    require(found && found->encoding==saves::UnitCoreEncoding::i32 &&
            found->values.size()==1,"Missing BNW territory unit field");
    return found->values.front();
}
const std::vector<std::int64_t> &array(const saves::TeamObservation &team,
                                      std::string_view name) {
    const saves::PlotArrayObservation *found=nullptr;
    for(const auto &entry:team.arrays)
        if(entry.name && std::string_view(entry.name)==name) {
            require(!found,"Duplicate BNW territory team array");found=&entry;
        }
    require(found && found->columns==1 && found->values.size()==64,
            "Missing BNW territory team relation array");
    return found->values;
}
const saves::TeamObservation &team(const LiveSessionState &state,std::int64_t id) {
    require(state.world.teams && state.world.teams->layout==bnw &&
            state.world.teams->teams.size()==64 && id>=0 && id<64,
            "BNW territory team roster unavailable");
    const auto &result=state.world.teams->teams.at(static_cast<std::size_t>(id));
    require(result.index==static_cast<std::uint64_t>(id) && field(result,"id")==id,
            "BNW territory team identity changed");
    return result;
}
std::int64_t relation(const saves::TeamObservation &owner,
                      std::string_view name,std::int64_t target) {
    require(target>=0 && target<64,"BNW territory relation target invalid");
    const auto value=array(owner,name).at(static_cast<std::size_t>(target));
    require(value==0 || value==1,"BNW territory relation is not a source boolean");
    return value;
}
std::int64_t player_team(const LiveSessionState &state,std::int64_t player) {
    const auto found=state.setup.players.find(player);
    require(found!=state.setup.players.end() && found->second.team>=0 &&
            found->second.team<64,"BNW territory player team unavailable");
    (void)team(state,found->second.team);
    return found->second.team;
}
bool is_minor(const LiveSessionState &state,std::int64_t id) {
    require(state.world.pregame && state.world.pregame->layout==bnw,
            "BNW territory pregame roster unavailable");
    const auto flags=state.world.pregame->archive.flag_arrays.find("minor_nation_civs");
    require(flags!=state.world.pregame->archive.flag_arrays.end() &&
            flags->second.size()==64,"BNW territory minor flags unavailable");
    for(std::int64_t player=22;player<64;++player) {
        const auto found=state.setup.players.find(player);
        require(found!=state.setup.players.end(),"BNW territory player roster incomplete");
        if(found->second.alive && found->second.team==id)
            return flags->second[static_cast<std::size_t>(player)];
    }
    return false;
}
const saves::SavedUnitObservation &unit(const LiveSessionState &state,
    saves::SavedPlotUnitReference handle,std::int64_t x,std::int64_t y) {
    require(state.world.all_players && state.world.all_players->layout==bnw,
            "BNW territory owned unit roster unavailable");
    const saves::SavedPlayerObservation *owner=nullptr;
    for(const auto &player:state.world.all_players->players)
        if(player.saved_player_id==handle.owner) {
            require(!owner,"Duplicate BNW territory unit owner");owner=&player;
        }
    require(owner,"BNW territory unit owner unavailable");
    const saves::SavedUnitObservation *found=nullptr;
    for(const auto &candidate:owner->units.units)
        if(core(candidate,"m_iID")==handle.id) {
            require(!found,"Duplicate BNW territory unit ID");found=&candidate;
        }
    require(found && found->core.layout==bnw &&
            core(*found,"m_eOwner")==handle.owner &&
            core(*found,"m_iX")==x && core(*found,"m_iY")==y,
            "BNW territory unit identity or position changed");
    return *found;
}
std::optional<bool> rival_rule(const content::RulesDatabase *rules,
                               std::int64_t type) {
    if(!rules) return std::nullopt;
    require(type>=0,"BNW territory unit type invalid");
    std::optional<bool> value;
    const auto report=rules->query_rows(
        {"SELECT RivalTerritory FROM Units WHERE ID=?",{type}},
        [&](const auto &row) {
            require(!value,"Duplicate BNW territory unit rule ID");
            const auto raw=row.integer_at(0);
            require(raw==0 || raw==1,"BNW territory rival rule is not boolean");
            value=raw!=0;
        },2);
    require(report.all_succeeded(),"BNW territory unit rival rule lookup failed");
    return value;
}
} // namespace

BnwTerritoryDecision quote_bnw_unit_can_enter_territory(
    const LiveSessionState &state,std::uint64_t index,
    saves::SavedPlotUnitReference handle,const content::RulesDatabase *rules) {
    BnwTerritoryDecision result;
    result.unit_owner=handle.owner;
    try {
        const auto &world=state.world;
        require(state.setup.profile==content::TechnologyProfile::bnw &&
                world.game_state.layout==bnw && world.map_header.layout==bnw &&
                world.map_plots && world.map_plots->layout==bnw &&
                world.map_projection &&
                world.map_plots->plots.size()==world.map_header.grid.size() &&
                world.map_projection->cells.size()==world.map_header.grid.size() &&
                index<world.map_header.grid.size(),
                "BNW territory requires a complete owned map");
        const auto &plot=world.map_plots->plots.at(static_cast<std::size_t>(index));
        const auto &cell=world.map_projection->cells.at(static_cast<std::size_t>(index));
        const auto xy=world.map_header.grid.coordinates(index);
        require(plot.index==index && plot.x==xy[0] && plot.y==xy[1] &&
                cell.index==index && cell.x==xy[0] && cell.y==xy[1] &&
                cell.owner && cell.units &&
                handle.owner>=0 && handle.owner<64 && handle.id>=0,
                "BNW territory plot or unit identity unavailable");
        const auto owner=field(plot,"owner");
        const auto city_owner=field(plot,"plot_city_owner");
        const auto city_id=field(plot,"plot_city_id");
        const bool is_city=city_owner!=-1;
        require(owner>=-1 && owner<64 && *cell.owner==owner &&
                city_owner>=-1 && city_owner<64 &&
                ((city_owner==-1 && city_id==-1) ||
                 (city_owner>=0 && city_id>=0)) &&
                cell.plot_city && cell.plot_city->owner==city_owner &&
                cell.plot_city->id==city_id,
                "BNW territory raw and live plot owner/city disagree");
        result.plot_owner=owner;
        const auto found=std::count_if(cell.units->begin(),cell.units->end(),
            [&](const auto &item){return item==handle;});
        require(found==1,"BNW territory live plot unit roster changed");
        const saves::PlotArrayObservation *raw_units=nullptr;
        for(const auto &entry:plot.arrays)
            if(entry.name && std::string_view(entry.name)=="units") {
                require(!raw_units,"Duplicate BNW territory raw unit list");
                raw_units=&entry;
            }
        require(raw_units && raw_units->columns==2 &&
                raw_units->values.size()==cell.units->size()*2,
                "BNW territory raw plot unit roster unavailable");
        for(std::size_t slot=0;slot<cell.units->size();++slot)
            require(raw_units->values[slot*2]==(*cell.units)[slot].owner &&
                    raw_units->values[slot*2+1]==(*cell.units)[slot].id,
                    "BNW territory raw and live plot unit rosters disagree");
        const auto &owned=unit(state,handle,static_cast<std::int64_t>(xy[0]),
                               static_cast<std::int64_t>(xy[1]));
        // CvPlot::getTeam returns NO_TEAM for an unowned plot. This is the
        // first CvUnit::canEnterTerritory branch; no diplomacy query occurs.
        if(owner==-1) {
            result.allowed=true;result.branch=BnwTerritoryBranch::no_team;
            return result;
        }
        result.unit_team=player_team(state,handle.owner);
        result.plot_team=player_team(state,owner);
        if(result.unit_team==result.plot_team) {
            result.allowed=true;result.branch=BnwTerritoryBranch::friendly;
            return result;
        }
        const auto &our=team(state,result.unit_team);
        const auto &their=team(state,result.plot_team);
        const auto hostile=core(owned,"m_iAlwaysHostileCount");
        require(hostile>=0,"BNW territory always-hostile count invalid");
        auto combat_team=result.unit_team;
        if(result.plot_team!=63 && hostile>0 && !is_city)
            combat_team=player_team(state,63);
        if(relation(team(state,combat_team),"at_war",result.plot_team)) {
            result.allowed=true;result.branch=BnwTerritoryBranch::enemy;
            return result;
        }
        const auto rival_count=core(owned,"m_iRivalTerritoryCount");
        require(rival_count>=0,"BNW territory rival count invalid");
        if(rival_count>0) {
            result.allowed=true;result.branch=BnwTerritoryBranch::rival;
            return result;
        }
        const auto rival=rival_rule(rules,core(owned,"m_eUnitType"));
        if(!rival) {
            result.reason="BNW territory requires active Units.RivalTerritory rule";
            return result;
        }
        if(*rival) {
            result.allowed=true;result.branch=BnwTerritoryBranch::rival;
            return result;
        }
        if(is_minor(state,result.plot_team) && !is_minor(state,result.unit_team)) {
            // CvPlot::verifyUnitValidPlot passes bIsDeclareWarMove=false.
            if(!relation(our,"has_met",result.plot_team)) {
                result.allowed=true;result.branch=BnwTerritoryBranch::unmet_minor;
                return result;
            }
            const auto player=state.setup.players.find(handle.owner);
            require(player!=state.setup.players.end(),
                    "BNW territory unit player unavailable");
            if(state.setup.is_human(player->second)) {
                result.allowed=true;result.branch=BnwTerritoryBranch::human_minor;
                return result;
            }
            if(core(owned,"m_iBaseCombat")<=0 || handle.owner==63) {
                result.allowed=true;result.branch=BnwTerritoryBranch::anger_free_minor;
                return result;
            }
            // AI-major/minor IsPlayerHasOpenBorders and IsMajorIntruding
            // depend on mutable MinorCivAI friendship/trait state. A later
            // team open-borders true still proves the final result true.
            if(field(their,"liberated_by")==result.unit_team &&
               !relation(their,"at_war",result.unit_team)) {
                result.allowed=true;result.branch=BnwTerritoryBranch::open_borders;
                return result;
            }
            if(relation(their,"open_borders",result.unit_team)) {
                result.allowed=true;result.branch=BnwTerritoryBranch::open_borders;
                return result;
            }
            result.reason="BNW territory requires live MinorCivAI friendship and intrusion";
            return result;
        }
        if((field(their,"liberated_by")==result.unit_team &&
            !relation(their,"at_war",result.unit_team)) ||
           relation(their,"open_borders",result.unit_team)) {
            result.allowed=true;result.branch=BnwTerritoryBranch::open_borders;
            return result;
        }
        result.allowed=false;result.branch=BnwTerritoryBranch::blocked;
    } catch(const std::exception &error) {
        result.allowed.reset();result.branch=BnwTerritoryBranch::unresolved;
        result.reason=error.what();
    }
    return result;
}

BnwUnitValidityProbe make_bnw_can_enter_territory_probe(
    const content::RulesDatabase *rules) {
    return [rules](const LiveSessionState &state,std::uint64_t plot,
                   saves::SavedPlotUnitReference handle) {
        return quote_bnw_unit_can_enter_territory(state,plot,handle,rules).allowed;
    };
}

} // namespace civ5::runtime
