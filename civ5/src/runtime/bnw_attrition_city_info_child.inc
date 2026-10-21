// Included after genuine CityStrength3, in the existing runtime Core TU.
struct BnwAttritionCityInfoDirtyLoan::Impl {
 const std::thread::id creator=std::this_thread::get_id();
 std::weak_ptr<const BnwNativeAttritionPrefix>prefix;
 std::weak_ptr<BnwRetainedTurnCursor>cursor;
 std::weak_ptr<ui::BnwClientSelectionControl>selection;
 std::weak_ptr<ui::BnwEngineUiTurnOwner>engine;
 ui::BnwClientSelectionControl::AttritionCityInfoState input;
 std::uint64_t previous=0,serial=0,revision=0,generation=0;
 std::int32_t unit_owner=-1,unit=-1,city_owner=-1,city=-1;
 bool entered=false,returned=false,failed=false;
};
BnwAttritionCityInfoDirtyLoan::BnwAttritionCityInfoDirtyLoan(std::unique_ptr<Impl> p):impl_(std::move(p)){}
BnwAttritionCityInfoDirtyLoan::~BnwAttritionCityInfoDirtyLoan()=default;
void BnwAttritionCityInfoDirtyLoan::require_alive_without_callbacks()const{
 need(impl_&&impl_->creator==std::this_thread::get_id(),"Attrition CityInfo immutable foreign creator");
 const auto cursor=impl_->cursor.lock();need(bool(cursor),"Attrition CityInfo genuine Cursor expired");
 cursor->require_attrition_city_info_facade_without_callbacks();
 need(!impl_->failed,"Attrition CityInfo sticky failed child");
}
void BnwAttritionCityInfoDirtyLoan::fail_without_callbacks()const noexcept{
 if(!impl_||impl_->creator!=std::this_thread::get_id())std::terminate();impl_->failed=true;
 const auto prefix=impl_->prefix.lock();if(prefix)prefix->impl_->failed=true;
}
void BnwAttritionCityInfoDirtyLoan::require_common_without_callbacks()const{
 require_alive_without_callbacks();const auto& q=*impl_;const auto prefix=q.prefix.lock();const auto cursor=q.cursor.lock();
 need(prefix&&cursor,"Attrition CityInfo real Prefix/Cursor expired");const auto r=prefix->impl_;r->thread();
 const auto core=r->core.lock();const auto source=r->source.lock();const auto host=r->host.lock();const auto world=r->world.lock();const auto scope=r->control.lock();
 need(core&&source&&host&&world&&scope&&r->city_info.get()==this&&r->city_strength&&r->damage&&
  q.previous==r->city_strength->serial&&q.previous<UINT64_MAX&&q.serial==q.previous+1&&q.revision==r->city_strength->after&&
  q.generation==r->generation&&world->ui_source_revision==q.revision&&r->damage->changed&&r->damage->localization_returned&&
  !q.engine.owner_before(r->damage->engine)&&!r->damage->engine.owner_before(q.engine),
  "Attrition CityInfo genuine original HP/localized Engine/strength/current child lineage differs");
 // Validates the real unchanged native64 rows, after-HP archive/plain nodes,
 // same city receiver, registered strength archive/cache and generation.
 prefix->require_city_strength_without_callbacks(*core,*host,*world,*source,scope.get(),q.generation,q.previous,true);
 source->require_attrition_city_info_operation_without_callbacks(*cursor,nullptr,q.serial,q.revision,q.returned);
 const auto city=r->city_strength->owner->facts_without_callbacks();
 need(city.executed&&!city.failed&&city.owner==std::optional{q.city_owner}&&city.city==std::optional{q.city}&&
  city.unit==std::optional{q.unit}&&city.observed_damage==std::optional{r->damage->new_damage}&&q.unit_owner==r->actor,
  "Attrition CityInfo actual full city/unit IDs and after-HP node differ");
}
void BnwAttritionCityInfoDirtyLoan::enter_engine_once_without_callbacks(const ui::BnwEngineUiTurnOwner& engine,const BnwStockLuaAttemptLease& lease)const{
 require_alive_without_callbacks();auto& q=*impl_;
 if(q.entered||q.returned){fail_without_callbacks();need(false,"Attrition CityInfo caught/repeated Engine entry");}
 q.entered=true;require_engine_without_callbacks(engine,lease);
}
void BnwAttritionCityInfoDirtyLoan::require_engine_without_callbacks(const ui::BnwEngineUiTurnOwner& engine,const BnwStockLuaAttemptLease& lease)const{
 require_common_without_callbacks();const auto& q=*impl_;const auto actual=q.engine.lock();
 need(q.entered&&actual&&actual.get()==&engine,"Attrition CityInfo wrong/expired original Engine");lease.require_operation(q.serial);
 require_alive_without_callbacks();require_common_without_callbacks();
}
void BnwAttritionCityInfoDirtyLoan::require_setter_source(const ui::BnwClientSelectionControl& selection,
 const ui::SelectedBnwPreviewHost& host,const LiveSessionState& state,const BnwStockLuaAttemptLease& lease)const{
 require_common_without_callbacks();const auto& q=*impl_;const auto input=q.selection.lock();const auto prefix=q.prefix.lock();
 need(q.entered&&!q.returned&&input&&input.get()==&selection&&prefix&&prefix->impl_->host.lock().get()==&host&&
  prefix->impl_->world.lock().get()==&state&&selection.attrition_city_info_state_matches_without_callbacks(q.input,false),
  "Attrition CityInfo SAME canonical Selection/Host/world/phase differs");lease.require_operation(q.serial);require_alive_without_callbacks();require_common_without_callbacks();
 need(selection.attrition_city_info_state_matches_without_callbacks(q.input,false),"Attrition CityInfo state changed during source callback");
}
BnwAttritionCityInfoDirtyLoan::Operands BnwAttritionCityInfoDirtyLoan::operands_without_callbacks()const{
 require_common_without_callbacks();const auto& q=*impl_;return {q.generation,q.revision,q.serial,q.input.revision,q.input.city_count,q.unit_owner,q.unit,q.city_owner,q.city};
}
void BnwAttritionCityInfoDirtyLoan::require_final_setter_without_callbacks(const ui::BnwClientSelectionControl& selection,
 const ui::SelectedBnwPreviewHost&host,const LiveSessionState&state,const BnwStockLuaAttemptLease&lease,std::uint64_t revision,std::uint64_t count)const{
 require_lease_without_callbacks(lease);const auto& q=*impl_;const auto actual=q.selection.lock();const auto prefix=q.prefix.lock();
 need(q.entered&&!q.returned&&actual&&actual.get()==&selection&&prefix&&prefix->impl_->host.lock().get()==&host&&prefix->impl_->world.lock().get()==&state&&selection.attrition_city_info_state_matches_without_callbacks(q.input,false),"Attrition CityInfo final canonical receiver differs");
 need(q.input.revision==revision&&q.input.city_count==count&&revision<UINT64_MAX&&count<UINT64_MAX,
  "Attrition CityInfo final exact scalar operands/capacity differ");
}
void BnwAttritionCityInfoDirtyLoan::record_setter_and_adopt_without_callbacks(const ui::BnwClientSelectionControl& selection,
 std::uint64_t revision,std::uint64_t count)const noexcept{
 // All storage and parent admission were prepared before physical setDirty.
 // No allocation, provider, callback, or fallible validator in this tail.
 if(!impl_||impl_->creator!=std::this_thread::get_id())std::terminate();auto& q=*impl_;
 const auto actual=q.selection.lock();const auto prefix=q.prefix.lock();const auto cursor=q.cursor.lock();
 if(q.failed||!q.entered||q.returned||!actual||actual.get()!=&selection||!prefix||!cursor||
  q.input.revision==UINT64_MAX||q.input.city_count==UINT64_MAX||revision!=q.input.revision+1||count!=q.input.city_count+1)std::terminate();
 q.returned=true;
 cursor->record_attrition_city_info_return_without_callbacks(*this,revision,count);
 const auto source=prefix->impl_->source.lock();if(!source)std::terminate();
 source->adopt_attrition_city_info_without_callbacks(q.revision);
}
void BnwAttritionCityInfoDirtyLoan::require_return_without_callbacks(const ui::BnwClientSelectionControl& selection,
 const ui::SelectedBnwPreviewHost&host,const LiveSessionState&state,std::uint64_t revision,std::uint64_t count)const{
 require_common_without_callbacks();const auto& q=*impl_;const auto actual=q.selection.lock();const auto prefix=q.prefix.lock();
 need(q.entered&&q.returned&&actual&&actual.get()==&selection&&prefix&&prefix->impl_->host.lock().get()==&host&&
  prefix->impl_->world.lock().get()==&state&&q.input.revision<UINT64_MAX&&q.input.city_count<UINT64_MAX&&
  revision==q.input.revision+1&&count==q.input.city_count+1&&selection.attrition_city_info_state_matches_without_callbacks(q.input,true),
  "Attrition CityInfo physical return/client occurrence differs (not frame delivery)");
}
std::shared_ptr<const BnwAttritionCityInfoDirtyLoan> BnwNativeAttritionPrefix::prepare_city_info_owned(
 const BnwNativeUnitAllocatorCore& core,const ui::SelectedBnwPreviewHost&host,const LiveSessionState&state,
 const BnwSelectedStockLuaSourceOwner&source,const void*control,std::uint64_t generation,std::uint64_t serial,
 const BnwRetainedTurnCursor& cursor,const std::shared_ptr<ui::BnwClientSelectionControl>&selection)const{
 const auto r=impl_;r->thread();
 if(r->city_info){r->city_info->fail_without_callbacks();need(false,"Attrition CityInfo actual Core child replay");}
 need(r->city_strength&&r->damage&&selection&&r->city_strength->serial<UINT64_MAX&&serial==r->city_strength->serial+1,
  "Attrition CityInfo requires genuine next CityStrength3 operation");
 require_city_strength_without_callbacks(core,host,state,source,control,generation,r->city_strength->serial,true);
 auto prepared=std::make_unique<BnwAttritionCityInfoDirtyLoan::Impl>();
 prepared->prefix=r->self;prepared->cursor=cursor.retain_engine_factory_owner();cursor.require_attrition_city_info_facade_without_callbacks();prepared->selection=selection;prepared->engine=r->damage->engine;
 prepared->input=selection->capture_attrition_city_info_without_callbacks();
 prepared->previous=r->city_strength->serial;prepared->serial=serial;prepared->revision=state.ui_source_revision;prepared->generation=generation;
 const auto facts=r->city_strength->owner->facts_without_callbacks();
 need(facts.owner&&facts.city&&facts.unit&&prepared->input.revision<UINT64_MAX&&prepared->input.city_count<UINT64_MAX,
  "Attrition CityInfo real city/unit/counter operands unavailable");
 prepared->unit_owner=r->actor;prepared->unit=*facts.unit;prepared->city_owner=*facts.owner;prepared->city=*facts.city;
 require_city_strength_without_callbacks(core,host,state,source,control,generation,prepared->previous,true);
 need(selection->attrition_city_info_state_matches_without_callbacks(prepared->input,false),"Attrition CityInfo input changed during Core preparation");
 r->city_info=std::shared_ptr<BnwAttritionCityInfoDirtyLoan>(new BnwAttritionCityInfoDirtyLoan(std::move(prepared)));
 r->city_info->require_common_without_callbacks();return r->city_info;
}

void BnwAttritionCityInfoDirtyLoan::require_core_without_callbacks(const BnwNativeUnitAllocatorCore&core)const{
 require_common_without_callbacks();const auto prefix=impl_->prefix.lock();
 need(prefix&&prefix->impl_->core.lock().get()==&core,"CityInfo return supplied foreign Core");
}
void BnwAttritionCityInfoDirtyLoan::require_lease_without_callbacks(const BnwStockLuaAttemptLease&lease)const{
 require_common_without_callbacks();const auto&q=*impl_;const auto prefix=q.prefix.lock();const auto cursor=q.cursor.lock();
 need(prefix&&cursor,"Attrition CityInfo pure lease owners expired");const auto source=prefix->impl_->source.lock();
 need(bool(source),"Attrition CityInfo pure lease Source expired");
 source->require_attrition_city_info_operation_without_callbacks(*cursor,&lease,q.serial,q.revision,q.returned);
}
