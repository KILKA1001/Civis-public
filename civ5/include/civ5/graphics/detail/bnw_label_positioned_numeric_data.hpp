#pragma once
#include "civ5/ui/native_label_color_pack.hpp"
#include <array>
#include <cstdint>

namespace civ5::graphics::detail {
// Private value input. These bits do not attest a live Label, source or Paint.
struct BnwLabelPositionedNumericInputData final {
 std::array<std::uint32_t,9> glyph9_bits{}; // u0,v0,u1,v1,x0,y0,x1,y1,advance
 std::uint32_t cursor_x=0,line_y=0;
 std::array<std::uint32_t,3> style_xyz_offset_bits{};
 std::uint32_t packed_rgba=0;
 std::uint16_t actual_x87_control_word=0;
 std::uint32_t actual_mxcsr=0;
};
struct BnwLabelPositionedNumericResultData final {
 std::array<ui::NativeLabelPackedVertex,4> vertices{}; // TL,TR,BR,BL
 std::uint32_t next_cursor_x=0,advance_low32_bits=0;
 std::uint64_t advance_integer64_bits=0;
 std::uint16_t entry_x87_status=0,effect_x87_status=0,effect_x87_control_word=0;
 std::uint32_t entry_mxcsr=0,effect_mxcsr=0;
};
// Source-limited x87/SSE implementation; unsupported profiles/hosts STOP.
// Caller commits next_cursor_x once per genuine glyph, after its actual passes.
BnwLabelPositionedNumericResultData build_label_positioned_numeric_data(
 const BnwLabelPositionedNumericInputData&);
} // namespace civ5::graphics::detail
