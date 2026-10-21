#pragma once
#include "civ5/graphics/bnw_warrior_native_vertex_weights.hpp"
#include <bit>
#include <cstring>
#include <string_view>
#include <vector>
namespace civ5::graphics {
// Exact98B0C0 integer conversion. Native ties add0x1000; this is deliberately
// not a generic IEEE ties-to-even cast. NaN payload and signed zero survive.
inline std::uint16_t bnw_warrior_native_real16_bits(std::uint32_t source) {
    const std::uint32_t sign=(source>>16)&0x8000,exponent=(source>>23)&255;
    std::uint32_t mantissa=source&0x7fffff;
    int target=int(exponent)-112;
    if(target>30){
        if(exponent==255&&mantissa!=0){mantissa>>=13;if(mantissa==0)mantissa=1;return std::uint16_t(sign|0x7c00|mantissa);}
        return std::uint16_t(sign|0x7c00);
    }
    if(target < -10)return std::uint16_t(sign);
    if(target<=0)return std::uint16_t(sign|(((mantissa|0x800000)>>(1-target))+0x1000)>>13);
    mantissa+=0x1000;
    if(mantissa&0x800000){mantissa=0;++target;}
    return std::uint16_t(sign|(std::uint32_t(target)<<10)|(mantissa>>13));
}
struct BnwWarriorSdkVertexChannel {
    std::string_view name;
    std::uint32_t granny_type,array_count,offset;
    bool operator==(const BnwWarriorSdkVertexChannel&)const=default;
};
inline constexpr std::array<BnwWarriorSdkVertexChannel,7> bnw_warrior_sdk_vertex_declaration{{
    {"Position",10,3,0},{"BoneWeights",14,4,12},{"BoneIndices",12,4,16},
    {"Normal",10,3,20},{"Tangent",10,3,32},{"Binormal",10,3,44},
    {"TextureCoordinates0",10,3,56}}};
struct BnwWarriorNativeFormat4Vertices {
    std::vector<std::uint8_t> bytes;
    BnwWarriorNativeWeightStats weights;
};
// Only the accepted named GSD SDK layout is supported. Identity/provenance of
// its original GR2/GSD bytes remains the existing cache loader's obligation.
inline BnwWarriorNativeFormat4Vertices convert_bnw_warrior_native_format4_vertices(
    std::span<const std::uint8_t> source,std::size_t vertices,std::size_t source_stride,
    std::span<const BnwWarriorSdkVertexChannel> declaration) {
    static_assert(std::endian::native==std::endian::little,"Pinned Windows x64 stream endian");
    if(source_stride!=68 || declaration.size()!=7 || !std::equal(declaration.begin(),declaration.end(),bnw_warrior_sdk_vertex_declaration.begin()))
        throw std::invalid_argument("Unsupported/anonymous Warrior native source declaration");
    if(vertices>std::numeric_limits<std::size_t>::max()/68 || source.size()!=vertices*68)
        throw std::invalid_argument("Warrior native source stream cardinality");
    BnwWarriorNativeFormat4Vertices result;result.bytes.resize(vertices*28,0);
    const auto scalar=[&](std::size_t from,std::size_t to){
        std::uint32_t bits;std::memcpy(&bits,source.data()+from,4);
        const auto half=bnw_warrior_native_real16_bits(bits);std::memcpy(result.bytes.data()+to,&half,2);
    };
    for(std::size_t i=0;i<vertices;++i){
        const auto s=i*68,d=i*28;
        // Native99DF10 zeros destination arrays. Missing Position/Normal w=0,
        // not1. Native992D10 converts min(sourceCount,targetCount) scalars.
        for(unsigned j=0;j<3;++j){scalar(s+j*4,d+j*2);scalar(s+20+j*4,d+16+j*2);}
        for(unsigned j=0;j<2;++j)scalar(s+56+j*4,d+24+j*2);
        std::memcpy(result.bytes.data()+d+8,source.data()+s+12,8);
    }
    result.weights=postprocess_bnw_warrior_native_format4_weights(result.bytes,vertices,4);
    return result;
}
} // namespace civ5::graphics
