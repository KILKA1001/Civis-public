#include "civ5/runtime/bnw_population_totals_checkpoint.hpp"
#include "civ5/runtime/file_identity.hpp"
#include <algorithm>
#include <array>
#include <climits>
#include <map>
#include <set>
#include <span>
#include <stdexcept>
#include <string_view>
#include <type_traits>

namespace civ5::runtime {
namespace {
constexpr auto bnw=saves::GameStateLayout::expansion2_version1;
constexpr std::array<std::uint8_t,8> magic{'C','I','V','B','P','O','P','1'};
void need(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}
std::string_view name(const char* p){return p?std::string_view(p):std::string_view{};}
const auto& named(const auto& rows,std::string_view key){
 const typename std::remove_cvref_t<decltype(rows)>::value_type* found=nullptr;
 for(const auto& r:rows)if(name(r.name)==key){need(!found,"Population duplicate field/array/container");found=&r;}
 need(found,"Population native field/array/container unavailable");return *found;
}
std::int64_t scalar(const auto& city,std::string_view key){
 const auto& f=named(city.core.fields,key);need(f.values.size()==1,"Population city/player scalar extent");return f.values.front();
}
std::int32_t population(std::int64_t n){need(n>=0&&n<=INT32_MAX,"Population total outside native nonnegative int");return static_cast<std::int32_t>(n);}
void hash_shape(std::string_view s){need(s.size()==64&&std::all_of(s.begin(),s.end(),[](char c){return(c>='0'&&c<='9')||(c>='a'&&c<='f');}),"Population invalid SHA256");}
struct Writer {
 std::vector<std::uint8_t> bytes;
 void u32(std::uint32_t n){for(unsigned i=0;i<4;++i)bytes.push_back(static_cast<std::uint8_t>(n>>(i*8)));}
 void u64(std::uint64_t n){for(unsigned i=0;i<8;++i)bytes.push_back(static_cast<std::uint8_t>(n>>(i*8)));}
 void text(std::string_view s){bytes.insert(bytes.end(),s.begin(),s.end());}
 void label(std::string_view s){need(!s.empty()&&s.size()<=UINT32_MAX,"Population empty identity name");u32(static_cast<std::uint32_t>(s.size()));text(s);}
 void hash(std::string_view s){hash_shape(s);text(s);}
 void pop(std::int64_t n){u32(static_cast<std::uint32_t>(population(n)));}
};
struct Reader {
 std::span<const std::uint8_t> bytes;std::size_t at=8;
 std::uint32_t u32(){need(at<=bytes.size()&&bytes.size()-at>=4,"Population checkpoint truncated");std::uint32_t n=0;for(unsigned i=0;i<4;++i)n|=std::uint32_t(bytes[at++])<<(i*8);return n;}
 std::string hash(){need(at<=bytes.size()&&bytes.size()-at>=64,"Population hash truncated");std::string s(reinterpret_cast<const char*>(bytes.data()+at),64);at+=64;hash_shape(s);return s;}
 std::int32_t pop(){return population(u32());}
};
const auto& areas(const saves::SavedSessionSnapshot& w){
 need(w.map_regions&&w.map_regions->layout==bnw&&w.map_regions->dimensions,"Population original area dimensions unavailable");
 const auto& a=named(w.map_regions->containers,"areas");
 need(a.wire_count>=0&&static_cast<std::uint64_t>(a.wire_count)==a.records.size()&&a.records.size()<=w.map_header.grid.size(),"Population area container malformed");return a;
}
const auto& area_players(const saves::MapRegionRecord& a){const auto& v=named(a.arrays,"population_per_player");need(v.columns==1&&v.values.size()==80,"Population native area array must retain all80 original slots");for(auto n:v.values)(void)population(n);return v.values;}
struct Totals {std::int32_t game=0;std::array<std::int32_t,64> teams{};std::map<std::int32_t,std::array<std::int32_t,81>> areas;};
Totals read_totals(const saves::SavedSessionSnapshot& w){
 need(w.game_state.layout==bnw&&w.map_header.layout==bnw&&w.all_players&&w.all_players->layout==bnw&&w.all_players->players.size()==64&&w.teams&&w.teams->layout==bnw&&w.teams->teams.size()==64&&w.pregame&&w.pregame->layout==bnw&&w.map_plots&&w.map_plots->plots.size()==w.map_header.grid.size(),"Population requires complete plain BNW world");
 need(w.all_players->context.player_count==64&&w.all_players->context.profile_extensions.values.empty()&&w.teams->dimensions.teams==64&&w.teams->dimensions.players==64&&w.teams->dimensions.profile_extensions.values.empty(),"Population mixed/unknown player or team profile");
 need(w.map_regions&&w.map_regions->dimensions&&w.map_regions->dimensions->fixed_players==80&&w.map_regions->dimensions->players==64,"Population BNW fixed-player dimensions differ");
 const auto* prefix=std::get_if<saves::Expansion2PrefixObservation>(&w.game_state.prefix);need(prefix,"Population BNW Game prefix absent");Totals t;t.game=population(prefix->scalars.at(10));
 for(std::size_t i=0;i<64;++i){const auto& p=w.all_players->players[i];need(p.index==i,"Population original player index alias");const auto& team=w.teams->teams[i];need(team.index==i&&named(team.fields,"id").value==static_cast<std::int64_t>(i),"Population actual team identity differs");t.teams[i]=population(named(team.fields,"population").value);}
 for(const auto& a:areas(w).records){const auto id=named(a.fields,"id").value;need(id>=0&&id<=INT32_MAX,"Population area fullID invalid");std::array<std::int32_t,81> values{};values[0]=population(named(a.fields,"population").value);const auto& v=area_players(a);for(std::size_t i=0;i<80;++i)values[i+1]=population(v[i]);need(t.areas.emplace(static_cast<std::int32_t>(id),values).second,"Population duplicate area fullID");}
 return t;
}
std::string binding(const saves::SavedSessionSnapshot& w){
 (void)read_totals(w);Writer out;out.u64(w.map_header.grid.width);out.u64(w.map_header.grid.height);out.u32(w.map_header.wrap_x?1:0);out.u32(w.map_header.wrap_y?1:0);out.bytes.insert(out.bytes.end(),w.map_header.guid.begin(),w.map_header.guid.end());
 out.u64(w.game_state.start_offset);out.u64(w.game_state.end_offset);const auto& a=areas(w);out.u64(a.start_offset);out.u64(a.end_offset);out.u64(static_cast<std::uint64_t>(a.wire_count));out.u64(a.legacy_metadata.size());for(auto n:a.legacy_metadata)out.u64(static_cast<std::uint64_t>(n));out.u64(a.next_free.size());for(auto n:a.next_free)out.u64(static_cast<std::uint64_t>(n));
 for(const auto& r:a.records){out.u64(r.start_offset);out.u64(r.end_offset);need(r.version.has_value(),"Population area version absent");out.u64(*r.version);out.u64(static_cast<std::uint64_t>(named(r.fields,"id").value));out.u64(r.fields.size());for(const auto& f:r.fields)out.label(name(f.name));out.u64(r.arrays.size());for(const auto& v:r.arrays){out.label(name(v.name));out.u64(v.columns);out.u64(v.values.size());}}
 const auto& teams=w.pregame->archive.integer_arrays.at("team_types");need(teams.size()==64,"Population player/team binding extent differs");
 for(std::size_t i=0;i<64;++i){const auto& p=w.all_players->players[i];out.u64(i);out.u64(static_cast<std::uint64_t>(p.saved_player_id));out.u64(static_cast<std::uint64_t>(teams[i]));out.u64(p.start_offset);out.u64(p.end_offset);out.u64(p.cities.cities.size());for(const auto& c:p.cities.cities){need(c.core.layout==bnw&&c.production.layout==bnw&&c.management.layout==bnw&&c.tail.layout==bnw&&c.core.start_offset<c.core.end_offset&&c.production.start_offset==c.core.end_offset&&c.production.start_offset<c.production.end_offset&&c.management.start_offset==c.production.end_offset&&c.management.start_offset<c.management.end_offset&&c.tail.start_offset==c.management.end_offset&&c.tail.start_offset<c.tail.end_offset&&p.start_offset<=p.cities.start_offset&&p.cities.start_offset<=c.core.start_offset&&c.tail.end_offset<=p.cities.end_offset&&p.cities.end_offset<=p.end_offset,"Population actual city block layout/order/source bounds differ");out.u64(c.core.start_offset);out.u64(c.tail.end_offset);out.u64(static_cast<std::uint64_t>(scalar(c,"m_iID")));const auto x=scalar(c,"m_iX"),y=scalar(c,"m_iY");need(x>=0&&y>=0&&static_cast<std::uint64_t>(x)<w.map_header.grid.width&&static_cast<std::uint64_t>(y)<w.map_header.grid.height,"Population city coordinate invalid");out.u64(static_cast<std::uint64_t>(x));out.u64(static_cast<std::uint64_t>(y));const auto index=w.map_header.grid.index(static_cast<std::uint64_t>(x),static_cast<std::uint64_t>(y));const auto& plot=w.map_plots->plots.at(static_cast<std::size_t>(index));need(plot.index==index,"Population city plot index differs");out.u64(plot.start_offset);out.u64(plot.end_offset);out.u64(static_cast<std::uint64_t>(named(plot.fields,"area").value));}const auto& t=w.teams->teams[i];out.u64(t.start_offset);out.u64(t.end_offset);out.u64(t.fields.size());for(const auto& f:t.fields)out.label(name(f.name));}
 return identify_bytes(out.bytes).sha256;
}
std::array<const saves::CivisSaveSection*,5> parts(const BnwPopulationTotalsCheckpointBundle& b){return {&b.players,&b.cities.core,&b.cities.production,&b.cities.management,&b.cities.tail};}
struct Decoded {std::string payload,rules,binding;std::array<std::string,5> companions;Totals totals;};
Decoded decode(const saves::CivisSaveSection& s){
 need(s.id==46&&s.flags==0&&s.bytes.size()>=788&&s.bytes.size()<=788+1'000'000ULL*328&&std::equal(magic.begin(),magic.end(),s.bytes.begin()),"Population unsupported section");Reader in{s.bytes};need(in.u32()==1,"Population unsupported version");Decoded d;d.payload=in.hash();d.rules=in.hash();d.binding=in.hash();for(auto& h:d.companions)h=in.hash();d.totals.game=in.pop();for(auto& n:d.totals.teams)n=in.pop();const auto count=in.u32();need(count<=1'000'000&&s.bytes.size()-in.at==static_cast<std::size_t>(count)*328,"Population area extent/trailing bytes invalid");std::int64_t previous=-1;for(std::uint32_t i=0;i<count;++i){const auto id=in.u32();need(id<=INT32_MAX&&id>previous,"Population area IDs unordered/foreign");previous=id;std::array<std::int32_t,81> v{};for(auto& n:v)n=in.pop();need(d.totals.areas.emplace(static_cast<std::int32_t>(id),v).second,"Population duplicate area");}need(in.at==s.bytes.size(),"Population trailing bytes");return d;
}
void paired(const saves::SavedSessionSnapshot& original,const saves::SavedSessionSnapshot& current,const Totals& after){
 const auto before=read_totals(original);need(binding(original)==binding(current)&&before.areas.size()==after.areas.size(),"Population world/roster/area membership changed");
 std::array<std::int64_t,64> player_delta{},team_delta{};std::map<std::int32_t,std::array<std::int64_t,81>> area_delta;for(const auto& [id,v]:before.areas){(void)v;area_delta.emplace(id,std::array<std::int64_t,81>{});}
 const auto& team_types=original.pregame->archive.integer_arrays.at("team_types");std::int64_t game_delta=0;bool changed=false;
 for(std::size_t p=0;p<64;++p){const auto& o=original.all_players->players[p];const auto& c=current.all_players->players[p];need(o.saved_player_id==c.saved_player_id&&o.cities.cities.size()==c.cities.cities.size(),"Population paired roster changed");for(std::size_t i=0;i<o.cities.cities.size();++i){const auto delta=static_cast<std::int64_t>(population(scalar(c.cities.cities[i],"m_iPopulation")))-population(scalar(o.cities.cities[i],"m_iPopulation"));changed|=delta!=0;player_delta[p]+=delta;const auto x=scalar(o.cities.cities[i],"m_iX"),y=scalar(o.cities.cities[i],"m_iY");const auto index=original.map_header.grid.index(static_cast<std::uint64_t>(x),static_cast<std::uint64_t>(y));const auto id=named(original.map_plots->plots.at(static_cast<std::size_t>(index)).fields,"area").value;need(id>=0&&id<=INT32_MAX&&area_delta.contains(static_cast<std::int32_t>(id)),"Population city area fullID missing");auto& a=area_delta.at(static_cast<std::int32_t>(id));a[0]+=delta;a[p+1]+=delta;}
  need(static_cast<std::int64_t>(population(scalar(c,"m_iTotalPopulation")))-population(scalar(o,"m_iTotalPopulation"))==player_delta[p],"Population player/city totals unpaired");if(player_delta[p]){const auto t=team_types[p];need(t>=0&&t<64,"Population changed actor team invalid");team_delta[static_cast<std::size_t>(t)]+=player_delta[p];}game_delta+=player_delta[p];}
 need(changed,"Population unchanged companion state");need(static_cast<std::int64_t>(after.game)-before.game==game_delta,"Population Game total unpaired");for(std::size_t t=0;t<64;++t)need(static_cast<std::int64_t>(after.teams[t])-before.teams[t]==team_delta[t],"Population Team total unpaired");
 for(const auto& [id,v]:before.areas){need(after.areas.contains(id),"Population area fullID absent");for(std::size_t i=0;i<81;++i)need(static_cast<std::int64_t>(after.areas.at(id)[i])-v[i]==area_delta.at(id)[i],"Population area/player totals unpaired");}
}
}
void validate_bnw_population_totals_checkpoint_shape(const saves::CivisSaveSection& s){(void)decode(s);}
void require_bnw_population_totals_export_unchanged(const saves::SavedSessionSnapshot& current,const saves::SavedSessionSnapshot& original){
 if(!original.teams||!original.map_regions||!original.map_plots||!original.all_players||!original.pregame)return;
 const auto a=read_totals(original),b=read_totals(current);need(binding(original)==binding(current)&&a.game==b.game&&a.teams==b.teams&&a.areas==b.areas,"BNW partial export cannot discard native population totals; section46 and genuine growth continuation restore required");
}
void validate_bnw_population_totals_checkpoint_companions(const saves::CivisSaveDocument& document){
 const saves::CivisSaveSection* totals=nullptr;std::array<const saves::CivisSaveSection*,5> companions{};
 for(const auto& s:document.sections){if(s.id==46){need(!totals,"Population duplicate totals section");totals=&s;}if(s.id>=24&&s.id<=28){auto& slot=companions[s.id-24];need(!slot,"Population duplicate companion section");slot=&s;}}
 need(totals,"Population totals section absent");const auto decoded=decode(*totals);for(std::size_t i=0;i<5;++i)need(companions[i]&&companions[i]->flags==0&&identify_bytes(companions[i]->bytes).sha256==decoded.companions[i],"Population missing/foreign companion bytes");
}
saves::SavedSessionSnapshot restore_bnw_population_totals_checkpoint_bundle(const BnwPopulationTotalsCheckpointBundle& b,const saves::SavedSessionSnapshot& original,const SessionCheckpointIdentity& identity){
 hash_shape(identity.payload_sha256);hash_shape(identity.rules_sha256);const auto d=decode(b.totals);need(d.payload==identity.payload_sha256&&d.rules==identity.rules_sha256&&d.binding==binding(original),"Population original party/rules/objects differ");const auto p=parts(b);for(std::size_t i=0;i<5;++i)need(p[i]->id==24+i&&p[i]->flags==0&&identify_bytes(p[i]->bytes).sha256==d.companions[i],"Population mandatory companion ID/hash differs");
 auto staged=restore_bnw_city_checkpoint_quadruple(b.cities,original,identity);auto players=original;restore_bnw_player_core_checkpoint(b.players,players,original,identity);for(std::size_t i=0;i<64;++i)staged.all_players->players[i].core=std::move(players.all_players->players[i].core);paired(original,staged,d.totals);
 // Every parse/roster/companion/delta/allocation check has completed. These
 // writes affect only the unpublished detached result, not any live owner.
 std::get<saves::Expansion2PrefixObservation>(staged.game_state.prefix).scalars.at(10)=d.totals.game;
 for(std::size_t i=0;i<64;++i)for(auto& f:staged.teams->teams[i].fields)if(name(f.name)=="population")f.value=d.totals.teams[i];
 for(auto& container:staged.map_regions->containers)if(name(container.name)=="areas")for(auto& a:container.records){const auto id=static_cast<std::int32_t>(named(a.fields,"id").value);const auto& values=d.totals.areas.at(id);for(auto& f:a.fields)if(name(f.name)=="population")f.value=values[0];for(auto& v:a.arrays)if(name(v.name)=="population_per_player")for(std::size_t i=0;i<80;++i)v.values[i]=values[i+1];}
 return staged;
}
namespace {
// Shared data encoder. No boolean can relax changed-only restoration: the
// comparison entry point supplies exactly the SAME snapshot in both roles.
BnwPopulationTotalsCheckpointBundle encode_population_bundle_data(
 const saves::SavedSessionSnapshot& current,const saves::SavedSessionSnapshot& original,
 const SessionCheckpointIdentity& identity,const Totals& totals){
 hash_shape(identity.payload_sha256);hash_shape(identity.rules_sha256);BnwPopulationTotalsCheckpointBundle b;b.players=make_bnw_player_core_checkpoint(current,original,identity,true);b.cities=make_bnw_city_checkpoint_quadruple(current,original,identity,true);
 auto core_only=original;for(std::size_t p=0;p<64;++p)for(std::size_t c=0;c<core_only.all_players->players[p].cities.cities.size();++c)core_only.all_players->players[p].cities.cities[c].core=current.all_players->players[p].cities.cities[c].core;b.cities.core=make_bnw_city_core_baseline_checkpoint(core_only,original,identity,true);
 Writer out;out.bytes.insert(out.bytes.end(),magic.begin(),magic.end());out.u32(1);out.hash(identity.payload_sha256);out.hash(identity.rules_sha256);out.hash(binding(original));for(const auto* p:parts(b))out.hash(identify_bytes(p->bytes).sha256);out.pop(totals.game);for(auto n:totals.teams)out.pop(n);need(totals.areas.size()<=1'000'000,"Population area count exceeds bounded format");out.u32(static_cast<std::uint32_t>(totals.areas.size()));for(const auto& [id,v]:totals.areas){out.u32(static_cast<std::uint32_t>(id));for(auto n:v)out.pop(n);}b.totals={46,0,std::move(out.bytes)};(void)decode(b.totals);return b;
}
}
BnwPopulationTotalsCheckpointBundle make_bnw_population_totals_checkpoint_bundle(const saves::SavedSessionSnapshot& current,const saves::SavedSessionSnapshot& original,const SessionCheckpointIdentity& identity){
 hash_shape(identity.payload_sha256);hash_shape(identity.rules_sha256);const auto totals=read_totals(current);paired(original,current,totals);
 auto b=encode_population_bundle_data(current,original,identity,totals);
 (void)restore_bnw_population_totals_checkpoint_bundle(b,original,identity);return b;
}
BnwPopulationTotalsCheckpointBundle make_bnw_population_totals_baseline_checkpoint_bundle(
 const saves::SavedSessionSnapshot& snapshot,const SessionCheckpointIdentity& identity){
 const auto totals=read_totals(snapshot);
 return encode_population_bundle_data(snapshot,snapshot,identity,totals);
}
}
