#pragma once
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <span>
#include <stdexcept>

namespace civ5::graphics {
// Narrow source6617C0 postprocess, AFTER native Granny format4 conversion.
// Format4 uses half4 position at0, U8N4 weights at8, U8x4 indices at12,
// half4 normal at16, half2 UV at24: stride28. This helper does not pack halfs,
// remap palette indices, create a pose, or certify a complete draw buffer.
struct BnwWarriorNativeWeightReduction {
    std::array<std::uint8_t,4> weights;
    bool divisor_nonzero;
};
inline BnwWarriorNativeWeightReduction reduce_bnw_warrior_native_weights(
    std::array<std::uint8_t,4> weights) {
    // Raw native SSE order (second+third)+first. Exact for byte operands.
    const float sum=(float(weights[1])+float(weights[2]))+float(weights[0]);
    if(sum==0)return {weights,false}; // Native leaves all four bytes unchanged.
    // Preserve DIVSS/MULSS boundaries; imported floor receives the x87
    // addition of .5, not an extra rounded Float32 addition.
    const volatile float scale=255.f/sum;
    const volatile float first=scale*float(weights[0]);
    const volatile float second=scale*float(weights[1]);
    const auto a=static_cast<int>(std::floor(double(first)+.5));
    const auto b=static_cast<int>(std::floor(double(second)+.5));
    // Native third =255-(rounded first+second). MOVB preserves its low byte,
    // including the source (1,1,0,253)->(128,128,255,0) boundary. No clamp.
    weights={static_cast<std::uint8_t>(a),static_cast<std::uint8_t>(b),
             static_cast<std::uint8_t>(255-(a+b)),0};
    return {weights,true};
}
struct BnwWarriorNativeWeightStats {
    std::size_t vertices=0, changed=0, zero_divisors=0;
};
inline BnwWarriorNativeWeightStats postprocess_bnw_warrior_native_format4_weights(
    std::span<std::uint8_t> converted_format4, std::size_t vertices,
    std::uint32_t verified_format) {
    // Callers must explicitly provide the source-proven converted layout.
    // A float32 SDK stride68 stream is never silently treated as format4.
    if(verified_format!=4)throw std::invalid_argument("Native Warrior format4 required");
    if(vertices>std::numeric_limits<std::size_t>::max()/28 || converted_format4.size()!=vertices*28)
        throw std::invalid_argument("Native Warrior format4 cardinality/stride");
    BnwWarriorNativeWeightStats stats{vertices,0,0};
    for(std::size_t i=0;i<vertices;++i){
        const auto at=i*28+8;
        const std::array<std::uint8_t,4> before{converted_format4[at],converted_format4[at+1],converted_format4[at+2],converted_format4[at+3]};
        const auto r=reduce_bnw_warrior_native_weights(before);
        stats.changed+=r.weights!=before;stats.zero_divisors+=!r.divisor_nonzero;
        for(unsigned j=0;j<4;++j)converted_format4[at+j]=r.weights[j];
    }
    return stats;
}
} // namespace civ5::graphics
