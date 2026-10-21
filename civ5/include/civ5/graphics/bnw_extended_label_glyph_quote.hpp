#pragma once
#include "civ5/graphics/bnw_extended_label_glyph_geometry.hpp"
#include "civ5/graphics/gl_bnw_font_shader.hpp"
namespace civ5::graphics {
struct BnwExtendedGlyphQuotedGeometry {
 BnwFontShaderPackedQuad quad;
 BnwExtendedGlyphGeometryResult numeric;
};
// SAME immutable descriptor bytes only. This never resolves a second font or
// claims the cache/atlas quote is current. Coordinated publication/draw MUST
// fence the actual cache run, accepted upload quotes, pair and current client.
inline BnwExtendedGlyphQuotedGeometry prepare_bnw_extended_glyph_quote(
 const ui::BnwSystemFontSlotQuote&borrowed_quote,
 const BnwExtendedGlyphGeometryControls&borrowed_controls){
 const auto quote=borrowed_quote;const auto controls=borrowed_controls;
 const auto&s=quote.slot;BnwExtendedGlyphGeometryInput input;
 static_cast<BnwExtendedGlyphGeometryControls&>(input)=controls;
 input.retained_descriptor={quote.requested_glyph_index,s.glyph_index,
  s.u0,s.v0,s.u1,s.v1,s.x0,s.y0,s.x1,s.y1,s.advance_float};
 const auto numeric=prepare_bnw_extended_glyph_geometry(input);
 return {{quote,numeric.vertices},numeric};
}
} // namespace civ5::graphics
