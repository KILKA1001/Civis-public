// Inside runtime after genuine Garrison2; no caller value/factory authority.
void BnwNativeAttritionPrefix::prepare_city_strength_owned(const BnwNativeUnitAllocatorCore&core,
 const ui::SelectedBnwPreviewHost&host,const LiveSessionState&s,const BnwSelectedStockLuaSourceOwner&source,
 const void*control,std::uint64_t generation,std::uint64_t serial)const{
 const auto r=impl_;r->thread();need(r->garrison&&r->damage,"Attrition city strength actual parent absent");
 require_garrison_without_callbacks(core,host,s,source,control,generation,r->garrison->serial,true);
 need(r->garrison->boundary==BnwAttritionGarrisonBoundary::city_strength_owner&&r->garrison->returned==3&&
      !r->city_strength&&r->garrison->serial<UINT64_MAX&&serial==r->garrison->serial+1,
      "Attrition city strength requires true SAME receiver/NEXT Source operation");
 r->city_strength=std::make_unique<AttritionCityStrengthChild>();auto&q=*r->city_strength;
 q.previous=r->garrison->serial;q.serial=serial;q.before=s.ui_source_revision;
 const auto p=core.impl_;const auto borrower=BnwNativeUnitAllocatorCore::ShouldDimCoreBorrowedOwner::from_weak(core.factory_self_);
 try{BnwNativeUnitAllocatorCore::Impl::Guard invocation(p);p->check();(void)borrower->current();
  require_garrison_without_callbacks(core,host,s,source,control,generation,q.previous,true);
  require_imported_mission_payload_lineage(*p->source,host.identity());
  need(!p->source->source.live_bnw_city_core,
       "Attrition city archive requires closed dirty/delta history before city-core overlay restore");
  const auto receiver=r->garrison->reads[2]->city_without_callbacks();const auto facts=r->garrison->reads[2]->facts_without_callbacks();
  need(receiver&&facts.city_owner&&facts.city_id&&*facts.city_owner==r->actor&&
       p->source->snapshot.all_players&&p->source->snapshot.all_players->layout==bnw,
       "Attrition city strength original SAME actor/read lineage absent");
  const auto&original=p->source->snapshot.all_players->players.at(r->actor);
  need(original.saved_player_id==r->actor,"Attrition city strength original player identity changed");
  const saves::SavedCityObservation*loaded=nullptr;
  for(const auto&city:original.cities.cities)if(narrow(garrison_named(city.core.fields,"m_iID").values.at(0))==*facts.city_id){
   need(!loaded,"Attrition city archive duplicate original fullID");loaded=&city;}
  const auto*raw=static_cast<const saves::SavedCityObservation*>(receiver);
  need(loaded&&loaded->index==raw->index&&loaded->core.layout==bnw&&raw->core.layout==bnw&&
       loaded->core.start_offset==raw->core.start_offset&&loaded->core.end_offset==raw->core.end_offset&&
       garrison_named(loaded->core.fields,"m_eOwner").values==garrison_named(raw->core.fields,"m_eOwner").values&&
       garrison_named(raw->core.fields,"m_eOwner").values==std::vector<std::int64_t>{r->actor},
       "Attrition city archive original constructor/Read object spans/owner changed");
  const auto&before=garrison_named(loaded->core.fields,"m_iStrengthValue");const auto&current=garrison_named(raw->core.fields,"m_iStrengthValue");
  need(before.encoding==saves::CityCoreEncoding::i32&&current.encoding==before.encoding&&
       !before.wire_count&&!current.wire_count&&before.values.size()==1&&before.values==current.values&&
       before.bytes.empty()&&current.bytes.empty()&&before.hashes.empty()&&current.hashes.empty()&&
       before.row_keys.empty()&&current.row_keys.empty()&&before.row_counts.empty()&&current.row_counts.empty()&&
       before.start_offset==current.start_offset&&before.end_offset==current.end_offset,
       "Attrition city archive cannot mint clean Read after unregistered strength stores");
  // Closed registration creates exactly CvCity::m_iStrengthValue scalar delta
  // lineage from this original Read. It grants no other city/archive field.
  q.owner=BnwNativeAttritionCityStrengthRead::prepare_same_live(host,*p->state,r->garrison->reads);
  need(q.owner&&q.owner->receiver_without_callbacks()==receiver,
       "Attrition city strength closed producer changed receiver");
  const auto actual=q.owner->facts_without_callbacks();
  need(actual.defender==std::optional{r->damage->id}&&actual.observed_damage==std::optional{r->damage->new_damage}&&
       actual.before_revision==q.before&&actual.after_revision==q.before,
       "Attrition city strength current best defender after-HP join differs");
  need(q.before<UINT64_MAX,"Attrition city strength revision exhausted");q.after=q.before+1;
  p->check();(void)borrower->current();q.prepared=true;
  require_city_strength_without_callbacks(core,host,s,source,control,generation,serial,false);
 }catch(...){q.failed=true;throw;}
}
std::uint64_t BnwNativeAttritionPrefix::damage_descendant_revision_without_callbacks(
 const ui::SelectedBnwPreviewHost&host,const LiveSessionState&s)const{
 const auto r=impl_;r->thread();need(bool(r->damage),"Attrition damage descendant parent absent");
 if(r->city_strength&&r->city_strength->executed){const auto&q=*r->city_strength;
  need(!q.failed&&q.prepared&&q.attempted&&q.owner&&r->garrison&&r->garrison->executed&&!r->garrison->failed&&
       q.previous==r->garrison->serial&&q.previous<UINT64_MAX&&q.serial==q.previous+1&&q.before==r->damage->after&&
       q.before<UINT64_MAX&&q.after==q.before+1&&q.owner->receiver_without_callbacks()==r->garrison->reads[2]->city_without_callbacks(),
       "Attrition city descendant requires SAME genuine current executed node");
  q.owner->require_without_callbacks(host,s,true);
  need(q.owner->after_revision_without_callbacks()==q.after,"Attrition city private revision receipt changed");return q.after;
 }
 return r->damage->after;
}
void BnwNativeAttritionPrefix::require_city_strength_without_callbacks(const BnwNativeUnitAllocatorCore&core,
 const ui::SelectedBnwPreviewHost&host,const LiveSessionState&s,const BnwSelectedStockLuaSourceOwner&source,
 const void*control,std::uint64_t generation,std::uint64_t serial,bool executed)const{
 const auto r=impl_;r->thread();need(r->city_strength&&r->garrison&&r->damage,"Attrition city actual partial history absent");
 const auto&q=*r->city_strength;
 need(q.prepared&&!q.failed&&q.executed==executed&&q.attempted==executed&&q.serial==serial&&
      q.previous==r->garrison->serial&&q.previous<UINT64_MAX&&q.serial==q.previous+1&&q.before==r->damage->after&&
      q.before<UINT64_MAX&&q.after==q.before+1&&s.ui_source_revision==(executed?q.after:q.before)&&q.owner&&
      r->garrison->boundary==BnwAttritionGarrisonBoundary::city_strength_owner&&r->garrison->returned==3,
      "Attrition city SAME garrison/serial/prepared/store/revision phase differs");
 q.owner->require_without_callbacks(host,s,executed);
 // OLD metadata and original witnesses remain immutable. Only the private
 // executed child above supplies its earned revision to physical parent guards.
 require_garrison_without_callbacks(core,host,s,source,control,generation,q.previous,true);
 const auto actual=q.owner->facts_without_callbacks();need(actual.unit==std::optional{r->damage->id}&&
      actual.observed_damage==std::optional{r->damage->new_damage}&&!actual.city_info_dirty_returned&&!actual.update_strength_returned,
      "Attrition city current native unit/next mandatory physical dirty boundary changed");
}
void BnwNativeAttritionPrefix::execute_city_strength_without_callbacks(LiveSessionState&s)const{
 const auto r=impl_;r->thread();need(r->city_strength&&!r->city_strength->failed&&!r->city_strength->attempted&&!r->city_strength->executed,
      "Attrition city actual once strength reentry");
 const auto core=r->core.lock();const auto source=r->source.lock();const auto host=r->host.lock();const auto scope=r->control.lock();
 need(core&&source&&host&&scope,"Attrition city actual owners expired");
 require_city_strength_without_callbacks(*core,*host,s,*source,scope.get(),r->generation,r->city_strength->serial,false);
 auto&q=*r->city_strength;q.attempted=true;q.owner->execute_without_callbacks(s);q.executed=true;
 // No CityInfo18 setter return or outer updateStrengthValue/setDamage return.
}
BnwAttritionCityStrengthProgress BnwNativeAttritionPrefix::city_strength_progress_without_callbacks()const{
 const auto r=impl_;r->thread();BnwAttritionCityStrengthProgress o;if(!r->city_strength)return o;const auto&q=*r->city_strength;
 if(q.owner)o=q.owner->facts_without_callbacks();o.issued=true;o.prepared=q.prepared;o.attempted=q.attempted;o.executed=q.executed;o.failed=q.failed;
 o.previous_source_serial=q.previous;o.source_serial=q.serial;o.before_revision=q.before;o.after_revision=q.executed?q.after:q.before;return o;
}
void BnwNativeAttritionPrefix::fail_city_strength_without_callbacks()const noexcept{
 const auto r=impl_;if(r->creator!=std::this_thread::get_id())std::terminate();if(r->city_strength)r->city_strength->failed=true;
}
