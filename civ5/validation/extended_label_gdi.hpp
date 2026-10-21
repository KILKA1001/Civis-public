#pragma once
#include "civ5/ui/glyphgen.hpp"
#include "civ5/ui/native_extended_font_style.hpp"
#include <optional>
#include <span>
#include <stdexcept>

namespace civ5::validation {
// Restricted explicit GDI request. Height is never inferred from a GG name.
// Width zero preserves the old compatibility request; selected native width
// can be applied explicitly after its source/owner transformation is verified.
struct ExtendedLabelGdiSettings {
    std::string face;
    std::int32_t height = 0;
    std::uint32_t atlas_width = 512, atlas_height = 512;
    std::int32_t width = 0;
};
struct SelectedExtendedFontRequest {
    std::string font, style, manifest_sha256, locale_source_sha256;
    std::string language_settings_source_sha256, language_font_name;
    float language_width_scale = 1, language_height_scale = 1;
    ExtendedLabelGdiSettings gdi;
};
struct DecodedPlainLabel {
    std::string utf8, text_sha256, source_sha256;
    civ5::ui::NativeExtendedFontFlags flags;
    std::vector<std::uint32_t> code_points;
};
enum class ExtendedLabelGdiFailure {
    invalid_request, invalid_source_identity, unsupported_text, authored_route,
    font_creation, substituted_face, settings_changed, missing_glyph,
    glyph_metrics, raster_bounds, rasterization, atlas_overflow, advance_overflow
};
class ExtendedLabelGdiError : public std::runtime_error {
  public:
    ExtendedLabelGdiFailure failure;
    std::optional<std::uint32_t> code_point;
    ExtendedLabelGdiError(ExtendedLabelGdiFailure, const char *,
                          std::optional<std::uint32_t> = std::nullopt);
};
// Strict ordinary single-line BMP text. No markup interpretation, shaping,
// control characters, malformed UTF-8 or surrogate fallback is provided.
DecodedPlainLabel decode_extended_plain_label(std::string_view utf8,
    std::string source_sha256, civ5::ui::NativeExtendedFontFlags flags);
struct ExtendedLabelLogFont {
    std::int32_t height = 0, width = 0, escapement = 0, orientation = 0, weight = 0;
    std::uint8_t italic = 0, underline = 0, strikeout = 0, charset = 0;
    std::uint8_t out_precision = 0, clip_precision = 0, quality = 0, pitch_and_family = 0;
    std::string face;
    bool operator==(const ExtendedLabelLogFont &) const = default;
};
struct ExtendedLabelGdiMetrics {
    std::int32_t height = 0, ascent = 0, descent = 0, internal_leading = 0;
    std::int32_t external_leading = 0, average_width = 0, maximum_width = 0;
    std::int32_t weight = 0, overhang = 0, digitized_aspect_x = 0, digitized_aspect_y = 0;
    std::uint32_t first_char = 0, last_char = 0, default_char = 0, break_char = 0;
    std::uint8_t italic = 0, underline = 0, strikeout = 0, pitch_and_family = 0, charset = 0;
};
struct ExtendedLabelGlyphReceipt {
    std::uint32_t code_point = 0;
    std::uint16_t native_glyph_id = 0xffff;
    std::int32_t extent_width = 0, extent_height = 0;
    std::uint32_t metric_black_box_width = 0, metric_black_box_height = 0;
    std::int32_t metric_origin_x = 0, metric_origin_y = 0;
    std::int32_t metric_cell_increment_x = 0, metric_cell_increment_y = 0;
    bool zero_ink = false;
    civ5::ui::GlyphGenGlyphGeometry geometry;
};
struct ExtendedPreparedLabel {
    DecodedPlainLabel label;
    // Includes spaces with empty rectangles; all entries are generated_base.
    std::vector<civ5::ui::NativeExtendedBaseGlyph> glyphs;
    std::int32_t advance_width = 0;
};
struct ExtendedLabelBaseReceipt {
    static constexpr std::string_view schema = "civis-extended-label-base-preparation-v1";
    static constexpr std::string_view mode = "entire-eligible-label-generated";
    static constexpr std::string_view pixel_format = "alpha8-coverage-top-down-tight-rows";
    // GDI compatibility observations, NOT source-derived/native settings proof.
    static constexpr bool source_derived_logfont_proven = false;
    static constexpr bool native_metrics_or_pixels_proven = false;
    SelectedExtendedFontRequest request;
    ExtendedLabelLogFont requested_logfont, applied_logfont;
    std::string resolved_face;
    ExtendedLabelGdiMetrics metrics;
    std::uint32_t text_color = 0, text_align = 0, ext_text_out_options = 0;
    std::int32_t background_mode = 0, map_mode = 0, graphics_mode = 0;
    std::int32_t dpi_x = 0, dpi_y = 0;
    // Explicit compatibility raster policy: green channel, glyph-index draw,
    // isolated BMP glyphs, GetTextExtentPoint32W advances, no kerning/shaping.
    std::int32_t bitmap_side = 64, draw_origin_x = 16, draw_baseline_y = 32;
    std::string applied_settings_sha256, coverage_sha256, geometry_sha256, observation_sha256;
    std::vector<ExtendedLabelGlyphReceipt> generated_glyphs;
};
struct PreparedExtendedLabelBase {
    civ5::ui::GlyphGenAlphaAtlas coverage;
    civ5::ui::GlyphGenGeometry geometry;
    ExtendedLabelBaseReceipt receipt;
    std::vector<ExtendedPreparedLabel> labels;
};
// Fresh runtime plane only: no authored plane is copied or consulted. Every
// selected label must use the extended route, including its ASCII characters.
// On failure throws a typed error; no partial prepared result/fallback escapes.
PreparedExtendedLabelBase prepare_extended_label_base(
    const SelectedExtendedFontRequest &, std::span<const DecodedPlainLabel>);
// Rechecks retained coverage/geometry hashes and exact complete-label records.
// This does not authenticate caller source hashes or fence live UI owners.
bool extended_label_base_payload_matches(const PreparedExtendedLabelBase &);
// Actual preparer observations; caller-provided source identities are labeled
// as declarations, not promoted to an original/native authenticity claim.
std::string extended_label_base_receipt_json(const PreparedExtendedLabelBase &);
} // namespace civ5::validation
