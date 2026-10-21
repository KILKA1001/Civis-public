#pragma once

#include "civ5/content/manifest.hpp"
#include <array>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

namespace civ5::assets { class ResourceMountTable; }

namespace civ5::ui {
enum class MinimapMapOption : std::uint8_t {
    hide_recommendations, show_resources, show_yield, show_grid, show_trade, count
};
constexpr std::size_t minimap_map_option_count =
    static_cast<std::size_t>(MinimapMapOption::count);

struct MinimapMapOptionSource {
    MinimapMapOption option = MinimapMapOption::hide_recommendations;
    std::string id, text_key;
};
struct MinimapRect {
    std::int32_t x = 0, y = 0;
    std::uint32_t width = 0, height = 0;
};

// The XML file is selected by the caller's active UI skin mount. Native VFS
// lookup uses the basename, so the requested path cannot choose a skin.
struct MinimapPanelLayout {
    std::string source_key;
    std::string frame_style;
    std::string background_texture;
    std::string strategic_texture;
    std::string options_texture;
    std::string corner_texture;
    std::string options_panel_style;
    std::vector<MinimapMapOptionSource> map_options;
    MinimapRect frame, map, background, strategic, options, corner;
    MinimapRect options_panel;
    static MinimapPanelLayout from_xml(const content::XmlNode &root, std::string source_key);
};
struct MinimapPanelPlan {
    MinimapRect frame, map, background, strategic, options, corner, options_panel;
};

// The DX9 minimap producer renders into a fixed 320x190 texture. Depending on
// the world-space aspect, only its centered horizontal interval is active.
struct MinimapTextureGeometry {
    std::uint32_t padding_x = 0;
    std::uint32_t active_width = 320;
    float active_aspect = 320.0f / 190.0f;
};
MinimapTextureGeometry plan_minimap_texture_geometry(float world_span_x,
                                                     float world_span_y);

MinimapPanelLayout read_minimap_panel(const std::filesystem::path &path);
MinimapPanelLayout read_minimap_panel(const assets::ResourceMountTable &resources,
                                     std::string_view request_key = "MiniMapPanel.xml");
MinimapPanelPlan plan_minimap_panel(const MinimapPanelLayout &layout,
                                   std::uint32_t viewport_width,
                                   std::uint32_t viewport_height);
} // namespace civ5::ui
