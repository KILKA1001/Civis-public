#include "civ5/graphics/bnw_extended_label_glyph_geometry.hpp"
#include <array>
#include <bit>
#include <cmath>
#include <cstring>
#include <stdexcept>
#if defined(__i386__) || defined(__x86_64__)
#include <immintrin.h>
#endif
namespace civ5::graphics {
namespace {
void need(bool ok,const char*why){if(!ok)throw std::runtime_error(why);}
#if defined(__i386__) || defined(__x86_64__)
struct alignas(16) Extended80 {std::array<unsigned char,10>bytes{};};
struct alignas(16) Environment {
 std::array<unsigned char,512>bytes{};
 Environment(){__asm__ volatile("fxsave %0":"=m"(bytes)::"memory");}
 ~Environment(){
  // FXRSTOR restores architectural registers, not just CW/MXCSR. Tell the
  // compiler that no newly computed floating register may survive this point.
  __asm__ volatile("fxrstor %0"::"m"(bytes):"memory",
   "st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)",
   "xmm0","xmm1","xmm2","xmm3","xmm4","xmm5","xmm6","xmm7"
#if defined(__x86_64__)
   ,"xmm8","xmm9","xmm10","xmm11","xmm12","xmm13","xmm14","xmm15"
#endif
  );
 }
 std::uint32_t supported_mxcsr()const{std::uint32_t mask=0;std::memcpy(&mask,bytes.data()+28,4);return mask?mask:0xffbfu;}
};
Extended80 unsigned_cursor(std::uint32_t value){
 const auto signed_bits=std::bit_cast<std::int32_t>(value);Extended80 out;
 // Original FILD signed local, conditional FADD float32 source2^32.
 constexpr float two32=4294967296.0f;
 if(value&0x80000000u){
  __asm__ volatile("fildl %1\n\tfadds %2\n\tfstpt %0":"=m"(out):"m"(signed_bits),"m"(two32));
 }else __asm__ volatile("fildl %1\n\tfstpt %0":"=m"(out):"m"(signed_bits));
 return out;
}
float store_float(const Extended80&value){float out;
 __asm__ volatile("fldt %1\n\tfstps %0":"=m"(out):"m"(value));return out;
}
float add_extended(const Extended80&cursor,float bound){float out;
 __asm__ volatile("fldt %1\n\tfadds %2\n\tfstps %0":"=m"(out):"m"(cursor),"m"(bound));return out;
}
float add_sse(float left,float right){
 // Separate ADDSS, not contraction/reassociation or generic float expression.
 __asm__ volatile("addss %1,%0":"+x"(left):"xm"(right));return left;
}
#endif
}
BnwExtendedGlyphGeometryResult prepare_bnw_extended_glyph_geometry(const BnwExtendedGlyphGeometryInput&borrowed){
 const auto input=borrowed;
 need(input.caller_declared_numeric_inputs,"Current native TextUpdate/parent producer absent: numeric declaration required");
 need(input.native_slot<=2,"Only original extended slots0/1/2 are represented");
 const auto&s=input.retained_descriptor;
 need(s.requested_glyph_index==s.glyph_index,"Source glyph descriptor identity differs");
 for(const auto f:{s.u0,s.v0,s.u1,s.v1,s.x0,s.y0,s.x1,s.y1,s.advance_float})need(std::isfinite(f),"Nonfinite retained source descriptor");
 const auto cw=input.diagnostic_x87_control_word;const auto mx=input.diagnostic_mxcsr;
 need((cw&0x3fu)==0x3fu&&((cw>>8)&3u)!=1,"Explicit masked nonreserved x87 profile required");
 need((mx&0x1f80u)==0x1f80u,"Explicit masked SSE profile required");
 BnwExtendedGlyphGeometryResult out;
 out.native_slot=input.native_slot;out.retained_run_advance=s.advance_float;
 out.diagnostic_x87_control_word=cw;out.diagnostic_mxcsr=mx;
#if defined(__i386__) || defined(__x86_64__)
 {
  Environment previous;
  need((mx&~previous.supported_mxcsr())==0,"Reserved/unsupported MXCSR profile");
  __asm__ volatile("fninit\n\tfldcw %0"::"m"(cw):"memory",
   "st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)");_mm_setcsr(mx);
  const auto x=unsigned_cursor(input.cursor_x),y=unsigned_cursor(input.line_y);
  const float xf=store_float(x),yf=store_float(y);
  const float left_upper=add_extended(x,s.x0),top_left=add_extended(y,s.y0);
  const float right_upper=add_extended(x,s.x1),top_right=add_extended(y,s.y0);
  const float right_lower=add_sse(s.x1,xf),bottom_right=add_sse(s.y1,yf);
  const float left_lower=add_sse(xf,s.x0),bottom_left=add_sse(s.y1,yf);
  const float offset=input.native_slot==1?1.f:0.f;
  auto&v=out.vertices;
  v[0]={add_sse(offset,left_upper),add_sse(top_left,offset),0,input.packed_rgba,s.u0,s.v0};
  v[1]={add_sse(offset,right_upper),add_sse(top_right,offset),0,input.packed_rgba,s.u1,s.v0};
  v[2]={add_sse(offset,right_lower),add_sse(bottom_right,offset),0,input.packed_rgba,s.u1,s.v1};
  v[3]={add_sse(offset,left_lower),add_sse(bottom_left,offset),0,input.packed_rgba,s.u0,s.v1};
  __asm__ volatile("fnstsw %0":"=m"(out.x87_status));out.result_mxcsr=_mm_getcsr();
 }
 for(const auto&v:out.vertices)for(float f:{v.x,v.y,v.z,v.u,v.v})need(std::isfinite(f),"Original numeric geometry overflow: no accepted packed quad");
 return out;
#else
 throw std::runtime_error("Source x87/SSE numeric geometry requires supported x86 host");
#endif
}
} // namespace civ5::graphics
