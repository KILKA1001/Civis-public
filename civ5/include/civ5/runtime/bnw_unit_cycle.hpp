#pragma once
#include "civ5/runtime/bnw_ready_units.hpp"
#include "civ5/runtime/bnw_unit_order_owners.hpp"

namespace civ5::runtime {
enum class BnwUnitCycleDirection {forward,backward};
enum class BnwUnitCycleFilter {all,workers};
struct BnwUnitCycleObservation {
    std::optional<BnwUnitOrderKey> next;
    bool wrapped=false;
    std::int32_t source_wire_count=0;
};
struct BnwCycleStartUnitObservation {
    std::optional<BnwUnitOrderKey> next;
    std::optional<std::int32_t> distance;
};
enum class BnwGameUnitCycleBranch {no_selected_head,present_selected_head};
struct BnwGameUnitCycleObservation {
    BnwGameUnitCycleBranch branch=BnwGameUnitCycleBranch::no_selected_head;
    std::optional<BnwUnitOrderKey> cycle_unit,next;
    std::optional<std::int32_t> nearest_distance,source_wire_count;
    bool wrapped=false,send_auto_moves_required=false,clear_selection_required=false;
};
// Pure pre-selection Game::cycleUnits decision. A present foreign head uses
// PlayerCycle(nullptr); an absent head uses Map.findUnit(0,0). Required effects
// are obligations, never completion receipts. Camera selection is read AGAIN
// by the engine after actual select/clear effects and cannot be inferred here.
BnwGameUnitCycleObservation observe_bnw_game_unit_cycle(
    const LiveSessionState &,const BnwUpdateMovesImportOwner &,
    const BnwBusyStateOwner &,const SessionCheckpointIdentity &,std::int64_t player,
    std::optional<BnwUnitOrderKey> selected_head,BnwUnitCycleDirection,BnwUnitCycleFilter);
BnwGameUnitCycleObservation observe_bnw_game_unit_cycle(
    const BnwEndTurnRequestDraft &,const BnwUpdateMovesImportOwner &,std::int64_t player,
    std::optional<BnwUnitOrderKey> selected_head,BnwUnitCycleDirection,BnwUnitCycleFilter);
// Game::cycleUnits with NO selected head calls Map::findUnit(0,0,active,
// true,workers). This is a distinct lookup, not Cycle(nullptr). Strict distance
// ties retain the first live native FLTA slot; direction is not an input.
// Pure observation: no selection, auto-moves, camera or engine completion.
// Caller holds the SAME owner-thread read scope required by Cycle below.
BnwCycleStartUnitObservation observe_bnw_cycle_start_unit(
    const LiveSessionState &,const BnwUpdateMovesImportOwner &,
    const BnwBusyStateOwner &,const SessionCheckpointIdentity &,std::int64_t player,
    BnwUnitCycleFilter);
BnwCycleStartUnitObservation observe_bnw_cycle_start_unit(
    const BnwEndTurnRequestDraft &,const BnwUpdateMovesImportOwner &,
    std::int64_t player,BnwUnitCycleFilter);
// Pure CvUnitCycler::Cycle over the CURRENT decoded ordered CLinkList<int>.
// current is a full ID of an actual unit owned by player, or source nullptr.
// No selection, list rebuild, transport, camera or turn effect is performed.
// Caller must hold the actual owner thread/read scope: these borrowed state
// and retained owners have no public thread/lifetime lease of their own.
BnwUnitCycleObservation observe_bnw_unit_cycle(
    const LiveSessionState &,const BnwUpdateMovesImportOwner &,
    const BnwBusyStateOwner &,const SessionCheckpointIdentity &,std::int64_t player,
    std::optional<BnwUnitOrderKey> current,BnwUnitCycleDirection,BnwUnitCycleFilter);
// Uses the SAME busy owner in this draft; an absent owner is an explicit error.
BnwUnitCycleObservation observe_bnw_unit_cycle(
    const BnwEndTurnRequestDraft &,const BnwUpdateMovesImportOwner &,std::int64_t player,
    std::optional<BnwUnitOrderKey> current,BnwUnitCycleDirection,BnwUnitCycleFilter);
} // namespace civ5::runtime
