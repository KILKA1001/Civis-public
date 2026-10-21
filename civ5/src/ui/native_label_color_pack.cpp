#include "civ5/ui/native_label_color_pack.hpp"
#include <bit>
#include <limits>
#include <stdexcept>

namespace civ5::ui {
static_assert(sizeof(float)==4 && std::numeric_limits<float>::is_iec559);
namespace {
struct ByteResult {std::uint8_t value;bool invalid;};
ByteResult convert(std::uint32_t bits,std::uint16_t cw) {
 const bool negative=(bits>>31)!=0;
 const unsigned exponent=(bits>>23)&255u;
 if(exponent==255)throw std::invalid_argument("Label color must be finite float32");
 const std::uint32_t mantissa=(bits&0x7fffffu)|(exponent?0x800000u:0u);
 std::uint64_t product=std::uint64_t(mantissa)*255u;
 int power=exponent?int(exponent)-150:-149;
 // FMUL precedes FLDCW(CW|0xc00). 24-bit precision can change an integer
 // boundary before FISTP; 53/64 bits represent this <=32-bit product exactly.
 if(((cw>>8)&3u)==0 && product) {
  const unsigned width=64u-unsigned(std::countl_zero(product));
  if(width>24) {
   const unsigned drop=width-24;
   const std::uint64_t remainder=product&((std::uint64_t(1)<<drop)-1);
   std::uint64_t retained=product>>drop;
   const unsigned round=(cw>>10)&3u;
   const std::uint64_t half=std::uint64_t(1)<<(drop-1);
   bool increment=false;
   if(round==0)increment=remainder>half||(remainder==half&&(retained&1));
   if(round==1)increment=negative&&remainder;
   if(round==2)increment=!negative&&remainder;
   product=(retained+unsigned(increment))<<drop;
  }
 }
 const std::uint64_t limit=negative?(std::uint64_t(1)<<63):std::uint64_t(INT64_MAX);
 std::uint64_t integer=0;
 if(power>=0) {
  if(power>=64 || product>(limit>>unsigned(power)))return {0,true};
  integer=product<<unsigned(power);
 } else {
  const unsigned shift=unsigned(-power);
  integer=shift>=64?0:product>>shift;
  if(integer>limit)return {0,true};
 }
 if(negative)integer=std::uint64_t(0)-integer;
 return {std::uint8_t(integer&255u),false};
}
}
NativeLabelColorPackReceipt pack_native_label_color(const NativeLabelColorPackRequest&r) {
 const auto cw=r.incoming_x87_control_word;
 if((cw&0x3fu)!=0x3fu || ((cw>>8)&3u)==1)
  throw std::invalid_argument("Explicit masked, nonreserved x87 profile required");
 NativeLabelColorPackReceipt out;
 out.incoming_x87_control_word=cw;
 for(unsigned n=0;n<4;++n) {
  const auto bits=std::bit_cast<std::uint32_t>(r.rgba[n]);
  out.input_float32_bits[n]=bits;
  const auto byte=convert(bits,cw);
  out.rgba_bytes[n]=byte.value;
  out.masked_invalid_conversion_channels|=std::uint8_t(unsigned(byte.invalid)<<n);
  out.packed_rgba|=std::uint32_t(byte.value)<<(8*n);
 }
 return out;
}
} // namespace civ5::ui
