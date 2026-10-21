#pragma once

#include "civ5/runtime/session_checkpoint_identity.hpp"
#include "civ5/saves/civis_save.hpp"
#include "civ5/saves/session_snapshot.hpp"

namespace civ5::runtime {

inline constexpr std::uint32_t bnw_game_deals_checkpoint_section_id=39;

// Persists the three CvGameDeals lists, including legacy BNW deal/item
// versions, order and all serialized fields, against the immutable payload.
saves::CivisSaveSection make_bnw_game_deals_checkpoint(
    const saves::SavedSessionSnapshot &world,
    const saves::SavedSessionSnapshot &source,
    const SessionCheckpointIdentity &identity);

// Requires destination deals to match the original source baseline. The
// complete section is validated before the live deals replace that baseline.
void restore_bnw_game_deals_checkpoint(
    const saves::CivisSaveSection &section,
    saves::SavedSessionSnapshot &world,
    const saves::SavedSessionSnapshot &source,
    const SessionCheckpointIdentity &identity);

} // namespace civ5::runtime
