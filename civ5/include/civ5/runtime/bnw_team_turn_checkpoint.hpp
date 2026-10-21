#pragma once

#include "civ5/runtime/session_checkpoint_identity.hpp"
#include "civ5/saves/civis_save.hpp"
#include "civ5/saves/session_snapshot.hpp"

namespace civ5::runtime {

inline constexpr std::uint32_t bnw_team_turn_checkpoint_section_id=40;

// Raw CvTeam persistence for all64 saved teams. Emits internal version2:
// turns_at_war, turns_locked_at_war, CvTeamTechs::no_trade_tech,
// victory_points and complete hashed small_award_achieved wire rows.
// Stores raw state only; no SmallAwards action/notification/readiness grant.
// The owner must pair no_trade_tech with the matching ID8 owned research state.
saves::CivisSaveSection make_bnw_team_turn_checkpoint(
    const saves::SavedSessionSnapshot &world,
    const saves::SavedSessionSnapshot &source,
    const SessionCheckpointIdentity &identity);

// Reads versions1/2. Version1 neither requires nor writes awards/points.
// Validates source identity, complete roster and all old values before any
// destination write. Other team fields, including ID31 research progress,
// remain under their own section owners.
void restore_bnw_team_turn_checkpoint(
    const saves::CivisSaveSection &section,
    saves::SavedSessionSnapshot &world,
    const saves::SavedSessionSnapshot &source,
    const SessionCheckpointIdentity &identity);

} // namespace civ5::runtime
