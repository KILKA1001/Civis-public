// Private numerical bodies qualified by resource-index-source2 physical PE.
// Include inside namespace civ5::ui after the current GUI core definitions.
// These routines produce only Original callback/record scalar OUTPUTS.
// They do not issue a Bank, ID, completed ctor, DDS keeper, GPU entry or Source.
namespace detail {
[[nodiscard]] static inline std::uint32_t
original_nonfont_resource_format_bits_8e8280(std::uint32_t format)noexcept{
 // Actual unsigned CMP/JZ and format2..116 selector/jump-table records.
 if(format==0x201u)return 32u;
 if(format>0x201u){
  if(format==0x31545844u)return 4u;
  if(format==0x33545844u||format==0x35545844u)return 8u;
  return 0u;
 }
 const auto index=format-2u;
 if(index>0x72u)return 0u;
 static constexpr std::array<std::uint8_t,115> selector={0,1,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,2,2,2,3,3,3,3,5,4,5,3,2,5,5,2,5,0,5,5,5,5,5,5,5,5,5,5,5,5,5,4,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,2,5,5,5,5,5,5,5,5,3,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,3,2,0,2,0,1};
 static constexpr std::array<std::uint32_t,6> results={64,128,32,16,8,0};
 return results[selector[index]];
}

[[nodiscard]] static inline std::array<std::uint32_t,4>
original_nonfont_resource0_fill_8e2270_output_words(
 const std::array<std::uint32_t,2>& actual_texcoord_words)noexcept{
 // Actual MULSS/CVTTSS2SI execute on the caller's real current MXCSR.
 // The genuine device-init caller must admit its numeric domain/profile and
 // the actual callback coordinates; this helper does not invent a pixel-center
 // convention, quantization, sampler, surface or completed FillTexture grant.
 const auto scale=_mm_set_ss(std::bit_cast<float>(std::uint32_t{0x437f0000u}));
 // Preserve the Original Y-before-X statement order with observable stores.
 volatile std::int32_t actual_y=_mm_cvttss_si32(_mm_mul_ss(
  _mm_set_ss(std::bit_cast<float>(actual_texcoord_words[1])),scale));
 volatile std::int32_t actual_x=_mm_cvttss_si32(_mm_mul_ss(
  _mm_set_ss(std::bit_cast<float>(actual_texcoord_words[0])),scale));
 const auto alternating=(static_cast<std::uint32_t>(actual_y)^static_cast<std::uint32_t>(actual_x))&0x10u;
 if(!alternating)return {0x3f800000u,0x3f800000u,0x3f800000u,0x3f000000u};
 return {0x3f800000u,0u,0u,0x3f800000u};
}
}
