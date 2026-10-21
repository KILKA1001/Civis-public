#pragma once
#include "civ5/runtime/bnw_busy_state.hpp"
#include "civ5/runtime/bnw_update_moves_prefix.hpp"
#include "civ5/saves/civis_save.hpp"
#include <span>

namespace civ5::runtime {
struct BnwUiTransientCheckpoint {
    BnwUpdateMovesImportOwner moves;
    BnwBusyStateOwner busy;
};
// V1 retains actual busy fields at the original import clock. It does not
// persist engine delivery history, an advanced AI schedule, or UI selection.
// Unsupported scheduling states fail rather than acquire import defaults.
saves::CivisSaveSection make_bnw_ui_transient_checkpoint(
    const LiveSessionState &, const saves::SavedSession &,
    const SessionCheckpointIdentity &, const BnwUpdateMovesImportOwner &,
    const BnwBusyStateOwner &);
BnwUiTransientCheckpoint restore_bnw_ui_transient_checkpoint(
    std::span<const std::uint8_t>, const LiveSessionState &,
    const saves::SavedSession &, const SessionCheckpointIdentity &);
} // namespace civ5::runtime
