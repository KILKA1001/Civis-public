#include "civ5/runtime/bnw_player_turn_deactivation.hpp"
#include "civ5/runtime/bnw_unit_finish_moves.hpp"
#include "civ5/runtime/file_identity.hpp"
#include "civ5/ui/selected_bnw_preview_host.hpp"
#include <iostream>
#include <stdexcept>
using namespace civ5;using namespace civ5::runtime;
namespace {
void check(bool b,const char* why){if(!b)throw std::runtime_error(why);}
template<class Core>std::int64_t& scalar(Core& c,std::string_view n){for(auto& f:c.fields)if(f.name && std::string_view(f.name)==n && f.values.size()==1)return f.values[0];throw std::runtime_error("Fixture scalar missing");}
std::int64_t& admin(saves::PlayerAdministrationObservation& a,std::string_view n){saves::NamedSaveInteger* found=nullptr;const auto walk=[&](auto&& self,saves::PlayerAdministrationRecord& v)->void{for(auto& f:v.fields)if(f.name && std::string_view(f.name)==n){check(!found,"Fixture duplicate");found=&f;}for(auto& c:v.children)self(self,c);};for(auto& b:a.blocks)walk(walk,b);check(found,"Fixture admin missing");return found->value;}
// Explicit isolated registry fixture, never attestation of stock initialization.
struct FanoutFixture final:BnwGameplayLuaEpochOwner {
 SavedContentProfile selected;SessionCheckpointIdentity identity;
 std::int64_t player=0;int forks=0,callbacks=0,publishes=0;
 bool verified=true,empty=false,fail=false,change_units=false,bad_getters=false;
 std::int64_t observed_moves=-1;
 BnwGameplayLuaForkResult fork_at(const LiveSessionState &source) override {
  ++forks;auto e=std::make_unique<BnwTeamTechLuaEpoch>();
  e->world=std::make_unique<LiveSessionState>(source);e->source_world=&source;
  e->getter_world=bad_getters?&source:e->world.get();e->identity=identity;
  e->selected_dlc=selected.enabled_dlc;e->selected_mods=selected.enabled_mods;
  e->all_selected_contexts_loaded=verified;e->all_listeners_registered=verified;e->all_external_writes_escrowed=verified;
  ScriptUiEventProvider ui;ui.notifications={"ActivePlayerTurnEnd"};ScriptMapProvider map;
  map.grid_size=[width=source.world.map_header.grid.width,height=source.world.map_header.grid.height]{
   return std::pair<std::int64_t,std::int64_t>{width,height};};
  map.random=[this,w=e->world.get()](std::int64_t,std::string_view){
   ++callbacks;auto &moves=scalar(w->world.all_players->players.at(player).units.units.front().core,"m_iMoves");
   observed_moves=moves;if(change_units)moves=1;
   else w->player_culture_bomb_timers.at(player)=7;
   return std::int64_t{0};
  };
  e->host=std::make_unique<LuaScriptHost>(ScriptBudget{},ScriptSourceProvider{},ScriptModdingProvider{},
   ScriptGameplayProvider{},ScriptSaveDataProvider{},ScriptPregameProvider{},std::move(map),std::move(ui));
  if(!empty)e->host->execute("isolated-turn-end",fail?
   "Events.ActivePlayerTurnEnd.Add(function()Map.Rand(1,'fixture');error('fixture late failure')end)":
   "Events.ActivePlayerTurnEnd.Add(function()Map.Rand(1,'fixture')end)","=isolated-turn-end");
  e->host->complete_loading();return {std::move(e),{}};
 }
 BnwGameplayLuaPublishResult publish(std::unique_ptr<BnwTeamTechLuaEpoch>,LiveSessionState &)override {
  ++publishes;return {false,false,"fixture cannot publish"};
 }
};
}
int main(int argc,char** argv){
 check(argc>=3,"Need selected save/game/[SDK]");if(argc>3){for(const auto& f:kBnwDiplomacyTurnSources)check(identify_file(std::filesystem::path(argv[3])/f.path).sha256==f.sha256,"Diplomacy source pin mismatch");for(const auto& f:kBnwTurnDeactivationSources)check(identify_file(std::filesystem::path(argv[3])/f.path).sha256==f.sha256,"Game suffix source pin mismatch");}
 auto selected=ui::bootstrap_selected_bnw_preview(argv[1],argv[2],0);check(selected.preview.host && !selected.preview.blocker,"Selected unavailable");const auto& h=*selected.preview.host;const auto& state=h.session().state();const auto& source=h.session().source();const auto player=h.active_player_id();
 auto receipt=attest_bnw_diplomacy_read_context(state,source,h.identity(),h.database());auto diplomacy=bind_bnw_diplomacy_turn_read(state,std::move(receipt),h.database());bool reused=false;try{(void)bind_bnw_diplomacy_turn_read(state,std::move(receipt),h.database());}catch(const std::exception&){reused=true;}check(reused,"Used diplomacy Read receipt cold-rebound");
 const auto initial=execute_bnw_diplomacy_end_turn_draft(state,diplomacy,h.database(),player);check(initial.draft && initial.draft->object_present && initial.draft->after_next_ai==-1 && initial.draft->owner.operations()==1 && diplomacy.operations()==0,"Actual Read EndTurn assignment failed/published");
 const auto begun=execute_bnw_diplomacy_begin_turn_scalar_draft(state,diplomacy,h.database(),player);check(begun.draft && begun.draft->after_next_ai==0 && begun.draft->state.player_read_tail.requests.at(player).next_ai_player==0,"Source BeginTurn scalar not retained");
 const auto ended=execute_bnw_diplomacy_end_turn_draft(begun.draft->state,begun.draft->owner,h.database(),player);check(ended.draft && ended.draft->before_next_ai==0 && ended.draft->after_next_ai==-1 && ended.draft->state.player_read_tail.requests.at(player).next_ai_player==-1 && begun.draft->state.player_read_tail.requests.at(player).next_ai_player==0,"Positive native EndTurn assignment differs");
 const auto repeat=execute_bnw_diplomacy_end_turn_draft(ended.draft->state,ended.draft->owner,h.database(),player);check(repeat.draft && repeat.draft->owner.operations()==3,"Repeated native EndTurn reset/rebound owner");
 const auto minor=execute_bnw_diplomacy_end_turn_draft(state,diplomacy,h.database(),63);check(minor.draft && !minor.draft->object_present,"Minor/barbarian nonexistent native request object invented");
 auto badId=state;scalar(badId.world.all_players->players.at(63).core,"m_eID")=0;check(!execute_bnw_diplomacy_end_turn_draft(badId,diplomacy,h.database(),player).draft,"Late native64 GetID mismatch accepted");
 auto unknown=state;unknown.player_read_tail.current=false;check(!execute_bnw_diplomacy_end_turn_draft(unknown,diplomacy,h.database(),player).draft,"Unknown request lifecycle inferred empty");auto payload=state;payload.player_read_tail.requests.at(player).request_count=1;check(!execute_bnw_diplomacy_end_turn_draft(payload,diplomacy,h.database(),player).draft,"Unknown queued request payload silently replaced");
 auto stale=begun.draft->state;check(!execute_bnw_diplomacy_end_turn_draft(stale,diplomacy,h.database(),player).draft,"Stale retained request epoch accepted");bool reset=false;try{(void)attest_bnw_diplomacy_read_context(stale,source,h.identity(),h.database());}catch(const std::exception&){reset=true;}check(reset,"Read attestation reset advanced nextAI value");
 auto nr=attest_bnw_notification_read_context(state,source,h.identity(),h.database());auto owner=bind_bnw_notification_cleanup_read(state,std::move(nr),h.database());auto client=start_bnw_notification_ui_client(state,owner,h.database());
 BnwGameActivationOwner activation{h.identity()};const auto binding=bind_bnw_end_turn_import(source,activation);activation.busy_state=bind_bnw_busy_state_original_import(source,h.identity());activation.pathfinder_cache=make_bnw_pathfinder_cache_control_import_owner(source,h.identity());const auto moves=bind_bnw_update_moves_original_import(source,h.identity());
 const auto endfalse=execute_bnw_end_turn_request_draft(state,activation,binding,h.database(),{player,state.setup.game_turn,false,bnw_end_turn_input_revision(state,activation,binding,h.database(),player)});check(bool(endfalse.draft),"Selected initial request absent");auto copy=state;const BnwUnitOrderKey key{player,scalar(copy.world.all_players->players.at(player).units.units.front().core,"m_iID")};auto units=start_bnw_unit_presentation_client(endfalse.draft->state,h.identity(),h.database());select_bnw_unit_presentation(endfalse.draft->state,units,h.database(),key,0);
 const auto finish=execute_bnw_finish_moves_draft(*endfalse.draft,binding,units,h.database(),{key,state.setup.game_turn,bnw_finish_moves_input_revision(*endfalse.draft,binding,units,h.database(),key),units.client_revision()});check(bool(finish.draft),"Selected Finish missing");const auto end=execute_bnw_end_turn_request_draft(finish.draft->request.state,finish.draft->request.owner,binding,h.database(),{player,state.setup.game_turn,true,finish.draft->request.input_revision});check(bool(end.draft),"Selected End missing");const auto engine=bind_bnw_local_ai_barrier_original_import(source,end.draft->state,end.draft->owner,binding,h.database(),player);const auto autos=execute_bnw_local_human_auto_moves_draft(*end.draft,binding,moves,engine,h.database(),player);check(autos.draft && autos.draft->processed_auto_moves,"Selected Auto missing");const auto turns=start_bnw_turn_end_ui_client(autos.draft->request.state,h.identity(),h.database(),player);

 check(!has_received_bnw_local_turn_all_complete(autos.draft->request.state,autos.draft->engine,h.database()),"Fresh standalone all-complete history assumed delivered");auto transport=autos.draft->engine;send_bnw_local_ai_event(autos.draft->request.state,transport,h.database(),BnwLocalAiEventKind::turn_complete);check(has_received_bnw_local_turn_complete(autos.draft->request.state,transport,h.database()) && !has_received_bnw_local_turn_all_complete(autos.draft->request.state,transport,h.database()),"TurnComplete substituted for AllComplete");
 const auto packet=prepare_bnw_local_ai_packet(autos.draft->request.state,transport,h.database(),BnwLocalAiEventKind::turn_all_complete);check(!has_received_bnw_local_turn_all_complete(autos.draft->request.state,transport,h.database()),"Undelivered AllComplete accepted");auto foreign=packet;foreign.participant=1;bool invalid=false;try{receive_bnw_local_ai_packet(autos.draft->request.state,transport,h.database(),foreign);}catch(const std::exception&){invalid=true;}check(invalid && !has_received_bnw_local_turn_all_complete(autos.draft->request.state,transport,h.database()),"Foreign AllComplete partially delivered");receive_bnw_local_ai_packet(autos.draft->request.state,transport,h.database(),packet);receive_bnw_local_ai_packet(autos.draft->request.state,transport,h.database(),packet);check(has_received_bnw_local_turn_all_complete(autos.draft->request.state,transport,h.database()),"Exact delivered AllComplete missing");
 const auto suffix=execute_bnw_local_human_deactivation_suffix_prefix_draft(*autos.draft,binding,finish.draft->presentation,turns,owner,client,diplomacy,h.database(),h.types(),player);if(!suffix.prefix || !suffix.prefix->diplomacy_end_turn_complete)std::cerr<<suffix.blocker<<'\n';check(suffix.prefix && suffix.prefix->diplomacy_end_turn_complete && suffix.prefix->diplomacy_next_ai==-1 && suffix.prefix->active_player_end_typed_receiver_consumed && !suffix.prefix->mandatory_active_player_end_fanout_complete && suffix.completed_steps.back()==BnwDeactivationStep::active_player_end_publication_boundary && suffix.blocker.find("verified mandatory native/Lua listener fanout")!=std::string::npos,"Selected native prefix bypassed synchronous publication barrier");
 auto complete=*autos.draft;complete.engine=transport;const auto held=execute_bnw_local_human_deactivation_suffix_prefix_draft(complete,binding,finish.draft->presentation,turns,owner,client,diplomacy,h.database(),h.types(),player);check(held.prefix && held.prefix->diplomacy_end_turn_complete && !held.prefix->mandatory_active_player_end_fanout_complete && held.completed_steps.back()==BnwDeactivationStep::active_player_end_publication_boundary,"Delivered AllComplete skipped mandatory earlier publication listener");
 auto late=*autos.draft;late.request.state.player_read_tail.current=false;const auto rejected=execute_bnw_local_human_deactivation_suffix_prefix_draft(late,binding,finish.draft->presentation,turns,owner,client,diplomacy,h.database(),h.types(),player);check(!rejected.blocker.empty() && (!rejected.prefix || !rejected.prefix->diplomacy_end_turn_complete) && turns.turn_active() && !turns.end_turn_dirty() && !turns.active_player_turn_ended() && !turns.player_status_ended() && diplomacy.operations()==0 && owner.operations()==0 && client.received_removals().empty() && autos.draft->request.state.player_read_tail.requests.at(player).next_ai_player==-1,"Late diplomacy blocker published native/presentation/engine owners");
 check(suffix.pending_fanout && held.pending_fanout && !rejected.pending_fanout,
       "Complete supported prefix not retained, or failed prefix exported a world");
 const auto &pending=*suffix.pending_fanout;
 const auto &held_pending=*held.pending_fanout;
 check(!pending.turns().turn_active() && pending.turns().active_player_turn_ended() &&
       !pending.turns().player_status_ended() && pending.turns().active_count_debug_receipts().empty() &&
       pending.activation().completed.empty() && pending.state().setup.game_turn==state.setup.game_turn &&
       !pending.units().dim_flags().at(key) && pending.diplomacy().operations()==1 &&
       pending.notifications().operations()==1 &&
       pending.notification_client().received_removals().size()==suffix.prefix->removed_notifications.size(),
       "Staged native clients/clocks were reconstructed or later suffix executed");
 check(pending.turns().world_revision()==bnw_turn_end_world_revision(pending.state(),h.identity(),h.database(),player) &&
       pending.units().world_revision()==bnw_unit_order_world_revision(pending.state(),h.identity(),h.database()) &&
       pending.notifications().epoch()==bnw_notification_epoch(pending.state(),h.identity(),h.database()) &&
       pending.notification_client().epoch()==pending.notifications().epoch() &&
       pending.diplomacy().epoch()==bnw_diplomacy_turn_epoch(pending.state(),h.identity(),h.database()),
       "Retained clients refer to different staged worlds");
 check(!has_received_bnw_local_turn_all_complete(pending.state(),pending.engine(),h.database()) &&
       has_received_bnw_local_turn_all_complete(held_pending.state(),held_pending.engine(),h.database()),
       "Retaining fanout changed delivered standalone engine history");
 auto altered_input=*autos.draft;
 scalar(altered_input.request.state.world.all_players->players.at(player).units.units.front().core,"m_iMoves")=7;
 auto retained_world=pending.state();
 check(scalar(retained_world.world.all_players->players.at(player).units.units.front().core,"m_iMoves")==120 &&
       scalar(retained_world.world.all_players->players.at(player).core,"m_bTurnActive")==0 &&
       scalar(autos.draft->request.state.world.all_players->players.at(player).core,"m_bTurnActive")==1,
       "Fanout draft borrowed caller state or published prior stores");
 // Actual new client feed consumer must roll back a malformed late status event.
 auto uiBefore=state;admin(uiBefore.world.all_players->players.at(player).administration,"m_eEndTurnBlockingType")=-1;admin(uiBefore.world.all_players->players.at(player).administration,"m_iEndTurnBlockingNotificationIndex")=-1;auto uiOwner=start_bnw_turn_end_ui_client(uiBefore,h.identity(),h.database(),player);auto uiAfter=uiBefore;scalar(uiAfter.world.all_players->players.at(player).core,"m_bTurnActive")=0;
 BnwTurnEndUiBatch feed{h.identity(),state.world.map_header.guid,uiOwner.world_revision(),bnw_turn_end_world_revision(uiAfter,h.identity(),h.database(),player),uiOwner.client_revision(),{{BnwTurnEndUiEffectKind::publish_end_turn_dirty,player},{BnwTurnEndUiEffectKind::publish_active_player_turn_end,player},{BnwTurnEndUiEffectKind::player_turn_status_end,player,-1,-1,99}}};bool bad=false;try{apply_bnw_turn_end_ui_batch(uiAfter,uiOwner,h.database(),feed,&autos.draft->engine);}catch(const std::exception&){bad=true;}check(bad && !uiOwner.active_player_turn_ended() && !uiOwner.player_status_ended() && uiOwner.client_revision()==0 && uiOwner.received_effects().empty(),"Late malformed suffix event partially published client");feed.effects.back().active_count=-1;
 auto forged=feed;forged.effects.insert(forged.effects.begin()+2,{BnwTurnEndUiEffectKind::change_num_active_debug,player,-1,-1,bnw_notification_game_turn_active_count(uiAfter)});bool fake=false;try{apply_bnw_turn_end_ui_batch(uiAfter,uiOwner,h.database(),forged,&autos.draft->engine);}catch(const std::exception&){fake=true;}check(fake && uiOwner.received_effects().empty(),"Matching native count without delivered AllComplete fabricated debug receipt");
 auto counted=feed;counted.effects.insert(counted.effects.begin()+2,{BnwTurnEndUiEffectKind::change_num_active_debug,player,-1,-1,bnw_notification_game_turn_active_count(uiAfter)});auto countClient=uiOwner;apply_bnw_turn_end_ui_batch(uiAfter,countClient,h.database(),counted,&transport);check(countClient.active_count_debug_receipts()==std::vector<std::int32_t>{0} && countClient.player_status_ended(),"Isolated actual AllComplete conditional/debug receiver failed");
 apply_bnw_turn_end_ui_batch(uiAfter,uiOwner,h.database(),feed,&autos.draft->engine);check(uiOwner.active_player_turn_ended() && uiOwner.player_status_ended() && !uiOwner.turn_active(),"Actual suffix UI consumer failed");
 const auto pending_again=[&](bool with_all=false){auto prefix=execute_bnw_local_human_deactivation_suffix_prefix_draft(
  with_all?complete:*autos.draft,binding,finish.draft->presentation,turns,owner,client,diplomacy,h.database(),h.types(),player);
  check(bool(prefix.pending_fanout),"Actual native prefix missing for event fixture");return std::move(prefix.pending_fanout);};
 FanoutFixture fixture;fixture.selected=selected.profile;fixture.identity=h.identity();fixture.player=player;
 auto routed_input=pending_again();const auto *prefix_address=routed_input.get();
 auto routed=dispatch_bnw_deactivation_active_player_turn_end(std::move(routed_input),selected.profile,fixture,h.database());
 check(routed.draft && routed.dispatch_attempted && fixture.forks==1 && fixture.callbacks==1 &&
  fixture.observed_moves==120 && fixture.publishes==0 && routed.draft->listeners()==1 &&
  &routed.draft->native_prefix()==prefix_address && routed.draft->state().player_culture_bomb_timers.at(player)==7 &&
  routed.draft->native_prefix().state().player_culture_bomb_timers.at(player)==0 &&
  !routed.draft->native_prefix().turns().player_status_ended() &&
  routed.draft->native_prefix().activation().completed.empty(),
  "Real fixture Lua delivery lost prefix/escrow epoch, replayed stores or published next stages");
 auto replay=dispatch_bnw_deactivation_active_player_turn_end(std::move(routed_input),selected.profile,fixture,h.database());
 check(!replay.draft && !replay.dispatch_attempted && fixture.forks==1 && fixture.callbacks==1,"Consumed turn-end prefix replayed Lua");
 const auto *same_epoch=&routed.draft->state();
 const auto delivered_before=routed.draft->native_prefix().engine().delivered().size();
 auto status=continue_bnw_deactivation_status(std::move(routed.draft),h.database());
 check(status.draft&&&status.draft->state()==same_epoch&&!status.draft->all_complete_received()&&
  status.draft->turns().player_status_ended()&&status.draft->turns().active_count_debug_receipts().empty()&&
  status.draft->completed_steps()==std::vector<BnwDeactivationStep>{BnwDeactivationStep::all_complete_query,BnwDeactivationStep::player_turn_status_end}&&
  status.draft->state().player_culture_bomb_timers.at(player)==7&&
  status.draft->fanout().native_prefix().activation().completed.empty()&&
  status.draft->fanout().native_prefix().engine().delivered().size()==delivered_before&&
  fixture.forks==1&&fixture.callbacks==1,"Status replayed native/Lua/engine effects, lost same epoch or inferred AllComplete");
 auto status_replay=continue_bnw_deactivation_status(std::move(routed.draft),h.database());
 check(!status_replay.draft,"Consumed fanout status replayed");
 FanoutFixture complete_fixture;complete_fixture.selected=selected.profile;complete_fixture.identity=h.identity();complete_fixture.player=player;
 auto complete_event=dispatch_bnw_deactivation_active_player_turn_end(pending_again(true),selected.profile,complete_fixture,h.database());
 check(bool(complete_event.draft),"Actual delivered AllComplete prefix lost earlier mandatory fanout");
 auto complete_status=continue_bnw_deactivation_status(std::move(complete_event.draft),h.database());
 check(complete_status.draft&&complete_status.draft->all_complete_received()&&
  complete_status.draft->turns().active_count_debug_receipts()==std::vector<std::int32_t>{0}&&
  complete_status.draft->completed_steps()==std::vector<BnwDeactivationStep>{BnwDeactivationStep::all_complete_query,BnwDeactivationStep::change_num_active_debug,BnwDeactivationStep::player_turn_status_end}&&
  complete_fixture.forks==1&&complete_fixture.callbacks==1&&
  !complete_status.draft->fanout().native_prefix().turns().player_status_ended(),"Conditional status/debug did not follow actual receipt or mutated prior client");
 FanoutFixture wrong_rules_fixture;wrong_rules_fixture.selected=selected.profile;wrong_rules_fixture.identity=h.identity();wrong_rules_fixture.player=player;
 auto wrong_rules_event=dispatch_bnw_deactivation_active_player_turn_end(pending_again(),selected.profile,wrong_rules_fixture,h.database());
 check(bool(wrong_rules_event.draft),"Wrong rules negative fixture missing earlier epoch");
 auto changed_rules=h.database().clone_gameplay();changed_rules->apply_sql("UPDATE Defines SET Value=777 WHERE Name='MIN_CITY_RANGE'");
 check(!continue_bnw_deactivation_status(std::move(wrong_rules_event.draft),*changed_rules).draft&&
  !turns.player_status_ended()&&scalar(autos.draft->request.state.world.all_players->players.at(player).core,"m_bTurnActive")==1,
  "Late status source failure published caller client/world");
 for(int failure=0;failure<5;++failure){
  FanoutFixture rejected_fixture;rejected_fixture.selected=selected.profile;rejected_fixture.identity=h.identity();rejected_fixture.player=player;
  rejected_fixture.verified=failure!=0;rejected_fixture.empty=failure==1;rejected_fixture.fail=failure==2;
  rejected_fixture.change_units=failure==3;rejected_fixture.bad_getters=failure==4;
  auto rejected_event=dispatch_bnw_deactivation_active_player_turn_end(pending_again(),selected.profile,rejected_fixture,h.database());
  check(!rejected_event.draft && rejected_fixture.publishes==0 &&
   rejected_event.dispatch_attempted==(failure>=1&&failure<=3) &&
   bool(rejected_event.pending)==(failure==0||failure==4),"Unknown/empty/error/stale event fixture accepted, lost pending native stores or misclassified dispatch");
  if(failure==0){const auto *retained=rejected_event.pending.get();rejected_fixture.verified=true;
   auto resumed=dispatch_bnw_deactivation_active_player_turn_end(std::move(rejected_event.pending),selected.profile,rejected_fixture,h.database());
   check(resumed.draft&&&resumed.draft->native_prefix()==retained&&rejected_fixture.forks==2&&
    rejected_fixture.callbacks==1&&resumed.draft->native_prefix().notifications().operations()==1,
    "Registry completion replayed native prefix/cleanup or lost its retained address");}
 }
 auto catalog=content::discover_packages({std::filesystem::path(argv[2])/"Assets/DLC"});
 BnwSelectedStockLuaSourceOwner stock(argv[2],std::move(catalog),selected.profile,h.identity());
 const auto stock_event=dispatch_bnw_deactivation_active_player_turn_end(pending_again(),selected.profile,stock,h.database());
 check(!stock_event.draft && stock_event.pending && !stock_event.dispatch_attempted && !stock_event.blocker.empty() &&
  scalar(copy.world.all_players->players.at(player).units.units.front().core,"m_iMoves")==120 &&
  scalar(autos.draft->request.state.world.all_players->players.at(player).core,"m_bTurnActive")==1 &&
  !h.session().readiness().can_continue(),"Incomplete actual stock epoch or failed callback published caller/full turn");
 std::cout<<"PASS selected native deactivation retained at source fanout; isolated SAME-epoch real Lua delivery/escrow, missing stock/registry/getters/error/reentrant/replay gates; AllComplete/status/next activation remain mandatory\n";
}
