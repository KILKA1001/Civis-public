#include "civ5/runtime/plot_movement.hpp"
#include "civ5/runtime/vp_plot_movement_profile.hpp"
#include "civ5/runtime/vp_plot_passability_profile.hpp"

namespace civ5::runtime {
namespace {
template<class T> const T &named(const std::vector<T> &v,std::string_view name){
    const T *found=nullptr;for(const auto &f:v)if(f.name&&std::string_view(f.name)==name){
        if(found)throw std::runtime_error("Duplicate movement input: "+std::string(name));found=&f;
    }
    if(!found)throw std::runtime_error("Missing movement input: "+std::string(name));return *found;
}
std::int64_t scalar(const auto &v,const char *name){const auto &a=named(v,name).values;
    if(a.size()!=1)throw std::runtime_error("Invalid movement scalar");return a[0];}
bool boolean(const auto &v,const char *name){const auto n=scalar(v,name);
    if(n!=0&&n!=1)throw std::runtime_error("Invalid movement boolean");return n!=0;}
void validate(const PlotMovementContext &c){
    if(c.players.profile!=content::TechnologyProfile::vp542||c.diplomacy.profile!=c.players.profile||
       c.passability.layout!=saves::GameStateLayout::vp542_corpus||c.defines.profile()!=content::GlobalDefineProfile::vp542||
       !c.types.report().all_succeeded()||c.map.cells.size()!=c.map.grid.size()||c.passability.plots.size()!=c.map.cells.size())
        throw std::runtime_error("Plot movement requires one complete VP world");
}
std::int64_t required(const std::optional<std::int64_t> &v){if(!v)throw std::runtime_error("Unresolved movement plot input");return *v;}
std::int64_t feature(const PlotMovementContext &c,const saves::SavedMapCellProjection &p){
    if(p.feature_type&&p.feature_type_hash)throw std::runtime_error("Ambiguous movement feature identity");
    if(p.feature_type)return *p.feature_type;
    if(p.feature_type_hash){if(!*p.feature_type_hash)return -1;if(const auto *t=c.types.find_hash(*p.feature_type_hash))return t->id;}
    throw std::runtime_error("Unresolved movement feature identity");
}
std::int64_t define(const PlotMovementContext &c,const char *name){const auto *d=c.defines.find(name);
    if(!d||!d->lookup.error.empty()||!std::holds_alternative<std::int64_t>(d->lookup.value))throw std::runtime_error("Unresolved movement terrain define");
    return std::get<std::int64_t>(d->lookup.value);
}
}
PlotMovementState restore_plot_movement(const saves::SavedSessionSnapshot &s,const ResearchContext &players,const PlayerResearchContext &diplomacy){
    constexpr auto vp=saves::GameStateLayout::vp542_corpus;
    if(players.profile!=content::TechnologyProfile::vp542||diplomacy.profile!=players.profile||!s.all_players||!s.teams||
       s.all_players->layout!=vp||s.teams->layout!=vp)throw std::runtime_error("VP movement import profile mismatch");
    PlotMovementState result;
    for(const auto &source:s.all_players->players){const auto id=source.saved_player_id;
        if(!players.players.contains(id)||!diplomacy.players.contains(id)||source.core.layout!=vp||source.traits.layout!=vp)
            throw std::runtime_error("Movement saved player mismatch");
        PlotMovementState::Player p;
        p.embarked_all_water=boolean(source.traits.fields,"m_bEmbarkedAllWater");
        p.mountain_pass=boolean(source.traits.fields,"m_bMountainPass");
        p.mountains_after_general=boolean(source.traits.fields,"m_bCrossesMountainsAfterGreatGeneral");
        p.crosses_ice=boolean(source.traits.fields,"m_bCrossesIce");
        p.generals_created=scalar(source.core.fields,"m_iGreatGeneralsCreated");
        if(!result.players.emplace(id,p).second)throw std::runtime_error("Duplicate movement player");
    }
    for(const auto &source:s.teams->teams){const auto id=named(source.fields,"id").value;
        if(!diplomacy.teams.contains(id))throw std::runtime_error("Movement saved team mismatch");
        PlotMovementState::Team t;t.embark_count=named(source.fields,"can_embark").value;t.all_water_count=named(source.fields,"embarked_all_water").value;
        const auto &borders=named(source.arrays,"open_borders");if(borders.columns!=1)throw std::runtime_error("Invalid open-border array");
        // Original fixed arrays may have uninitialized tail bytes. Only the
        // imported roster defines live team relations; host IDs remain wide.
        for(const auto &[other,state]:diplomacy.teams){
            if(other<0||static_cast<std::uint64_t>(other)>=borders.values.size())throw std::runtime_error("Unresolved saved border relation");
            if(borders.values[other]!=0)t.open_borders.insert(other);
        }
        if(!result.teams.emplace(id,std::move(t)).second)throw std::runtime_error("Duplicate movement team");
    }
    if(result.players.size()!=players.players.size()||result.teams.size()!=diplomacy.teams.size())throw std::runtime_error("Incomplete movement roster");
    return result;
}
void refresh_plot_movement_team_status(PlotMovementContext &c,std::int64_t team){
    validate(c);bool minor=false;
    for(const auto id:c.diplomacy.teams.at(team).members)if(c.diplomacy.players.at(id).pregame_minor)minor=true;
    set_vp_plot_movement_minor(c.state,team,minor);
}
bool movement_open_borders(const PlotMovementContext &c,std::int64_t from,std::int64_t to){
    validate(c);if(to<0)return false;
    const auto &a=c.diplomacy.teams.at(from);(void)c.diplomacy.teams.at(to);
    if(from==to||(a.liberated_by==to&&!a.at_war.contains(to))||a.master==to)return true;
    return c.state.teams.at(from).open_borders.contains(to);
}
bool player_can_embark(const PlotMovementContext &c,std::int64_t id){
    validate(c);return c.state.teams.at(c.players.players.at(id).team).embark_count>0||c.state.players.at(id).embarked_all_water;
}
bool player_can_cross_ocean(const PlotMovementContext &c,std::int64_t id){
    validate(c);return c.state.teams.at(c.players.players.at(id).team).all_water_count>0||c.state.players.at(id).embarked_all_water;
}
bool player_can_cross_mountain(const PlotMovementContext &c,std::int64_t id){
    validate(c);const auto &p=c.state.players.at(id);return p.mountain_pass||(p.generals_created>0&&p.mountains_after_general);
}
bool player_can_cross_ice(const PlotMovementContext &c,std::int64_t id){validate(c);return c.state.players.at(id).crosses_ice;}
bool plot_is_valid_move(const PlotMovementContext &c,std::int64_t plot,std::int64_t player,bool check_territory){
    validate(c);if(plot<0)throw std::runtime_error("Invalid movement plot");const auto &p=c.map.cells.at(plot);const auto xy=c.map.grid.coordinates(plot);
    if(p.index!=static_cast<std::uint64_t>(plot)||p.x!=xy[0]||p.y!=xy[1])throw std::runtime_error("Movement plot identity mismatch");
    const auto &pass=c.passability.plots.at(plot);
    const auto route=[&]{if(!p.route)throw std::runtime_error("Unresolved movement route");return p.route->type!=-1&&!p.route->pillaged;};
    if(player==-1)return (!pass.impassable&&feature(c,p)!=0)||route();
    const auto team=c.players.players.at(player).team;
    if(vp_plot_team_impassable(c.passability,plot,team)){
        bool can_pass=false;
        if(feature(c,p)==0&&player_can_cross_ice(c,player))can_pass=true;
        if((required(p.terrain_type)==7||required(p.plot_type)==0)&&player_can_cross_mountain(c,player))can_pass=true;
        if(required(p.plot_type)==3){
            if(required(p.terrain_type)==define(c,"DEEP_WATER_TERRAIN")&&player_can_cross_ocean(c,player))can_pass=true;
            if(required(p.terrain_type)==define(c,"SHALLOW_WATER_TERRAIN")&&player_can_embark(c,player))can_pass=true;
        }
        if(route()&&(!p.is_city||required(p.owner)==player))can_pass=true;
        if(!can_pass)return false;
    }
    if(check_territory){const auto owner=required(p.owner);const auto other=owner==-1?-1:c.players.players.at(owner).team;
        if(other!=-1&&other!=team&&!vp_plot_movement_minor(c.state,other))
            if(!movement_open_borders(c,other,team)&&!c.diplomacy.teams.at(team).at_war.contains(other))return false;
    }
    return true;
}
} // namespace civ5::runtime
