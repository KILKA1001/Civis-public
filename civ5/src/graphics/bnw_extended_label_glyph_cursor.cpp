#include "civ5/graphics/bnw_extended_label_glyph_cursor.hpp"
#include <array>
#include <bit>
#include <stdexcept>
namespace civ5::graphics {
namespace {
void require(bool ok,const char *why){if(!ok)throw std::runtime_error(why);}
#if defined(__i386__) || defined(__x86_64__)
struct alignas(16) Environment {
    std::array<unsigned char,512> bytes{};
    Environment(){__asm__ volatile("fxsave %0":"=m"(bytes)::"memory");}
    ~Environment(){
        __asm__ volatile("fxrstor %0"::"m"(bytes):"memory",
            "st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)",
            "xmm0","xmm1","xmm2","xmm3","xmm4","xmm5","xmm6","xmm7"
#if defined(__x86_64__)
            ,"xmm8","xmm9","xmm10","xmm11","xmm12","xmm13","xmm14","xmm15"
#endif
        );
    }
};
#endif
}
BnwExtendedGlyphCursorResult continue_bnw_extended_glyph_cursor(
    const ui::BnwSystemFontSlotQuote &borrowed_quote,
    const BnwExtendedGlyphCursorControls &borrowed_controls){
    const auto quote=borrowed_quote;
    const auto controls=borrowed_controls;
    require(controls.caller_declared_numeric_inputs,"Current Label cursor producer absent: explicit numeric declaration required");
    require(quote.requested_glyph_index==quote.slot.glyph_index,"Retained source glyph identity differs");
    const auto cw=controls.diagnostic_incoming_x87_control_word;
    require((cw&0xffu)==0x7fu && (cw&~0x0f7fu)==0 && ((cw>>8)&3u)!=1,
        "Explicit masked PC24/53/64 diagnostic x87 profile required");
    // Bit tests avoid NaN comparisons or modifying caller SSE exception flags.
    const auto bits=std::bit_cast<std::uint32_t>(quote.slot.advance_float);
    const auto magnitude=bits&0x7fffffffu;
    require(magnitude<0x7f800000u,"Nonfinite source advance is a refused native masked-indefinite outcome");
    require(magnitude<0x5f000000u || (magnitude==0x5f000000u && (bits&0x80000000u)),
        "Source advance exceeds valid signed64 FISTP domain: masked indefinite is refused");
    BnwExtendedGlyphCursorResult out;
    out.retained_quote=quote;
    out.diagnostic_incoming_x87_control_word=cw;
#if defined(__i386__) || defined(__x86_64__)
    {
        Environment caller;
        __asm__ volatile("fninit\n\tfldcw %0"::"m"(cw):"memory",
            "st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)");
        std::uint16_t saved_cw=0;
        __asm__ volatile("flds %1\n\tfnstcw %0":"=m"(saved_cw):"m"(quote.slot.advance_float):"st");
        const auto truncation_cw=static_cast<std::uint16_t>(saved_cw|0x0c00u);
        __asm__ volatile("fldcw %1\n\tfistpll %0\n\tfldcw %2":"=m"(out.truncated_advance):
            "m"(truncation_cw),"m"(saved_cw):"st","memory");
        __asm__ volatile("fnstcw %0\n\tfnstsw %1":"=m"(out.diagnostic_restored_x87_control_word),
            "=m"(out.diagnostic_x87_status));
    }
    out.advance_low32=static_cast<std::uint32_t>(out.truncated_advance);
    out.cursor_x=controls.cursor_x+out.advance_low32;
    out.maximum_width=out.cursor_x>controls.maximum_width?out.cursor_x:controls.maximum_width;
    return out;
#else
    throw std::runtime_error("Source FISTP continuation requires supported x86 host");
#endif
}
} // namespace civ5::graphics
