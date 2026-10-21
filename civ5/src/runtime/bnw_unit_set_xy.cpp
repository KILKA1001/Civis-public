#include "civ5/runtime/bnw_unit_set_xy.hpp"
#include <array>
#include <climits>
#include <cstdlib>
#include <stdexcept>
#include <string_view>

namespace civ5::runtime {
namespace {
constexpr auto bnw=saves::GameStateLayout::expansion2_version1;
constexpr std::array<BnwSetXYPhase,12> source_order{
    BnwSetXYPhase::popup_guard_and_ranged_setup,
    BnwSetXYPhase::destination_collision,
    BnwSetXYPhase::old_plot_remove,
    BnwSetXYPhase::old_plot_sight_area_and_move_clock,
    BnwSetXYPhase::coordinates_and_facing,
    BnwSetXYPhase::new_plot_fortify_sight_roster_and_area,
    BnwSetXYPhase::city_transport_meeting_and_adjacent_effects,
    BnwSetXYPhase::graphics_and_cargo_follow,
    BnwSetXYPhase::goody_camp_and_territory,
    BnwSetXYPhase::capture_and_ui_dirty,
    BnwSetXYPhase::lua_unit_set_xy,
    BnwSetXYPhase::popup_release_and_garrison_ui,
};
constexpr std::array<BnwSetXYBlocker,8> missing_services{
    BnwSetXYBlocker::unit_rules_and_combat,
    BnwSetXYBlocker::plot_sight_and_revelation,
    BnwSetXYBlocker::area_unit_counters,
    BnwSetXYBlocker::facing_and_fortify,
    BnwSetXYBlocker::diplomacy_and_adjacent_effects,
    BnwSetXYBlocker::ui_publication,
    BnwSetXYBlocker::lua_unit_set_xy,
    BnwSetXYBlocker::complete_unit_persistence,
};
void require(bool value,const char *why) {if(!value) throw std::runtime_error(why);}
std::string_view name(const char *value) {return value?std::string_view(value):std::string_view();}
std::int64_t core(const saves::SavedUnitObservation &unit,std::string_view wanted,
                  saves::UnitCoreEncoding encoding=saves::UnitCoreEncoding::i32) {
    const saves::UnitCoreField *found=nullptr;
    for(const auto &field:unit.core.fields)
        if(name(field.name)==wanted) {
            require(!found,"Duplicate BNW unit core field");found=&field;
        }
    require(found && found->encoding==encoding && found->values.size()==1,
            "Missing BNW unit core field");
    return found->values.front();
}
const saves::UnitTailNode *tail_field(const saves::UnitTailNode &node,
                                      std::string_view wanted) {
    if(name(node.name)==wanted) return &node;
    for(const auto &child:node.children)
        if(const auto *found=tail_field(child,wanted)) return found;
    return nullptr;
}
std::int64_t tail(const saves::SavedUnitObservation &unit,std::string_view wanted) {
    const saves::UnitTailNode *found=nullptr;
    for(const auto &block:unit.tail.blocks)
        if(const auto *candidate=tail_field(block,wanted)) {
            require(!found,"Duplicate BNW unit tail field");found=candidate;
        }
    require(found && found->values.size()==1,"Missing BNW unit tail field");
    return found->values.front();
}
std::int64_t plot_field(const saves::PlotObservation &plot,std::string_view wanted) {
    const saves::NamedSaveInteger *found=nullptr;
    for(const auto &field:plot.fields)
        if(name(field.name)==wanted) {
            require(!found,"Duplicate BNW plot field");found=&field;
        }
    require(found,"Missing BNW plot field");
    return found->value;
}
std::vector<saves::SavedPlotUnitReference> plot_handles(const saves::PlotObservation &plot) {
    const saves::PlotArrayObservation *found=nullptr;
    for(const auto &array:plot.arrays)
        if(name(array.name)=="units") {
            require(!found,"Duplicate BNW plot unit list");found=&array;
        }
    require(found && found->columns==2 && found->values.size()%2==0,
            "Missing BNW plot unit list");
    std::vector<saves::SavedPlotUnitReference> result;
    for(std::size_t i=0;i<found->values.size();i+=2)
        result.push_back({found->values[i],found->values[i+1]});
    return result;
}
void supported_empty_plot(const saves::SavedSessionSnapshot &world,std::uint64_t index,
                          bool old,saves::SavedPlotUnitReference handle) {
    const auto &plot=world.map_plots->plots.at(index);
    const auto &cell=world.map_projection->cells.at(index);
    const auto xy=world.map_header.grid.coordinates(index);
    require(plot.index==index && plot.x==xy[0] && plot.y==xy[1] &&
            cell.index==index && cell.x==xy[0] && cell.y==xy[1],
            "BNW unit plot identity changed");
    const auto raw=plot_handles(plot);
    require(cell.units && *cell.units==raw,"BNW raw/projected unit list disagrees");
    require(old ? raw==std::vector<saves::SavedPlotUnitReference>{handle} : raw.empty(),
            "BNW setXY candidate needs singleton source and empty destination");
    require(plot_field(plot,"plot_city_owner")==-1 &&
            plot_field(plot,"plot_city_id")==-1,
            "BNW setXY city/garrison path needs dedicated services");
    if(!old)
        require(plot_field(plot,"owner")==-1 &&
                plot_field(plot,"improvement_type_hash")==0,
                "BNW setXY destination territory or improvement needs dedicated services");
}
} // namespace

BnwSetXYPreflight preflight_bnw_set_xy(
    const saves::SavedSessionSnapshot &world,const BnwSetXYRequest &request) {
    BnwSetXYPreflight result;
    result.sdk_order.assign(source_order.begin(),source_order.end());
    try {
        require(world.game_state.layout==bnw && world.map_header.layout==bnw &&
                world.map_plots && world.map_plots->layout==bnw &&
                world.map_projection && world.all_players &&
                world.all_players->layout==bnw &&
                std::holds_alternative<saves::Expansion2PrefixObservation>(
                    world.game_state.prefix),
                "BNW setXY requires complete decoded BNW map, players and game clock");
        const auto &grid=world.map_header.grid;
        require(world.map_plots->plots.size()==grid.size() &&
                world.map_projection->cells.size()==grid.size() &&
                request.owner>=0 && request.unit_id>=0 &&
                request.to_x>=0 && request.to_y>=0 &&
                request.to_x<INT32_MAX && request.to_y<INT32_MAX &&
                static_cast<std::uint64_t>(request.to_x)<grid.width &&
                static_cast<std::uint64_t>(request.to_y)<grid.height,
                "BNW setXY destination or map shape is invalid");
        const saves::SavedUnitObservation *selected=nullptr;
        for(const auto &player:world.all_players->players)
            for(const auto &unit:player.units.units)
                if(core(unit,"m_eOwner")==request.owner &&
                   core(unit,"m_iID")==request.unit_id) {
                    require(!selected && player.saved_player_id==request.owner &&
                            unit.core.layout==bnw && unit.tail.layout==bnw,
                            "BNW setXY unit identity is duplicate or outside player roster");
                    selected=&unit;
                }
        require(selected,"BNW setXY unit is absent from player roster");
        const auto &unit=*selected;
        const auto x=core(unit,"m_iX"),y=core(unit,"m_iY");
        require(x>=0 && y>=0 && x<INT32_MAX && y<INT32_MAX &&
                static_cast<std::uint64_t>(x)<grid.width &&
                static_cast<std::uint64_t>(y)<grid.height &&
                std::llabs(request.to_x-x)+std::llabs(request.to_y-y)==1,
                "BNW setXY candidate needs adjacent on-map relocation");
        require(tail(unit,"m_iMapLayer")==0 &&
                tail(unit,"m_transportUnit.eOwner")==-1 &&
                tail(unit,"m_combatUnit.eOwner")==-1 &&
                core(unit,"m_bDeathDelay",saves::UnitCoreEncoding::boolean)==0,
                "BNW setXY cargo, combat, delayed death or map-layer path needs services");
        const auto old_index=grid.index(x,y);
        const auto new_index=grid.index(request.to_x,request.to_y);
        const saves::SavedPlotUnitReference handle{request.owner,request.unit_id};
        supported_empty_plot(world,old_index,true,handle);
        supported_empty_plot(world,new_index,false,handle);
        const auto &prefix=std::get<saves::Expansion2PrefixObservation>(world.game_state.prefix);
        const auto turn_slice=prefix.scalars[7];
        require(turn_slice>=0 && turn_slice<=INT32_MAX,
                "BNW setXY turn slice is outside unit last-move wire range");
        result.simple_candidate=true;
        result.from_x=x;result.from_y=y;
        result.to_x=request.to_x;result.to_y=request.to_y;
        result.source_last_move_turn=core(unit,"m_iLastMoveTurn");
        result.next_last_move_turn=turn_slice;
        result.clears_ranged_setup=
            core(unit,"m_bSetUpForRangedAttack",saves::UnitCoreEncoding::boolean)!=0;
        result.resets_fortify=core(unit,"m_iFortifyTurns")!=0;
        result.blockers.assign(missing_services.begin(),missing_services.end());
        result.reason="BNW setXY requires sight, area, unit rules, diplomacy, UI, Lua and persistence services";
    } catch(const std::exception &error) {
        result.simple_candidate=false;
        result.blockers={BnwSetXYBlocker::unsupported_candidate};
        result.reason=error.what();
    }
    return result;
}
} // namespace civ5::runtime
