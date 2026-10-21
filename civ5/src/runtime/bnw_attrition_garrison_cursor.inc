namespace civ5::runtime {
std::uint64_t BnwRetainedTurnCursor::attrition_garrison_previous_serial_without_callbacks()const{
 const auto h=impl_->history;h->thread_check();need(bool(h->attrition_damage_return),"Garrison damage history absent");return h->attrition_damage_return->serial;
}
void BnwRetainedTurnCursor::require_attrition_garrison_entry_without_callbacks(const BnwNativeUnitAllocatorCore&core,
 const BnwSelectedStockLuaSourceOwner&source)const{
 const auto h=impl_->history;h->thread_check();
 if(h->attrition_garrison_attempted){if(h->attrition_first)h->attrition_first->fail_garrison_without_callbacks();h->quarantine();need(false,"Garrison actual once reentry");}
 need(!h->attrition_garrison_return&&h->frontier.load()==BnwRetainedTurnFrontier::game_turn_player_set_damage_garrison_owner,
      "Garrison requires genuine native changed HP frontier");
 source.require_attrition_garrison_parent_source_without_callbacks(*this,core,nullptr,true);
}
void BnwRetainedTurnCursor::execute_attrition_garrison_with_actual_stock(const BnwNativeUnitAllocatorCore&core,
 const BnwSelectedStockLuaSourceOwner&source,const BnwStockLuaAttemptLease&lease){
 const auto i=impl_;auto&h=*i->history;h.thread_check();
 need(!h.attrition_garrison_attempted&&!h.attrition_garrison_return&&h.attrition_first&&h.operation<UINT64_MAX,
      "Garrison genuine once entry absent");h.attrition_garrison_attempted=true;
 try{i->check_engine();const auto serial=lease.operation_serial();lease.require_operation(serial);
  source.require_attrition_garrison_parent_source_without_callbacks(*this,core,&lease,false);
  const auto allocation=i->session->state_;const auto&state=*allocation;History::AttritionGarrisonReturn staged;
  staged.issuer=source.action_self_;staged.world=allocation;staged.generation=h.generation;staged.serial=serial;
  staged.previous=attrition_garrison_previous_serial_without_callbacks();staged.operation=h.operation;staged.revision=state.ui_source_revision;
  source.issue_attrition_garrison_owned(*this,core,lease,*h.attrition_first);
  i->check_engine();lease.require_operation(serial);source.require_attrition_garrison_parent_source_without_callbacks(*this,core,&lease,false);
  source.seal_attrition_garrison_without_callbacks(core,*h.attrition_first,lease,false);
  need(i->session->state_==allocation&&state.ui_source_revision==staged.revision&&h.operation==staged.operation,
       "Garrison world replaced before native query return");
  static_assert(std::is_nothrow_move_constructible_v<History::AttritionGarrisonReturn>);
  h.attrition_garrison_return.emplace(std::move(staged));auto&earned=*h.attrition_garrison_return;
  h.attrition_first->execute_garrison_without_callbacks(state);const auto q=h.attrition_first->garrison_progress_without_callbacks();
  earned.next=q.boundary==BnwAttritionGarrisonBoundary::city_strength_owner?
      BnwRetainedTurnFrontier::game_turn_player_set_damage_city_strength_owner:
      BnwRetainedTurnFrontier::game_turn_player_set_damage_info_bar_dirty_owner;
  source.adopt_actual_attrition_garrison_return(*this,core,lease);earned.source_adopted=true;
  ++h.operation;h.frontier.store(earned.next);i->check_engine();lease.require_operation(serial);
  source.require_attrition_garrison_source_without_callbacks(*this,core,&lease,false);
 }catch(...){fail_attrition_garrison_without_callbacks();throw;}
}
void BnwRetainedTurnCursor::require_attrition_garrison_return_without_callbacks(const BnwNativeUnitAllocatorCore&core,
 const ui::SelectedBnwPreviewHost&host,const LiveSessionState&state,const BnwSelectedStockLuaSourceOwner&source,
 const void*control,std::uint64_t generation,std::uint64_t serial,bool adopted)const{
 const auto h=impl_->history;h->thread_check();
 need(!h->retired.load()&&!h->poisoned.load()&&h->attrition_garrison_attempted&&h->attrition_garrison_return&&
      h->attrition_damage_return&&h->attrition_first,"Garrison typed actual return absent");
 const auto&r=*h->attrition_garrison_return;const auto&p=*h->attrition_damage_return;
 const auto issuer=r.issuer.lock();const auto world=r.world.lock();const auto parent_issuer=p.issuer.lock();const auto parent_world=p.world.lock();
 need(issuer&&issuer.get()==&source&&world&&world.get()==&state&&parent_issuer&&parent_issuer.get()==&source&&parent_world&&parent_world.get()==&state&&
      !r.issuer.owner_before(source.action_self_)&&!source.action_self_.owner_before(r.issuer)&&
      !r.issuer.owner_before(p.issuer)&&!p.issuer.owner_before(r.issuer)&&!r.world.owner_before(p.world)&&!p.world.owner_before(r.world)&&
      r.generation==generation&&p.generation==generation&&r.serial==serial&&r.previous==p.serial&&p.serial<UINT64_MAX&&serial==p.serial+1&&
      p.source_adopted&&p.operation<UINT64_MAX&&r.operation==p.operation+1&&r.revision==p.after&&state.ui_source_revision==r.revision&&
      p.next==BnwRetainedTurnFrontier::game_turn_player_set_damage_garrison_owner&&r.source_adopted==adopted&&
      h->operation==r.operation+(adopted?1U:0U)&&h->frontier.load()==(adopted?r.next:p.next),
      "Garrison SAME old damage/new operation/source/world phase differs");
 // Original damage receipt metadata stays unchanged; new child validates the
 // actual parent after-HP witness using its OLD serial, not this new serial.
 h->attrition_first->require_garrison_without_callbacks(core,host,state,source,control,generation,serial,true);
 const auto q=h->attrition_first->garrison_progress_without_callbacks();
 need(q.executed&&!q.failed&&q.previous_source_serial==p.serial&&q.source_serial==serial&&q.revision==r.revision&&
      r.next==(q.boundary==BnwAttritionGarrisonBoundary::city_strength_owner?
       BnwRetainedTurnFrontier::game_turn_player_set_damage_city_strength_owner:
       BnwRetainedTurnFrontier::game_turn_player_set_damage_info_bar_dirty_owner),
      "Garrison physical NULL/equal/required consumer boundary differs");
}
void BnwRetainedTurnCursor::fail_attrition_garrison_without_callbacks()noexcept{
 const auto h=impl_->history;if(h->attrition_first)h->attrition_first->fail_garrison_without_callbacks();h->quarantine();
}
BnwAttritionGarrisonProgress BnwRetainedTurnCursor::attrition_garrison_progress()const{
 const auto h=impl_->history;h->thread_check();return h->attrition_first?h->attrition_first->garrison_progress_without_callbacks():BnwAttritionGarrisonProgress{};
}
}
