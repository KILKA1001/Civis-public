#include "civ5/saves/session_snapshot.hpp"
#include "civ5/runtime/bnw_city_current_growth_threshold.hpp"
#include "civ5/runtime/file_identity.hpp"
#include <bit>
#include <climits>
#include <cstdint>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <optional>
#include <span>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string_view>
#include <type_traits>
using namespace civ5;
namespace {
constexpr auto bnw=saves::GameStateLayout::expansion2_version1;
void need(bool v,const char* why){if(!v)throw std::runtime_error(why);}
const auto& named(const auto& rows,std::string_view key){
 using T=typename std::remove_cvref_t<decltype(rows)>::value_type;const T* result=nullptr;
 for(const auto& f:rows)if(f.name&&std::string_view(f.name)==key){need(!result,"duplicate scalar name");result=&f;}
 need(result,"missing scalar name");return *result;
}
std::string hex(std::span<const std::uint8_t> bytes){
 std::ostringstream out;out<<std::hex<<std::setfill('0');for(const auto b:bytes)out<<std::setw(2)<<static_cast<unsigned>(b);return out.str();
}
template<class Field>std::int32_t scalar(const Field& f,std::span<const std::uint8_t> bytes,bool boolean=false){
 using E=decltype(f.encoding);need(f.encoding==(boolean?E::boolean:E::i32)&&f.values.size()==1&&!f.wire_count,
  "native scalar encoding/arity unavailable");
 const auto width=boolean?std::uint64_t{1}:std::uint64_t{4};
 need(f.start_offset<=bytes.size()&&f.end_offset>=f.start_offset&&f.end_offset<=bytes.size()&&f.end_offset-f.start_offset==width,
  "native scalar exact offset/width unavailable");
 const auto begin=static_cast<std::size_t>(f.start_offset);std::int32_t value=0;
 if(boolean){need(bytes[begin]<=1,"native raw bool not0/1");value=bytes[begin];}
 else{std::uint32_t bits=0;for(unsigned n=0;n<4;++n)bits|=static_cast<std::uint32_t>(bytes[begin+n])<<(8u*n);value=std::bit_cast<std::int32_t>(bits);}
 need(f.values.front()>=INT32_MIN&&f.values.front()<=INT32_MAX&&f.values.front()==value,
  "observed scalar differs from exact LE source bytes");return value;
}
template<class Field>void field_wire(const Field& f,std::span<const std::uint8_t> bytes){
 std::cout<<" offset="<<f.start_offset<<" end="<<f.end_offset<<" LE="
  <<hex(bytes.subspan(static_cast<std::size_t>(f.start_offset),static_cast<std::size_t>(f.end_offset-f.start_offset)));
}
std::optional<bool> human(const saves::PregameObservation& preg,std::size_t slot){
 const auto status=preg.archive.integer_arrays.at("slot_status").at(slot);
 if(status==3)return true; // SS_TAKEN
 if(status==4)return preg.archive.integer_arrays.at("slot_claims").at(slot)==2; // assigned observer
 if(status!=0)return false;
 const auto type=preg.archive.integers.at("game_type");if(type==2)return true; // native hotseat first
 const auto pit=runtime::detail::bnw_growth_saved_pitboss_option(preg.game_options);
 if(!pit)return std::nullopt; // actual DB Default producer not part of source-only diagnostic
 if(*pit!=0){return true;}return type==3;
}
void observe(const std::filesystem::path& path,std::size_t ordinal){
 const auto before=runtime::identify_file(path);
 std::cout<<"SAVE ordinal="<<ordinal<<" sha="<<before.sha256<<" bytes="<<before.bytes<<'\n';
 saves::SavedSessionDecodeContext context;context.decode_map_regions=true;context.decode_teams=true;
 context.decode_all_players=true;context.decode_supporting_classes=true;context.all_players=saves::AllPlayersContext{};
 saves::SavePayloadBudget budget;budget.maximum_inflated_bytes=128ull*1024*1024;
 auto saved=saves::read_saved_session(path,context,budget);
 const auto& src=saved.source;const auto& s=saved.snapshot;
 need(src.container==saves::SaveContainerKind::civ5&&src.header.mods.empty(),"diagnostic requires original unmodded Civ5 container");
 bool expansion=false;for(const auto& d:src.header.dlcs)if(d.id=="6da07636-4123-4018-b643-6575b4ec336b")expansion=true;
 need(expansion&&s.game_state.layout==bnw&&s.pregame&&s.pregame->layout==bnw&&s.all_players&&s.all_players->layout==bnw&&
  s.all_players->context.player_count==64&&s.all_players->players.size()==64&&s.payload_end_offset==src.bytes.size(),
  "source BNW roster/payload/header boundary unavailable");
 const auto& preg=*s.pregame;
 for(const auto* name:{"slot_status","slot_claims","team_types"})need(preg.archive.integer_arrays.at(name).size()==64,"native Pregame roster64 unavailable");
 need(preg.archive.flag_arrays.at("minor_nation_civs").size()==64,"native minor roster64 unavailable");
 std::cout<<"HEADER revision="<<src.header.revision<<" turn="<<src.header.turn<<" civ="<<src.header.civilization
  <<" world="<<src.header.world_size<<" payloadSHA="<<runtime::identify_bytes(src.bytes).sha256
  <<" fullProfileVerified=UNAVAILABLE loadHooksApplied=0\n";
 const std::span<const std::uint8_t> bytes=src.bytes;std::set<std::int32_t> ids;
 bool identity=true;std::size_t total_cities=0,major_cities=0,unknown_control_cities=0;
 for(std::size_t i=0;i<64;++i){const auto& p=s.all_players->players.at(i);
  need(p.core.layout==bnw&&p.core.version&&p.cities.layout==bnw&&p.cities.header.layout==bnw&&p.tail.layout==bnw,"native player/city/tail schema unavailable");
  const auto& id_field=named(p.core.fields,"m_eID");const auto id=scalar(id_field,bytes);
  const auto alive=scalar(named(p.core.fields,"m_bAlive"),bytes,true);const auto ever=scalar(named(p.core.fields,"m_bEverAlive"),bytes,true);
  need(id>=0&&id<64,"decoded player ID outside native64");const auto slot=static_cast<std::size_t>(id);
  const bool same=p.index==i&&p.saved_player_id==id&&slot==i&&ids.insert(id).second;identity=identity&&same;
  const auto control=human(preg,slot);const bool minor=preg.archive.flag_arrays.at("minor_nation_civs").at(slot);
  const auto count=p.cities.cities.size();total_cities+=count;
  std::cout<<"PLAYER outer="<<i<<" index="<<p.index<<" savedID="<<p.saved_player_id<<" rawID="<<id<<" identity="<<same
   <<" coreVersion="<<*p.core.version<<" core="<<p.core.start_offset<<':'<<p.core.end_offset
   <<" alive="<<alive<<" everAlive="<<ever<<" status="<<preg.archive.integer_arrays.at("slot_status").at(slot)
   <<" claim="<<preg.archive.integer_arrays.at("slot_claims").at(slot)<<" team="<<preg.archive.integer_arrays.at("team_types").at(slot)
   <<" human="<<(control?(*control?"1":"0"):"UNAVAILABLE_DB_PITBOSS_DEFAULT")<<" minor="<<minor<<" barbarian="<<(id==63)
   <<" cities="<<count<<" cityWireCount="<<p.cities.header.wire_city_count
   <<" units="<<p.units.units.size()<<" serializedTail="<<p.tail.start_offset<<':'<<p.tail.base_player_end_offset<<':'<<p.tail.end_offset
   <<" ReadTailInitOwner=NOT_APPLIED";field_wire(id_field,bytes);std::cout<<'\n';
  if(alive&&id<22&&!minor&&same){if(!control)unknown_control_cities+=count;else if(!*control)major_cities+=count;}
  for(const auto& city:p.cities.cities){need(city.core.layout==bnw&&city.core.version,"native city schema unavailable");
   const auto& owner_field=named(city.core.fields,"m_eOwner");const auto owner=scalar(owner_field,bytes);
   const auto full=scalar(named(city.core.fields,"m_iID"),bytes);const auto previous=scalar(named(city.core.fields,"m_ePreviousOwner"),bytes);
   const auto original=scalar(named(city.core.fields,"m_eOriginalOwner"),bytes);const auto x=scalar(named(city.core.fields,"m_iX"),bytes),y=scalar(named(city.core.fields,"m_iY"),bytes);
   const auto pop=scalar(named(city.core.fields,"m_iPopulation"),bytes);const auto puppet=scalar(named(city.core.fields,"m_bPuppet"),bytes,true);
   identity=identity&&(owner==id);
   std::cout<<"CITY collection="<<id<<" rawOwner="<<owner<<" previous="<<previous<<" original="<<original<<" fullID="<<full
    <<" x="<<x<<" y="<<y<<" population="<<pop<<" puppet="<<puppet<<" version="<<*city.core.version
    <<" core="<<city.core.start_offset<<':'<<city.core.end_offset;field_wire(owner_field,bytes);std::cout<<'\n';
  }
 }
 const auto after=runtime::identify_file(path);need(before.bytes==after.bytes&&before.sha256==after.sha256,"original source save changed while read");
 std::cout<<"SUMMARY cities="<<total_cities<<" sourceIdentity64="<<identity<<" aliveMajorNonhumanCities="<<major_cities
  <<" unknownMajorControlCities="<<unknown_control_cities<<" originalUnchanged=1 numericGrowthOwner=NOT_ISSUED city63Provenance=UNKNOWN\n";
 need(identity,"actual decoded identity or city collection owner mismatch; preserve printed witnesses");
}
}
int main(int argc,char** argv){
 if(argc<2||argc>6){std::cerr<<"Usage: civis_bnw_save_roster_diagnostic SAVE1 [SAVE2..SAVE5]\n";return 2;}
 bool failed=false;for(int i=1;i<argc;++i){try{observe(std::filesystem::path(argv[i]),static_cast<std::size_t>(i));}
  catch(const std::exception& e){std::cerr<<"SAVE ordinal="<<i<<" FAILURE="<<e.what()<<'\n';failed=true;}}
 return failed?1:0;
}
