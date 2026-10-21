// Included after first UnitInfo1 in the existing Core TU (runtime namespace).
struct BnwAttritionUnitSelectedRead::Impl {
 const std::thread::id creator=std::this_thread::get_id();
 std::weak_ptr<const BnwNativeAttritionPrefix>prefix;std::weak_ptr<BnwRetainedTurnCursor>cursor;
 std::weak_ptr<ui::BnwClientSelectionControl>selection;std::weak_ptr<ui::BnwEngineUiTurnOwner>engine;
 std::uint64_t previous=0,serial=0,revision=0,generation=0;
 std::int32_t owner=-1,unit=-1;bool entered=false,returned=false,result=false,failed=false;
};
BnwAttritionUnitSelectedRead::BnwAttritionUnitSelectedRead(std::unique_ptr<Impl>p):impl_(std::move(p)){}
BnwAttritionUnitSelectedRead::~BnwAttritionUnitSelectedRead()=default;
void BnwAttritionUnitSelectedRead::require_alive_without_callbacks()const{
 need(impl_&&impl_->creator==std::this_thread::get_id(),"IsSelected immutable foreign creator");
 const auto cursor=impl_->cursor.lock();need(bool(cursor),"IsSelected original Cursor expired");
 cursor->require_attrition_city_info_facade_without_callbacks();need(!impl_->failed,"IsSelected sticky failed child");
}
void BnwAttritionUnitSelectedRead::fail_without_callbacks()const noexcept{
 if(!impl_||impl_->creator!=std::this_thread::get_id())std::terminate();impl_->failed=true;
 const auto prefix=impl_->prefix.lock();if(prefix)prefix->impl_->failed=true;
}
void BnwNativeAttritionPrefix::require_unit_selected_parent_without_callbacks(const BnwNativeUnitAllocatorCore&core,
 const ui::SelectedBnwPreviewHost&host,const LiveSessionState&state,const BnwSelectedStockLuaSourceOwner&source,
 const void*control,std::uint64_t generation,std::uint64_t previous)const{
 const auto r=impl_;r->thread();need(r->unit_info&&r->damage,"IsSelected requires the real first UnitInfo child");
 r->unit_info->require_alive_without_callbacks();const auto&old=*r->unit_info->impl_;
 const auto issuer=old.prefix.lock();const auto self=r->self.lock();
 need(!old.failed&&old.entered&&old.returned&&old.serial==previous&&old.previous<UINT64_MAX&&old.serial==old.previous+1&&
  old.generation==generation&&old.revision==state.ui_source_revision&&issuer&&self&&issuer.get()==this&&self.get()==this&&
  !old.prefix.owner_before(r->self)&&!r->self.owner_before(old.prefix)&&old.owner==r->actor&&old.unit==r->damage->id&&
  old.from_city==bool(r->city_info)&&!old.engine.owner_before(r->damage->engine)&&!r->damage->engine.owner_before(old.engine),
  "IsSelected OLD returned UnitInfo node/once/fullID/localized Engine lineage differs");
 // Preserve its OLD serial validator; authenticate the physical parent nodes
 // callbacklessly, then admit only this new child at its new Source operation.
 require_unit_info_parent_without_callbacks(core,host,state,source,control,generation,old.previous);
 const auto input=old.selection.lock();need(input&&input->attrition_unit_info_state_matches_without_callbacks(old.input,old.event,true),
  "IsSelected requires exact earned first UnitInfo state, not dirty/event/frame ACK");
}
void BnwAttritionUnitSelectedRead::require_common_without_callbacks()const{
 require_alive_without_callbacks();const auto&q=*impl_;const auto prefix=q.prefix.lock();const auto cursor=q.cursor.lock();
 need(prefix&&cursor,"IsSelected actual Prefix/Cursor expired");const auto r=prefix->impl_;r->thread();
 const auto core=r->core.lock();const auto source=r->source.lock();const auto host=r->host.lock();const auto world=r->world.lock();const auto control=r->control.lock();
 need(core&&source&&host&&world&&control&&r->unit_selected.get()==this&&q.previous<UINT64_MAX&&q.serial==q.previous+1&&
  q.generation==r->generation&&q.revision==world->ui_source_revision&&r->damage&&q.owner==r->actor&&q.unit==r->damage->id&&
  !q.engine.owner_before(r->damage->engine)&&!r->damage->engine.owner_before(q.engine),"IsSelected genuine current Core/world/fullID/exact operation differs");
 prefix->require_unit_selected_parent_without_callbacks(*core,*host,*world,*source,control.get(),q.generation,q.previous);
 const auto&old=*r->unit_info->impl_;const auto input=old.selection.lock();const auto current=q.selection.lock();
 need(input&&current&&input==current&&!old.selection.owner_before(q.selection)&&!q.selection.owner_before(old.selection),
  "IsSelected must reuse the exact canonical first UnitInfo receiver");
 source->require_attrition_unit_selected_operation_without_callbacks(*cursor,nullptr,q.serial,q.revision,q.returned);
 if(q.returned)need(q.result==bool(old.input.selected_unit&&old.input.selected_owner&&*old.input.selected_owner==q.owner&&*old.input.selected_unit==q.unit),
  "IsSelected returned canonical predicate differs");
}
void BnwAttritionUnitSelectedRead::require_core_without_callbacks(const BnwNativeUnitAllocatorCore&core)const{
 require_common_without_callbacks();const auto prefix=impl_->prefix.lock();need(prefix&&prefix->impl_->core.lock().get()==&core,"IsSelected foreign supplied Core");
}
void BnwAttritionUnitSelectedRead::require_lease_without_callbacks(const BnwStockLuaAttemptLease&lease)const{
 require_common_without_callbacks();const auto&q=*impl_;const auto prefix=q.prefix.lock();const auto cursor=q.cursor.lock();
 need(prefix&&cursor,"IsSelected lease real owner expired");const auto source=prefix->impl_->source.lock();need(bool(source),"IsSelected lease Source expired");
 source->require_attrition_unit_selected_operation_without_callbacks(*cursor,&lease,q.serial,q.revision,q.returned);
}
void BnwAttritionUnitSelectedRead::enter_engine_once_without_callbacks(const ui::BnwEngineUiTurnOwner&engine,const BnwStockLuaAttemptLease&lease)const{
 require_alive_without_callbacks();auto&q=*impl_;if(q.entered||q.returned){fail_without_callbacks();need(false,"IsSelected caught/repeated Engine reentry");}
 q.entered=true;require_engine_without_callbacks(engine,lease);
}
void BnwAttritionUnitSelectedRead::require_engine_without_callbacks(const ui::BnwEngineUiTurnOwner&engine,const BnwStockLuaAttemptLease&lease)const{
 require_common_without_callbacks();const auto&q=*impl_;const auto actual=q.engine.lock();need(q.entered&&actual&&actual.get()==&engine,"IsSelected wrong/expired original Engine");
 lease.require_operation(q.serial);require_alive_without_callbacks();require_lease_without_callbacks(lease);
}
void BnwAttritionUnitSelectedRead::require_query_source(const ui::BnwClientSelectionControl&selection,const ui::SelectedBnwPreviewHost&host,
 const LiveSessionState&state,const BnwStockLuaAttemptLease&lease)const{
 require_common_without_callbacks();const auto&q=*impl_;const auto input=q.selection.lock();const auto prefix=q.prefix.lock();
 need(q.entered&&!q.returned&&input&&input.get()==&selection&&prefix&&prefix->impl_->host.lock().get()==&host&&prefix->impl_->world.lock().get()==&state,
  "IsSelected canonical pre-read phase differs");lease.require_operation(q.serial);require_alive_without_callbacks();require_lease_without_callbacks(lease);
}
BnwAttritionUnitSelectedRead::Operands BnwAttritionUnitSelectedRead::operands_without_callbacks()const{
 require_common_without_callbacks();const auto&q=*impl_;return {q.generation,q.revision,q.serial,q.owner,q.unit};
}
void BnwAttritionUnitSelectedRead::require_final_query_without_callbacks(const ui::BnwClientSelectionControl&selection,const ui::SelectedBnwPreviewHost&host,
 const LiveSessionState&state,const BnwStockLuaAttemptLease&lease,bool result)const{
 require_lease_without_callbacks(lease);const auto&q=*impl_;const auto input=q.selection.lock();const auto prefix=q.prefix.lock();
 need(q.entered&&!q.returned&&input&&input.get()==&selection&&prefix&&prefix->impl_->host.lock().get()==&host&&prefix->impl_->world.lock().get()==&state,
  "IsSelected final pure original receiver differs");const auto&old=*prefix->impl_->unit_info->impl_;
 need(result==bool(old.input.selected_unit&&old.input.selected_owner&&*old.input.selected_owner==q.owner&&*old.input.selected_unit==q.unit),
  "IsSelected final actual canonical predicate differs");
}
void BnwAttritionUnitSelectedRead::record_query_and_adopt_without_callbacks(const ui::BnwClientSelectionControl&selection,bool result)const noexcept{
 if(!impl_||impl_->creator!=std::this_thread::get_id())std::terminate();auto&q=*impl_;const auto input=q.selection.lock();
 const auto prefix=q.prefix.lock();const auto cursor=q.cursor.lock();
 if(q.failed||!q.entered||q.returned||!input||input.get()!=&selection||!prefix||!cursor)std::terminate();
 q.result=result;q.returned=true;cursor->record_attrition_unit_selected_return_without_callbacks(*this,result);
 const auto source=prefix->impl_->source.lock();if(!source)std::terminate();source->adopt_attrition_unit_selected_without_callbacks(q.revision);
}
void BnwAttritionUnitSelectedRead::require_return_without_callbacks(const ui::BnwClientSelectionControl&selection,const ui::SelectedBnwPreviewHost&host,
 const LiveSessionState&state,bool result)const{
 require_common_without_callbacks();const auto&q=*impl_;const auto input=q.selection.lock();const auto prefix=q.prefix.lock();
 need(q.entered&&q.returned&&q.result==result&&input&&input.get()==&selection&&prefix&&prefix->impl_->host.lock().get()==&host&&
  prefix->impl_->world.lock().get()==&state,"IsSelected actual returned query differs");
}
bool BnwAttritionUnitSelectedRead::result_without_callbacks()const{
 require_common_without_callbacks();need(impl_->entered&&impl_->returned,"IsSelected query has not returned");return impl_->result;
}
std::shared_ptr<const BnwAttritionUnitSelectedRead> BnwNativeAttritionPrefix::prepare_unit_selected_owned(const BnwNativeUnitAllocatorCore&core,
 const ui::SelectedBnwPreviewHost&host,const LiveSessionState&state,const BnwSelectedStockLuaSourceOwner&source,const void*control,
 std::uint64_t generation,std::uint64_t serial,const BnwRetainedTurnCursor&cursor,const std::shared_ptr<ui::BnwClientSelectionControl>&selection)const{
 const auto r=impl_;r->thread();if(r->unit_selected){r->unit_selected->fail_without_callbacks();need(false,"IsSelected Core-issued child replay");}
 const auto previous=cursor.attrition_unit_selected_previous_serial_without_callbacks();
 need(previous&&previous<UINT64_MAX&&serial==previous+1&&selection,"IsSelected exact genuine next operation absent");
 require_unit_selected_parent_without_callbacks(core,host,state,source,control,generation,previous);
 auto q=std::make_unique<BnwAttritionUnitSelectedRead::Impl>();q->prefix=r->self;q->cursor=cursor.retain_engine_factory_owner();
 cursor.require_attrition_city_info_facade_without_callbacks();q->selection=selection;q->engine=r->damage->engine;
 q->previous=previous;q->serial=serial;q->revision=state.ui_source_revision;q->generation=generation;q->owner=r->actor;q->unit=r->damage->id;
 require_unit_selected_parent_without_callbacks(core,host,state,source,control,generation,previous);
 r->unit_selected=std::shared_ptr<BnwAttritionUnitSelectedRead>(new BnwAttritionUnitSelectedRead(std::move(q)));
 r->unit_selected->require_common_without_callbacks();return r->unit_selected;
}
