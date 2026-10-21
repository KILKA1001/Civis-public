#include "civ5/runtime/bnw_unit_mission_continuation_checkpoint.hpp"
#include <algorithm>
#include <bit>
#include <climits>
#include <span>
#include <stdexcept>
#include <string_view>
namespace civ5::runtime {namespace {
constexpr std::array<std::uint8_t,8>magic{'C','I','V','B','M','C','N','1'};
void require(bool b,const char*why){if(!b)throw std::runtime_error(why);}
void hash_valid(std::string_view s){require(s.size()==64&&std::all_of(s.begin(),s.end(),[](char c){return (c>='0'&&c<='9')||(c>='a'&&c<='f');}),"Invalid mission continuation SHA256");}
struct Writer {
 std::vector<std::uint8_t>bytes;
 void u32(std::uint32_t x){for(unsigned i=0;i<4;++i)bytes.push_back(static_cast<std::uint8_t>(x>>(8*i)));}
 void u64(std::uint64_t x){for(unsigned i=0;i<8;++i)bytes.push_back(static_cast<std::uint8_t>(x>>(8*i)));}
 void i32(std::int32_t x){u32(std::bit_cast<std::uint32_t>(x));}
 void flag(bool x){u32(x?1:0);}
 void hash(std::string_view x){hash_valid(x);bytes.insert(bytes.end(),x.begin(),x.end());}
 void rng(const SimulationRandomState&s){u64(s.seed);u64(s.calls);u64(s.resets);}
 void path(const BnwUnitRetainedPath&p){u32(p.destination);u64(p.clear_operations);u64(p.generation_operations);u64(p.path_to_reject_clear_operations);u64(p.nodes.size());for(const auto&n:p.nodes){i32(n.x);i32(n.y);i32(n.data1);i32(n.data2);u32(n.flags);}}
};
struct Reader {
 std::span<const std::uint8_t>bytes;std::size_t at=0;
 void enough(std::size_t n){require(at<=bytes.size()&&n<=bytes.size()-at,"Truncated mission continuation checkpoint");}
 std::uint32_t u32(){enough(4);std::uint32_t x=0;for(unsigned i=0;i<4;++i)x|=std::uint32_t(bytes[at++])<<(8*i);return x;}
 std::uint64_t u64(){enough(8);std::uint64_t x=0;for(unsigned i=0;i<8;++i)x|=std::uint64_t(bytes[at++])<<(8*i);return x;}
 std::int32_t i32(){return std::bit_cast<std::int32_t>(u32());}
 bool flag(){const auto x=u32();require(x<=1,"Noncanonical mission continuation Boolean");return x!=0;}
 std::string hash(){enough(64);std::string x(bytes.begin()+at,bytes.begin()+at+64);at+=64;hash_valid(x);return x;}
 SimulationRandomState rng(){SimulationRandomState s;s.seed=u64();s.calls=u64();s.resets=u64();return s;}
 BnwUnitRetainedPath path(std::uint64_t cells){BnwUnitRetainedPath p;p.destination=u32();p.clear_operations=u64();p.generation_operations=u64();p.path_to_reject_clear_operations=u64();const auto n=u64();require(n<=4'000'000&&n<=cells*2&&n<=(bytes.size()-at)/20,"Mission continuation historical path size invalid");p.nodes.reserve(static_cast<std::size_t>(n));for(std::uint64_t i=0;i<n;++i){BnwUnitRetainedPathNode v;v.x=i32();v.y=i32();v.data1=i32();v.data2=i32();v.flags=u32();p.nodes.push_back(v);}return p;}
};
// No historical node is installed as an active path or movement admission.
template<class R>void valid(const R&r){
 hash_valid(r.identity.payload_sha256);hash_valid(r.identity.rules_sha256);hash_valid(r.before_world_revision);hash_valid(r.current_world_revision);
 require(r.width>0&&r.height>0&&r.width<=INT32_MAX&&r.height<=INT32_MAX&&r.width<=UINT32_MAX/r.height,"Mission continuation map domain invalid");const auto cells=r.width*r.height;
 require(r.actor>=0&&r.actor<64&&r.full_id>=0&&r.slot==(r.full_id&8191)&&r.game_turn>=0&&r.steps==0&&r.eta==-1,"Mission continuation actor/fullID/clock invalid");
 require(r.core_begin<r.core_end&&r.core_end<=r.tail_begin&&r.tail_begin<r.tail_end,"Mission continuation source object interval invalid");
 require(r.map_rng.seed<=UINT32_MAX&&r.game_rng.seed<=UINT32_MAX,"Mission continuation BNW RNG seed invalid");
 require(r.native_archive_index<4096&&r.flags_loaded==r.flags_before,"Mission continuation original Flags lineage unsupported");
 const auto phase=static_cast<std::uint32_t>(r.phase);require(phase>=1&&phase<=4,"Unknown mission continuation phase");
 if(phase==1)require(r.flags_current==r.flags_before&&r.changes==0&&!r.dirty&&r.before_world_revision==r.current_world_revision,"Mission continuation pre-Flags phase has fabricated effects");
 else {const auto evaluating=std::bit_cast<std::int32_t>(std::bit_cast<std::uint32_t>(r.flags_before)|1U);const bool entered_change=evaluating!=r.flags_before;
  const auto requested=phase==2?evaluating:std::bit_cast<std::int32_t>(std::bit_cast<std::uint32_t>(evaluating)&~1U);
  const auto changes=(entered_change?1U:0U)+(phase>=3?1U:0U);require(r.flags_current==requested&&r.changes==changes&&r.dirty==(changes!=0),"Mission continuation Flags store lineage invalid");
  if(requested==r.flags_before)require(r.before_world_revision==r.current_world_revision,"Net-unchanged mission Flags changed world revision");}
 if(phase==4)require(!r.combat_warned_after&&r.combat_warned_before_ordinal<UINT64_MAX&&r.combat_warned_ordinal==r.combat_warned_before_ordinal+1,"Mission completion Game ordinal/false-return lineage invalid");
 else require(!r.combat_warned_before&&!r.combat_warned_after&&r.combat_warned_before_ordinal==0&&r.combat_warned_ordinal==0,"Pre-Game mission phase contains fabricated completion history");
 require(r.before_path.nodes.size()<=4'000'000&&r.before_path.nodes.size()<=cells*2&&r.before_path.clear_operations<UINT64_MAX,"Mission continuation historical cache extent invalid");
 require(r.before_path.destination==UINT32_MAX||r.before_path.destination<cells,"Mission continuation historical destination invalid");
 for(const auto&n:r.before_path.nodes)require(n.x>=0&&n.y>=0&&static_cast<std::uint64_t>(n.x)<r.width&&static_cast<std::uint64_t>(n.y)<r.height&&n.data1>=0&&n.data2>=1&&(n.flags&~3U)==0,"Mission continuation historical path node invalid");
 require(r.current_path.nodes.empty()&&r.current_path.destination==UINT32_MAX&&r.current_path.clear_operations==r.before_path.clear_operations+1&&r.current_path.generation_operations==r.before_path.generation_operations&&r.current_path.path_to_reject_clear_operations==r.before_path.path_to_reject_clear_operations,"Mission continuation actual ClearPathCache once relation invalid");
}
}
BnwUnitMissionContinuationCheckpoint::BnwUnitMissionContinuationCheckpoint(const Record&r):section_(encode(r)){}
saves::CivisSaveSection BnwUnitMissionContinuationCheckpoint::encode(const Record&r){
 valid(r);Writer w;w.bytes.insert(w.bytes.end(),magic.begin(),magic.end());w.u32(4);w.hash(r.identity.payload_sha256);w.hash(r.identity.rules_sha256);w.bytes.insert(w.bytes.end(),r.map_guid.begin(),r.map_guid.end());w.u64(r.width);w.u64(r.height);w.flag(r.wrap_x);w.flag(r.wrap_y);w.i32(r.actor);w.i32(r.full_id);w.i32(r.slot);w.i32(r.game_turn);w.i32(r.steps);w.i32(r.eta);w.u64(r.core_begin);w.u64(r.core_end);w.u64(r.tail_begin);w.u64(r.tail_end);w.hash(r.before_world_revision);w.hash(r.current_world_revision);
 // Version4 has BNW algorithms only: zero identifies RandomAlgorithm::bnw.
 w.u32(0);w.rng(r.map_rng);w.u32(0);w.rng(r.game_rng);w.u32(static_cast<std::uint32_t>(r.phase));w.i32(r.flags_loaded);w.i32(r.flags_before);w.i32(r.flags_current);w.u64(r.native_archive_index);w.u64(r.changes);w.flag(r.dirty);w.path(r.before_path);w.path(r.current_path);w.flag(r.combat_warned_before);w.flag(r.combat_warned_after);w.u64(r.combat_warned_before_ordinal);w.u64(r.combat_warned_ordinal);return {bnw_unit_mission_continuation_section_id,0,std::move(w.bytes)};
}
BnwUnitMissionContinuationCheckpoint::Record BnwUnitMissionContinuationCheckpoint::decode(const saves::CivisSaveSection&s,const SessionCheckpointIdentity&identity){
 require(s.id==bnw_unit_mission_continuation_section_id&&s.flags==0,"Wrong mission continuation section identity/flags");Reader r{s.bytes};r.enough(magic.size());require(std::equal(magic.begin(),magic.end(),s.bytes.begin()),"Wrong mission continuation magic");r.at=magic.size();require(r.u32()==4,"Unsupported mission continuation version");Record x;x.identity.payload_sha256=r.hash();x.identity.rules_sha256=r.hash();require(x.identity.payload_sha256==identity.payload_sha256&&x.identity.rules_sha256==identity.rules_sha256,"Foreign mission continuation payload/rules");r.enough(x.map_guid.size());std::copy_n(s.bytes.begin()+r.at,x.map_guid.size(),x.map_guid.begin());r.at+=x.map_guid.size();x.width=r.u64();x.height=r.u64();require(x.width>0&&x.height>0&&x.width<=INT32_MAX&&x.height<=INT32_MAX&&x.width<=UINT32_MAX/x.height,"Mission continuation decoded map domain invalid");x.wrap_x=r.flag();x.wrap_y=r.flag();x.actor=r.i32();x.full_id=r.i32();x.slot=r.i32();x.game_turn=r.i32();x.steps=r.i32();x.eta=r.i32();x.core_begin=r.u64();x.core_end=r.u64();x.tail_begin=r.u64();x.tail_end=r.u64();x.before_world_revision=r.hash();x.current_world_revision=r.hash();require(r.u32()==0,"Foreign mission map RNG algorithm");x.map_rng=r.rng();require(r.u32()==0,"Foreign mission gameplay RNG algorithm");x.game_rng=r.rng();x.phase=static_cast<BnwUnitMissionContinuationPhase>(r.u32());x.flags_loaded=r.i32();x.flags_before=r.i32();x.flags_current=r.i32();x.native_archive_index=r.u64();x.changes=r.u64();x.dirty=r.flag();x.before_path=r.path(x.width*x.height);x.current_path=r.path(x.width*x.height);x.combat_warned_before=r.flag();x.combat_warned_after=r.flag();x.combat_warned_before_ordinal=r.u64();x.combat_warned_ordinal=r.u64();require(r.at==s.bytes.size(),"Trailing mission continuation bytes");valid(x);require(encode(x).bytes==s.bytes,"Noncanonical mission continuation checkpoint");return x;
}

void validate_bnw_unit_mission_continuation_checkpoint_shape(const saves::CivisSaveSection&s){
 // Only the fixed envelope's hashes are read here. The SAME complete private
 // decoder validates magic/version, phase, bounds, history and canonical bytes.
 Reader envelope{s.bytes};envelope.enough(magic.size()+4);envelope.at=magic.size()+4;
 SessionCheckpointIdentity embedded;embedded.payload_sha256=envelope.hash();embedded.rules_sha256=envelope.hash();
 (void)BnwUnitMissionContinuationCheckpoint::decode(s,embedded);
}
void validate_bnw_unit_mission_continuation_checkpoint_binding(const saves::CivisSaveSection&s,const SessionCheckpointIdentity&identity){
 (void)BnwUnitMissionContinuationCheckpoint::decode(s,identity);
}
} // namespace civ5::runtime
