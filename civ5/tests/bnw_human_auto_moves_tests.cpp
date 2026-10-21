#include "civ5/runtime/bnw_update_moves_local_sp.hpp"
#include "civ5/runtime/bnw_ready_units.hpp"
#include "civ5/runtime/file_identity.hpp"
#include "civ5/ui/selected_bnw_preview_host.hpp"
#include <algorithm>
#include <iostream>
#include <stdexcept>
using namespace civ5;using namespace civ5::runtime;
namespace {
void check(bool value,const char *why){if(!value)throw std::runtime_error(why);}
template<class Core>std::int64_t &scalar(Core &core,std::string_view name) {
    for(auto &field:core.fields)if(field.name && std::string_view(field.name)==name && field.values.size()==1)return field.values[0];
    throw std::runtime_error("Missing auto-moves fixture scalar");
}
bool step(const BnwLocalHumanAutoMovesExecution &run,BnwLocalUpdateMovesStep wanted) {
    return std::find(run.prefix.completed_steps.begin(),run.prefix.completed_steps.end(),wanted)!=run.prefix.completed_steps.end();
}
}
int main(int argc,char **argv) {
    check(argc==3 || argc==4,"Expected selected save, game root and optional SDK CvGameCoreSource");
    if(argc==4)for(const auto &file:kBnwUpdateMovesSourceFiles)
        check(identify_file(std::filesystem::path(argv[3])/file.path).sha256==file.sha256,"SDK auto-moves source pin differs");
    auto selected=ui::bootstrap_selected_bnw_preview(argv[1],argv[2],0);
    check(selected.preview.host && !selected.preview.blocker,"Selected BNW unavailable");
    auto &host=*selected.preview.host;const auto player=host.active_player_id();const auto &state=host.session().state();
    BnwGameActivationOwner activation{host.identity()};
    const auto input=bind_bnw_end_turn_import(host.session().source(),activation);
    activation.pathfinder_cache=make_bnw_pathfinder_cache_control_import_owner(host.session().source(),host.identity());
    activation.busy_state=bind_bnw_busy_state_original_import(host.session().source(),host.identity());
    const auto moves=bind_bnw_update_moves_original_import(host.session().source(),host.identity());
    check(!moves.force_ending_turn(),"CvGame::Read reset inferred ForceEndingTurn=true");
    const auto engine=bind_bnw_local_ai_barrier_original_import(host.session().source(),state,activation,input,host.database(),player);
    const auto revision=bnw_end_turn_input_revision(state,activation,input,host.database(),player);
    const auto requested=execute_bnw_end_turn_request_draft(state,activation,input,host.database(),{player,state.setup.game_turn,true,revision});
    check(bool(requested.draft),"Actual root end-turn request rejected");
    const auto &request=*requested.draft;
    check(observe_bnw_ready_player(request,moves,player).count_ready==1,"Selected actual Warrior is not ready for a unit order");
    const auto run=execute_bnw_local_human_auto_moves_draft(request,input,moves,engine,host.database(),player);
    check(bool(run.draft) && run.blocker.empty(),"Selected actual empty queue suffix failed");
    auto &d=*run.draft;const auto id=scalar(d.request.state.world.all_players->players.at(player).units.units.front().core,"m_iID");
    check(d.unit_orders_needed==std::vector<std::int64_t>{id} && !d.processed_auto_moves && !d.request.requires_auto_move_processing &&
          scalar(d.request.state.world.all_players->players.at(player).core,"m_bEndTurn")==0 &&
          scalar(d.request.state.world.all_players->players.at(player).core,"m_bAutoMoves")==0 &&
          scalar(d.request.state.world.all_players->players.at(player).core,"m_bTurnActive")==1 &&
          scalar(d.request.state.world.all_players->players.at(player).units.units.front().core,"m_iMoves")==120 &&
          d.request.state.setup.game_turn==state.setup.game_turn && d.request.owner.completed.empty() &&
          d.moves.last_ai_civs_processed()==state.setup.game_turn && d.moves.initial_ai_processed(player)==state.setup.game_turn &&
          d.request.owner.pathfinder_cache->at(BnwActivationFinder::common).force_reset() &&
          all_bnw_local_ai_civs_processed(d.request.state,d.engine,host.database()) &&
          has_bnw_local_initial_ai_processed(d.request.state,d.engine,host.database()) &&
          !has_received_bnw_local_turn_complete(d.request.state,d.engine,host.database()) && d.engine.delivered().size()==3 &&
          step(run,BnwLocalUpdateMovesStep::empty_auto_mission) && step(run,BnwLocalUpdateMovesStep::turn_unready_delivery) &&
          step(run,BnwLocalUpdateMovesStep::end_turn_cancelled) && !step(run,BnwLocalUpdateMovesStep::processed_auto_moves_set) &&
          step(run,BnwLocalUpdateMovesStep::update_moves_finished),"Source cancelled end-turn was incorrectly completed or lost actual engine/native effects");
    check(moves.last_ai_civs_processed()==-1 && engine.delivered().empty() &&
          !request.owner.pathfinder_cache->at(BnwActivationFinder::common).force_reset() &&
          !request.owner.processed_auto_moves.at(player) &&
          bnw_end_turn_input_revision(request.state,request.owner,input,host.database(),player)==request.input_revision,
          "Successful private draft changed caller state/owners");
    const auto retry_revision=bnw_end_turn_input_revision(d.request.state,d.request.owner,input,host.database(),player);
    const auto retry=execute_bnw_end_turn_request_draft(d.request.state,d.request.owner,input,host.database(),{player,state.setup.game_turn,true,retry_revision});
    check(bool(retry.draft),"Repeat typed end-turn request rejected");
    const auto repeat=execute_bnw_local_human_auto_moves_draft(*retry.draft,input,d.moves,d.engine,host.database(),player);
    check(bool(repeat.draft) && repeat.draft->unit_orders_needed==std::vector<std::int64_t>{id} &&
          repeat.draft->engine.delivered().size()==4 && !step(repeat,BnwLocalUpdateMovesStep::send_ai_civs_processed) &&
          !step(repeat,BnwLocalUpdateMovesStep::local_initial_ai_delivery),"Repeat source suffix recreated AI receipts or skipped unit order cancellation");
    const auto lost_receipts=execute_bnw_local_human_auto_moves_draft(*retry.draft,input,d.moves,engine,host.database(),player);
    check(!lost_receipts.draft && lost_receipts.blocker.find("all required endpoint")!=std::string::npos &&
          !step(lost_receipts,BnwLocalUpdateMovesStep::send_ai_civs_processed) && engine.delivered().empty(),
          "Retained last-AI field manufactured a new receipt after owner loss");
    auto later=state;++later.setup.game_turn;later.world.pregame->archive.integers.at("game_turn")=later.setup.game_turn;
    bool late_rejected=false;
    try { (void)bind_bnw_local_ai_barrier_original_import(host.session().source(),later,activation,input,host.database(),player); }
    catch(const std::exception &) {late_rejected=true;}
    check(late_rejected,"Later epoch recreated original empty local receiver defaults");
    // Synthetic exhausted-movement branch proves completion of AutoMoves only;
    // it is not a typed unit-order implementation or completed player turn.
    auto exhausted=*retry.draft;scalar(exhausted.state.world.all_players->players.at(player).units.units.front().core,"m_iMoves")=0;
    const auto done=execute_bnw_local_human_auto_moves_draft(exhausted,input,d.moves,d.engine,host.database(),player);
    check(bool(done.draft) && done.draft->unit_orders_needed.empty() && done.draft->processed_auto_moves &&
          !done.draft->request.requires_auto_move_processing && step(done,BnwLocalUpdateMovesStep::processed_auto_moves_set) &&
          scalar(done.draft->request.state.world.all_players->players.at(player).core,"m_bTurnActive")==1 &&
          done.draft->request.owner.completed.empty(),"Exhausted movement skipped native processed flag or closed/replayed active turn");
    auto forced=moves;forced.set_force_ending_turn(true);
    const auto force=execute_bnw_local_human_auto_moves_draft(request,input,forced,engine,host.database(),player);
    check(bool(force.draft) && force.draft->unit_orders_needed.empty() && !force.draft->moves.force_ending_turn() &&
          force.draft->processed_auto_moves && step(force,BnwLocalUpdateMovesStep::force_ending_turn_clear) &&
          !step(force,BnwLocalUpdateMovesStep::turn_unready_delivery),"Native ForceEndingTurn branch was lost or fabricated local unready");
    auto death=request;scalar(death.state.world.all_players->players.at(player).units.units.front().core,"m_bDeathDelay")=1;
    const auto dying=execute_bnw_local_human_auto_moves_draft(death,input,moves,engine,host.database(),player);
    check(!dying.draft && dying.blocker.find("kill owner")!=std::string::npos && engine.delivered().empty(),"Effectful delayed death was silently passed");
    auto queued=request;auto &tail=queued.state.world.all_players->players.at(player).units.units.front().tail;
    for(auto &node:tail.blocks)if(node.name && std::string_view(node.name)=="m_missionQueue"){node.wire_count=1;node.values={0,0,0,0,0};}
    const auto mission=execute_bnw_local_human_auto_moves_draft(queued,input,moves,engine,host.database(),player);
    check(!mission.draft && mission.blocker.find("nonempty mission queue")!=std::string::npos && engine.delivered().empty(),"Nonempty AutoMission used fake successful handler");
    auto no_busy=request;no_busy.owner.busy_state.reset();
    check(!execute_bnw_local_human_auto_moves_draft(no_busy,input,moves,engine,host.database(),player).draft,"Missing retained busy owner fabricated defaults");
    auto ready_engine=engine;
    check(!has_received_bnw_local_turn_complete(state,ready_engine,host.database()),"Fresh engine manufactured ready state");
    send_bnw_local_ai_event(state,ready_engine,host.database(),BnwLocalAiEventKind::turn_complete);
    check(has_received_bnw_local_turn_complete(state,ready_engine,host.database()),"Actual complete message did not reach local receiver");
    check(send_bnw_local_turn_unready(state,ready_engine,host.database()) && !has_received_bnw_local_turn_complete(state,ready_engine,host.database()),
          "Actual unready receipt failed to clear local readiness");
    std::cout<<"PASS selected native empty AutoMission cancels EndTurn via actual local unready; needs Warrior order, repeat receipts retained, caller unchanged; active turn not closed\n";
}
