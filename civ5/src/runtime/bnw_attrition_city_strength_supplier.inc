// Included outside namespaces after the actual imported garrison supplier.
#include "civ5/runtime/bnw_attrition_city_strength_child.hpp"
#include <set>
namespace civ5::runtime {
struct BnwNativeAttritionCityStrengthRead::Impl {
 const std::thread::id creator=std::this_thread::get_id();
 const ui::SelectedBnwPreviewHost*host=nullptr;std::weak_ptr<const LiveSessionState>world;
 std::array<std::shared_ptr<const BnwNativeImportedGarrisonRead>,3>parents;
 std::uint64_t generation=0,before=0,after=0,changes=0;
 std::int32_t owner=-1,city=-1,unit=-1,damage=0,old=0,base=0,final=0,known=0;
 const saves::SavedCityObservation*receiver=nullptr;
 saves::CityCoreField*field=nullptr;std::int64_t*value=nullptr;
 // Exact CvCity constructor/declaration order, confirmed in evidence. A
 // registered scalar node/delta-key is retained by the SAME private child;
 // the diagnostic dirty bool below grants no authority on its own.
 static constexpr std::size_t native_archive_index=46;
 static constexpr std::string_view binding_name="CvCity::m_iStrengthValue";
 static constexpr std::string_view binding_member="m_iStrengthValue";
 static constexpr bool native_callstack_tracking=true;
 std::set<const saves::CityCoreField*>deltas;
 std::set<const saves::CityCoreField*>::node_type reserved_delta;
 const saves::PlotObservation*plot=nullptr;const saves::NamedSaveInteger*plot_type=nullptr;
 CityYieldState*owned=nullptr;
 Seal inputs;SimulationRandomState rng,map_rng;
 std::vector<BnwCityDirtyEvent>events;std::uint64_t next_event=0;
 std::map<std::type_index,std::any>::node_type mirror;
 bool prepared=false,attempted=false,executed=false,dirty=false;
 bool base_invoked=false,hills_returned=false,hill_invoked=false,cache_stored=false;
 std::uint32_t garrison_queries=0;
};
BnwNativeAttritionCityStrengthRead::BnwNativeAttritionCityStrengthRead(std::shared_ptr<Impl>p):impl_(std::move(p)){}
BnwNativeAttritionCityStrengthRead::~BnwNativeAttritionCityStrengthRead()=default;
std::shared_ptr<BnwNativeAttritionCityStrengthRead>BnwNativeAttritionCityStrengthRead::prepare_same_live(
 const ui::SelectedBnwPreviewHost&host,LiveSessionState&s,
 const std::array<std::shared_ptr<const BnwNativeImportedGarrisonRead>,3>&parents){
 const auto world=host.session().read_state_lease();need(world&&world.get()==&s,
      "Attrition city strength supplier requires genuine SAME Host world");
 for(const auto&p:parents){need(bool(p),"Attrition city strength genuine garrison return absent");p->require_without_callbacks(host,s);}
 const auto facts=parents[2]->facts_without_callbacks();const auto receiver=parents[2]->city_without_callbacks();
 need(receiver&&receiver==parents[1]->city_without_callbacks()&&facts.garrisoned&&facts.city_owner&&facts.city_id&&facts.best_full_id,
      "Attrition city strength requires SAME nonnull native repeated city receiver");
 auto p=std::make_shared<Impl>();p->host=&host;p->world=world;p->parents=parents;
 p->generation=host.session().state_generation();p->before=s.ui_source_revision;
 need(p->before<UINT64_MAX,"Attrition city strength cache revision exhausted");p->after=p->before+1;
 p->owner=*facts.city_owner;p->city=*facts.city_id;p->unit=*facts.best_full_id;p->damage=facts.observed_damage;
 p->rng=s.random.gameplay.state();p->map_rng=s.random.map.state();
 p->events=s.city_dirty_events;p->next_event=s.city_dirty_next_occurrence;
 InputCount count;initial_city_inputs(s,p->owner,p->city,count);p->inputs.reserve(count.size());
 initial_city_inputs(s,p->owner,p->city,p->inputs);need(p->inputs.size()==count.size(),"Attrition city strength source changed during reservation");
 // Actual CvCity::GetGarrisonedUnit: a new current best-defender query, not
 // cached Garrison2 fullID or its old saved damage. Existing strict ordinary
 // LAND comparator domain is unchanged and missing contributors still refuse.
 const auto*raw=resolve_bnw_city_registry(s.world,s.cities,p->owner,p->city);
 need(raw&&raw==receiver,"Attrition city strength resolved receiver differs");p->receiver=raw;
 const auto index=s.world.map_header.grid.index(city_scalar(*raw,"m_iX"),city_scalar(*raw,"m_iY"));
 p->plot=&s.world.map_plots->plots.at(index);p->plot_type=&named(p->plot->fields,"plot_type");
 const std::function<InitialDefenderView(std::int32_t,std::int32_t)>resolve=[&](std::int32_t actor,std::int32_t handle){
  const auto*u=plot_unit(s,actor,handle);return InitialDefenderView{u,nullptr,nullptr,&s,actor,u?unit_scalar(*u,"m_iID"):-1,{}};
 };
 const std::function<std::int32_t(const saves::PlotObservation&)>actual_defender=[&](const saves::PlotObservation&plot){
  need(&plot==p->plot&&p->garrison_queries==0,"Attrition city strength genuine GetGarrisonedUnit plot/once differs");
  const auto defender=initial_selected_defender(s,host,p->owner,plot,resolve);
  need(defender.raw&&unit_scalar(*defender.raw,"m_iID")==p->unit&&unit_scalar(*defender.raw,"m_iDamage")==p->damage,
       "Attrition city strength GetGarrisonedUnit did not observe SAME live after-HP unit");
  const auto hp=integer(static_cast<std::int64_t>(define(host,"MAX_HIT_POINTS",content::GlobalDefineKind::integer)));
  need(hp>0&&p->damage>=0&&p->damage<=hp,"Attrition city strength native HP divisor invalid");
  const auto defense=mul(mul(unit_scalar(*defender.raw,"m_iBaseCombat"),100),hp-p->damage)/hp;
  ++p->garrison_queries;return defense;
 };
 const auto computed=compute_any_living(s,host,p->owner,p->city,&actual_defender);
 need(computed.raw==raw&&computed.id==p->city&&computed.plot==static_cast<std::int64_t>(index)&&p->garrison_queries==1,
      "Attrition city strength actual major/minor receiver/plot changed");
 p->old=computed.before;p->base=computed.base;p->final=computed.after;p->known=computed.known;
 // Alias registration is reserved for Core's SAME original Read lineage check.
 // This supplier cannot be constructed or executed by a public factory/DTO.
 for(auto&city:s.world.all_players->players.at(p->owner).cities.cities)
  if(&city==raw)p->field=&named(city.core.fields,"m_iStrengthValue");
 need(p->field&&p->field->values.size()==1&&p->field->values.front()==p->old,
      "Attrition city strength canonical mutable scalar alias unavailable");p->value=&p->field->values.front();
 std::set<const saves::CityCoreField*>reserved;reserved.insert(p->field);p->reserved_delta=reserved.extract(reserved.begin());
 p->owned=&s.cities.players.at(p->owner).cities.at(p->city);
 need(!p->owned->extensions.values.contains(std::type_index(typeid(BnwMinorCityStrengthScratch::LiveStored))),
      "Attrition city strength needs closed archive history before preexisting cache mirror");
 std::map<std::type_index,std::any>staged;
 staged.emplace(std::type_index(typeid(BnwMinorCityStrengthScratch::LiveStored)),
  std::any(BnwMinorCityStrengthScratch::LiveStored{p->final,p->known,p->generation}));
 p->mirror=staged.extract(staged.begin());
 p->prepared=true;const auto issued=std::shared_ptr<BnwNativeAttritionCityStrengthRead>(new BnwNativeAttritionCityStrengthRead(p));
 issued->require_without_callbacks(host,s,false);return issued;
}
void BnwNativeAttritionCityStrengthRead::require_without_callbacks(const ui::SelectedBnwPreviewHost&host,
 const LiveSessionState&s,bool executed)const{
 const auto p=impl_;need(p&&p->creator==std::this_thread::get_id()&&p->host==&host&&p->prepared&&p->garrison_queries==1&&p->executed==executed&&p->attempted==executed,
      "Attrition city strength private creator/prepared/store phase changed");
 const auto world=p->world.lock(),current=host.session().read_state_lease();
 need(world&&world.get()==&s&&current.get()==world.get()&&!world.owner_before(current)&&!current.owner_before(world)&&
      host.session().state_generation()==p->generation&&s.ui_source_revision==(executed?p->after:p->before)&&
      s.random.gameplay.state()==p->rng&&s.random.map.state()==p->map_rng&&
      s.city_dirty_events==p->events&&s.city_dirty_next_occurrence==p->next_event,
      "Attrition city strength SAME Host world/generation/RNG/revision changed");
 InputMatch match{p->inputs};initial_city_inputs(s,p->owner,p->city,match);
 need(match.equal&&match.position==p->inputs.size()&&resolve_bnw_city_registry(s.world,s.cities,p->owner,p->city)==p->receiver&&
      &named(p->receiver->core.fields,"m_iStrengthValue")==p->field&&p->field->values.size()==1&&&p->field->values.front()==p->value&&
      p->field->encoding==saves::CityCoreEncoding::i32&&!p->field->wire_count&&p->field->bytes.empty()&&
      p->field->hashes.empty()&&p->field->row_keys.empty()&&p->field->row_counts.empty()&&
      *p->value==(executed?p->final:p->old)&&&s.cities.players.at(p->owner).cities.at(p->city)==p->owned&&
      p->plot&&&named(p->plot->fields,"plot_type")==p->plot_type&&
      p->base_invoked==executed&&p->hills_returned==executed&&p->hill_invoked==(executed&&p->plot_type->value==1)&&p->cache_stored==executed,
      "Attrition city strength current operands/physical archive alias/native store sequence changed");
 const auto expected_changes=static_cast<std::uint64_t>(p->base!=p->old)+static_cast<std::uint64_t>(p->plot_type->value==1&&p->final!=p->base);
 need(p->changes==(executed?expected_changes:0)&&p->dirty==(executed&&expected_changes!=0),
      "Attrition city strength native touch delta history changed");
 need(p->binding_name=="CvCity::m_iStrengthValue"&&p->binding_member=="m_iStrengthValue"&&p->native_archive_index==46&&
      p->native_callstack_tracking&&p->field->name&&std::string_view(p->field->name)==p->binding_member&&
      (executed&&expected_changes?(p->deltas.size()==1&&p->deltas.contains(p->field)&&p->reserved_delta.empty()):
       (p->deltas.empty()&&!p->reserved_delta.empty()&&p->reserved_delta.value()==p->field)),
      "Attrition city strength actual registered native archive slot/delta-key phase changed");
 const auto*mirror=p->owned->extensions.find<BnwMinorCityStrengthScratch::LiveStored>();
 need(executed?(mirror&&mirror->strength==p->final&&mirror->known_technologies==p->known&&mirror->generation==p->generation&&p->mirror.empty()):
      (!mirror&&!p->owned->extensions.values.contains(std::type_index(typeid(BnwMinorCityStrengthScratch::LiveStored)))&&!p->mirror.empty()),
      "Attrition city strength SAME actual cache mirror phase changed");
 for(const auto&parent:p->parents){
  if(executed)parent->require_after_city_strength_without_callbacks(host,s,*this);
  else parent->require_without_callbacks(host,s);
 }
}
void BnwNativeAttritionCityStrengthRead::execute_without_callbacks(LiveSessionState&s)const{
 const auto p=impl_;need(p&&p->prepared&&!p->attempted&&!p->executed&&p->host,"Attrition city strength once store replay");
 require_without_callbacks(*p->host,s,false);p->attempted=true;
 // Original FINAL_RELEASE FAutoVariable<int,CvCity>::operator=/set:
 // only changed values touch the registered delta BEFORE actual scalar write.
 // The tracked=true debug callstack is compiled out by original BNW's Release
 // FINAL_RELEASE definition. No caller-selected flag or synthetic stack exists.
 p->base_invoked=true;
 if(p->base!=*p->value){p->deltas.insert(std::move(p->reserved_delta));p->dirty=true;++p->changes;*p->value=p->base;}
 // The real SAME receiver plot()->isHills() is reached AFTER base assignment.
 // All pointer/shape/overflow validation was completed before this pure tail.
 const bool hills=p->plot_type->value==1;p->hills_returned=true;
 if(hills){p->hill_invoked=true;
  if(p->final!=*p->value){if(p->deltas.empty())p->deltas.insert(std::move(p->reserved_delta));
   p->dirty=true;++p->changes;*p->value=p->final;}}
 p->owned->extensions.values.insert(std::move(p->mirror));p->cache_stored=true;
 s.ui_source_revision=p->after;p->executed=true;
 // DLLUI->setDirty(CityInfo18,true) has NOT been invoked or queued as success.
}
const void*BnwNativeAttritionCityStrengthRead::receiver_without_callbacks()const{
 need(impl_&&impl_->creator==std::this_thread::get_id(),"Attrition city strength receiver foreign thread");return impl_->receiver;
}
std::uint64_t BnwNativeAttritionCityStrengthRead::after_revision_without_callbacks()const{
 need(impl_&&impl_->executed&&impl_->creator==std::this_thread::get_id(),"Attrition city strength descendant revision not earned");return impl_->after;
}
BnwAttritionCityStrengthProgress BnwNativeAttritionCityStrengthRead::facts_without_callbacks()const{
 const auto p=impl_;need(p&&p->creator==std::this_thread::get_id(),"Attrition city strength facts foreign thread");
 BnwAttritionCityStrengthProgress o;o.prepared=p->prepared;o.attempted=p->attempted;o.executed=p->executed;
 o.base_assignment_invoked=p->base_invoked;o.hills_query_returned=p->hills_returned;o.hill_assignment_invoked=p->hill_invoked;
 o.archive_dirty=p->dirty;o.archive_changes=p->changes;o.cache_stored=p->cache_stored;
 o.owner=p->owner;o.city=p->city;o.unit=p->unit;o.defender=p->unit;o.observed_damage=p->damage;
 o.before_strength=p->old;o.base_strength=p->base;o.after_strength=p->executed?p->final:p->old;
 o.before_revision=p->before;o.after_revision=p->executed?p->after:p->before;o.get_garrisoned_unit_returned=p->garrison_queries;return o;
}
void BnwNativeImportedGarrisonRead::require_after_city_strength_without_callbacks(const ui::SelectedBnwPreviewHost&host,
 const LiveSessionState&s,const BnwNativeAttritionCityStrengthRead&successor)const{
 const auto p=impl_;const auto child=successor.impl_;
 need(p&&child&&p->creator==std::this_thread::get_id()&&child->creator==p->creator&&p->host==&host&&child->host==&host&&
      child->executed&&child->prepared&&child->attempted&&child->before==p->revision&&
      child->generation==p->generation&&child->receiver==p->reached.city&&child->damage==p->reached.facts.observed_damage&&
      std::any_of(child->parents.begin(),child->parents.end(),[&](const auto&parent){return parent.get()==this;}),
      "Attrition garrison after-city requires exact privately issued executed child");
 const auto world=p->world.lock(),current=host.session().read_state_lease(),child_world=child->world.lock();
 need(world&&world.get()==&s&&current.get()==world.get()&&child_world.get()==world.get()&&!world.owner_before(current)&&!current.owner_before(world)&&
      !child->world.owner_before(p->world)&&!p->world.owner_before(child->world)&&s.ui_source_revision==child->after&&
      host.session().state_generation()==p->generation&&s.random.gameplay.state()==p->rng&&s.random.map.state()==p->map_rng,
      "Attrition garrison after-city SAME allocations/generation/RNG/revision differ");
 InputMatch match{p->inputs};const auto reached=attrition_garrison_semantic(s,p->actor,p->id,match);
 need(match.equal&&match.position==p->inputs.size()&&reached.city==p->reached.city&&reached.plot==p->reached.plot&&reached.best_required==p->reached.best_required,
      "Attrition garrison original immutable current comparator after-city changed");
}
}
