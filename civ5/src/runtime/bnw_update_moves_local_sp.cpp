#include "civ5/runtime/bnw_update_moves_local_sp.hpp"
#include "civ5/runtime/file_identity.hpp"
#include "civ5/runtime/bnw_ready_units.hpp"
#include <algorithm>
#include <limits>
#include <stdexcept>

namespace civ5::runtime {
namespace {
constexpr auto bnw=saves::GameStateLayout::expansion2_version1;
void need(bool value,const char *why){if(!value)throw std::runtime_error(why);}
std::int64_t scalar(const saves::PlayerCoreObservation &core,std::string_view name) {
    const saves::PlayerCoreField *found=nullptr;
    for(const auto &f:core.fields)if(f.name && std::string_view(f.name)==name){need(!found,"Duplicate local-SP player scalar");found=&f;}
    need(found && found->values.size()==1,"Missing local-SP player scalar");return found->values[0];
}
void append(std::vector<std::uint8_t> &bytes,std::int64_t value){
    for(unsigned i=0;i<8;++i)bytes.push_back(static_cast<std::uint8_t>(std::uint64_t(value)>>(8*i)));
}
bool same_identity(const SessionCheckpointIdentity &a,const SessionCheckpointIdentity &b){
    return a.payload_sha256==b.payload_sha256 && a.rules_sha256==b.rules_sha256;
}
bool same_packet(const BnwLocalAiPacket &a,const BnwLocalAiPacket &b){
    return same_identity(a.identity,b.identity) && a.map_guid==b.map_guid && a.roster_sha256==b.roster_sha256 &&
           a.turn==b.turn && a.participant==b.participant && a.sequence==b.sequence && a.kind==b.kind;
}
std::string participant_roster(const LiveSessionState &state,const content::RulesDatabase &rules,std::int64_t local) {
    need(state.setup.profile==content::TechnologyProfile::bnw && state.setup.game_type==0 &&
         state.world.game_state.layout==bnw && state.world.map_header.layout==bnw && state.world.pregame &&
         state.world.pregame->layout==bnw && state.world.all_players && state.world.all_players->layout==bnw &&
         state.world.all_players->players.size()==64 && state.world.all_players->context.player_count==64 &&
         state.setup.players.size()==64 && state.setup.game_turn>=0 && local>=0 && local<63,
         "Local AI receiver requires a complete ordinary BNW single-player world");
    const auto &pregame=state.world.pregame->archive;
    need(pregame.integers.at("game_type")==0 && pregame.integers.at("game_turn")==state.setup.game_turn &&
         pregame.integers.at("active_player")==local,"Local AI receiver participant/turn binding differs");
    auto setup=state.setup;
    need(setup.game_option_value(rules,"GAMEOPTION_SIMULTANEOUS_TURNS")==0 &&
         setup.game_option_value(rules,"GAMEOPTION_DYNAMIC_TURNS")==0 &&
         setup.game_option_value(rules,"GAMEOPTION_PITBOSS")==0 && state.setup.pitboss!=true,
         "Local AI receiver does not support network/dynamic/simultaneous/PitBoss mode");
    const auto &statuses=pregame.integer_arrays.at("slot_status"), &claims=pregame.integer_arrays.at("slot_claims");
    need(statuses.size()>=64 && claims.size()>=64,"Local AI receiver source slot roster is incomplete");
    std::vector<std::uint8_t> bytes;std::int64_t humans=0;
    for(std::int64_t id=0;id<64;++id) {
        const auto &raw=state.world.all_players->players.at(id);const auto &owned=state.setup.players.at(id);
        need(raw.core.layout==bnw && raw.index==static_cast<std::uint64_t>(id) && raw.saved_player_id==id &&
             scalar(raw.core,"m_eID")==id && owned.slot_status==statuses.at(id) && owned.slot_claim==claims.at(id),
             "Local AI receiver raw/owned roster differs");
        const auto alive=scalar(raw.core,"m_bAlive");need((alive==0 || alive==1) && bool(alive)==owned.alive,
             "Local AI receiver raw/owned alive flag differs");
        need(owned.slot_status>=0 && owned.slot_status<=5 && owned.slot_claim>=0 && owned.slot_claim<=2,
             "Local AI receiver does not infer unknown slot participation");
        // CvPreGame::isHuman: unassigned inactive observer is not an endpoint;
        // SS_MAX_SLOT_STATUS is a known nonhuman sentinel (including empty slots).
        // Assigned/live observer modes remain outside this standalone contract.
        need(owned.slot_status!=4 || (!owned.alive && owned.slot_claim!=2),
             "Local AI receiver does not support assigned or live observer participation");
        if(state.setup.is_human(owned)) {
            ++humans;need(id==local && owned.slot_status==3 && owned.alive,
                "Local AI receiver requires exactly one ordinary local SS_TAKEN human");
        }
        append(bytes,id);append(bytes,owned.slot_status);append(bytes,owned.slot_claim);append(bytes,alive);
    }
    need(humans==1,"Local AI receiver participant set is not exactly one human endpoint");
    append(bytes,local);return identify_bytes(bytes).sha256;
}
}
struct BnwUpdateMovesLocalOperations {
    static void validate(const LiveSessionState &state,const BnwLocalAiBarrierOwner &owner,const content::RulesDatabase &rules) {
        need(owner.identity_.payload_sha256.size()==64 && owner.identity_.rules_sha256.size()==64 &&
             identify_bytes(rules.gameplay_fingerprint_data()).sha256==owner.identity_.rules_sha256 &&
             state.world.map_header.guid==owner.map_guid_ && state.setup.game_turn==owner.turn_ &&
             participant_roster(state,rules,owner.participant_)==owner.roster_sha256_,
             "Local AI receiver world/identity/roster/epoch differs");
    }
    static BnwLocalAiPacket prepare(const LiveSessionState &state,BnwLocalAiBarrierOwner &owner,
        const content::RulesDatabase &rules,BnwLocalAiEventKind kind) {
        validate(state,owner,rules);
        need(kind==BnwLocalAiEventKind::ai_civs_processed || kind==BnwLocalAiEventKind::player_initial_ai_processed ||
             kind==BnwLocalAiEventKind::turn_complete || kind==BnwLocalAiEventKind::turn_unready || kind==BnwLocalAiEventKind::turn_all_complete,
             "Unknown local AI packet kind");
        need(owner.next_sequence_>0 && owner.next_sequence_<UINT64_MAX,"Local AI packet sequence overflow");
        auto draft=owner;
        BnwLocalAiPacket packet{draft.identity_,draft.map_guid_,draft.roster_sha256_,draft.turn_,draft.participant_,draft.next_sequence_++,kind};
        need(draft.outgoing_.emplace(packet.sequence,packet).second,"Duplicate local AI outgoing sequence");
        owner=std::move(draft);return packet;
    }
    static void receive(const LiveSessionState &state,BnwLocalAiBarrierOwner &owner,
        const content::RulesDatabase &rules,const BnwLocalAiPacket &packet) {
        validate(state,owner,rules);
        need(same_identity(packet.identity,owner.identity_) && packet.map_guid==owner.map_guid_ &&
             packet.roster_sha256==owner.roster_sha256_ && packet.turn==owner.turn_ &&
             packet.participant==owner.participant_ && packet.sequence>0,
             "Foreign or stale local AI packet binding");
        const auto seen=std::find_if(owner.delivered_.begin(),owner.delivered_.end(),[&](const auto &p){return p.sequence==packet.sequence;});
        if(seen!=owner.delivered_.end()){need(same_packet(*seen,packet),"Changed local AI retransmission");return;}
        const auto outgoing=owner.outgoing_.find(packet.sequence);
        need(outgoing!=owner.outgoing_.end() && same_packet(outgoing->second,packet),
             "Local AI packet has no matching actual sender operation");
        auto draft=owner;draft.delivered_.push_back(packet);draft.outgoing_.erase(packet.sequence);owner=std::move(draft);
    }
    static bool received(const LiveSessionState &state,const BnwLocalAiBarrierOwner &owner,
        const content::RulesDatabase &rules,BnwLocalAiEventKind kind) {
        validate(state,owner,rules);
        // Exactly one validated endpoint is required. A receipt from the other
        // event channel or an undelivered message cannot satisfy this barrier.
        return std::any_of(owner.delivered_.begin(),owner.delivered_.end(),[&](const auto &p){
            return p.kind==kind && p.turn==owner.turn_ && p.participant==owner.participant_ &&
                   same_identity(p.identity,owner.identity_) && p.roster_sha256==owner.roster_sha256_ && p.map_guid==owner.map_guid_;
        });
    }
    static void run_prefix(BnwEndTurnRequestDraft &request,const BnwEndTurnInputBinding &binding,
        BnwUpdateMovesImportOwner &moves,BnwLocalAiBarrierOwner &barrier,
        const content::RulesDatabase &rules,std::int64_t player,BnwLocalUpdateMovesExecution &result) {
            const auto gate=gate_bnw_update_moves_original_draft(request.state,request.owner,binding,moves,rules,player,request.input_revision);
            need(gate.source_ai_scan_completed && (gate.reason==BnwUpdateMovesPrefixBlocker::send_ai_civs_processed_owner_missing ||
                  gate.reason==BnwUpdateMovesPrefixBlocker::all_ai_civs_processed_owner_missing),
                 gate.blocker.c_str());
            validate(request.state,barrier,rules);
            need(same_identity(moves.identity(),barrier.identity_) && moves.map_guid()==barrier.map_guid_ &&
                 barrier.participant_==player && barrier.outgoing_.empty(),
                 "updateMoves local engine/source owners differ or have undelivered traffic");
            result.completed_steps.push_back(BnwLocalUpdateMovesStep::ai_roster_scan);
            // Native SetLastTurnAICivsProcessed exact order: engine Send FIRST,
            // then member assignment. All messages stay in this private owner.
            if(moves.last_ai_civs_processed_!=request.state.setup.game_turn) {
            send_bnw_local_ai_event(request.state,barrier,rules,BnwLocalAiEventKind::ai_civs_processed);
            result.completed_steps.push_back(BnwLocalUpdateMovesStep::send_ai_civs_processed);
            moves.last_ai_civs_processed_=request.state.setup.game_turn;
            result.completed_steps.push_back(BnwLocalUpdateMovesStep::last_ai_turn_store);
            }
            need(all_bnw_local_ai_civs_processed(request.state,barrier,rules),"Local AI barrier did not receive all required endpoint messages");
            result.completed_steps.push_back(BnwLocalUpdateMovesStep::all_ai_receipt_check);
            moves.process_player_auto_moves_=true; // sequential branch, not simultaneous ready-for-all
            bool delivered_initial=false;
            for(std::int64_t id=0;id<64;++id) {
                const auto &raw=request.state.world.all_players->players.at(id);
                const auto active=scalar(raw.core,"m_bTurnActive");need(active==0 || active==1,"Unknown updateMoves active flag");
                if(moves.initial_ai_processed_.at(id)!=request.state.setup.game_turn) {
                    bool needs_ai=false;
                    if(active) {
                        const auto units=observe_bnw_update_moves_units(request.state,moves,id);
                        needs_ai=std::any_of(units.begin(),units.end(),[](const auto &unit){return unit.needs_ai_update;});
                    }
                    if(!active || !needs_ai) {
                        moves.initial_ai_processed_.at(id)=request.state.setup.game_turn;
                        if(id==player){send_bnw_local_ai_event(request.state,barrier,rules,BnwLocalAiEventKind::player_initial_ai_processed);delivered_initial=true;}
                    }
                }
                if(active && request.state.setup.is_human(request.state.setup.players.at(id)))result.selected_humans.push_back(id);
            }
            result.completed_steps.push_back(BnwLocalUpdateMovesStep::initial_ai_player_flags);
            if(delivered_initial)result.completed_steps.push_back(BnwLocalUpdateMovesStep::local_initial_ai_delivery);
            result.completed_steps.push_back(BnwLocalUpdateMovesStep::human_roster_select);
            need(result.selected_humans==std::vector<std::int64_t>{player},"Bounded local updateMoves does not own exactly one active human");
            result.all_unit_ai_processed=all_bnw_unit_ai_processed(request.state,moves);result.unit_ai_query_complete=true;
            // This separate exact native completion query must remain false
            // when automated units still require AI. Do not substitute receipt.
            need(result.all_unit_ai_processed,"updateMoves requires native AI_unitUpdate for actual unprocessed automated units; local receipts do not complete unit AI");
            need(request.owner.pathfinder_cache.has_value(),"updateMoves common finder ForceReset owner is unavailable");
            need(same_identity(request.owner.pathfinder_cache->identity,moves.identity()),"updateMoves common finder source identity differs");
            force_reset_bnw_update_moves_common_cache(request.state,*request.owner.pathfinder_cache,player);
            result.completed_steps.push_back(BnwLocalUpdateMovesStep::common_finder_force_reset);
    }
    static BnwLocalUpdateMovesExecution execute(const BnwEndTurnRequestDraft &input,
        const BnwEndTurnInputBinding &binding,const BnwUpdateMovesImportOwner &source_moves,
        const BnwLocalAiBarrierOwner &source_barrier,const content::RulesDatabase &rules,std::int64_t player) {
        BnwLocalUpdateMovesExecution result;
        try {
            auto request=input;auto moves=source_moves;auto barrier=source_barrier;
            run_prefix(request,binding,moves,barrier,rules,player,result);
            result.blocker="BNW updateMoves blocked at GetCountReadyUnits/ReadyToMove: native retained IsBusy mission-timer/combat owners are required before AutoMission and processed/deactivation changes";
        } catch(const std::exception &error){result.blocker=error.what();}
        return result;
    }
    static bool turn_complete(const LiveSessionState &state,const BnwLocalAiBarrierOwner &owner,const content::RulesDatabase &rules) {
        validate(state,owner,rules);const BnwLocalAiPacket *last=nullptr;
        for(const auto &packet:owner.delivered_)if(packet.kind==BnwLocalAiEventKind::turn_complete || packet.kind==BnwLocalAiEventKind::turn_unready)
            if(!last || packet.sequence>last->sequence)last=&packet;
        return last && last->kind==BnwLocalAiEventKind::turn_complete;
    }
    static BnwLocalHumanAutoMovesExecution full(const BnwEndTurnRequestDraft &input,
        const BnwEndTurnInputBinding &binding,const BnwUpdateMovesImportOwner &source_moves,
        const BnwLocalAiBarrierOwner &source_barrier,const content::RulesDatabase &rules,std::int64_t player) {
        BnwLocalHumanAutoMovesExecution result;
        try {
            auto draft=std::make_unique<BnwLocalHumanAutoMovesDraft>(BnwLocalHumanAutoMovesDraft{input,source_moves,source_barrier,{},false});
            auto &request=draft->request;auto &moves=draft->moves;auto &engine=draft->engine;
            run_prefix(request,binding,moves,engine,rules,player,result.prefix);
            const auto ready=observe_bnw_ready_player(request,moves,player);
            result.prefix.completed_steps.push_back(BnwLocalUpdateMovesStep::ready_unit_query);
            const auto units=observe_bnw_update_moves_units(request.state,moves,player);
            const auto busy=observe_bnw_busy_player(request.state,*request.owner.busy_state,request.owner.identity,player);
            auto setup=request.state.setup;
            const bool timed=setup.game_option_value(rules,"GAMEOPTION_END_TURN_TIMER_ENABLED")!=0;
            need(scalar(request.state.world.all_players->players.at(player).core,"m_bAutoMoves")==1 && moves.process_player_auto_moves_,
                 "Bounded human AutoMission branch is not actually active");
            for(std::size_t index=0;index<units.size();++index) {
                const auto &unit=units.at(index);const auto &readiness=ready.units.at(index);
                need(unit.mission_count==0,"AutoMission needs a native owner for the actual nonempty mission queue");
                const bool is_busy=std::find(busy.busy_units.begin(),busy.busy_units.end(),unit.id)!=busy.busy_units.end();
                // AutoMission head==null performs only doDelayedDeath. A false
                // delay or actual busy state proves the kill branch is excluded.
                need(!readiness.delayed_death || is_busy,"Empty AutoMission requires native delayed-death kill owner");
                result.prefix.completed_steps.push_back(BnwLocalUpdateMovesStep::empty_auto_mission);
                // HasCompletedMoveMission=false and IsDoingPartialMove=false:
                // both return at head==null before any per-unit path access.
                const auto end=scalar(request.state.world.all_players->players.at(player).core,"m_bEndTurn");
                if(!timed && end && unit.activity==0 && unit.can_move && !unit.automated) {
                    if(moves.force_ending_turn_) {
                        moves.force_ending_turn_=false;
                        result.prefix.completed_steps.push_back(BnwLocalUpdateMovesStep::force_ending_turn_clear);
                    } else {
                        need(send_bnw_local_turn_unready(request.state,engine,rules),"Local SendTurnUnready did not deliver its actual packet");
                        result.prefix.completed_steps.push_back(BnwLocalUpdateMovesStep::turn_unready_delivery);
                        const auto revision=bnw_end_turn_input_revision(request.state,request.owner,binding,rules,player);
                        auto cancel=execute_bnw_end_turn_request_draft(request.state,request.owner,binding,rules,
                            {player,request.state.setup.game_turn,false,revision});
                        need(bool(cancel.draft),cancel.blocker.c_str());request=std::move(*cancel.draft);
                        draft->unit_orders_needed.push_back(unit.id);
                        result.prefix.completed_steps.push_back(BnwLocalUpdateMovesStep::end_turn_cancelled);
                    }
                }
            }
            const auto end=scalar(request.state.world.all_players->players.at(player).core,"m_bEndTurn");
            if(end) {
                request.owner.processed_auto_moves.at(player)=true;
                request.input_revision=bnw_end_turn_input_revision(request.state,request.owner,binding,rules,player);
                result.prefix.completed_steps.push_back(BnwLocalUpdateMovesStep::processed_auto_moves_set);
            } else {
                // Actual accepted local unready clears the readiness channel.
                // A later ready packet would require the native recovery branch.
                need(!has_received_bnw_local_turn_complete(request.state,engine,rules),
                     "Cancelled human end-turn needs native received-turn-complete recovery");
            }
            draft->processed_auto_moves=request.owner.processed_auto_moves.at(player);
            request.requires_auto_move_processing=scalar(request.state.world.all_players->players.at(player).core,"m_bAutoMoves")!=0 && !draft->processed_auto_moves;
            result.prefix.completed_steps.push_back(BnwLocalUpdateMovesStep::update_moves_finished);
            result.draft=std::move(draft);
        } catch(const std::exception &error){result.blocker=error.what();result.prefix.blocker=result.blocker;}
        return result;
    }

};
BnwLocalAiBarrierOwner bind_bnw_local_ai_barrier_original_import(
    const saves::SavedSession &source,const LiveSessionState &state,const BnwGameActivationOwner &activation,
    const BnwEndTurnInputBinding &binding,const content::RulesDatabase &rules,std::int64_t player) {
    const auto moves=bind_bnw_update_moves_original_import(source,activation.identity);
    (void)bnw_end_turn_input_revision(state,activation,binding,rules,player);
    need(source.snapshot.pregame && source.snapshot.pregame->archive.integers.at("game_turn")==state.setup.game_turn &&
         moves.map_guid()==state.world.map_header.guid && same_identity(moves.identity(),binding.identity()),
         "Local AI barrier original source differs");
    return BnwLocalAiBarrierOwner(activation.identity,moves.map_guid(),participant_roster(state,rules,player),player,state.setup.game_turn);
}
BnwLocalAiPacket prepare_bnw_local_ai_packet(const LiveSessionState &state,BnwLocalAiBarrierOwner &owner,
    const content::RulesDatabase &rules,BnwLocalAiEventKind kind){return BnwUpdateMovesLocalOperations::prepare(state,owner,rules,kind);}
void receive_bnw_local_ai_packet(const LiveSessionState &state,BnwLocalAiBarrierOwner &owner,
    const content::RulesDatabase &rules,const BnwLocalAiPacket &packet){BnwUpdateMovesLocalOperations::receive(state,owner,rules,packet);}
void send_bnw_local_ai_event(const LiveSessionState &state,BnwLocalAiBarrierOwner &owner,
    const content::RulesDatabase &rules,BnwLocalAiEventKind kind){
    auto draft=owner;const auto packet=prepare_bnw_local_ai_packet(state,draft,rules,kind);
    receive_bnw_local_ai_packet(state,draft,rules,packet);owner=std::move(draft);
}
bool all_bnw_local_ai_civs_processed(const LiveSessionState &state,const BnwLocalAiBarrierOwner &owner,
    const content::RulesDatabase &rules){return BnwUpdateMovesLocalOperations::received(state,owner,rules,BnwLocalAiEventKind::ai_civs_processed);}
bool has_bnw_local_initial_ai_processed(const LiveSessionState &state,const BnwLocalAiBarrierOwner &owner,
    const content::RulesDatabase &rules){return BnwUpdateMovesLocalOperations::received(state,owner,rules,BnwLocalAiEventKind::player_initial_ai_processed);}
bool has_received_bnw_local_turn_complete(const LiveSessionState &state,const BnwLocalAiBarrierOwner &owner,
    const content::RulesDatabase &rules){return BnwUpdateMovesLocalOperations::turn_complete(state,owner,rules);}
bool has_received_bnw_local_turn_all_complete(const LiveSessionState &state,const BnwLocalAiBarrierOwner &owner,
    const content::RulesDatabase &rules){return BnwUpdateMovesLocalOperations::received(state,owner,rules,BnwLocalAiEventKind::turn_all_complete);}
bool send_bnw_local_turn_unready(const LiveSessionState &state,BnwLocalAiBarrierOwner &owner,const content::RulesDatabase &rules) {
    auto draft=owner;const auto packet=prepare_bnw_local_ai_packet(state,draft,rules,BnwLocalAiEventKind::turn_unready);
    receive_bnw_local_ai_packet(state,draft,rules,packet);
    const bool delivered=std::any_of(draft.delivered().begin(),draft.delivered().end(),[&](const auto &seen){return same_packet(seen,packet);});
    need(delivered,"Local unready receiver did not retain delivery");owner=std::move(draft);return delivered;
}
bool all_bnw_unit_ai_processed(const LiveSessionState &state,const BnwUpdateMovesImportOwner &owner){
    need(state.setup.profile==content::TechnologyProfile::bnw && state.world.game_state.layout==bnw &&
         state.world.map_header.layout==bnw && state.world.map_header.guid==owner.map_guid() &&
         state.world.all_players && state.world.all_players->layout==bnw &&
         state.world.all_players->context.player_count==64 && state.world.all_players->players.size()==64,
         "Native allUnitAIProcessed bound world/roster is unknown");
    for(std::int64_t id=0;id<64;++id) {
        const auto &raw=state.world.all_players->players.at(id);
        need(raw.core.layout==bnw && raw.index==static_cast<std::uint64_t>(id) && raw.saved_player_id==id &&
             scalar(raw.core,"m_eID")==id,"Native allUnitAIProcessed player identity differs");
        const auto active=scalar(raw.core,"m_bTurnActive");need(active==0 || active==1,"Native allUnitAIProcessed active flag is unknown");
        if(active){const auto units=observe_bnw_update_moves_units(state,owner,id);
            if(std::any_of(units.begin(),units.end(),[](const auto &unit){return unit.needs_ai_update;}))return false;}
    }
    return true;
}
BnwLocalUpdateMovesExecution execute_bnw_local_update_moves_prefix_draft(
    const BnwEndTurnRequestDraft &request,const BnwEndTurnInputBinding &binding,
    const BnwUpdateMovesImportOwner &moves,const BnwLocalAiBarrierOwner &barrier,
    const content::RulesDatabase &rules,std::int64_t player){return BnwUpdateMovesLocalOperations::execute(request,binding,moves,barrier,rules,player);}
BnwLocalHumanAutoMovesExecution execute_bnw_local_human_auto_moves_draft(
    const BnwEndTurnRequestDraft &request,const BnwEndTurnInputBinding &binding,
    const BnwUpdateMovesImportOwner &moves,const BnwLocalAiBarrierOwner &engine,
    const content::RulesDatabase &rules,std::int64_t player) {
    return BnwUpdateMovesLocalOperations::full(request,binding,moves,engine,rules,player);
}
} // namespace civ5::runtime
