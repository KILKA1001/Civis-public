#include "civ5/runtime/bnw_city_food_base_yield.hpp"
#include "civ5/runtime/bnw_city_strategy_yields.hpp"
#include <climits>
#include <stdexcept>
#include <string_view>
#include <thread>
namespace civ5::runtime {
namespace {
void need(bool b,const char* m){if(!b)throw std::runtime_error(m);}
std::int32_t i32(std::int64_t n){need(n>=INT32_MIN&&n<=INT32_MAX,"Food base yield native int32 overflow");return static_cast<std::int32_t>(n);}
const saves::CityCoreField& field(const saves::SavedCityObservation& c,std::string_view name){
 const saves::CityCoreField* out=nullptr;for(const auto& f:c.core.fields)if(f.name&&std::string_view(f.name)==name){need(!out,"Food base yield duplicate source field");out=&f;}need(out,"Food base yield source field absent");return *out;
}
struct Inputs {
 std::int64_t owner=-1;std::int32_t city=-1;
 std::pair<std::uint64_t,std::uint64_t> span;
 std::array<std::int32_t,5> values{};
 std::int32_t result=0;
 bool operator==(const Inputs&)const=default;
};
Inputs read(const LiveSessionState& s,std::int64_t owner,std::int64_t id){
 const auto* c=resolve_bnw_current_city(s,i32(owner),i32(id));need(c,"Food base yield current city absent");
 need(s.cities.yield_slots==6,"Food base yield requires native BNW six-yield span");
 const auto& raw_id=field(*c,"m_iID");need(raw_id.encoding==saves::CityCoreEncoding::i32&&raw_id.values.size()==1,"Food base yield source city ID shape unavailable");
 Inputs out;out.owner=owner;out.city=i32(raw_id.values.front());out.span={c->core.start_offset,c->core.end_offset};
 const auto& owned=s.cities.players.at(owner).cities.at(out.city);
 const auto found=owned.yields.find(0);const auto values=found==owned.yields.end()?CityYieldComponents{}:found->second;
 constexpr const char* names[]={"m_aiBaseYieldRateFromTerrain","m_aiBaseYieldRateFromBuildings","m_aiBaseYieldRateFromSpecialists","m_aiBaseYieldRateFromMisc","m_aiBaseYieldRateFromReligion"};
 const std::int64_t mirrors[]={values.terrain,values.buildings,values.specialists,values.misc,values.religion};
 for(std::size_t n=0;n<5;++n){
  const auto& f=field(*c,names[n]);
  need(f.encoding==saves::CityCoreEncoding::vector_i32&&f.wire_count==6&&f.values.size()==6&&f.bytes.empty()&&f.hashes.empty()&&f.row_counts.empty()&&f.row_keys.empty(),"Food base yield source six-vector shape unavailable");
  out.values[n]=i32(f.values[0]);need(out.values[n]==i32(mirrors[n]),"Food base yield raw/owned component stale");
  // Source starts at zero, then adds each getter in this order. An overflowing
  // intermediate refuses even if a later component would cancel it to fit.
  out.result=i32(std::int64_t(out.result)+out.values[n]);
 }
 return out;
}
}
struct BnwCityFoodBaseYield::Impl {const std::thread::id thread=std::this_thread::get_id();Inputs inputs;};
BnwCityFoodBaseYield::BnwCityFoodBaseYield(std::shared_ptr<const Impl> p)noexcept:impl_(std::move(p)){}
void BnwCityFoodBaseYield::check_thread()const{need(impl_&&impl_->thread==std::this_thread::get_id(),"Food base yield foreign thread/moved result");}
std::int32_t BnwCityFoodBaseYield::value()const{check_thread();return impl_->inputs.result;}
std::array<std::int32_t,5> BnwCityFoodBaseYield::components()const{check_thread();return impl_->inputs.values;}
void BnwCityFoodBaseYield::require_same_inputs(const LiveSessionState& s)const{check_thread();need(read(s,impl_->inputs.owner,impl_->inputs.city)==impl_->inputs,"Food base yield retained inputs stale");}
BnwCityFoodBaseYield inspect_bnw_city_food_base_yield(const LiveSessionState& s,std::int64_t owner,std::int64_t city){auto p=std::make_shared<BnwCityFoodBaseYield::Impl>();p->inputs=read(s,owner,city);return BnwCityFoodBaseYield(std::move(p));}
}
