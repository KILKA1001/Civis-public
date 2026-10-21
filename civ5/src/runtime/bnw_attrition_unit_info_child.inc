// Included after CityInfo2 in the existing runtime Core TU.
struct BnwAttritionUnitInfoDirtyLoan::Impl {
 const std::thread::id creator=std::this_thread::get_id();
 std::weak_ptr<const BnwNativeAttritionPrefix>prefix;std::weak_ptr<BnwRetainedTurnCursor>cursor;
 std::weak_ptr<ui::BnwClientSelectionControl>selection;std::weak_ptr<ui::BnwEngineUiTurnOwner>engine;
 ui::BnwClientSelectionControl::AttritionUnitInfoState input;
 ui::BnwClientUnitDirtyEvent event;
 std::uint64_t previous=0,serial=0,revision=0,generation=0;
 std::int32_t owner=-1,unit=-1;bool from_city=false,entered=false,returned=false,failed=false;
};
BnwAttritionUnitInfoDirtyLoan::BnwAttritionUnitInfoDirtyLoan(std::unique_ptr<Impl>p):impl_(std::move(p)){}
BnwAttritionUnitInfoDirtyLoan::~BnwAttritionUnitInfoDirtyLoan()=default;
void BnwAttritionUnitInfoDirtyLoan::require_alive_without_callbacks()const{
 need(impl_&&impl_->creator==std::this_thread::get_id(),"UnitInfo immutable foreign creator");
 const auto cursor=impl_->cursor.lock();need(bool(cursor),"UnitInfo original Cursor expired");
 cursor->require_attrition_city_info_facade_without_callbacks();need(!impl_->failed,"UnitInfo sticky failed child");
}
void BnwAttritionUnitInfoDirtyLoan::fail_without_callbacks()const noexcept{
 if(!impl_||impl_->creator!=std::this_thread::get_id())std::terminate();impl_->failed=true;
 const auto prefix=impl_->prefix.lock();if(prefix)prefix->impl_->failed=true;
}
void BnwNativeAttritionPrefix::require_unit_info_parent_without_callbacks(const BnwNativeUnitAllocatorCore&core,
 const ui::SelectedBnwPreviewHost&host,const LiveSessionState&state,const BnwSelectedStockLuaSourceOwner&source,
 const void*control,std::uint64_t generation,std::uint64_t previous)const{
 const auto r=impl_;r->thread();need(r->damage&&r->damage->executed&&!r->damage->failed&&r->damage->changed&&r->garrison,
  "UnitInfo requires real changed HP and genuine Garrison2");
 if(r->city_info){
  // Read OLD issued node/metadata, never admit its old serial at the new Source.
  r->city_info->require_alive_without_callbacks();const auto&q=*r->city_info->impl_;
  const auto old_prefix=q.prefix.lock();const auto self=r->self.lock();
  need(!q.failed&&q.entered&&q.returned&&r->city_strength&&q.serial==previous&&q.previous==r->city_strength->serial&&
   q.previous<UINT64_MAX&&q.serial==q.previous+1&&q.revision==state.ui_source_revision&&q.generation==generation&&
   old_prefix&&self&&old_prefix.get()==this&&self.get()==this&&!q.prefix.owner_before(r->self)&&!r->self.owner_before(q.prefix)&&
   q.unit_owner==r->actor&&q.unit==r->damage->id&&q.city_owner==r->actor&&
   !q.engine.owner_before(r->damage->engine)&&!r->damage->engine.owner_before(q.engine),
   "UnitInfo original returned CityInfo2 node/once/localized Engine lineage differs");
  require_city_strength_without_callbacks(core,host,state,source,control,generation,q.previous,true);
 }else{
  need(!r->city_strength&&r->garrison->boundary==BnwAttritionGarrisonBoundary::info_bar_dirty_owner&&
   r->garrison->serial==previous,"UnitInfo false/NULL garrison cannot skip mandatory strength/CityInfo");
  require_garrison_without_callbacks(core,host,state,source,control,generation,previous,true);
 }
}
void BnwAttritionUnitInfoDirtyLoan::require_common_without_callbacks()const{
 require_alive_without_callbacks();const auto&q=*impl_;const auto prefix=q.prefix.lock();const auto cursor=q.cursor.lock();
 need(prefix&&cursor,"UnitInfo actual Prefix/Cursor expired");const auto r=prefix->impl_;r->thread();
 const auto core=r->core.lock();const auto source=r->source.lock();const auto host=r->host.lock();const auto world=r->world.lock();const auto control=r->control.lock();
 need(core&&source&&host&&world&&control&&r->unit_info.get()==this&&q.previous<UINT64_MAX&&q.serial==q.previous+1&&
  q.generation==r->generation&&q.revision==world->ui_source_revision&&r->damage&&q.owner==r->actor&&q.unit==r->damage->id&&
  q.from_city==bool(r->city_info)&&!q.engine.owner_before(r->damage->engine)&&!r->damage->engine.owner_before(q.engine),
  "UnitInfo current genuine Source/world/fullID/exact operation parent differs");
 prefix->require_unit_info_parent_without_callbacks(*core,*host,*world,*source,control.get(),q.generation,q.previous);
 source->require_attrition_unit_info_operation_without_callbacks(*cursor,nullptr,q.serial,q.revision,q.returned);
 need(q.event.occurrence==q.input.unit.next_occurrence&&q.event.move_occurrence==0&&q.event.generation==q.generation&&
  q.event.world_revision==q.revision&&q.input.unit.client_revision<UINT64_MAX&&q.event.client_revision==q.input.unit.client_revision+1&&
  q.event.owner==q.owner&&q.event.full_id==q.unit&&q.event.kind==ui::BnwClientUnitDirtyKind::unit_info&&q.event.previous_value==q.input.unit.unit_info,
  "UnitInfo prepared genuine canonical receiver event differs");
 if(q.from_city){const auto&old=*r->city_info->impl_;const auto old_selection=old.selection.lock();const auto current=q.selection.lock();
  need(old_selection&&current&&old_selection.get()==current.get()&&!old.selection.owner_before(q.selection)&&!q.selection.owner_before(old.selection)&&
   old.input.revision<UINT64_MAX&&q.input.unit.client_revision==old.input.revision+1&&old.input.city_count<UINT64_MAX&&
   q.input.city_count==old.input.city_count+1&&q.input.city_flag==std::optional<bool>{true}&&q.input.generation==old.input.generation&&
   q.input.host==old.input.host&&q.input.state==old.input.state&&q.input.source==old.input.source,
   "UnitInfo closed successor requires exact earned CityInfo client node, not stale old admission");
 }
}
void BnwAttritionUnitInfoDirtyLoan::require_core_without_callbacks(const BnwNativeUnitAllocatorCore&core)const{
 require_common_without_callbacks();const auto prefix=impl_->prefix.lock();need(prefix&&prefix->impl_->core.lock().get()==&core,"UnitInfo foreign supplied Core");
}
void BnwAttritionUnitInfoDirtyLoan::require_lease_without_callbacks(const BnwStockLuaAttemptLease&lease)const{
 require_common_without_callbacks();const auto&q=*impl_;const auto prefix=q.prefix.lock();const auto cursor=q.cursor.lock();
 need(prefix&&cursor,"UnitInfo lease real owners expired");const auto source=prefix->impl_->source.lock();need(bool(source),"UnitInfo lease Source expired");
 source->require_attrition_unit_info_operation_without_callbacks(*cursor,&lease,q.serial,q.revision,q.returned);
}
void BnwAttritionUnitInfoDirtyLoan::enter_engine_once_without_callbacks(const ui::BnwEngineUiTurnOwner&engine,const BnwStockLuaAttemptLease&lease)const{
 require_alive_without_callbacks();auto&q=*impl_;if(q.entered||q.returned){fail_without_callbacks();need(false,"UnitInfo caught/repeated Engine reentry");}
 q.entered=true;require_engine_without_callbacks(engine,lease);
}
void BnwAttritionUnitInfoDirtyLoan::require_engine_without_callbacks(const ui::BnwEngineUiTurnOwner&engine,const BnwStockLuaAttemptLease&lease)const{
 require_common_without_callbacks();const auto&q=*impl_;const auto actual=q.engine.lock();
 need(q.entered&&actual&&actual.get()==&engine,"UnitInfo wrong/expired original localization Engine");
 lease.require_operation(q.serial);require_alive_without_callbacks();require_lease_without_callbacks(lease);
}
void BnwAttritionUnitInfoDirtyLoan::require_setter_source(const ui::BnwClientSelectionControl&selection,
 const ui::SelectedBnwPreviewHost&host,const LiveSessionState&state,const BnwStockLuaAttemptLease&lease)const{
 require_common_without_callbacks();const auto&q=*impl_;const auto input=q.selection.lock();const auto prefix=q.prefix.lock();
 need(q.entered&&!q.returned&&input&&input.get()==&selection&&prefix&&prefix->impl_->host.lock().get()==&host&&prefix->impl_->world.lock().get()==&state&&
  selection.attrition_unit_info_state_matches_without_callbacks(q.input,q.event,false),"UnitInfo actual canonical Selection/pre-store phase differs");
 lease.require_operation(q.serial);require_alive_without_callbacks();require_lease_without_callbacks(lease);
 need(selection.attrition_unit_info_state_matches_without_callbacks(q.input,q.event,false),"UnitInfo input changed during source callback");
}
BnwAttritionUnitInfoDirtyLoan::Operands BnwAttritionUnitInfoDirtyLoan::operands_without_callbacks()const{
 require_common_without_callbacks();const auto&q=*impl_;return {q.generation,q.revision,q.serial,q.input.unit.client_revision,q.input.unit.next_occurrence,q.owner,q.unit};
}
ui::BnwClientUnitDirtyEvent BnwAttritionUnitInfoDirtyLoan::event_without_callbacks()const{require_common_without_callbacks();return impl_->event;}
void BnwAttritionUnitInfoDirtyLoan::require_final_setter_without_callbacks(const ui::BnwClientSelectionControl&selection,
 const ui::SelectedBnwPreviewHost&host,const LiveSessionState&state,const BnwStockLuaAttemptLease&lease,std::uint64_t revision,std::uint64_t occurrence)const{
 require_lease_without_callbacks(lease);const auto&q=*impl_;const auto actual=q.selection.lock();const auto prefix=q.prefix.lock();
 need(q.entered&&!q.returned&&actual&&actual.get()==&selection&&prefix&&prefix->impl_->host.lock().get()==&host&&prefix->impl_->world.lock().get()==&state&&
  q.input.unit.client_revision==revision&&q.input.unit.next_occurrence==occurrence&&revision<UINT64_MAX&&occurrence>0&&occurrence<UINT64_MAX&&
  selection.attrition_unit_info_state_matches_without_callbacks(q.input,q.event,false),"UnitInfo final pure actual setter operands differ");
}
void BnwAttritionUnitInfoDirtyLoan::record_setter_and_adopt_without_callbacks(const ui::BnwClientSelectionControl&selection,std::uint64_t revision,std::uint64_t next)const noexcept{
 if(!impl_||impl_->creator!=std::this_thread::get_id())std::terminate();auto&q=*impl_;
 const auto actual=q.selection.lock();const auto prefix=q.prefix.lock();const auto cursor=q.cursor.lock();
 if(q.failed||!q.entered||q.returned||!actual||actual.get()!=&selection||!prefix||!cursor||q.input.unit.client_revision==UINT64_MAX||
  q.input.unit.next_occurrence==UINT64_MAX||revision!=q.input.unit.client_revision+1||next!=q.input.unit.next_occurrence+1)std::terminate();
 q.returned=true;cursor->record_attrition_unit_info_return_without_callbacks(*this,revision,q.event.occurrence);
 const auto source=prefix->impl_->source.lock();if(!source)std::terminate();source->adopt_attrition_unit_info_without_callbacks(q.revision);
}
void BnwAttritionUnitInfoDirtyLoan::require_return_without_callbacks(const ui::BnwClientSelectionControl&selection,const ui::SelectedBnwPreviewHost&host,
 const LiveSessionState&state,std::uint64_t revision,std::uint64_t next)const{
 require_common_without_callbacks();const auto&q=*impl_;const auto actual=q.selection.lock();const auto prefix=q.prefix.lock();
 need(q.entered&&q.returned&&actual&&actual.get()==&selection&&prefix&&prefix->impl_->host.lock().get()==&host&&prefix->impl_->world.lock().get()==&state&&
  q.input.unit.client_revision<UINT64_MAX&&q.input.unit.next_occurrence<UINT64_MAX&&revision==q.input.unit.client_revision+1&&next==q.input.unit.next_occurrence+1&&
  selection.attrition_unit_info_state_matches_without_callbacks(q.input,q.event,true),"UnitInfo real flag/journal return differs; not event/frame ACK");
}
std::shared_ptr<const BnwAttritionUnitInfoDirtyLoan> BnwNativeAttritionPrefix::prepare_unit_info_owned(const BnwNativeUnitAllocatorCore&core,
 const ui::SelectedBnwPreviewHost&host,const LiveSessionState&state,const BnwSelectedStockLuaSourceOwner&source,const void*control,
 std::uint64_t generation,std::uint64_t serial,const BnwRetainedTurnCursor&cursor,const std::shared_ptr<ui::BnwClientSelectionControl>&selection)const{
 const auto r=impl_;r->thread();if(r->unit_info){r->unit_info->fail_without_callbacks();need(false,"UnitInfo Core-issued child replay");}
 const auto previous=cursor.attrition_unit_info_previous_serial_without_callbacks();
 need(previous&&previous<UINT64_MAX&&serial==previous+1&&selection,"UnitInfo requires exact genuine next parent operation");
 require_unit_info_parent_without_callbacks(core,host,state,source,control,generation,previous);
 auto q=std::make_unique<BnwAttritionUnitInfoDirtyLoan::Impl>();q->prefix=r->self;q->cursor=cursor.retain_engine_factory_owner();
 cursor.require_attrition_city_info_facade_without_callbacks();q->selection=selection;q->engine=r->damage->engine;
 q->input=selection->capture_attrition_unit_info_without_callbacks();q->previous=previous;q->serial=serial;q->revision=state.ui_source_revision;
 q->generation=generation;q->owner=r->actor;q->unit=r->damage->id;q->from_city=bool(r->city_info);
 need(q->input.unit.client_revision<UINT64_MAX&&q->input.unit.next_occurrence>0&&q->input.unit.next_occurrence<UINT64_MAX&&
  q->input.unit.events.size()<q->input.unit.events.max_size(),"UnitInfo actual client/ordinal/journal capacity exhausted");
 // Zero move_occurrence is the existing canonical non-move setter journal
 // sentinel (center/best-land); this direct native setter issues no MovePending.
 q->event={q->input.unit.next_occurrence,0,generation,q->revision,q->input.unit.client_revision+1,q->owner,q->unit,ui::BnwClientUnitDirtyKind::unit_info,q->input.unit.unit_info};
 require_unit_info_parent_without_callbacks(core,host,state,source,control,generation,previous);
 need(selection->attrition_unit_info_state_matches_without_callbacks(q->input,q->event,false),"UnitInfo actual input changed during preparation");
 r->unit_info=std::shared_ptr<BnwAttritionUnitInfoDirtyLoan>(new BnwAttritionUnitInfoDirtyLoan(std::move(q)));
 r->unit_info->require_common_without_callbacks();return r->unit_info;
}
