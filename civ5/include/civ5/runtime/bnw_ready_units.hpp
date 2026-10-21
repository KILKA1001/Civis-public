#pragma once
#include "civ5/runtime/bnw_busy_state.hpp"
#include "civ5/runtime/bnw_update_moves_prefix.hpp"

namespace civ5::runtime {
struct BnwReadyUnitObservation {
    std::int64_t id=-1;
    bool ready_to_move=false,ready_to_auto=false,delayed_death=false;
};
struct BnwReadyPlayerObservation {
    std::vector<BnwReadyUnitObservation> units;
    std::vector<std::int64_t> counted_ready_ids; // source unit iteration order
    std::int64_t count_ready=0;
    bool has_ready_unit=false,has_auto_unit=false;
};
// Exact CvUnit::ReadyToMove/ReadyToAuto and CvPlayer::GetCountReadyUnits,
// hasReadyUnit/hasAutoUnit. Requires the SAME actual retained busy owner, not
// a saved-cache approximation. No unit action, turn or movement is performed.
BnwReadyPlayerObservation observe_bnw_ready_player(
    const LiveSessionState &,const BnwUpdateMovesImportOwner &,
    const BnwBusyStateOwner &,const SessionCheckpointIdentity &,std::int64_t player);
// Preferred updateMoves binding: consumes the SAME retained busy owner already
// carried by the root end-turn draft; absent owner is a blocker, never rebuilt.
BnwReadyPlayerObservation observe_bnw_ready_player(
    const BnwEndTurnRequestDraft &,const BnwUpdateMovesImportOwner &,std::int64_t player);
} // namespace civ5::runtime
