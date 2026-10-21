namespace civ5::runtime {
std::uint64_t BnwRetainedTurnCursor::attrition_city_strength_previous_serial_without_callbacks()const{
 const auto h=impl_->history;h->thread_check();need(bool(h->attrition_garrison_return),"City strength garrison history absent");return h->attrition_garrison_return->serial;
}
void BnwRetainedTurnCursor::require_attrition_city_strength_entry_without_callbacks(const BnwNativeUnitAllocatorCore&core,
 const BnwSelectedStockLuaSourceOwner&source)const{
 const auto h=impl_->history;h->thread_check();
 if(h->attrition_city_strength_attempted){if(h->attrition_first)h->attrition_first->fail_city_strength_without_callbacks();h->quarantine();need(false,"City strength actual once reentry");}
 need(!h->attrition_city_strength_return&&h->frontier.load()==BnwRetainedTurnFrontier::game_turn_player_set_damage_city_strength_owner,
      "City strength requires genuine true garrison SAME receiver frontier");
 source.require_attrition_city_strength_parent_source_without_callbacks(*this,core,nullptr,true);
}
void BnwRetainedTurnCursor::execute_attrition_city_strength_with_actual_stock(const BnwNativeUnitAllocatorCore&core,
 const BnwSelectedStockLuaSourceOwner&source,const BnwStockLuaAttemptLease&lease){
 const auto i=impl_;auto&h=*i->history;h.thread_check();
 need(!h.attrition_city_strength_attempted&&!h.attrition_city_strength_return&&h.attrition_first&&h.operation<UINT64_MAX,
      "City strength genuine once entry absent");h.attrition_city_strength_attempted=true;
 try{i->check_engine();const auto serial=lease.operation_serial();lease.require_operation(serial);
  source.require_attrition_city_strength_parent_source_without_callbacks(*this,core,&lease,false);
  const auto allocation=i->session->state_;auto&state=*allocation;History::AttritionCityStrengthReturn staged;
  staged.issuer=source.action_self_;staged.world=allocation;staged.generation=h.generation;staged.serial=serial;
  staged.previous=attrition_city_strength_previous_serial_without_callbacks();staged.operation=h.operation;staged.before=state.ui_source_revision;
  source.issue_attrition_city_strength_owned(*this,core,lease,*h.attrition_first);
  i->check_engine();lease.require_operation(serial);source.require_attrition_city_strength_parent_source_without_callbacks(*this,core,&lease,false);
  source.seal_attrition_city_strength_without_callbacks(core,*h.attrition_first,lease,false);
  need(i->session->state_==allocation&&state.ui_source_revision==staged.before&&h.operation==staged.operation,
       "City strength world replaced before native ordered stores");
  static_assert(std::is_nothrow_move_constructible_v<History::AttritionCityStrengthReturn>);
  h.attrition_city_strength_return.emplace(std::move(staged));auto&earned=*h.attrition_city_strength_return;
  h.attrition_first->execute_city_strength_without_callbacks(state);
  const auto q=h.attrition_first->city_strength_progress_without_callbacks();earned.after=q.after_revision;
  source.adopt_actual_attrition_city_strength_return(*this,core,lease);earned.source_adopted=true;
  ++h.operation;h.frontier.store(earned.next);i->check_engine();lease.require_operation(serial);
  source.require_attrition_city_strength_source_without_callbacks(*this,core,&lease,false);
 }catch(...){fail_attrition_city_strength_without_callbacks();throw;}
}
void BnwRetainedTurnCursor::require_attrition_city_strength_return_without_callbacks(const BnwNativeUnitAllocatorCore&core,
 const ui::SelectedBnwPreviewHost&host,const LiveSessionState&state,const BnwSelectedStockLuaSourceOwner&source,
 const void*control,std::uint64_t generation,std::uint64_t serial,bool adopted)const{
 const auto h=impl_->history;h->thread_check();
 need(!h->retired.load()&&!h->poisoned.load()&&h->attrition_city_strength_attempted&&h->attrition_city_strength_return&&
      h->attrition_garrison_return&&h->attrition_first,"City strength typed actual return absent");
 const auto&r=*h->attrition_city_strength_return;const auto&p=*h->attrition_garrison_return;
 const auto issuer=r.issuer.lock(),parent_issuer=p.issuer.lock();const auto world=r.world.lock(),parent_world=p.world.lock();
 need(issuer&&issuer.get()==&source&&world&&world.get()==&state&&parent_issuer&&parent_issuer.get()==&source&&parent_world&&parent_world.get()==&state&&
      !r.issuer.owner_before(source.action_self_)&&!source.action_self_.owner_before(r.issuer)&&
      !r.issuer.owner_before(p.issuer)&&!p.issuer.owner_before(r.issuer)&&!r.world.owner_before(p.world)&&!p.world.owner_before(r.world)&&
      r.generation==generation&&p.generation==generation&&r.serial==serial&&r.previous==p.serial&&p.serial<UINT64_MAX&&serial==p.serial+1&&
      p.source_adopted&&p.operation<UINT64_MAX&&r.operation==p.operation+1&&r.before==p.revision&&r.before<UINT64_MAX&&r.after==r.before+1&&
      state.ui_source_revision==r.after&&p.next==BnwRetainedTurnFrontier::game_turn_player_set_damage_city_strength_owner&&
      r.next==BnwRetainedTurnFrontier::game_turn_player_set_damage_city_info_dirty_owner&&r.source_adopted==adopted&&
      h->operation==r.operation+(adopted?1U:0U)&&h->frontier.load()==(adopted?r.next:p.next),
      "City strength SAME old garrison/new operation/source/world native stores phase differs");
 // No old Cursor receipt is validated at a new serial/revision. The closed
 // Core successor validates the immutable old metadata and exact after-node.
 h->attrition_first->require_city_strength_without_callbacks(core,host,state,source,control,generation,serial,true);
 const auto q=h->attrition_first->city_strength_progress_without_callbacks();
 need(q.executed&&!q.failed&&q.previous_source_serial==p.serial&&q.source_serial==serial&&q.before_revision==r.before&&q.after_revision==r.after&&
      q.base_assignment_invoked&&q.hills_query_returned&&q.cache_stored&&!q.city_info_dirty_returned&&!q.update_strength_returned,
      "City strength original ordered stores/mandatory physical CityInfo frontier differs");
}
void BnwRetainedTurnCursor::fail_attrition_city_strength_without_callbacks()noexcept{
 const auto h=impl_->history;if(h->attrition_first)h->attrition_first->fail_city_strength_without_callbacks();h->quarantine();
}
BnwAttritionCityStrengthProgress BnwRetainedTurnCursor::attrition_city_strength_progress()const{
 const auto h=impl_->history;h->thread_check();return h->attrition_first?h->attrition_first->city_strength_progress_without_callbacks():BnwAttritionCityStrengthProgress{};
}
}
