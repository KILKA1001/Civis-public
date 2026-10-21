namespace civ5::runtime {
std::uint64_t BnwRetainedTurnCursor::attrition_dll_wrapper_previous_serial_without_callbacks()const{
 const auto h=impl_->history;h->thread_check();need(bool(h->attrition_selection_plot_return),"Damage wrapper genuine plot query return absent");
 return h->attrition_plot_list_return?h->attrition_plot_list_return->serial:h->attrition_selection_plot_return->serial;
}
void BnwRetainedTurnCursor::require_attrition_dll_wrapper_entry_without_callbacks(const BnwNativeUnitAllocatorCore&core,const BnwSelectedStockLuaSourceOwner&source)const{
 const auto h=impl_->history;h->thread_check();if(h->attrition_dll_wrapper_attempted){fail_attrition_dll_wrapper_without_callbacks();need(false,"Damage wrapper caught actual once reentry");}
 need(!h->attrition_dll_wrapper_return&&!h->attrition_dll_wrapper_loan&&h->frontier.load()==BnwRetainedTurnFrontier::game_turn_player_set_damage_gameplay_damage_owner,
  "Damage wrapper original mandatory graphics frontier absent");source.require_attrition_dll_wrapper_parent_without_callbacks(*this,core,nullptr,true);
}
void BnwRetainedTurnCursor::require_attrition_dll_wrapper_parent_return_without_callbacks(const BnwNativeUnitAllocatorCore&core,
 const ui::SelectedBnwPreviewHost&host,const LiveSessionState&state,const BnwSelectedStockLuaSourceOwner&source,const void*control,std::uint64_t generation,std::uint64_t previous)const{
 const auto h=impl_->history;h->thread_check();need(!h->retired.load()&&!h->poisoned.load()&&h->attrition_first&&h->attrition_selection_plot_return&&h->attrition_selection_plot_loan,
  "Damage wrapper actual native branch parent missing/retired");const auto&q=*h->attrition_selection_plot_return;
 const auto verify=[&](const auto&p){const auto issuer=p.issuer.lock();const auto world=p.world.lock();
  need(p.source_adopted&&p.serial==previous&&p.generation==generation&&issuer&&issuer.get()==&source&&world&&world.get()==&state&&
   !p.issuer.owner_before(source.action_self_)&&!source.action_self_.owner_before(p.issuer)&&p.revision==state.ui_source_revision&&p.operation<UINT64_MAX&&h->operation==p.operation+1,
   "Damage wrapper exact original source/world/operation parent differs");};
 need(q.query_returned&&q.unit_plot_returned&&q.source_adopted,"Damage wrapper native plot/index query unreturned");
 if(q.plot_list_required){need(h->attrition_plot_list_return&&h->attrition_plot_list_loan,"Damage wrapper index-match cannot skip native PlotList5");
  const auto&p=*h->attrition_plot_list_return;need(p.setter_returned&&p.plot_list_returned,"Damage wrapper native PlotList setter unreturned");verify(p);
 }else{need(!h->attrition_plot_list_attempted&&!h->attrition_plot_list_return&&!h->attrition_plot_list_loan,"Damage wrapper cannot fabricate nonmatch PlotList5");verify(q);}
 need(h->frontier.load()==BnwRetainedTurnFrontier::game_turn_player_set_damage_gameplay_damage_owner,"Damage wrapper exact mandatory native call frontier differs");
 h->attrition_first->require_damage_wrapper_parent_without_callbacks(core,host,state,source,control,generation,previous);
}
void BnwRetainedTurnCursor::require_attrition_dll_wrapper_operation_without_callbacks(std::uint64_t serial,std::uint64_t revision,bool adopted)const{
 const auto h=impl_->history;h->thread_check();need(!h->retired.load()&&!h->poisoned.load()&&h->attrition_dll_wrapper_attempted&&h->attrition_selection_plot_return,
  "Damage command exact original query parent missing/retired");
 const auto previous=h->attrition_plot_list_return?h->attrition_plot_list_return->serial:h->attrition_selection_plot_return->serial;
 const auto operation=h->attrition_plot_list_return?h->attrition_plot_list_return->operation:h->attrition_selection_plot_return->operation;
 const auto world_revision=h->attrition_plot_list_return?h->attrition_plot_list_return->revision:h->attrition_selection_plot_return->revision;
 need(previous<UINT64_MAX&&serial==previous+1&&operation<=UINT64_MAX-(adopted?2U:1U)&&h->operation==operation+1+(adopted?1U:0U)&&
  revision==world_revision&&h->frontier.load()==(adopted?BnwRetainedTurnFrontier::game_turn_player_set_damage_popup_owner:
   BnwRetainedTurnFrontier::game_turn_player_set_damage_gameplay_damage_owner),"Damage command exact original/next operation and frontier differ");
 if(h->attrition_dll_wrapper_return){const auto&r=*h->attrition_dll_wrapper_return;
  need(r.previous==previous&&r.serial==serial&&r.operation==operation+1&&r.revision==revision&&r.generation==h->generation&&
   r.source_adopted==adopted&&r.call_returned==adopted,"Damage command exact prepared/returned occurrence differs");
 }else need(!adopted,"Damage command missing prepared return cannot be adopted");
}
void BnwRetainedTurnCursor::record_attrition_dll_wrapper_return_without_callbacks(const BnwAttritionDamageDllWrapperPrefix&loan,bool final_initialized)noexcept{
 const auto h=impl_->history;
 if(h->thread!=std::this_thread::get_id()||h->retired.load()||h->poisoned.load()||!h->attrition_dll_wrapper_return||
  h->attrition_dll_wrapper_loan.get()!=&loan||h->attrition_dll_wrapper_return->source_adopted||h->operation==UINT64_MAX)std::terminate();
 auto&r=*h->attrition_dll_wrapper_return;r.final_initialized=final_initialized;r.call_returned=true;r.source_adopted=true;
 ++h->operation;h->frontier.store(BnwRetainedTurnFrontier::game_turn_player_set_damage_popup_owner);
}
void BnwRetainedTurnCursor::require_attrition_dll_wrapper_return_without_callbacks(const BnwNativeUnitAllocatorCore&core,const BnwSelectedStockLuaSourceOwner&source)const{
 const auto h=impl_->history;h->thread_check();need(h->attrition_dll_wrapper_return&&h->attrition_dll_wrapper_loan&&h->attrition_first,"Damage command actual call-return child absent");
 const auto&r=*h->attrition_dll_wrapper_return;const auto issuer=r.issuer.lock();const auto world=r.world.lock();
 need(issuer&&issuer.get()==&source&&!r.issuer.owner_before(source.action_self_)&&!source.action_self_.owner_before(r.issuer)&&world&&
  world==impl_->session->state_&&r.generation==h->generation&&r.call_returned&&r.source_adopted,
  "Damage command SAME Source/world/returned call differs");
 require_attrition_dll_wrapper_operation_without_callbacks(r.serial,r.revision,true);
 h->attrition_dll_wrapper_loan->require_core_without_callbacks(core);
 h->attrition_dll_wrapper_loan->require_call_return_without_callbacks();
}
void BnwRetainedTurnCursor::execute_attrition_dll_wrapper_until_popup(const BnwNativeUnitAllocatorCore&core,const BnwSelectedStockLuaSourceOwner&source,
 const BnwStockLuaAttemptLease&lease,const std::shared_ptr<ui::BnwClientSceneFrame>&scene,const std::shared_ptr<const ui::BnwOwnedCameraGuiScreen>&gui){
 const auto i=impl_;auto&h=*i->history;h.thread_check();need(!h.attrition_dll_wrapper_attempted&&!h.attrition_dll_wrapper_return&&!h.attrition_dll_wrapper_loan&&h.attrition_first&&h.operation<UINT64_MAX,
  "Damage command actual once prepared operation absent");h.attrition_dll_wrapper_attempted=true;
 try{
  i->check_engine();require_attrition_city_info_facade_without_callbacks();const auto serial=lease.operation_serial();lease.require_operation(serial);require_attrition_city_info_facade_without_callbacks();
  source.require_attrition_dll_wrapper_parent_without_callbacks(*this,core,&lease,false);
  const auto world=i->session->state_;History::AttritionDllWrapperReturn staged;staged.issuer=source.action_self_;staged.world=world;
  staged.generation=h.generation;staged.serial=serial;staged.previous=attrition_dll_wrapper_previous_serial_without_callbacks();staged.operation=h.operation;staged.revision=world->ui_source_revision;
  h.attrition_dll_wrapper_loan=source.issue_attrition_dll_wrapper_owned(*this,core,lease,*h.attrition_first);
  require_attrition_city_info_facade_without_callbacks();source.require_attrition_dll_wrapper_parent_without_callbacks(*this,core,&lease,false);
  static_assert(std::is_nothrow_move_constructible_v<History::AttritionDllWrapperReturn>);h.attrition_dll_wrapper_return.emplace(std::move(staged));
  h.attrition_dll_wrapper_loan->execute_owned_command_until_popup(lease,scene,gui);
  require_attrition_city_info_facade_without_callbacks();require_attrition_dll_wrapper_return_without_callbacks(core,source);
 }catch(...){fail_attrition_dll_wrapper_without_callbacks();throw;}
}
void BnwRetainedTurnCursor::fail_attrition_dll_wrapper_without_callbacks()const noexcept{
 const auto h=impl_->history;if(h->attrition_dll_wrapper_loan)h->attrition_dll_wrapper_loan->fail_without_callbacks();h->quarantine();
}
}
