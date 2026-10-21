#pragma once
#include "civ5/ui/glyphgen.hpp"
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>
#include <map>
#include <memory>
#include <array>
#include <optional>
namespace civ5::ui {
struct BnwTechAwardMarkupIcon {
 std::string font_key,font_sha256;
 std::shared_ptr<const GlyphGenFont> font;
 std::shared_ptr<const GlyphGenGeometry> geometry;
 std::shared_ptr<const GlyphGenRgbaAtlas> rgba_atlas;
 std::string packed_sha256,index_sha256;
 std::uint32_t code_point=0;
};
struct BnwTechAwardMarkupSources {
 std::map<std::string,std::array<std::uint8_t,4>> colors;
 std::map<std::string,BnwTechAwardMarkupIcon> icons;
};
struct BnwTechAwardStyledGlyph {
 GlyphGenProjectedGlyph glyph;
 std::array<std::optional<std::array<std::uint8_t,4>>,3> color_groups;
 std::optional<BnwTechAwardMarkupIcon> icon;
};
struct BnwTechAwardPlainLabelLine {
 std::size_t byte_begin=0,byte_end=0;
 std::int32_t width=0,baseline_offset=0;
 GlyphGenLine glyphs;
 std::vector<BnwTechAwardStyledGlyph> styled_glyphs;
};
struct BnwTechAwardPlainLabelLayout {
 std::int32_t width=0,height=0,line_step=0;
 std::vector<BnwTechAwardPlainLabelLine> lines;
};
// Source-backed arithmetic only. The real context owner selects the authored
// font, actual language/force route and text before invoking it. No font/GUI
// owner, initialization bit, Style, parent callback or paint is issued here.
// Bounded BMP UTF8, actual SPACE fallback and NEWLINE. The plain entry refuses
// color/icon tags unless the selected owner supplies their real source data.
BnwTechAwardPlainLabelLayout project_bnw_tech_award_plain_label(
 const GlyphGenGeometry&,std::string_view,std::int32_t wrap_width,std::int32_t leading_offset);
// Pure arithmetic input: these maps issue no gameplay/UI authority.
BnwTechAwardPlainLabelLayout project_bnw_tech_award_rich_label(
 const GlyphGenGeometry&,std::string_view,std::int32_t wrap_width,std::int32_t leading_offset,
 const BnwTechAwardMarkupSources&);
}
