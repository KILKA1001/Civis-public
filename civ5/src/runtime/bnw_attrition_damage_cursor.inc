// Included after existing Mountain1 Cursor methods; common TU owned by Root.
namespace civ5::runtime {
std::uint64_t BnwRetainedTurnCursor::attrition_damage_previous_serial_without_callbacks()const{
 const auto h=impl_->history;h->thread_check();
 need(h->attrition_first_return&&h->attrition_first,"Attrition damage real predecessor absent");
 return h->attrition_mountain_return?h->attrition_mountain_return->serial:h->attrition_first_return->serial;
}
void BnwRetainedTurnCursor::require_attrition_damage_parent_without_callbacks(const BnwNativeUnitAllocatorCore&core,
 const ui::SelectedBnwPreviewHost&host,const LiveSessionState&state,const BnwSelectedStockLuaSourceOwner&source,
 const void*control,std::uint64_t generation,std::uint64_t previous)const{
 const auto h=impl_->history;h->thread_check();
 need(!h->retired.load()&&!h->poisoned.load()&&h->attrition_first_return&&h->attrition_first,
      "Attrition damage parent retired or absent");
 if(h->attrition_mountain_return){
  need(h->frontier.load()==BnwRetainedTurnFrontier::game_turn_player_attrition_mountain_damage_owner&&
       h->attrition_mountain_return->serial==previous,"Attrition damage genuine true mountain frontier absent");
  require_attrition_mountain_return_without_callbacks(core,host,state,source,control,generation,previous,true);
 }else{
  need(h->frontier.load()==BnwRetainedTurnFrontier::game_turn_player_attrition_damage_owner&&
       h->attrition_first_return->serial==previous,"Attrition damage genuine territory hit frontier absent");
  require_attrition_first_return_without_callbacks(core,host,state,source,control,generation,previous,true);
 }
}
void BnwRetainedTurnCursor::require_attrition_damage_entry_without_callbacks(const BnwNativeUnitAllocatorCore&core,
 const BnwSelectedStockLuaSourceOwner&source)const{
 const auto h=impl_->history;h->thread_check();
 if(h->attrition_damage_attempted){if(h->attrition_first)h->attrition_first->fail_damage_without_callbacks();
  h->quarantine();need(false,"Attrition damage actual once reentry");}
 need(!h->attrition_damage_return,"Attrition damage preexisting actual return");
 source.require_attrition_damage_parent_source_without_callbacks(*this,core,nullptr,true);
}
void BnwRetainedTurnCursor::execute_attrition_damage_with_actual_stock(const BnwNativeUnitAllocatorCore&core,
 const BnwSelectedStockLuaSourceOwner&source,const BnwStockLuaAttemptLease&lease,
 const std::shared_ptr<const ui::LiveUiHost>&supplied_ui){
 const auto i=impl_;auto&h=*i->history;h.thread_check();
 need(!h.attrition_damage_attempted&&!h.attrition_damage_return&&h.attrition_first&&h.operation<UINT64_MAX,
      "Attrition damage actual once entry absent");h.attrition_damage_attempted=true;
 try{i->check_engine();const auto serial=lease.operation_serial();lease.require_operation(serial);
  source.require_attrition_damage_parent_source_without_callbacks(*this,core,&lease,false);
  const auto allocation=i->session->state_;auto&state=*allocation;History::AttritionDamageReturn staged;
  staged.issuer=source.action_self_;staged.world=allocation;staged.generation=h.generation;
  staged.serial=serial;staged.previous=attrition_damage_previous_serial_without_callbacks();
  staged.operation=h.operation;staged.before=state.ui_source_revision;staged.entry=h.frontier.load();
  const auto actual_ui=i->cycle_ui.lock();
  need(actual_ui&&supplied_ui&&actual_ui.get()==supplied_ui.get()&&
       !actual_ui.owner_before(supplied_ui)&&!supplied_ui.owner_before(actual_ui)&&actual_ui->engine_ui_turn_owner_,
       "Attrition damage supplied UI is not SAME genuine retained cycle UI");
  const auto control=require_actual_ui(*actual_ui);const auto engine=actual_ui->engine_ui_turn_owner_;
  ui::BnwEngineUiTurnOwner::EndTurnCursorPin pin(*engine,*actual_ui,state);
  const std::weak_ptr<ui::BnwEngineUiTurnOwner> weak_engine=engine;
  source.issue_attrition_damage_owned(*this,core,lease,*h.attrition_first,actual_ui,weak_engine);
  engine->require_current();i->check_engine();lease.require_operation(serial);
  need(require_actual_ui(*actual_ui)==control&&actual_ui->engine_ui_turn_owner_==engine,
       "Attrition damage actual Engine/Selection receiver substituted");
  source.require_attrition_damage_parent_source_without_callbacks(*this,core,&lease,false);
  source.seal_attrition_damage_without_callbacks(core,*h.attrition_first,lease,false);
  need(i->session->state_==allocation&&state.ui_source_revision==staged.before&&h.operation==staged.operation,
       "Attrition damage exact world replaced before physical stores");
  static_assert(std::is_nothrow_move_constructible_v<History::AttritionDamageReturn>);
  h.attrition_damage_return.emplace(std::move(staged));auto&earned=*h.attrition_damage_return;
  h.attrition_first->execute_damage_without_callbacks(state);
  const auto q=h.attrition_first->damage_progress_without_callbacks();earned.after=q.after_revision;
  earned.next=q.damage_changed?BnwRetainedTurnFrontier::game_turn_player_set_damage_garrison_owner:
                              BnwRetainedTurnFrontier::game_turn_player_set_damage_death_owner;
  // Adopt ONLY the authentic just-executed partial child before callbackful POST.
  source.adopt_actual_attrition_damage_return(*this,core,lease);earned.source_adopted=true;
  ++h.operation;h.frontier.store(earned.next);
  i->check_engine();lease.require_operation(serial);
  source.require_attrition_damage_source_without_callbacks(*this,core,&lease,false);
 }catch(...){fail_attrition_damage_without_callbacks();throw;}
}
void BnwRetainedTurnCursor::require_attrition_damage_return_without_callbacks(const BnwNativeUnitAllocatorCore&core,
 const ui::SelectedBnwPreviewHost&host,const LiveSessionState&state,const BnwSelectedStockLuaSourceOwner&source,
 const void*control,std::uint64_t generation,std::uint64_t serial,bool adopted)const{
 const auto h=impl_->history;h->thread_check();
 need(!h->retired.load()&&!h->poisoned.load()&&h->attrition_damage_attempted&&h->attrition_damage_return&&
      h->attrition_first&&h->attrition_first_return,"Attrition damage actual retained partial child absent");
 const auto&r=*h->attrition_damage_return;const auto issuer=r.issuer.lock();const auto world=r.world.lock();
 const auto&first=*h->attrition_first_return;const auto first_issuer=first.issuer.lock();const auto first_world=first.world.lock();
 need(issuer&&issuer.get()==&source&&!r.issuer.owner_before(source.action_self_)&&!source.action_self_.owner_before(r.issuer)&&
      world&&world.get()==&state&&first_issuer&&first_issuer.get()==&source&&first_world&&first_world.get()==&state&&
      !first.issuer.owner_before(r.issuer)&&!r.issuer.owner_before(first.issuer)&&
      !first.world.owner_before(r.world)&&!r.world.owner_before(first.world)&&first.source_adopted&&first.generation==generation&&
      r.generation==generation&&r.serial==serial&&r.previous<UINT64_MAX&&serial==r.previous+1&&r.source_adopted==adopted&&
      h->operation==r.operation+(adopted?1U:0U)&&h->frontier.load()==(adopted?r.next:r.entry)&&
      r.before==first.after&&state.ui_source_revision==r.after,
      "Attrition damage SAME Source/world/operation/previous/revision differs");
 // Old receipt records are unchanged. Their OLD physical validators are not
 // invoked with this new serial or after HP; the private exact child guards all64.
 if(h->attrition_mountain_return){const auto&m=*h->attrition_mountain_return;
  need(h->attrition_mountain_attempted&&m.source_adopted&&m.generation==generation&&m.serial==first.serial+1&&
       m.operation==first.operation+1&&m.revision==first.after&&m.serial==r.previous&&
       m.operation<UINT64_MAX&&r.operation==m.operation+1&&m.next==r.entry&&
       r.entry==BnwRetainedTurnFrontier::game_turn_player_attrition_mountain_damage_owner&&
       !m.issuer.owner_before(r.issuer)&&!r.issuer.owner_before(m.issuer)&&
       !m.world.owner_before(r.world)&&!r.world.owner_before(m.world),
       "Attrition damage original Mountain parent record changed");
 }else need(first.serial==r.previous&&first.operation<UINT64_MAX&&r.operation==first.operation+1&&
            first.next==r.entry&&r.entry==BnwRetainedTurnFrontier::game_turn_player_attrition_damage_owner,
            "Attrition damage original territory parent record changed");
 h->attrition_first->require_damage_without_callbacks(core,host,state,source,control,generation,serial,true);
 const auto q=h->attrition_first->damage_progress_without_callbacks();
 need(q.executed&&!q.failed&&q.before_revision==r.before&&q.after_revision==r.after&&
      q.previous_source_serial==r.previous&&q.source_serial==r.serial&&
      r.next==(q.damage_changed?BnwRetainedTurnFrontier::game_turn_player_set_damage_garrison_owner:
                              BnwRetainedTurnFrontier::game_turn_player_set_damage_death_owner),
      "Attrition damage required native suffix/physical partial history differs");
}
void BnwRetainedTurnCursor::fail_attrition_damage_without_callbacks()noexcept{
 const auto h=impl_->history;if(h->attrition_first)h->attrition_first->fail_damage_without_callbacks();h->quarantine();
}
BnwAttritionDamageProgress BnwRetainedTurnCursor::attrition_damage_progress()const{
 const auto h=impl_->history;h->thread_check();
 return h->attrition_first?h->attrition_first->damage_progress_without_callbacks():BnwAttritionDamageProgress{};
}
}
