#include "civ5/runtime/bnw_player_turn_deactivation.hpp"
#include "civ5/runtime/bnw_unit_finish_moves.hpp"
#include "civ5/runtime/bnw_update_moves_local_sp.hpp"
#include "civ5/runtime/file_identity.hpp"
#include "civ5/ui/selected_bnw_preview_host.hpp"
#include <algorithm>
#include <iostream>
#include <stdexcept>
using namespace civ5;using namespace civ5::runtime;
namespace {
void check(bool b,const char *why){if(!b)throw std::runtime_error(why);}
template<class Core>std::int64_t &scalar(Core &c,std::string_view n){for(auto &f:c.fields)if(f.name && std::string_view(f.name)==n && f.values.size()==1)return f.values[0];throw std::runtime_error("Fixture scalar missing");}
std::int64_t &field(saves::PlotObservation &p,std::string_view n){for(auto &f:p.fields)if(f.name && std::string_view(f.name)==n)return f.value;throw std::runtime_error("Fixture plot field missing");}
saves::UnitTailNode &ignore(saves::UnitTailObservation &t){saves::UnitTailNode *out=nullptr;const auto v=[&](auto &&self,saves::UnitTailNode &n)->void{if(n.name && std::string_view(n.name)=="m_bIgnoreDangerWakeup")out=&n;for(auto &c:n.children)self(self,c);};for(auto &n:t.blocks)v(v,n);check(out,"Fixture ignore missing");return *out;}
std::int64_t &adminfield(saves::PlayerAdministrationObservation &a,std::string_view n){saves::NamedSaveInteger *out=nullptr;const auto visit=[&](auto &&self,saves::PlayerAdministrationRecord &r)->void{for(auto &f:r.fields)if(f.name && std::string_view(f.name)==n){check(!out,"Fixture duplicate blocking field");out=&f;}for(auto &c:r.children)self(self,c);};for(auto &r:a.blocks)visit(visit,r);check(out,"Fixture blocking field missing");return out->value;}
saves::PlotArrayObservation &war(saves::TeamObservation &t){for(auto &a:t.arrays)if(a.name && std::string_view(a.name)=="at_war")return a;throw std::runtime_error("Fixture war missing");}
}
int main(int argc,char **argv) {
 check(argc==3 || argc==4,"Expected save/game root/optional SDK root");
 if(argc==4){for(const auto &f:kBnwTurnDeactivationSources)check(identify_file(std::filesystem::path(argv[3])/f.path).sha256==f.sha256,"Deactivation source differs");for(const auto &f:kBnwUnitMovementSources)check(identify_file(std::filesystem::path(argv[3])/f.path).sha256==f.sha256,"Movement source differs");for(const auto &f:kBnwUnitResetSources)check(identify_file(std::filesystem::path(argv[3])/f.path).sha256==f.sha256,"Reset source differs");}
 auto selected=ui::bootstrap_selected_bnw_preview(argv[1],argv[2],0);check(selected.preview.host && !selected.preview.blocker,"Selected missing");
 const auto &h=*selected.preview.host;const auto &state=h.session().state();const auto player=h.active_player_id();auto copy=state;
 const BnwUnitOrderKey key{player,scalar(copy.world.all_players->players.at(player).units.units.front().core,"m_iID")};
 const auto quote=[&](const LiveSessionState &s){return quote_bnw_near_enemy_citadel(s,h.database(),h.types(),h.identity(),key);};
 const auto q=quote(state);if(!q.near_enemy_citadel)std::cerr<<q.blocker<<'\n';
 check(q.near_enemy_citadel==false && !q.damage && q.visited_plots.size()==7,"Selected actual native citadel absence not proven");
 const auto *citadel=h.types().find("IMPROVEMENT_CITADEL");check(citadel && citadel->table=="Improvements","Selected citadel rules missing");
 auto danger=state;const auto index=q.visited_plots.front();auto &plot=danger.world.map_plots->plots.at(index);auto &cell=danger.world.map_projection->cells.at(index);
 field(plot,"improvement_type_hash")=citadel->hash;cell.improvement_type_hash=citadel->hash;cell.improvement_type=citadel->id;
 field(plot,"improvement_pillaged")=0;cell.improvement_pillaged=false;const auto opponent=player==0?1:0;
 field(plot,"owner")=opponent;cell.owner=opponent;const auto ownteam=danger.setup.players.at(player).team,theirteam=danger.setup.players.at(opponent).team;
 check(ownteam!=theirteam,"Fixture distinct enemy team missing");war(danger.world.teams->teams.at(ownteam)).values.at(theirteam)=1;
 const auto positive=quote(danger);check(positive.near_enemy_citadel==true && positive.damage && *positive.damage>0 && positive.damaging_plot==index && positive.visited_plots.size()==1,"Citadel failed first native damaging plot/war rule");
 const auto positiveworld=danger;
 field(plot,"improvement_pillaged")=1;cell.improvement_pillaged=true;check(quote(danger).near_enemy_citadel==false,"Pillaged citadel still damages");
 field(plot,"improvement_pillaged")=0;cell.improvement_pillaged=false;field(plot,"owner")=-1;cell.owner=-1;check(quote(danger).near_enemy_citadel==false,"Unowned citadel still damages");
 field(plot,"owner")=opponent;cell.owner=opponent;war(danger.world.teams->teams.at(ownteam)).values.at(theirteam)=0;check(quote(danger).near_enemy_citadel==false,"Peaceful citadel damages");
 cell.improvement_type_hash.reset();check(!quote(danger).near_enemy_citadel,"Unknown raw/projected citadel binding silently absent");
 BnwGameActivationOwner owner{h.identity()};const auto binding=bind_bnw_end_turn_import(h.session().source(),owner);
 owner.busy_state=bind_bnw_busy_state_original_import(h.session().source(),h.identity());owner.pathfinder_cache=make_bnw_pathfinder_cache_control_import_owner(h.session().source(),h.identity());
 const auto moves=bind_bnw_update_moves_original_import(h.session().source(),h.identity());
 auto endfalse=execute_bnw_end_turn_request_draft(state,owner,binding,h.database(),{player,state.setup.game_turn,false,bnw_end_turn_input_revision(state,owner,binding,h.database(),player)});check(bool(endfalse.draft),"Initial draft missing");
 auto client=start_bnw_unit_presentation_client(endfalse.draft->state,h.identity(),h.database());select_bnw_unit_presentation(endfalse.draft->state,client,h.database(),key,0);
 auto finish=execute_bnw_finish_moves_draft(*endfalse.draft,binding,client,h.database(),{key,state.setup.game_turn,bnw_finish_moves_input_revision(*endfalse.draft,binding,client,h.database(),key),client.client_revision()});check(bool(finish.draft),"FinishMoves missing");
 auto end=execute_bnw_end_turn_request_draft(finish.draft->request.state,finish.draft->request.owner,binding,h.database(),{player,state.setup.game_turn,true,finish.draft->request.input_revision});check(bool(end.draft),"End draft missing");
 const auto engine=bind_bnw_local_ai_barrier_original_import(h.session().source(),end.draft->state,end.draft->owner,binding,h.database(),player);
 auto automoves=execute_bnw_local_human_auto_moves_draft(*end.draft,binding,moves,engine,h.database(),player);check(automoves.draft && automoves.draft->processed_auto_moves,"Native AutoMoves not processed");
 const auto &input=automoves.draft->request;const auto &beforeclient=finish.draft->presentation;
 const auto run=[&](const BnwEndTurnRequestDraft &d,const BnwUnitPresentationOwner &c){return execute_bnw_human_unit_reset_draft(d,binding,c,h.database(),h.types(),player);};
 const auto reset=run(input,beforeclient);if(!reset.draft)std::cerr<<reset.blocker<<'\n';check(bool(reset.draft),"Actual completed-auto draft unit reset failed");
 auto &actual=reset.draft->request.state.world.all_players->players.at(player).units.units.front();
 check(scalar(actual.core,"m_iMoves")==120 && scalar(actual.core,"m_iAttacksMade")==0 && scalar(actual.core,"m_iMadeInterceptionCount")==0 && ignore(actual.tail).values[0]==0 && !reset.draft->presentation.dim_flags().at(key) && reset.draft->batch.effects.size()==3 && reset.draft->reset_units==std::vector<BnwUnitOrderKey>{key},"Native unit reset stores/presentation mismatch");
 auto caller=input;check(scalar(caller.state.world.all_players->players.at(player).units.units.front().core,"m_iMoves")==0 && beforeclient.dim_flags().at(key) && scalar(actual.core,"m_eActivityType")==0 && scalar(reset.draft->request.state.world.all_players->players.at(player).core,"m_bTurnActive")==1 && reset.draft->request.owner.completed.empty(),"Unit reset published/deactivated/replayed imported DoTurn");
 auto positiveinput=input;positiveinput.state.world.map_plots=positiveworld.world.map_plots;positiveinput.state.world.map_projection=positiveworld.world.map_projection;positiveinput.state.world.teams=positiveworld.world.teams;
 const auto blockedcitadel=run(positiveinput,beforeclient);check(!blockedcitadel.draft && blockedcitadel.blocker.find("changeDamage")!=std::string::npos && beforeclient.dim_flags().at(key),"Positive citadel changeDamage silently skipped or published reset");
 auto notprocessed=input;notprocessed.owner.processed_auto_moves.at(player)=false;check(!run(notprocessed,beforeclient).draft,"Unit reset manufactured processed automoves");
 auto heal=input;scalar(heal.state.world.all_players->players.at(player).units.units.front().core,"m_iAlwaysHealCount")=1;auto healclient=start_bnw_unit_presentation_client(heal.state,h.identity(),h.database());const auto blockedheal=run(heal,healclient);check(!blockedheal.draft && blockedheal.blocker.find("doHeal")!=std::string::npos,"AlwaysHeal callback was silently skipped");
 auto late=input;ignore(late.state.world.all_players->players.at(player).units.units.front().tail).values.clear();auto lateclient=start_bnw_unit_presentation_client(late.state,h.identity(),h.database());const auto fail=run(late,lateclient);check(!fail.draft && fail.blocker.find("ignore-danger")!=std::string::npos && lateclient.dim_flags().at(key) && scalar(late.state.world.all_players->players.at(player).units.units.front().core,"m_iMoves")==0,"Late reset failure published partial movement/dim state");
 auto damaged=input;scalar(damaged.state.world.all_players->players.at(player).units.units.front().core,"m_iDamage")=5;auto damageclient=start_bnw_unit_presentation_client(damaged.state,h.identity(),h.database());check(bool(run(damaged,damageclient).draft),"Moved non-AlwaysHeal unit incorrectly heals merely because hurt");
 auto unmoved=reset.draft->request;scalar(unmoved.state.world.all_players->players.at(player).units.units.front().core,"m_iDamage")=5;auto unmovedclient=start_bnw_unit_presentation_client(unmoved.state,h.identity(),h.database());check(!run(unmoved,unmovedclient).draft,"Unmoved hurt unit silently skipped native doHeal");
 const auto again=run(reset.draft->request,reset.draft->presentation);check(again.draft && again.draft->batch.effects.empty() && again.draft->presentation.client_revision()==reset.draft->presentation.client_revision(),"Equal max-move reset repeated source setMoves effects");
 auto turns=start_bnw_turn_end_ui_client(input.state,h.identity(),h.database(),player);
 const auto deactivation=execute_bnw_local_human_deactivation_prefix_draft(*automoves.draft,binding,beforeclient,turns,h.database(),h.types(),player);
 if(!deactivation.prefix)std::cerr<<deactivation.blocker<<'\n';
 check(deactivation.prefix && deactivation.blocker.find("EndOfTurnCleanup")!=std::string::npos && deactivation.completed_steps.front()==BnwDeactivationStep::end_request_and_processed && deactivation.completed_steps.back()==BnwDeactivationStep::notification_boundary,"Real source deactivation prefix did not reach exact mandatory notification gate");
 const auto &w=*deactivation.prefix;const bool blockingchanged=turns.blocking_type()!=-1 || turns.blocking_notification()!=-1;
 check(!w.private_turn_active && w.private_blocking_type==-1 && w.private_blocking_notification==-1 && w.reset_units==std::vector<BnwUnitOrderKey>{key} && w.consumed_ui_effects.size()==(blockingchanged?3:1) && w.consumed_ui_effects.front().kind==BnwTurnEndUiEffectKind::publish_end_turn_dirty && w.notification_transients_known,"Native deactivation flag/blocking/reset/actual UI source order differs");
 check(turns.turn_active() && !turns.end_turn_dirty() && turns.received_effects().empty() && beforeclient.dim_flags().at(key) && scalar(caller.state.world.all_players->players.at(player).core,"m_bTurnActive")==1 && scalar(caller.state.world.all_players->players.at(player).units.units.front().core,"m_iMoves")==0 && caller.owner.completed.empty(),"Blocked deactivation published actor/client/unit state or replayed DoTurn");
 auto indexonly=*automoves.draft;adminfield(indexonly.request.state.world.all_players->players.at(player).administration,"m_eEndTurnBlockingType")=-1;adminfield(indexonly.request.state.world.all_players->players.at(player).administration,"m_iEndTurnBlockingNotificationIndex")=0;
 auto indexui=start_bnw_turn_end_ui_client(indexonly.request.state,h.identity(),h.database(),player);
 const auto indexrun=execute_bnw_local_human_deactivation_prefix_draft(indexonly,binding,beforeclient,indexui,h.database(),h.types(),player);
 check(indexrun.prefix && indexrun.prefix->consumed_ui_effects.size()==3 && indexrun.prefix->consumed_ui_effects[1].kind==BnwTurnEndUiEffectKind::blocking_changed && indexrun.prefix->consumed_ui_effects[1].previous_type==-1 && indexrun.prefix->consumed_ui_effects[1].new_type==-1 && indexui.blocking_notification()==0 && indexui.received_effects().empty(),"Index-only native blocking change failed source OR predicate or published owner");
 auto wrong=*automoves.draft;wrong.request.owner.processed_auto_moves.at(player)=false;check(!execute_bnw_local_human_deactivation_prefix_draft(wrong,binding,beforeclient,turns,h.database(),h.types(),player).prefix,"Unprocessed flag reached private deactivation");
 auto busy=*automoves.draft;busy.request.owner.busy_state->set_unit_mission_timer({player,key.second},1);
 const auto blockedbusy=execute_bnw_local_human_deactivation_prefix_draft(busy,binding,beforeclient,turns,h.database(),h.types(),player);check(!blockedbusy.prefix && blockedbusy.blocker.find("turn-complete receipt")!=std::string::npos,"Busy human assumed complete without actual delivered engine receipt");
 send_bnw_local_ai_event(busy.request.state,busy.engine,h.database(),BnwLocalAiEventKind::turn_complete);
 const auto receivedbusy=execute_bnw_local_human_deactivation_prefix_draft(busy,binding,beforeclient,turns,h.database(),h.types(),player);
 check(receivedbusy.prefix && std::find(receivedbusy.completed_steps.begin(),receivedbusy.completed_steps.end(),BnwDeactivationStep::received_turn_complete)!=receivedbusy.completed_steps.end(),"Actually delivered local complete receipt did not unlock source busy fallback");
 // Concrete retained receiver and atomic rollback on a malformed late event.
 auto after=input.state;scalar(after.world.all_players->players.at(player).core,"m_bTurnActive")=0;
 const auto setadmin=[&](std::string_view name,std::int64_t value){bool found=false;const auto visit=[&](auto &&self,saves::PlayerAdministrationRecord &r)->void{for(auto &f:r.fields)if(f.name && std::string_view(f.name)==name){check(!found,"Fixture admin duplicate");f.value=value;found=true;}for(auto &c:r.children)self(self,c);};for(auto &r:after.world.all_players->players.at(player).administration.blocks)visit(visit,r);check(found,"Fixture admin missing");};
 setadmin("m_eEndTurnBlockingType",-1);setadmin("m_iEndTurnBlockingNotificationIndex",-1);
 BnwTurnEndUiBatch feed{h.identity(),after.world.map_header.guid,turns.world_revision(),bnw_turn_end_world_revision(after,h.identity(),h.database(),player),turns.client_revision(),{{BnwTurnEndUiEffectKind::publish_end_turn_dirty,player}}};
 if(blockingchanged){feed.effects.push_back({BnwTurnEndUiEffectKind::blocking_changed,player,turns.blocking_type(),-1});feed.effects.push_back({BnwTurnEndUiEffectKind::update_end_turn,player});}
 auto receiver=turns;apply_bnw_turn_end_ui_batch(after,receiver,h.database(),feed);check(!receiver.turn_active() && receiver.end_turn_dirty() && receiver.blocking_type()==-1 && receiver.blocking_notification()==-1 && receiver.end_turn_updates()==(blockingchanged?1:0) && receiver.received_effects().size()==feed.effects.size(),"Actual turn-end receiver did not retain source effects");
 auto bad=feed;bad.effects.push_back({static_cast<BnwTurnEndUiEffectKind>(255),player});auto untouched=turns;bool threw=false;try{apply_bnw_turn_end_ui_batch(after,untouched,h.database(),bad);}catch(const std::exception&){threw=true;}check(threw && untouched.turn_active() && !untouched.end_turn_dirty() && untouched.received_effects().empty() && untouched.client_revision()==turns.client_revision(),"Late malformed turn-end feed partially consumed source effects");
 std::cout<<"PASS deactivation prefix source activefalse/EndTurnDirty/serialized blocking/real reset, retained UI receipt rollback, busy real-receipt gate; notifications remain mandatory; regression native citadel query and FinishMoves-End-Auto-DoUnitReset: maxMoves restored120/dimfalse/finder+dirty/resetflags, heal/unknown/late atomic gates, no deactivate/full turn\n";
}
