// Include once at global namespace in a private implementation TU.
#include "civ5/graphics/detail/bnw_label_positioned_numeric_data.hpp"
#include <cstring>
#include <stdexcept>
#if (defined(__GNUC__) || defined(__clang__)) && !defined(_MSC_VER) && \
 (defined(__i386__) || defined(__x86_64__)) && defined(__SSE__)
#define CIV5_PRIVATE_LABEL_NUMERIC_GNU_X86 1
#include <cpuid.h>
#else
#define CIV5_PRIVATE_LABEL_NUMERIC_GNU_X86 0
#endif

namespace civ5::graphics::detail {
namespace positioned_numeric_data_private {
void require(bool ok,const char* reason) {
 if(!ok) throw std::runtime_error(reason);
}
#if CIV5_PRIVATE_LABEL_NUMERIC_GNU_X86
struct alignas(16) Environment final {
 std::array<unsigned char,512> bytes;
 Environment() noexcept {
  __asm__ volatile("fxsave %0":"=m"(bytes)::"memory");
 }
 Environment(const Environment&)=delete;
 Environment& operator=(const Environment&)=delete;
 ~Environment() noexcept {
  // Restore the whole captured bank. All FP registers are compiler clobbers:
  // it must spill newly computed values before this architectural restoration.
  __asm__ volatile("fxrstor %0"::"m"(bytes):"memory",
   "st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)",
   "xmm0","xmm1","xmm2","xmm3","xmm4","xmm5","xmm6","xmm7"
#if defined(__x86_64__)
   ,"xmm8","xmm9","xmm10","xmm11","xmm12","xmm13","xmm14","xmm15"
#endif
  );
 }
 template<class T> T at(std::size_t offset) const noexcept {
  T result;std::memcpy(&result,bytes.data()+offset,sizeof(result));return result;
 }
};
void require_host() {
 unsigned a=0,b=0,c=0,d=0;
 require(__get_cpuid(1,&a,&b,&c,&d)!=0 && (d&(1u<<24)) && (d&(1u<<25)),
  "Native Label numeric DATA requires hardware FXSR and SSE");
}
// Raw memory only: no float return ABI, C++ FP arithmetic or NaN admission.
std::uint32_t addss_bits(std::uint32_t left,std::uint32_t right) noexcept {
 std::uint32_t result;
 __asm__ volatile("movss %1,%%xmm0\n\taddss %2,%%xmm0\n\tmovss %%xmm0,%0"
  :"=m"(result):"m"(left),"m"(right):"xmm0","memory");
 return result;
}
void corner_bits(const BnwLabelPositionedNumericInputData& input,
                 std::array<std::uint32_t,10>& scratch) noexcept {
 // scratch: xf,yf,leftUpper,topLeft,rightUpper,topRight,rightLower,
 // leftLower,bottomRight,bottomLeft. Preserve native x87 operand/stack order.
 const std::uint32_t two32=0x4f800000u;
 const auto* glyph=input.glyph9_bits.data();auto* dest=scratch.data();
 __asm__ volatile(
  "fildl %[cx]\n\tcmpl $0,%[cx]\n\tjns 1f\n\tfadds %[two]\n1:\n\t"
  "fsts 0(%[dest])\n\tfld %%st(0)\n\tfadds 16(%[glyph])\n\tfstps 8(%[dest])\n\t"
  "fildl %[cy]\n\tcmpl $0,%[cy]\n\tjns 2f\n\tfadds %[two]\n2:\n\t"
  "fsts 4(%[dest])\n\tfld %%st(0)\n\tfadds 20(%[glyph])\n\tfstps 12(%[dest])\n\t"
  "flds 24(%[glyph])\n\t.byte 0xde,0xc2\n\t" // native FADDP ST2,ST0
  "fxch %%st(1)\n\t"
  "movss 0(%[dest]),%%xmm0\n\tmovss 24(%[glyph]),%%xmm7\n\taddss %%xmm0,%%xmm7\n\t"
  "fstps 16(%[dest])\n\taddss 16(%[glyph]),%%xmm0\n\tfadds 20(%[glyph])\n\t"
  "movss 4(%[dest]),%%xmm2\n\tmovss 28(%[glyph]),%%xmm3\n\taddss %%xmm2,%%xmm3\n\t"
  "movss %%xmm0,28(%[dest])\n\tfstps 20(%[dest])\n\t"
  "movss 28(%[glyph]),%%xmm0\n\tmovss %%xmm3,32(%[dest])\n\taddss %%xmm2,%%xmm0\n\t"
  "movss %%xmm7,24(%[dest])\n\tmovss %%xmm0,36(%[dest])"
  ::[cx]"m"(input.cursor_x),[cy]"m"(input.line_y),[two]"m"(two32),
    [glyph]"r"(glyph),[dest]"r"(dest)
  :"memory","cc","st","st(1)","st(2)","xmm0","xmm2","xmm3","xmm7");
}
std::uint64_t advance_bits(std::uint32_t advance) noexcept {
 std::uint16_t saved_cw=0,trunc_cw=0;std::uint64_t integer_bits=0;
 unsigned temporary=0;
 // FLD precedes FNSTCW exactly as native591D88. Keep live x87 value entirely
 // within one assembly effect; preserve precision/masks, override only RC.
 __asm__ volatile(
  "flds %[advance]\n\tfnstcw %[saved]\n\tmovzwl %[saved],%[temp]\n\t"
  "orl $0xc00,%[temp]\n\tmovw %w[temp],%[trunc]\n\tfldcw %[trunc]\n\t"
  "fistpll %[integer]\n\tfldcw %[saved]"
  :[saved]"=m"(saved_cw),[trunc]"=m"(trunc_cw),[integer]"=m"(integer_bits),
   [temp]"=&r"(temporary)
  :[advance]"m"(advance):"memory","cc","st");
 return integer_bits;
}
#endif
} // namespace positioned_numeric_data_private

BnwLabelPositionedNumericResultData build_label_positioned_numeric_data(
 const BnwLabelPositionedNumericInputData& borrowed) {
 // Only immutable DATA copied here; this entry issues no source/currentness.
 const auto input=borrowed;
#if CIV5_PRIVATE_LABEL_NUMERIC_GNU_X86
 using namespace positioned_numeric_data_private;
 require_host();
 BnwLabelPositionedNumericResultData out;
 std::array<std::uint32_t,24> words{};
 {
  Environment previous;
  const auto cw=previous.at<std::uint16_t>(0);
  const auto mx=previous.at<std::uint32_t>(24);
  const auto advertised_mask=previous.at<std::uint32_t>(28);
  const auto mx_mask=advertised_mask?advertised_mask:0xffbfu;
  require(cw==input.actual_x87_control_word && mx==input.actual_mxcsr,
   "Native Label numeric DATA raw controls no longer match current environment");
  require((cw&0x3fu)==0x3fu && ((cw>>8)&3u)!=1u,
   "Native Label numeric DATA requires masked nonreserved x87 profile");
  require((mx&0x1f80u)==0x1f80u && (mx&~mx_mask)==0,
   "Native Label numeric DATA requires masked supported SSE profile");
  require(previous.at<unsigned char>(4)==0,
   "Native Label numeric DATA requires entire captured x87 stack empty");
  out.entry_x87_status=previous.at<std::uint16_t>(2);out.entry_mxcsr=mx;
  std::array<std::uint32_t,10> base{};
  corner_bits(input,base);
  const std::array<std::uint32_t,4> xs={base[2],base[4],base[6],base[7]};
  const std::array<std::uint32_t,4> ys={base[3],base[5],base[8],base[9]};
  constexpr std::array<unsigned,4> u_index={0,2,2,0},v_index={1,1,3,3};
  for(unsigned i=0;i!=4;++i) {
   auto* vertex=words.data()+i*6;
   vertex[0]=addss_bits(input.style_xyz_offset_bits[0],xs[i]);
   vertex[1]=addss_bits(ys[i],input.style_xyz_offset_bits[1]);
   vertex[2]=input.style_xyz_offset_bits[2];vertex[3]=input.packed_rgba;
   vertex[4]=input.glyph9_bits[u_index[i]];vertex[5]=input.glyph9_bits[v_index[i]];
  }
  out.advance_integer64_bits=advance_bits(input.glyph9_bits[8]);
  out.advance_low32_bits=static_cast<std::uint32_t>(out.advance_integer64_bits);
  out.next_cursor_x=input.cursor_x+out.advance_low32_bits; // unsigned modulo2^32
  __asm__ volatile("fnstsw %0":"=m"(out.effect_x87_status)::"memory");
  __asm__ volatile("fnstcw %0":"=m"(out.effect_x87_control_word)::"memory");
  __asm__ volatile("stmxcsr %0":"=m"(out.effect_mxcsr)::"memory");
 } // Restore original whole x87/XMM/control/status bank before result copy.
 static_assert(sizeof(words)==sizeof(out.vertices));
 std::memcpy(out.vertices.data(),words.data(),sizeof(words));
 return out;
#else
 (void)input;
 throw std::runtime_error("Native Label numeric DATA needs GNU-mode x86 SSE/FXSAVE assembly; this compiler/host is unsupported");
#endif
}
} // namespace civ5::graphics::detail
#undef CIV5_PRIVATE_LABEL_NUMERIC_GNU_X86
