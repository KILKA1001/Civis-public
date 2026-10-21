#include "civ5/runtime/bnw_unit_jump_detached.hpp"
#include <algorithm>
#include <climits>
#include <limits>
#include <stdexcept>
#include <string_view>
#include <utility>

namespace civ5::runtime {
namespace {
constexpr auto bnw=saves::GameStateLayout::expansion2_version1;
std::int64_t unit_value(const saves::SavedUnitObservation &unit,
                        std::string_view name) {
    const saves::UnitCoreField *found=nullptr;
    for(const auto &field:unit.core.fields)
        if(field.name && std::string_view(field.name)==name) {
            if(found) throw std::runtime_error("duplicate unit core field");
            found=&field;
        }
    if(!found || found->encoding!=saves::UnitCoreEncoding::i32 ||
       found->values.size()!=1)
        throw std::runtime_error("missing unit core field: "+std::string(name));
    return found->values.front();
}
void validate_source(const LiveSessionState &state,std::uint64_t plot,
                     saves::SavedPlotUnitReference handle) {
    const auto &world=state.world;
    if(world.game_state.layout!=bnw || !world.map_projection ||
       !world.all_players || plot>=world.map_header.grid.size() ||
       world.map_projection->cells.size()!=world.map_header.grid.size() ||
       world.map_header.grid.size()==0)
        throw std::runtime_error("jump requires complete BNW map and unit registry");
    const auto &cell=world.map_projection->cells.at(static_cast<std::size_t>(plot));
    if(cell.index!=plot || !cell.units)
        throw std::runtime_error("jump source plot roster is unavailable");
    std::size_t plot_matches=0,registry_matches=0;
    for(const auto &entry:*cell.units)
        if(entry.owner==handle.owner && entry.id==handle.id) ++plot_matches;
    for(const auto &player:world.all_players->players)
        for(const auto &unit:player.units.units)
            if(unit_value(unit,"m_eOwner")==handle.owner &&
               unit_value(unit,"m_iID")==handle.id) {
                ++registry_matches;
                const auto xy=world.map_header.grid.coordinates(plot);
                if(xy[0]>INT32_MAX || xy[1]>INT32_MAX ||
                   unit_value(unit,"m_iX")!=static_cast<std::int64_t>(xy[0]) ||
                   unit_value(unit,"m_iY")!=static_cast<std::int64_t>(xy[1]) ||
                   player.saved_player_id!=handle.owner)
                    throw std::runtime_error("jump unit position or owner disagrees");
            }
    if(plot_matches!=1 || registry_matches!=1)
        throw std::runtime_error("jump source unit identity is absent or duplicated");
}
template<class F>
auto ask(const F &fn,const LiveSessionState &state,std::uint64_t plot,
         saves::SavedPlotUnitReference handle,const char *name) {
    if(!fn) throw std::runtime_error(std::string("jump callback missing: ")+name);
    const auto value=fn(state,plot,handle);
    if(!value) throw std::runtime_error(std::string("jump callback unresolved: ")+name);
    return *value;
}
std::int64_t distance(const BnwJumpDistance &fn,const LiveSessionState &state,
                      std::uint64_t a,std::uint64_t b) {
    if(!fn) throw std::runtime_error("jump callback missing: plotDistance");
    const auto value=fn(state,a,b);
    if(!value || *value<0 || *value>INT_MAX)
        throw std::runtime_error("jump plotDistance is unresolved or out of range");
    return *value;
}
void step(LiveSessionState &draft,
          const std::function<LiveActionResult(
              LiveSessionState &,saves::SavedPlotUnitReference)> &effect,
          saves::SavedPlotUnitReference handle,const char *name) {
    const auto result=effect(draft,handle);
    if(result.outcome!=LiveActionOutcome::committed)
        throw std::runtime_error(std::string(name)+": "+result.reason);
}
} // namespace

BnwJumpSelection select_bnw_nearest_valid_plot(
    const LiveSessionState &state,std::uint64_t source_plot,
    saves::SavedPlotUnitReference handle,std::int64_t plot_unit_limit,
    const BnwJumpReadServices &services) {
    BnwJumpSelection selection;
    selection.source_plot=source_plot;
    try {
        validate_source(state,source_plot,handle);
        if(plot_unit_limit<0 || plot_unit_limit>INT_MAX)
            throw std::runtime_error("jump plot unit limit is out of range");
        const auto city=ask(services.nearest_city_plot,state,source_plot,handle,
                            "findCity");
        if(city < -1 || (city>=0 &&
           static_cast<std::uint64_t>(city)>=state.world.map_header.grid.size()))
            throw std::runtime_error("jump nearest city index is invalid");

        const auto count=state.world.map_header.grid.size();
        for(std::uint64_t index=0;index<count;++index) {
            selection.scanned_plots=index+1;
            const auto &cell=state.world.map_projection->cells.at(
                static_cast<std::size_t>(index));
            if(cell.index!=index)
                throw std::runtime_error("jump destination plot identity changed");
            if(!ask(services.valid_domain,state,index,handle,"valid domain"))
                continue;
            if(!ask(services.can_move_into,state,index,handle,"canMoveInto"))
                continue;
            const auto friendly=ask(services.friendly_units_of_type,state,index,
                                    handle,"friendly units of type");
            if(friendly<0)
                throw std::runtime_error("jump friendly unit count is negative");
            if(friendly>=plot_unit_limit) continue;
            if(!ask(services.can_enter_territory,state,index,handle,
                    "canEnterTerritory"))
                continue;
            const auto barbarian=ask(services.barbarian,state,index,handle,
                                     "isBarbarian");
            if(!barbarian &&
                ask(services.enemy,state,index,handle,"isEnemy"))
                continue;
            if(ask(services.mountain,state,index,handle,"isMountain"))
                continue;
            if(index==source_plot)
                throw std::runtime_error("jump candidate is the current plot");
            const auto air=ask(services.air,state,index,handle,"DOMAIN_AIR");
            if(air &&
               !ask(services.friendly_city,state,index,handle,"isFriendlyCity"))
                continue;
            const auto sea=air?false:
                ask(services.sea,state,index,handle,"DOMAIN_SEA");
            if(sea) {
                const auto friendly_city=ask(services.friendly_city,state,index,handle,
                                             "isFriendlyCity");
                bool coastal_friendly=false;
                if(friendly_city)
                    coastal_friendly=ask(services.coastal_land,state,index,handle,
                                         "isCoastalLand");
                if(!coastal_friendly &&
                   !ask(services.water,state,index,handle,"isWater"))
                    continue;
            }
            if(!ask(services.revealed,state,index,handle,"isRevealed"))
                continue;
            auto value=distance(services.plot_distance,state,source_plot,index)*2;
            if(city>=0)
                value+=distance(services.plot_distance,state,index,
                                static_cast<std::uint64_t>(city));
            const auto area=ask(services.area_id,state,index,handle,
                                "destination area");
            const auto source_area=ask(services.area_id,state,source_plot,handle,
                                       "source area");
            if(area<0 || source_area<0)
                throw std::runtime_error("jump plot area is invalid");
            if(area!=source_area) value*=3;
            if(value>INT_MAX)
                throw std::runtime_error("jump candidate score exceeds SDK int");
            if(!selection.destination_plot || value<selection.best_value) {
                selection.destination_plot=index;
                selection.best_value=value;
            }
        }
        selection.outcome=selection.destination_plot?
            BnwJumpOutcome::destination:BnwJumpOutcome::no_destination;
    } catch(const std::exception &error) {
        selection.outcome=BnwJumpOutcome::unresolved;
        selection.destination_plot.reset();
        selection.best_value=-1;
        selection.reason=error.what();
    }
    return selection;
}

BnwJumpDetachedResult project_bnw_jump_detached(
    const LiveSessionState &state,std::uint64_t source_plot,
    saves::SavedPlotUnitReference handle,std::int64_t plot_unit_limit,
    const BnwJumpReadServices &reads,const BnwJumpDetachedServices &effects) {
    BnwJumpDetachedResult result;
    result.selection=select_bnw_nearest_valid_plot(
        state,source_plot,handle,plot_unit_limit,reads);
    if(result.selection.outcome==BnwJumpOutcome::unresolved) {
        result.reason=result.selection.reason;return result;
    }
    if(result.selection.outcome==BnwJumpOutcome::no_destination) {
        result.reason="jumpToNearestValidPlot returned false; kill(false) is required";
        return result;
    }
    if(!effects.set_xy || !effects.clear_mission_queue ||
       !effects.set_activity_awake) {
        result.reason="jump requires complete detached setXY, mission and activity services";
        return result;
    }
    try {
        auto draft=state;
        auto moved=effects.set_xy(draft,handle,*result.selection.destination_plot);
        if(moved.outcome!=LiveActionOutcome::committed)
            throw std::runtime_error("setXY: "+moved.reason);
        step(draft,effects.clear_mission_queue,handle,"ClearMissionQueue");
        step(draft,effects.set_activity_awake,handle,"SetActivityType");
        result.detached=std::move(draft);
    } catch(const std::exception &error) {result.reason=error.what();}
    return result;
}

} // namespace civ5::runtime
