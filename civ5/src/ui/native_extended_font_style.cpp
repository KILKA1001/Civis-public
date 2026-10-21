#include "civ5/ui/native_extended_font_style.hpp"
#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>

namespace civ5::ui {
namespace {
void require(bool condition, const char *reason) {
    if (!condition) throw std::invalid_argument(reason);
}
bool digest(std::string_view value) {
    return value.size() == 64 && std::all_of(value.begin(), value.end(), [](char c) {
        return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f');
    });
}
void color(const std::optional<NativeExtendedSourceColor> &value) {
    require(value && digest(value->source_sha256), "Extended Shadow lacks a selected source color");
    for (float component : value->rgba)
        require(std::isfinite(component) && component >= 0 && component <= 1,
                "Extended Shadow source color is nonfinite or outside [0,1]");
}
NativeExtendedGlyphRect translated(NativeExtendedGlyphRect rect, std::int32_t dx, std::int32_t dy) {
    require(rect.width >= 0 && rect.height >= 0, "Extended glyph dimensions are negative");
    const auto x = std::int64_t{rect.x} + dx, y = std::int64_t{rect.y} + dy;
    constexpr auto low = std::numeric_limits<std::int32_t>::min();
    constexpr auto high = std::numeric_limits<std::int32_t>::max();
    require(x >= low && x <= high && y >= low && y <= high &&
            x + rect.width <= high && y + rect.height <= high,
            "Extended glyph/Shadow rectangle overflows UI coordinates");
    return {static_cast<std::int32_t>(x), static_cast<std::int32_t>(y), rect.width, rect.height};
}
bool scalar(std::uint32_t value) {
    return value != 0 && value <= 0x10ffff && !(value >= 0xd800 && value <= 0xdfff);
}
} // namespace
NativeExtendedFontRoute native_extended_font_route(NativeExtendedFontFlags flags) noexcept {
    return flags.force_ime || (flags.use_extended_font && !flags.force_non_ime)
        ? NativeExtendedFontRoute::extended_label : NativeExtendedFontRoute::authored_label;
}
NativeExtendedShadowPlan make_native_extended_shadow_plan(const NativeExtendedShadowInput &input) {
    require(native_extended_font_route(input.flags) == NativeExtendedFontRoute::extended_label,
            "Selected label uses the authored font route");
    require(digest(input.locale_source_sha256) && digest(input.label_source_sha256),
            "Extended label lacks locale/XML source identity");
    require(input.active_font && !input.active_font->font.empty(), "Extended label has no active font");
    const auto &font = *input.active_font;
    require(font.style == "Shadow", "Only the source-defined extended Shadow plan is supported");
    require(digest(font.manifest_sha256) && digest(font.settings_sha256) && digest(font.base_plane_sha256),
            "Extended active font lacks manifest/settings/Base plane identity");
    color(input.foreground); color(input.shadow);
    require(input.advance_width >= 0, "Extended label advance width is negative");
    require(input.label_code_points.size() == input.base_glyphs.size(),
            "Generated Base does not cover the entire eligible label");
    for (std::size_t n = 0; n < input.label_code_points.size(); ++n) {
        const auto &glyph = input.base_glyphs[n];
        require(scalar(input.label_code_points[n]) && glyph.code_point == input.label_code_points[n] &&
                glyph.origin == NativeExtendedGlyphOrigin::generated_base,
                "Extended label coverage is missing, reordered or mixes authored glyphs");
        translated(glyph.rectangle, 0, 0);
        translated(glyph.rectangle, NativeExtendedFontProof::shadow_offset_x,
                   NativeExtendedFontProof::shadow_offset_y);
    }
    NativeExtendedShadowPlan plan;
    plan.font_ = font;
    plan.locale_source_sha256_ = input.locale_source_sha256;
    plan.label_source_sha256_ = input.label_source_sha256;
    plan.glyphs_.assign(input.base_glyphs.begin(), input.base_glyphs.end());
    plan.advance_width_ = input.advance_width;
    plan.layers_ = {{{1, 1, NativeExtendedFontProof::shadow_offset_x, NativeExtendedFontProof::shadow_offset_y, *input.shadow},
                     {0, 0, 0, 0, *input.foreground}}};
    return plan;
}
NativeExtendedGlyphRect NativeExtendedShadowPlan::glyph_rectangle(std::size_t glyph, std::size_t layer) const {
    const auto &selected = layers_.at(layer);
    return translated(glyphs_.at(glyph).rectangle, selected.offset_x, selected.offset_y);
}
} // namespace civ5::ui
