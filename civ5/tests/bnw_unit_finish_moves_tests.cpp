#include "civ5/runtime/bnw_unit_finish_moves.hpp"
#include "civ5/runtime/bnw_ready_units.hpp"
#include "civ5/runtime/bnw_update_moves_local_sp.hpp"
#include "civ5/runtime/file_identity.hpp"
#include "civ5/ui/selected_bnw_preview_host.hpp"
#include <iostream>
#include <stdexcept>
using namespace civ5;using namespace civ5::runtime;
namespace {
void check(bool value,const char *why){if(!value)throw std::runtime_error(why);}
template<class Core>std::int64_t &scalar(Core &core,std::string_view name){for(auto &field:core.fields)if(field.name && std::string_view(field.name)==name && field.values.size()==1)return field.values[0];throw std::runtime_error("Missing FinishMoves fixture scalar");}
}
int main(int argc,char **argv) {
    check(argc==3 || argc==4,"Expected save/game root/optional SDK root");
    if(argc==4)for(const auto &file:kBnwUnitOrderOwnerSources)check(identify_file(std::filesystem::path(argv[3])/file.path).sha256==file.sha256,"FinishMoves source pin differs");
    auto selected=ui::bootstrap_selected_bnw_preview(argv[1],argv[2],0);check(selected.preview.host && !selected.preview.blocker,"Selected unavailable");
    const auto &host=*selected.preview.host;const auto &state=host.session().state();const auto player=host.active_player_id();
    BnwGameActivationOwner owner{host.identity()};const auto binding=bind_bnw_end_turn_import(host.session().source(),owner);
    owner.busy_state=bind_bnw_busy_state_original_import(host.session().source(),host.identity());
    owner.pathfinder_cache=make_bnw_pathfinder_cache_control_import_owner(host.session().source(),host.identity());
    const auto moves=bind_bnw_update_moves_original_import(host.session().source(),host.identity());
    auto cancelled=execute_bnw_end_turn_request_draft(state,owner,binding,host.database(),{player,state.setup.game_turn,false,bnw_end_turn_input_revision(state,owner,binding,host.database(),player)});
    check(bool(cancelled.draft),"Root cancelled draft unavailable");const auto &input=*cancelled.draft;
    auto copy=input.state;const BnwUnitOrderKey key{player,scalar(copy.world.all_players->players.at(player).units.units.front().core,"m_iID")};
    auto client=start_bnw_unit_presentation_client(input.state,host.identity(),host.database());select_bnw_unit_presentation(input.state,client,host.database(),key,client.client_revision());
    const BnwFinishMovesCommand command{key,state.setup.game_turn,bnw_finish_moves_input_revision(input,binding,client,host.database(),key),client.client_revision()};
    const auto run=execute_bnw_finish_moves_draft(input,binding,client,host.database(),command);
    check(bool(run.draft) && run.draft->movement_changed,"Actual selected FinishMoves failed");auto &d=*run.draft;
    const std::vector<BnwFinishMovesStep> order{BnwFinishMovesStep::moves_store,BnwFinishMovesStep::dim_flag,BnwFinishMovesStep::common_finder_force_reset,BnwFinishMovesStep::unit_info_dirty,BnwFinishMovesStep::plot_list_dirty};
    check(d.completed_steps==order && d.batch.effects.size()==3 && d.presentation.dim_flags().at(key) &&
          d.presentation.unit_info_dirty() && d.presentation.plot_list_dirty() && !d.presentation.has_moved_unit() &&
          d.presentation.cycle_selection_counter()==0 && d.presentation.received_effects().size()==3 &&
          d.request.owner.pathfinder_cache->at(BnwActivationFinder::common).force_reset() &&
          scalar(d.request.state.world.all_players->players.at(player).units.units.front().core,"m_iMoves")==0 &&
          scalar(d.request.state.world.all_players->players.at(player).units.units.front().core,"m_eActivityType")==0 &&
          observe_bnw_ready_player(d.request,moves,player).count_ready==0 && d.request.state.setup.game_turn==state.setup.game_turn &&
          scalar(d.request.state.world.all_players->players.at(player).core,"m_bTurnActive")==1 && d.request.owner.completed.empty(),
          "FinishMoves omitted mandatory source effects or acted as Skip/closed a turn");
    check(scalar(copy.world.all_players->players.at(player).units.units.front().core,"m_iMoves")==120 && !client.dim_flags().at(key) &&
          client.received_effects().empty() && !input.owner.pathfinder_cache->at(BnwActivationFinder::common).force_reset(),"Private FinishMoves published caller changes");
    const auto same=BnwFinishMovesCommand{key,state.setup.game_turn,bnw_finish_moves_input_revision(d.request,binding,d.presentation,host.database(),key),d.presentation.client_revision()};
    const auto noop=execute_bnw_finish_moves_draft(d.request,binding,d.presentation,host.database(),same);
    check(noop.draft && !noop.draft->movement_changed && noop.draft->completed_steps.empty() && noop.draft->batch.effects.empty() &&
          noop.draft->presentation.client_revision()==d.presentation.client_revision() && noop.draft->presentation.received_effects().size()==3,"Native equal-zero setMoves generated duplicate effects");
    auto missing=input;missing.owner.pathfinder_cache.reset();
    const auto blocked=execute_bnw_finish_moves_draft(missing,binding,client,host.database(),command);
    check(!blocked.draft && blocked.blocker.find("common finder")!=std::string::npos && !client.dim_flags().at(key),"Missing selected finder faked success or published dim effect");
    auto stale=command;++stale.expected_client_revision;check(!execute_bnw_finish_moves_draft(input,binding,client,host.database(),stale).draft,"Stale actual selection accepted");
    auto unselected=start_bnw_unit_presentation_client(input.state,host.identity(),host.database());
    const auto nonselected=BnwFinishMovesCommand{key,state.setup.game_turn,bnw_finish_moves_input_revision(missing,binding,unselected,host.database(),key),unselected.client_revision()};
    const auto free=execute_bnw_finish_moves_draft(missing,binding,unselected,host.database(),nonselected);
    check(free.draft && free.draft->batch.effects.size()==1 && free.draft->presentation.dim_flags().at(key) &&
          !free.draft->presentation.unit_info_dirty() && !free.draft->presentation.plot_list_dirty(),"Nonselected FinishMoves invented selected dirty/finder effects");
    const auto engine=bind_bnw_local_ai_barrier_original_import(host.session().source(),d.request.state,d.request.owner,binding,host.database(),player);
    auto end=execute_bnw_end_turn_request_draft(d.request.state,d.request.owner,binding,host.database(),{player,state.setup.game_turn,true,d.request.input_revision});
    check(bool(end.draft),"Actual FinishMoves followed by end-turn request rejected");
    const auto auto_moves=execute_bnw_local_human_auto_moves_draft(*end.draft,binding,moves,engine,host.database(),player);
    check(auto_moves.draft && auto_moves.draft->processed_auto_moves && auto_moves.draft->unit_orders_needed.empty() &&
          scalar(auto_moves.draft->request.state.world.all_players->players.at(player).core,"m_bTurnActive")==1 &&
          auto_moves.draft->request.owner.completed.empty(),"Actual typed FinishMoves did not unlock source automove completion or replayed DoTurn");
    std::cout<<"PASS selected typed stock FinishMoves: moves0/awake, actual dim+selected finder+dirty feed, rollback/noop/nonselected; subsequent owned AutoMoves processed without closing active turn\n";
}
