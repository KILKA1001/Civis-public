#include "civ5/runtime/bnw_unit_serial_checkpoint.hpp"
#include <algorithm>
#include <array>
#include <climits>
#include <set>
#include <span>
#include <stdexcept>
#include <string_view>

namespace civ5::runtime {
namespace {
constexpr std::array<std::uint8_t,8> magic{'C','I','V','B','F','L','T','1'};
constexpr std::size_t header_size=8+4+64*3+16+4+4;
constexpr std::size_t record_size=4*3;
void need(bool ok,const char *why){if(!ok)throw std::runtime_error(why);}
void sha(std::string_view s){need(s.size()==64&&std::all_of(s.begin(),s.end(),[](char c){return(c>='0'&&c<='9')||(c>='a'&&c<='f');}),"Unit serial checkpoint invalid SHA256");}
struct Writer{
 std::vector<std::uint8_t> b;
 void u32(std::uint32_t v){for(unsigned i=0;i<4;++i)b.push_back(static_cast<std::uint8_t>(v>>(8*i)));}
 void hash(std::string_view s){sha(s);b.insert(b.end(),s.begin(),s.end());}
};
struct Reader{
 std::span<const std::uint8_t>b;std::size_t at=8;
 std::uint32_t u32(){need(at<=b.size()&&b.size()-at>=4,"Unit serial checkpoint truncated");std::uint32_t v=0;for(unsigned i=0;i<4;++i)v|=std::uint32_t(b[at++])<<(8*i);return v;}
 std::string hash(){need(at<=b.size()&&b.size()-at>=64,"Unit serial checkpoint truncated hash");std::string s(reinterpret_cast<const char*>(b.data()+at),64);at+=64;sha(s);return s;}
};
struct Record{std::uint32_t actor,before,after;};
struct Decoded{std::string payload,rules,baseline;std::array<std::uint8_t,16>guid;std::uint32_t wrap;std::vector<Record>records;};
void serial(std::uint32_t v){need(v>=8192&&v<=static_cast<std::uint32_t>(INT32_MAX)&&v%8192==0,"Unit serial checkpoint invalid native serial");}
Decoded decode(const saves::CivisSaveSection&s){
 need(s.id==bnw_unit_serial_checkpoint_section_id&&s.flags==0&&s.bytes.size()>=header_size&&s.bytes.size()<=header_size+64*record_size&&std::equal(magic.begin(),magic.end(),s.bytes.begin()),"Unit serial checkpoint unsupported section");
 Reader in{s.bytes};need(in.u32()==1,"Unit serial checkpoint unsupported version");Decoded d{in.hash(),in.hash(),in.hash(),{},0, {}};
 std::copy_n(s.bytes.begin()+static_cast<std::ptrdiff_t>(in.at),16,d.guid.begin());in.at+=16;
 d.wrap=in.u32();need(d.wrap<=3,"Unit serial checkpoint invalid wrap bits");const auto n=in.u32();need(n>0&&n<=64&&s.bytes.size()==header_size+static_cast<std::size_t>(n)*record_size,"Unit serial checkpoint invalid sparse extent");
 for(std::uint32_t i=0;i<n;++i){Record r{in.u32(),in.u32(),in.u32()};serial(r.before);serial(r.after);need(r.actor<64&&(d.records.empty()||r.actor>d.records.back().actor)&&r.after>r.before,"Unit serial checkpoint unordered actor or regressed serial");d.records.push_back(r);}
 need(in.at==s.bytes.size(),"Unit serial checkpoint trailing bytes");return d;
}
const saves::NamedSaveInteger& serial_field(const saves::SavedPlayerObservation&p){
 const auto&h=p.units.header;constexpr std::array<std::string_view,5>names{"num_slots","last_index","free_list_head","free_list_count","current_id"};
 need(h.fields.size()==5,"Unit serial checkpoint invalid FLTA field count");for(std::size_t i=0;i<5;++i)need(h.fields[i].name&&names[i]==h.fields[i].name,"Unit serial checkpoint foreign FLTA schema");
 const auto v=h.fields[4].value;need(v>=8192&&v<=INT32_MAX&&v%8192==0,"Unit serial checkpoint invalid current_id");return h.fields[4];
}
std::int64_t unit_scalar(const saves::SavedUnitObservation&u,std::string_view key){
 const saves::UnitCoreField*out=nullptr;for(const auto&f:u.core.fields)if(f.name&&key==f.name){need(!out,"Unit serial checkpoint duplicate unit field");out=&f;}
 need(out&&out->encoding==saves::UnitCoreEncoding::i32&&out->values.size()==1,"Unit serial checkpoint missing native unit scalar");return out->values.front();
}
void flta(const saves::SavedPlayerObservation&p){
 const auto&h=p.units.header;const auto current=serial_field(p).value;
 const auto slots=h.fields[0].value,last=h.fields[1].value,head=h.fields[2].value,count=h.fields[3].value;
 need(slots>=0&&slots<=8192&&(slots==0||(slots&(slots-1))==0)&&h.source_slots==static_cast<std::uint64_t>(slots)&&h.next_free_indices.size()==static_cast<std::size_t>(slots)&&last>=-1&&last<slots&&count>=0&&count<=last+1&&h.wire_unit_count>=0&&static_cast<std::uint64_t>(h.wire_unit_count)==p.units.units.size(),"Unit serial checkpoint corrupt FLTA extent");
 std::set<std::int64_t>used,ids;std::int64_t previous=-1;std::size_t index=0;for(const auto&u:p.units.units){const auto id=unit_scalar(u,"m_iID"),owner=unit_scalar(u,"m_eOwner");need(u.index==index++&&id>=0&&id<=INT32_MAX&&owner==p.saved_player_id&&ids.insert(id).second,"Unit serial checkpoint invalid existing full ID/owner");const auto slot=id&8191;need(slot>previous&&slot<=last&&(id&~std::int64_t{8191})<current&&used.insert(slot).second&&h.next_free_indices.at(static_cast<std::size_t>(slot))==-1,"Unit serial checkpoint existing ID aliases free slot or current serial");previous=slot;}
 auto next=head;for(std::int64_t i=0;i<count;++i){need(next>=0&&next<=last&&used.insert(next).second,"Unit serial checkpoint corrupt free chain");next=h.next_free_indices.at(static_cast<std::size_t>(next));}
 need(next==-1&&used.size()==static_cast<std::size_t>(last+1),"Unit serial checkpoint incomplete FLTA free chain");
}
void roster(const saves::SavedSessionSnapshot&w){
 need(w.all_players&&w.all_players->players.size()==64,"Unit serial checkpoint requires full BNW roster");
 for(std::size_t i=0;i<64;++i){const auto&p=w.all_players->players[i];need(p.index==i,"Unit serial checkpoint actor index alias");flta(p);}
}
}
void validate_bnw_unit_serial_checkpoint_shape(const saves::CivisSaveSection&s){(void)decode(s);}
void require_bnw_unit_serial_export_unchanged(const saves::SavedSessionSnapshot&w,const saves::SavedSessionSnapshot&o){
 need(w.all_players&&o.all_players&&w.all_players->players.size()==o.all_players->players.size(),"Unit serial export missing roster");
 for(std::size_t i=0;i<o.all_players->players.size();++i){const auto&a=o.all_players->players[i],&b=w.all_players->players[i];need(a.index==b.index&&a.saved_player_id==b.saved_player_id,"Unit serial export actor changed");need(serial_field(a).value==serial_field(b).value,"BNW partial export cannot discard earned FLTA serial; closed section45 export required");}
}
saves::CivisSaveSection make_bnw_unit_serial_checkpoint(const saves::SavedSessionSnapshot&w,const saves::SavedSessionSnapshot&o,const SessionCheckpointIdentity&id){
 sha(id.payload_sha256);sha(id.rules_sha256);roster(o);roster(w);need(w.map_header.guid==o.map_header.guid&&w.map_header.wrap_x==o.map_header.wrap_x&&w.map_header.wrap_y==o.map_header.wrap_y,"Unit serial checkpoint map identity changed");
 auto masked=w;std::vector<Record>changes;
 for(std::size_t i=0;i<64;++i){const auto before=serial_field(o.all_players->players[i]).value,after=serial_field(w.all_players->players[i]).value;need(after>=before,"Unit serial checkpoint native serial regressed");if(after!=before){need(o.all_players->players[i].saved_player_id==static_cast<std::int64_t>(i)&&w.all_players->players[i].saved_player_id==static_cast<std::int64_t>(i),"Unit serial checkpoint changed actor alias");changes.push_back({static_cast<std::uint32_t>(i),static_cast<std::uint32_t>(before),static_cast<std::uint32_t>(after)});}masked.all_players->players[i].units.header.fields[4].value=before;}
 require_bnw_unit_checkpoint_unchanged_except_serial_mask(masked,o);need(!changes.empty(),"Unit serial checkpoint has no serial change");
 Writer out;out.b.insert(out.b.end(),magic.begin(),magic.end());out.u32(1);out.hash(id.payload_sha256);out.hash(id.rules_sha256);out.hash(bnw_unit_checkpoint_original_identity_digest(o));out.b.insert(out.b.end(),o.map_header.guid.begin(),o.map_header.guid.end());out.u32((o.map_header.wrap_x?1U:0U)|(o.map_header.wrap_y?2U:0U));out.u32(static_cast<std::uint32_t>(changes.size()));
 for(const auto&r:changes){out.u32(r.actor);out.u32(r.before);out.u32(r.after);}saves::CivisSaveSection result{bnw_unit_serial_checkpoint_section_id,0,std::move(out.b)};validate_bnw_unit_serial_checkpoint_shape(result);return result;
}
void restore_bnw_unit_serial_checkpoint(const saves::CivisSaveSection&s,saves::SavedSessionSnapshot&w,const SessionCheckpointIdentity&id){
 sha(id.payload_sha256);sha(id.rules_sha256);const auto d=decode(s);roster(w);
 need(d.payload==id.payload_sha256&&d.rules==id.rules_sha256,"Unit serial checkpoint source/rules mismatch");need(d.guid==w.map_header.guid&&d.wrap==((w.map_header.wrap_x?1U:0U)|(w.map_header.wrap_y?2U:0U))&&d.baseline==bnw_unit_checkpoint_original_identity_digest(w),"Unit serial checkpoint original world/FLTA identity mismatch");
 for(const auto&r:d.records){const auto&p=w.all_players->players.at(r.actor);need(p.saved_player_id==static_cast<std::int64_t>(r.actor)&&serial_field(p).value==r.before,"Unit serial checkpoint original actor/current_id mismatch");}
 // No allocation/callback after all canonical shape, identity and FLTA checks.
 for(const auto&r:d.records)w.all_players->players[r.actor].units.header.fields[4].value=r.after;
}
} // namespace civ5::runtime
