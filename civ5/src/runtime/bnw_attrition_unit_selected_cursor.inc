namespace civ5::runtime {
std::uint64_t BnwRetainedTurnCursor::attrition_unit_selected_previous_serial_without_callbacks()const{
 const auto h=impl_->history;h->thread_check();need(bool(h->attrition_unit_info_return),"IsSelected first UnitInfo return absent");return h->attrition_unit_info_return->serial;
}
void BnwRetainedTurnCursor::require_attrition_unit_selected_entry_without_callbacks(const BnwNativeUnitAllocatorCore&core,const BnwSelectedStockLuaSourceOwner&source)const{
 const auto h=impl_->history;h->thread_check();
 if(h->attrition_unit_selected_attempted){if(h->attrition_unit_selected_loan)h->attrition_unit_selected_loan->fail_without_callbacks();h->quarantine();need(false,"IsSelected actual once reentry");}
 need(!h->attrition_unit_selected_return&&!h->attrition_unit_selected_loan&&h->frontier.load()==BnwRetainedTurnFrontier::game_turn_player_set_damage_selected_owner,
  "IsSelected requires exact original next read frontier");source.require_attrition_unit_selected_parent_without_callbacks(*this,core,nullptr,true);
}
void BnwRetainedTurnCursor::execute_attrition_unit_selected_with_actual_stock(const BnwNativeUnitAllocatorCore&core,
 const BnwSelectedStockLuaSourceOwner&source,const BnwStockLuaAttemptLease&lease){
 const auto i=impl_;auto&h=*i->history;h.thread_check();
 need(!h.attrition_unit_selected_attempted&&!h.attrition_unit_selected_return&&h.attrition_first&&h.operation<UINT64_MAX,"IsSelected actual prepared once entry absent");
 h.attrition_unit_selected_attempted=true;
 try{
  i->check_engine();require_attrition_city_info_facade_without_callbacks();const auto serial=lease.operation_serial();lease.require_operation(serial);
  require_attrition_city_info_facade_without_callbacks();
  source.require_attrition_unit_selected_parent_without_callbacks(*this,core,&lease,false);
  const auto world=i->session->state_;auto&state=*world;const auto actual_ui=i->cycle_ui.lock();
  need(actual_ui&&actual_ui->engine_ui_turn_owner_,"IsSelected actual retained UI/Engine absent");
  const auto selection=require_actual_ui(*actual_ui);require_attrition_city_info_facade_without_callbacks();const auto engine=actual_ui->engine_ui_turn_owner_;
  ui::BnwEngineUiTurnOwner::EndTurnCursorPin pin(*engine,*actual_ui,state);
  History::AttritionUnitSelectedReturn staged;staged.issuer=source.action_self_;staged.world=world;
  staged.generation=h.generation;staged.serial=serial;staged.previous=attrition_unit_selected_previous_serial_without_callbacks();
  staged.operation=h.operation;staged.revision=state.ui_source_revision;
  h.attrition_unit_selected_loan=source.issue_attrition_unit_selected_owned(*this,core,lease,*h.attrition_first,selection);
  i->check_engine();require_attrition_city_info_facade_without_callbacks();lease.require_operation(serial);require_attrition_city_info_facade_without_callbacks();
  const auto current_selection=require_actual_ui(*actual_ui);require_attrition_city_info_facade_without_callbacks();
  need(current_selection==selection&&actual_ui->engine_ui_turn_owner_==engine&&i->session->state_==world&&
   h.operation==staged.operation&&state.ui_source_revision==staged.revision,"IsSelected actual caller receivers changed before query");
  source.require_attrition_unit_selected_parent_without_callbacks(*this,core,&lease,false);
  static_assert(std::is_nothrow_move_constructible_v<History::AttritionUnitSelectedReturn>);
  h.attrition_unit_selected_return.emplace(std::move(staged));
  const bool result=ui::BnwEngineUiTurnOwner::consume_attrition_unit_selected_read_from_owned_weak(engine,*h.attrition_unit_selected_loan,state,lease);
  i->check_engine();require_attrition_city_info_facade_without_callbacks();lease.require_operation(serial);require_attrition_city_info_facade_without_callbacks();
  source.require_attrition_unit_selected_source_without_callbacks(*this,core,&lease,false);
  need(h.attrition_unit_selected_return->result==result,"IsSelected Engine/query branch return differs");
 }catch(...){fail_attrition_unit_selected_without_callbacks();throw;}
}
void BnwRetainedTurnCursor::require_attrition_unit_selected_parent_return_without_callbacks(const BnwNativeUnitAllocatorCore&core,
 const ui::SelectedBnwPreviewHost&host,const LiveSessionState&state,const BnwSelectedStockLuaSourceOwner&source,const void*control,
 std::uint64_t generation,std::uint64_t previous)const{
 const auto h=impl_->history;h->thread_check();need(!h->retired.load()&&!h->poisoned.load()&&h->attrition_first&&h->attrition_unit_info_return&&h->attrition_unit_info_loan,
  "IsSelected actual first UnitInfo return absent/retired");const auto&p=*h->attrition_unit_info_return;const auto issuer=p.issuer.lock();const auto world=p.world.lock();
 need(p.source_adopted&&p.setter_returned&&p.info_bar_returned&&p.serial==previous&&p.generation==generation&&issuer&&issuer.get()==&source&&
  !p.issuer.owner_before(source.action_self_)&&!source.action_self_.owner_before(p.issuer)&&world&&world.get()==&state&&p.revision==state.ui_source_revision&&
  p.operation<UINT64_MAX&&h->operation==p.operation+1&&h->frontier.load()==BnwRetainedTurnFrontier::game_turn_player_set_damage_selected_owner,
  "IsSelected immutable UnitInfo parent/earned original frontier differs");
 h->attrition_first->require_unit_selected_parent_without_callbacks(core,host,state,source,control,generation,previous);
}
void BnwRetainedTurnCursor::require_attrition_unit_selected_operation_without_callbacks(std::uint64_t serial,std::uint64_t revision,bool adopted)const{
 const auto h=impl_->history;h->thread_check();need(!h->retired.load()&&!h->poisoned.load()&&h->attrition_unit_selected_attempted&&h->attrition_unit_info_return,
  "IsSelected exact operation retired/missing actual parent");const auto&p=*h->attrition_unit_info_return;
 need(p.serial<UINT64_MAX&&serial==p.serial+1&&p.operation<=UINT64_MAX-(adopted?2U:1U)&&h->operation==p.operation+1+(adopted?1U:0U)&&
  revision==p.revision,"IsSelected immutable parent/new serial/operation/revision differs");
 if(h->attrition_unit_selected_return){const auto&r=*h->attrition_unit_selected_return;
  need(r.serial==serial&&r.previous==p.serial&&r.operation==p.operation+1&&r.revision==revision&&r.source_adopted==adopted&&r.generation==p.generation&&
   !r.issuer.owner_before(p.issuer)&&!p.issuer.owner_before(r.issuer)&&!r.world.owner_before(p.world)&&!p.world.owner_before(r.world),
   "IsSelected prepared/earned same actual parent metadata differs");
  need(h->frontier.load()==(adopted?(r.result?BnwRetainedTurnFrontier::game_turn_player_set_damage_selected_info_dirty_owner:
   BnwRetainedTurnFrontier::game_turn_player_set_damage_selection_plot_owner):BnwRetainedTurnFrontier::game_turn_player_set_damage_selected_owner),
   "IsSelected original result/mandatory next branch differs");
 }else need(!adopted&&h->frontier.load()==BnwRetainedTurnFrontier::game_turn_player_set_damage_selected_owner,"IsSelected unprepared return cannot be adopted");
}
void BnwRetainedTurnCursor::record_attrition_unit_selected_return_without_callbacks(const BnwAttritionUnitSelectedRead&loan,bool result)noexcept{
 const auto h=impl_->history;if(h->thread!=std::this_thread::get_id()||h->retired.load()||h->poisoned.load()||!h->attrition_unit_selected_return||
  h->attrition_unit_selected_loan.get()!=&loan||h->attrition_unit_selected_return->source_adopted||h->operation==UINT64_MAX)std::terminate();
 auto&r=*h->attrition_unit_selected_return;r.result=result;r.query_returned=true;r.source_adopted=true;++h->operation;
 h->frontier.store(result?BnwRetainedTurnFrontier::game_turn_player_set_damage_selected_info_dirty_owner:BnwRetainedTurnFrontier::game_turn_player_set_damage_selection_plot_owner);
}
void BnwRetainedTurnCursor::require_attrition_unit_selected_return_without_callbacks(const BnwNativeUnitAllocatorCore&core,const BnwSelectedStockLuaSourceOwner&source)const{
 const auto h=impl_->history;h->thread_check();need(h->attrition_unit_selected_return&&h->attrition_unit_selected_loan&&h->attrition_first,"IsSelected actual return absent");
 const auto&r=*h->attrition_unit_selected_return;const auto issuer=r.issuer.lock();const auto world=r.world.lock();
 need(issuer&&issuer.get()==&source&&!r.issuer.owner_before(source.action_self_)&&!source.action_self_.owner_before(r.issuer)&&world&&world==impl_->session->state_&&
  r.generation==h->generation&&r.query_returned&&r.source_adopted,"IsSelected actual source/world/query return differs");
 require_attrition_unit_selected_operation_without_callbacks(r.serial,r.revision,true);h->attrition_unit_selected_loan->require_common_without_callbacks();
 const auto ui=impl_->cycle_ui.lock();need(bool(ui),"IsSelected actual UI expired after return");const auto selection=ui->city_selection_control_;
 need(bool(selection),"IsSelected actual Selection expired after return");selection->require_attrition_unit_selected_read_return(*h->attrition_unit_selected_loan);
 need(h->attrition_unit_selected_loan->result_without_callbacks()==r.result,"IsSelected typed result differs");h->attrition_unit_selected_loan->require_core_without_callbacks(core);
}
void BnwRetainedTurnCursor::fail_attrition_unit_selected_without_callbacks()noexcept{
 const auto h=impl_->history;if(h->attrition_unit_selected_loan)h->attrition_unit_selected_loan->fail_without_callbacks();h->quarantine();
}
}
