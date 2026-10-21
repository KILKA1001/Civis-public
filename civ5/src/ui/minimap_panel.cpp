#include "civ5/ui/minimap_panel.hpp"
#include "civ5/assets/asset_source.hpp"
#include "civ5/assets/resource_mounts.hpp"
#include <algorithm>
#include <charconv>
#include <cmath>
#include <limits>
#include <stdexcept>
#include <variant>

namespace civ5::ui {
namespace {
template <class Predicate>
const content::XmlNode &one(const std::vector<content::XmlNode> &nodes,
                            std::string_view label, Predicate predicate) {
    const content::XmlNode *found = nullptr;
    for (const auto &node : nodes) {
        if (!predicate(node)) continue;
        if (found) throw std::runtime_error("Duplicate MiniMapPanel " + std::string(label));
        found = &node;
    }
    if (!found) throw std::runtime_error("Missing MiniMapPanel " + std::string(label));
    return *found;
}

std::int32_t integer(std::string_view value, std::string_view label) {
    std::int32_t result = 0;
    const auto [end, error] = std::from_chars(value.data(), value.data() + value.size(), result);
    if (value.empty() || error != std::errc{} || end != value.data() + value.size())
        throw std::runtime_error("Invalid MiniMapPanel " + std::string(label));
    return result;
}

std::pair<std::int32_t, std::int32_t> pair(std::string_view value, char separator,
                                           std::string_view label) {
    const auto split = value.find(separator);
    if (split == std::string_view::npos || value.find(separator, split + 1) != std::string_view::npos)
        throw std::runtime_error("Invalid MiniMapPanel " + std::string(label));
    return {integer(value.substr(0, split), label), integer(value.substr(split + 1), label)};
}

MinimapRect size(const content::XmlNode &node) {
    const auto value = node.attribute("Size");
    const auto [width, height] = pair(value, value.find(',') != std::string::npos ? ',' : '.', "Size");
    if (width <= 0 || height <= 0)
        throw std::runtime_error("MiniMapPanel element has nonpositive Size");
    return {0, 0, static_cast<std::uint32_t>(width), static_cast<std::uint32_t>(height)};
}

MinimapRect anchored(const content::XmlNode &node) {
    auto rect = size(node);
    const auto [x, y] = pair(node.attribute("Offset"), ',', "Offset");
    rect.x = x;
    rect.y = y;
    return rect;
}

void require(bool condition, std::string_view reason) {
    if (!condition) throw std::runtime_error("MiniMapPanel " + std::string(reason));
}

MinimapRect right_bottom(MinimapRect rect, std::uint32_t viewport_width,
                         std::uint32_t viewport_height) {
    const auto x = static_cast<std::int64_t>(viewport_width) - rect.width - rect.x;
    const auto y = static_cast<std::int64_t>(viewport_height) - rect.height - rect.y;
    require(x >= std::numeric_limits<std::int32_t>::min() &&
                x <= std::numeric_limits<std::int32_t>::max() &&
                y >= std::numeric_limits<std::int32_t>::min() &&
                y <= std::numeric_limits<std::int32_t>::max(),
            "anchored rectangle overflows int32");
    rect.x = static_cast<std::int32_t>(x);
    rect.y = static_cast<std::int32_t>(y);
    return rect;
}
std::int32_t checked_coordinate(std::int64_t value) {
    require(value >= std::numeric_limits<std::int32_t>::min() &&
                value <= std::numeric_limits<std::int32_t>::max(),
            "coordinate overflows int32");
    return static_cast<std::int32_t>(value);
}
} // namespace

MinimapTextureGeometry plan_minimap_texture_geometry(float world_span_x,
                                                     float world_span_y) {
    require(std::isfinite(world_span_x) && std::isfinite(world_span_y) &&
                world_span_x > 0.0f && world_span_y > 0.0f,
            "world spans are invalid");
    constexpr std::uint32_t texture_width = 320, texture_height = 190;
    const float texture_aspect = static_cast<float>(texture_width) /
                                 static_cast<float>(texture_height);
    // 0x004FB761 compares two float32 aspect ratios. The narrow-map branch
    // at 0x004FB780..0x004FB804 uses x87 arithmetic, then FISTP with the
    // truncation control word (OR 0xC00), not round-to-nearest.
    const float world_aspect = world_span_x / world_span_y;
    if (texture_aspect <= world_aspect)
        return {0, texture_width, texture_aspect};
    const double scaled = static_cast<double>(texture_width) *
                          (static_cast<double>(world_span_x) /
                           (static_cast<double>(texture_aspect) * world_span_y));
    require(std::isfinite(scaled) && scaled >= 1.0 && scaled < texture_width,
            "active world span does not fit the minimap texture");
    const auto active_width = static_cast<std::uint32_t>(scaled);
    return {(texture_width - active_width) >> 1, active_width,
            static_cast<float>(active_width) / static_cast<float>(texture_height)};
}

MinimapPanelLayout MinimapPanelLayout::from_xml(const content::XmlNode &root,
                                                std::string source_key) {
    require(root.name == "Context", "root must be Context");
    require(!source_key.empty(), "source key is empty");
    const auto &frame = one(root.children, "frame", [](const auto &node) {
        return node.name == "Grid" && node.attribute("Anchor") == "R,B" &&
               node.attribute("Style") == "Grid9DetailSix140" &&
               node.attribute("Size") == "355,276";
    });
    const auto &map_box = one(frame.children, "map box", [](const auto &node) {
        return node.name == "Box" && node.attribute("Anchor") == "L,T" &&
               node.attribute("Size") == "320,190";
    });
    (void)one(map_box.children, "Minimap button", [](const auto &node) {
        return node.name == "Button" && node.attribute("ID") == "Minimap" &&
               node.attribute("Anchor") == "C,C" && node.attribute("Size") == "320,190";
    });
    const auto &background = one(root.children, "bottom-right background", [](const auto &node) {
        return node.name == "Image" && node.attribute("Anchor") == "R,B" &&
               node.attribute("Texture") == "assets\\UI\\Art\\WorldView\\BottomRight128x224.dds";
    });
    const auto &side_stack = one(root.children, "SideStack", [](const auto &node) {
        return node.name == "Stack" && node.attribute("ID") == "SideStack" &&
               node.attribute("Anchor") == "R,B" &&
               node.attribute("StackGrowth") == "Right";
    });
    const auto &options_panel = one(side_stack.children, "OptionsPanel", [](const auto &node) {
        return node.name == "Grid" && node.attribute("ID") == "OptionsPanel" &&
               node.attribute("Anchor") == "R,B" &&
               node.attribute("Style") == "Grid9DetailSix140" &&
               node.attribute("Hidden") == "true" &&
               node.attribute("ConsumeMouse") == "1";
    });
    const auto &main_stack = one(options_panel.children, "MainStack", [](const auto &node) {
        return node.name == "Stack" && node.attribute("ID") == "MainStack" &&
               node.attribute("StackGrowth") == "Bottom";
    });
    constexpr std::array<std::pair<std::string_view, std::string_view>,
                         minimap_map_option_count> map_option_sources{{
        {"HideRecommendation", "TXT_KEY_MAP_OPTIONS_RECOMMENDATIONS"},
        {"ShowResources", "TXT_KEY_MAP_OPTIONS_RESOURCE_ICONS"},
        {"ShowYield", "TXT_KEY_MAP_OPTIONS_YIELD_ICONS"},
        {"ShowGrid", "TXT_KEY_MAP_OPTIONS_HEX_GRID"},
        {"ShowTrade", "TXT_KEY_MAP_OPTIONS_TRADE_ROUTES"}}};
    const auto &button_stack = one(root.children, "button stack", [](const auto &node) {
        return node.name == "Stack" && node.attribute("Anchor") == "R,B" &&
               node.attribute("StackGrowth") == "Top";
    });
    const auto &strategic = one(button_stack.children, "StrategicViewButton", [](const auto &node) {
        return node.name == "Button" && node.attribute("ID") == "StrategicViewButton";
    });
    const auto &options = one(button_stack.children, "MapOptionsButton", [](const auto &node) {
        return node.name == "Button" && node.attribute("ID") == "MapOptionsButton";
    });
    require(button_stack.attribute("Padding") == "0" &&
                button_stack.children.size() == 2 &&
                button_stack.children[0].attribute("ID") == "StrategicViewButton" &&
                button_stack.children[1].attribute("ID") == "MapOptionsButton",
            "button stack growth/order changed");
    MinimapPanelLayout result;
    result.source_key = std::move(source_key);
    result.frame_style = frame.attribute("Style");
    result.background_texture = background.attribute("Texture");
    result.strategic_texture = strategic.attribute("Texture");
    result.options_texture = options.attribute("Texture");
    result.options_panel_style = options_panel.attribute("Style");
    require(options_panel.attribute("Size") == "300,300" &&
                options_panel.attribute("Padding") == "30,46" &&
                side_stack.attribute("Padding") == "-20",
            "OptionsPanel source geometry changed");
    const auto [side_x, side_y] = pair(side_stack.attribute("Offset"), ',', "SideStack Offset");
    require((side_x == 334 || side_x == 377) && side_y == -39,
            "SideStack source anchor changed");
    result.options_panel = size(options_panel);
    result.options_panel.x = side_x;
    result.options_panel.y = side_y;
    std::size_t option_index = 0;
    for (const auto &child : main_stack.children) {
        if (child.name != "CheckBox") continue;
        require(option_index < map_option_sources.size(), "unexpected map option checkbox");
        const auto &[id, text_key] = map_option_sources[option_index];
        require(child.attribute("ID") == id && child.attribute("String") == text_key &&
                    child.attribute("IsChecked") == "0",
                "map option checkbox source or order changed");
        result.map_options.push_back({
            static_cast<MinimapMapOption>(option_index), std::string(id), std::string(text_key)});
        ++option_index;
    }
    require(option_index >= 4 &&
                (result.source_key.find("Expansion2") == std::string::npos ||
                 option_index == map_option_sources.size()),
            "map option checkbox is missing");
    const auto corner = std::find_if(frame.children.begin(), frame.children.end(), [](const auto &node) {
        return node.name == "Image" && !node.attribute("Texture").empty();
    });
    if (corner != frame.children.end()) {
        require(corner->attribute("Anchor") == "R,T" &&
                    corner->attribute("AnchorSide") == "I.O",
                "corner anchor changed");
        result.corner_texture = corner->attribute("Texture");
        result.corner = anchored(*corner);
    }
    result.frame = anchored(frame);
    result.map = anchored(map_box);
    result.background = anchored(background);
    result.strategic = size(strategic);
    result.options = size(options);
    const auto [stack_x, stack_y] = pair(button_stack.attribute("Offset"), ',', "button stack Offset");
    result.strategic.x = stack_x;
    result.strategic.y = stack_y;
    result.options.x = stack_x;
    result.options.y = stack_y;
    require(frame.attribute("Anchor") == "R,B" && background.attribute("Anchor") == "R,B" &&
                strategic.attribute("Anchor") == "C,T" && options.attribute("Anchor") == "C,T" &&
                result.strategic.width == result.options.width &&
                result.strategic.height == result.options.height &&
                result.strategic.width == 45 && result.strategic.height == 45 &&
                !result.strategic_texture.empty() && !result.options_texture.empty(),
            "source geometry or textures changed");
    return result;
}

MinimapPanelLayout read_minimap_panel(const std::filesystem::path &path) {
    return MinimapPanelLayout::from_xml(content::read_xml(path), path.generic_string());
}

MinimapPanelLayout read_minimap_panel(const assets::ResourceMountTable &resources,
                                     std::string_view request_key) {
    const auto selected = resources.resolve(assets::AssetKey(std::string(request_key))).asset;
    const auto *handle = std::get_if<assets::AssetHandle>(&selected);
    if (!handle) throw std::runtime_error("MiniMapPanel VFS source is missing or ambiguous");
    const auto bytes = handle->read_bytes(64ULL * 1024 * 1024);
    return MinimapPanelLayout::from_xml(
        content::read_xml_metadata(std::string(bytes.begin(), bytes.end())),
        handle->candidate().original_name);
}

MinimapPanelPlan plan_minimap_panel(const MinimapPanelLayout &layout,
                                   std::uint32_t viewport_width,
                                   std::uint32_t viewport_height) {
    require(viewport_width > 0 && viewport_height > 0 &&
                viewport_width <= INT32_MAX && viewport_height <= INT32_MAX,
            "viewport is invalid");
    MinimapPanelPlan result;
    result.frame = right_bottom(layout.frame, viewport_width, viewport_height);
    result.background = right_bottom(layout.background, viewport_width, viewport_height);
    result.map = layout.map;
    result.map.x = checked_coordinate(static_cast<std::int64_t>(result.frame.x) + layout.map.x);
    result.map.y = checked_coordinate(static_cast<std::int64_t>(result.frame.y) + layout.map.y);
    if (!layout.corner_texture.empty()) {
        result.corner = layout.corner;
        // XML R,T / I.O: horizontal offset is measured in from the right
        // edge; vertical offset places the ornament above the frame top.
        result.corner.x = checked_coordinate(static_cast<std::int64_t>(result.frame.x) +
                                             result.frame.width - layout.corner.width - layout.corner.x);
        result.corner.y = checked_coordinate(static_cast<std::int64_t>(result.frame.y) +
                                             layout.corner.y);
    }
    const auto stack_height = static_cast<std::uint64_t>(layout.strategic.height) + layout.options.height;
    require(stack_height <= viewport_height, "buttons exceed viewport");
    result.strategic = right_bottom(layout.strategic, viewport_width, viewport_height);
    result.options = result.strategic;
    result.options.y = checked_coordinate(static_cast<std::int64_t>(result.strategic.y) -
                                          layout.strategic.height);
    // The Lua owner calls DoAutoSize() after setting checks. This is the
    // authored 300x300 shell, not a measured post-layout retail rectangle.
    result.options_panel = right_bottom(layout.options_panel, viewport_width, viewport_height);
    // The authored frame intentionally bleeds 8 pixels right and 39 pixels
    // below the viewport. Its map, controls and separate background stay visible.
    for (const auto rect : {result.map, result.background, result.strategic, result.options})
        require(rect.x >= 0 && rect.y >= 0 &&
                    static_cast<std::uint64_t>(rect.x) + rect.width <= viewport_width &&
                    static_cast<std::uint64_t>(rect.y) + rect.height <= viewport_height,
                "visible geometry exceeds viewport");
    require(result.frame.x >= 0 && result.frame.y >= 0,
            "frame begins outside viewport");
    require(result.map.x >= result.frame.x && result.map.y >= result.frame.y &&
                static_cast<std::int64_t>(result.map.x) + result.map.width <=
                    static_cast<std::int64_t>(result.frame.x) + result.frame.width &&
                static_cast<std::int64_t>(result.map.y) + result.map.height <=
                    static_cast<std::int64_t>(result.frame.y) + result.frame.height,
            "map is outside the frame");
    return result;
}
} // namespace civ5::ui
