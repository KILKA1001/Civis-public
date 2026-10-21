#pragma once

#include "civ5/ui/popup_screens.hpp"

namespace civ5::ui {

// ChooseReligionPopup.lua obtains these lists only when the player opens a
// belief category. The active Lua owner must publish that click's sequence,
// localized GameInfo.Beliefs rows, and the clipped rectangles after the
// dynamically sized BeliefItemInstance buttons are laid out. Saved rules do
// not establish availability or geometry for a live popup.
enum class ReligionBeliefCategory {
    pantheon, founder, follower, follower2, enhancer, bonus
};

struct ReligionBeliefVisibleRow {
    std::int64_t belief_id = -1;
    std::string name, description;
    WorldViewRect clipped_bounds;
};

struct ReligionBeliefOwnerEvidence {
    std::int64_t player_id = -1;
    std::int32_t turn = -1;
    std::uint64_t owner_sequence = 0, activation_sequence = 0;
    std::int64_t event_id = -1, religion_id = -1;
    std::string selected_source_path;
    std::int32_t viewport_width = 0, viewport_height = 0;
    ReligionBeliefCategory category = ReligionBeliefCategory::pantheon;
    WorldViewRect panel_bounds;
    std::vector<ReligionBeliefVisibleRow> visible_rows;
};

// This contract deliberately has no hit list or gameplay command. Opening a
// category, choosing a row, scrolling, and FoundReligion need later owner
// callbacks and a transactional GameCore/Lua backend.
struct ReligionBeliefReadOnlyPreview {
    bool resolved = false;
    std::string unavailable_reason;
    std::optional<ReligionBeliefCategory> category;
    WorldViewRect panel_bounds;
    std::vector<ReligionBeliefVisibleRow> visible_rows;
};

ReligionBeliefReadOnlyPreview capture_religion_belief_preview(
    const UiFrameSnapshot &frame, std::int32_t viewport_width,
    std::int32_t viewport_height);

} // namespace civ5::ui
