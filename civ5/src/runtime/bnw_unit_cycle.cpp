#include "civ5/runtime/bnw_unit_cycle.hpp"
#include "civ5/runtime/map_geometry.hpp"
#include <limits>
#include <map>
#include <stdexcept>
#include <type_traits>

namespace civ5::runtime {
namespace {
constexpr std::int32_t slot_mask=(1<<13)-1; // FFreeListTrashArray.h:20-24
// CvEnums.h:1517-1538, native UnitAITypes (NO_UNITAI=-1).
enum class NativeUnitAI : std::int32_t {worker=2,worker_sea=17};
void need(bool value,const char *why){if(!value)throw std::runtime_error(why);}
std::int32_t native(std::int64_t value){
    need(value>=INT32_MIN && value<=INT32_MAX,"Cycle value exceeds native signed int");
    return static_cast<std::int32_t>(value);
}
struct NodeList {std::int32_t wire_count;const std::vector<std::int64_t> *ids;};
NodeList nodes(const saves::PlayerAdministrationObservation &administration){
    const saves::PlayerAdministrationRecord *record=nullptr;
    for(const auto &block:administration.blocks)if(block.name && std::string_view(block.name)=="pre_city"){
        need(!record,"Duplicate cycle pre_city block");record=&block;
    }
    need(record && administration.layout==saves::GameStateLayout::expansion2_version1,
         "BNW cycle administration unavailable");
    const saves::NamedSaveInteger *count=nullptr;
    for(const auto &field:record->fields)if(field.name && std::string_view(field.name)=="m_UnitCycle_wire_count"){
        need(!count,"Duplicate cycle wire count");count=&field;
    }
    const saves::PlotArrayObservation *list=nullptr;
    for(const auto &array:record->arrays)if(array.name && std::string_view(array.name)=="m_UnitCycle"){
        need(!list,"Duplicate cycle list");list=&array;
    }
    need(count && list && list->columns==1,"Cycle source list shape unavailable");
    const auto signed_count=native(count->value);
    const auto length=signed_count>0?static_cast<std::uint64_t>(signed_count):0;
    need(list->values.size()==length,"Cycle signed wire count differs from list");
    for(const auto id:list->values)(void)native(id);
    return {signed_count,&list->values};
}
}
BnwUnitCycleObservation observe_bnw_unit_cycle(
    const LiveSessionState &state,const BnwUpdateMovesImportOwner &moves,
    const BnwBusyStateOwner &busy,const SessionCheckpointIdentity &identity,std::int64_t player,
    std::optional<BnwUnitOrderKey> current,BnwUnitCycleDirection direction,BnwUnitCycleFilter filter){
    // Existing immutable owner guards FIRST, before any world/roster borrow.
    need(player>=0 && player<64 && identity.payload_sha256.size()==64 && identity.rules_sha256.size()==64 &&
         moves.identity().payload_sha256==identity.payload_sha256 && moves.identity().rules_sha256==identity.rules_sha256 &&
         busy.identity().payload_sha256==identity.payload_sha256 && busy.identity().rules_sha256==identity.rules_sha256 &&
         moves.map_guid()==busy.map_guid(),"Cycle retained source owners differ");
    need(direction==BnwUnitCycleDirection::forward || direction==BnwUnitCycleDirection::backward,"Unknown cycle direction");
    need(filter==BnwUnitCycleFilter::all || filter==BnwUnitCycleFilter::workers,"Unknown cycle filter");
    // Reuse exact moves/busy currentness, full native roster metadata and UNIT
    // readiness. The PLAYER counted-ready list excludes deathDelay; Cycle does not.
    const auto ready=observe_bnw_ready_player(state,moves,busy,identity,player);
    const auto evidence=observe_bnw_update_moves_units(state,moves,player);
    need(ready.units.size()==evidence.size(),"Cycle readiness roster differs");
    std::map<std::int32_t,std::size_t> slots;
    for(std::size_t i=0;i<evidence.size();++i){
        need(evidence[i].id==ready.units[i].id,"Cycle readiness unit identity differs");
        (void)native(evidence[i].moves);(void)native(evidence[i].activity);
        (void)native(evidence[i].automate);(void)native(evidence[i].unit_ai);
        need(slots.emplace(native(evidence[i].id)&slot_mask,i).second,"Cycle native slot duplicated");
    }
    const auto resolve=[&](std::int64_t handle)->std::optional<std::size_t>{
        const auto id=native(handle);if(id==-1)return std::nullopt;
        const auto found=slots.find(id&slot_mask);
        if(found==slots.end())return std::nullopt;
        const auto index=found->second;
        if((id&~slot_mask)!=0 && evidence[index].id!=id)return std::nullopt;
        return index;
    };
    std::optional<std::size_t> selected;
    if(current){
        need(current->first==player,"Cycle current unit has foreign owner");
        selected=resolve(current->second);
        need(selected && evidence[*selected].id==current->second,"Cycle current is not an actual full unit ID");
    }
    const auto list=nodes(state.world.all_players->players.at(static_cast<std::size_t>(player)).administration);
    BnwUnitCycleObservation result;result.source_wire_count=list.wire_count;
    const auto &ids=*list.ids;const bool forward=direction==BnwUnitCycleDirection::forward;
    std::size_t position=0;
    if(selected){
        position=ids.size();
        for(std::size_t i=0;i<ids.size();++i)if(resolve(ids[i])==selected){
            position=forward?i+1:(i==0?ids.size():i-1);break;
        }
    }
    if(position==ids.size()){
        result.wrapped=true;
        position=forward || ids.empty()?0:ids.size()-1;
    }
    if(ids.empty())return result;
    const auto first=position;
    do{
        const auto unit=resolve(ids[position]);
        if(unit && ready.units[*unit].ready_to_move &&
           (filter==BnwUnitCycleFilter::all || evidence[*unit].unit_ai==static_cast<std::int32_t>(NativeUnitAI::worker) ||
            evidence[*unit].unit_ai==static_cast<std::int32_t>(NativeUnitAI::worker_sea))){
            if(selected && unit==selected)result.wrapped=true;
            result.next=BnwUnitOrderKey{player,evidence[*unit].id};return result;
        }
        if(forward){if(++position==ids.size()){position=0;result.wrapped=true;}}
        else if(position==0){position=ids.size()-1;result.wrapped=true;}else --position;
    }while(position!=first);
    return result;
}
BnwUnitCycleObservation observe_bnw_unit_cycle(
    const BnwEndTurnRequestDraft &request,const BnwUpdateMovesImportOwner &moves,std::int64_t player,
    std::optional<BnwUnitOrderKey> current,BnwUnitCycleDirection direction,BnwUnitCycleFilter filter){
    need(request.owner.busy_state.has_value(),"Cycle same retained busy owner unavailable");
    return observe_bnw_unit_cycle(request.state,moves,*request.owner.busy_state,request.owner.identity,player,current,direction,filter);
}
} // namespace civ5::runtime

