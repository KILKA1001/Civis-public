#pragma once
#include "civ5/ui/native_label_color_pack.hpp"
#include <array>
namespace civ5::graphics {
// Numeric source591020 extended branch ONLY. This does not parse text, select
// a font/style/layer, wrap, advance a run, build a parent, or upload an atlas.
struct BnwExtendedGlyphDescriptor {
 std::uint16_t requested_glyph_index=0,glyph_index=0;
 // Original returned descriptor=raw cache slot+8; the ninth float is
 // returned+20/raw+28 GCP advance. No cache readiness/currentness is asserted.
 float u0=0,v0=0,u1=0,v1=0,x0=0,y0=0,x1=0,y1=0,advance_float=0;
};
struct BnwExtendedGlyphGeometryControls {
 std::uint32_t cursor_x=0,line_y=0; // source uint32 local fields, not float cursor
 unsigned native_slot=0;
 std::uint32_t packed_rgba=0; // actual run word or explicitly diagnostic input
 std::uint16_t diagnostic_x87_control_word=0;
 std::uint32_t diagnostic_mxcsr=0;
 bool caller_declared_numeric_inputs=false;
};
struct BnwExtendedGlyphGeometryInput : BnwExtendedGlyphGeometryControls {
 BnwExtendedGlyphDescriptor retained_descriptor;
};
struct BnwExtendedGlyphGeometryResult {
 std::array<ui::NativeLabelPackedVertex,4> vertices; // TL,TR,BR,BL
 unsigned native_slot=0;
 std::uint16_t diagnostic_x87_control_word=0,x87_status=0;
 std::uint32_t diagnostic_mxcsr=0,result_mxcsr=0;
 // source descriptor+20 = rawslot+28. Retained, NEVER used as raster/quad width.
 float retained_run_advance=0;
 bool native_label_ready=false,production_text_update_observed=false;
 bool incoming_production_cw_observed=false,original_whole_run_class_inputs_verified=false;
};
// Explicit masked x87/SSE profiles are scoped and restored. On x86 hosts this
// uses independent primitive FILD/FADD/FST and ADDSS operations, reproducing
// original mixed precision order. Throws on unmasked/reserved profiles,
// nonfinite input/output or authored/unknown layer. It does not certify that
// the supplied descriptor is current: SAME cache/run/atlas fences are required at
// the coordinated publisher and GPU consumer. No callbacks inside this kernel.
BnwExtendedGlyphGeometryResult prepare_bnw_extended_glyph_geometry(const BnwExtendedGlyphGeometryInput&);
} // namespace civ5::graphics
