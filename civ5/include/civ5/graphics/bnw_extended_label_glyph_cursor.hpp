#pragma once
#include "civ5/ui/bnw_system_font_cache_owner.hpp"
namespace civ5::graphics {
// Bounded original591D88..591DC1 numeric continuation. No glyph/line loop,
// TextUpdate, wrapping, font/cache lookup, atlas upload or parent is created.
struct BnwExtendedGlyphCursorControls {
    std::uint32_t cursor_x=0, maximum_width=0;
    std::uint16_t diagnostic_incoming_x87_control_word=0;
    bool caller_declared_numeric_inputs=false;
};
struct BnwExtendedGlyphCursorResult {
    ui::BnwSystemFontSlotQuote retained_quote;
    std::int64_t truncated_advance=0;
    std::uint32_t advance_low32=0, cursor_x=0, maximum_width=0;
    std::uint16_t diagnostic_incoming_x87_control_word=0;
    std::uint16_t diagnostic_restored_x87_control_word=0, diagnostic_x87_status=0;
    bool native_label_ready=false, production_cursor_observed=false;
    bool incoming_production_cw_observed=false;
};
// SAME quote is pinned first. Only advance_float is consumed, at returned+20 /
// rawslot+28 hexadecimal. Raster advance/quad bounds cannot substitute for it.
// Explicit masked PC24/53/64 and all incoming rounding modes are admitted;
// source overrides rounding ONLY for FISTP qword, then restores it. Finite
// representable signed64 inputs only: native masked indefinite outcomes are
// explicitly refused, never clamped or accepted as a valid cursor. Low32 add
// wraps modulo2^32; max width uses unsigned comparison. No currentness claim.
BnwExtendedGlyphCursorResult continue_bnw_extended_glyph_cursor(
    const ui::BnwSystemFontSlotQuote &, const BnwExtendedGlyphCursorControls &);
} // namespace civ5::graphics
