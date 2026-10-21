#include "civ5/runtime/bnw_city_strategy_runtime.hpp"
#include "civ5/runtime/live_session.hpp"
#include "civ5/ui/selected_bnw_preview_host.hpp"
#include <climits>
#include <functional>
#include <stdexcept>
#include <string_view>
#include <thread>
#include <type_traits>
namespace civ5::runtime {
namespace {
void need(bool b,const char* why){if(!b)throw std::invalid_argument(why);}
std::int32_t native(std::int64_t n){need(n>=INT32_MIN&&n<=INT32_MAX,"City strategy Read native32 overflow");return static_cast<std::int32_t>(n);}
const auto& named(const auto& rows,std::string_view name){
 using T=std::remove_reference_t<decltype(rows.front())>;const T* found=nullptr;
 for(const auto& row:rows)if(row.name&&name==row.name){need(!found,"City strategy Read duplicate named field");found=&row;}
 need(found,"City strategy Read missing named field");return *found;
}
void version(const saves::CityManagementBlock& b){need(b.fields.size()==1&&named(b.fields,"version").value==1,"City strategy production Read unsupported version/schema");}
std::optional<std::size_t> slot(const saves::TeamHashedValues& row,
 const content::InfoTypeCatalog& types,std::string_view table,std::size_t count,bool null_payload){
 need(row.type_hash<=UINT32_MAX,"City strategy Read hash width differs");
 if(!row.type_hash){need(row.values.size()==(null_payload?1u:0u),"City strategy Read null hash payload differs");if(null_payload)(void)native(row.values.front());return {};}
 need(row.values.size()==1,"City strategy Read scalar payload differs");(void)native(row.values.front());
 const auto* t=types.find_hash(row.type_hash);if(!t)return {}; // ReadHashed consumes unknown payload; no assignment.
 need(t->table==table&&t->id>=0&&static_cast<std::uint64_t>(t->id)<count,"City strategy Read known hash outside exact native table/slot");
 return static_cast<std::size_t>(t->id);
}
void hash_shape(const saves::TeamHashedArray& a,bool signed_count){
 need(a.wire_count>=0&&static_cast<std::uint64_t>(a.wire_count)<=(signed_count?std::uint64_t(INT32_MAX):std::uint64_t(UINT32_MAX))&&
  a.rows.size()==static_cast<std::size_t>(a.wire_count),"City strategy Read hashed array count differs");
}
std::uint32_t span(const ui::SelectedBnwPreviewHost& host,const char* table,bool native_holes=false){
 const auto report=host.database().prefetch_info(table,[&](auto index,auto authored,const auto& row){
  need(index<=INT32_MAX&&authored==static_cast<std::int64_t>(index),"City strategy owned Info native ID/order differs");
  const auto type=row.text("Type");need(type&&!type->empty(),"City strategy owned Info Type absent");
  const auto* t=host.types().find(*type);const auto* hash=host.types().find_hash(content::saved_info_type_hash(*type));
  need(t&&hash&&t->table==table&&hash->table==table&&t->id==authored&&hash->id==authored&&hash->type==*type,
   "City strategy owned SAME native Type/hash differs");return true;
 });
 need(report.all_succeeded()&&(native_holes||!report.holes)&&!report.overlapping_ids&&report.slots<=INT32_MAX&&report.rows+report.holes==report.slots,
  "City strategy owned Init/Read requires exact bounded actual Info span");
 return static_cast<std::uint32_t>(report.slots);
}
}
BnwCityStrategyRuntimeValues detail::decode_bnw_city_strategy_runtime_read(
 const saves::CityManagementObservation& m,const content::InfoTypeCatalog& types,
 const BnwCityStrategyRuntimeReadShape& shape){
 need(m.layout==saves::GameStateLayout::expansion2_version1&&shape.strategies<=INT32_MAX&&shape.flavors>0&&shape.flavors<=INT32_MAX,
  "City strategy owned Read BNW/native dimensions unavailable");
 BnwCityStrategyRuntimeValues out;
 // These zeros/-1 are actually executed Init/Reset operations in the own
 // implementation; absent saved rows alone are never interpreted as values.
 out.using_strategy.assign(shape.strategies,0);out.adopted.assign(shape.strategies,-1);
 out.latest.assign(shape.flavors,0);
 const auto& strategy=named(m.blocks,"strategy");
 need(strategy.fields.size()==4&&named(strategy.fields,"version").value==1&&strategy.arrays.size()==1&&strategy.hashes.size()==2,
  "City strategy owned Read strategy schema differs");
 (void)native(named(strategy.fields,"m_eSpecialization").value);(void)native(named(strategy.fields,"m_eDefaultSpecialization").value);
 const auto& latest=named(strategy.arrays,"m_piLatestFlavorValues");const auto count=native(named(strategy.fields,"flavor_wire_count").value);
 need(count>=0&&static_cast<std::uint64_t>(count)<=shape.flavors&&latest.columns==1&&latest.values.size()==static_cast<std::size_t>(count),
  "City strategy owned Read flavor prefix exceeds actual Init allocation");
 for(std::size_t i=0;i<latest.values.size();++i)out.latest[i]=native(latest.values[i]);
 const auto& using_rows=named(strategy.hashes,"m_pabUsingCityStrategy");hash_shape(using_rows,false);
 for(const auto& r:using_rows.rows){const auto i=slot(r,types,"AICityStrategies",shape.strategies,false);if(r.type_hash)need(r.values.front()==0||r.values.front()==1,"City strategy owned Read invalid native bool");if(i)out.using_strategy[*i]=static_cast<std::uint8_t>(r.values.front());}
 const auto& adopted=named(strategy.hashes,"m_paiTurnCityStrategyAdopted");hash_shape(adopted,false);
 for(const auto& r:adopted.rows)if(const auto i=slot(r,types,"AICityStrategies",shape.strategies,false))out.adopted[*i]=native(r.values.front());
 constexpr const char* blocks[]{"building_ai","unit_ai","project_ai","process_ai"};
 constexpr const char* names[]{"m_BuildingAIWeights","m_UnitAIWeights","m_ProjectAIWeights","m_ProcessAIWeights"};
 constexpr const char* tables[]{"Buildings","Units","Projects","Processes"};
 for(std::size_t k=0;k<4;++k){
  need(shape.production[k]<=INT32_MAX,"City strategy owned production Init dimension overflow");
  out.weights[k].assign(shape.production[k],0);const auto& b=named(m.blocks,blocks[k]);version(b);
  if(k==2){need(b.hashes.empty()&&b.arrays.size()==1&&m.context.bnw_projects==std::optional<std::uint64_t>(shape.production[k]),"City strategy owned Project Read fixed native count differs");
   const auto& a=named(b.arrays,names[k]);need(a.columns==1&&a.values.size()==shape.production[k],"City strategy owned Project Read vector differs");
   for(std::size_t i=0;i<a.values.size();++i)out.weights[k][i]=native(a.values[i]);
  }else{
   need(b.arrays.empty()&&b.hashes.size()==1,"City strategy owned hashed production Read schema differs");const auto& a=named(b.hashes,names[k]);hash_shape(a,true);
   for(const auto& r:a.rows)if(const auto i=slot(r,types,tables[k],shape.production[k],k==3)){
    const auto value=native(r.values.front());
    if(k==3)out.weights[k][*i]=value; // Process Read SetWeight: last wins.
    else out.weights[k][*i]=native(std::int64_t(out.weights[k][*i])+value); // Building/unit IncreaseWeight.
   }
  }
 }
 return out;
}
struct BnwCityStrategyRuntime::Impl {
 const std::thread::id thread=std::this_thread::get_id();const LiveSessionState* allocation=nullptr;
 std::int32_t owner=-1,city=-1;std::uint64_t begin=0,end=0,revision=0;
 std::function<bool()> rules;std::string rules_sha;BnwCityStrategyRuntimeValues values;
};
BnwCityStrategyRuntime::BnwCityStrategyRuntime(std::shared_ptr<const Impl> p)noexcept:impl_(std::move(p)){}
BnwCityStrategyRuntime::~BnwCityStrategyRuntime()=default;
std::shared_ptr<const BnwCityStrategyRuntime> BnwCityStrategyRuntime::read_current(
 const ui::SelectedBnwPreviewHost& host,const LiveSessionState& allocation,
 const saves::CityManagementObservation& management,std::int32_t owner,std::int32_t city,
 std::uint64_t begin,std::uint64_t end,std::uint64_t revision){
 // Private method only actual registered Pending can invoke. No public shape,
 // source copy, caller SHA or bool creates this actual owned runtime node.
 (void)host.session().bnw_game_debug_mode();
 const auto guard=host.database().immutable_preview_fingerprint_currentness();need(guard&&guard(),"City strategy owned Read immutable rules retired");
 need(host.session().read_state_lease().get()==&allocation&&host.database().gameplay_fingerprint_sha256()==host.identity().rules_sha256,
  "City strategy owned Read SAME current allocation/rules identity differs");
 auto p=std::make_shared<Impl>();p->allocation=&allocation;p->owner=owner;p->city=city;p->begin=begin;p->end=end;p->revision=revision;p->rules=guard;p->rules_sha=host.identity().rules_sha256;
 detail::BnwCityStrategyRuntimeReadShape shape;shape.strategies=span(host,"AICityStrategies");shape.flavors=span(host,"Flavors");
 constexpr const char* tables[]{"Buildings","Units","Projects","Processes"};for(std::size_t k=0;k<4;++k)shape.production[k]=span(host,tables[k],true);
 p->values=detail::decode_bnw_city_strategy_runtime_read(management,host.types(),shape);
 need(guard(),"City strategy owned Read rules retired during initialization");
 return std::shared_ptr<const BnwCityStrategyRuntime>(new BnwCityStrategyRuntime(std::move(p)));
}
void BnwCityStrategyRuntime::require_binding(const LiveSessionState& state,std::int32_t owner,std::int32_t city,
 std::uint64_t begin,std::uint64_t end,std::uint64_t revision)const{
 need(impl_&&impl_->thread==std::this_thread::get_id(),"City strategy owned node foreign constructor thread");
 need(impl_->rules&&impl_->rules()&&impl_->allocation==&state&&impl_->owner==owner&&impl_->city==city&&
  impl_->begin==begin&&impl_->end==end&&impl_->revision==revision,"City strategy owned node retired allocation/revision/identity");
}
BnwCityStrategyRuntimeValues BnwCityStrategyRuntime::values()const{return impl_->values;}
std::uint64_t BnwCityStrategyRuntime::revision()const{return impl_->revision;}
}
