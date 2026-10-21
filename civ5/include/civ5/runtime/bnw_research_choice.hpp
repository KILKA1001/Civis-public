#pragma once

#include "civ5/runtime/live_action_transaction.hpp"
#include <functional>
#include <optional>
#include <string_view>

namespace civ5::runtime {

// The Lua evaluator must be observational: it may inspect the detached state,
// but may not mutate a script VM, the published session, or external services.
// A normal mutable LuaResearchSelectionServices is not suitable here.
struct BnwResearchChoiceServices {
    std::function<bool()> lua_ready;
    std::function<std::optional<bool>(std::string_view, std::int64_t,
                                      std::int64_t, const LiveSessionState &)> lua_test_all_read_only;
    // UI notifications are emitted only after the detached world commits.
    std::function<void(std::int64_t)> dirty_after_commit;
};

struct BnwResearchChoiceRules {
    const content::TechnologyRules &technologies;
    const content::GlobalDefineCache &defines;
    std::int64_t active_team = -1;
};

// This only edits the owned queue and researching flags. It does not grant a
// technology, advance a turn, or make the entire session continuable.
LiveActionResult push_bnw_research_choice(
    LiveSession &session, std::int64_t player, std::int64_t technology,
    bool clear, const BnwResearchChoiceRules &rules,
    const BnwResearchChoiceServices &services);

} // namespace civ5::runtime
