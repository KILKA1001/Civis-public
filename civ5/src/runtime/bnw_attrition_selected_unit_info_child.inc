// Included after IsSelected1 in the existing runtime Core TU.
struct BnwAttritionSelectedUnitInfoDirtyLoan::Impl {
 const std::thread::id creator=std::this_thread::get_id();
 std::weak_ptr<const BnwNativeAttritionPrefix>prefix;std::weak_ptr<BnwRetainedTurnCursor>cursor;
 std::weak_ptr<ui::BnwClientSelectionControl>selection;std::weak_ptr<ui::BnwEngineUiTurnOwner>engine;
 ui::BnwClientSelectionControl::AttritionUnitInfoState input;
 ui::BnwClientUnitDirtyEvent event;
 std::uint64_t previous=0,serial=0,revision=0,generation=0;
 std::int32_t owner=-1,unit=-1;bool from_city=false,entered=false,returned=false,failed=false;
};
BnwAttritionSelectedUnitInfoDirtyLoan::BnwAttritionSelectedUnitInfoDirtyLoan(std::unique_ptr<Impl>p):impl_(std::move(p)){}
BnwAttritionSelectedUnitInfoDirtyLoan::~BnwAttritionSelectedUnitInfoDirtyLoan()=default;
void BnwAttritionSelectedUnitInfoDirtyLoan::require_alive_without_callbacks()const{
 need(impl_&&impl_->creator==std::this_thread::get_id(),"UnitInfo immutable foreign creator");
 const auto cursor=impl_->cursor.lock();need(bool(cursor),"UnitInfo original Cursor expired");
 cursor->require_attrition_city_info_facade_without_callbacks();need(!impl_->failed,"UnitInfo sticky failed child");
}
void BnwAttritionSelectedUnitInfoDirtyLoan::fail_without_callbacks()const noexcept{
 if(!impl_||impl_->creator!=std::this_thread::get_id())std::terminate();impl_->failed=true;
 const auto prefix=impl_->prefix.lock();if(prefix)prefix->impl_->failed=true;
}
void BnwNativeAttritionPrefix::require_selected_unit_info_parent_without_callbacks(const BnwNativeUnitAllocatorCore&core,
 const ui::SelectedBnwPreviewHost&host,const LiveSessionState&state,const BnwSelectedStockLuaSourceOwner&source,
 const void*control,std::uint64_t generation,std::uint64_t previous)const{
 const auto r=impl_;r->thread();need(r->unit_selected&&r->unit_info&&r->damage,"Second UnitInfo requires genuine IsSelected/first UnitInfo");
 r->unit_selected->require_alive_without_callbacks();r->unit_info->require_alive_without_callbacks();
 const auto&read=*r->unit_selected->impl_;const auto&first=*r->unit_info->impl_;
 const auto read_prefix=read.prefix.lock();const auto first_prefix=first.prefix.lock();const auto self=r->self.lock();
 const auto read_input=read.selection.lock();const auto first_input=first.selection.lock();
 need(!read.failed&&read.entered&&read.returned&&read.result&&read.serial==previous&&read.previous==first.serial&&read.previous<UINT64_MAX&&
  read.serial==read.previous+1&&read.generation==generation&&read.revision==state.ui_source_revision&&read_prefix&&self&&read_prefix.get()==this&&self.get()==this&&
  !read.prefix.owner_before(r->self)&&!r->self.owner_before(read.prefix)&&!first.failed&&first.entered&&first.returned&&first_prefix&&first_prefix.get()==this&&
  !first.prefix.owner_before(r->self)&&!r->self.owner_before(first.prefix)&&first.previous<UINT64_MAX&&first.serial==first.previous+1&&
  first.generation==generation&&first.revision==read.revision&&
  first.owner==r->actor&&read.owner==r->actor&&first.unit==r->damage->id&&read.unit==r->damage->id&&first.from_city==bool(r->city_info)&&
  read_input&&first_input&&read_input==first_input&&!read.selection.owner_before(first.selection)&&!first.selection.owner_before(read.selection)&&
  !read.engine.owner_before(r->damage->engine)&&!r->damage->engine.owner_before(read.engine)&&
  !first.engine.owner_before(r->damage->engine)&&!r->damage->engine.owner_before(first.engine),
  "Second UnitInfo requires immutable actual true IsSelected and original first UnitInfo nodes/once/fullID/Engine");
 // Never renew the old UI source serial after the second setter. Its physical
 // HP/garrison/city nodes stay exact; the new child proves its own UI after.
 require_unit_info_parent_without_callbacks(core,host,state,source,control,generation,first.previous);
 need(first.input.selected_unit&&first.input.selected_owner&&*first.input.selected_owner==r->actor&&*first.input.selected_unit==r->damage->id,
  "Second UnitInfo cannot replace the native selected=true predicate");
}
void BnwAttritionSelectedUnitInfoDirtyLoan::require_common_without_callbacks()const{
 require_alive_without_callbacks();const auto&q=*impl_;const auto prefix=q.prefix.lock();const auto cursor=q.cursor.lock();
 need(prefix&&cursor,"UnitInfo actual Prefix/Cursor expired");const auto r=prefix->impl_;r->thread();
 const auto core=r->core.lock();const auto source=r->source.lock();const auto host=r->host.lock();const auto world=r->world.lock();const auto control=r->control.lock();
 need(core&&source&&host&&world&&control&&r->selected_unit_info.get()==this&&q.previous<UINT64_MAX&&q.serial==q.previous+1&&
  q.generation==r->generation&&q.revision==world->ui_source_revision&&r->damage&&q.owner==r->actor&&q.unit==r->damage->id&&
  q.from_city==bool(r->city_info)&&!q.engine.owner_before(r->damage->engine)&&!r->damage->engine.owner_before(q.engine),
  "UnitInfo current genuine Source/world/fullID/exact operation parent differs");
 prefix->require_selected_unit_info_parent_without_callbacks(*core,*host,*world,*source,control.get(),q.generation,q.previous);
 source->require_attrition_selected_unit_info_operation_without_callbacks(*cursor,nullptr,q.serial,q.revision,q.returned);
 need(q.event.occurrence==q.input.unit.next_occurrence&&q.event.move_occurrence==0&&q.event.generation==q.generation&&
  q.event.world_revision==q.revision&&q.input.unit.client_revision<UINT64_MAX&&q.event.client_revision==q.input.unit.client_revision+1&&
  q.event.owner==q.owner&&q.event.full_id==q.unit&&q.event.kind==ui::BnwClientUnitDirtyKind::unit_info&&q.event.previous_value==q.input.unit.unit_info,
  "UnitInfo prepared genuine canonical receiver event differs");
 const auto&old=*r->unit_info->impl_;const auto old_selection=old.selection.lock();const auto current=q.selection.lock();
 need(old_selection&&current&&old_selection==current&&!old.selection.owner_before(q.selection)&&!q.selection.owner_before(old.selection)&&
  old.input.unit.client_revision<UINT64_MAX&&q.input.unit.client_revision==old.input.unit.client_revision+1&&
  old.input.unit.next_occurrence<UINT64_MAX&&q.input.unit.next_occurrence==old.input.unit.next_occurrence+1&&q.input.unit.unit_info&&
  q.input.unit.plot_list_buttons==old.input.unit.plot_list_buttons&&old.input.unit.events.size()<old.input.unit.events.max_size()&&
  q.input.unit.events.size()==old.input.unit.events.size()+1&&std::equal(old.input.unit.events.begin(),old.input.unit.events.end(),q.input.unit.events.begin())&&
  q.input.unit.events.back()==old.event&&q.input.city_flag==old.input.city_flag&&q.input.city_count==old.input.city_count&&q.input.generation==old.input.generation&&
  q.input.host==old.input.host&&q.input.state==old.input.state&&q.input.source==old.input.source&&q.input.selected==old.input.selected&&
  q.input.selected_unit==old.input.selected_unit&&q.input.selected_owner==old.input.selected_owner&&q.input.selected_plot==old.input.selected_plot,
  "Second UnitInfo NEW input must be exact earned first UnitInfo state, never an old renewed admission");
}
void BnwAttritionSelectedUnitInfoDirtyLoan::require_core_without_callbacks(const BnwNativeUnitAllocatorCore&core)const{
 require_common_without_callbacks();const auto prefix=impl_->prefix.lock();need(prefix&&prefix->impl_->core.lock().get()==&core,"UnitInfo foreign supplied Core");
}
void BnwAttritionSelectedUnitInfoDirtyLoan::require_lease_without_callbacks(const BnwStockLuaAttemptLease&lease)const{
 require_common_without_callbacks();const auto&q=*impl_;const auto prefix=q.prefix.lock();const auto cursor=q.cursor.lock();
 need(prefix&&cursor,"UnitInfo lease real owners expired");const auto source=prefix->impl_->source.lock();need(bool(source),"UnitInfo lease Source expired");
 source->require_attrition_selected_unit_info_operation_without_callbacks(*cursor,&lease,q.serial,q.revision,q.returned);
}
void BnwAttritionSelectedUnitInfoDirtyLoan::enter_engine_once_without_callbacks(const ui::BnwEngineUiTurnOwner&engine,const BnwStockLuaAttemptLease&lease)const{
 require_alive_without_callbacks();auto&q=*impl_;if(q.entered||q.returned){fail_without_callbacks();need(false,"UnitInfo caught/repeated Engine reentry");}
 q.entered=true;require_engine_without_callbacks(engine,lease);
}
void BnwAttritionSelectedUnitInfoDirtyLoan::require_engine_without_callbacks(const ui::BnwEngineUiTurnOwner&engine,const BnwStockLuaAttemptLease&lease)const{
 require_common_without_callbacks();const auto&q=*impl_;const auto actual=q.engine.lock();
 need(q.entered&&actual&&actual.get()==&engine,"UnitInfo wrong/expired original localization Engine");
 lease.require_operation(q.serial);require_alive_without_callbacks();require_lease_without_callbacks(lease);
}
void BnwAttritionSelectedUnitInfoDirtyLoan::require_setter_source(const ui::BnwClientSelectionControl&selection,
 const ui::SelectedBnwPreviewHost&host,const LiveSessionState&state,const BnwStockLuaAttemptLease&lease)const{
 require_common_without_callbacks();const auto&q=*impl_;const auto input=q.selection.lock();const auto prefix=q.prefix.lock();
 need(q.entered&&!q.returned&&input&&input.get()==&selection&&prefix&&prefix->impl_->host.lock().get()==&host&&prefix->impl_->world.lock().get()==&state&&
  selection.attrition_unit_info_state_matches_without_callbacks(q.input,q.event,false),"UnitInfo actual canonical Selection/pre-store phase differs");
 lease.require_operation(q.serial);require_alive_without_callbacks();require_lease_without_callbacks(lease);
 need(selection.attrition_unit_info_state_matches_without_callbacks(q.input,q.event,false),"UnitInfo input changed during source callback");
}
BnwAttritionSelectedUnitInfoDirtyLoan::Operands BnwAttritionSelectedUnitInfoDirtyLoan::operands_without_callbacks()const{
 require_common_without_callbacks();const auto&q=*impl_;return {q.generation,q.revision,q.serial,q.input.unit.client_revision,q.input.unit.next_occurrence,q.owner,q.unit};
}
ui::BnwClientUnitDirtyEvent BnwAttritionSelectedUnitInfoDirtyLoan::event_without_callbacks()const{require_common_without_callbacks();return impl_->event;}
void BnwAttritionSelectedUnitInfoDirtyLoan::require_final_setter_without_callbacks(const ui::BnwClientSelectionControl&selection,
 const ui::SelectedBnwPreviewHost&host,const LiveSessionState&state,const BnwStockLuaAttemptLease&lease,std::uint64_t revision,std::uint64_t occurrence)const{
 require_lease_without_callbacks(lease);const auto&q=*impl_;const auto actual=q.selection.lock();const auto prefix=q.prefix.lock();
 need(q.entered&&!q.returned&&actual&&actual.get()==&selection&&prefix&&prefix->impl_->host.lock().get()==&host&&prefix->impl_->world.lock().get()==&state&&
  q.input.unit.client_revision==revision&&q.input.unit.next_occurrence==occurrence&&revision<UINT64_MAX&&occurrence>0&&occurrence<UINT64_MAX&&
  selection.attrition_unit_info_state_matches_without_callbacks(q.input,q.event,false),"UnitInfo final pure actual setter operands differ");
}
void BnwAttritionSelectedUnitInfoDirtyLoan::record_setter_and_adopt_without_callbacks(const ui::BnwClientSelectionControl&selection,std::uint64_t revision,std::uint64_t next)const noexcept{
 if(!impl_||impl_->creator!=std::this_thread::get_id())std::terminate();auto&q=*impl_;
 const auto actual=q.selection.lock();const auto prefix=q.prefix.lock();const auto cursor=q.cursor.lock();
 if(q.failed||!q.entered||q.returned||!actual||actual.get()!=&selection||!prefix||!cursor||q.input.unit.client_revision==UINT64_MAX||
  q.input.unit.next_occurrence==UINT64_MAX||revision!=q.input.unit.client_revision+1||next!=q.input.unit.next_occurrence+1)std::terminate();
 q.returned=true;cursor->record_attrition_selected_unit_info_return_without_callbacks(*this,revision,q.event.occurrence);
 const auto source=prefix->impl_->source.lock();if(!source)std::terminate();source->adopt_attrition_selected_unit_info_without_callbacks(q.revision);
}
void BnwAttritionSelectedUnitInfoDirtyLoan::require_return_without_callbacks(const ui::BnwClientSelectionControl&selection,const ui::SelectedBnwPreviewHost&host,
 const LiveSessionState&state,std::uint64_t revision,std::uint64_t next)const{
 require_common_without_callbacks();const auto&q=*impl_;const auto actual=q.selection.lock();const auto prefix=q.prefix.lock();
 need(q.entered&&q.returned&&actual&&actual.get()==&selection&&prefix&&prefix->impl_->host.lock().get()==&host&&prefix->impl_->world.lock().get()==&state&&
  q.input.unit.client_revision<UINT64_MAX&&q.input.unit.next_occurrence<UINT64_MAX&&revision==q.input.unit.client_revision+1&&next==q.input.unit.next_occurrence+1&&
  selection.attrition_unit_info_state_matches_without_callbacks(q.input,q.event,true),"UnitInfo real flag/journal return differs; not event/frame ACK");
}
std::shared_ptr<const BnwAttritionSelectedUnitInfoDirtyLoan> BnwNativeAttritionPrefix::prepare_selected_unit_info_owned(const BnwNativeUnitAllocatorCore&core,
 const ui::SelectedBnwPreviewHost&host,const LiveSessionState&state,const BnwSelectedStockLuaSourceOwner&source,const void*control,
 std::uint64_t generation,std::uint64_t serial,const BnwRetainedTurnCursor&cursor,const std::shared_ptr<ui::BnwClientSelectionControl>&selection)const{
 const auto r=impl_;r->thread();if(r->selected_unit_info){r->selected_unit_info->fail_without_callbacks();need(false,"UnitInfo Core-issued child replay");}
 const auto previous=cursor.attrition_selected_unit_info_previous_serial_without_callbacks();
 need(previous&&previous<UINT64_MAX&&serial==previous+1&&selection,"UnitInfo requires exact genuine next parent operation");
 require_selected_unit_info_parent_without_callbacks(core,host,state,source,control,generation,previous);
 auto q=std::make_unique<BnwAttritionSelectedUnitInfoDirtyLoan::Impl>();q->prefix=r->self;q->cursor=cursor.retain_engine_factory_owner();
 cursor.require_attrition_city_info_facade_without_callbacks();q->selection=selection;q->engine=r->damage->engine;
 const auto&first=*r->unit_info->impl_;need(selection->attrition_unit_info_state_matches_without_callbacks(first.input,first.event,true),
  "Second UnitInfo real canonical input changed since native IsSelected");
 q->input=selection->capture_attrition_unit_info_without_callbacks();q->previous=previous;q->serial=serial;q->revision=state.ui_source_revision;
 q->generation=generation;q->owner=r->actor;q->unit=r->damage->id;q->from_city=bool(r->city_info);
 need(q->input.unit.client_revision<UINT64_MAX&&q->input.unit.next_occurrence>0&&q->input.unit.next_occurrence<UINT64_MAX&&
  q->input.unit.events.size()<q->input.unit.events.max_size(),"UnitInfo actual client/ordinal/journal capacity exhausted");
 // Zero move_occurrence is the existing canonical non-move setter journal
 // sentinel (center/best-land); this direct native setter issues no MovePending.
 q->event={q->input.unit.next_occurrence,0,generation,q->revision,q->input.unit.client_revision+1,q->owner,q->unit,ui::BnwClientUnitDirtyKind::unit_info,q->input.unit.unit_info};
 require_selected_unit_info_parent_without_callbacks(core,host,state,source,control,generation,previous);
 need(selection->attrition_unit_info_state_matches_without_callbacks(q->input,q->event,false),"UnitInfo actual input changed during preparation");
 r->selected_unit_info=std::shared_ptr<BnwAttritionSelectedUnitInfoDirtyLoan>(new BnwAttritionSelectedUnitInfoDirtyLoan(std::move(q)));
 r->selected_unit_info->require_common_without_callbacks();return r->selected_unit_info;
}
