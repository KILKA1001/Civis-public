#pragma once

#include "civ5/ui/selected_bnw_preview_host.hpp"
#include <cstdint>
#include <string>
#include <vector>

namespace civ5::ui {

enum class SelectedBnwResearchBlocker {
    none,
    selected_state_unverified,
    stock_ui_contexts_unverified,
};

// Native candidates are diagnostic only: GameEvents Lua may still veto them.
// This selected owner does not yet have a complete observational Lua binding,
// so the result never authorizes a technology action or hit.
struct SelectedBnwResearchPreview {
    std::int64_t player_id = -1;
    std::int64_t team_id = -1;
    std::int64_t turn = -1;
    std::vector<std::int64_t> native_candidates;
    SelectedBnwResearchBlocker blocker_code = SelectedBnwResearchBlocker::selected_state_unverified;
    std::string blocker;
};

SelectedBnwResearchPreview preview_selected_bnw_research(
    const SelectedBnwPreviewHost &selected);

} // namespace civ5::ui
