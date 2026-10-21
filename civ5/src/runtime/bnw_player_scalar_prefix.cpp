#include "civ5/runtime/bnw_player_scalar_prefix.hpp"
#include "civ5/runtime/live_session.hpp"
#include <utility>

namespace civ5::runtime {
namespace {
BnwPlayerScalarPrefix apply_scalars_in_source_order(
    LiveSessionState &draft, std::int64_t player_id,
    std::int64_t active_player_id) {
    BnwPlayerScalarPrefix result;
    result.culture_bomb = apply_bnw_culture_bomb_timer_turn(draft, player_id);
    result.conversion = apply_bnw_conversion_timer_turn(draft, player_id,
                                                        active_player_id);
    result.settled_city = apply_bnw_turns_since_settled_city_turn(draft, player_id);
    result.conscript = apply_bnw_conscript_count_turn(draft, player_id);
    return result;
}
}

BnwPlayerScalarPrefix apply_bnw_player_scalar_prefix(
    LiveSessionState &state, std::int64_t player_id,
    std::int64_t active_player_id) {
    // A later step can reject a malformed or overflowing source field. Keep
    // the caller's draft intact in that case, including all derived caches.
    auto draft = state;
    auto result = apply_scalars_in_source_order(draft, player_id,
                                                active_player_id);
    state = std::move(draft);
    return result;
}

BnwPlayerScalarAndCrampedPrefix apply_bnw_player_scalar_and_cramped_prefix(
    LiveSessionState &state, std::int64_t player_id,
    std::int64_t active_player_id,
    const content::RulesDatabase &selected_rules,
    const SessionCheckpointIdentity &identity) {
    auto draft = state;
    BnwPlayerScalarAndCrampedPrefix result;
    result.scalars = apply_scalars_in_source_order(draft, player_id,
                                                   active_player_id);
    result.cramped = apply_bnw_player_cramped_turn(draft, player_id,
                                                   selected_rules, identity);
    state = std::move(draft);
    return result;
}

BnwPlayerEarlyTurnPrefix apply_bnw_player_early_turn_prefix(
    LiveSessionState &state, std::int64_t player_id,
    std::int64_t active_player_id,
    const content::RulesDatabase &selected_rules,
    const SessionCheckpointIdentity &identity) {
    auto draft = state;
    BnwPlayerEarlyTurnPrefix result;
    result.initial.scalars = apply_scalars_in_source_order(
        draft, player_id, active_player_id);
    result.initial.cramped = apply_bnw_player_cramped_turn(
        draft, player_id, selected_rules, identity);
    result.uprising = apply_bnw_player_uprising_turn(
        draft, player_id, selected_rules, identity);
    result.city_revolt = apply_bnw_player_city_revolt_turn(
        draft, player_id, selected_rules, identity);
    result.maya = apply_bnw_player_maya_turn(
        draft, player_id, selected_rules, identity);
    state = std::move(draft);
    return result;
}
} // namespace civ5::runtime
