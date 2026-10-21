#include "civ5/runtime/bnw_ui_transient_checkpoint.hpp"
#include "civ5/runtime/file_identity.hpp"
#include <algorithm>
#include <bit>
#include <limits>
#include <stdexcept>
#include <variant>

namespace civ5::runtime {
// The codec is the only new constructor of these retained owners. A decoded
// owner still requires all existing current-world and selected-client guards.
struct BnwUiTransientCheckpointAccess {
    static BnwUiTransientCheckpoint empty(const SessionCheckpointIdentity &id,
                                         std::array<std::uint8_t,16> guid) {
        return {BnwUpdateMovesImportOwner(id,guid), BnwBusyStateOwner(id,guid)};
    }
    static void unit(BnwBusyStateOwner &owner,BnwBusyObjectKey key,BnwBusyUnitTransient value) {
        if(!owner.units_.emplace(key,value).second)
            throw std::runtime_error("Duplicate BNW transient unit identity");
    }
    static void city(BnwBusyStateOwner &owner,BnwBusyObjectKey key,BnwBusyReference value) {
        if(!owner.cities_.emplace(key,value).second)
            throw std::runtime_error("Duplicate BNW transient city identity");
    }
};
namespace {
constexpr std::array<std::uint8_t,8> magic{'C','I','V','B','U','I','T','1'};
constexpr auto bnw=saves::GameStateLayout::expansion2_version1;
void need(bool value,const char *message) {if(!value)throw std::runtime_error(message);}
void digest(std::string_view hash) {
    need(hash.size()==64 && std::all_of(hash.begin(),hash.end(),[](char c){
        return (c>='0'&&c<='9')||(c>='a'&&c<='f');
    }),"Invalid BNW transient checkpoint digest");
}
void native(std::int64_t value) {
    need(value>=INT32_MIN && value<=INT32_MAX,"BNW transient exceeds native integer range");
}
struct Clock {std::int64_t turn,elapsed;};
Clock clock(const saves::SavedSessionSnapshot &world) {
    need(world.map_header.layout==bnw && world.game_state.layout==bnw &&
         world.pregame && world.pregame->layout==bnw &&
         std::holds_alternative<saves::Expansion2PrefixObservation>(world.game_state.prefix),
         "BNW transient checkpoint requires a complete BNW world");
    const auto turn=world.pregame->archive.integers.find("game_turn");
    const auto elapsed=std::get<saves::Expansion2PrefixObservation>(world.game_state.prefix).scalars[1];
    need(turn!=world.pregame->archive.integers.end() && turn->second>=0 && elapsed>=0,
         "BNW transient checkpoint clock is unavailable");
    return {turn->second,elapsed};
}
Clock boundary(const LiveSessionState &state,const saves::SavedSession &source,
               const SessionCheckpointIdentity &identity) {
    digest(identity.payload_sha256);digest(identity.rules_sha256);
    need(identify_bytes(source.source.bytes).sha256==identity.payload_sha256,
         "BNW transient checkpoint payload identity differs");
    const auto base=clock(source.snapshot),live=clock(state.world);
    need(state.setup.profile==content::TechnologyProfile::bnw &&
         base.turn==live.turn && base.elapsed==live.elapsed &&
         state.setup.game_turn==live.turn && state.maintenance.elapsed_turns==live.elapsed &&
         !state.game_clock.pending,
         "BNW transient V1 cannot persist an advanced or pending clock");
    need(state.world.map_header.guid==source.snapshot.map_header.guid &&
         state.world.pregame->engine_guid==source.snapshot.pregame->engine_guid &&
         state.world.pregame->gamecore_guid==source.snapshot.pregame->gamecore_guid,
         "BNW transient checkpoint immutable world identity differs");
    return live;
}
void schedule(const BnwUpdateMovesImportOwner &owner) {
    need(owner.last_ai_civs_processed()==-1 && !owner.process_player_auto_moves() &&
         !owner.force_ending_turn(),"BNW transient V1 lacks progressed engine delivery history");
    for(std::size_t i=0;i<64;++i)
        need(owner.initial_ai_processed(i)==-1,"BNW transient V1 lacks initial-AI receipt history");
}
void validate(const LiveSessionState &state,const SessionCheckpointIdentity &id,
              const BnwUpdateMovesImportOwner &moves,const BnwBusyStateOwner &busy) {
    need(moves.identity().payload_sha256==id.payload_sha256 &&
         moves.identity().rules_sha256==id.rules_sha256 &&
         moves.map_guid()==state.world.map_header.guid,
         "BNW movement checkpoint owner identity differs");
    schedule(moves);
    for(const auto &[key,value]:busy.units()) {
        native(key.first);native(key.second);native(value.mission_timer);
        need(value.mission_timer>=0,"Negative BNW checkpoint mission timer");
        native(value.combat_city.owner);native(value.combat_city.id);
    }
    for(const auto &[key,value]:busy.cities()) {
        native(key.first);native(key.second);native(value.owner);native(value.id);
    }
    for(std::int64_t i=0;i<64;++i) {
        (void)observe_bnw_busy_player(state,busy,id,i);
        (void)observe_bnw_update_moves_units(state,moves,i);
    }
}
struct Writer {
    std::vector<std::uint8_t> bytes;
    template<class Range> void raw(const Range &v) {bytes.insert(bytes.end(),v.begin(),v.end());}
    void u32(std::uint32_t v) {for(unsigned i=0;i<4;++i)bytes.push_back(static_cast<std::uint8_t>(v>>(8*i)));}
    void u64(std::uint64_t v) {for(unsigned i=0;i<8;++i)bytes.push_back(static_cast<std::uint8_t>(v>>(8*i)));}
    void i64(std::int64_t v) {u64(std::bit_cast<std::uint64_t>(v));}
};
struct Reader {
    std::span<const std::uint8_t> bytes;std::size_t at=0;
    void available(std::size_t count) const {need(count<=bytes.size()-at,"Truncated BNW transient checkpoint");}
    template<class Range> void equal(const Range &expected) {
        available(expected.size());
        need(std::equal(expected.begin(),expected.end(),bytes.begin()+at),"BNW transient checkpoint identity/version differs");
        at+=expected.size();
    }
    std::uint32_t u32() {available(4);std::uint32_t v=0;for(unsigned i=0;i<4;++i)v|=std::uint32_t(bytes[at++])<<(8*i);return v;}
    std::uint64_t u64() {available(8);std::uint64_t v=0;for(unsigned i=0;i<8;++i)v|=std::uint64_t(bytes[at++])<<(8*i);return v;}
    std::int64_t i64() {return std::bit_cast<std::int64_t>(u64());}
    bool flag() {available(1);auto v=bytes[at++];need(v<=1,"Invalid BNW checkpoint boolean");return v!=0;}
};
std::pair<std::size_t,std::size_t> counts(const LiveSessionState &state) {
    need(state.world.all_players && state.world.all_players->players.size()==64,
         "BNW transient checkpoint object roster is unavailable");
    std::size_t units=0,cities=0;
    for(const auto &p:state.world.all_players->players) {
        need(p.units.units.size()<=std::numeric_limits<std::size_t>::max()-units &&
             p.cities.cities.size()<=std::numeric_limits<std::size_t>::max()-cities,
             "BNW transient checkpoint roster size overflow");
        units+=p.units.units.size();cities+=p.cities.cities.size();
    }
    return {units,cities};
}
} // namespace
saves::CivisSaveSection make_bnw_ui_transient_checkpoint(
    const LiveSessionState &state,const saves::SavedSession &source,
    const SessionCheckpointIdentity &identity,const BnwUpdateMovesImportOwner &moves,
    const BnwBusyStateOwner &busy) {
    const auto time=boundary(state,source,identity);validate(state,identity,moves,busy);
    Writer w;w.raw(magic);w.u32(1);w.raw(identity.payload_sha256);w.raw(identity.rules_sha256);
    w.raw(state.world.map_header.guid);w.raw(state.world.pregame->engine_guid);w.raw(state.world.pregame->gamecore_guid);
    w.i64(time.turn);w.i64(time.elapsed);w.i64(moves.last_ai_civs_processed());
    for(std::size_t i=0;i<64;++i)w.i64(moves.initial_ai_processed(i));
    w.bytes.push_back(moves.process_player_auto_moves()?1:0);w.bytes.push_back(moves.force_ending_turn()?1:0);
    w.u64(busy.units().size());w.u64(busy.cities().size());
    for(const auto &[key,value]:busy.units()) {
        w.i64(key.first);w.i64(key.second);w.i64(value.mission_timer);
        w.i64(value.combat_city.owner);w.i64(value.combat_city.id);
    }
    for(const auto &[key,value]:busy.cities()) {
        w.i64(key.first);w.i64(key.second);w.i64(value.owner);w.i64(value.id);
    }
    return {static_cast<std::uint32_t>(saves::CivisSaveSectionId::live_bnw_ui_transients),0,std::move(w.bytes)};
}
BnwUiTransientCheckpoint restore_bnw_ui_transient_checkpoint(
    std::span<const std::uint8_t> bytes,const LiveSessionState &state,
    const saves::SavedSession &source,const SessionCheckpointIdentity &identity) {
    const auto time=boundary(state,source,identity);
    Reader r{bytes};r.equal(magic);need(r.u32()==1,"Unsupported BNW transient checkpoint version");
    r.equal(identity.payload_sha256);r.equal(identity.rules_sha256);
    r.equal(state.world.map_header.guid);r.equal(state.world.pregame->engine_guid);r.equal(state.world.pregame->gamecore_guid);
    const auto saved_turn=r.i64(),saved_elapsed=r.i64();
    need(saved_turn==time.turn && saved_elapsed==time.elapsed,"BNW transient checkpoint belongs to another clock");
    need(r.i64()==-1,"BNW transient V1 lacks last-AI delivery history");
    for(std::size_t i=0;i<64;++i)need(r.i64()==-1,"BNW transient V1 lacks initial-AI history");
    const auto automatic=r.flag(),forced=r.flag();
    need(!automatic && !forced,"BNW transient V1 cannot restore a progressed scheduler");
    const auto unit_count=r.u64(),city_count=r.u64();const auto expected=counts(state);
    need(unit_count==expected.first && city_count==expected.second,"BNW transient checkpoint roster counts differ");
    need(unit_count<=(bytes.size()-r.at)/40,"Truncated BNW checkpoint unit roster");
    const auto remaining=bytes.size()-r.at-static_cast<std::size_t>(unit_count)*40;
    need(city_count<=remaining/32 && remaining==city_count*32,"BNW checkpoint city roster length/trailing bytes differ");
    auto result=BnwUiTransientCheckpointAccess::empty(identity,state.world.map_header.guid);
    for(std::uint64_t i=0;i<unit_count;++i) {
        const auto owner=r.i64(),id=r.i64(),timer=r.i64(),combat_owner=r.i64(),combat_id=r.i64();
        BnwUiTransientCheckpointAccess::unit(result.busy,{owner,id},{timer,{combat_owner,combat_id}});
    }
    for(std::uint64_t i=0;i<city_count;++i) {
        const auto owner=r.i64(),id=r.i64(),combat_owner=r.i64(),combat_id=r.i64();
        BnwUiTransientCheckpointAccess::city(result.busy,{owner,id},{combat_owner,combat_id});
    }
    need(r.at==bytes.size(),"Trailing BNW transient checkpoint bytes");
    validate(state,identity,result.moves,result.busy);return result;
}
} // namespace civ5::runtime
