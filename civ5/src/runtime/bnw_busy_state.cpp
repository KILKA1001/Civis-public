#include "civ5/runtime/bnw_busy_state.hpp"
#include "civ5/runtime/file_identity.hpp"
#include <limits>
#include <set>
#include <stdexcept>

namespace civ5::runtime {
namespace {
constexpr auto bnw=saves::GameStateLayout::expansion2_version1;
constexpr std::int32_t slot_mask=(1<<13)-1; // SDK FFreeListTrashArray.h:20-24
constexpr std::int32_t invalid_plot=-INT32_MAX; // CvDLLUtilDefines.h:38
void need(bool value,const char *why) {if(!value)throw std::runtime_error(why);}
void native_int(std::int64_t value) {
    need(value>=INT32_MIN && value<=INT32_MAX,"BNW busy value exceeds native int");
}
template<class Core>std::int64_t scalar(const Core &core,std::string_view name) {
    const typename std::decay_t<decltype(core.fields)>::value_type *found=nullptr;
    for(const auto &field:core.fields)if(field.name && std::string_view(field.name)==name) {
        need(!found,"Duplicate BNW busy scalar");found=&field;
    }
    need(found && found->values.size()==1,"Missing BNW busy scalar");
    native_int(found->values[0]);return found->values[0];
}
std::int64_t tail_scalar(const saves::UnitTailObservation &tail,std::string_view name) {
    const saves::UnitTailNode *state=nullptr;
    for(const auto &block:tail.blocks)if(block.name && std::string_view(block.name)=="state") {
        need(!state,"Duplicate BNW busy unit-tail state block");state=&block;
    }
    need(state,"Missing BNW busy unit-tail state block");
    const saves::UnitTailNode *found=nullptr;
    for(const auto &node:state->children)if(node.name && std::string_view(node.name)==name) {
        need(!found,"Duplicate BNW busy unit-tail scalar");found=&node;
    }
    need(found && found->values.size()==1 && std::string_view(found->encoding)=="i32",
         "Missing BNW busy unit-tail scalar");
    native_int(found->values[0]);return found->values[0];
}
struct Roster {
    std::map<BnwBusyObjectKey,const saves::SavedUnitObservation*> units;
    std::set<BnwBusyObjectKey> cities;
    std::map<BnwBusyObjectKey,std::int64_t> unit_slots,city_slots;
};
template<class Header>void add_slot(std::map<BnwBusyObjectKey,std::int64_t>&slots,
    BnwBusyObjectKey key,const Header &header) {
    need(key.second>=0,"BNW busy object ID is negative");
    const BnwBusyObjectKey slot{key.first,static_cast<std::int32_t>(key.second)&slot_mask};
    const saves::NamedSaveInteger *last=nullptr;
    for(const auto &field:header.fields)if(field.name && std::string_view(field.name)=="last_index") {
        need(!last,"Duplicate BNW busy free-list last index");last=&field;
    }
    need(last && slot.second<=last->value &&
         static_cast<std::uint64_t>(slot.second)<header.source_slots,
         "BNW busy object slot is outside native collection metadata");
    need(slots.emplace(slot,key.second).second,"BNW busy roster has duplicate native slot IDs");
}
Roster roster(const saves::SavedSessionSnapshot &world) {
    need(world.game_state.layout==bnw && world.map_header.layout==bnw && world.all_players &&
         world.all_players->layout==bnw && world.all_players->context.player_count==64 &&
         world.all_players->players.size()==64,"BNW busy complete original roster is unavailable");
    Roster result;
    for(std::int64_t player=0;player<64;++player) {
        const auto &raw=world.all_players->players.at(player);
        need(raw.index==static_cast<std::uint64_t>(player) && raw.saved_player_id==player &&
             raw.core.layout==bnw && scalar(raw.core,"m_eID")==player &&
             raw.units.layout==bnw && raw.units.header.layout==bnw &&
             raw.cities.layout==bnw && raw.cities.header.layout==bnw &&
             raw.units.header.wire_unit_count>=0 && raw.cities.header.wire_city_count>=0 &&
             static_cast<std::uint64_t>(raw.units.header.wire_unit_count)==raw.units.units.size() &&
             static_cast<std::uint64_t>(raw.cities.header.wire_city_count)==raw.cities.cities.size(),
             "BNW busy player/object collections are incomplete");
        for(const auto &unit:raw.units.units) {
            need(unit.core.layout==bnw && unit.tail.layout==bnw && unit.core.version && *unit.core.version<=9 &&
                 unit.tail.context.bnw_unit_version==unit.core.version &&
                 scalar(unit.core,"m_eOwner")==player,"BNW busy unit owner/version differs");
            const BnwBusyObjectKey key{player,scalar(unit.core,"m_iID")};
            need(result.units.emplace(key,&unit).second,"Duplicate BNW busy unit ID");
            add_slot(result.unit_slots,key,raw.units.header);
        }
        for(const auto &city:raw.cities.cities) {
            need(city.core.layout==bnw && city.core.version && *city.core.version<=6 &&
                 scalar(city.core,"m_eOwner")==player,
                 "BNW busy city owner/version differs");
            const BnwBusyObjectKey key{player,scalar(city.core,"m_iID")};
            need(result.cities.insert(key).second,"Duplicate BNW busy city ID");
            add_slot(result.city_slots,key,raw.cities.header);
        }
    }
    return result;
}
bool resolves(const std::map<BnwBusyObjectKey,std::int64_t>&slots,BnwBusyReference ref) {
    native_int(ref.owner);native_int(ref.id);
    if(ref.owner<0 || ref.owner>=64 || ref.id==-1)return false;
    const auto id=static_cast<std::int32_t>(ref.id);
    const auto found=slots.find({ref.owner,id&slot_mask});
    return found!=slots.end() && ((id&~slot_mask)==0 || found->second==id);
}
void validate_ref(BnwBusyReference ref) {native_int(ref.owner);native_int(ref.id);}
}
void BnwBusyStateOwner::set_unit_mission_timer(BnwBusyObjectKey key,std::int64_t timer) {
    native_int(timer);need(timer>=0,"BNW mission timer cannot be negative");units_.at(key).mission_timer=timer;
}
void BnwBusyStateOwner::set_unit_combat_city(BnwBusyObjectKey key,BnwBusyReference ref) {
    validate_ref(ref);units_.at(key).combat_city=ref;
}
void BnwBusyStateOwner::set_city_combat_unit(BnwBusyObjectKey key,BnwBusyReference ref) {
    validate_ref(ref);cities_.at(key)=ref;
}
BnwBusyStateOwner bind_bnw_busy_state_original_import(
    const saves::SavedSession &source,const SessionCheckpointIdentity &identity) {
    need(source.source.container==saves::SaveContainerKind::civ5 &&
         identify_bytes(source.source.bytes).sha256==identity.payload_sha256 &&
         identity.rules_sha256.size()==64,"BNW busy defaults require a fresh original import");
    const auto objects=roster(source.snapshot);
    BnwBusyStateOwner result(identity,source.snapshot.map_header.guid);
    for(const auto &[key,unit]:objects.units)result.units_.emplace(key,BnwBusyUnitTransient{});
    for(const auto &key:objects.cities)result.cities_.emplace(key,BnwBusyReference{});
    return result;
}
BnwBusyPlayerObservation observe_bnw_busy_player(
    const LiveSessionState &state,const BnwBusyStateOwner &owner,
    const SessionCheckpointIdentity &identity,std::int64_t player) {
    need(state.setup.profile==content::TechnologyProfile::bnw && player>=0 && player<64 &&
         owner.identity().payload_sha256==identity.payload_sha256 &&
         owner.identity().rules_sha256==identity.rules_sha256 &&
         owner.map_guid()==state.world.map_header.guid,"BNW busy world/owner identity differs");
    const auto objects=roster(state.world);
    need(objects.units.size()==owner.units().size() && objects.cities.size()==owner.cities().size(),
         "BNW busy transient lifecycle roster is incomplete");
    for(const auto &[key,unit]:objects.units)need(owner.units().contains(key),"BNW busy unit transient is unknown");
    for(const auto &key:objects.cities)need(owner.cities().contains(key),"BNW busy city transient is unknown");
    const auto grid=state.world.map_header.grid;
    need(grid.width>0 && grid.height>0,"BNW busy attack map is unavailable");
    BnwBusyPlayerObservation result;
    for(const auto &[key,unit]:objects.units)if(key.first==player) {
        const auto &transient=owner.units().at(key);
        native_int(transient.mission_timer);need(transient.mission_timer>=0,"Invalid BNW retained mission timer");
        // Preserve IsBusy short circuit: a positive mission timer needs no combat query.
        bool busy=transient.mission_timer>0;
        if(!busy) {
            busy=resolves(objects.unit_slots,{tail_scalar(unit->tail,"m_combatUnit.eOwner"),
                                             tail_scalar(unit->tail,"m_combatUnit.iID")});
            if(!busy)busy=resolves(objects.city_slots,transient.combat_city);
            if(!busy) {
                const auto x=scalar(unit->core,"m_iAttackPlotX"),y=scalar(unit->core,"m_iAttackPlotY");
                if(x!=invalid_plot && y!=invalid_plot) {
                    need(x>=0 && static_cast<std::uint64_t>(x)<grid.width &&
                         y>=0 && static_cast<std::uint64_t>(y)<grid.height,
                         "BNW attack plot is outside unchecked source map bounds");
                    const auto delayed=scalar(unit->core,"m_bDeathDelay");
                    need(delayed==0 || delayed==1,"BNW delayed-death flag is not boolean");
                    busy=!delayed;
                }
            }
        }
        if(busy)result.busy_units.push_back(key.second);
    }
    for(const auto &key:objects.cities)if(key.first==player && resolves(objects.unit_slots,owner.cities().at(key)))
        result.busy_cities.push_back(key.second);
    return result;
}
} // namespace civ5::runtime
