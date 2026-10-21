#include "civ5/runtime/bnw_city_current_food_consumption.hpp"
#include "civ5/runtime/bnw_city_food_consumption.hpp"
#include "civ5/runtime/bnw_city_strategy_yields.hpp"
#include "civ5/runtime/bnw_city_specialist_counts.hpp"
#include <climits>
#include <thread>
namespace civ5::runtime {
namespace {
void need(bool b,const char* m){if(!b)throw BnwCityCurrentFoodConsumptionUnavailable(m);}
std::int32_t i32(std::int64_t n){need(n>=INT32_MIN&&n<=INT32_MAX,"Current food consumption native width unavailable");return static_cast<std::int32_t>(n);}
const saves::CityCoreField& field(const saves::SavedCityObservation& c,std::string_view name){
 const saves::CityCoreField* out=nullptr;for(const auto& f:c.core.fields)if(f.name&&std::string_view(f.name)==name){need(!out,"Current food consumption duplicate city field");out=&f;}need(out,"Current food consumption city field absent");return *out;
}
struct City {std::int64_t owner=-1;std::int32_t id=-1,population=0;bool operator==(const City&)const=default;};
City bind(const LiveSessionState& s,std::int64_t owner,std::int64_t id){
 const auto* raw=resolve_bnw_current_city(s,i32(owner),i32(id));need(raw,"Current food consumption city absent");
 const auto& f=field(*raw,"m_iID");need(f.encoding==saves::CityCoreEncoding::i32&&f.values.size()==1,"Current food consumption city ID shape unavailable");
 City out{owner,i32(f.values.front()),0};const auto& pop=field(*raw,"m_iPopulation");need(pop.values.size()==1,"Current food consumption population shape unavailable");out.population=i32(pop.values.front());
 need(out.population==i32(s.cities.players.at(owner).cities.at(out.id).population),"Current food consumption raw/owned population stale");return out;
}
template<class F>auto typed(F&& f)->decltype(f()){
 try{return f();}catch(const BnwCityCurrentFoodConsumptionUnavailable&){throw;}catch(const std::exception& e){throw BnwCityCurrentFoodConsumptionUnavailable(e.what());}
}
}
struct BnwCityCurrentFoodConsumption::Impl {
 const std::thread::id thread=std::this_thread::get_id();City city;std::string sha;std::function<bool()> current;
 BnwCityFoodConsumptionDiagnostic source;
 std::optional<BnwCityCurrentFoodRate> parent;std::optional<BnwCitySpecialistCounts> specialists;
 Impl(City c,std::string identity,std::function<bool()> guard,BnwCityFoodConsumptionDiagnostic&& q)
  :city(c),sha(std::move(identity)),current(std::move(guard)),source(std::move(q)){}
};
BnwCityCurrentFoodConsumption::BnwCityCurrentFoodConsumption(std::shared_ptr<const Impl> p)noexcept:impl_(std::move(p)){}
void BnwCityCurrentFoodConsumption::check_current()const{
 need(impl_&&impl_->thread==std::this_thread::get_id(),"Current food consumption foreign thread/moved result");
 if(impl_->parent){impl_->parent->require_current();need(impl_->specialists.has_value(),"Current food consumption closed source leaf absent");(void)impl_->specialists->reached_source_seal();impl_->source.require_same_raw_inputs(impl_->parent->current_source_state().world);}
 need(impl_->current&&impl_->current(),"Current food consumption SAME immutable rules stale");
}
std::int32_t BnwCityCurrentFoodConsumption::value()const{check_current();return impl_->source.consumption();}
std::int32_t BnwCityCurrentFoodConsumption::population_with_extra()const{check_current();return impl_->source.population_with_extra();}
std::int32_t BnwCityCurrentFoodConsumption::specialist_total()const{check_current();return impl_->source.specialist_total();}
std::int32_t BnwCityCurrentFoodConsumption::reduction()const{check_current();return impl_->source.reduction();}
bool BnwCityCurrentFoodConsumption::reads_specialists()const{check_current();return impl_->source.reads_specialists();}
void BnwCityCurrentFoodConsumption::require_same_inputs(const LiveSessionState& s,std::string_view sha)const{
 typed([&]{check_current();need(impl_->sha==sha,"Current food consumption rules identity mismatch");if(impl_->parent){need(&s==&impl_->parent->current_source_state(),"Current food consumption requires SAME actual state allocation");(void)impl_->specialists->reached_source_seal();}need(bind(s,impl_->city.owner,impl_->city.id)==impl_->city,"Current food consumption retained population stale");impl_->source.require_same_raw_inputs(s.world);});
}
BnwCityCurrentFoodConsumption BnwCityCurrentFoodConsumption::make(
 const LiveSessionState& s,const content::RulesDatabase& db,std::string_view sha,std::int64_t owner,std::int64_t city,std::int64_t extra,const BnwCityCurrentFoodRate* parent){
 return typed([&]{
  std::optional<BnwCitySpecialistCounts> specialists;
  if(parent){parent->require_current();need(&s==&parent->current_source_state()&&parent->current_source_database_owner().get()==&db&&parent->current_rules_sha()==sha&&parent->canonical_owner()==owner&&parent->canonical_city()==city,"Current food consumption closed parent allocation/identity mismatch");specialists=BnwCitySpecialistCounts::from_same_rate(*parent);}
  auto current=db.immutable_preview_fingerprint_currentness();need(current&&current(),"Current food consumption requires SAME immutable activated rules");
  auto identity=db.gameplay_fingerprint_sha256();need(identity==sha,"Current food consumption rules identity mismatch");
  const auto bound=bind(s,owner,city);
  // Defines come from this exact database; callers cannot mix in another cache.
  const auto defines=db.cache_global_defines(content::GlobalDefineProfile::bnw);
  auto source=detail::diagnose_bnw_city_food_consumption_source(s.world,db,defines,bound.owner,bound.id,extra,specialists?&*specialists:nullptr);
  need(current()&&db.gameplay_fingerprint_sha256()==identity,"Current food consumption rules retired during calculation");
  need(bind(s,owner,city)==bound,"Current food consumption city changed during calculation");source.require_same_raw_inputs(s.world);
  auto out=std::make_shared<BnwCityCurrentFoodConsumption::Impl>(bound,std::move(identity),std::move(current),std::move(source));
  if(parent){parent->require_current();out->parent=*parent;out->specialists=std::move(specialists);}return BnwCityCurrentFoodConsumption(std::move(out));
 });
}
BnwCityCurrentFoodConsumption BnwCityCurrentFoodConsumption::from_same_rate(const BnwCityCurrentFoodRate& parent,std::int64_t owner,std::int64_t city,std::int64_t extra){
 return typed([&]{parent.require_current();const auto database=parent.current_source_database_owner();return make(parent.current_source_state(),*database,parent.current_rules_sha(),owner,city,extra,&parent);});
}
BnwCityCurrentFoodConsumption inspect_bnw_city_current_food_consumption(
 const LiveSessionState& s,const content::RulesDatabase& db,std::string_view sha,std::int64_t owner,std::int64_t city,std::int64_t extra){
 return BnwCityCurrentFoodConsumption::make(s,db,sha,owner,city,extra,nullptr);
}
}
