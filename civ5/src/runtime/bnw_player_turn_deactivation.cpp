#include "civ5/runtime/bnw_player_turn_deactivation.hpp"
#include "civ5/runtime/file_identity.hpp"
#include <climits>
#include <stdexcept>
namespace civ5::runtime {
namespace {
void need(bool b,const char *why){if(!b)throw std::runtime_error(why);}
std::int64_t &core(saves::PlayerCoreObservation &c,std::string_view n){saves::PlayerCoreField *out=nullptr;for(auto &f:c.fields)if(f.name && std::string_view(f.name)==n){need(!out && f.values.size()==1,"Unknown deactivation core scalar");out=&f;}need(out && (out->encoding==saves::PlayerCoreEncoding::boolean || out->encoding==saves::PlayerCoreEncoding::i32),"Missing native deactivation scalar");return out->values[0];}
std::int64_t core(const saves::PlayerCoreObservation &c,std::string_view n){return core(const_cast<saves::PlayerCoreObservation&>(c),n);}
std::int64_t &admin(saves::PlayerAdministrationObservation &a,std::string_view n){saves::NamedSaveInteger *out=nullptr;const auto v=[&](auto &&self,saves::PlayerAdministrationRecord &r)->void{for(auto &f:r.fields)if(f.name && std::string_view(f.name)==n){need(!out,"Duplicate turn-end blocking field");out=&f;}for(auto &c:r.children)self(self,c);};for(auto &r:a.blocks)v(v,r);need(out && out->value>=INT32_MIN && out->value<=INT32_MAX,"Missing/native-range turn-end blocking field");return out->value;}
std::int64_t admin(const saves::PlayerAdministrationObservation &a,std::string_view n){return admin(const_cast<saves::PlayerAdministrationObservation&>(a),n);}
void number(std::vector<std::uint8_t>&b,std::uint64_t v){for(int n=0;n<8;++n)b.push_back(v>>(8*n));}
void text(std::vector<std::uint8_t>&b,std::string_view s){number(b,s.size());b.insert(b.end(),s.begin(),s.end());}
void record(std::vector<std::uint8_t>&b,const saves::PlayerAdministrationRecord &r){text(b,r.name?r.name:"");number(b,r.index);number(b,r.fields.size());for(const auto &f:r.fields){text(b,f.name?f.name:"");number(b,f.value);}number(b,r.arrays.size());for(const auto &a:r.arrays){text(b,a.name?a.name:"");number(b,a.columns);number(b,a.values.size());for(const auto v:a.values)number(b,v);}number(b,r.blobs.size());for(const auto &blob:r.blobs){text(b,blob.name?blob.name:"");number(b,blob.bytes.size());b.insert(b.end(),blob.bytes.begin(),blob.bytes.end());}number(b,r.children.size());for(const auto &c:r.children)record(b,c);}
}
std::string bnw_turn_end_world_revision(const LiveSessionState &state,const SessionCheckpointIdentity &identity,const content::RulesDatabase &rules,std::int64_t player) {
 const auto unit_revision=bnw_unit_order_world_revision(state,identity,rules);need(player>=0 && player<63 && state.world.pregame->archive.integers.at("active_player")==player && state.setup.is_human(state.setup.players.at(player)),"Turn-end client needs actual local-human player");
 std::vector<std::uint8_t>b; text(b,unit_revision);number(b,player);const auto &a=state.world.all_players->players.at(player).administration;
 need(a.layout==saves::GameStateLayout::expansion2_version1,"Turn-end administration layout unknown");for(const auto &r:a.blocks)record(b,r);
 return identify_bytes(b).sha256;
}
BnwTurnEndUiOwner start_bnw_turn_end_ui_client(const LiveSessionState &state,const SessionCheckpointIdentity &identity,const content::RulesDatabase &rules,std::int64_t player) {
 BnwTurnEndUiOwner out(identity,state.world.map_header.guid,bnw_turn_end_world_revision(state,identity,rules,player),player);
 const auto &p=state.world.all_players->players.at(player);const auto active=core(p.core,"m_bTurnActive");need(active==0 || active==1,"New turn-end client active flag unknown");out.turn_active_=active!=0;
 out.blocking_type_=admin(p.administration,"m_eEndTurnBlockingType");out.blocking_notification_=admin(p.administration,"m_iEndTurnBlockingNotificationIndex");return out;
}
struct BnwDeactivationStagingAccess {
 static std::unique_ptr<BnwDeactivationFanoutDraft> retain(
  const BnwLocalHumanAutoMovesDraft &input,BnwEndTurnRequestDraft request,
  BnwUnitPresentationOwner units,BnwTurnEndUiOwner turns,
  BnwNotificationCleanupOwner notifications,BnwNotificationUiOwner notification_client,
  BnwDiplomacyTurnOwner diplomacy,const content::RulesDatabase &rules) {
  const auto &world=request.state;
  need(!turns.turn_active() && turns.active_player_turn_ended() &&
   !turns.player_status_ended() && turns.active_count_debug_receipts().empty() &&
   turns.world_revision()==bnw_turn_end_world_revision(world,turns.identity(),rules,turns.player()) &&
   units.world_revision()==bnw_unit_order_world_revision(world,units.identity(),rules) &&
   notifications.epoch()==bnw_notification_epoch(world,notifications.identity(),rules) &&
   notification_client.epoch()==notifications.epoch() &&
   diplomacy.epoch()==bnw_diplomacy_turn_epoch(world,diplomacy.identity(),rules),
   "Cannot retain mismatched native world/client epochs before turn-end fanout");
  auto human=input;
  human.request=std::move(request);
  return std::unique_ptr<BnwDeactivationFanoutDraft>(new BnwDeactivationFanoutDraft(
   std::move(human),std::move(units),std::move(turns),std::move(notifications),
   std::move(notification_client),std::move(diplomacy)));
 }
};
void apply_bnw_turn_end_ui_batch(const LiveSessionState &state,BnwTurnEndUiOwner &owner,const content::RulesDatabase &rules,const BnwTurnEndUiBatch &batch,const BnwLocalAiBarrierOwner *engine) {
 need(owner.identity_.payload_sha256==batch.identity.payload_sha256 && owner.identity_.rules_sha256==batch.identity.rules_sha256 && owner.map_guid_==batch.map_guid && batch.map_guid==state.world.map_header.guid && batch.before_revision==owner.world_revision_ && batch.after_revision==bnw_turn_end_world_revision(state,owner.identity_,rules,owner.player_) && batch.expected_client_revision==owner.client_revision_ && owner.client_revision_<UINT64_MAX,"Turn-end actual UI feed stale/source mismatch");
 const auto &p=state.world.all_players->players.at(owner.player_);need(core(p.core,"m_bTurnActive")==0,"Turn-end UI events require actual inactive after-world");auto d=owner;
 for(const auto &e:batch.effects) {
  need(e.player==owner.player_,"Turn-end UI feed wrong local player");
  switch(e.kind) {
   case BnwTurnEndUiEffectKind::publish_end_turn_dirty:need(e.previous_type==-1 && e.new_type==-1 && e.active_count==-1,"End-turn dirty payload invalid");d.turn_active_=false;d.end_turn_dirty_=true;break;
   case BnwTurnEndUiEffectKind::blocking_changed:need(!d.received_effects_.empty() && d.received_effects_.back().kind==BnwTurnEndUiEffectKind::publish_end_turn_dirty && e.previous_type==d.blocking_type_ && e.new_type==admin(p.administration,"m_eEndTurnBlockingType") && e.new_type==-1 && admin(p.administration,"m_iEndTurnBlockingNotificationIndex")==-1,"Turn-end blocking event mismatches source order/actual fields");d.blocking_type_=e.new_type;d.blocking_notification_=-1;break;
   case BnwTurnEndUiEffectKind::update_end_turn:need(!d.received_effects_.empty() && d.received_effects_.back().kind==BnwTurnEndUiEffectKind::blocking_changed && e.previous_type==-1 && e.new_type==-1 && d.blocking_type_==admin(p.administration,"m_eEndTurnBlockingType") && d.blocking_notification_==admin(p.administration,"m_iEndTurnBlockingNotificationIndex") && d.end_turn_updates_<UINT64_MAX,"UpdateEndTurn lacks actual blocking event/state");++d.end_turn_updates_;break;
   case BnwTurnEndUiEffectKind::publish_active_player_turn_end:
    need(d.end_turn_dirty_ && !d.active_player_turn_ended_ && !d.player_status_ended_ && admin(p.administration,"m_eEndTurnBlockingType")==-1 && admin(p.administration,"m_iEndTurnBlockingNotificationIndex")==-1 && e.previous_type==-1 && e.new_type==-1 && e.active_count==-1,"ActivePlayerTurnEnd wrong order/payload/native fields");d.active_player_turn_ended_=true;break;
   case BnwTurnEndUiEffectKind::change_num_active_debug:
    need(engine && engine->participant()==owner.player_ && has_received_bnw_local_turn_all_complete(state,*engine,rules) && d.active_player_turn_ended_ && !d.player_status_ended_ && d.active_count_debug_receipts_.empty() && e.previous_type==-1 && e.new_type==-1 && e.active_count==bnw_notification_game_turn_active_count(state),"changeNumGameTurnActive(-1) wrong source order/count/payload");d.active_count_debug_receipts_.push_back(e.active_count);break;
   case BnwTurnEndUiEffectKind::player_turn_status_end:
    need(engine && engine->participant()==owner.player_,"PlayerTurnStatus(END) missing SAME local transport");
    if(!has_received_bnw_local_turn_all_complete(state,*engine,rules)){need(std::holds_alternative<saves::Expansion2PrefixObservation>(state.world.game_state.prefix) && std::get<saves::Expansion2PrefixObservation>(state.world.game_state.prefix).version==1 && std::get<saves::Expansion2PrefixObservation>(state.world.game_state.prefix).scalars[21]==0,"Typed status consumer needs actual normalized zero-autoplay mode when all-complete false");}
    need((has_received_bnw_local_turn_all_complete(state,*engine,rules)==!d.active_count_debug_receipts_.empty()) && d.active_player_turn_ended_ && !d.player_status_ended_ && e.previous_type==-1 && e.new_type==-1 && e.active_count==-1,"PlayerTurnStatus(END) wrong source order/payload/conditional receipt");d.player_status_ended_=true;break;
   default:throw std::runtime_error("Unknown turn-end UI effect");
  }
  d.received_effects_.push_back(e);
 }
 d.world_revision_=batch.after_revision;++d.client_revision_;owner=std::move(d);
}
static BnwDeactivationPrefixExecution deactivation_prefix(const BnwDiplomacyTurnOwner *diplomacy_owner,const BnwNotificationCleanupOwner *notification_owner,const BnwNotificationUiOwner *notification_client,const BnwLocalHumanAutoMovesDraft &input,const BnwEndTurnInputBinding &binding,const BnwUnitPresentationOwner &unit_client,const BnwTurnEndUiOwner &turn_client,const content::RulesDatabase &rules,const content::InfoTypeCatalog &types,std::int64_t player) {
 BnwDeactivationPrefixExecution out;
 try {
  const auto &before=input.request;(void)bnw_end_turn_input_revision(before.state,before.owner,binding,rules,player);
  need(core(before.state.world.all_players->players.at(player).core,"m_bEndTurn")==1 && before.owner.processed_auto_moves.at(player) && input.processed_auto_moves,"CheckPlayerTurnDeactivate requires actual EndTurn/processed automoves");out.completed_steps.push_back(BnwDeactivationStep::end_request_and_processed);
  need(before.owner.busy_state.has_value(),"CheckPlayerTurnDeactivate missing SAME actual busy owner");const auto busy=observe_bnw_busy_player(before.state,*before.owner.busy_state,before.owner.identity,player);out.completed_steps.push_back(BnwDeactivationStep::busy_query);
  if(!busy.busy_units.empty() || !busy.busy_cities.empty()){need(has_received_bnw_local_turn_complete(before.state,input.engine,rules),"Busy human has no actually delivered turn-complete receipt");out.completed_steps.push_back(BnwDeactivationStep::received_turn_complete);}
  need(turn_client.player()==player && turn_client.world_revision()==bnw_turn_end_world_revision(before.state,before.owner.identity,rules,player) && turn_client.identity().payload_sha256==before.owner.identity.payload_sha256 && turn_client.identity().rules_sha256==before.owner.identity.rules_sha256,"Deactivation missing current actual turn-end UI owner");
  auto request=before;auto units=unit_client;auto turns=turn_client;auto &p=request.state.world.all_players->players.at(player);
  BnwTurnEndUiBatch batch{before.owner.identity,before.state.world.map_header.guid,turn_client.world_revision(),"",turn_client.client_revision(),{}};
  const auto consume=[&](std::vector<BnwTurnEndUiEffect> effects){BnwTurnEndUiBatch feed{request.owner.identity,request.state.world.map_header.guid,turns.world_revision(),bnw_turn_end_world_revision(request.state,request.owner.identity,rules,player),turns.client_revision(),std::move(effects)};apply_bnw_turn_end_ui_batch(request.state,turns,rules,feed,&input.engine);};
  core(p.core,"m_bTurnActive")=0;out.completed_steps.push_back(BnwDeactivationStep::turn_active_false);
  batch.effects.push_back({BnwTurnEndUiEffectKind::publish_end_turn_dirty,player});consume({batch.effects.back()});out.completed_steps.push_back(BnwDeactivationStep::publish_end_turn_dirty);
  auto &type=admin(p.administration,"m_eEndTurnBlockingType"),&index=admin(p.administration,"m_iEndTurnBlockingNotificationIndex");const auto old=type;const bool changed=type!=-1 || index!=-1;type=-1;index=-1;out.completed_steps.push_back(BnwDeactivationStep::blocking_stores);
  if(changed){batch.effects.push_back({BnwTurnEndUiEffectKind::blocking_changed,player,old,-1});out.completed_steps.push_back(BnwDeactivationStep::blocking_changed);batch.effects.push_back({BnwTurnEndUiEffectKind::update_end_turn,player});out.completed_steps.push_back(BnwDeactivationStep::update_end_turn);consume({batch.effects[1],batch.effects[2]});}
  // Bind the actual unit client to this player-flag epoch; no unit event is
  // invented for a player flag. The source turn-end event has its own owner.
  BnwUnitPresentationBatch epoch{before.owner.identity,request.state.world.map_header.guid,units.world_revision(),bnw_unit_order_world_revision(request.state,request.owner.identity,rules),units.client_revision(),{}};
  apply_bnw_unit_presentation_batch(request.state,units,rules,epoch);
  auto reset=bnw_unit_reset_detail::execute_after_turn_flag_draft(before,request,binding,units,rules,types,player);need(bool(reset.draft),reset.blocker.c_str());out.completed_steps.push_back(BnwDeactivationStep::unit_reset);
  request=std::move(reset.draft->request);units=std::move(reset.draft->presentation);
  // Native unit stores changed the world epoch, not the turn-end event list.
  consume({});
  BnwDeactivationPrefixObservation proof;proof.private_turn_active=turns.turn_active();proof.private_blocking_type=turns.blocking_type();proof.private_blocking_notification=turns.blocking_notification();proof.reset_units=reset.draft->reset_units;proof.consumed_ui_effects=batch.effects;
  const auto &notification=request.state.notification_load_transients;proof.notification_transients_known=notification.current && notification.players.contains(player);if(proof.notification_transients_known)proof.notifications_pending=notification.players.at(player).active_undismissed.size();out.prefix=std::move(proof);out.completed_steps.push_back(BnwDeactivationStep::notification_boundary);
  need(notification_owner && notification_client,"setTurnActive(false) requires actual retained notification EndOfTurnCleanup/Dismiss owner; loaded transients are read-only, no turn completed");
  auto cleaned=execute_bnw_notification_end_turn_cleanup_draft(request.state,*notification_owner,*notification_client,rules,player);need(bool(cleaned.draft),cleaned.blocker.c_str());
  request.state=std::move(cleaned.draft->state);consume({});
  out.prefix->notification_cleanup_complete=true;out.prefix->removed_notifications=cleaned.draft->batch.removals;out.prefix->waited_notification_slots=cleaned.draft->waited_slots;out.prefix->native_active_players_after=cleaned.draft->active_players_after;
  out.prefix->notifications_pending=request.state.notification_load_transients.players.contains(player)?request.state.notification_load_transients.players.at(player).active_undismissed.size():0;
  out.completed_steps.push_back(BnwDeactivationStep::notification_cleanup);out.completed_steps.push_back(BnwDeactivationStep::diplomacy_boundary);
  need(diplomacy_owner,"setTurnActive(false) requires actual retained CvDiplomacyRequests::EndTurn owner and subsequent mandatory turn-end event receivers; no turn completed");
  auto diplo=execute_bnw_diplomacy_end_turn_draft(request.state,*diplomacy_owner,rules,player);need(bool(diplo.draft),diplo.blocker.c_str());
  request.state=std::move(diplo.draft->state);consume({});out.prefix->diplomacy_end_turn_complete=true;out.prefix->diplomacy_next_ai=diplo.draft->after_next_ai;out.completed_steps.push_back(BnwDeactivationStep::diplomacy_end_turn);
  // The new Civis receiver really consumes its typed effect, but the source
  // DLLUI interface gives no proven asynchronous enqueue/fanout semantics.
  // Mandatory selected Lua listeners cannot be put after the next mechanics.
  consume({{BnwTurnEndUiEffectKind::publish_active_player_turn_end,player}});
  out.prefix->active_player_end_typed_receiver_consumed=turns.active_player_turn_ended();
  out.completed_steps.push_back(BnwDeactivationStep::active_player_end_publication_boundary);
  out.pending_fanout=BnwDeactivationStagingAccess::retain(input,std::move(request),
   std::move(units),std::move(turns),std::move(cleaned.draft->owner),
   std::move(cleaned.draft->client),std::move(diplo.draft->owner),rules);
  need(false,"PublishActivePlayerTurnEnd requires verified mandatory native/Lua listener fanout on SAME staged world; typed client consumed only, no async enqueue proof. Subsequent HasReceivedTurnAllComplete/status/isDiploActive/next activation not executed; no turn completed");
 }catch(const std::exception &e){out.blocker=e.what();}
 return out;
}
BnwDeactivationPrefixExecution execute_bnw_local_human_deactivation_prefix_draft(const BnwLocalHumanAutoMovesDraft &input,const BnwEndTurnInputBinding &binding,const BnwUnitPresentationOwner &units,const BnwTurnEndUiOwner &turns,const content::RulesDatabase &rules,const content::InfoTypeCatalog &types,std::int64_t player){return deactivation_prefix(nullptr,nullptr,nullptr,input,binding,units,turns,rules,types,player);}
BnwDeactivationPrefixExecution execute_bnw_local_human_deactivation_notifications_prefix_draft(const BnwLocalHumanAutoMovesDraft &input,const BnwEndTurnInputBinding &binding,const BnwUnitPresentationOwner &units,const BnwTurnEndUiOwner &turns,const BnwNotificationCleanupOwner &notifications,const BnwNotificationUiOwner &client,const content::RulesDatabase &rules,const content::InfoTypeCatalog &types,std::int64_t player){return deactivation_prefix(nullptr,&notifications,&client,input,binding,units,turns,rules,types,player);}
BnwDeactivationPrefixExecution execute_bnw_local_human_deactivation_suffix_prefix_draft(const BnwLocalHumanAutoMovesDraft &input,const BnwEndTurnInputBinding &binding,const BnwUnitPresentationOwner &units,const BnwTurnEndUiOwner &turns,const BnwNotificationCleanupOwner &notifications,const BnwNotificationUiOwner &client,const BnwDiplomacyTurnOwner &diplomacy,const content::RulesDatabase &rules,const content::InfoTypeCatalog &types,std::int64_t player){return deactivation_prefix(&diplomacy,&notifications,&client,input,binding,units,turns,rules,types,player);}

struct BnwDeactivationUiFanoutAccess {
 static BnwDeactivationUiFanoutExecution dispatch(std::unique_ptr<BnwDeactivationFanoutDraft> input,
  const SavedContentProfile &selected,BnwGameplayLuaEpochOwner &lua,const content::RulesDatabase &rules) {
  BnwDeactivationUiFanoutExecution out;
  bool input_validated=false;
  try {
   need(input!=nullptr,"ActivePlayerTurnEnd prefix absent or already consumed");
   const auto &id=input->activation().identity;
   const auto p=input->turns().player();
   const auto validate_clients=[&](const LiveSessionState &world){
    need(world.world.map_header.guid==input->turns().map_guid() &&
     !input->turns().turn_active() && input->turns().active_player_turn_ended() &&
     !input->turns().player_status_ended() && input->activation().completed.empty() &&
     input->turns().world_revision()==bnw_turn_end_world_revision(world,id,rules,p) &&
     input->units().world_revision()==bnw_unit_order_world_revision(world,id,rules) &&
     input->notifications().epoch()==bnw_notification_epoch(world,id,rules) &&
     input->notification_client().epoch()==input->notifications().epoch() &&
     input->diplomacy().epoch()==bnw_diplomacy_turn_epoch(world,id,rules),
     "ActivePlayerTurnEnd native/client operand epochs changed; required rebinding unavailable");
   };
   validate_clients(input->state());
   input_validated=true;
   auto fork=lua.fork_at(input->state());
   need(fork.epoch!=nullptr,fork.blocker.empty()?"Actual selected turn-end Lua epoch unavailable":fork.blocker.c_str());
   auto &e=*fork.epoch;
   need(e.world && e.host && e.source_world==&input->state() && e.getter_world==e.world.get() &&
    e.identity.payload_sha256==id.payload_sha256 && e.identity.rules_sha256==id.rules_sha256 &&
    e.selected_dlc==selected.enabled_dlc && e.selected_mods==selected.enabled_mods &&
    e.all_selected_contexts_loaded && e.all_listeners_registered && e.all_external_writes_escrowed &&
    e.host->loading_complete(),"ActivePlayerTurnEnd selected registry/getters/loading/escrow incomplete or stale");
   validate_clients(*e.world);
   out.dispatch_attempted=true;
   const auto delivered=e.host->emit_ui_event("ActivePlayerTurnEnd");
   need(delivered.errors.empty(),delivered.errors.empty()?"ActivePlayerTurnEnd callback failed":delivered.errors.front().c_str());
   need(delivered.listeners>0,"ActivePlayerTurnEnd selected consumers absent; empty registry does not attest delivery");
   need(e.host->loading_complete(),"ActivePlayerTurnEnd callback invalidated selected loading");
   validate_clients(*e.world);
   out.draft=std::unique_ptr<BnwDeactivationUiFanoutDraft>(new BnwDeactivationUiFanoutDraft(
    std::move(input),std::move(fork.epoch),delivered.listeners));
   out.blocker="ActivePlayerTurnEnd delivered on retained private selected epoch; AllComplete/status/next activation and atomic publication remain";
  }catch(const std::exception &error){
   out.blocker=error.what();
   if(input_validated&&!out.dispatch_attempted)out.pending=std::move(input);
  }
  return out;
 }
};
BnwDeactivationUiFanoutExecution dispatch_bnw_deactivation_active_player_turn_end(
 std::unique_ptr<BnwDeactivationFanoutDraft> input,const SavedContentProfile &selected,
 BnwGameplayLuaEpochOwner &lua,const content::RulesDatabase &rules) {
 return BnwDeactivationUiFanoutAccess::dispatch(std::move(input),selected,lua,rules);
}

struct BnwDeactivationStatusAccess {
 static BnwDeactivationStatusExecution run(std::unique_ptr<BnwDeactivationUiFanoutDraft> input,
  const content::RulesDatabase &rules){
  BnwDeactivationStatusExecution out;
  try{
   need(input&&input->listeners()>0,"Turn-end status requires unconsumed delivered ActivePlayerTurnEnd epoch");
   const auto &world=input->state();const auto &prefix=input->native_prefix();
   const auto &id=prefix.activation().identity;const auto player=prefix.turns().player();
   need(prefix.turns().world_revision()==bnw_turn_end_world_revision(world,id,rules,player)&&
    prefix.units().world_revision()==bnw_unit_order_world_revision(world,id,rules)&&
    prefix.notifications().epoch()==bnw_notification_epoch(world,id,rules)&&
    prefix.notification_client().epoch()==prefix.notifications().epoch()&&
    prefix.diplomacy().epoch()==bnw_diplomacy_turn_epoch(world,id,rules),
    "Turn-end status native/client operands changed after fanout");
   const auto &p=world.world.all_players->players.at(player);
   need(world.setup.is_human(world.setup.players.at(player))&&core(p.core,"m_bAlive")==1&&
    core(p.core,"m_bTurnActive")==0&&
    std::holds_alternative<saves::Expansion2PrefixObservation>(world.world.game_state.prefix)&&
    std::get<saves::Expansion2PrefixObservation>(world.world.game_state.prefix).version==1&&
    std::get<saves::Expansion2PrefixObservation>(world.world.game_state.prefix).scalars[21]==0,
    "Turn-end status requires supported living local human and zero native AIAutoPlay");
   const bool received=has_received_bnw_local_turn_all_complete(world,prefix.engine(),rules);
   std::vector<BnwDeactivationStep> steps{BnwDeactivationStep::all_complete_query};
   auto turns=prefix.turns();
   BnwTurnEndUiBatch batch{id,world.world.map_header.guid,turns.world_revision(),
    bnw_turn_end_world_revision(world,id,rules,player),turns.client_revision(),{}};
   if(received){batch.effects.push_back({BnwTurnEndUiEffectKind::change_num_active_debug,
    player,-1,-1,bnw_notification_game_turn_active_count(world)});
    steps.push_back(BnwDeactivationStep::change_num_active_debug);}
   batch.effects.push_back({BnwTurnEndUiEffectKind::player_turn_status_end,player});
   apply_bnw_turn_end_ui_batch(world,turns,rules,batch,&prefix.engine());
   steps.push_back(BnwDeactivationStep::player_turn_status_end);
   out.draft=std::unique_ptr<BnwDeactivationStatusDraft>(new BnwDeactivationStatusDraft(
    std::move(input),std::move(turns),received,std::move(steps)));
   out.blocker="Native AllComplete query and private typed status consumed; mandatory external status delivery, outer deactivation and atomic publication remain";
  }catch(const std::exception &error){out.blocker=error.what();}
  return out;
 }
};
BnwDeactivationStatusExecution continue_bnw_deactivation_status(
 std::unique_ptr<BnwDeactivationUiFanoutDraft> input,const content::RulesDatabase &rules){
 return BnwDeactivationStatusAccess::run(std::move(input),rules);
}
} // namespace civ5::runtime
