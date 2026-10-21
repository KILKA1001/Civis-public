// Included after PlotList1 in the existing Core TU (runtime namespace).
struct BnwAttritionDamageDllWrapperPrefix::Impl {
 const std::thread::id creator=std::this_thread::get_id();
 std::weak_ptr<const BnwNativeAttritionPrefix>prefix;std::weak_ptr<BnwRetainedTurnCursor>cursor;
 std::uint64_t previous=0,serial=0,revision=0,generation=0;
 const saves::SavedUnitObservation*unit=nullptr;
 std::int32_t owner=-1,id=-1,old_damage=0,new_damage=0;
 bool entered=false,ctor_returned=false,destroy_called=false,decrement_called=false,destructor_returned=false,free_returned=false,failed=false;
 const void*allocation=nullptr;std::uint32_t initial_references=0,cleanup_references=0;
 // These are the only wrapper operations reached before the missing call.
 // QueryInterface/IncrementReference were not called and are not granted.
 struct Wrapper {
  Impl*receipt;const saves::SavedUnitObservation*native_unit;
  std::uint32_t references;
  Wrapper(Impl&out,const saves::SavedUnitObservation&unit)noexcept:receipt(&out),native_unit(&unit),references(1){
   out.allocation=this;out.initial_references=references;
  }
  ~Wrapper()noexcept{receipt->destructor_returned=true;} // SDK body is empty.
  std::uint32_t decrement_reference()noexcept{
   auto&out=*receipt;
   if(out.creator!=std::this_thread::get_id()||out.allocation!=this||!out.ctor_returned||out.decrement_called||native_unit!=out.unit||references!=1)std::terminate();
   out.decrement_called=true;out.cleanup_references=references;
   // No receiver has borrowed/QueryInterfaced the wrapper. Only the proved
   // original count==1 branch is reached: do NOT store refcount=0 first.
   delete this;out.free_returned=true;return 0;
  }
  void destroy()noexcept{
   if(receipt->destroy_called)std::terminate();receipt->destroy_called=true;(void)decrement_reference();
  }
  void interface_delete()noexcept{destroy();}
 };
 struct Release {
  void operator()(Wrapper*wrapper)const noexcept{
   if(!wrapper)return;auto&out=*wrapper->receipt;
   if(out.creator!=std::this_thread::get_id()||out.allocation!=wrapper||!out.ctor_returned||out.destroy_called||
    wrapper->native_unit!=out.unit||wrapper->references!=1)std::terminate();
   // Original interface delete -> Destroy -> DecrementReference(1) -> delete
   // wrapper -> Free. No underlying CvUnit reference counter is modified.
   wrapper->interface_delete();
  }

 };
 std::unique_ptr<Wrapper,Release>wrapper;
 std::shared_ptr<graphics::BnwUnitDamageCommandOwner>queue;
 std::shared_ptr<const graphics::BnwUnitDamageCommandOwner::Receipt>command;
 std::int32_t read_owner=-1,read_id=-1,read_max_hp=0,second_id=-1,second_owner=-1;
 bool final_read=false,final_initialized=false,getters_returned=false,game_released=false,call_returned=false,source_adopted=false;
};
BnwAttritionDamageDllWrapperPrefix::BnwAttritionDamageDllWrapperPrefix(std::unique_ptr<Impl>p):impl_(std::move(p)){}
BnwAttritionDamageDllWrapperPrefix::~BnwAttritionDamageDllWrapperPrefix()=default;
void BnwAttritionDamageDllWrapperPrefix::require_alive_without_callbacks()const{
 need(impl_&&impl_->creator==std::this_thread::get_id(),"Damage wrapper foreign immutable creator");const auto cursor=impl_->cursor.lock();
 need(bool(cursor),"Damage wrapper actual Cursor expired");cursor->require_attrition_city_info_facade_without_callbacks();
 need(!impl_->failed,"Damage wrapper sticky failed prefix");
}
void BnwAttritionDamageDllWrapperPrefix::fail_without_callbacks()const noexcept{
 if(!impl_||impl_->creator!=std::this_thread::get_id())std::terminate();cleanup_wrapper_without_callbacks();impl_->failed=true;
 const auto prefix=impl_->prefix.lock();if(prefix)prefix->impl_->failed=true;
}
void BnwNativeAttritionPrefix::require_damage_wrapper_parent_without_callbacks(const BnwNativeUnitAllocatorCore&core,
 const ui::SelectedBnwPreviewHost&host,const LiveSessionState&state,const BnwSelectedStockLuaSourceOwner&source,const void*control,
 std::uint64_t generation,std::uint64_t previous)const{
 const auto r=impl_;r->thread();need(r->selection_plot&&r->damage,"Damage wrapper genuine native plot/damage parent absent");
 r->selection_plot->require_alive_without_callbacks();const auto&plot=*r->selection_plot->impl_;
 if(r->plot_list){r->plot_list->require_alive_without_callbacks();const auto&dirty=*r->plot_list->impl_;const auto issuer=dirty.prefix.lock();
  need(!dirty.failed&&dirty.entered&&dirty.returned&&dirty.serial==previous&&dirty.previous==plot.serial&&dirty.previous<UINT64_MAX&&
   dirty.serial==dirty.previous+1&&dirty.generation==generation&&dirty.revision==state.ui_source_revision&&issuer&&issuer.get()==this&&
   !dirty.prefix.owner_before(r->self)&&!r->self.owner_before(dirty.prefix)&&dirty.owner==r->actor&&dirty.unit==r->damage->id&&
   !dirty.engine.owner_before(r->damage->engine)&&!r->damage->engine.owner_before(dirty.engine),"Damage wrapper requires immutable actual returned PlotList5");
  require_plot_list_parent_without_callbacks(core,host,state,source,control,generation,dirty.previous);
  const auto input=dirty.selection.lock();need(input&&input->attrition_plot_list_state_matches_without_callbacks(dirty.input,dirty.event,true),
   "Damage wrapper exact earned PlotList5 input changed");
 }else{
  need(!plot.failed&&plot.entered&&plot.returned&&plot.serial==previous&&plot.unit_plot>=0&&(!plot.result||*plot.result!=plot.unit_plot),
   "Damage wrapper cannot skip mandatory PlotList5 on actual index-match");
  require_selection_plot_parent_without_callbacks(core,host,state,source,control,generation,plot.previous);
 }
}
void BnwAttritionDamageDllWrapperPrefix::require_common_without_callbacks()const{
 require_alive_without_callbacks();const auto&q=*impl_;const auto prefix=q.prefix.lock();const auto cursor=q.cursor.lock();need(prefix&&cursor,"Damage wrapper real Prefix/Cursor expired");
 const auto r=prefix->impl_;r->thread();const auto core=r->core.lock();const auto source=r->source.lock();const auto host=r->host.lock();const auto world=r->world.lock();const auto control=r->control.lock();
 need(core&&source&&host&&world&&control&&r->dll_wrapper.get()==this&&q.previous<UINT64_MAX&&q.serial==q.previous+1&&q.generation==r->generation&&
  q.revision==world->ui_source_revision&&r->damage&&r->damage->executed&&!r->damage->failed&&r->damage->changed&&q.owner==r->actor&&q.id==r->damage->id&&
  q.old_damage==r->damage->old_damage&&q.new_damage==r->damage->new_damage&&q.unit==native_imported_get_at(*world,q.owner,q.id),
  "Damage wrapper SAME actual after-HP receiver/old-new damage/native source/operation differs");
 prefix->require_damage_wrapper_parent_without_callbacks(*core,*host,*world,*source,control.get(),q.generation,q.previous);
 source->require_attrition_dll_wrapper_operation_without_callbacks(*cursor,nullptr,q.serial,q.revision,q.source_adopted);
}
void BnwAttritionDamageDllWrapperPrefix::require_lease_without_callbacks(const BnwStockLuaAttemptLease&lease)const{
 require_common_without_callbacks();const auto prefix=impl_->prefix.lock();const auto cursor=impl_->cursor.lock();const auto source=prefix->impl_->source.lock();
 need(source&&cursor,"Damage wrapper actual lease Source/Cursor expired");source->require_attrition_dll_wrapper_operation_without_callbacks(*cursor,&lease,impl_->serial,impl_->revision,impl_->source_adopted);
}
void BnwAttritionDamageDllWrapperPrefix::cleanup_wrapper_without_callbacks()const noexcept{
 if(!impl_||impl_->creator!=std::this_thread::get_id())std::terminate();impl_->wrapper.reset();
}
void BnwAttritionDamageDllWrapperPrefix::require_command_binding_without_callbacks(const graphics::BnwUnitDamageCommandOwner&queue)const{
 require_common_without_callbacks();const auto&q=*impl_;const auto r=q.prefix.lock()->impl_;const auto&b=queue.binding_;
 const auto same=[](const auto&a,const auto&b){return !a.owner_before(b)&&!b.owner_before(a);};
 need(q.queue.get()==&queue&&b.generation==q.generation&&same(b.host,r->host)&&same(b.world,r->world)&&same(b.core,r->core)&&
  same(b.source,r->source)&&same(b.control,r->control),"Damage command SAME genuine Core/Source/control/world binding differs");
}
void BnwAttritionDamageDllWrapperPrefix::require_command_payload_without_callbacks(std::int32_t owner,std::int32_t id,std::int32_t damage,
 std::int32_t previous,std::int32_t max_hp,const void*unit,const void*wrapper,std::uint64_t serial,std::uint64_t revision)const{
 require_common_without_callbacks();const auto&q=*impl_;
 need(q.entered&&q.ctor_returned&&q.final_read&&q.final_initialized&&q.getters_returned&&!q.call_returned&&!q.source_adopted&&q.wrapper&&
  q.wrapper.get()==wrapper&&q.allocation==wrapper&&unit==q.unit&&q.wrapper->native_unit==q.unit&&q.wrapper->references==1&&
  q.read_owner==q.owner&&q.read_id==q.id&&q.second_owner==q.owner&&q.second_id==q.id&&owner==q.owner&&id==q.id&&damage==q.new_damage&&
  previous==q.old_damage&&max_hp==q.read_max_hp&&max_hp>0&&serial==q.serial&&revision==q.revision,
  "Damage command original getter/closed native payload/once operands differ");
}
void BnwAttritionDamageDllWrapperPrefix::require_core_without_callbacks(const BnwNativeUnitAllocatorCore&core)const{
 require_common_without_callbacks();const auto actual=impl_->prefix.lock()->impl_->core.lock();
 need(actual&&actual.get()==&core,"Damage command actual caller Core differs from retained issuer");
}
void BnwAttritionDamageDllWrapperPrefix::require_call_return_without_callbacks()const{
 require_common_without_callbacks();const auto&q=*impl_;
 need(q.entered&&q.ctor_returned&&q.final_read&&q.game_released&&q.call_returned&&q.source_adopted&&q.wrapper&&
  !q.destroy_called&&!q.free_returned&&q.wrapper.get()==q.allocation&&q.wrapper->references==1,
  "Damage command actual utility call-return/living wrapper scope absent");
 if(q.final_initialized){need(q.getters_returned&&q.queue&&q.command,"Damage command true branch must append");q.queue->require_return_without_callbacks(*q.command);}
 else need(!q.getters_returned&&!q.queue&&!q.command,"Damage command genuine false branch cannot invent unit reads/append");
}
void BnwAttritionDamageDllWrapperPrefix::execute_owned_command_until_popup(const BnwStockLuaAttemptLease&lease,
 const std::shared_ptr<ui::BnwClientSceneFrame>&scene,const std::shared_ptr<const ui::BnwOwnedCameraGuiScreen>&gui)const{
 require_alive_without_callbacks();auto&q=*impl_;if(q.entered){fail_without_callbacks();need(false,"Damage command caught constructor replay");}q.entered=true;
 try{
  lease.require_operation(q.serial);require_alive_without_callbacks();require_lease_without_callbacks(lease);
  q.wrapper.reset(new Impl::Wrapper(q,*q.unit));q.ctor_returned=true;
  need(q.allocation==q.wrapper.get()&&q.wrapper->native_unit==q.unit&&q.wrapper->references==1,"Damage command actual wrapper ctor differs");
  require_lease_without_callbacks(lease);const auto prefix=q.prefix.lock();const auto r=prefix->impl_;const auto host=r->host.lock();const auto cursor=q.cursor.lock();
  need(host&&cursor,"Damage command actual utility issuer expired");
  // Native utility GetGame/IsFinalInitialized precedes every unit getter.
  const auto context=BnwLiveGameContext::retain_for_actual_host(host);require_alive_without_callbacks();require_lease_without_callbacks(lease);
  const auto game=context->get_game();require_alive_without_callbacks();game->require_actual_binding(*host);
  q.final_initialized=game->is_final_initialized();q.final_read=true;require_alive_without_callbacks();require_lease_without_callbacks(lease);
  if(q.final_initialized){
   // Exactly native owner, ID, captured new/old operands, maxHP, ID, owner.
   q.read_owner=narrow(core(*q.wrapper->native_unit,"m_eOwner"));
   q.read_id=narrow(core(*q.wrapper->native_unit,"m_iID"));
   const auto damage=q.new_damage,previous=q.old_damage;
   q.read_max_hp=attrition_actual_max_hit_points(*host);
   q.second_id=narrow(core(*q.wrapper->native_unit,"m_iID"));
   q.second_owner=narrow(core(*q.wrapper->native_unit,"m_eOwner"));q.getters_returned=true;
   graphics::BnwUnitDamageCommandOwner::Binding binding;binding.scene=scene;binding.gui=gui;binding.host=r->host;binding.world=r->world;
   binding.core=r->core;binding.source=r->source;binding.control=r->control;binding.generation=q.generation;
   require_lease_without_callbacks(lease);q.queue=graphics::BnwUnitDamageCommandOwner::bind_owned(binding);
   require_alive_without_callbacks();require_lease_without_callbacks(lease);
   graphics::BnwUnitDamageCommandOwner::Payload payload;payload.owner=q.read_owner;payload.id=q.read_id;
   payload.new_damage=damage;payload.previous_damage=previous;payload.max_hit_points=q.read_max_hp;
   q.command=q.queue->append_owned(r->dll_wrapper,payload,q.unit,q.wrapper.get(),q.serial,q.revision);
   // No callback/old-frame validation between actual append return and adoption.
  }
  game->destroy();game->require_destroyed_for(*host);q.game_released=true;q.call_returned=true;
  cursor->record_attrition_dll_wrapper_return_without_callbacks(*this,q.final_initialized);
  const auto source=r->source.lock();if(!source)std::terminate();source->adopt_attrition_dll_wrapper_without_callbacks(q.revision);q.source_adopted=true;
  if(q.final_initialized)q.queue->record_source_adoption_without_callbacks(*q.command,*this);
  require_call_return_without_callbacks();
 }catch(...){cleanup_wrapper_without_callbacks();if(q.ctor_returned)require_earned_cleanup_without_callbacks();throw;}
}
void BnwAttritionDamageDllWrapperPrefix::require_earned_cleanup_without_callbacks()const{
 need(impl_&&impl_->creator==std::this_thread::get_id(),"Damage wrapper cleanup foreign creator");const auto&q=*impl_;
 need(q.entered&&q.ctor_returned&&q.allocation&&q.initial_references==1&&q.cleanup_references==1&&q.destroy_called&&q.decrement_called&&
  q.destructor_returned&&q.free_returned,"Damage wrapper exact reached Destroy/DecrementReference/delete/Free cleanup absent");
}
std::shared_ptr<const BnwAttritionDamageDllWrapperPrefix> BnwNativeAttritionPrefix::prepare_damage_wrapper_owned(const BnwNativeUnitAllocatorCore&core,
 const ui::SelectedBnwPreviewHost&host,const LiveSessionState&state,const BnwSelectedStockLuaSourceOwner&source,const void*control,
 std::uint64_t generation,std::uint64_t serial,const BnwRetainedTurnCursor&cursor)const{
 const auto r=impl_;r->thread();if(r->dll_wrapper){r->dll_wrapper->fail_without_callbacks();need(false,"Damage wrapper genuine Core issued prefix replay");}
 const auto previous=cursor.attrition_dll_wrapper_previous_serial_without_callbacks();need(previous&&previous<UINT64_MAX&&serial==previous+1,"Damage wrapper exact next operation absent");
 require_damage_wrapper_parent_without_callbacks(core,host,state,source,control,generation,previous);
 auto q=std::make_unique<BnwAttritionDamageDllWrapperPrefix::Impl>();q->prefix=r->self;q->cursor=cursor.retain_engine_factory_owner();
 cursor.require_attrition_city_info_facade_without_callbacks();q->previous=previous;q->serial=serial;q->revision=state.ui_source_revision;q->generation=generation;
 q->owner=r->actor;q->id=r->damage->id;q->unit=native_imported_get_at(state,q->owner,q->id);q->old_damage=r->damage->old_damage;q->new_damage=r->damage->new_damage;
 need(q->unit,"Damage wrapper original native unit pointer absent");require_damage_wrapper_parent_without_callbacks(core,host,state,source,control,generation,previous);
 r->dll_wrapper=std::shared_ptr<BnwAttritionDamageDllWrapperPrefix>(new BnwAttritionDamageDllWrapperPrefix(std::move(q)));
 r->dll_wrapper->require_common_without_callbacks();return r->dll_wrapper;
}
