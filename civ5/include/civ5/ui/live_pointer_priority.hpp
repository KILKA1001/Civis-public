#pragma once

#include "civ5/ui/world_view.hpp"
#include <algorithm>
#include <cstdint>
#include <vector>

namespace civ5::ui {

// Measured UI hits use the same top-left client coordinates as Win32 pointer
// messages. A focused modal owns the entire viewport; ordinary WorldView
// screens own only their measured controls, leaving empty map pixels to camera.
inline bool live_ui_blocks_camera_pointer(const std::vector<WorldViewRect> &hits,
    bool focused_modal, bool top_panel_visible, std::int32_t x, std::int32_t y) {
    if (focused_modal) return true;
    if (top_panel_visible && y >= 0 && y < 32) return true;
    return std::any_of(hits.begin(), hits.end(), [=](const WorldViewRect &r) {
        return r.width > 0 && r.height > 0 &&
            x >= r.x && y >= r.y &&
            static_cast<std::int64_t>(x) < static_cast<std::int64_t>(r.x) + r.width &&
            static_cast<std::int64_t>(y) < static_cast<std::int64_t>(r.y) + r.height;
    });
}

} // namespace civ5::ui
