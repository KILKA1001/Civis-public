#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <type_traits>

namespace civ5::ui {
// Numeric conversion of ONE supplied Label color group. This does not attest
// a live Label, its parser, parent, style, current frame, or native vertex run.
struct NativeLabelColorPackRequest {
 std::array<float,4> rgba{}; // source Label offsets 84,88,8c,90
 std::uint16_t incoming_x87_control_word=0;
};
struct NativeLabelColorPackReceipt {
 std::array<std::uint32_t,4> input_float32_bits{};
 std::uint16_t incoming_x87_control_word=0;
 std::array<std::uint8_t,4> rgba_bytes{};
 std::uint32_t packed_rgba=0; // little endian R,G,B,A; no BGRA conversion
 std::uint8_t masked_invalid_conversion_channels=0;
 bool native_label_ready=false;
};
// Original 5a75ef..5a76c0: FMUL under incoming precision/rounding, then
// FISTP int64 with rounding forced to truncation, followed by low-byte masks.
// All exception masks must be set. NaN/Inf and reserved precision reject.
// No clamping: finite non-unit values preserve the original wrap/indefinite.
NativeLabelColorPackReceipt pack_native_label_color(const NativeLabelColorPackRequest&);

struct NativeLabelPackedVertex {
 float x=0,y=0,z=0;
 std::uint32_t rgba=0;
 float u=0,v=0;
};
// Source format9, UBYTE4N COLOR0; native source offsets, not a live producer.
static_assert(sizeof(NativeLabelPackedVertex)==24);
static_assert(std::is_standard_layout_v<NativeLabelPackedVertex>);
static_assert(sizeof(float)==4 && std::numeric_limits<float>::is_iec559);
static_assert(offsetof(NativeLabelPackedVertex,x)==0);
static_assert(offsetof(NativeLabelPackedVertex,rgba)==12);
static_assert(offsetof(NativeLabelPackedVertex,u)==16);
static_assert(offsetof(NativeLabelPackedVertex,v)==20);
} // namespace civ5::ui
