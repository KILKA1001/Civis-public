#pragma once

#include "civ5/runtime/live_session.hpp"
#include <functional>
#include <string>

namespace civ5::runtime {

enum class LiveActionOutcome { committed, rejected };

struct LiveActionResult {
    LiveActionOutcome outcome = LiveActionOutcome::rejected;
    std::string reason;
};

// Internal mutation boundary for a single synchronous game action. The action
// may change only the owned state passed to it; external Lua/UI/audio effects
// must be emitted after commit. The callback runs on a detached copy, so
// rejection and exceptions leave the pre-action state, RNG and caches intact.
LiveActionResult apply_live_action(
    LiveSession &session,
    const std::function<LiveActionResult(LiveSessionState &)> &action);

} // namespace civ5::runtime
