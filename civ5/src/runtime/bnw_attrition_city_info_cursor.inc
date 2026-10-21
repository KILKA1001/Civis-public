namespace civ5::runtime {
void BnwRetainedTurnCursor::require_attrition_city_info_facade_without_callbacks()const{
 const auto h=impl_->history;h->thread_check();
 need(h->lifetime&&!h->lifetime->destroyed.load()&&!h->retired.load()&&!h->poisoned.load(),"CityInfo genuine facade retired before raw touch");
}

std::uint64_t BnwRetainedTurnCursor::attrition_city_info_previous_serial_without_callbacks()const{
 const auto h=impl_->history;h->thread_check();need(bool(h->attrition_city_strength_return),"CityInfo original strength history absent");return h->attrition_city_strength_return->serial;
}
void BnwRetainedTurnCursor::require_attrition_city_info_entry_without_callbacks(const BnwNativeUnitAllocatorCore&core,const BnwSelectedStockLuaSourceOwner&source)const{
 const auto h=impl_->history;h->thread_check();
 if(h->attrition_city_info_attempted){if(h->attrition_city_info_loan)h->attrition_city_info_loan->fail_without_callbacks();h->quarantine();need(false,"CityInfo actual once reentry");}
 need(!h->attrition_city_info_return&&!h->attrition_city_info_loan&&h->frontier.load()==BnwRetainedTurnFrontier::game_turn_player_set_damage_city_info_dirty_owner,
  "CityInfo requires original physical dirty frontier");source.require_attrition_city_info_parent_without_callbacks(*this,core,nullptr,true);
}
void BnwRetainedTurnCursor::execute_attrition_city_info_with_actual_stock(const BnwNativeUnitAllocatorCore&core,
 const BnwSelectedStockLuaSourceOwner&source,const BnwStockLuaAttemptLease&lease){
 const auto i=impl_;auto& h=*i->history;h.thread_check();
 need(!h.attrition_city_info_attempted&&!h.attrition_city_info_return&&h.attrition_first&&h.operation<UINT64_MAX,"CityInfo actual prepared once entry absent");
 h.attrition_city_info_attempted=true;
 try{
  i->check_engine();const auto serial=lease.operation_serial();lease.require_operation(serial);
  source.require_attrition_city_info_parent_without_callbacks(*this,core,&lease,false);
  const auto world=i->session->state_;auto&state=*world;const auto actual_ui=i->cycle_ui.lock();
  need(actual_ui&&actual_ui->engine_ui_turn_owner_,"CityInfo actual retained UI/Engine absent");
  const auto selection=require_actual_ui(*actual_ui);const auto engine=actual_ui->engine_ui_turn_owner_;
  ui::BnwEngineUiTurnOwner::EndTurnCursorPin pin(*engine,*actual_ui,state);
  History::AttritionCityInfoReturn staged;staged.issuer=source.action_self_;staged.world=world;
  staged.generation=h.generation;staged.serial=serial;staged.previous=attrition_city_info_previous_serial_without_callbacks();
  staged.operation=h.operation;staged.revision=state.ui_source_revision;
  h.attrition_city_info_loan=source.issue_attrition_city_info_owned(*this,core,lease,*h.attrition_first,selection);
  i->check_engine();lease.require_operation(serial);
  need(require_actual_ui(*actual_ui)==selection&&actual_ui->engine_ui_turn_owner_==engine&&i->session->state_==world&&
   h.operation==staged.operation&&state.ui_source_revision==staged.revision,"CityInfo actual caller receivers changed before setter");
  source.require_attrition_city_info_parent_without_callbacks(*this,core,&lease,false);
  static_assert(std::is_nothrow_move_constructible_v<History::AttritionCityInfoReturn>);
  h.attrition_city_info_return.emplace(std::move(staged));
  ui::BnwEngineUiTurnOwner::consume_attrition_city_info_dirty_from_owned_weak(engine,*h.attrition_city_info_loan,state,lease);
  // Actual setter and operation adoption already finished before Engine POST.
  i->check_engine();lease.require_operation(serial);
  source.require_attrition_city_info_source_without_callbacks(*this,core,&lease,false);
 }catch(...){fail_attrition_city_info_without_callbacks();throw;}
}
void BnwRetainedTurnCursor::require_attrition_city_info_operation_without_callbacks(std::uint64_t serial,std::uint64_t revision,bool adopted)const{
 const auto h=impl_->history;h->thread_check();need(!h->retired.load()&&!h->poisoned.load()&&h->attrition_city_info_attempted&&h->attrition_city_strength_return,
  "CityInfo operation retired/missing original parent");const auto& parent=*h->attrition_city_strength_return;
 need(parent.source_adopted&&parent.serial<UINT64_MAX&&serial==parent.serial+1&&parent.operation<=UINT64_MAX-(adopted?2U:1U)&&
  h->operation==parent.operation+1+(adopted?1U:0U)&&revision==parent.after&&
  h->frontier.load()==(adopted?BnwRetainedTurnFrontier::game_turn_player_set_damage_info_bar_dirty_owner:parent.next),
  "CityInfo genuine old metadata/new operation/unchanged world revision differs");
 if(h->attrition_city_info_return){const auto& r=*h->attrition_city_info_return;
  need(r.serial==serial&&r.previous==parent.serial&&r.operation==parent.operation+1&&r.revision==revision&&r.source_adopted==adopted&&
   r.generation==parent.generation&&!r.issuer.owner_before(parent.issuer)&&!parent.issuer.owner_before(r.issuer)&&
   !r.world.owner_before(parent.world)&&!parent.world.owner_before(r.world),"CityInfo prepared/earned typed return changed");
 }else need(!adopted,"CityInfo unprepared return cannot be adopted");
}
void BnwRetainedTurnCursor::record_attrition_city_info_return_without_callbacks(const BnwAttritionCityInfoDirtyLoan&loan,
 std::uint64_t client_revision,std::uint64_t city_count)noexcept{
 const auto h=impl_->history;
 if(h->thread!=std::this_thread::get_id()||h->retired.load()||h->poisoned.load()||!h->attrition_city_info_return||
  h->attrition_city_info_loan.get()!=&loan||h->attrition_city_info_return->source_adopted||h->operation==UINT64_MAX)std::terminate();
 auto&r=*h->attrition_city_info_return;r.client_after=client_revision;r.city_count_after=city_count;r.setter_returned=true;r.update_strength_returned=true;r.source_adopted=true;
 ++h->operation;h->frontier.store(BnwRetainedTurnFrontier::game_turn_player_set_damage_info_bar_dirty_owner);
}
void BnwRetainedTurnCursor::require_attrition_city_info_return_without_callbacks(const BnwNativeUnitAllocatorCore&core,const BnwSelectedStockLuaSourceOwner&source)const{
 const auto h=impl_->history;h->thread_check();need(h->attrition_city_info_return&&h->attrition_city_info_loan&&h->attrition_first,"CityInfo physical actual return absent");
 const auto&r=*h->attrition_city_info_return;const auto issuer=r.issuer.lock();const auto world=r.world.lock();
 need(issuer&&issuer.get()==&source&&!r.issuer.owner_before(source.action_self_)&&!source.action_self_.owner_before(r.issuer)&&
  world&&world==impl_->session->state_&&r.generation==h->generation&&r.setter_returned&&r.update_strength_returned&&r.source_adopted,
  "CityInfo actual source/world/setter return differs");
 require_attrition_city_info_operation_without_callbacks(r.serial,r.revision,true);
 h->attrition_city_info_loan->require_common_without_callbacks();
 const auto ui=impl_->cycle_ui.lock();need(bool(ui),"CityInfo actual UI expired after return");
 const auto selection=ui->city_selection_control_;need(bool(selection),"CityInfo actual Selection expired after return");
 selection->require_attrition_city_info_dirty_return(*h->attrition_city_info_loan);
 h->attrition_city_info_loan->require_core_without_callbacks(core);
}
void BnwRetainedTurnCursor::fail_attrition_city_info_without_callbacks()noexcept{
 const auto h=impl_->history;if(h->attrition_city_info_loan)h->attrition_city_info_loan->fail_without_callbacks();h->quarantine();
}
}
