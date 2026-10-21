// Included after conditional second UnitInfo1 in the existing Core TU (runtime namespace).
struct BnwAttritionSelectionPlotRead::Impl {
 const std::thread::id creator=std::this_thread::get_id();
 std::weak_ptr<const BnwNativeAttritionPrefix>prefix;std::weak_ptr<BnwRetainedTurnCursor>cursor;
 std::weak_ptr<ui::BnwClientSelectionControl>selection;std::weak_ptr<ui::BnwEngineUiTurnOwner>engine;
 std::uint64_t previous=0,serial=0,revision=0,generation=0;
 std::int32_t owner=-1,unit=-1;std::optional<std::int32_t>result;std::int32_t unit_plot=-1;bool entered=false,returned=false,failed=false;
};
BnwAttritionSelectionPlotRead::BnwAttritionSelectionPlotRead(std::unique_ptr<Impl>p):impl_(std::move(p)){}
BnwAttritionSelectionPlotRead::~BnwAttritionSelectionPlotRead()=default;
void BnwAttritionSelectionPlotRead::require_alive_without_callbacks()const{
 need(impl_&&impl_->creator==std::this_thread::get_id(),"IsSelected immutable foreign creator");
 const auto cursor=impl_->cursor.lock();need(bool(cursor),"IsSelected original Cursor expired");
 cursor->require_attrition_city_info_facade_without_callbacks();need(!impl_->failed,"IsSelected sticky failed child");
}
void BnwAttritionSelectionPlotRead::fail_without_callbacks()const noexcept{
 if(!impl_||impl_->creator!=std::this_thread::get_id())std::terminate();impl_->failed=true;
 const auto prefix=impl_->prefix.lock();if(prefix)prefix->impl_->failed=true;
}
void BnwNativeAttritionPrefix::require_selection_plot_parent_without_callbacks(const BnwNativeUnitAllocatorCore&core,
 const ui::SelectedBnwPreviewHost&host,const LiveSessionState&state,const BnwSelectedStockLuaSourceOwner&source,
 const void*control,std::uint64_t generation,std::uint64_t previous)const{
 const auto r=impl_;r->thread();need(r->unit_selected&&r->unit_info&&r->damage,"SelectionPlot genuine native query/first UnitInfo absent");
 r->unit_selected->require_alive_without_callbacks();const auto&read=*r->unit_selected->impl_;const auto read_prefix=read.prefix.lock();const auto self=r->self.lock();
 need(!read.failed&&read.entered&&read.returned&&read_prefix&&self&&read_prefix.get()==this&&self.get()==this&&
  !read.prefix.owner_before(r->self)&&!r->self.owner_before(read.prefix)&&read.generation==generation&&read.revision==state.ui_source_revision&&
  read.owner==r->actor&&read.unit==r->damage->id&&!read.engine.owner_before(r->damage->engine)&&!r->damage->engine.owner_before(read.engine),
  "SelectionPlot immutable actual native selected query/once/fullID/Engine differs");
 if(r->selected_unit_info){
  r->selected_unit_info->require_alive_without_callbacks();const auto&second=*r->selected_unit_info->impl_;const auto issuer=second.prefix.lock();
  need(read.result&&!second.failed&&second.entered&&second.returned&&second.serial==previous&&second.previous==read.serial&&second.previous<UINT64_MAX&&
   second.serial==second.previous+1&&second.generation==generation&&second.revision==read.revision&&issuer&&issuer.get()==this&&
   !second.prefix.owner_before(r->self)&&!r->self.owner_before(second.prefix)&&second.owner==r->actor&&second.unit==r->damage->id&&
   !second.selection.owner_before(read.selection)&&!read.selection.owner_before(second.selection)&&
   !second.engine.owner_before(r->damage->engine)&&!r->damage->engine.owner_before(second.engine),"SelectionPlot true path requires real returned SECOND UnitInfo");
  require_selected_unit_info_parent_without_callbacks(core,host,state,source,control,generation,second.previous);
  const auto input=second.selection.lock();need(input&&input->attrition_unit_info_state_matches_without_callbacks(second.input,second.event,true),
   "SelectionPlot exact earned SECOND UnitInfo input changed");
 }else{
  need(!read.result&&read.serial==previous,"SelectionPlot cannot skip mandatory second UnitInfo on true");
  require_unit_selected_parent_without_callbacks(core,host,state,source,control,generation,read.previous);
 }
}
std::pair<std::shared_ptr<ui::BnwClientSelectionControl>,std::optional<std::int32_t>> BnwNativeAttritionPrefix::selection_plot_input_without_callbacks()const{
 const auto r=impl_;r->thread();need(r->unit_selected&&r->unit_info,"SelectionPlot real predecessor absent");
 if(r->selected_unit_info)return {r->selected_unit_info->impl_->selection.lock(),r->selected_unit_info->impl_->input.selected_plot};
 return {r->unit_info->impl_->selection.lock(),r->unit_info->impl_->input.selected_plot};
}
void BnwAttritionSelectionPlotRead::require_common_without_callbacks()const{
 require_alive_without_callbacks();const auto&q=*impl_;const auto prefix=q.prefix.lock();const auto cursor=q.cursor.lock();
 need(prefix&&cursor,"IsSelected actual Prefix/Cursor expired");const auto r=prefix->impl_;r->thread();
 const auto core=r->core.lock();const auto source=r->source.lock();const auto host=r->host.lock();const auto world=r->world.lock();const auto control=r->control.lock();
 need(core&&source&&host&&world&&control&&r->selection_plot.get()==this&&q.previous<UINT64_MAX&&q.serial==q.previous+1&&
  q.generation==r->generation&&q.revision==world->ui_source_revision&&r->damage&&q.owner==r->actor&&q.unit==r->damage->id&&
  !q.engine.owner_before(r->damage->engine)&&!r->damage->engine.owner_before(q.engine),"IsSelected genuine current Core/world/fullID/exact operation differs");
 prefix->require_selection_plot_parent_without_callbacks(*core,*host,*world,*source,control.get(),q.generation,q.previous);
 const auto parent=prefix->selection_plot_input_without_callbacks();const auto current=q.selection.lock();
 need(parent.first&&current&&parent.first==current&&!parent.first.owner_before(current)&&!current.owner_before(parent.first),
  "SelectionPlot requires SAME canonical input from its actual native predecessor");
 source->require_attrition_selection_plot_operation_without_callbacks(*cursor,nullptr,q.serial,q.revision,q.returned);
 if(q.returned)need(q.result==parent.second&&q.unit_plot>=0,"SelectionPlot returned actual nullable input/native unit index differs");
}
void BnwAttritionSelectionPlotRead::require_core_without_callbacks(const BnwNativeUnitAllocatorCore&core)const{
 require_common_without_callbacks();const auto prefix=impl_->prefix.lock();need(prefix&&prefix->impl_->core.lock().get()==&core,"IsSelected foreign supplied Core");
}
void BnwAttritionSelectionPlotRead::require_lease_without_callbacks(const BnwStockLuaAttemptLease&lease)const{
 require_common_without_callbacks();const auto&q=*impl_;const auto prefix=q.prefix.lock();const auto cursor=q.cursor.lock();
 need(prefix&&cursor,"IsSelected lease real owner expired");const auto source=prefix->impl_->source.lock();need(bool(source),"IsSelected lease Source expired");
 source->require_attrition_selection_plot_operation_without_callbacks(*cursor,&lease,q.serial,q.revision,q.returned);
}
void BnwAttritionSelectionPlotRead::enter_engine_once_without_callbacks(const ui::BnwEngineUiTurnOwner&engine,const BnwStockLuaAttemptLease&lease)const{
 require_alive_without_callbacks();auto&q=*impl_;if(q.entered||q.returned){fail_without_callbacks();need(false,"IsSelected caught/repeated Engine reentry");}
 q.entered=true;require_engine_without_callbacks(engine,lease);
}
void BnwAttritionSelectionPlotRead::require_engine_without_callbacks(const ui::BnwEngineUiTurnOwner&engine,const BnwStockLuaAttemptLease&lease)const{
 require_common_without_callbacks();const auto&q=*impl_;const auto actual=q.engine.lock();need(q.entered&&actual&&actual.get()==&engine,"IsSelected wrong/expired original Engine");
 lease.require_operation(q.serial);require_alive_without_callbacks();require_lease_without_callbacks(lease);
}
void BnwAttritionSelectionPlotRead::require_query_source(const ui::BnwClientSelectionControl&selection,const ui::SelectedBnwPreviewHost&host,
 const LiveSessionState&state,const BnwStockLuaAttemptLease&lease)const{
 require_common_without_callbacks();const auto&q=*impl_;const auto input=q.selection.lock();const auto prefix=q.prefix.lock();
 need(q.entered&&!q.returned&&input&&input.get()==&selection&&prefix&&prefix->impl_->host.lock().get()==&host&&prefix->impl_->world.lock().get()==&state,
  "IsSelected canonical pre-read phase differs");lease.require_operation(q.serial);require_alive_without_callbacks();require_lease_without_callbacks(lease);
}
BnwAttritionSelectionPlotRead::Operands BnwAttritionSelectionPlotRead::operands_without_callbacks()const{
 require_common_without_callbacks();const auto&q=*impl_;return {q.generation,q.revision,q.serial,q.owner,q.unit};
}
std::int32_t BnwAttritionSelectionPlotRead::require_final_query_without_callbacks(const ui::BnwClientSelectionControl&selection,const ui::SelectedBnwPreviewHost&host,
 const LiveSessionState&state,const BnwStockLuaAttemptLease&lease,std::optional<std::int32_t>result)const{
 require_lease_without_callbacks(lease);const auto&q=*impl_;const auto input=q.selection.lock();const auto prefix=q.prefix.lock();
 need(q.entered&&!q.returned&&input&&input.get()==&selection&&prefix&&prefix->impl_->host.lock().get()==&host&&prefix->impl_->world.lock().get()==&state&&
  result==prefix->selection_plot_input_without_callbacks().second,"SelectionPlot final pure original nullable receiver differs");
 // Native order: actual nullable getSelectionPlot/GetPlotIndex has been read
 // by the canonical input owner BEFORE this unit.plot()->GetPlotIndex query.
 const auto*unit=native_imported_get_at(state,q.owner,q.unit);need(unit&&narrow(core(*unit,"m_iID"))==q.unit&&narrow(core(*unit,"m_eOwner"))==q.owner,
  "SelectionPlot native current full unit receiver differs");const auto x=narrow(core(*unit,"m_iX")),y=narrow(core(*unit,"m_iY"));
 const auto&grid=state.world.map_header.grid;
 need(x>=0&&y>=0&&static_cast<std::uint64_t>(x)<grid.width&&static_cast<std::uint64_t>(y)<grid.height,
  "SelectionPlot actual unit plotCheckInvalid receiver unavailable/outside current map");
 // SDK plotCheckInvalid does not wrap; GetPlotIndex uses y*width+x.
 const auto at=grid.index(x,y);need(at<=static_cast<std::uint64_t>(INT32_MAX),"SelectionPlot native unit index overflow");
 const auto&raw=native_path_plot(state,at);const auto&cell=state.world.map_projection->cells.at(at);
 need(raw.index==at&&cell.index==at&&raw.x==x&&raw.y==y&&cell.x==x&&cell.y==y,"SelectionPlot native unit raw/projected pointer/index/coordinates differ");
 return static_cast<std::int32_t>(at);
}
void BnwAttritionSelectionPlotRead::record_query_and_adopt_without_callbacks(const ui::BnwClientSelectionControl&selection,
 std::optional<std::int32_t>result,std::int32_t unit_plot)const noexcept{
 if(!impl_||impl_->creator!=std::this_thread::get_id())std::terminate();auto&q=*impl_;const auto input=q.selection.lock();
 const auto prefix=q.prefix.lock();const auto cursor=q.cursor.lock();
 if(q.failed||!q.entered||q.returned||!input||input.get()!=&selection||!prefix||!cursor||unit_plot<0||(result&&*result<0))std::terminate();
 q.result=result;q.unit_plot=unit_plot;q.returned=true;cursor->record_attrition_selection_plot_return_without_callbacks(*this,result,unit_plot);
 const auto source=prefix->impl_->source.lock();if(!source)std::terminate();source->adopt_attrition_selection_plot_without_callbacks(q.revision);
}
void BnwAttritionSelectionPlotRead::require_return_without_callbacks(const ui::BnwClientSelectionControl&selection,const ui::SelectedBnwPreviewHost&host,
 const LiveSessionState&state,std::optional<std::int32_t>result)const{
 require_common_without_callbacks();const auto&q=*impl_;const auto input=q.selection.lock();const auto prefix=q.prefix.lock();
 need(q.entered&&q.returned&&q.result==result&&q.unit_plot>=0&&input&&input.get()==&selection&&prefix&&prefix->impl_->host.lock().get()==&host&&
  prefix->impl_->world.lock().get()==&state,"SelectionPlot actual nullable returned query differs");
}
std::optional<std::int32_t> BnwAttritionSelectionPlotRead::result_without_callbacks()const{
 require_common_without_callbacks();need(impl_->entered&&impl_->returned,"SelectionPlot query has not returned");return impl_->result;
}
bool BnwAttritionSelectionPlotRead::plot_list_required_without_callbacks()const{
 require_common_without_callbacks();need(impl_->entered&&impl_->returned&&impl_->unit_plot>=0,"SelectionPlot native comparison has not returned");
 // SDK uses -1 only for actual NULL. Actual unit plot index is nonnegative.
 return impl_->result&&*impl_->result==impl_->unit_plot;
}
std::shared_ptr<const BnwAttritionSelectionPlotRead> BnwNativeAttritionPrefix::prepare_selection_plot_owned(const BnwNativeUnitAllocatorCore&core,
 const ui::SelectedBnwPreviewHost&host,const LiveSessionState&state,const BnwSelectedStockLuaSourceOwner&source,const void*control,
 std::uint64_t generation,std::uint64_t serial,const BnwRetainedTurnCursor&cursor,const std::shared_ptr<ui::BnwClientSelectionControl>&selection)const{
 const auto r=impl_;r->thread();if(r->selection_plot){r->selection_plot->fail_without_callbacks();need(false,"IsSelected Core-issued child replay");}
 const auto previous=cursor.attrition_selection_plot_previous_serial_without_callbacks();
 need(previous&&previous<UINT64_MAX&&serial==previous+1&&selection,"IsSelected exact genuine next operation absent");
 require_selection_plot_parent_without_callbacks(core,host,state,source,control,generation,previous);
 auto q=std::make_unique<BnwAttritionSelectionPlotRead::Impl>();q->prefix=r->self;q->cursor=cursor.retain_engine_factory_owner();
 cursor.require_attrition_city_info_facade_without_callbacks();q->selection=selection;q->engine=r->damage->engine;
 q->previous=previous;q->serial=serial;q->revision=state.ui_source_revision;q->generation=generation;q->owner=r->actor;q->unit=r->damage->id;
 require_selection_plot_parent_without_callbacks(core,host,state,source,control,generation,previous);
 r->selection_plot=std::shared_ptr<BnwAttritionSelectionPlotRead>(new BnwAttritionSelectionPlotRead(std::move(q)));
 r->selection_plot->require_common_without_callbacks();return r->selection_plot;
}
