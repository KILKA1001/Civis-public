namespace civ5::runtime {
std::uint64_t BnwRetainedTurnCursor::attrition_unit_info_previous_serial_without_callbacks()const{
 const auto h=impl_->history;h->thread_check();need(bool(h->attrition_garrison_return),"UnitInfo original strength history absent");return h->attrition_city_info_return?h->attrition_city_info_return->serial:h->attrition_garrison_return->serial;
}
void BnwRetainedTurnCursor::require_attrition_unit_info_entry_without_callbacks(const BnwNativeUnitAllocatorCore&core,const BnwSelectedStockLuaSourceOwner&source)const{
 const auto h=impl_->history;h->thread_check();
 if(h->attrition_unit_info_attempted){if(h->attrition_unit_info_loan)h->attrition_unit_info_loan->fail_without_callbacks();h->quarantine();need(false,"UnitInfo actual once reentry");}
 need(!h->attrition_unit_info_return&&!h->attrition_unit_info_loan&&h->frontier.load()==BnwRetainedTurnFrontier::game_turn_player_set_damage_info_bar_dirty_owner,
  "UnitInfo requires original physical dirty frontier");source.require_attrition_unit_info_parent_without_callbacks(*this,core,nullptr,true);
}
void BnwRetainedTurnCursor::execute_attrition_unit_info_with_actual_stock(const BnwNativeUnitAllocatorCore&core,
 const BnwSelectedStockLuaSourceOwner&source,const BnwStockLuaAttemptLease&lease){
 const auto i=impl_;auto& h=*i->history;h.thread_check();
 need(!h.attrition_unit_info_attempted&&!h.attrition_unit_info_return&&h.attrition_first&&h.operation<UINT64_MAX,"UnitInfo actual prepared once entry absent");
 h.attrition_unit_info_attempted=true;
 try{
  i->check_engine();const auto serial=lease.operation_serial();lease.require_operation(serial);
  source.require_attrition_unit_info_parent_without_callbacks(*this,core,&lease,false);
  const auto world=i->session->state_;auto&state=*world;const auto actual_ui=i->cycle_ui.lock();
  need(actual_ui&&actual_ui->engine_ui_turn_owner_,"UnitInfo actual retained UI/Engine absent");
  const auto selection=require_actual_ui(*actual_ui);const auto engine=actual_ui->engine_ui_turn_owner_;
  ui::BnwEngineUiTurnOwner::EndTurnCursorPin pin(*engine,*actual_ui,state);
  History::AttritionUnitInfoReturn staged;staged.issuer=source.action_self_;staged.world=world;
  staged.generation=h.generation;staged.serial=serial;staged.previous=attrition_unit_info_previous_serial_without_callbacks();
  staged.operation=h.operation;staged.revision=state.ui_source_revision;
  h.attrition_unit_info_loan=source.issue_attrition_unit_info_owned(*this,core,lease,*h.attrition_first,selection);
  i->check_engine();lease.require_operation(serial);
  need(require_actual_ui(*actual_ui)==selection&&actual_ui->engine_ui_turn_owner_==engine&&i->session->state_==world&&
   h.operation==staged.operation&&state.ui_source_revision==staged.revision,"UnitInfo actual caller receivers changed before setter");
  source.require_attrition_unit_info_parent_without_callbacks(*this,core,&lease,false);
  static_assert(std::is_nothrow_move_constructible_v<History::AttritionUnitInfoReturn>);
  h.attrition_unit_info_return.emplace(std::move(staged));
  ui::BnwEngineUiTurnOwner::consume_attrition_unit_info_dirty_from_owned_weak(engine,*h.attrition_unit_info_loan,state,lease);
  // Actual setter and operation adoption already finished before Engine POST.
  i->check_engine();lease.require_operation(serial);
  source.require_attrition_unit_info_source_without_callbacks(*this,core,&lease,false);
 }catch(...){fail_attrition_unit_info_without_callbacks();throw;}
}
void BnwRetainedTurnCursor::require_attrition_unit_info_parent_return_without_callbacks(const BnwNativeUnitAllocatorCore&core,
 const ui::SelectedBnwPreviewHost&host,const LiveSessionState&state,const BnwSelectedStockLuaSourceOwner&source,const void*control,
 std::uint64_t generation,std::uint64_t previous)const{
 const auto h=impl_->history;h->thread_check();need(!h->retired.load()&&!h->poisoned.load()&&h->attrition_first&&h->attrition_garrison_return,
  "UnitInfo original parent absent/retired");
 if(h->attrition_city_info_return){const auto&p=*h->attrition_city_info_return;const auto issuer=p.issuer.lock();const auto world=p.world.lock();
  need(h->attrition_city_info_loan&&p.source_adopted&&p.setter_returned&&p.update_strength_returned&&p.serial==previous&&p.generation==generation&&
   issuer&&issuer.get()==&source&&!p.issuer.owner_before(source.action_self_)&&!source.action_self_.owner_before(p.issuer)&&
   world&&world.get()==&state&&p.revision==state.ui_source_revision&&p.operation<UINT64_MAX&&h->operation==p.operation+1&&
   h->frontier.load()==BnwRetainedTurnFrontier::game_turn_player_set_damage_info_bar_dirty_owner,
   "UnitInfo original CityInfo metadata and real mandatory next frontier differ");
 }else{
  need(!h->attrition_city_info_loan&&!h->attrition_city_strength_return&&!h->attrition_city_strength_attempted,
   "UnitInfo cannot replace unfinished mandatory CityInfo/strength");
  require_attrition_garrison_return_without_callbacks(core,host,state,source,control,generation,previous,true);
  need(h->attrition_garrison_return->next==BnwRetainedTurnFrontier::game_turn_player_set_damage_info_bar_dirty_owner,
   "UnitInfo genuine false/NULL garrison next frontier absent");
 }
 h->attrition_first->require_unit_info_parent_without_callbacks(core,host,state,source,control,generation,previous);
}
void BnwRetainedTurnCursor::require_attrition_unit_info_operation_without_callbacks(std::uint64_t serial,std::uint64_t revision,bool adopted)const{
 const auto h=impl_->history;h->thread_check();need(!h->retired.load()&&!h->poisoned.load()&&h->attrition_unit_info_attempted&&h->attrition_garrison_return,
  "UnitInfo exact operation retired/missing parent");
 const auto previous=h->attrition_city_info_return?h->attrition_city_info_return->serial:h->attrition_garrison_return->serial;
 const auto operation=h->attrition_city_info_return?h->attrition_city_info_return->operation:h->attrition_garrison_return->operation;
 const auto parent_revision=h->attrition_city_info_return?h->attrition_city_info_return->revision:h->attrition_garrison_return->revision;
 const auto parent_generation=h->attrition_city_info_return?h->attrition_city_info_return->generation:h->attrition_garrison_return->generation;
 need(previous<UINT64_MAX&&serial==previous+1&&operation<=UINT64_MAX-(adopted?2U:1U)&&h->operation==operation+1+(adopted?1U:0U)&&
  revision==parent_revision&&h->frontier.load()==(adopted?BnwRetainedTurnFrontier::game_turn_player_set_damage_selected_owner:BnwRetainedTurnFrontier::game_turn_player_set_damage_info_bar_dirty_owner),
  "UnitInfo immutable original parent/new serial/operation/revision differs");
 if(h->attrition_unit_info_return){const auto&r=*h->attrition_unit_info_return;
  const auto&issuer=h->attrition_city_info_return?h->attrition_city_info_return->issuer:h->attrition_garrison_return->issuer;
  const auto&world=h->attrition_city_info_return?h->attrition_city_info_return->world:h->attrition_garrison_return->world;
  need(r.serial==serial&&r.previous==previous&&r.operation==operation+1&&r.revision==revision&&r.source_adopted==adopted&&r.generation==parent_generation&&
   !r.issuer.owner_before(issuer)&&!issuer.owner_before(r.issuer)&&!r.world.owner_before(world)&&!world.owner_before(r.world),"UnitInfo prepared/earned same parent return changed");
 }else need(!adopted,"UnitInfo unprepared return cannot be adopted");
}
void BnwRetainedTurnCursor::record_attrition_unit_info_return_without_callbacks(const BnwAttritionUnitInfoDirtyLoan&loan,
 std::uint64_t client_revision,std::uint64_t ordinal)noexcept{
 const auto h=impl_->history;
 if(h->thread!=std::this_thread::get_id()||h->retired.load()||h->poisoned.load()||!h->attrition_unit_info_return||
  h->attrition_unit_info_loan.get()!=&loan||h->attrition_unit_info_return->source_adopted||h->operation==UINT64_MAX)std::terminate();
 auto&r=*h->attrition_unit_info_return;r.client_after=client_revision;r.ordinal_after=ordinal;r.setter_returned=true;r.info_bar_returned=true;r.source_adopted=true;
 ++h->operation;h->frontier.store(BnwRetainedTurnFrontier::game_turn_player_set_damage_selected_owner);
}
void BnwRetainedTurnCursor::require_attrition_unit_info_return_without_callbacks(const BnwNativeUnitAllocatorCore&core,const BnwSelectedStockLuaSourceOwner&source)const{
 const auto h=impl_->history;h->thread_check();need(h->attrition_unit_info_return&&h->attrition_unit_info_loan&&h->attrition_first,"UnitInfo physical actual return absent");
 const auto&r=*h->attrition_unit_info_return;const auto issuer=r.issuer.lock();const auto world=r.world.lock();
 need(issuer&&issuer.get()==&source&&!r.issuer.owner_before(source.action_self_)&&!source.action_self_.owner_before(r.issuer)&&
  world&&world==impl_->session->state_&&r.generation==h->generation&&r.setter_returned&&r.info_bar_returned&&r.source_adopted,
  "UnitInfo actual source/world/setter return differs");
 require_attrition_unit_info_operation_without_callbacks(r.serial,r.revision,true);
 h->attrition_unit_info_loan->require_common_without_callbacks();
 const auto ui=impl_->cycle_ui.lock();need(bool(ui),"UnitInfo actual UI expired after return");
 const auto selection=ui->city_selection_control_;need(bool(selection),"UnitInfo actual Selection expired after return");
 selection->require_attrition_unit_info_dirty_return(*h->attrition_unit_info_loan);
 h->attrition_unit_info_loan->require_core_without_callbacks(core);
}
void BnwRetainedTurnCursor::fail_attrition_unit_info_without_callbacks()noexcept{
 const auto h=impl_->history;if(h->attrition_unit_info_loan)h->attrition_unit_info_loan->fail_without_callbacks();h->quarantine();
}
}
