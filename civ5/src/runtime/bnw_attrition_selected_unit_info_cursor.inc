namespace civ5::runtime {
std::uint64_t BnwRetainedTurnCursor::attrition_selected_unit_info_previous_serial_without_callbacks()const{
 const auto h=impl_->history;h->thread_check();need(bool(h->attrition_unit_selected_return),"Second UnitInfo native query return absent");return h->attrition_unit_selected_return->serial;
}
void BnwRetainedTurnCursor::require_attrition_selected_unit_info_entry_without_callbacks(const BnwNativeUnitAllocatorCore&core,const BnwSelectedStockLuaSourceOwner&source)const{
 const auto h=impl_->history;h->thread_check();
 if(h->attrition_selected_unit_info_attempted){if(h->attrition_selected_unit_info_loan)h->attrition_selected_unit_info_loan->fail_without_callbacks();h->quarantine();need(false,"UnitInfo actual once reentry");}
 need(!h->attrition_selected_unit_info_return&&!h->attrition_selected_unit_info_loan&&h->frontier.load()==BnwRetainedTurnFrontier::game_turn_player_set_damage_selected_info_dirty_owner,
  "UnitInfo requires original physical dirty frontier");source.require_attrition_selected_unit_info_parent_without_callbacks(*this,core,nullptr,true);
}
void BnwRetainedTurnCursor::execute_attrition_selected_unit_info_with_actual_stock(const BnwNativeUnitAllocatorCore&core,
 const BnwSelectedStockLuaSourceOwner&source,const BnwStockLuaAttemptLease&lease){
 const auto i=impl_;auto& h=*i->history;h.thread_check();
 need(!h.attrition_selected_unit_info_attempted&&!h.attrition_selected_unit_info_return&&h.attrition_first&&h.operation<UINT64_MAX,"UnitInfo actual prepared once entry absent");
 h.attrition_selected_unit_info_attempted=true;
 try{
  i->check_engine();require_attrition_city_info_facade_without_callbacks();const auto serial=lease.operation_serial();lease.require_operation(serial);require_attrition_city_info_facade_without_callbacks();
  source.require_attrition_selected_unit_info_parent_without_callbacks(*this,core,&lease,false);
  const auto world=i->session->state_;auto&state=*world;const auto actual_ui=i->cycle_ui.lock();
  need(actual_ui&&actual_ui->engine_ui_turn_owner_,"UnitInfo actual retained UI/Engine absent");
  const auto selection=require_actual_ui(*actual_ui);require_attrition_city_info_facade_without_callbacks();const auto engine=actual_ui->engine_ui_turn_owner_;
  ui::BnwEngineUiTurnOwner::EndTurnCursorPin pin(*engine,*actual_ui,state);
  History::AttritionSelectedUnitInfoReturn staged;staged.issuer=source.action_self_;staged.world=world;
  staged.generation=h.generation;staged.serial=serial;staged.previous=attrition_selected_unit_info_previous_serial_without_callbacks();
  staged.operation=h.operation;staged.revision=state.ui_source_revision;
  h.attrition_selected_unit_info_loan=source.issue_attrition_selected_unit_info_owned(*this,core,lease,*h.attrition_first,selection);
  i->check_engine();require_attrition_city_info_facade_without_callbacks();lease.require_operation(serial);require_attrition_city_info_facade_without_callbacks();
  const auto current_selection=require_actual_ui(*actual_ui);require_attrition_city_info_facade_without_callbacks();
  need(current_selection==selection&&actual_ui->engine_ui_turn_owner_==engine&&i->session->state_==world&&
   h.operation==staged.operation&&state.ui_source_revision==staged.revision,"UnitInfo actual caller receivers changed before setter");
  source.require_attrition_selected_unit_info_parent_without_callbacks(*this,core,&lease,false);
  static_assert(std::is_nothrow_move_constructible_v<History::AttritionSelectedUnitInfoReturn>);
  h.attrition_selected_unit_info_return.emplace(std::move(staged));
  ui::BnwEngineUiTurnOwner::consume_attrition_selected_unit_info_dirty_from_owned_weak(engine,*h.attrition_selected_unit_info_loan,state,lease);
  // Actual setter and operation adoption already finished before Engine POST.
  i->check_engine();require_attrition_city_info_facade_without_callbacks();lease.require_operation(serial);require_attrition_city_info_facade_without_callbacks();
  source.require_attrition_selected_unit_info_source_without_callbacks(*this,core,&lease,false);
 }catch(...){fail_attrition_selected_unit_info_without_callbacks();throw;}
}
void BnwRetainedTurnCursor::require_attrition_selected_unit_info_parent_return_without_callbacks(const BnwNativeUnitAllocatorCore&core,
 const ui::SelectedBnwPreviewHost&host,const LiveSessionState&state,const BnwSelectedStockLuaSourceOwner&source,const void*control,
 std::uint64_t generation,std::uint64_t previous)const{
 const auto h=impl_->history;h->thread_check();need(!h->retired.load()&&!h->poisoned.load()&&h->attrition_first&&h->attrition_unit_selected_return&&h->attrition_unit_selected_loan,
  "Second UnitInfo actual IsSelected parent absent/retired");const auto&p=*h->attrition_unit_selected_return;const auto issuer=p.issuer.lock();const auto world=p.world.lock();
 need(p.source_adopted&&p.query_returned&&p.result&&p.serial==previous&&p.generation==generation&&issuer&&issuer.get()==&source&&
  !p.issuer.owner_before(source.action_self_)&&!source.action_self_.owner_before(p.issuer)&&world&&world.get()==&state&&p.revision==state.ui_source_revision&&
  p.operation<UINT64_MAX&&h->operation==p.operation+1&&h->frontier.load()==BnwRetainedTurnFrontier::game_turn_player_set_damage_selected_info_dirty_owner,
  "Second UnitInfo requires real returned IsSelected=true and exact native next frontier");
 h->attrition_first->require_selected_unit_info_parent_without_callbacks(core,host,state,source,control,generation,previous);
}
void BnwRetainedTurnCursor::require_attrition_selected_unit_info_operation_without_callbacks(std::uint64_t serial,std::uint64_t revision,bool adopted)const{
 const auto h=impl_->history;h->thread_check();need(!h->retired.load()&&!h->poisoned.load()&&h->attrition_selected_unit_info_attempted&&h->attrition_unit_selected_return,
  "Second UnitInfo exact operation missing actual parent");const auto&p=*h->attrition_unit_selected_return;
 need(p.query_returned&&p.result&&p.serial<UINT64_MAX&&serial==p.serial+1&&p.operation<=UINT64_MAX-(adopted?2U:1U)&&
  h->operation==p.operation+1+(adopted?1U:0U)&&revision==p.revision&&h->frontier.load()==(adopted?BnwRetainedTurnFrontier::game_turn_player_set_damage_selection_plot_owner:
   BnwRetainedTurnFrontier::game_turn_player_set_damage_selected_info_dirty_owner),"Second UnitInfo immutable selected=true parent/new serial/operation/revision differs");
 if(h->attrition_selected_unit_info_return){const auto&r=*h->attrition_selected_unit_info_return;
  need(r.serial==serial&&r.previous==p.serial&&r.operation==p.operation+1&&r.revision==revision&&r.source_adopted==adopted&&r.generation==p.generation&&
   !r.issuer.owner_before(p.issuer)&&!p.issuer.owner_before(r.issuer)&&!r.world.owner_before(p.world)&&!p.world.owner_before(r.world),
   "Second UnitInfo prepared/earned same native query return changed");
 }else need(!adopted,"Second UnitInfo unprepared return cannot be adopted");
}
void BnwRetainedTurnCursor::record_attrition_selected_unit_info_return_without_callbacks(const BnwAttritionSelectedUnitInfoDirtyLoan&loan,
 std::uint64_t client_revision,std::uint64_t ordinal)noexcept{
 const auto h=impl_->history;
 if(h->thread!=std::this_thread::get_id()||h->retired.load()||h->poisoned.load()||!h->attrition_selected_unit_info_return||
  h->attrition_selected_unit_info_loan.get()!=&loan||h->attrition_selected_unit_info_return->source_adopted||h->operation==UINT64_MAX)std::terminate();
 auto&r=*h->attrition_selected_unit_info_return;r.client_after=client_revision;r.ordinal_after=ordinal;r.setter_returned=true;r.conditional_dirty_returned=true;r.source_adopted=true;
 ++h->operation;h->frontier.store(BnwRetainedTurnFrontier::game_turn_player_set_damage_selection_plot_owner);
}
void BnwRetainedTurnCursor::require_attrition_selected_unit_info_return_without_callbacks(const BnwNativeUnitAllocatorCore&core,const BnwSelectedStockLuaSourceOwner&source)const{
 const auto h=impl_->history;h->thread_check();need(h->attrition_selected_unit_info_return&&h->attrition_selected_unit_info_loan&&h->attrition_first,"UnitInfo physical actual return absent");
 const auto&r=*h->attrition_selected_unit_info_return;const auto issuer=r.issuer.lock();const auto world=r.world.lock();
 need(issuer&&issuer.get()==&source&&!r.issuer.owner_before(source.action_self_)&&!source.action_self_.owner_before(r.issuer)&&
  world&&world==impl_->session->state_&&r.generation==h->generation&&r.setter_returned&&r.conditional_dirty_returned&&r.source_adopted,
  "UnitInfo actual source/world/setter return differs");
 require_attrition_selected_unit_info_operation_without_callbacks(r.serial,r.revision,true);
 h->attrition_selected_unit_info_loan->require_common_without_callbacks();
 const auto ui=impl_->cycle_ui.lock();need(bool(ui),"UnitInfo actual UI expired after return");
 const auto selection=ui->city_selection_control_;need(bool(selection),"UnitInfo actual Selection expired after return");
 selection->require_attrition_selected_unit_info_dirty_return(*h->attrition_selected_unit_info_loan);
 h->attrition_selected_unit_info_loan->require_core_without_callbacks(core);
}
void BnwRetainedTurnCursor::fail_attrition_selected_unit_info_without_callbacks()noexcept{
 const auto h=impl_->history;if(h->attrition_selected_unit_info_loan)h->attrition_selected_unit_info_loan->fail_without_callbacks();h->quarantine();
}
}
