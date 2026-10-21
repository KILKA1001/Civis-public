// Private DATA helper; include at GLOBAL namespace, never inside an existing namespace.
// Actual C4 authority/lifetime belongs to the sealed caller, not this conversion.
#include <array>
#include <cstdint>
#include <string>
#include <string_view>
#include <stdexcept>
namespace civ5::ui::detail {
struct BnwLabelNativeUtf8Conversion final {
 std::u16string units;
 std::size_t actual_byte_length=0,byte_prefix_length=0,consumed_byte_count=0;
 std::size_t output_capacity_units=0,output_unit_count=0;
 // Exact 83B780 return: 0 complete, 1 input partial, 2 target exhausted.
 // 410900 ignores this status and consumes the produced unit count.
 std::uint32_t native_status=0;
};
// Table bytes at C9F4A8, C9F490 in the frozen original PE, not strict UTF8.
static constexpr std::array<std::uint8_t,256> bnw_label_native_extra_bytes{{
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
  3,3,3,3,3,3,3,3,4,4,4,4,5,5,5,5,
}};
static constexpr std::array<std::uint32_t,6> bnw_label_native_offsets{{0x00000000u,0x00003080u,0x000e2080u,0x03c82080u,0xfa082080u,0x82082080u}};
static BnwLabelNativeUtf8Conversion convert_label_native_utf8_data(std::string_view actual_c4_storage){
 static_assert(sizeof(char16_t)==2,"Native Label conversion needs 16-bit UTF16 units");
 // View includes the real terminating NUL and all genuine bytes before it.
 // The caller retains immutable physical storage until conversion returns.
 // Never give this helper a clamped view with a manufactured NUL.
 const auto length=actual_c4_storage.find('\0');
 if(length==std::string_view::npos)
  throw std::runtime_error("Native Label actual C4 terminating NUL unavailable");
 if(length>0x7fffffffu)
  throw std::runtime_error("Native Label signed strlen domain unavailable");
 const std::size_t prefix=length<4096?length:4096;
 // 41093E sets output_end to buffer+prefix*2; 4096 is the maximum capacity.
 std::array<char16_t,4096> converted{};
 std::size_t position=0,produced=0;std::uint32_t status=0;
 while(position<prefix){
  const auto lead=static_cast<unsigned char>(actual_c4_storage[position]);
  const auto additional=bnw_label_native_extra_bytes[lead];
  // Native 83B7C7 is JA, not JAE: the real byte at prefix end is admissible.
  if(additional>prefix-position){status=1;break;}
  if(additional>length-position)
   throw std::runtime_error("Native Label converter read beyond actual C4 terminator");
  std::uint32_t codepoint=0;
  for(std::size_t byte=0;byte<=additional;++byte)
   codepoint=(codepoint<<6)+static_cast<unsigned char>(actual_c4_storage[position+byte]);
  codepoint-=bnw_label_native_offsets[additional]; // Native DWORD wraparound.
  position+=static_cast<std::size_t>(additional)+1; // Advances before target test.
  if(produced>=prefix){status=2;break;}
  if(codepoint<=0xffffu)converted[produced++]=static_cast<char16_t>(codepoint);
  else if(codepoint>0x10ffffu)converted[produced++]=static_cast<char16_t>(0xfffd);
  else{
   if(produced+1>=prefix){status=2;break;}
   const auto value=codepoint-0x10000u;
   converted[produced++]=static_cast<char16_t>((value>>10)-0x2800u);
   converted[produced++]=static_cast<char16_t>((value&0x3ffu)-0x2400u);
  }
 }
 // All raw C4 reads are finished before the sole allocating output copy.
 // A callback may retire the caller's run here; caller must perform fresh POST.
 BnwLabelNativeUtf8Conversion result;
 result.actual_byte_length=length;result.byte_prefix_length=prefix;
 result.consumed_byte_count=position;result.output_capacity_units=prefix;
 result.output_unit_count=produced;result.native_status=status;
 result.units.assign(converted.data(),produced);
 return result;
}
} // namespace civ5::ui::detail
