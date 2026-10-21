#pragma once
#include "civ5/assets/dds.hpp"
#include "civ5/assets/resource_mounts.hpp"
#include "civ5/content/manifest.hpp"
#include <cstdint>
#include <filesystem>
#include <functional>
#include <array>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

namespace civ5::ui {
struct GlyphGenTexture {
    std::vector<std::pair<std::string, std::string>> attributes;
    std::string name() const;
    std::string source() const;
};
struct GlyphGenStyle {
    std::string name;
    std::vector<std::string> layers;
    std::vector<std::pair<std::string, std::string>> attributes;
};
struct GlyphGenGlyph {
    std::uint32_t code_point = 0;
    std::vector<std::pair<std::string, std::string>> attributes;
    std::string attribute(std::string_view key) const;
};

// Strict reader for the caller-selected Civ V *.ggxml source. Raw authored
// attributes remain available; numeric metric semantics are assigned only by
// project_glyphgen_line below.
struct GlyphGenFont {
    std::vector<std::pair<std::string, std::string>> root_attributes;
    std::vector<std::pair<std::string, std::string>> texture_atlas_attributes;
    std::vector<GlyphGenTexture> textures;
    std::vector<GlyphGenStyle> styles;
    std::vector<std::pair<std::string, std::string>> glyph_container_attributes;
    std::int32_t declared_glyph_count = 0;
    std::vector<GlyphGenGlyph> glyphs;
    static GlyphGenFont from_xml(const content::XmlNode &root);
    const GlyphGenGlyph *glyph(std::uint32_t code_point) const;
    std::uint32_t atlas_width() const;
    std::uint32_t atlas_height() const;
};
GlyphGenFont read_glyphgen(const std::filesystem::path &path);
// Read authored *.ggxml through the caller's active VFS mount order.
GlyphGenFont read_glyphgen(const assets::ResourceMountTable &resources, std::string_view key);

struct GlyphGenGlyphGeometry {
    std::uint32_t code_point = 0;
    std::int32_t atlas_x = 0, atlas_y = 0, width = 0, height = 0;
    std::int32_t a = 0, b = 0, c = 0, origin_x = 0, origin_y = 0;
    std::int64_t advance() const { return std::int64_t{a} + b + c; }
};
struct GlyphGenGeometry {
    std::uint32_t atlas_width = 0, atlas_height = 0;
    std::int32_t line_height = 0, ascent = 0, descent = 0, ascent_adjustment = 0;
    std::int32_t spacing = 0, advance_adjustment = 0;
    std::vector<GlyphGenGlyphGeometry> glyphs;
};
GlyphGenGeometry glyphgen_geometry(const GlyphGenFont &font);

struct GlyphGenProjectedGlyph {
    std::uint32_t code_point = 0;
    std::int32_t atlas_x = 0, atlas_y = 0, width = 0, height = 0;
    std::int32_t left_from_line_origin = 0, top_from_baseline = 0;
};
struct GlyphGenLine {
    std::vector<GlyphGenProjectedGlyph> glyphs;
    std::int32_t advance_width = 0;
};
// Text is UTF-8; malformed sequences and code points absent from the selected
// geometry fail closed. The caller supplies any locale-specific glyph geometry.
GlyphGenLine project_glyphgen_line(const GlyphGenGeometry &geometry, std::string_view text);

struct GlyphGenAlphaAtlas {
    std::uint32_t width = 0, height = 0, block_width = 0, block_height = 0;
    std::uint32_t packed_tile_capacity = 0, maximum_index = 0;
    assets::Bytes alpha;
};
// Reconstruct the observed L16 dictionary-indexed alpha atlas. Both inputs
// must be one-face/one-mip DDS objects with the retail L16 and alpha-only
// profiles respectively.
GlyphGenAlphaAtlas reconstruct_glyphgen_alpha(const GlyphGenFont &font,
                                              const assets::DdsData &index,
                                              const assets::DdsData &packed);

struct GlyphGenRgbaAtlas {
    std::uint32_t width = 0, height = 0, block_width = 0, block_height = 0;
    std::uint32_t packed_tile_capacity = 0, maximum_index = 0;
    assets::Bytes rgba;
};
// Reconstruct an indexed atlas whose packed payload is a verified 32-bit
// masked-RGB DDS. This is the FontIcons counterpart to the alpha-only Base
// layer and preserves the authored channel masks when expanding each pixel.
GlyphGenRgbaAtlas reconstruct_glyphgen_rgba(const GlyphGenFont &font,
                                            const assets::DdsData &index,
                                            const assets::DdsData &packed);

struct GlyphGenInlineGlyph {
    bool icon = false;
    std::uint32_t code_point = 0;
    std::int32_t atlas_x = 0, atlas_y = 0, width = 0, height = 0;
    std::int32_t left_from_line_origin = 0, top_from_baseline = 0;
    std::array<std::uint8_t, 4> color{255, 255, 200, 255};
};
struct GlyphGenInlineLine {
    std::vector<GlyphGenInlineGlyph> glyphs;
    std::int32_t advance_width = 0;
};
// Renderer-neutral single-line bridge for the markup forms used by the base
// TopPanel. Unknown icons, unbalanced colors, newlines, tabs and malformed
// color values fail closed instead of silently dropping source text.
GlyphGenInlineLine project_glyphgen_markup(
    const GlyphGenGeometry &text_geometry, const GlyphGenGeometry &icon_geometry, std::string_view text,
    const std::function<std::optional<std::uint32_t>(std::string_view)> &icon_code_point);
// Preview text layout: retain colors across explicit NEWLINE controls and
// wrap at spaces using the same glyph advances. This compatibility layout
// does not establish the original engine's line-breaking/auto-size semantics.
std::vector<GlyphGenInlineLine> project_glyphgen_markup_block(
    const GlyphGenGeometry &text_geometry, const GlyphGenGeometry &icon_geometry, std::string_view text,
    const std::function<std::optional<std::uint32_t>(std::string_view)> &icon_code_point,
    std::int32_t wrap_width);
} // namespace civ5::ui
