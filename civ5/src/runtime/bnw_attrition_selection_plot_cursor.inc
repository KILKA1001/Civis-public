namespace civ5::runtime {
std::uint64_t BnwRetainedTurnCursor::attrition_selection_plot_previous_serial_without_callbacks()const{
 const auto h=impl_->history;h->thread_check();need(bool(h->attrition_unit_selected_return),"SelectionPlot real selected-query return absent");
 return h->attrition_selected_unit_info_return?h->attrition_selected_unit_info_return->serial:h->attrition_unit_selected_return->serial;
}
void BnwRetainedTurnCursor::require_attrition_selection_plot_entry_without_callbacks(const BnwNativeUnitAllocatorCore&core,const BnwSelectedStockLuaSourceOwner&source)const{
 const auto h=impl_->history;h->thread_check();
 if(h->attrition_selection_plot_attempted){if(h->attrition_selection_plot_loan)h->attrition_selection_plot_loan->fail_without_callbacks();h->quarantine();need(false,"IsSelected actual once reentry");}
 need(!h->attrition_selection_plot_return&&!h->attrition_selection_plot_loan&&h->frontier.load()==BnwRetainedTurnFrontier::game_turn_player_set_damage_selection_plot_owner,
  "IsSelected requires exact original next read frontier");source.require_attrition_selection_plot_parent_without_callbacks(*this,core,nullptr,true);
}
void BnwRetainedTurnCursor::execute_attrition_selection_plot_with_actual_stock(const BnwNativeUnitAllocatorCore&core,
 const BnwSelectedStockLuaSourceOwner&source,const BnwStockLuaAttemptLease&lease){
 const auto i=impl_;auto&h=*i->history;h.thread_check();
 need(!h.attrition_selection_plot_attempted&&!h.attrition_selection_plot_return&&h.attrition_first&&h.operation<UINT64_MAX,"IsSelected actual prepared once entry absent");
 h.attrition_selection_plot_attempted=true;
 try{
  i->check_engine();require_attrition_city_info_facade_without_callbacks();const auto serial=lease.operation_serial();lease.require_operation(serial);
  require_attrition_city_info_facade_without_callbacks();
  source.require_attrition_selection_plot_parent_without_callbacks(*this,core,&lease,false);
  const auto world=i->session->state_;auto&state=*world;const auto actual_ui=i->cycle_ui.lock();
  need(actual_ui&&actual_ui->engine_ui_turn_owner_,"IsSelected actual retained UI/Engine absent");
  const auto selection=require_actual_ui(*actual_ui);require_attrition_city_info_facade_without_callbacks();const auto engine=actual_ui->engine_ui_turn_owner_;
  ui::BnwEngineUiTurnOwner::EndTurnCursorPin pin(*engine,*actual_ui,state);
  History::AttritionSelectionPlotReturn staged;staged.issuer=source.action_self_;staged.world=world;
  staged.generation=h.generation;staged.serial=serial;staged.previous=attrition_selection_plot_previous_serial_without_callbacks();
  staged.operation=h.operation;staged.revision=state.ui_source_revision;
  h.attrition_selection_plot_loan=source.issue_attrition_selection_plot_owned(*this,core,lease,*h.attrition_first,selection);
  i->check_engine();require_attrition_city_info_facade_without_callbacks();lease.require_operation(serial);require_attrition_city_info_facade_without_callbacks();
  const auto current_selection=require_actual_ui(*actual_ui);require_attrition_city_info_facade_without_callbacks();
  need(current_selection==selection&&actual_ui->engine_ui_turn_owner_==engine&&i->session->state_==world&&
   h.operation==staged.operation&&state.ui_source_revision==staged.revision,"IsSelected actual caller receivers changed before query");
  source.require_attrition_selection_plot_parent_without_callbacks(*this,core,&lease,false);
  static_assert(std::is_nothrow_move_constructible_v<History::AttritionSelectionPlotReturn>);
  h.attrition_selection_plot_return.emplace(std::move(staged));
  const auto result=ui::BnwEngineUiTurnOwner::consume_attrition_selection_plot_read_from_owned_weak(engine,*h.attrition_selection_plot_loan,state,lease);
  i->check_engine();require_attrition_city_info_facade_without_callbacks();lease.require_operation(serial);require_attrition_city_info_facade_without_callbacks();
  source.require_attrition_selection_plot_source_without_callbacks(*this,core,&lease,false);
  need(h.attrition_selection_plot_return->result==result,"IsSelected Engine/query branch return differs");
 }catch(...){fail_attrition_selection_plot_without_callbacks();throw;}
}
void BnwRetainedTurnCursor::require_attrition_selection_plot_parent_return_without_callbacks(const BnwNativeUnitAllocatorCore&core,
 const ui::SelectedBnwPreviewHost&host,const LiveSessionState&state,const BnwSelectedStockLuaSourceOwner&source,const void*control,
 std::uint64_t generation,std::uint64_t previous)const{
 const auto h=impl_->history;h->thread_check();need(!h->retired.load()&&!h->poisoned.load()&&h->attrition_first&&h->attrition_unit_selected_return&&h->attrition_unit_selected_loan,
  "SelectionPlot actual native branch parent absent/retired");const auto&q=*h->attrition_unit_selected_return;
 const auto verify=[&](const auto&p){const auto issuer=p.issuer.lock();const auto world=p.world.lock();
  need(p.source_adopted&&p.serial==previous&&p.generation==generation&&issuer&&issuer.get()==&source&&
   !p.issuer.owner_before(source.action_self_)&&!source.action_self_.owner_before(p.issuer)&&world&&world.get()==&state&&p.revision==state.ui_source_revision&&
   p.operation<UINT64_MAX&&h->operation==p.operation+1,"SelectionPlot exact immutable source/world/operation parent differs");};
 need(q.query_returned&&q.source_adopted,"SelectionPlot genuine native selected query unreturned");
 if(q.result){need(h->attrition_selected_unit_info_return&&h->attrition_selected_unit_info_loan,"SelectionPlot true branch cannot skip mandatory second UnitInfo");
  const auto&p=*h->attrition_selected_unit_info_return;need(p.setter_returned&&p.conditional_dirty_returned,"SelectionPlot second native setter unreturned");verify(p);
 }else{need(!h->attrition_selected_unit_info_attempted&&!h->attrition_selected_unit_info_return&&!h->attrition_selected_unit_info_loan,
   "SelectionPlot false branch cannot fabricate second UnitInfo");verify(q);}
 need(h->frontier.load()==BnwRetainedTurnFrontier::game_turn_player_set_damage_selection_plot_owner,"SelectionPlot exact mandatory native frontier differs");
 h->attrition_first->require_selection_plot_parent_without_callbacks(core,host,state,source,control,generation,previous);
}
void BnwRetainedTurnCursor::require_attrition_selection_plot_operation_without_callbacks(std::uint64_t serial,std::uint64_t revision,bool adopted)const{
 const auto h=impl_->history;h->thread_check();need(!h->retired.load()&&!h->poisoned.load()&&h->attrition_selection_plot_attempted&&h->attrition_unit_selected_return,
  "SelectionPlot exact operation retired/missing actual parent");
 const auto previous=h->attrition_selected_unit_info_return?h->attrition_selected_unit_info_return->serial:h->attrition_unit_selected_return->serial;
 const auto operation=h->attrition_selected_unit_info_return?h->attrition_selected_unit_info_return->operation:h->attrition_unit_selected_return->operation;
 const auto parent_revision=h->attrition_selected_unit_info_return?h->attrition_selected_unit_info_return->revision:h->attrition_unit_selected_return->revision;
 const auto generation=h->attrition_selected_unit_info_return?h->attrition_selected_unit_info_return->generation:h->attrition_unit_selected_return->generation;
 const auto&issuer=h->attrition_selected_unit_info_return?h->attrition_selected_unit_info_return->issuer:h->attrition_unit_selected_return->issuer;
 const auto&world=h->attrition_selected_unit_info_return?h->attrition_selected_unit_info_return->world:h->attrition_unit_selected_return->world;
 need(previous<UINT64_MAX&&serial==previous+1&&operation<=UINT64_MAX-(adopted?2U:1U)&&h->operation==operation+1+(adopted?1U:0U)&&
  revision==parent_revision,"SelectionPlot immutable actual parent/new serial/operation/revision differs");
 if(h->attrition_selection_plot_return){const auto&r=*h->attrition_selection_plot_return;
  need(r.serial==serial&&r.previous==previous&&r.operation==operation+1&&r.revision==revision&&r.source_adopted==adopted&&r.generation==generation&&
   !r.issuer.owner_before(issuer)&&!issuer.owner_before(r.issuer)&&!r.world.owner_before(world)&&!world.owner_before(r.world),"SelectionPlot exact native branch receipt differs");
  need(h->frontier.load()==(adopted?(r.plot_list_required?BnwRetainedTurnFrontier::game_turn_player_set_damage_plot_list_dirty_owner:
   BnwRetainedTurnFrontier::game_turn_player_set_damage_gameplay_damage_owner):BnwRetainedTurnFrontier::game_turn_player_set_damage_selection_plot_owner),
   "SelectionPlot exact native index comparison/mandatory next branch differs");
 }else need(!adopted&&h->frontier.load()==BnwRetainedTurnFrontier::game_turn_player_set_damage_selection_plot_owner,"SelectionPlot unprepared return cannot be adopted");
}
void BnwRetainedTurnCursor::record_attrition_selection_plot_return_without_callbacks(const BnwAttritionSelectionPlotRead&loan,
 std::optional<std::int32_t>result,std::int32_t unit_plot)noexcept{
 const auto h=impl_->history;if(h->thread!=std::this_thread::get_id()||h->retired.load()||h->poisoned.load()||!h->attrition_selection_plot_return||
  h->attrition_selection_plot_loan.get()!=&loan||h->attrition_selection_plot_return->source_adopted||h->operation==UINT64_MAX||unit_plot<0||(result&&*result<0))std::terminate();
 auto&r=*h->attrition_selection_plot_return;r.result=result;r.unit_plot=unit_plot;r.plot_list_required=result&&*result==unit_plot;
 r.query_returned=true;r.unit_plot_returned=true;r.source_adopted=true;++h->operation;
 h->frontier.store(r.plot_list_required?BnwRetainedTurnFrontier::game_turn_player_set_damage_plot_list_dirty_owner:BnwRetainedTurnFrontier::game_turn_player_set_damage_gameplay_damage_owner);
}
void BnwRetainedTurnCursor::require_attrition_selection_plot_return_without_callbacks(const BnwNativeUnitAllocatorCore&core,const BnwSelectedStockLuaSourceOwner&source)const{
 const auto h=impl_->history;h->thread_check();need(h->attrition_selection_plot_return&&h->attrition_selection_plot_loan&&h->attrition_first,"IsSelected actual return absent");
 const auto&r=*h->attrition_selection_plot_return;const auto issuer=r.issuer.lock();const auto world=r.world.lock();
 need(issuer&&issuer.get()==&source&&!r.issuer.owner_before(source.action_self_)&&!source.action_self_.owner_before(r.issuer)&&world&&world==impl_->session->state_&&
  r.generation==h->generation&&r.query_returned&&r.unit_plot_returned&&r.source_adopted,"IsSelected actual source/world/query return differs");
 require_attrition_selection_plot_operation_without_callbacks(r.serial,r.revision,true);h->attrition_selection_plot_loan->require_common_without_callbacks();
 const auto ui=impl_->cycle_ui.lock();need(bool(ui),"IsSelected actual UI expired after return");const auto selection=ui->city_selection_control_;
 need(bool(selection),"IsSelected actual Selection expired after return");selection->require_attrition_selection_plot_read_return(*h->attrition_selection_plot_loan);
 need(h->attrition_selection_plot_loan->result_without_callbacks()==r.result&&h->attrition_selection_plot_loan->plot_list_required_without_callbacks()==r.plot_list_required,"IsSelected typed result differs");h->attrition_selection_plot_loan->require_core_without_callbacks(core);
}
void BnwRetainedTurnCursor::fail_attrition_selection_plot_without_callbacks()noexcept{
 const auto h=impl_->history;if(h->attrition_selection_plot_loan)h->attrition_selection_plot_loan->fail_without_callbacks();h->quarantine();
}
}
