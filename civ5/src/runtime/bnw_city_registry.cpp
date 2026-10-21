#include "civ5/runtime/bnw_city_registry.hpp"
#include "civ5/runtime/city_yields.hpp"
#include "civ5/saves/session_snapshot.hpp"
#include <climits>
#include <cstddef>
#include <set>
#include <stdexcept>
#include <string_view>
#include <type_traits>
namespace civ5::runtime {
namespace {
constexpr auto bnw=saves::GameStateLayout::expansion2_version1;
void need(bool value,const char* reason){if(!value)throw std::runtime_error(reason);}
std::int32_t integer(std::int64_t value){need(value>=INT32_MIN&&value<=INT32_MAX,"City strategy operand exceeds native int");return static_cast<std::int32_t>(value);}
auto& named(auto& rows,std::string_view name){using Row=std::remove_reference_t<decltype(rows.front())>;Row* found=nullptr;for(auto& row:rows)if(row.name&&name==row.name){need(!found,"Duplicate city strategy source field");found=&row;}need(found,"Missing city strategy current source field");return *found;}
std::int32_t scalar(const auto& rows,std::string_view name){const auto& field=named(rows,name);need(field.values.size()==1,"City strategy scalar shape differs");return integer(field.values[0]);}
}
const saves::SavedCityObservation *resolve_bnw_city_registry(const saves::SavedSessionSnapshot &world,const CityYieldContext &cities,std::int32_t owner,std::int32_t id){
 if(owner<0||owner>=64||id==-1)return nullptr;
 need(cities.profile==content::TechnologyProfile::bnw&&world.all_players&&world.all_players->layout==bnw&&world.all_players->players.size()==64,
      "Current city lookup requires SAME owned BNW player/city registry" );
 const auto &p=world.all_players->players.at(static_cast<std::size_t>(owner));const auto &live=cities.players.at(owner);const auto &c=p.cities;
 need(p.saved_player_id==owner&&c.layout==bnw&&c.header.layout==bnw&&c.header.wire_city_count==static_cast<std::int64_t>(c.cities.size())&&c.cities.size()==live.cities.size(),"City strategy native city roster unknown");
 const auto last=integer(named(c.header.fields,"last_index").value);const auto free=integer(named(c.header.fields,"free_list_count").value);need(last>=-1&&free>=0&&static_cast<std::int64_t>(last)-free+1==static_cast<std::int64_t>(c.cities.size()),"City strategy native FLTA GetCount differs from current roster");need(named(c.header.fields,"num_slots").value==static_cast<std::int64_t>(c.header.source_slots),"City strategy native FLTA capacity stale");
 const saves::SavedCityObservation *found=nullptr;std::set<std::int32_t> slots;
 for(const auto &raw:c.cities){const auto current=scalar(raw.core.fields,"m_iID");const auto slot=current&8191;const auto &owned=live.cities.at(current);
  need(raw.core.layout==bnw&&current>=0&&slot<=last&&static_cast<std::uint64_t>(slot)<c.header.source_slots&&slots.insert(slot).second&&scalar(raw.core.fields,"m_eOwner")==owner&&owned.id==current&&owned.owner==owner&&owned.source_core==std::optional{std::pair{raw.core.start_offset,raw.core.end_offset}},"City strategy native city FLTA/source binding differs");
  if(slot==(id&8191)&&((id&~8191)==0||current==id))found=&raw;
 }return found;
}
const CityYieldState* resolve_bnw_city_yield_registry(
 const saves::SavedSessionSnapshot& world,const CityYieldContext& cities,
 std::int32_t owner,std::int32_t handle){
 const auto* raw=resolve_bnw_city_registry(world,cities,owner,handle);
 if(!raw)return nullptr;
 return &cities.players.at(owner).cities.at(scalar(raw->core.fields,"m_iID"));
}
}