namespace civ5::runtime {
namespace {
std::int32_t start_scalar(const auto &core,std::string_view name){
    const typename std::remove_cvref_t<decltype(core.fields)>::value_type *found=nullptr;
    for(const auto &field:core.fields)if(field.name && std::string_view(field.name)==name){
        need(!found,"Nearest-ready duplicate source scalar");found=&field;
    }
    need(found && found->values.size()==1,"Nearest-ready source scalar unavailable");
    return native(found->values.front());
}
}
BnwCycleStartUnitObservation observe_bnw_cycle_start_unit(
    const LiveSessionState &state,const BnwUpdateMovesImportOwner &moves,
    const BnwBusyStateOwner &busy,const SessionCheckpointIdentity &identity,
    std::int64_t player,BnwUnitCycleFilter filter){
    need(player>=0 && player<64 && identity.payload_sha256.size()==64 && identity.rules_sha256.size()==64 &&
         moves.identity().payload_sha256==identity.payload_sha256 && moves.identity().rules_sha256==identity.rules_sha256 &&
         busy.identity().payload_sha256==identity.payload_sha256 && busy.identity().rules_sha256==identity.rules_sha256 &&
         moves.map_guid()==busy.map_guid() && moves.map_guid()==state.world.map_header.guid,
         "Nearest-ready retained source owners differ");
    need(filter==BnwUnitCycleFilter::all || filter==BnwUnitCycleFilter::workers,"Unknown nearest-ready filter");
    constexpr auto bnw=saves::GameStateLayout::expansion2_version1;
    need(state.setup.profile==content::TechnologyProfile::bnw && state.world.game_state.layout==bnw &&
         state.world.map_header.layout==bnw && state.world.all_players && state.world.all_players->layout==bnw &&
         state.world.all_players->players.size()==64,"Nearest-ready BNW source roster unavailable");
    const auto &raw=state.world.all_players->players.at(static_cast<std::size_t>(player));
    need(raw.index==static_cast<std::uint64_t>(player) && raw.saved_player_id==player && raw.core.layout==bnw &&
         start_scalar(raw.core,"m_eID")==player,"Nearest-ready native player identity differs");
    const auto alive=start_scalar(raw.core,"m_bAlive");
    need((alive==0 || alive==1) && (alive!=0)==state.setup.players.at(player).alive,
         "Nearest-ready raw/owned player alive flag differs");
    BnwCycleStartUnitObservation result;
    if(!alive)return result; // Native findUnit does not enter a dead player's units.
    const auto ready=observe_bnw_ready_player(state,moves,busy,identity,player);
    const auto evidence=observe_bnw_update_moves_units(state,moves,player);
    need(ready.units.size()==evidence.size() && evidence.size()==raw.units.units.size(),
         "Nearest-ready current readiness roster differs");
    // Native ReadStreamable loads by full object ID; firstUnit/nextUnit iterate
    // FLTA slots ascending. A mutable wire vector is not itself this ordering.
    std::map<std::int32_t,std::size_t> order;
    for(std::size_t i=0;i<evidence.size();++i){
        need(evidence[i].id==ready.units[i].id,"Nearest-ready readiness unit identity differs");
        need(order.emplace(native(evidence[i].id)&slot_mask,i).second,"Nearest-ready duplicate native unit slot");
    }
    const auto &header=state.world.map_header;
    need(header.grid.width>0 && header.grid.height>0 && header.grid.width<=INT32_MAX && header.grid.height<=INT32_MAX,
         "Nearest-ready map dimensions exceed native signed domain");
    const MapGeometry geometry{header.grid,header.wrap_x,header.wrap_y,bnw};geometry.validate();
    auto best=std::numeric_limits<std::int32_t>::max();
    for(const auto &[slot,i]:order){
        (void)slot;
        if(!ready.units[i].ready_to_move)continue;
        if(filter==BnwUnitCycleFilter::workers && evidence[i].unit_ai!=static_cast<std::int32_t>(NativeUnitAI::worker) &&
           evidence[i].unit_ai!=static_cast<std::int32_t>(NativeUnitAI::worker_sea))continue;
        const auto &unit=raw.units.units[i];
        const auto x=start_scalar(unit.core,"m_iX"),y=start_scalar(unit.core,"m_iY");
        need(x>=0 && y>=0 && static_cast<std::uint64_t>(x)<header.grid.width && static_cast<std::uint64_t>(y)<header.grid.height,
             "Nearest-ready live candidate has invalid plot coordinates");
        const auto distance=native(geometry.distance(0,0,x,y));
        if(distance<best){best=distance;result.next=BnwUnitOrderKey{player,evidence[i].id};result.distance=distance;}
    }
    return result;
}
BnwCycleStartUnitObservation observe_bnw_cycle_start_unit(
    const BnwEndTurnRequestDraft &request,const BnwUpdateMovesImportOwner &moves,
    std::int64_t player,BnwUnitCycleFilter filter){
    need(request.owner.busy_state.has_value(),"Nearest-ready same retained busy owner unavailable");
    return observe_bnw_cycle_start_unit(request.state,moves,*request.owner.busy_state,request.owner.identity,player,filter);
}

BnwGameUnitCycleObservation observe_bnw_game_unit_cycle(
    const LiveSessionState &state,const BnwUpdateMovesImportOwner &moves,
    const BnwBusyStateOwner &busy,const SessionCheckpointIdentity &identity,std::int64_t player,
    std::optional<BnwUnitOrderKey> selected_head,BnwUnitCycleDirection direction,BnwUnitCycleFilter filter){
    need(player>=0 && player<64 && identity.payload_sha256.size()==64 && identity.rules_sha256.size()==64 &&
         moves.identity().payload_sha256==identity.payload_sha256 && moves.identity().rules_sha256==identity.rules_sha256 &&
         busy.identity().payload_sha256==identity.payload_sha256 && busy.identity().rules_sha256==identity.rules_sha256 &&
         moves.map_guid()==busy.map_guid() && moves.map_guid()==state.world.map_header.guid,
         "Game cycle retained source owners differ");
    need(direction==BnwUnitCycleDirection::forward || direction==BnwUnitCycleDirection::backward,"Unknown game cycle direction");
    need(filter==BnwUnitCycleFilter::all || filter==BnwUnitCycleFilter::workers,"Unknown game cycle filter");
    BnwGameUnitCycleObservation result;
    if(!selected_head){
        const auto nearest=observe_bnw_cycle_start_unit(state,moves,busy,identity,player,filter);
        result.next=nearest.next;result.nearest_distance=nearest.distance;
        return result;
    }
    // A non-null UI pointer is a real full-ID unit. Do not accept a native
    // slot-only/stale pointer or change foreign-head into the absent branch.
    need(selected_head->first>=0 && selected_head->first<64 && selected_head->second>=0,
         "Game cycle selected pointer identity invalid");
    (void)native(selected_head->second);
    result.branch=BnwGameUnitCycleBranch::present_selected_head;
    if(selected_head->first==player)result.cycle_unit=selected_head;
    else {
        const auto foreign=observe_bnw_update_moves_units(state,moves,selected_head->first);
        bool found=false;
        for(const auto &unit:foreign)if(unit.id==selected_head->second){found=true;break;}
        need(found,"Game cycle foreign head is not a current full unit ID");
    }
    const auto cycle=observe_bnw_unit_cycle(state,moves,busy,identity,player,result.cycle_unit,direction,filter);
    result.next=cycle.next;result.wrapped=cycle.wrapped;result.source_wire_count=cycle.source_wire_count;
    // CvPlayer::hasAutoUnit is ReadyToAuto, not isAutomated: canMove and
    // nonempty mission queue. It intentionally ignores busy/deathDelay/awake.
    if(result.wrapped)result.send_auto_moves_required=observe_bnw_ready_player(state,moves,busy,identity,player).has_auto_unit;
    result.clear_selection_required=!result.next && result.cycle_unit.has_value();
    return result;
}
BnwGameUnitCycleObservation observe_bnw_game_unit_cycle(
    const BnwEndTurnRequestDraft &request,const BnwUpdateMovesImportOwner &moves,std::int64_t player,
    std::optional<BnwUnitOrderKey> selected_head,BnwUnitCycleDirection direction,BnwUnitCycleFilter filter){
    need(request.owner.busy_state.has_value(),"Game cycle same retained busy owner unavailable");
    return observe_bnw_game_unit_cycle(request.state,moves,*request.owner.busy_state,request.owner.identity,player,selected_head,direction,filter);
}
} // namespace civ5::runtime
