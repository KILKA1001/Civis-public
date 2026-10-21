#pragma once

#include "civ5/runtime/bnw_player_conscript_turn.hpp"
#include "civ5/runtime/bnw_player_conversion_timer_turn.hpp"
#include "civ5/runtime/bnw_player_culture_bomb_turn.hpp"
#include "civ5/runtime/bnw_player_cramped_turn.hpp"
#include "civ5/runtime/bnw_player_uprising_turn.hpp"
#include "civ5/runtime/bnw_player_city_revolt_turn.hpp"
#include "civ5/runtime/bnw_player_maya_turn.hpp"
#include "civ5/runtime/bnw_player_settled_city_turn.hpp"

namespace civ5::runtime {
struct LiveSessionState;

struct BnwPlayerScalarPrefix {
    BnwCultureBombTurnStep culture_bomb;
    BnwConversionTimerTurnStep conversion;
    BnwSettledCityTurnStep settled_city;
    BnwConscriptTurnStep conscript;
};

// CvPlayer::doTurn scalar prefix after doUpdateCacheOnTurn and AI_doTurnPre.
// Applies the four operations in source order to an owned draft atomically.
// The UI intent remains deferred until the enclosing complete turn commits.
// DoUpdateCramped and all later phases are still required before publication.
BnwPlayerScalarPrefix apply_bnw_player_scalar_prefix(
    LiveSessionState &detached_state, std::int64_t player_id,
    std::int64_t active_player_id);

struct BnwPlayerScalarAndCrampedPrefix {
    BnwPlayerScalarPrefix scalars;
    BnwCrampedTurnStep cramped;
};

// Includes the next source-order DoUpdateCramped call. Both the scalar fields
// and the map-derived cramped field roll back if any input is unavailable.
// DoUpdateUprisings and later phases still prevent turn publication.
BnwPlayerScalarAndCrampedPrefix apply_bnw_player_scalar_and_cramped_prefix(
    LiveSessionState &detached_state, std::int64_t player_id,
    std::int64_t active_player_id,
    const content::RulesDatabase &selected_rules,
    const SessionCheckpointIdentity &identity);

struct BnwPlayerEarlyTurnPrefix {
    BnwPlayerScalarAndCrampedPrefix initial;
    BnwUprisingTurnStep uprising;
    BnwCityRevoltTurnStep city_revolt;
    BnwMayaTurnStep maya;
};

// Source-order segment through the Maya calendar check. All changes, including
// gameplay RNG, remain on a detached draft until every step succeeds. The
// enclosing CvPlayer::doTurn and later effects are still unavailable.
BnwPlayerEarlyTurnPrefix apply_bnw_player_early_turn_prefix(
    LiveSessionState &detached_state, std::int64_t player_id,
    std::int64_t active_player_id,
    const content::RulesDatabase &selected_rules,
    const SessionCheckpointIdentity &identity);
} // namespace civ5::runtime
