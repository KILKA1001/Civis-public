#pragma once
#include "civ5/runtime/live_session.hpp"

namespace civ5::runtime {

// Proposed ID34. Expansion2 CvTreasury.cpp:651-665 assigns these two fields
// in CalculatePreInflatedCosts; Read/Write uses them at 1011-1012/1032-1033.
// The port holds their current values in MaintenanceContext. ID34 v2
// permits proved ID30 history wire-count transitions; v1 remains readable.
// ID30 v1/v2/v3 owns
// gold/lifetime/history, so it must restore before ID34 when both are present.
inline constexpr std::uint32_t bnw_maintenance_expense_checkpoint_section_id=34;

struct BnwMaintenanceExpenseRestored {
    saves::SavedSessionSnapshot world;
    MaintenanceContext maintenance;
};

// Projects the two raw treasury expense fields from the owned BNW context.
// Raw input must be either source or completely synchronized per player.
// This codec does not execute DoGold, publish UI, or authorize a full turn.
saves::CivisSaveSection make_bnw_maintenance_expense_checkpoint(
    const LiveSessionState &state,
    const saves::SavedSessionSnapshot &source,
    const MaintenanceContext &baseline,
    const SessionCheckpointIdentity &identity,
    const saves::CivisSaveSection *gold_companion=nullptr);

// Returns a detached replacement; rejection leaves both inputs untouched.
// world_before may contain an already restored ID30 gold overlay. It may not
// contain an independently edited expense field or changed player roster.
BnwMaintenanceExpenseRestored restore_bnw_maintenance_expense_checkpoint(
    const saves::CivisSaveSection &section,
    const saves::SavedSessionSnapshot &world_before,
    const saves::SavedSessionSnapshot &source,
    const MaintenanceContext &maintenance_before,
    const MaintenanceContext &baseline,
    const SessionCheckpointIdentity &identity,
    const saves::CivisSaveSection *gold_companion=nullptr);

} // namespace civ5::runtime
