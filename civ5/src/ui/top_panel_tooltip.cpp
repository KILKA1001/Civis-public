#include "civ5/ui/top_panel.hpp"
#include <algorithm>
#include <limits>
#include <stdexcept>

namespace civ5::ui {
TopPanelTooltipPlan plan_top_panel_tooltip(
    const TopPanelLayout &layout, const TopPanelGrid9Style &style,
    const GlyphGenGeometry &text_geometry, const GlyphGenGeometry &icon_geometry,
    std::string_view text,
    const std::function<std::optional<std::uint32_t>(std::string_view)> &icon_code_point,
    TopPanelPoint anchor, std::uint32_t viewport_width, std::uint32_t viewport_height) {
    if (!viewport_width || !viewport_height || viewport_width > INT32_MAX || viewport_height > INT32_MAX)
        throw std::invalid_argument("TopPanel tooltip needs a positive int32 viewport");
    const auto offset = top_panel_element_offset(layout.tooltip_label);
    const auto wrap = top_panel_element_integer(layout.tooltip_label, "WrapWidth");
    const auto leading = top_panel_element_integer(layout.tooltip_label, "LeadingOffset");
    // Ask the existing Grid9 planner for its content inset, so measurement
    // and rendering use precisely the same padding interpretation.
    const auto available = plan_top_panel_grid9(layout, style, {0, 0, viewport_width, viewport_height});
    const auto inset_x = std::int64_t{available.content.x} + offset.x;
    const auto inset_y = std::int64_t{available.content.y} + offset.y;
    const auto line_step = std::int64_t{text_geometry.line_height} + leading;
    const auto room = std::int64_t{viewport_width} - 2 * inset_x;
    if (offset.x < 0 || offset.y < 0 || wrap <= 0 || text_geometry.line_height <= 0 ||
        line_step <= 0 || line_step > INT32_MAX || room <= 0)
        throw std::invalid_argument("TopPanel tooltip has invalid label metrics or padding");

    TopPanelTooltipPlan result;
    result.line_step = static_cast<std::int32_t>(line_step);
    result.lines = project_glyphgen_markup_block(text_geometry, icon_geometry, text, icon_code_point,
                                                static_cast<std::int32_t>(std::min<std::int64_t>(wrap, room)));
    if (result.lines.size() > static_cast<std::size_t>(INT32_MAX / line_step))
        throw std::invalid_argument("TopPanel tooltip has too many lines");
    constexpr std::int64_t baseline_offset = 8;
    std::int64_t left = 0, top = 0, right = 0;
    std::int64_t bottom = (result.lines.size() - 1) * line_step + text_geometry.line_height;
    for (std::size_t i = 0; i < result.lines.size(); ++i) {
        const auto &line = result.lines[i];
        right = std::max(right, std::int64_t{line.advance_width});
        for (const auto &glyph : line.glyphs) {
            const auto x = std::int64_t{glyph.left_from_line_origin};
            const auto y = static_cast<std::int64_t>(i) * line_step - glyph.top_from_baseline;
            left = std::min(left, x);
            right = std::max(right, x + glyph.width);
            top = std::min(top, y);
            bottom = std::max(bottom, y + glyph.height);
        }
    }
    const auto min_width = std::int64_t{style.source[0].width} + style.source[2].width;
    const auto min_height = std::int64_t{style.source[0].height} + style.source[6].height;
    const auto width = std::max(min_width, right - left + 2 * inset_x);
    const auto height = std::max(min_height, bottom - top + 2 * inset_y + baseline_offset);
    if (width <= 0 || height <= 0 || width > viewport_width || height > viewport_height)
        throw std::invalid_argument("TopPanel tooltip text cannot fit the viewport");
    const auto x = std::clamp<std::int64_t>(std::int64_t{anchor.x} - width / 2, 0, viewport_width - width);
    const auto y = std::clamp<std::int64_t>(anchor.y, 0, viewport_height - height);
    result.grid = plan_top_panel_grid9(layout, style,
                                     {static_cast<std::uint32_t>(x), static_cast<std::uint32_t>(y),
                                      static_cast<std::uint32_t>(width), static_cast<std::uint32_t>(height)});
    result.text_origin = {static_cast<std::int32_t>(x + inset_x - left),
                          static_cast<std::int32_t>(y + inset_y + baseline_offset - top)};
    return result;
}
} // namespace civ5::ui
