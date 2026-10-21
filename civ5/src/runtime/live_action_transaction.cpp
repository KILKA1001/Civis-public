#include "civ5/runtime/live_action_transaction.hpp"
#include <memory>
#include <stdexcept>

namespace civ5::runtime {

LiveActionResult apply_live_action(
    LiveSession &session,
    const std::function<LiveActionResult(LiveSessionState &)> &action) {
    if (!action)
        throw std::invalid_argument("Live action callback is missing");
    // Mutate a detached copy. Rejection, exceptions and allocation failure
    // cannot leave a partially changed world or advance its RNG.
    auto replacement = std::make_shared<LiveSessionState>(*session.state_);
    auto result = action(*replacement);
    if (result.outcome == LiveActionOutcome::committed) {
        replacement->nearby_cities.current=false;
        replacement->nearby_cities.order_gap.reset();
        replacement->city_flavor_recipients.current=false;
        replacement->city_strategy_runtime.clear();
        replacement->player_flavor_recipients.current=false;
        replacement->plot_impassability.current=false;
        replacement->adjacent_land.current=false;
        replacement->plot_visibility.current=false;
        replacement->diplomacy_scratchpad.invalidate();
        replacement->notification_load_transients.current=false;
        replacement->player_plot_lists.current=false;
        replacement->player_read_tail.current=false;
        replacement->game_plot_extras.current=false;
        session.advance_state_generation();
        session.state_.swap(replacement);
        // An arbitrary callback is not a typed native debug transition. A
        // rejected/throwing trial retains the old proof; a committed trial
        // cannot publish its unknown cache state as the imported false value.
        session.retire_bnw_client_selection();
        session.retire_bnw_game_debug_owner();
        session.bnw_unhappiness_current_ = false;
    }
    return result;
}

} // namespace civ5::runtime
