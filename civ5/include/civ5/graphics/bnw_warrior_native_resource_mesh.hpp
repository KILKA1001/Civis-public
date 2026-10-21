#pragma once
#include "civ5/graphics/bnw_warrior_native_resource_basis.hpp"
#include <cstring>
#include <vector>
namespace civ5::graphics {
namespace warrior_resource_detail {
// Primary source9B1930 branch: normalize=false, morph=false. Normal and
// TangentBinormalCross use inverse transpose; Tangent/Binormal use linear.
// This is the intermediate source named FLOAT3 stream, BEFORE optimized packing.
inline std::array<float,15> warrior_resource_mesh_vectors(std::array<float,15> canonical,const BnwWarriorNativeResourceBasis& basis) {
    const auto& linear=basis.linear; const auto& inverse=basis.inverse; const auto& translation=basis.translation;
    std::array<float,8> x{}; std::array<float,64> scratch{};
    std::copy(canonical.begin(),canonical.end(),scratch.begin()+2);
    x[6] = scratch[2]; // 0x9b19aa
    x[5] = scratch[3]; // 0x9b19b0
    x[2] = linear[2]; // 0x9b19b6
    x[3] = linear[1]; // 0x9b19bb
    x[1] = linear[0]; // 0x9b19c0
    x[7] = linear[4]; // 0x9b19c4
    scratch[23] = x[2]; // 0x9b19cc
    x[2] = x[2] * scratch[4]; // 0x9b19d2
    scratch[36] = x[5]; // 0x9b19d8
    x[0] = x[1]; // 0x9b19e1
    x[0] = x[0] * x[6]; // 0x9b19e4
    x[4] = x[3]; // 0x9b19e8
    x[4] = x[4] * x[5]; // 0x9b19eb
    x[0] = x[0] + x[4]; // 0x9b19ef
    x[4] = linear[5]; // 0x9b19f3
    x[0] = x[0] + x[2]; // 0x9b19f8
    x[2] = linear[3]; // 0x9b19fc
    scratch[21] = x[7]; // 0x9b1a01
    x[7] = x[7] * x[5]; // 0x9b1a07
    scratch[28] = x[2]; // 0x9b1a0b
    x[2] = x[2] * x[6]; // 0x9b1a14
    x[2] = x[2] + x[7]; // 0x9b1a18
    x[7] = linear[7]; // 0x9b1a1c
    x[5] = x[4]; // 0x9b1a21
    x[5] = x[5] * scratch[4]; // 0x9b1a24
    x[2] = x[2] + x[5]; // 0x9b1a2a
    x[5] = linear[8]; // 0x9b1a2e
    scratch[18] = x[5]; // 0x9b1a33
    x[5] = linear[6]; // 0x9b1a39
    scratch[19] = x[5]; // 0x9b1a3e
    x[5] = x[5] * x[6]; // 0x9b1a44
    x[6] = scratch[18]; // 0x9b1a48
    x[6] = x[6] * scratch[4]; // 0x9b1a4e
    scratch[20] = x[7]; // 0x9b1a54
    x[7] = x[7] * scratch[36]; // 0x9b1a5a
    x[5] = x[5] + x[7]; // 0x9b1a66
    x[5] = x[5] + x[6]; // 0x9b1a6a
    scratch[2] = x[0]; // 0x9b1a6e
    scratch[3] = x[2]; // 0x9b1a74
    scratch[4] = x[5]; // 0x9b1a7a
    x[6] = translation[0]; // 0x9b1a87
    x[6] = x[6] + x[0]; // 0x9b1a8b
    x[0] = translation[1]; // 0x9b1a8f
    x[0] = x[0] + x[2]; // 0x9b1a94
    scratch[3] = x[0]; // 0x9b1a98
    x[0] = translation[2]; // 0x9b1a9e
    x[0] = x[0] + x[5]; // 0x9b1aa3
    scratch[2] = x[6]; // 0x9b1aa7
    scratch[4] = x[0]; // 0x9b1aad
    x[7] = scratch[8]; // 0x9b1ab3
    x[6] = scratch[9]; // 0x9b1ab9
    x[2] = linear[1]; // 0x9b1abf
    x[5] = linear[2]; // 0x9b1ac4
    scratch[49] = x[2]; // 0x9b1ac9
    x[2] = x[2] * x[6]; // 0x9b1ad2
    x[0] = x[1]; // 0x9b1ad6
    x[0] = x[0] * x[7]; // 0x9b1ad9
    x[0] = x[0] + x[2]; // 0x9b1add
    x[2] = x[5]; // 0x9b1ae1
    x[2] = x[2] * scratch[10]; // 0x9b1ae4
    x[0] = x[0] + x[2]; // 0x9b1aea
    scratch[8] = x[0]; // 0x9b1aee
    x[0] = linear[5]; // 0x9b1af4
    scratch[26] = x[0]; // 0x9b1af9
    x[0] = linear[4]; // 0x9b1aff
    scratch[29] = x[0]; // 0x9b1b04
    x[0] = linear[3]; // 0x9b1b0a
    scratch[47] = x[6]; // 0x9b1b0f
    scratch[24] = x[7]; // 0x9b1b18
    x[2] = x[0]; // 0x9b1b1e
    x[2] = x[2] * x[7]; // 0x9b1b21
    x[7] = scratch[29]; // 0x9b1b25
    x[7] = x[7] * x[6]; // 0x9b1b2b
    x[6] = scratch[26]; // 0x9b1b2f
    x[6] = x[6] * scratch[10]; // 0x9b1b35
    x[2] = x[2] + x[7]; // 0x9b1b3b
    x[2] = x[2] + x[6]; // 0x9b1b3f
    scratch[9] = x[2]; // 0x9b1b43
    x[2] = linear[8]; // 0x9b1b49
    scratch[30] = x[2]; // 0x9b1b4e
    x[2] = linear[7]; // 0x9b1b54
    scratch[22] = x[2]; // 0x9b1b59
    x[2] = linear[6]; // 0x9b1b5f
    x[6] = x[2]; // 0x9b1b64
    x[6] = x[6] * scratch[24]; // 0x9b1b67
    scratch[17] = x[6]; // 0x9b1b6d
    x[7] = scratch[17]; // 0x9b1b73
    x[6] = scratch[22]; // 0x9b1b79
    x[6] = x[6] * scratch[47]; // 0x9b1b7f
    x[7] = x[7] + x[6]; // 0x9b1b88
    x[6] = scratch[30]; // 0x9b1b8c
    x[6] = x[6] * scratch[10]; // 0x9b1b92
    scratch[17] = x[7]; // 0x9b1b98
    x[7] = x[7] + x[6]; // 0x9b1b9e
    x[6] = scratch[12]; // 0x9b1ba2
    scratch[44] = x[6]; // 0x9b1ba8
    scratch[10] = x[7]; // 0x9b1bb1
    x[7] = scratch[11]; // 0x9b1bb7
    scratch[38] = x[7]; // 0x9b1bbd
    x[6] = x[1]; // 0x9b1bc6
    x[6] = x[6] * x[7]; // 0x9b1bc9
    scratch[17] = x[6]; // 0x9b1bcd
    x[6] = scratch[49]; // 0x9b1bd3
    x[6] = x[6] * scratch[12]; // 0x9b1bdc
    x[7] = scratch[17]; // 0x9b1be2
    x[7] = x[7] + x[6]; // 0x9b1be8
    x[6] = scratch[13]; // 0x9b1bec
    x[5] = x[5] * x[6]; // 0x9b1bf2
    x[7] = x[7] + x[5]; // 0x9b1bf6
    x[5] = scratch[29]; // 0x9b1bfa
    x[5] = x[5] * scratch[12]; // 0x9b1c00
    scratch[11] = x[7]; // 0x9b1c06
    x[7] = scratch[38]; // 0x9b1c0c
    x[0] = x[0] * x[7]; // 0x9b1c15
    x[0] = x[0] + x[5]; // 0x9b1c19
    x[5] = scratch[26]; // 0x9b1c1d
    x[5] = x[5] * x[6]; // 0x9b1c23
    x[0] = x[0] + x[5]; // 0x9b1c27
    scratch[12] = x[0]; // 0x9b1c2b
    x[0] = scratch[22]; // 0x9b1c31
    x[0] = x[0] * scratch[44]; // 0x9b1c37
    x[2] = x[2] * x[7]; // 0x9b1c40
    x[2] = x[2] + x[0]; // 0x9b1c44
    x[0] = scratch[30]; // 0x9b1c48
    x[0] = x[0] * x[6]; // 0x9b1c4e
    x[2] = x[2] + x[0]; // 0x9b1c52
    scratch[13] = x[2]; // 0x9b1c56
    x[1] = scratch[15]; // 0x9b1c67
    x[2] = inverse[3]; // 0x9b1c6d
    x[3] = scratch[14]; // 0x9b1c72
    x[7] = inverse[6]; // 0x9b1c78
    x[4] = inverse[0]; // 0x9b1c7d
    x[0] = x[1]; // 0x9b1c81
    x[6] = x[3]; // 0x9b1c84
    x[3] = x[3] * x[4]; // 0x9b1c87
    x[4] = inverse[4]; // 0x9b1c8b
    scratch[40] = x[1]; // 0x9b1c90
    x[1] = x[1] * x[4]; // 0x9b1c99
    x[0] = x[0] * x[2]; // 0x9b1c9d
    x[2] = scratch[16]; // 0x9b1ca1
    x[5] = x[2]; // 0x9b1ca7
    x[5] = x[5] * x[7]; // 0x9b1caa
    x[0] = x[0] + x[5]; // 0x9b1cae
    x[5] = inverse[1]; // 0x9b1cb2
    x[0] = x[0] + x[3]; // 0x9b1cb7
    x[3] = inverse[7]; // 0x9b1cbb
    x[4] = x[2]; // 0x9b1cc0
    x[4] = x[4] * x[3]; // 0x9b1cc3
    x[3] = inverse[5]; // 0x9b1cc7
    x[1] = x[1] + x[4]; // 0x9b1ccc
    x[4] = inverse[2]; // 0x9b1cd0
    scratch[48] = x[5]; // 0x9b1cd5
    x[5] = x[5] * x[6]; // 0x9b1cde
    x[1] = x[1] + x[5]; // 0x9b1ce2
    x[5] = x[1]; // 0x9b1ce6
    x[1] = inverse[8]; // 0x9b1ce9
    x[2] = x[2] * x[1]; // 0x9b1cee
    scratch[37] = x[3]; // 0x9b1cf2
    x[3] = x[3] * scratch[40]; // 0x9b1cfb
    x[1] = x[4]; // 0x9b1d04
    x[1] = x[1] * x[6]; // 0x9b1d07
    x[2] = x[2] + x[1]; // 0x9b1d0b
    x[1] = scratch[5]; // 0x9b1d0f
    x[2] = x[2] + x[3]; // 0x9b1d15
    x[3] = scratch[6]; // 0x9b1d19
    scratch[46] = x[4]; // 0x9b1d1f
    x[4] = inverse[3]; // 0x9b1d28
    scratch[42] = x[1]; // 0x9b1d2d
    x[1] = x[3]; // 0x9b1d36
    x[1] = x[1] * x[4]; // 0x9b1d39
    x[4] = scratch[7]; // 0x9b1d3d
    x[6] = x[4]; // 0x9b1d43
    x[6] = x[6] * x[7]; // 0x9b1d46
    x[7] = inverse[0]; // 0x9b1d4a
    x[1] = x[1] + x[6]; // 0x9b1d4e
    x[6] = scratch[5]; // 0x9b1d52
    x[6] = x[6] * x[7]; // 0x9b1d58
    x[1] = x[1] + x[6]; // 0x9b1d5c
    x[6] = inverse[4]; // 0x9b1d60
    scratch[39] = x[3]; // 0x9b1d65
    x[3] = x[3] * x[6]; // 0x9b1d6e
    x[6] = inverse[7]; // 0x9b1d72
    x[7] = x[4]; // 0x9b1d77
    x[7] = x[7] * x[6]; // 0x9b1d7a
    x[6] = scratch[48]; // 0x9b1d7e
    x[3] = x[3] + x[7]; // 0x9b1d87
    x[7] = scratch[42]; // 0x9b1d8b
    x[6] = x[6] * x[7]; // 0x9b1d94
    x[3] = x[3] + x[6]; // 0x9b1d98
    x[6] = inverse[8]; // 0x9b1d9c
    x[4] = x[4] * x[6]; // 0x9b1da1
    x[6] = scratch[46]; // 0x9b1da5
    x[6] = x[6] * x[7]; // 0x9b1dae
    x[4] = x[4] + x[6]; // 0x9b1db2
    x[6] = scratch[37]; // 0x9b1db6
    x[6] = x[6] * scratch[39]; // 0x9b1dbf
    scratch[14] = x[0]; // 0x9b1dc8
    scratch[15] = x[5]; // 0x9b1dce
    scratch[16] = x[2]; // 0x9b1dd4
    x[4] = x[4] + x[6]; // 0x9b1eff
    scratch[7] = x[4]; // 0x9b1f03
    scratch[6] = x[3]; // 0x9b1f09
    scratch[5] = x[1]; // 0x9b1f0f
    std::copy_n(scratch.begin()+2,15,canonical.begin());
    return canonical;
}
} // namespace warrior_resource_detail
struct BnwWarriorNativeResourceMesh {
    BnwWarriorNativeResourceBasisKey basis_key;
    std::string raw_stream_identity;
    std::vector<std::uint8_t> vertices_stride68;
};
// The caller owns a verified raw SDK mesh artifact. The returned vector keeps
// source weights, indices and all three authored UV floats unchanged. It does
// not replace the same mesh palette, topology, materials or instance placement.
inline BnwWarriorNativeResourceMesh convert_bnw_warrior_native_resource_mesh(std::span<const std::uint8_t> raw,std::string_view verified_raw_stream_identity,const BnwWarriorNativeResourceBasis& basis) {
    constexpr std::array<std::string_view,3> identities{
      "d78b64da4eaadc285f2c8f8f27fbdb6b6f01e790d0262ab2bfc7e5f6a714c21d",
      "56104434706e2921b8fe142de65b07e2aad67aa64008b95be80d532892eac9c0",
      "e03a7afa5f2ff04e2103381aa7cb4475c92dcb87629b659cb3069f3ba513688d"};
    warrior_curve_require(std::ranges::find(identities,verified_raw_stream_identity)!=identities.end(),"Foreign raw Warrior mesh stream identity");
    warrior_curve_require(!raw.empty()&&raw.size()%68==0,"Source Warrior mesh stride/cardinality");
    constexpr std::array<std::size_t,3> counts{829,863,796};
    const auto model=std::ranges::find(identities,verified_raw_stream_identity)-identities.begin();
    warrior_curve_require(raw.size()==counts[model]*68,"Selected raw Warrior mesh cardinality");
    validate_bnw_warrior_native_resource_basis(basis);
    BnwWarriorNativeResourceMesh out{basis.key,std::string(verified_raw_stream_identity),std::vector<std::uint8_t>(raw.begin(),raw.end())};
    for(std::size_t i=0;i<raw.size();i+=68) {
        std::array<float,15> channels{};std::memcpy(channels.data(),raw.data()+i,12);std::memcpy(channels.data()+3,raw.data()+i+20,36);
        warrior_curve_require(std::ranges::all_of(channels,[](float f){return std::isfinite(f);}),"Nonfinite raw Warrior mesh vector");
        channels=warrior_resource_detail::warrior_resource_mesh_vectors(channels,basis);
        warrior_curve_require(std::ranges::all_of(channels,[](float f){return std::isfinite(f);}),"Source mesh conversion overflow");
        std::memcpy(out.vertices_stride68.data()+i,channels.data(),12);std::memcpy(out.vertices_stride68.data()+i+20,channels.data()+3,36);
    }
    return out;
}
} // namespace civ5::graphics

