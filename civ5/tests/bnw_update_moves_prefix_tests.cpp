#include "civ5/runtime/bnw_update_moves_prefix.hpp"
#include "civ5/runtime/file_identity.hpp"
#include "civ5/ui/selected_bnw_preview_host.hpp"
#include <iostream>
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
    check(argc==3 || argc==4,"Expected selected save, game root and optional SDK CvGameCoreSource directory");
    if(argc==4)for(const auto &file:kBnwUpdateMovesSourceFiles)
        check(identify_file(std::filesystem::path(argv[3])/file.path).sha256==file.sha256,"SDK updateMoves source pin differs");
    auto selected=ui::bootstrap_selected_bnw_preview(argv[1],argv[2],0);
    check(selected.preview.host && !selected.preview.blocker,"Selected BNW unavailable");
    auto &host=*selected.preview.host;const auto player=host.active_player_id();
    const auto &state=host.session().state();
    BnwGameActivationOwner activation{host.identity()};
    const auto input=bind_bnw_end_turn_import(host.session().source(),activation);
    const auto owner=bind_bnw_update_moves_original_import(host.session().source(),host.identity());
    check(owner.last_ai_civs_processed()==-1,"CvGame::Read reset was inferred current");
    auto continuation=host.session().source();continuation.source.container=saves::SaveContainerKind::civis;
    rejects([&]{bind_bnw_update_moves_original_import(continuation,host.identity());},"Civis continuation invented original Reset state");
    const auto units=observe_bnw_update_moves_units(state,owner,player);
    check(!units.empty(),"Selected actual unit evidence unexpectedly empty");
    std::uint64_t missions=0,automated=0,need_ai=0;
    for(const auto &unit:units) {
        missions+=unit.mission_count;automated+=unit.automated;need_ai+=unit.needs_ai_update;
        check(unit.needs_ai_update==(!unit.turn_processed && unit.automated && unit.unit_ai!=0 && unit.moves>0),"Native needs-AI predicate differs");
    }
    auto missing=state;missing.world.all_players->players.at(player).units.units.front().tail.blocks.clear();
    rejects([&]{observe_bnw_update_moves_units(missing,owner,player);},"Unknown mission queue was inferred empty");
    auto count=state;++count.world.all_players->players.at(player).units.header.wire_unit_count;
    rejects([&]{observe_bnw_update_moves_units(count,owner,player);},"Incomplete unit collection was inferred complete");
    auto predicate=state;auto &core=predicate.world.all_players->players.at(player).units.units.front().core;
    scalar(core,"m_bAITurnProcessed")=0;scalar(core,"m_eAutomateType")=0;
    scalar(core,"m_eUnitAIType")=1;scalar(core,"m_iMoves")=1;
    check(observe_bnw_update_moves_units(predicate,owner,player).front().needs_ai_update,"Automated movable unprocessed unit skipped native AI");
    scalar(core,"m_eUnitAIType")=0;
    check(!observe_bnw_update_moves_units(predicate,owner,player).front().needs_ai_update,"UNITAI_UNKNOWN incorrectly requested AI");
    scalar(core,"m_eUnitAIType")=-1;
    check(observe_bnw_update_moves_units(predicate,owner,player).front().needs_ai_update,"Raw NO_UNITAI was incorrectly normalized to UNITAI_UNKNOWN");
    scalar(core,"m_bAITurnProcessed")=1;
    check(!observe_bnw_update_moves_units(predicate,owner,player).front().needs_ai_update,"Processed unit requested AI");

    const auto before=bnw_end_turn_input_revision(state,activation,input,host.database(),player);
    const auto request=execute_bnw_end_turn_request_draft(state,activation,input,host.database(),{player,state.setup.game_turn,true,before});
    check(bool(request.draft),"Actual end-turn flag request rejected");
    const auto &requested=*request.draft;
    const auto gate=gate_bnw_update_moves_original_draft(requested.state,requested.owner,input,owner,host.database(),player,requested.input_revision);
    check(gate.source_ai_scan_completed && gate.source_players_to_process.empty() &&
          gate.reason==BnwUpdateMovesPrefixBlocker::send_ai_civs_processed_owner_missing &&
          gate.last_ai_civs_processed==-1 && gate.attempted_turn==state.setup.game_turn &&
          owner.last_ai_civs_processed()==-1 && !requested.owner.processed_auto_moves.at(player) &&
          scalar(const_cast<saves::PlayerCoreObservation&>(requested.state.world.all_players->players.at(player).core),"m_bTurnActive")==1,
          "Native updateMoves skipped engine barrier or changed active turn/processed flags");
    check(gate.blocker.find("SendAICivsProcessed")!=std::string::npos &&
          gate.blocker.find("ForceReset")!=std::string::npos,"Source first blocker is imprecise");
    check(bnw_end_turn_input_revision(requested.state,requested.owner,input,host.database(),player)==requested.input_revision,
          "Rejected updateMoves changed input revision");
    const auto stale=gate_bnw_update_moves_original_draft(requested.state,requested.owner,input,owner,host.database(),player,"stale");
    check(!stale.source_ai_scan_completed && stale.reason==BnwUpdateMovesPrefixBlocker::invalid_input,"Stale completion request entered source prefix");
    auto wrong= requested.state;wrong.world.map_header.guid[0]^=1;
    check(!gate_bnw_update_moves_original_draft(wrong,requested.owner,input,owner,host.database(),player,requested.input_revision).source_ai_scan_completed,"Foreign map entered prefix");
    auto ai=requested.state;std::int64_t first_ai=-1;
    for(std::int64_t id=0;id<64;++id)if(ai.setup.players.at(id).alive && !ai.setup.is_human(ai.setup.players.at(id))) {
        scalar(ai.world.all_players->players.at(id).core,"m_bTurnActive")=1;
        if(first_ai<0)first_ai=id;
    }
    check(first_ai>=0,"Selected lacks synthetic AI roster fixture");
    const auto ai_gate=gate_bnw_update_moves_original_draft(ai,requested.owner,input,owner,host.database(),player,requested.input_revision);
    check(ai_gate.reason==BnwUpdateMovesPrefixBlocker::active_ai_branch && ai_gate.source_players_to_process==std::vector<std::int64_t>{first_ai},
          "Source AI-first scan did not break on first active AI");
    std::cout<<"PASS selected updateMoves exact first engine barrier, original Read reset, AI-first ordering and immutable rejection; units="<<units.size()
             <<" queued_missions="<<missions<<" automated="<<automated<<" needs_ai="<<need_ai<<"; busy/AutoMission/deactivation unexecuted\n";
}
