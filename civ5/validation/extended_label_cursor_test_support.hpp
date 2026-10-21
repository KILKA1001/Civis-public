#pragma once
#include "civ5/graphics/bnw_extended_label_glyph_cursor.hpp"
#include "extended_label_cursor_io.hpp"
namespace cursor_test {
inline civ5::ui::BnwSystemFontSlotQuote quote(std::uint32_t advance_bits){
    civ5::ui::BnwSystemFontSlotQuote q;q.requested_glyph_index=17;q.slot_index=37;q.cache_hit=true;
    auto &s=q.slot;s.glyph_index=17;s.advance=177;s.advance_float=std::bit_cast<float>(advance_bits);
    s.u0=.11f;s.v0=.23f;s.u1=.67f;s.v1=.89f;s.x0=-.374f;s.y0=-.374f;s.x1=12.0f;s.y1=23.0f;
    s.generation=0x123456789abcdef0ull;s.raster_uploaded=false;return q;
}
inline void same_quote(const civ5::ui::BnwSystemFontSlotQuote &a,const civ5::ui::BnwSystemFontSlotQuote &b){
    need(a.requested_glyph_index==b.requested_glyph_index&&a.slot_index==b.slot_index&&a.cache_hit==b.cache_hit&&
        a.slot.glyph_index==b.slot.glyph_index&&a.slot.advance==b.slot.advance&&a.slot.raster_uploaded==b.slot.raster_uploaded&&a.slot.generation==b.slot.generation,"Continuation mutated retained quote metadata");
    const auto &x=a.slot,&y=b.slot;
    const std::array<float,9> left{x.u0,x.v0,x.u1,x.v1,x.x0,x.y0,x.x1,x.y1,x.advance_float};
    const std::array<float,9> right{y.u0,y.v0,y.u1,y.v1,y.x0,y.y0,y.x1,y.y1,y.advance_float};
    for(unsigned n=0;n<9;++n)need(std::bit_cast<std::uint32_t>(left[n])==std::bit_cast<std::uint32_t>(right[n]),"Continuation retargeted descriptor bytes");
}
inline void diagnostic_only(const civ5::graphics::BnwExtendedGlyphCursorResult &r){need(!r.native_label_ready&&!r.production_cursor_observed&&!r.incoming_production_cw_observed,"Continuation minted native cursor/Label readiness");}
} // namespace cursor_test
