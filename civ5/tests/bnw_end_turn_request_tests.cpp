#include "civ5/runtime/bnw_end_turn_request.hpp"
#include "civ5/ui/selected_bnw_preview_host.hpp"
#include <iostream>
#include <stdexcept>
using namespace civ5;using namespace civ5::runtime;
namespace {
void check(bool v,const char *why) {if(!v)throw std::runtime_error(why);}
std::int64_t &field(LiveSessionState &state,std::string_view name) {
    for(auto &f:state.world.all_players->players.at(0).core.fields)
        if(f.name && std::string_view(f.name)==name)return f.values.at(0);
    throw std::runtime_error("Missing fixture scalar");
}
}
int main(int argc,char **argv) {
    check(argc==3,"Expected selected save and game root");
    auto selected=ui::bootstrap_selected_bnw_preview(argv[1],argv[2],0);
    check(selected.preview.host && !selected.preview.blocker,"Selected host unavailable");
    auto &host=*selected.preview.host;auto source=host.session().state();
    auto continuation=host.session().source();continuation.source.container=saves::SaveContainerKind::civis;
    BnwGameActivationOwner unknown_import{host.identity()};
    bool refused=false;
    try {bind_bnw_end_turn_import(continuation,unknown_import);}catch(const std::exception&){refused=true;}
    check(refused && unknown_import.processed_auto_moves.empty(),"Continuation guessed original-load transients");
    auto malformed=host.session().source();malformed.snapshot.all_players->players.back().saved_player_id=-1;
    refused=false;
    try {bind_bnw_end_turn_import(malformed,unknown_import);}catch(const std::exception&){refused=true;}
    check(refused && unknown_import.processed_auto_moves.empty(),"Failed import published a partial transient roster");
    BnwGameActivationOwner owner{host.identity()};
    const auto binding=bind_bnw_end_turn_import(host.session().source(),owner);
    refused=false;
    try {bind_bnw_end_turn_import(host.session().source(),owner);}catch(const std::exception&){refused=true;}
    check(refused && owner.processed_auto_moves.size()==64,"Live processed-auto-moves owner was reset twice");
    check(owner.processed_auto_moves.size()==64,"Original reset transient roster was not created");
    check(field(source,"m_bEndTurn")==0 && field(source,"m_bAutoMoves")==0 &&
          field(source,"m_bTurnActive")==1,"Selected active human fixture differs");
    const auto revision=bnw_end_turn_input_revision(source,owner,binding,host.database(),0);
    const BnwEndTurnRequest request{0,source.setup.game_turn,true,revision};
    const auto result=execute_bnw_end_turn_request_draft(source,owner,binding,host.database(),request);
    check(result.draft && result.draft->flags_changed && result.draft->requires_auto_move_processing &&
          field(result.draft->state,"m_bEndTurn")==1 && field(result.draft->state,"m_bAutoMoves")==1 &&
          !result.draft->owner.processed_auto_moves.at(0),"Actual request flags were not applied");
    check(field(source,"m_bEndTurn")==0 && field(source,"m_bAutoMoves")==0 &&
          field(result.draft->state,"m_bTurnActive")==1 && result.draft->state.setup.game_turn==source.setup.game_turn &&
          result.draft->state.random.gameplay.state()==source.random.gameplay.state() && owner.pending.empty(),
          "Request mutated its source, deactivated a player, advanced time/RNG or published effects");
    auto stale=execute_bnw_end_turn_request_draft(result.draft->state,result.draft->owner,binding,host.database(),request);
    check(!stale.draft && stale.blocker.find("stale")!=std::string::npos,"Stale request was accepted");
    auto processed=result.draft->owner;processed.processed_auto_moves.at(0)=true;
    const auto now=bnw_end_turn_input_revision(result.draft->state,processed,binding,host.database(),0);
    auto noop=execute_bnw_end_turn_request_draft(result.draft->state,processed,binding,host.database(),
        {0,source.setup.game_turn,true,now});
    check(noop.draft && !noop.draft->flags_changed && !noop.draft->requires_auto_move_processing &&
          noop.draft->owner.processed_auto_moves.at(0),"Idempotent true request reset processed state");
    auto cancelled=execute_bnw_end_turn_request_draft(result.draft->state,processed,binding,host.database(),
        {0,source.setup.game_turn,false,now});
    check(cancelled.draft && field(cancelled.draft->state,"m_bEndTurn")==0 &&
          field(cancelled.draft->state,"m_bAutoMoves")==0 && !cancelled.draft->owner.processed_auto_moves.at(0),
          "Cancellation did not execute the source auto-moves reset");
    auto unusual=source;field(unusual,"m_bAutoMoves")=1;processed=owner;processed.processed_auto_moves.at(0)=true;
    const auto unusual_revision=bnw_end_turn_input_revision(unusual,processed,binding,host.database(),0);
    auto unchanged_end=execute_bnw_end_turn_request_draft(unusual,processed,binding,host.database(),
        {0,source.setup.game_turn,false,unusual_revision});
    check(unchanged_end.draft && unchanged_end.draft->flags_changed &&
          field(unchanged_end.draft->state,"m_bAutoMoves")==0 && !unchanged_end.draft->owner.processed_auto_moves.at(0),
          "Source same-false fallback was skipped");
    auto bad=source;field(bad,"m_bEndTurn")=2;
    check(!execute_bnw_end_turn_request_draft(bad,owner,binding,host.database(),request).draft,
          "Nonboolean flag was accepted");
    bad=source;bad.world.map_header.guid[0]^=1;
    check(!execute_bnw_end_turn_request_draft(bad,owner,binding,host.database(),request).draft,
          "Another map reused the request binding");
    bad=source;bad.setup.players.at(0).slot_status=1;
    check(!execute_bnw_end_turn_request_draft(bad,owner,binding,host.database(),request).draft,
          "AI busy-unit branch was inferred safe");
    auto unknown=owner;unknown.processed_auto_moves.erase(1);unknown.processed_auto_moves.emplace(64,false);
    check(!execute_bnw_end_turn_request_draft(source,unknown,binding,host.database(),request).draft,
          "Malformed unsaved transient roster was accepted");
    auto wrong_turn=request;++wrong_turn.expected_turn;
    check(!execute_bnw_end_turn_request_draft(source,owner,binding,host.database(),wrong_turn).draft,
          "Another turn accepted stale input");
    check(!host.session().readiness().can_continue(),"Request enabled incomplete full-turn readiness");
    std::cout<<"PASS selected active-human setEndTurn/setAutoMoves flags, idempotence, revision and rollback guards\n";
}
