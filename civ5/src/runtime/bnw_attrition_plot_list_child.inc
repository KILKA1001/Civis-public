// Included after SelectionPlot1 in the existing runtime Core TU.
struct BnwAttritionPlotListDirtyLoan::Impl {
 const std::thread::id creator=std::this_thread::get_id();
 std::weak_ptr<const BnwNativeAttritionPrefix>prefix;std::weak_ptr<BnwRetainedTurnCursor>cursor;
 std::weak_ptr<ui::BnwClientSelectionControl>selection;std::weak_ptr<ui::BnwEngineUiTurnOwner>engine;
 ui::BnwClientSelectionControl::AttritionUnitInfoState input;
 ui::BnwClientUnitDirtyEvent event;
 std::uint64_t previous=0,serial=0,revision=0,generation=0;
 std::int32_t owner=-1,unit=-1;bool from_city=false,entered=false,returned=false,failed=false;
};
BnwAttritionPlotListDirtyLoan::BnwAttritionPlotListDirtyLoan(std::unique_ptr<Impl>p):impl_(std::move(p)){}
BnwAttritionPlotListDirtyLoan::~BnwAttritionPlotListDirtyLoan()=default;
void BnwAttritionPlotListDirtyLoan::require_alive_without_callbacks()const{
 need(impl_&&impl_->creator==std::this_thread::get_id(),"UnitInfo immutable foreign creator");
 const auto cursor=impl_->cursor.lock();need(bool(cursor),"UnitInfo original Cursor expired");
 cursor->require_attrition_city_info_facade_without_callbacks();need(!impl_->failed,"UnitInfo sticky failed child");
}
void BnwAttritionPlotListDirtyLoan::fail_without_callbacks()const noexcept{
 if(!impl_||impl_->creator!=std::this_thread::get_id())std::terminate();impl_->failed=true;
 const auto prefix=impl_->prefix.lock();if(prefix)prefix->impl_->failed=true;
}
void BnwNativeAttritionPrefix::require_plot_list_parent_without_callbacks(const BnwNativeUnitAllocatorCore&core,
 const ui::SelectedBnwPreviewHost&host,const LiveSessionState&state,const BnwSelectedStockLuaSourceOwner&source,
 const void*control,std::uint64_t generation,std::uint64_t previous)const{
 const auto r=impl_;r->thread();need(r->selection_plot&&r->unit_selected&&r->unit_info&&r->damage,"PlotList genuine native plot/query parents absent");
 r->selection_plot->require_alive_without_callbacks();r->unit_info->require_alive_without_callbacks();r->unit_selected->require_alive_without_callbacks();
 const auto&plot=*r->selection_plot->impl_;const auto&first=*r->unit_info->impl_;const auto&read=*r->unit_selected->impl_;
 const auto issuer=plot.prefix.lock();const auto self=r->self.lock();
 need(!plot.failed&&plot.entered&&plot.returned&&plot.result&&plot.unit_plot>=0&&*plot.result==plot.unit_plot&&plot.serial==previous&&plot.previous<UINT64_MAX&&
  plot.serial==plot.previous+1&&plot.generation==generation&&plot.revision==state.ui_source_revision&&issuer&&self&&issuer.get()==this&&self.get()==this&&
  !plot.prefix.owner_before(r->self)&&!r->self.owner_before(plot.prefix)&&plot.owner==r->actor&&plot.unit==r->damage->id&&
  !plot.engine.owner_before(r->damage->engine)&&!r->damage->engine.owner_before(plot.engine)&&
  !read.failed&&read.entered&&read.returned&&!first.failed&&first.entered&&first.returned&&first.generation==generation&&first.revision==plot.revision&&
  first.previous<UINT64_MAX&&first.serial==first.previous+1&&read.previous==first.serial&&read.previous<UINT64_MAX&&read.serial==read.previous+1&&
  !plot.selection.owner_before(read.selection)&&!read.selection.owner_before(plot.selection)&&
  !first.selection.owner_before(read.selection)&&!read.selection.owner_before(first.selection),"PlotList immutable actual index-match/once/source/fullID/receiver differs");
 if(r->selected_unit_info){r->selected_unit_info->require_alive_without_callbacks();const auto&second=*r->selected_unit_info->impl_;
  need(read.result&&!second.failed&&second.entered&&second.returned&&plot.previous==second.serial,
   "PlotList true path requires returned mandatory SECOND UnitInfo");
  require_selected_unit_info_parent_without_callbacks(core,host,state,source,control,generation,second.previous);
 }else{need(!read.result&&plot.previous==read.serial,"PlotList false path cannot invent/skip conditional second UnitInfo");
  require_unit_info_parent_without_callbacks(core,host,state,source,control,generation,first.previous);}
 // No historical UI-currentness seal is renewed after PlotList changes it.
}
void BnwAttritionPlotListDirtyLoan::require_common_without_callbacks()const{
 require_alive_without_callbacks();const auto&q=*impl_;const auto prefix=q.prefix.lock();const auto cursor=q.cursor.lock();
 need(prefix&&cursor,"UnitInfo actual Prefix/Cursor expired");const auto r=prefix->impl_;r->thread();
 const auto core=r->core.lock();const auto source=r->source.lock();const auto host=r->host.lock();const auto world=r->world.lock();const auto control=r->control.lock();
 need(core&&source&&host&&world&&control&&r->plot_list.get()==this&&q.previous<UINT64_MAX&&q.serial==q.previous+1&&
  q.generation==r->generation&&q.revision==world->ui_source_revision&&r->damage&&q.owner==r->actor&&q.unit==r->damage->id&&
  q.from_city==bool(r->city_info)&&!q.engine.owner_before(r->damage->engine)&&!r->damage->engine.owner_before(q.engine),
  "UnitInfo current genuine Source/world/fullID/exact operation parent differs");
 prefix->require_plot_list_parent_without_callbacks(*core,*host,*world,*source,control.get(),q.generation,q.previous);
 source->require_attrition_plot_list_operation_without_callbacks(*cursor,nullptr,q.serial,q.revision,q.returned);
 need(q.event.occurrence==q.input.unit.next_occurrence&&q.event.move_occurrence==0&&q.event.generation==q.generation&&
  q.event.world_revision==q.revision&&q.input.unit.client_revision<UINT64_MAX&&q.event.client_revision==q.input.unit.client_revision+1&&
  q.event.owner==q.owner&&q.event.full_id==q.unit&&q.event.kind==ui::BnwClientUnitDirtyKind::plot_list_buttons&&q.event.previous_value==q.input.unit.plot_list_buttons,
  "UnitInfo prepared genuine canonical receiver event differs");
 const auto&old_input=r->selected_unit_info?r->selected_unit_info->impl_->input:r->unit_info->impl_->input;
 const auto&old_event=r->selected_unit_info?r->selected_unit_info->impl_->event:r->unit_info->impl_->event;
 const auto old_selection=(r->selected_unit_info?r->selected_unit_info->impl_->selection:r->unit_info->impl_->selection).lock();const auto current=q.selection.lock();
 need(old_selection&&current&&old_selection==current&&!old_selection.owner_before(current)&&!current.owner_before(old_selection)&&
  old_input.unit.client_revision<UINT64_MAX&&q.input.unit.client_revision==old_input.unit.client_revision+1&&
  old_input.unit.next_occurrence<UINT64_MAX&&q.input.unit.next_occurrence==old_input.unit.next_occurrence+1&&q.input.unit.unit_info&&
  q.input.unit.plot_list_buttons==old_input.unit.plot_list_buttons&&old_input.unit.events.size()<old_input.unit.events.max_size()&&
  q.input.unit.events.size()==old_input.unit.events.size()+1&&std::equal(old_input.unit.events.begin(),old_input.unit.events.end(),q.input.unit.events.begin())&&
  q.input.unit.events.back()==old_event&&q.input.city_flag==old_input.city_flag&&q.input.city_count==old_input.city_count&&q.input.generation==old_input.generation&&
  q.input.host==old_input.host&&q.input.state==old_input.state&&q.input.source==old_input.source&&q.input.selected==old_input.selected&&
  q.input.selected_unit==old_input.selected_unit&&q.input.selected_owner==old_input.selected_owner&&q.input.selected_plot==old_input.selected_plot&&
  q.input.selected_plot==r->selection_plot->impl_->result,"PlotList NEW input must exactly preserve genuine getter's earned native branch state");
}
void BnwAttritionPlotListDirtyLoan::require_core_without_callbacks(const BnwNativeUnitAllocatorCore&core)const{
 require_common_without_callbacks();const auto prefix=impl_->prefix.lock();need(prefix&&prefix->impl_->core.lock().get()==&core,"UnitInfo foreign supplied Core");
}
void BnwAttritionPlotListDirtyLoan::require_lease_without_callbacks(const BnwStockLuaAttemptLease&lease)const{
 require_common_without_callbacks();const auto&q=*impl_;const auto prefix=q.prefix.lock();const auto cursor=q.cursor.lock();
 need(prefix&&cursor,"UnitInfo lease real owners expired");const auto source=prefix->impl_->source.lock();need(bool(source),"UnitInfo lease Source expired");
 source->require_attrition_plot_list_operation_without_callbacks(*cursor,&lease,q.serial,q.revision,q.returned);
}
void BnwAttritionPlotListDirtyLoan::enter_engine_once_without_callbacks(const ui::BnwEngineUiTurnOwner&engine,const BnwStockLuaAttemptLease&lease)const{
 require_alive_without_callbacks();auto&q=*impl_;if(q.entered||q.returned){fail_without_callbacks();need(false,"UnitInfo caught/repeated Engine reentry");}
 q.entered=true;require_engine_without_callbacks(engine,lease);
}
void BnwAttritionPlotListDirtyLoan::require_engine_without_callbacks(const ui::BnwEngineUiTurnOwner&engine,const BnwStockLuaAttemptLease&lease)const{
 require_common_without_callbacks();const auto&q=*impl_;const auto actual=q.engine.lock();
 need(q.entered&&actual&&actual.get()==&engine,"UnitInfo wrong/expired original localization Engine");
 lease.require_operation(q.serial);require_alive_without_callbacks();require_lease_without_callbacks(lease);
}
void BnwAttritionPlotListDirtyLoan::require_setter_source(const ui::BnwClientSelectionControl&selection,
 const ui::SelectedBnwPreviewHost&host,const LiveSessionState&state,const BnwStockLuaAttemptLease&lease)const{
 require_common_without_callbacks();const auto&q=*impl_;const auto input=q.selection.lock();const auto prefix=q.prefix.lock();
 need(q.entered&&!q.returned&&input&&input.get()==&selection&&prefix&&prefix->impl_->host.lock().get()==&host&&prefix->impl_->world.lock().get()==&state&&
  selection.attrition_plot_list_state_matches_without_callbacks(q.input,q.event,false),"UnitInfo actual canonical Selection/pre-store phase differs");
 lease.require_operation(q.serial);require_alive_without_callbacks();require_lease_without_callbacks(lease);
 need(selection.attrition_plot_list_state_matches_without_callbacks(q.input,q.event,false),"UnitInfo input changed during source callback");
}
BnwAttritionPlotListDirtyLoan::Operands BnwAttritionPlotListDirtyLoan::operands_without_callbacks()const{
 require_common_without_callbacks();const auto&q=*impl_;return {q.generation,q.revision,q.serial,q.input.unit.client_revision,q.input.unit.next_occurrence,q.owner,q.unit};
}
ui::BnwClientUnitDirtyEvent BnwAttritionPlotListDirtyLoan::event_without_callbacks()const{require_common_without_callbacks();return impl_->event;}
void BnwAttritionPlotListDirtyLoan::require_final_setter_without_callbacks(const ui::BnwClientSelectionControl&selection,
 const ui::SelectedBnwPreviewHost&host,const LiveSessionState&state,const BnwStockLuaAttemptLease&lease,std::uint64_t revision,std::uint64_t occurrence)const{
 require_lease_without_callbacks(lease);const auto&q=*impl_;const auto actual=q.selection.lock();const auto prefix=q.prefix.lock();
 need(q.entered&&!q.returned&&actual&&actual.get()==&selection&&prefix&&prefix->impl_->host.lock().get()==&host&&prefix->impl_->world.lock().get()==&state&&
  q.input.unit.client_revision==revision&&q.input.unit.next_occurrence==occurrence&&revision<UINT64_MAX&&occurrence>0&&occurrence<UINT64_MAX&&
  selection.attrition_plot_list_state_matches_without_callbacks(q.input,q.event,false),"UnitInfo final pure actual setter operands differ");
}
void BnwAttritionPlotListDirtyLoan::record_setter_and_adopt_without_callbacks(const ui::BnwClientSelectionControl&selection,std::uint64_t revision,std::uint64_t next)const noexcept{
 if(!impl_||impl_->creator!=std::this_thread::get_id())std::terminate();auto&q=*impl_;
 const auto actual=q.selection.lock();const auto prefix=q.prefix.lock();const auto cursor=q.cursor.lock();
 if(q.failed||!q.entered||q.returned||!actual||actual.get()!=&selection||!prefix||!cursor||q.input.unit.client_revision==UINT64_MAX||
  q.input.unit.next_occurrence==UINT64_MAX||revision!=q.input.unit.client_revision+1||next!=q.input.unit.next_occurrence+1)std::terminate();
 q.returned=true;cursor->record_attrition_plot_list_return_without_callbacks(*this,revision,q.event.occurrence);
 const auto source=prefix->impl_->source.lock();if(!source)std::terminate();source->adopt_attrition_plot_list_without_callbacks(q.revision);
}
void BnwAttritionPlotListDirtyLoan::require_return_without_callbacks(const ui::BnwClientSelectionControl&selection,const ui::SelectedBnwPreviewHost&host,
 const LiveSessionState&state,std::uint64_t revision,std::uint64_t next)const{
 require_common_without_callbacks();const auto&q=*impl_;const auto actual=q.selection.lock();const auto prefix=q.prefix.lock();
 need(q.entered&&q.returned&&actual&&actual.get()==&selection&&prefix&&prefix->impl_->host.lock().get()==&host&&prefix->impl_->world.lock().get()==&state&&
  q.input.unit.client_revision<UINT64_MAX&&q.input.unit.next_occurrence<UINT64_MAX&&revision==q.input.unit.client_revision+1&&next==q.input.unit.next_occurrence+1&&
  selection.attrition_plot_list_state_matches_without_callbacks(q.input,q.event,true),"UnitInfo real flag/journal return differs; not event/frame ACK");
}
std::shared_ptr<const BnwAttritionPlotListDirtyLoan> BnwNativeAttritionPrefix::prepare_plot_list_owned(const BnwNativeUnitAllocatorCore&core,
 const ui::SelectedBnwPreviewHost&host,const LiveSessionState&state,const BnwSelectedStockLuaSourceOwner&source,const void*control,
 std::uint64_t generation,std::uint64_t serial,const BnwRetainedTurnCursor&cursor,const std::shared_ptr<ui::BnwClientSelectionControl>&selection)const{
 const auto r=impl_;r->thread();if(r->plot_list){r->plot_list->fail_without_callbacks();need(false,"UnitInfo Core-issued child replay");}
 const auto previous=cursor.attrition_plot_list_previous_serial_without_callbacks();
 need(previous&&previous<UINT64_MAX&&serial==previous+1&&selection,"UnitInfo requires exact genuine next parent operation");
 require_plot_list_parent_without_callbacks(core,host,state,source,control,generation,previous);
 auto q=std::make_unique<BnwAttritionPlotListDirtyLoan::Impl>();q->prefix=r->self;q->cursor=cursor.retain_engine_factory_owner();
 cursor.require_attrition_city_info_facade_without_callbacks();q->selection=selection;q->engine=r->damage->engine;
 require_selection_plot_parent_without_callbacks(core,host,state,source,control,generation,r->selection_plot->impl_->previous);
 q->input=selection->capture_attrition_unit_info_without_callbacks();q->previous=previous;q->serial=serial;q->revision=state.ui_source_revision;
 q->generation=generation;q->owner=r->actor;q->unit=r->damage->id;q->from_city=bool(r->city_info);
 need(q->input.unit.client_revision<UINT64_MAX&&q->input.unit.next_occurrence>0&&q->input.unit.next_occurrence<UINT64_MAX&&
  q->input.unit.events.size()<q->input.unit.events.max_size(),"UnitInfo actual client/ordinal/journal capacity exhausted");
 // Zero move_occurrence is the existing canonical non-move setter journal
 // sentinel (center/best-land); this direct native setter issues no MovePending.
 q->event={q->input.unit.next_occurrence,0,generation,q->revision,q->input.unit.client_revision+1,q->owner,q->unit,ui::BnwClientUnitDirtyKind::plot_list_buttons,q->input.unit.plot_list_buttons};
 require_plot_list_parent_without_callbacks(core,host,state,source,control,generation,previous);
 need(selection->attrition_plot_list_state_matches_without_callbacks(q->input,q->event,false),"UnitInfo actual input changed during preparation");
 r->plot_list=std::shared_ptr<BnwAttritionPlotListDirtyLoan>(new BnwAttritionPlotListDirtyLoan(std::move(q)));
 r->plot_list->require_common_without_callbacks();return r->plot_list;
}
