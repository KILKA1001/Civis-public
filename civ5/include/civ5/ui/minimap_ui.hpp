#pragma once

#include "civ5/ui/minimap_panel.hpp"
#include "civ5/ui/minimap_visible_bounds.hpp"
#include "civ5/ui/minimap_viewport_ring.hpp"
#include "civ5/ui/ui_runtime.hpp"
#include <array>
#include <functional>
#include <optional>
#include <vector>

namespace civ5::ui {

// Published by the live world owner. The texture itself is produced by the
// minimap renderer; this payload only describes the currently visible world.
struct MinimapUiState {
    MinimapVisibleBounds visible{};
    std::vector<MinimapViewportCorners> camera_corners;
    // A full-grid preview is useful for rendering, but its post-load bounds
    // can differ from the retail minimap owner. Panning needs the latter.
    bool visible_bounds_confirmed = false;
    std::optional<bool> strategic_view;
    std::optional<bool> options_visible;
    // Supplied by the actual UI/options owner. XML IsChecked values are only
    // initial widget declarations and cannot stand in for live player state.
    std::array<std::optional<bool>, minimap_map_option_count> map_display{};
};

struct MinimapPanToWorld { float x = 0, y = 0; };
struct MinimapSetStrategicView { bool enabled = false; };
struct MinimapSetOptionsVisible { bool visible = false; };
struct MinimapSetMapDisplay {
    MinimapMapOption option = MinimapMapOption::hide_recommendations;
    bool enabled = false;
    std::int64_t player_id = -1;
    std::int32_t turn = -1;
    std::uint64_t frame = 0;
};

enum class MinimapHitKind { map, strategic, options, map_display };
struct MinimapHit {
    MinimapHitKind kind;
    MinimapRect bounds;
    MinimapMapOption option = MinimapMapOption::hide_recommendations;
};
struct MinimapUiPlan {
    MinimapPanelPlan panel;
    std::vector<MinimapHit> hits;
    std::vector<MinimapViewportRing> viewport_rings;
    UiCommandResult strategic_button{UiCommandStatus::unavailable,
                                     "Strategic view backend is unavailable"};
    UiCommandResult options_button{UiCommandStatus::unavailable,
                                   "Map options panel is unavailable"};
    std::array<UiCommandResult, minimap_map_option_count> map_display_buttons{};
};

MinimapUiPlan plan_minimap_ui(const MinimapPanelLayout &layout,
                             std::uint32_t viewport_width, std::uint32_t viewport_height,
                             const UiFrameSnapshot &frame,
                             const UiGameplayBackend *backend = nullptr);
std::optional<UiCommand> minimap_command_at(const MinimapUiPlan &plan,
                                            const UiFrameSnapshot &frame,
                                            std::int32_t x, std::int32_t y);
std::optional<UiCommand> minimap_map_display_command(const UiFrameSnapshot &frame,
                                                      MinimapMapOption option);
// Provisional Civis row placement within the authored 300x300 OptionsPanel.
// The original Lua calls DoAutoSize(); native row bounds still need capture.
MinimapRect minimap_map_display_row(MinimapRect panel, std::size_t index);

// The session supplies only mechanics that really exist. An omitted callback
// or a rejected action is reported unavailable by UiScreenController.
struct MinimapActionCallbacks {
    std::function<bool(float, float)> pan_to_world;
    std::function<bool(bool)> set_strategic_view;
    std::function<bool(bool)> set_options_visible;
    std::function<bool(MinimapMapOption, bool)> set_map_display;
};
class MinimapUiBackend final : public UiGameplayBackend {
  public:
    explicit MinimapUiBackend(MinimapActionCallbacks callbacks)
        : callbacks_(std::move(callbacks)) {}
    UiCommandResult availability(const UiGameplayCommand &command,
                                  const UiFrameSnapshot &frame) const override;
    UiCommandResult execute(const UiGameplayCommand &command,
                             const UiFrameSnapshot &frame) override;

  private:
    MinimapActionCallbacks callbacks_;
};

} // namespace civ5::ui
