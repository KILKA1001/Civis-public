#pragma once

#include "civ5/ui/live_ui_host.hpp"

namespace civ5::ui {

// Published by the owner of the active BUTTONPOPUP_CHOOSE_IDEOLOGY event.
// BNW ChooseIdeologyPopup.lua counts every alive adopter, including players
// whose teams have not met. This payload must be captured in the same frame
// as the popup event; saved policy rows alone cannot supply it.
struct IdeologyAdopterCount {
    std::int64_t branch_id = -1;
    std::uint32_t alive_adopters = 0;
};
// Game.GetNumFreePolicies(branch.ID), read by the owner of this popup event.
// This is the prospective early-adopter award for each branch, not the
// active player's current free-policy balance.
struct IdeologyFreePolicyCount {
    std::int64_t branch_id = -1;
    std::uint32_t free_policies = 0;
};
// Supplied by the selected GameInfo.Policies owner in its native query order.
// label_width/height are the presenter's measurement after LocalizeAndSetText
// with the selected TwCenMT18 and WrapWidth=532 control. Lua sets row height
// to measured label height + 30; the UI never estimates wrapped text metrics.
struct IdeologyTenetHelpRow {
    std::int64_t policy_id = -1;
    std::string policy_type, branch_type, help_key, localized_help;
    std::int32_t level = 0;
    std::int32_t label_width = 0, label_height = 0;
};
struct IdeologyLayoutEvidence {
    std::int64_t player_id = -1;
    std::int32_t turn = -1;
    std::uint64_t frame = 0, event_token = 0;
    std::string selected_source_path;
    std::vector<IdeologyAdopterCount> branches;
    // nullopt means the GameCore value was not read for this event/frame.
    std::optional<std::vector<IdeologyFreePolicyCount>> free_policy_counts;
    // nullopt means no owner evidence; an empty vector is an attested empty
    // result for all three level queries on each ideology branch.
    std::optional<std::vector<IdeologyTenetHelpRow>> tenet_help;
};

// These commands only navigate the local popup. The confirmation Yes hit is
// a disabled ProgressionChooseIdeology and never reaches GameCore.
enum class IdeologyUiActionKind {
    select_branch, view_tenets, close_overlay, toggle_tenet_level
};
struct IdeologyUiAction {
    IdeologyUiActionKind kind = IdeologyUiActionKind::select_branch;
    std::int64_t branch_id = -1;
    std::uint64_t event_token = 0;
    std::int32_t level = 0;
};
struct IdeologyTooltipKey {
    WorldViewRect bounds;
    std::string key;
};
struct IdeologyTenetHeaderState {
    WorldViewRect bounds;
    std::int32_t level = 0;
    bool collapsed = false;
    std::string label_key, icon_markup;
    std::optional<std::string> localized_title;
};

struct IdeologyLayoutPlan {
    std::string selected_source_path;
    std::optional<WorldViewRect> panel, scroll_panel, close_button;
    std::vector<WorldViewPaint> paints;
    std::vector<LiveUiHit> hits;
    // Authored XML key only. A hover presenter/localizer must resolve it.
    std::vector<IdeologyTooltipKey> tooltip_keys;
    // The selected Lua uses [ICON_PLUS]/[ICON_MINUS] with localized header
    // labels. The presenter may resolve these; the row layout uses collapsed.
    std::vector<IdeologyTenetHeaderState> tenet_headers;
    std::optional<IdeologyScrollRegion> scroll_region;
    std::vector<std::string> unresolved;
};

// Authored geometry for the selected Expansion2 XML and its Lua row heights.
// A current owner view enables local row/overlay navigation only. Confirmation
// Yes remains disabled until transactional GameCore/Lua adoption exists.
// Absent popup event or adopter evidence never creates a branch hit.
using IdeologyHeaderLocalizer =
    std::function<std::optional<std::string>(std::string_view)>;
IdeologyLayoutPlan build_ideology_layout(const ProgressionScreenSource &source,
    const UiFrameSnapshot &frame, std::int32_t width, std::int32_t height,
    const LiveUiFrame::IdeologyView *view = nullptr,
    const IdeologyHeaderLocalizer &localize = {});

} // namespace civ5::ui
