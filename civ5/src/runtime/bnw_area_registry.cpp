#include "civ5/runtime/bnw_area_registry.hpp"
#include "civ5/saves/session_snapshot.hpp"
#include <climits>
#include <set>
#include <stdexcept>
#include <string_view>
#include <type_traits>
namespace civ5::runtime {
namespace {
void need(bool b,const char* m){if(!b)throw std::runtime_error(m);}
std::int32_t integer(std::int64_t n){need(n>=INT32_MIN&&n<=INT32_MAX,"Area registry native int overflow");return static_cast<std::int32_t>(n);}
const auto& named(const auto& rows,std::string_view name){
 using T=typename std::remove_cvref_t<decltype(rows)>::value_type;const T* found=nullptr;
 for(const auto& r:rows)if(r.name&&std::string_view(r.name)==name){need(!found,"Area registry duplicate source field");found=&r;}
 need(found,"Area registry missing source field");return *found;
}
const saves::MapRegionContainer& validated_areas(const saves::SavedSessionSnapshot& world){
 need(world.map_regions&&world.map_regions->layout==saves::GameStateLayout::expansion2_version1,"Area registry BNW source missing");
 const auto& areas=named(world.map_regions->containers,"areas");
 need(areas.legacy_metadata.size()==5,"Area registry native FLTA metadata missing");
 const auto slots=integer(areas.legacy_metadata[0]),last=integer(areas.legacy_metadata[1]),free=integer(areas.legacy_metadata[3]);
 need(slots>=0&&slots<=8192&&last>=-1&&last<slots&&free>=0&&
      std::int64_t(last)+1-free==areas.wire_count&&areas.wire_count==static_cast<std::int64_t>(areas.records.size())&&
      areas.next_free.size()==static_cast<std::size_t>(slots),"Area registry native FLTA shape stale");
 std::set<std::int32_t> occupied;
 for(const auto& area:areas.records){
  const auto id=integer(named(area.fields,"id").value),slot=id&8191;
  need(id>=0&&slot<=last&&slot<slots&&occupied.insert(slot).second,"Area registry native slot ambiguous");
 }
 return areas;
}
}
void validate_bnw_area_registry(const saves::SavedSessionSnapshot& world){(void)validated_areas(world);}
const saves::MapRegionRecord* resolve_bnw_area_registry(const saves::SavedSessionSnapshot& world,std::int32_t handle){
 if(handle==-1)return nullptr; // Native INVALID_INDEX: do not borrow the FLTA.
 const auto& areas=validated_areas(world);const saves::MapRegionRecord* found=nullptr;
 for(const auto& area:areas.records){const auto id=integer(named(area.fields,"id").value),slot=id&8191;
  if(slot==(handle&8191)&&((handle&~8191)==0||id==handle))found=&area;
 }
 return found;
}
}
