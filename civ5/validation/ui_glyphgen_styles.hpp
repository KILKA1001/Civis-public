#pragma once
#include "ui_screen_gl.hpp"
#include "civ5/ui/glyphgen_style.hpp"
#include <stdexcept>

namespace civ5::ui {
// Prepare Base using the existing locale GDI helper; authored effects stay
// unchanged. Missing runtime effect glyphs are reported by the painter.
struct PreparedUiGlyphStyle {
    std::string font,style;
    GlyphGenStyleAsset authored;
    GlyphGenAlphaAtlas base_atlas;
    GlyphGenGeometry base_geometry;
};
class UiGlyphGenStyleSet {
    struct Entry {
        std::string font,style;
        std::vector<GlyphGenGeometry> geometries;
        std::vector<graphics::GlTexture> textures;
        UiGlyphStyleBinding binding;
    };
    std::vector<Entry> entries_;
  public:
    UiGlyphGenStyleSet(const graphics::GlContext &context,std::vector<PreparedUiGlyphStyle> prepared) {
        entries_.reserve(prepared.size());
        for(auto &source:prepared) {
            if(source.authored.layers.empty()||source.base_geometry.line_height!=glyphgen_geometry(source.authored.font).line_height||
                source.base_atlas.width!=source.base_geometry.atlas_width||source.base_atlas.height!=source.base_geometry.atlas_height)
                throw std::runtime_error("Prepared styled Base does not match selected font metrics");
            entries_.push_back({source.font,source.style,{},{},{}});
            auto &entry=entries_.back();const auto count=source.authored.layers.size();
            entry.geometries.reserve(count);entry.textures.reserve(count);entry.binding.layers.reserve(count);
            for(std::size_t n=0;n<count;++n) {
                const auto &atlas=n==0?source.base_atlas:source.authored.layers[n].atlas;
                entry.geometries.push_back(n==0?std::move(source.base_geometry):std::move(source.authored.layers[n].geometry));
                entry.textures.push_back(graphics::GlTexture::alpha8(context,atlas.width,atlas.height,atlas.alpha));
                entry.binding.layers.push_back({{&entry.geometries.back(),&entry.textures.back()},source.authored.layers[n].color_index});
            }
            entry.binding.layout_geometry=&entry.geometries.front();
        }
    }
    UiGlyphGenStyleSet(const UiGlyphGenStyleSet&)=delete;
    UiGlyphGenStyleSet& operator=(const UiGlyphGenStyleSet&)=delete;
    const UiGlyphStyleBinding *resolve(std::string_view font,std::string_view style) const {
        for(const auto &entry:entries_)if(entry.font==font&&entry.style==style)return &entry.binding;
        return nullptr;
    }
};
} // namespace civ5::ui
