#pragma once
#include "civ5/ui/glyphgen.hpp"
#include "civ5/ui/ui_texture_asset.hpp"

namespace civ5::ui {
struct GlyphGenStyleLayer {
    std::string name;
    std::size_t color_index = 0;
    GlyphGenAlphaAtlas atlas;
    GlyphGenGeometry geometry;
    UiTextureAssetSource source, index_source;
};
struct GlyphGenStyleAsset {
    GlyphGenFont font;
    assets::DdsData base, base_index;
    // Authored foreground-first declarations; paint backgrounds before layer0.
    std::vector<GlyphGenStyleLayer> layers;
};
// Owned source metadata only. This does not create a UIManager/font registry,
// initialized Label, HFONT, GCP input or renderer publication authority.
struct GlyphGenDeclaredStyleLayer {
    std::string name, texture_source;
    std::size_t texture_index = 0;
};
struct GlyphGenStyleMetadata {
    GlyphGenFont font;
    UiTextureAssetSource font_source;
    std::size_t source_style_index = 0;
    std::vector<GlyphGenDeclaredStyleLayer> layers;
};
// Caller retains its selected mount table for this synchronous call. Inputs
// are pinned before resolver callbacks; exact missing styles refuse, without
// adopting the separate native style resolver's missing-name index0 branch.
GlyphGenStyleMetadata read_glyphgen_style_metadata(
    const assets::ResourceMountTable &resources,
    std::string_view font, std::string_view style);
GlyphGenGeometry glyphgen_layer_geometry(const GlyphGenGeometry &layout,
                                         const GlyphGenAlphaAtlas &atlas);
GlyphGenStyleAsset load_glyphgen_style(const assets::ResourceMountTable &resources,
                                      std::string_view font, std::string_view style);
std::optional<std::uint32_t> resolve_ui_label_color(
    const assets::ResourceMountTable &resources, std::string_view declaration);
std::optional<std::uint32_t> city_name_shadow_color(
    const assets::ResourceMountTable &resources);
} // namespace civ5::ui
