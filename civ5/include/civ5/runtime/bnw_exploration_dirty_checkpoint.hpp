#pragma once
#include "civ5/runtime/session_checkpoint_identity.hpp"
#include "civ5/saves/session_snapshot.hpp"

namespace civ5::runtime {

// Proposed common section ID. The importer and LiveSession restoration are
// deliberately separate integration work; this is not a player-AI checkpoint.
inline constexpr std::uint32_t bnw_exploration_dirty_checkpoint_section_id=22;

// CvPlot::changeVisibilityCount marks EconomicAI discovery dirty for living
// members of the revealing team. Records the complete 64-player flag domain,
// and rejects any other PlayerStrategy mutation. Sight, meet, notifications,
// and the remaining player state still require their own checkpoints.
saves::CivisSaveSection make_bnw_exploration_dirty_checkpoint(
    const saves::SavedSessionSnapshot &world,
    const saves::SavedSessionSnapshot &source,
    const SessionCheckpointIdentity &identity);

// Validates the entire section and destination before the first write.
void restore_bnw_exploration_dirty_checkpoint(
    const saves::CivisSaveSection &section,
    saves::SavedSessionSnapshot &world,
    const saves::SavedSessionSnapshot &source,
    const SessionCheckpointIdentity &identity);

} // namespace civ5::runtime
