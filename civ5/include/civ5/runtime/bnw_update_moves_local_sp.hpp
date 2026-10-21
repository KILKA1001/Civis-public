#pragma once
#include "civ5/runtime/bnw_update_moves_prefix.hpp"

namespace civ5::runtime {
enum class BnwLocalAiEventKind : std::uint8_t {ai_civs_processed,player_initial_ai_processed,turn_complete,turn_unready,turn_all_complete};
struct BnwLocalAiPacket {
    SessionCheckpointIdentity identity;
    std::array<std::uint8_t,16> map_guid{};
    std::string roster_sha256;
    std::int64_t turn=-1,participant=-1;
    std::uint64_t sequence=0;
    BnwLocalAiEventKind kind=BnwLocalAiEventKind::ai_civs_processed;
};
class BnwLocalAiBarrierOwner {
  public:
    const SessionCheckpointIdentity &identity() const noexcept {return identity_;}
    std::int64_t participant() const noexcept {return participant_;}
    std::int64_t turn() const noexcept {return turn_;}
    const std::vector<BnwLocalAiPacket> &delivered() const noexcept {return delivered_;}
    std::size_t pending_count() const noexcept {return outgoing_.size();}
  private:
    friend BnwLocalAiBarrierOwner bind_bnw_local_ai_barrier_original_import(
        const saves::SavedSession &,const LiveSessionState &,const BnwGameActivationOwner &,
        const BnwEndTurnInputBinding &,const content::RulesDatabase &,std::int64_t);
    friend struct BnwUpdateMovesLocalOperations;
    BnwLocalAiBarrierOwner(SessionCheckpointIdentity identity,std::array<std::uint8_t,16> guid,
        std::string roster,std::int64_t participant,std::int64_t turn)
        :identity_(std::move(identity)),map_guid_(guid),roster_sha256_(std::move(roster)),participant_(participant),turn_(turn) {}
    SessionCheckpointIdentity identity_;
    std::array<std::uint8_t,16> map_guid_{};
    std::string roster_sha256_;
    std::int64_t participant_=-1,turn_=-1;
    std::uint64_t next_sequence_=1;
    std::map<std::uint64_t,BnwLocalAiPacket> outgoing_;
    std::vector<BnwLocalAiPacket> delivered_;
};
// This is the standalone sequential-SP engine transport: exactly one ordinary
// local human endpoint, no observer/network/hotseat/email/simultaneous mode.
// Original-import bind starts a NEW Civis transport with an actual empty
// delivered history, not restored original gDLL receipts. Civis continuation/
// next-turn rebind needs durable reconstruction and is rejected.
BnwLocalAiBarrierOwner bind_bnw_local_ai_barrier_original_import(
    const saves::SavedSession &,const LiveSessionState &,const BnwGameActivationOwner &,
    const BnwEndTurnInputBinding &,const content::RulesDatabase &,std::int64_t player);
BnwLocalAiPacket prepare_bnw_local_ai_packet(const LiveSessionState &,
    BnwLocalAiBarrierOwner &,const content::RulesDatabase &,BnwLocalAiEventKind);
// Real typed local receiver. Only an outstanding packet from this owner can
// create a receipt. Exact retransmission is idempotent; changed/foreign/stale
// messages reject atomically. A staged but undelivered Send is not a receipt.
void receive_bnw_local_ai_packet(const LiveSessionState &,BnwLocalAiBarrierOwner &,
    const content::RulesDatabase &,const BnwLocalAiPacket &);
void send_bnw_local_ai_event(const LiveSessionState &,BnwLocalAiBarrierOwner &,
    const content::RulesDatabase &,BnwLocalAiEventKind);
bool all_bnw_local_ai_civs_processed(const LiveSessionState &,const BnwLocalAiBarrierOwner &,
    const content::RulesDatabase &);
bool has_bnw_local_initial_ai_processed(const LiveSessionState &,const BnwLocalAiBarrierOwner &,
    const content::RulesDatabase &);
// Source local turn readiness derives from actual delivered complete/unready
// messages. Last sequence wins; other event channels do not affect readiness.
bool has_received_bnw_local_turn_complete(const LiveSessionState &,const BnwLocalAiBarrierOwner &,
    const content::RulesDatabase &);
// Separate standalone client's all-complete channel. Fresh client history is
// empty, not reconstructed original gDLL state; only exact delivered messages
// satisfy the query. No automatic Send and no turn_complete substitution.
bool has_received_bnw_local_turn_all_complete(const LiveSessionState &,const BnwLocalAiBarrierOwner &,
    const content::RulesDatabase &);
bool send_bnw_local_turn_unready(const LiveSessionState &,BnwLocalAiBarrierOwner &,const content::RulesDatabase &);
// Exact CvGame::allUnitAIProcessed query, independent of engine receipt state.
// False is not normalized away; incomplete active unit observations throw.
bool all_bnw_unit_ai_processed(const LiveSessionState &,const BnwUpdateMovesImportOwner &);

enum class BnwLocalUpdateMovesStep : std::uint8_t {
    ai_roster_scan,send_ai_civs_processed,last_ai_turn_store,all_ai_receipt_check,
    initial_ai_player_flags,local_initial_ai_delivery,human_roster_select,common_finder_force_reset,ready_unit_query,empty_auto_mission,
    force_ending_turn_clear,turn_unready_delivery,end_turn_cancelled,processed_auto_moves_set,update_moves_finished
};
struct BnwLocalUpdateMovesExecution {
    // This bounded prefix does not publish a partially finished updateMoves.
    // All delivered packets/native writes occur on private copies and rollback
    // at the first remaining operation lacking a real owner.
    std::vector<BnwLocalUpdateMovesStep> completed_steps;
    std::vector<std::int64_t> selected_humans;
    bool unit_ai_query_complete=false,all_unit_ai_processed=false;
    std::string blocker;
};
// Consumes the SAME typed root end-turn draft/processed-auto-moves owner.
// Does not call DoTurn for an imported active human, deactivate, advance clocks,
// publish external UI, or grant continuation. No generic committed callback.
BnwLocalUpdateMovesExecution execute_bnw_local_update_moves_prefix_draft(
    const BnwEndTurnRequestDraft &,const BnwEndTurnInputBinding &,
    const BnwUpdateMovesImportOwner &,const BnwLocalAiBarrierOwner &,
    const content::RulesDatabase &,std::int64_t player);
struct BnwLocalHumanAutoMovesDraft {
    BnwEndTurnRequestDraft request;
    BnwUpdateMovesImportOwner moves;
    BnwLocalAiBarrierOwner engine;
    std::vector<std::int64_t> unit_orders_needed;
    bool processed_auto_moves=false;
};
struct BnwLocalHumanAutoMovesExecution {
    std::unique_ptr<BnwLocalHumanAutoMovesDraft> draft;
    BnwLocalUpdateMovesExecution prefix;
    std::string blocker;
};
// Complete bounded updateMoves branch for the current local human with known
// empty unit mission queues. Actual unready delivery may cancel EndTurn and
// requires unit orders. Imported active DoTurn/deactivation are never executed.
// A repeat on the same native epoch reuses retained delivered AI receipts.
BnwLocalHumanAutoMovesExecution execute_bnw_local_human_auto_moves_draft(
    const BnwEndTurnRequestDraft &,const BnwEndTurnInputBinding &,
    const BnwUpdateMovesImportOwner &,const BnwLocalAiBarrierOwner &,
    const content::RulesDatabase &,std::int64_t player);
} // namespace civ5::runtime
