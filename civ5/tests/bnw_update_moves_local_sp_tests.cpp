#include "civ5/runtime/bnw_update_moves_local_sp.hpp"
#include "civ5/runtime/file_identity.hpp"
#include "civ5/ui/selected_bnw_preview_host.hpp"
#include <iostream>
#include <algorithm>
#include <stdexcept>
using namespace civ5;using namespace civ5::runtime;
namespace {
void check(bool value,const char *why){if(!value)throw std::runtime_error(why);}
template<class F>void rejects(F &&f,const char *why){try{f();}catch(const std::exception&){return;}throw std::runtime_error(why);}
template<class Core>std::int64_t &scalar(Core &core,std::string_view name) {
    for(auto &field:core.fields)if(field.name && std::string_view(field.name)==name && field.values.size()==1)return field.values[0];
    throw std::runtime_error("Missing fixture scalar");
}
}
int main(int argc,char **argv) {
    check(argc==3 || argc==4,"Expected selected save, game root and optional SDK CvGameCoreSource");
    if(argc==4)for(const auto &file:kBnwUpdateMovesSourceFiles)
        check(identify_file(std::filesystem::path(argv[3])/file.path).sha256==file.sha256,"SDK local updateMoves source pin differs");
    auto selected=ui::bootstrap_selected_bnw_preview(argv[1],argv[2],0);
    check(selected.preview.host && !selected.preview.blocker,"Selected BNW unavailable");
    auto &host=*selected.preview.host;const auto player=host.active_player_id();const auto &state=host.session().state();
    BnwGameActivationOwner activation{host.identity()};
    const auto input=bind_bnw_end_turn_import(host.session().source(),activation);
    activation.pathfinder_cache=make_bnw_pathfinder_cache_control_import_owner(host.session().source(),host.identity());
    const auto moves=bind_bnw_update_moves_original_import(host.session().source(),host.identity());
    const auto barrier=bind_bnw_local_ai_barrier_original_import(host.session().source(),state,activation,input,host.database(),player);
    check(moves.last_ai_civs_processed()==-1 && !moves.process_player_auto_moves(),"Native original transient defaults differ");
    for(std::size_t id=0;id<64;++id)check(moves.initial_ai_processed(id)==-1,"Initial player AI reset receipt was inferred current");
    check(barrier.delivered().empty() && barrier.pending_count()==0 && !all_bnw_local_ai_civs_processed(state,barrier,host.database()) &&
          !has_bnw_local_initial_ai_processed(state,barrier,host.database()),"Absent local messages manufactured a receipt");
    auto staged=barrier;const auto packet=prepare_bnw_local_ai_packet(state,staged,host.database(),BnwLocalAiEventKind::ai_civs_processed);
    check(staged.pending_count()==1 && !all_bnw_local_ai_civs_processed(state,staged,host.database()),"Undelivered local packet satisfied barrier");
    auto bad=packet;++bad.turn;
    rejects([&]{receive_bnw_local_ai_packet(state,staged,host.database(),bad);},"Stale packet was delivered");
    bad=packet;++bad.participant;
    rejects([&]{receive_bnw_local_ai_packet(state,staged,host.database(),bad);},"Foreign participant was delivered");
    bad=packet;bad.roster_sha256[0]^=1;
    rejects([&]{receive_bnw_local_ai_packet(state,staged,host.database(),bad);},"Foreign roster was delivered");
    bad=packet;bad.identity.payload_sha256[0]^=1;
    rejects([&]{receive_bnw_local_ai_packet(state,staged,host.database(),bad);},"Foreign party was delivered");
    bad=packet;++bad.sequence;
    rejects([&]{receive_bnw_local_ai_packet(state,staged,host.database(),bad);},"Unsent matching packet invented a receipt");
    check(staged.pending_count()==1 && staged.delivered().empty(),"Rejected delivery changed retained messages");
    receive_bnw_local_ai_packet(state,staged,host.database(),packet);
    check(staged.pending_count()==0 && staged.delivered().size()==1 && all_bnw_local_ai_civs_processed(state,staged,host.database()) &&
          !has_bnw_local_initial_ai_processed(state,staged,host.database()),"Actual local receiver did not produce the correct isolated receipt");
    receive_bnw_local_ai_packet(state,staged,host.database(),packet);
    check(staged.delivered().size()==1,"Exact retransmission duplicated delivery");
    bad=packet;bad.kind=BnwLocalAiEventKind::player_initial_ai_processed;
    rejects([&]{receive_bnw_local_ai_packet(state,staged,host.database(),bad);},"Changed retransmission bypassed the receiver");
    send_bnw_local_ai_event(state,staged,host.database(),BnwLocalAiEventKind::player_initial_ai_processed);
    check(has_bnw_local_initial_ai_processed(state,staged,host.database()) && staged.delivered().size()==2,
          "Initial AI message did not pass through the actual local receiver");
    auto other_turn=state;++other_turn.setup.game_turn;other_turn.world.pregame->archive.integers.at("game_turn")=other_turn.setup.game_turn;
    rejects([&]{all_bnw_local_ai_civs_processed(other_turn,staged,host.database());},"Previous-turn receipt was reused");
    auto other_party=state;other_party.world.map_header.guid[0]^=1;
    rejects([&]{all_bnw_local_ai_civs_processed(other_party,staged,host.database());},"Foreign map reused receiver");
    auto many=state;std::int64_t second=-1;
    for(std::int64_t id=0;id<63;++id)if(id!=player && many.setup.players.at(id).alive){second=id;break;}
    check(second>=0,"Missing second participant fixture");
    many.setup.players.at(second).slot_status=3;many.world.pregame->archive.integer_arrays.at("slot_status").at(second)=3;
    rejects([&]{bind_bnw_local_ai_barrier_original_import(host.session().source(),many,activation,input,host.database(),player);},
            "Two human participants were normalized to one");
    auto observer=state;observer.setup.players.at(second).slot_status=4;
    observer.world.pregame->archive.integer_arrays.at("slot_status").at(second)=4;
    rejects([&]{bind_bnw_local_ai_barrier_original_import(host.session().source(),observer,activation,input,host.database(),player);},
            "Observer participation was silently guessed");
    auto continuation=host.session().source();continuation.source.container=saves::SaveContainerKind::civis;
    rejects([&]{bind_bnw_local_ai_barrier_original_import(continuation,state,activation,input,host.database(),player);},
            "Continuation inferred original empty receipt state");
    check(all_bnw_unit_ai_processed(state,moves),"Selected real nonautomated unit unexpectedly needs AI");
    const auto before=bnw_end_turn_input_revision(state,activation,input,host.database(),player);
    const auto requested=execute_bnw_end_turn_request_draft(state,activation,input,host.database(),{player,state.setup.game_turn,true,before});
    check(bool(requested.draft),"Actual root end-turn draft rejected");
    const auto &request=*requested.draft;
    const auto run=execute_bnw_local_update_moves_prefix_draft(request,input,moves,barrier,host.database(),player);
    const std::vector<BnwLocalUpdateMovesStep> order{
        BnwLocalUpdateMovesStep::ai_roster_scan,BnwLocalUpdateMovesStep::send_ai_civs_processed,
        BnwLocalUpdateMovesStep::last_ai_turn_store,BnwLocalUpdateMovesStep::all_ai_receipt_check,
        BnwLocalUpdateMovesStep::initial_ai_player_flags,BnwLocalUpdateMovesStep::local_initial_ai_delivery,
        BnwLocalUpdateMovesStep::human_roster_select,BnwLocalUpdateMovesStep::common_finder_force_reset};
    check(run.completed_steps==order && run.selected_humans==std::vector<std::int64_t>{player} &&
          run.unit_ai_query_complete && run.all_unit_ai_processed && run.blocker.find("GetCountReadyUnits")!=std::string::npos,
          "Actual local prefix source order or first native busy blocker differs");
    check(barrier.delivered().empty() && barrier.pending_count()==0 && moves.last_ai_civs_processed()==-1 &&
          !moves.process_player_auto_moves() && moves.initial_ai_processed(player)==-1 &&
          !request.owner.pathfinder_cache->at(BnwActivationFinder::common).force_reset() &&
          !request.owner.processed_auto_moves.at(player) && request.owner.completed.empty() &&
          bnw_end_turn_input_revision(request.state,request.owner,input,host.database(),player)==request.input_revision,
          "Incomplete updateMoves published native messages/flags/finder reset or repeated imported DoTurn");
    auto unprocessed=request;auto &core=unprocessed.state.world.all_players->players.at(player).units.units.front().core;
    scalar(core,"m_bAITurnProcessed")=0;scalar(core,"m_eAutomateType")=0;scalar(core,"m_eUnitAIType")=1;scalar(core,"m_iMoves")=60;
    check(!all_bnw_unit_ai_processed(unprocessed.state,moves),"Actual unprocessed automated unit was marked complete");
    const auto ai_run=execute_bnw_local_update_moves_prefix_draft(unprocessed,input,moves,barrier,host.database(),player);
    check(ai_run.unit_ai_query_complete && !ai_run.all_unit_ai_processed &&
          ai_run.blocker.find("AI_unitUpdate")!=std::string::npos &&
          std::find(ai_run.completed_steps.begin(),ai_run.completed_steps.end(),BnwLocalUpdateMovesStep::local_initial_ai_delivery)==ai_run.completed_steps.end() &&
          std::find(ai_run.completed_steps.begin(),ai_run.completed_steps.end(),BnwLocalUpdateMovesStep::common_finder_force_reset)==ai_run.completed_steps.end(),
          "AI required branch used engine receipt to skip native unit work");
    auto missing=request;missing.state.world.all_players->players.at(player).units.units.front().tail.blocks.clear();
    const auto unknown=execute_bnw_local_update_moves_prefix_draft(missing,input,moves,barrier,host.database(),player);
    check(!unknown.unit_ai_query_complete && unknown.blocker.find("mission queue")!=std::string::npos,
          "Unknown unit inputs were inferred empty after local receipt");
    std::cout<<"PASS selected actual local-SP packet/receiver receipts, original resets, native prefix source order and atomic rollback; missing busy/mission owner remains blocker\n";
}
