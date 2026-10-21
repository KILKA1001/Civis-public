#include "civ5/runtime/bnw_city_food_consumption.hpp"
#include <limits>
#include "civ5/runtime/bnw_city_specialist_counts.hpp"
#include <map>
#include <set>
#include <type_traits>
#include <stdexcept>
namespace civ5::runtime {
namespace {
constexpr auto bnw=saves::GameStateLayout::expansion2_version1;
void need(bool b,const char* m){if(!b)throw std::runtime_error(m);}
std::int32_t i32(std::int64_t n){need(n>=INT32_MIN&&n<=INT32_MAX,"BNW food consumption native int32 overflow");return static_cast<std::int32_t>(n);}
const auto& named(const auto& values,std::string_view name){using T=typename std::remove_cvref_t<decltype(values)>::value_type;const T* out=nullptr;for(const auto& f:values)if(f.name&&std::string_view(f.name)==name){need(!out,"BNW food consumption duplicate field");out=&f;}need(out,"BNW food consumption required field unavailable");return *out;}
std::int32_t scalar(const auto& fields,std::string_view name){const auto& f=named(fields,name);need(f.values.size()==1,"BNW food consumption scalar shape unavailable");return i32(f.values.front());}
std::int32_t city_scalar(const saves::CityCoreObservation& core,std::string_view name){const auto& f=named(core.fields,name);need(f.encoding==saves::CityCoreEncoding::i32&&!f.wire_count&&f.bytes.empty()&&f.hashes.empty()&&f.row_keys.empty()&&f.row_counts.empty(),"BNW food consumption city scalar encoding unavailable");return scalar(core.fields,name);}
std::int32_t player_scalar(const saves::PlayerCoreObservation& core,std::string_view name){const auto& f=named(core.fields,name);need(f.encoding==saves::PlayerCoreEncoding::i32&&!f.wire_count&&f.bytes.empty()&&f.hashes.empty()&&f.row_counts.empty(),"BNW food consumption player scalar encoding unavailable");return scalar(core.fields,name);}
struct Bound{const saves::SavedPlayerObservation* player;const saves::SavedCityObservation* city;};
Bound bind(const saves::SavedSessionSnapshot& world,std::int64_t owner,std::int64_t id){
 need(world.game_state.layout==bnw&&world.all_players&&world.all_players->layout==bnw,"BNW food consumption requires BNW world");
 Bound out{};for(const auto& p:world.all_players->players)if(p.saved_player_id==owner){need(!out.player&&p.core.layout==bnw&&p.cities.layout==bnw,"BNW food consumption owner shape/profile mismatch");out.player=&p;
  for(const auto& c:p.cities.cities)if(city_scalar(c.core,"m_iID")==id){need(!out.city&&c.core.layout==bnw&&city_scalar(c.core,"m_eOwner")==owner,"BNW food consumption city shape/profile mismatch");out.city=&c;}}
 need(out.player&&out.city,"BNW food consumption player/city absent");return out;
}
std::int32_t define(const content::GlobalDefineCache& d,std::string_view name){need(d.profile()==content::GlobalDefineProfile::bnw,"BNW food consumption define profile mismatch");const auto* v=d.find(name);need(v&&v->lookup.found&&v->lookup.error.empty()&&std::holds_alternative<std::int64_t>(v->lookup.value),"BNW food consumption diagnostic define unresolved");return i32(std::get<std::int64_t>(v->lookup.value));}
const saves::CityManagementBlock& citizens(const saves::SavedCityObservation& c){need(c.management.layout==bnw,"BNW food consumption citizen profile mismatch");return named(c.management.blocks,"citizens");}
bool same_rows(const auto& a,const auto& b){if(a.size()!=b.size())return false;for(std::size_t i=0;i<a.size();++i)if(a[i].type_hash!=b[i].type_hash||a[i].values!=b[i].values)return false;return true;}
}
BnwCityFoodConsumptionDiagnostic detail::diagnose_bnw_city_food_consumption_source(
 const saves::SavedSessionSnapshot& world,const content::RulesDatabase& db,
 const content::GlobalDefineCache& d,std::int64_t owner,std::int64_t id,std::int64_t extra,const BnwCitySpecialistCounts* source){
 if(source)source->require_same_world_database(world,db);
 const auto [p,c]=bind(world,owner,id);BnwCityFoodConsumptionDiagnostic q;q.owner_=owner;q.city_=id;
 q.player_span_={p->core.start_offset,p->core.end_offset};q.city_span_={c->core.start_offset,c->core.end_offset};
 q.population_=city_scalar(c->core,"m_iPopulation");q.population_extra_=i32(std::int64_t(q.population_)+i32(extra));
 const auto per_pop=define(d,"FOOD_CONSUMPTION_PER_POPULATION");
 q.total_=i32(std::int64_t(q.population_extra_)*per_pop);
 q.half_=player_scalar(p->core,"m_iHalfSpecialistFoodCount");
 if(q.half_>0){
  if(source){
   q.specialists_=source->total_for_city(owner,id);
   const auto& block=citizens(*c);q.citizens_span_={block.start_offset,block.end_offset};const auto& hash=named(block.hashes,"m_aiSpecialistCounts");
   q.specialist_wire_count_=hash.wire_count;q.raw_specialists_=hash.rows;
  }else{
  // These are diagnostic current DB slots, never authenticated native Info.
  std::map<std::uint32_t,std::uint64_t> slots;std::set<std::uint64_t> seen;
  const auto report=db.prefetch_info("Specialists",[&](auto slot,auto,const auto& row){const auto type=row.text("Type");need(type&&!type->empty()&&slot<=INT32_MAX,"BNW specialist diagnostic slot/type unavailable");need(seen.insert(slot).second&&slots.emplace(content::saved_info_type_hash(*type),slot).second,"BNW specialist duplicate slot/hash");return true;});
  need(report.all_succeeded()&&!report.holes&&!report.overlapping_ids&&report.slots<=INT32_MAX&&seen.size()==report.slots,"BNW specialist diagnostic dense Info span unavailable");
  const auto default_id=define(d,"DEFAULT_SPECIALIST");
  const auto& block=citizens(*c);q.citizens_span_={block.start_offset,block.end_offset};const auto& hash=named(block.hashes,"m_aiSpecialistCounts");
  need(hash.wire_count>=0&&static_cast<std::uint64_t>(hash.wire_count)==hash.rows.size(),"BNW specialist wire row count mismatch");
  std::map<std::uint64_t,std::int32_t> counts;
  for(const auto& row:hash.rows){
   need(row.type_hash&&row.type_hash<=UINT32_MAX&&row.values.size()==1,"BNW specialist missing/null raw count cannot assume initialization zero");
   const auto it=slots.find(static_cast<std::uint32_t>(row.type_hash));need(it!=slots.end()&&counts.emplace(it->second,i32(row.values.front())).second,"BNW specialist unresolved/duplicate raw count");
  }
  // Do not synthesize a missing native array cell from a saved sparse hash.
  for(std::uint64_t slot=0;slot<report.slots;++slot)if(static_cast<std::int64_t>(slot)!=default_id){const auto it=counts.find(slot);need(it!=counts.end(),"BNW specialist missing raw count requires native initialization/read owner");q.specialists_=i32(std::int64_t(q.specialists_)+it->second);}
   q.specialist_wire_count_=hash.wire_count;q.raw_specialists_=hash.rows;
  }
  q.reduction_=i32(std::int64_t(q.specialists_)*per_pop);q.reduction_/=2;
  q.total_=i32(std::int64_t(q.total_)-q.reduction_);
 }
 return q;
}
BnwCityFoodConsumptionDiagnostic diagnose_bnw_city_food_consumption(
 const saves::SavedSessionSnapshot& world,const content::RulesDatabase& db,
 const content::GlobalDefineCache& d,std::int64_t owner,std::int64_t id,std::int64_t extra){
 return detail::diagnose_bnw_city_food_consumption_source(world,db,d,owner,id,extra,nullptr);
}
void BnwCityFoodConsumptionDiagnostic::require_same_raw_inputs(const saves::SavedSessionSnapshot& world)const{
 need(thread_==std::this_thread::get_id(),"BNW food consumption foreign thread");
 const auto [p,c]=bind(world,owner_,city_);
 need(player_span_==std::pair{p->core.start_offset,p->core.end_offset}&&city_span_==std::pair{c->core.start_offset,c->core.end_offset}&&population_==city_scalar(c->core,"m_iPopulation")&&half_==player_scalar(p->core,"m_iHalfSpecialistFoodCount"),"BNW food consumption raw identity/population/policy stale");
 if(half_>0){const auto& block=citizens(*c);const auto& hash=named(block.hashes,"m_aiSpecialistCounts");need(citizens_span_==std::pair{block.start_offset,block.end_offset}&&specialist_wire_count_==hash.wire_count&&same_rows(raw_specialists_,hash.rows),"BNW food consumption specialist inputs stale");}
}
}
