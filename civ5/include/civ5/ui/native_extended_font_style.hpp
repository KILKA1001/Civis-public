#pragma once
#include <array>
#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace civ5::ui {
struct NativeExtendedFontFlags {
    bool use_extended_font = false, force_ime = false, force_non_ime = false;
};
// A per-label choice, including ASCII. This is never a missing-glyph fallback.
enum class NativeExtendedFontRoute { authored_label, extended_label };
NativeExtendedFontRoute native_extended_font_route(NativeExtendedFontFlags flags) noexcept;

struct NativeExtendedFontProof {
    static constexpr std::string_view executable_sha256 =
        "7c9f93d512c49a7ab2bc70f51c6caa10a82ddd697ac11d8a24ff1bb7ce7c3686";
    static constexpr std::uint32_t route_function = 0x710270;
    static constexpr std::uint32_t geometry_function = 0x591020;
    static constexpr std::uint32_t offset_constant = 0x9d144c;
    static constexpr std::uint32_t draw_function = 0x653b50;
    // Verified pre-projection local glyph/UI units; not physical pixels.
    static constexpr std::int32_t shadow_offset_x = 1, shadow_offset_y = 1;
    static constexpr bool native_pixel_parity_proven = false;
};
struct NativeExtendedGlyphRect {
    std::int32_t x = 0, y = 0, width = 0, height = 0;
    bool operator==(const NativeExtendedGlyphRect &) const = default;
};
enum class NativeExtendedGlyphOrigin { unknown, authored, generated_base };
struct NativeExtendedBaseGlyph {
    std::uint32_t code_point = 0;
    NativeExtendedGlyphRect rectangle;
    NativeExtendedGlyphOrigin origin = NativeExtendedGlyphOrigin::unknown;
};
struct NativeExtendedActiveFont {
    std::string font, style, manifest_sha256, settings_sha256, base_plane_sha256;
};
struct NativeExtendedSourceColor {
    std::array<float, 4> rgba{};
    std::string source_sha256;
};
struct NativeExtendedShadowInput {
    NativeExtendedFontFlags flags;
    std::string locale_source_sha256, label_source_sha256;
    std::optional<NativeExtendedActiveFont> active_font;
    std::optional<NativeExtendedSourceColor> foreground, shadow;
    // Decoded/projected plain-label glyph sequence, with spaces retained.
    std::span<const std::uint32_t> label_code_points;
    std::span<const NativeExtendedBaseGlyph> base_glyphs;
    std::int32_t advance_width = 0;
};
// Both layers reference the SAME caller-retained generated Base plane.
// No derived third-slot plane and no Stroke/Soft morphology are represented.
struct NativeExtendedShadowLayer {
    std::uint32_t native_slot = 0, color_index = 0;
    std::int32_t offset_x = 0, offset_y = 0;
    NativeExtendedSourceColor color;
};
class NativeExtendedShadowPlan {
    NativeExtendedActiveFont font_;
    std::string locale_source_sha256_, label_source_sha256_;
    std::vector<NativeExtendedBaseGlyph> glyphs_;
    std::array<NativeExtendedShadowLayer, 2> layers_;
    std::int32_t advance_width_ = 0;
    NativeExtendedShadowPlan() = default;
    friend NativeExtendedShadowPlan make_native_extended_shadow_plan(const NativeExtendedShadowInput &);
  public:
    const NativeExtendedActiveFont &active_font() const noexcept { return font_; }
    std::string_view locale_source_sha256() const noexcept { return locale_source_sha256_; }
    std::string_view label_source_sha256() const noexcept { return label_source_sha256_; }
    std::span<const NativeExtendedBaseGlyph> glyphs() const noexcept { return glyphs_; }
    // Submission order: background slot1, then foreground slot0.
    const std::array<NativeExtendedShadowLayer, 2> &layers() const noexcept { return layers_; }
    std::int32_t advance_width() const noexcept { return advance_width_; }
    NativeExtendedGlyphRect glyph_rectangle(std::size_t glyph, std::size_t submission_layer) const;
};
// Checks complete ordered generated coverage and bounded geometry/color data.
// Digest syntax is checked, NOT authenticity: caller must validate selected
// source payloads, current owners, GDI settings/plane bytes and lifetime fences.
// This pure plan neither rasterizes nor establishes native metrics/blend/parity.
NativeExtendedShadowPlan make_native_extended_shadow_plan(const NativeExtendedShadowInput &input);
} // namespace civ5::ui
