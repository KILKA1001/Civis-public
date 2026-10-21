#pragma once
#include "civ5/runtime/bnw_unit_order_owners.hpp"
namespace civ5::runtime {
struct BnwFinishMovesCommand {
    BnwUnitOrderKey unit;
    std::int64_t expected_turn=-1;
    std::string expected_input_revision;
    std::uint64_t expected_client_revision=0;
};
enum class BnwFinishMovesStep : std::uint8_t {moves_store,dim_flag,common_finder_force_reset,unit_info_dirty,plot_list_dirty};
struct BnwFinishMovesDraft {
    BnwEndTurnRequestDraft request;
    BnwUnitPresentationOwner presentation;
    BnwUnitPresentationBatch batch;
    bool movement_changed=false;
    std::vector<BnwFinishMovesStep> completed_steps;
};
struct BnwFinishMovesExecution {
    std::unique_ptr<BnwFinishMovesDraft> draft;
    std::string blocker;
};
std::string bnw_finish_moves_input_revision(const BnwEndTurnRequestDraft &,const BnwEndTurnInputBinding &,
    const BnwUnitPresentationOwner &,const content::RulesDatabase &,BnwUnitOrderKey);
// The actual stock Unit:FinishMoves API, not the UI MISSION_SKIP command.
// CvLuaUnit::lFinishMoves -> finishMoves -> setMoves(0), including mandatory
// dim and selected cache/dirty effects. Returns private world/client owners;
// no publication, player turn completion, replayed DoTurn or readiness grant.
BnwFinishMovesExecution execute_bnw_finish_moves_draft(const BnwEndTurnRequestDraft &,
    const BnwEndTurnInputBinding &,const BnwUnitPresentationOwner &,
    const content::RulesDatabase &,const BnwFinishMovesCommand &);
} // namespace civ5::runtime
