#pragma once
#include "civ5/graphics/bnw_warrior_native_bone_matrices.hpp"
#include "civ5/graphics/bnw_warrior_gr2_native_basis.hpp"
namespace civ5::graphics {
struct BnwWarriorNativeResourceBasisKey {
    std::uint32_t source_units_bits, compiled_scale_bits;
    friend bool operator==(const BnwWarriorNativeResourceBasisKey&,const BnwWarriorNativeResourceBasisKey&)=default;
};
struct BnwWarriorNativeResourceBasis {
    BnwWarriorNativeResourceBasisKey key;
    std::array<float,9> linear,inverse;
    std::array<float,3> translation;
};
namespace warrior_resource_detail {
// Exact source990C70/9909C0: float32 arithmetic and rounded double SQRT.
inline std::array<float,4> warrior_resource_matrix_to_quaternion(const std::array<float,9>& m) {
    std::array<float,4> q{};const float trace0=m[0]+m[4],trace=trace0+m[8];
    if(trace>0.f) {
        const float sum=trace+1.f,n=static_cast<float>(std::sqrt(static_cast<double>(sum)));const float factor=.5f/n;
        q[3]=n*.5f;q[0]=(m[7]-m[5])*factor;q[1]=(m[2]-m[6])*factor;q[2]=(m[3]-m[1])*factor;
    } else {
        unsigned i=0;if(m[4]>m[0])i=1;if(m[8]>m[i*4])i=2;const auto j=(i+1)%3,k=(i+2)%3;
        const float pair=m[k*4]+m[j*4],diff=m[i*4]-pair,sum=diff+1.f,n=static_cast<float>(std::sqrt(static_cast<double>(sum)));const float factor=.5f/n;
        q[i]=n*.5f;q[j]=(m[j*3+i]+m[i*3+j])*factor;q[k]=(m[k*3+i]+m[i*3+k])*factor;q[3]=(m[k*3+j]-m[j*3+k])*factor;
    }
    return q;
}
// Pinned source 990D30, 1446 bytes, SHA256 991d4e8679a35ec44713308f5a579a789d3b81810625b6370b89d732441797fc
inline std::array<float,4> warrior_resource_quaternion(std::array<float,4> q,const std::array<float,9>& linear,const std::array<float,9>& inverse) {
    std::array<float,8> x{}; std::array<float,24> scratch{}; double d=0;
    x[3] = q[0]; // 0x990d39
    x[2] = q[1]; // 0x990d3d
    x[1] = q[2]; // 0x990d42
    x[0] = q[3]; // 0x990d47
    x[4] = x[3]; // 0x990d4c
    x[4] = x[4] * x[3]; // 0x990d4f
    x[3] = x[2]; // 0x990d53
    x[3] = x[3] * x[2]; // 0x990d56
    x[2] = x[1]; // 0x990d5a
    x[2] = x[2] * x[1]; // 0x990d5d
    x[1] = x[0]; // 0x990d61
    x[1] = x[1] * x[0]; // 0x990d64
    x[4] = x[4] + x[3]; // 0x990d68
    x[4] = x[4] + x[2]; // 0x990d6c
    x[2] = q[1]; // 0x990d70
    x[4] = x[4] + x[1]; // 0x990d75
    x[1] = q[0]; // 0x990d79
    d = static_cast<double>(x[4]); // 0x990d7d
    d = std::sqrt(d); // 0x990d81
    x[4] = static_cast<float>(d); // 0x990d85
    x[0] = std::bit_cast<float>(0x3f800000u); // 0x990d89
    x[0] = x[0] / x[4]; // 0x990d91
    x[1] = x[1] * x[0]; // 0x990d95
    x[2] = x[2] * x[0]; // 0x990d99
    x[3] = x[0]; // 0x990d9d
    x[0] = x[0] * q[3]; // 0x990da0
    x[3] = x[3] * q[2]; // 0x990da5
    scratch[2] = x[0]; // 0x990daa
    q[3] = x[0]; // 0x990db0
    x[0] = x[1]; // 0x990db5
    x[0] = x[0] * x[1]; // 0x990db8
    scratch[4] = x[0]; // 0x990dbc
    x[0] = x[2]; // 0x990dc2
    x[0] = x[0] * x[1]; // 0x990dc5
    scratch[1] = x[0]; // 0x990dc9
    x[0] = x[3]; // 0x990dcf
    x[0] = x[0] * x[2]; // 0x990dd2
    scratch[3] = x[0]; // 0x990dd6
    x[0] = scratch[2]; // 0x990ddc
    x[0] = x[0] * x[1]; // 0x990de2
    q[0] = x[1]; // 0x990de6
    x[7] = x[3]; // 0x990dea
    x[7] = x[7] * x[1]; // 0x990ded
    x[5] = x[2]; // 0x990df1
    x[6] = x[3]; // 0x990df4
    x[6] = x[6] * x[3]; // 0x990df7
    x[5] = x[5] * x[2]; // 0x990dfb
    q[1] = x[2]; // 0x990dff
    q[2] = x[3]; // 0x990e04
    scratch[5] = x[0]; // 0x990e09
    x[0] = scratch[2]; // 0x990e0f
    x[1] = x[0]; // 0x990e15
    x[1] = x[1] * x[2]; // 0x990e18
    x[0] = x[0] * x[3]; // 0x990e1c
    scratch[2] = x[1]; // 0x990e20
    x[1] = std::bit_cast<float>(0x3f800000u); // 0x990e26
    x[2] = x[0]; // 0x990e2e
    x[0] = std::bit_cast<float>(0x40000000u); // 0x990e31
    x[3] = x[6]; // 0x990e39
    x[6] = x[6] + scratch[4]; // 0x990e3c
    x[3] = x[3] + x[5]; // 0x990e42
    x[3] = x[3] * x[0]; // 0x990e46
    x[1] = x[1] - x[3]; // 0x990e4a
    x[3] = scratch[1]; // 0x990e4e
    scratch[6] = x[1]; // 0x990e54
    x[1] = scratch[2]; // 0x990e5a
    x[3] = x[3] - x[2]; // 0x990e60
    x[2] = x[2] + scratch[1]; // 0x990e64
    x[1] = x[1] + x[7]; // 0x990e6a
    x[2] = x[2] * x[0]; // 0x990e6e
    x[1] = x[1] * x[0]; // 0x990e72
    scratch[9] = x[2]; // 0x990e76
    scratch[8] = x[1]; // 0x990e7c
    x[1] = std::bit_cast<float>(0x3f800000u); // 0x990e82
    x[6] = x[6] * x[0]; // 0x990e8a
    x[2] = x[1]; // 0x990e8e
    x[2] = x[2] - x[6]; // 0x990e91
    x[3] = x[3] * x[0]; // 0x990e95
    scratch[10] = x[2]; // 0x990e99
    x[2] = scratch[3]; // 0x990e9f
    scratch[7] = x[3]; // 0x990ea5
    x[5] = x[5] + scratch[4]; // 0x990eab
    x[7] = x[7] - scratch[2]; // 0x990eb1
    x[6] = scratch[5]; // 0x990eb7
    x[7] = x[7] * x[0]; // 0x990ebd
    x[5] = x[5] * x[0]; // 0x990ec1
    x[2] = x[2] - x[6]; // 0x990ec5
    x[6] = x[6] + scratch[3]; // 0x990ec9
    x[6] = x[6] * x[0]; // 0x990ecf
    x[2] = x[2] * x[0]; // 0x990ed3
    x[0] = linear[2]; // 0x990ed7
    x[1] = x[1] - x[5]; // 0x990edc
    x[5] = linear[0]; // 0x990ee0
    scratch[4] = x[5]; // 0x990ee4
    scratch[11] = x[2]; // 0x990eea
    x[2] = scratch[6]; // 0x990ef0
    x[2] = x[2] * x[5]; // 0x990ef6
    x[5] = x[0]; // 0x990efa
    x[5] = x[5] * x[7]; // 0x990efd
    scratch[13] = x[6]; // 0x990f01
    x[6] = linear[1]; // 0x990f07
    scratch[12] = x[7]; // 0x990f0c
    x[7] = scratch[4]; // 0x990f12
    x[7] = x[7] * x[3]; // 0x990f18
    x[2] = x[2] + x[5]; // 0x990f1c
    x[5] = scratch[9]; // 0x990f20
    x[5] = x[5] * x[6]; // 0x990f26
    x[2] = x[2] + x[5]; // 0x990f2a
    x[5] = x[0]; // 0x990f2e
    x[5] = x[5] * scratch[13]; // 0x990f31
    x[0] = x[0] * x[1]; // 0x990f37
    x[5] = x[5] + x[7]; // 0x990f3b
    x[7] = scratch[10]; // 0x990f3f
    x[7] = x[7] * x[6]; // 0x990f45
    x[6] = scratch[4]; // 0x990f49
    x[6] = x[6] * scratch[8]; // 0x990f4f
    x[0] = x[0] + x[6]; // 0x990f55
    x[6] = linear[1]; // 0x990f59
    x[5] = x[5] + x[7]; // 0x990f5e
    x[7] = scratch[11]; // 0x990f62
    x[7] = x[7] * x[6]; // 0x990f68
    x[6] = linear[4]; // 0x990f6c
    x[0] = x[0] + x[7]; // 0x990f71
    scratch[17] = x[0]; // 0x990f75
    x[0] = linear[5]; // 0x990f7b
    x[0] = x[0] * scratch[12]; // 0x990f80
    scratch[2] = x[6]; // 0x990f86
    x[6] = linear[3]; // 0x990f8c
    scratch[5] = x[6]; // 0x990f91
    x[6] = x[6] * scratch[6]; // 0x990f97
    x[6] = x[6] + x[0]; // 0x990f9d
    x[0] = scratch[2]; // 0x990fa1
    x[0] = x[0] * scratch[9]; // 0x990fa7
    x[6] = x[6] + x[0]; // 0x990fad
    x[0] = scratch[2]; // 0x990fb1
    x[0] = x[0] * scratch[10]; // 0x990fb7
    scratch[1] = x[0]; // 0x990fbd
    x[7] = scratch[1]; // 0x990fc3
    x[0] = linear[5]; // 0x990fc9
    x[0] = x[0] * scratch[13]; // 0x990fce
    x[7] = x[7] + x[0]; // 0x990fd4
    scratch[1] = x[7]; // 0x990fd8
    x[7] = scratch[5]; // 0x990fde
    x[0] = x[7]; // 0x990fe4
    x[7] = x[7] * scratch[8]; // 0x990fe7
    x[0] = x[0] * x[3]; // 0x990fed
    x[3] = scratch[1]; // 0x990ff1
    x[3] = x[3] + x[0]; // 0x990ff7
    scratch[19] = x[3]; // 0x990ffb
    x[3] = linear[5]; // 0x991001
    x[0] = x[1]; // 0x991006
    x[0] = x[0] * x[3]; // 0x991009
    x[3] = scratch[2]; // 0x99100d
    x[3] = x[3] * scratch[11]; // 0x991013
    x[0] = x[0] + x[3]; // 0x991019
    x[3] = linear[8]; // 0x99101d
    x[0] = x[0] + x[7]; // 0x991022
    scratch[20] = x[0]; // 0x991026
    x[0] = linear[7]; // 0x99102c
    scratch[1] = x[0]; // 0x991031
    x[0] = linear[6]; // 0x991037
    scratch[4] = x[0]; // 0x99103c
    x[3] = x[3] * scratch[12]; // 0x991042
    x[0] = x[0] * scratch[6]; // 0x991048
    x[0] = x[0] + x[3]; // 0x99104e
    x[3] = scratch[1]; // 0x991052
    x[3] = x[3] * scratch[9]; // 0x991058
    x[0] = x[0] + x[3]; // 0x99105e
    x[3] = scratch[1]; // 0x991062
    x[3] = x[3] * scratch[10]; // 0x991068
    scratch[2] = x[3]; // 0x99106e
    x[3] = linear[8]; // 0x991074
    x[3] = x[3] * scratch[13]; // 0x991079
    x[7] = scratch[2]; // 0x99107f
    x[7] = x[7] + x[3]; // 0x991085
    x[3] = scratch[4]; // 0x991089
    x[3] = x[3] * scratch[7]; // 0x99108f
    scratch[2] = x[7]; // 0x991095
    x[7] = x[7] + x[3]; // 0x99109b
    x[3] = linear[8]; // 0x99109f
    x[1] = x[1] * x[3]; // 0x9910a4
    x[3] = scratch[1]; // 0x9910a8
    x[3] = x[3] * scratch[11]; // 0x9910ae
    x[1] = x[1] + x[3]; // 0x9910b4
    x[3] = scratch[4]; // 0x9910b8
    x[3] = x[3] * scratch[8]; // 0x9910be
    x[1] = x[1] + x[3]; // 0x9910c4
    x[3] = inverse[0]; // 0x9910c8
    scratch[22] = x[7]; // 0x9910cc
    x[7] = x[2]; // 0x9910d2
    x[7] = x[7] * x[3]; // 0x9910d5
    scratch[23] = x[1]; // 0x9910d9
    x[1] = inverse[3]; // 0x9910df
    x[3] = x[5]; // 0x9910e4
    x[3] = x[3] * x[1]; // 0x9910e7
    x[1] = inverse[6]; // 0x9910eb
    x[7] = x[7] + x[3]; // 0x9910f0
    x[3] = scratch[17]; // 0x9910f4
    x[3] = x[3] * x[1]; // 0x9910fa
    x[1] = inverse[4]; // 0x9910fe
    x[7] = x[7] + x[3]; // 0x991103
    x[3] = inverse[1]; // 0x991107
    scratch[6] = x[7]; // 0x99110c
    x[7] = x[2]; // 0x991112
    x[7] = x[7] * x[3]; // 0x991115
    x[3] = x[5]; // 0x991119
    x[3] = x[3] * x[1]; // 0x99111c
    x[1] = inverse[7]; // 0x991120
    x[7] = x[7] + x[3]; // 0x991125
    x[3] = scratch[17]; // 0x991129
    x[3] = x[3] * x[1]; // 0x99112f
    x[1] = inverse[5]; // 0x991133
    x[7] = x[7] + x[3]; // 0x991138
    x[3] = inverse[8]; // 0x99113c
    scratch[7] = x[7]; // 0x991141
    scratch[4] = x[1]; // 0x991147
    x[1] = inverse[2]; // 0x99114d
    x[7] = x[1]; // 0x991152
    x[7] = x[7] * x[2]; // 0x991155
    x[2] = scratch[4]; // 0x991159
    x[2] = x[2] * x[5]; // 0x99115f
    x[7] = x[7] + x[2]; // 0x991163
    x[2] = x[3]; // 0x991167
    x[2] = x[2] * scratch[17]; // 0x99116a
    x[7] = x[7] + x[2]; // 0x991170
    x[2] = inverse[0]; // 0x991174
    scratch[8] = x[7]; // 0x991178
    x[7] = scratch[19]; // 0x99117e
    x[5] = x[6]; // 0x991184
    x[5] = x[5] * x[2]; // 0x991187
    x[2] = inverse[3]; // 0x99118b
    x[7] = x[7] * x[2]; // 0x991190
    x[2] = inverse[6]; // 0x991194
    x[5] = x[5] + x[7]; // 0x991199
    x[7] = scratch[20]; // 0x99119d
    x[7] = x[7] * x[2]; // 0x9911a3
    x[2] = inverse[1]; // 0x9911a7
    x[5] = x[5] + x[7]; // 0x9911ac
    x[7] = scratch[19]; // 0x9911b0
    scratch[9] = x[5]; // 0x9911b6
    x[5] = x[6]; // 0x9911bc
    x[5] = x[5] * x[2]; // 0x9911bf
    x[2] = inverse[4]; // 0x9911c3
    x[3] = x[3] * scratch[20]; // 0x9911c8
    x[7] = x[7] * x[2]; // 0x9911ce
    x[2] = inverse[7]; // 0x9911d2
    x[5] = x[5] + x[7]; // 0x9911d7
    x[7] = scratch[20]; // 0x9911db
    x[1] = x[1] * x[6]; // 0x9911e1
    x[7] = x[7] * x[2]; // 0x9911e5
    x[2] = scratch[4]; // 0x9911e9
    x[2] = x[2] * scratch[19]; // 0x9911ef
    x[1] = x[1] + x[2]; // 0x9911f5
    x[2] = scratch[22]; // 0x9911f9
    x[1] = x[1] + x[3]; // 0x9911ff
    x[5] = x[5] + x[7]; // 0x991203
    scratch[10] = x[5]; // 0x991207
    scratch[11] = x[1]; // 0x99120d
    x[1] = inverse[0]; // 0x991213
    x[5] = x[0]; // 0x991217
    x[5] = x[5] * x[1]; // 0x99121a
    x[1] = inverse[3]; // 0x99121e
    x[3] = x[2]; // 0x991223
    x[3] = x[3] * x[1]; // 0x991226
    x[1] = scratch[23]; // 0x99122a
    x[5] = x[5] + x[3]; // 0x991230
    x[3] = inverse[6]; // 0x991234
    x[6] = x[1]; // 0x991239
    x[6] = x[6] * x[3]; // 0x99123c
    x[3] = inverse[1]; // 0x991240
    x[5] = x[5] + x[6]; // 0x991245
    scratch[12] = x[5]; // 0x991249
    x[5] = inverse[7]; // 0x99124f
    x[5] = x[5] * x[1]; // 0x991254
    x[1] = x[1] * inverse[8]; // 0x991258
    x[6] = x[0]; // 0x99125d
    x[0] = x[0] * inverse[2]; // 0x991260
    x[6] = x[6] * x[3]; // 0x991265
    x[1] = x[1] + x[0]; // 0x991269
    x[0] = inverse[5]; // 0x99126d
    x[3] = x[2]; // 0x991272
    x[3] = x[3] * inverse[4]; // 0x991275
    x[0] = x[0] * x[2]; // 0x99127a
    x[5] = x[5] + x[6]; // 0x99127e
    x[5] = x[5] + x[3]; // 0x991282
    x[1] = x[1] + x[0]; // 0x991286
    scratch[13] = x[5]; // 0x991290
    scratch[14] = x[1]; // 0x991296
    q = warrior_resource_matrix_to_quaternion({scratch[6],scratch[7],scratch[8],scratch[9],scratch[10],scratch[11],scratch[12],scratch[13],scratch[14]}); // 0x99129c
    x[0] = q[0]; // 0x9912a1
    x[0] = x[0] * x[4]; // 0x9912a5
    q[0] = x[0]; // 0x9912a9
    x[0] = q[1]; // 0x9912ad
    x[0] = x[0] * x[4]; // 0x9912b2
    q[1] = x[0]; // 0x9912b6
    x[0] = x[4]; // 0x9912bb
    x[0] = x[0] * q[2]; // 0x9912be
    x[4] = x[4] * q[3]; // 0x9912c3
    q[2] = x[0]; // 0x9912c8
    q[3] = x[4]; // 0x9912cd
    return q;
}

// Pinned source 98F9F0, 881 bytes, SHA256 e35c19ea85bd2c4e554c1f8ab7169d47c711c304c34a7b43ee0b6679a7404cd9
inline std::array<float,9> warrior_resource_conjugate(std::array<float,9> matrix,const std::array<float,9>& linear,const std::array<float,9>& inverse) {
    std::array<float,8> x{}; std::array<float,13> scratch{};
    x[2] = linear[2]; // 0x98f9f7
    x[4] = linear[0]; // 0x98f9fc
    x[1] = matrix[6]; // 0x98fa00
    x[7] = matrix[0]; // 0x98fa05
    x[5] = linear[1]; // 0x98fa09
    x[6] = matrix[3]; // 0x98fa0e
    x[3] = x[2]; // 0x98fa13
    x[0] = x[4]; // 0x98fa16
    x[0] = x[0] * x[7]; // 0x98fa19
    x[3] = x[3] * x[1]; // 0x98fa1d
    x[3] = x[3] + x[0]; // 0x98fa21
    x[0] = x[5]; // 0x98fa25
    x[0] = x[0] * x[6]; // 0x98fa28
    x[3] = x[3] + x[0]; // 0x98fa2c
    x[0] = matrix[1]; // 0x98fa30
    x[0] = x[0] * x[4]; // 0x98fa35
    x[4] = matrix[4]; // 0x98fa39
    x[4] = x[4] * x[5]; // 0x98fa3e
    x[0] = x[0] + x[4]; // 0x98fa42
    x[4] = matrix[7]; // 0x98fa46
    x[4] = x[4] * x[2]; // 0x98fa4b
    x[2] = linear[0]; // 0x98fa4f
    x[0] = x[0] + x[4]; // 0x98fa53
    x[4] = matrix[2]; // 0x98fa57
    x[4] = x[4] * x[2]; // 0x98fa5c
    x[2] = matrix[5]; // 0x98fa60
    x[2] = x[2] * x[5]; // 0x98fa65
    x[4] = x[4] + x[2]; // 0x98fa69
    x[2] = matrix[8]; // 0x98fa6d
    x[5] = x[2]; // 0x98fa72
    x[2] = linear[2]; // 0x98fa75
    x[5] = x[5] * x[2]; // 0x98fa7a
    x[2] = linear[5]; // 0x98fa7e
    scratch[0] = x[2]; // 0x98fa83
    x[2] = linear[4]; // 0x98fa88
    scratch[12] = x[2]; // 0x98fa8d
    x[2] = linear[3]; // 0x98fa93
    x[4] = x[4] + x[5]; // 0x98fa98
    x[5] = x[2]; // 0x98fa9c
    x[5] = x[5] * x[7]; // 0x98fa9f
    x[7] = scratch[12]; // 0x98faa3
    x[7] = x[7] * x[6]; // 0x98faa9
    x[6] = scratch[0]; // 0x98faad
    x[6] = x[6] * x[1]; // 0x98fab2
    x[5] = x[5] + x[7]; // 0x98fab6
    x[7] = scratch[12]; // 0x98faba
    x[5] = x[5] + x[6]; // 0x98fac0
    x[6] = matrix[1]; // 0x98fac4
    x[1] = x[2]; // 0x98fac9
    x[1] = x[1] * x[6]; // 0x98facc
    x[6] = matrix[4]; // 0x98fad0
    x[7] = x[7] * x[6]; // 0x98fad5
    x[6] = matrix[7]; // 0x98fad9
    x[1] = x[1] + x[7]; // 0x98fade
    x[7] = scratch[0]; // 0x98fae2
    x[7] = x[7] * x[6]; // 0x98fae7
    x[6] = matrix[2]; // 0x98faeb
    x[2] = x[2] * x[6]; // 0x98faf0
    x[6] = scratch[12]; // 0x98faf4
    x[1] = x[1] + x[7]; // 0x98fafa
    x[7] = matrix[5]; // 0x98fafe
    x[6] = x[6] * x[7]; // 0x98fb03
    x[7] = matrix[8]; // 0x98fb07
    x[2] = x[2] + x[6]; // 0x98fb0c
    x[6] = scratch[0]; // 0x98fb10
    x[6] = x[6] * x[7]; // 0x98fb15
    x[7] = linear[7]; // 0x98fb19
    x[2] = x[2] + x[6]; // 0x98fb1e
    x[6] = linear[6]; // 0x98fb22
    scratch[7] = x[2]; // 0x98fb27
    x[2] = linear[8]; // 0x98fb2d
    scratch[12] = x[2]; // 0x98fb32
    x[2] = matrix[0]; // 0x98fb38
    scratch[0] = x[6]; // 0x98fb3c
    x[6] = x[6] * x[2]; // 0x98fb41
    x[2] = matrix[3]; // 0x98fb45
    scratch[1] = x[7]; // 0x98fb4a
    x[7] = x[7] * x[2]; // 0x98fb50
    x[6] = x[6] + x[7]; // 0x98fb54
    x[7] = scratch[12]; // 0x98fb58
    x[2] = matrix[6]; // 0x98fb5e
    x[7] = x[7] * x[2]; // 0x98fb63
    x[6] = x[6] + x[7]; // 0x98fb67
    x[2] = scratch[0]; // 0x98fb6b
    x[7] = scratch[1]; // 0x98fb70
    scratch[8] = x[6]; // 0x98fb76
    x[6] = matrix[1]; // 0x98fb7c
    x[2] = x[2] * x[6]; // 0x98fb81
    x[6] = matrix[4]; // 0x98fb85
    x[7] = x[7] * x[6]; // 0x98fb8a
    x[6] = matrix[7]; // 0x98fb8e
    x[2] = x[2] + x[7]; // 0x98fb93
    x[7] = scratch[12]; // 0x98fb97
    x[7] = x[7] * x[6]; // 0x98fb9d
    x[6] = matrix[2]; // 0x98fba1
    x[2] = x[2] + x[7]; // 0x98fba6
    x[7] = scratch[0]; // 0x98fbaa
    x[7] = x[7] * x[6]; // 0x98fbaf
    x[6] = scratch[1]; // 0x98fbb3
    scratch[0] = x[7]; // 0x98fbb9
    x[7] = matrix[5]; // 0x98fbbe
    x[6] = x[6] * x[7]; // 0x98fbc3
    x[7] = scratch[0]; // 0x98fbc7
    x[7] = x[7] + x[6]; // 0x98fbcc
    x[6] = scratch[12]; // 0x98fbd0
    scratch[0] = x[7]; // 0x98fbd6
    x[7] = matrix[8]; // 0x98fbdb
    x[6] = x[6] * x[7]; // 0x98fbe0
    x[7] = scratch[0]; // 0x98fbe4
    x[7] = x[7] + x[6]; // 0x98fbe9
    x[6] = inverse[6]; // 0x98fbed
    x[6] = x[6] * x[4]; // 0x98fbf2
    scratch[10] = x[7]; // 0x98fbf6
    x[7] = inverse[3]; // 0x98fbfc
    x[7] = x[7] * x[0]; // 0x98fc01
    x[6] = x[6] + x[7]; // 0x98fc05
    x[7] = inverse[0]; // 0x98fc09
    x[7] = x[7] * x[3]; // 0x98fc0d
    x[6] = x[6] + x[7]; // 0x98fc11
    matrix[0] = x[6]; // 0x98fc15
    x[6] = inverse[7]; // 0x98fc19
    x[7] = inverse[1]; // 0x98fc1e
    x[6] = x[6] * x[4]; // 0x98fc23
    x[7] = x[7] * x[3]; // 0x98fc27
    x[6] = x[6] + x[7]; // 0x98fc2b
    x[7] = inverse[4]; // 0x98fc2f
    x[7] = x[7] * x[0]; // 0x98fc34
    x[6] = x[6] + x[7]; // 0x98fc38
    matrix[1] = x[6]; // 0x98fc3c
    x[6] = inverse[8]; // 0x98fc41
    x[0] = x[0] * inverse[5]; // 0x98fc46
    x[6] = x[6] * x[4]; // 0x98fc4b
    x[4] = inverse[2]; // 0x98fc4f
    x[4] = x[4] * x[3]; // 0x98fc54
    x[6] = x[6] + x[4]; // 0x98fc58
    x[6] = x[6] + x[0]; // 0x98fc5c
    x[0] = scratch[7]; // 0x98fc60
    matrix[2] = x[6]; // 0x98fc66
    x[3] = inverse[6]; // 0x98fc6b
    x[4] = inverse[3]; // 0x98fc70
    x[3] = x[3] * x[0]; // 0x98fc75
    x[4] = x[4] * x[1]; // 0x98fc79
    x[3] = x[3] + x[4]; // 0x98fc7d
    x[4] = inverse[0]; // 0x98fc81
    x[4] = x[4] * x[5]; // 0x98fc85
    x[3] = x[3] + x[4]; // 0x98fc89
    matrix[3] = x[3]; // 0x98fc8d
    x[3] = inverse[7]; // 0x98fc92
    x[4] = inverse[1]; // 0x98fc97
    x[3] = x[3] * x[0]; // 0x98fc9c
    x[4] = x[4] * x[5]; // 0x98fca0
    x[3] = x[3] + x[4]; // 0x98fca4
    x[4] = inverse[4]; // 0x98fca8
    x[4] = x[4] * x[1]; // 0x98fcad
    x[3] = x[3] + x[4]; // 0x98fcb1
    matrix[4] = x[3]; // 0x98fcb5
    x[3] = inverse[8]; // 0x98fcba
    x[3] = x[3] * x[0]; // 0x98fcbf
    x[0] = inverse[2]; // 0x98fcc3
    x[0] = x[0] * x[5]; // 0x98fcc8
    x[3] = x[3] + x[0]; // 0x98fccc
    x[1] = x[1] * inverse[5]; // 0x98fcd0
    x[3] = x[3] + x[1]; // 0x98fcd5
    x[1] = scratch[10]; // 0x98fcd9
    matrix[5] = x[3]; // 0x98fcdf
    x[3] = inverse[6]; // 0x98fce4
    x[0] = inverse[3]; // 0x98fce9
    x[4] = inverse[0]; // 0x98fcee
    x[3] = x[3] * x[1]; // 0x98fcf2
    x[0] = x[0] * x[2]; // 0x98fcf6
    x[3] = x[3] + x[0]; // 0x98fcfa
    x[0] = scratch[8]; // 0x98fcfe
    x[4] = x[4] * x[0]; // 0x98fd04
    x[3] = x[3] + x[4]; // 0x98fd08
    matrix[6] = x[3]; // 0x98fd0c
    x[3] = inverse[7]; // 0x98fd11
    x[4] = inverse[1]; // 0x98fd16
    x[3] = x[3] * x[1]; // 0x98fd1b
    x[4] = x[4] * x[0]; // 0x98fd1f
    x[3] = x[3] + x[4]; // 0x98fd23
    x[4] = inverse[4]; // 0x98fd27
    x[4] = x[4] * x[2]; // 0x98fd2c
    x[3] = x[3] + x[4]; // 0x98fd30
    matrix[7] = x[3]; // 0x98fd34
    x[3] = inverse[8]; // 0x98fd39
    x[2] = x[2] * inverse[5]; // 0x98fd3e
    x[3] = x[3] * x[1]; // 0x98fd43
    x[1] = inverse[2]; // 0x98fd47
    x[1] = x[1] * x[0]; // 0x98fd4c
    x[3] = x[3] + x[1]; // 0x98fd50
    x[3] = x[3] + x[2]; // 0x98fd54
    matrix[8] = x[3]; // 0x98fd58
    return matrix;
}

// Pinned source 9900A0, 442 bytes, SHA256 ae128ad18e2b2598541b72ea31ff3b27be69f083d38d075c4d95a5bcf1bd84a5
inline std::array<float,9> warrior_resource_invert(const std::array<float,9>& linear) {
    std::array<float,8> x{}; std::array<float,9> inverse{};
    x[3] = linear[8]; // 0x9900a0
    x[4] = linear[4]; // 0x9900a5
    x[6] = linear[5]; // 0x9900aa
    x[5] = linear[7]; // 0x9900af
    x[1] = linear[3]; // 0x9900b4
    x[2] = linear[6]; // 0x9900b9
    x[0] = x[4]; // 0x9900be
    x[0] = x[0] * x[3]; // 0x9900c1
    x[7] = x[6]; // 0x9900c5
    x[7] = x[7] * x[5]; // 0x9900c8
    x[0] = x[0] - x[7]; // 0x9900cc
    x[0] = x[0] * linear[0]; // 0x9900d0
    x[7] = x[1]; // 0x9900d4
    x[7] = x[7] * x[3]; // 0x9900d7
    x[3] = x[2]; // 0x9900db
    x[3] = x[3] * x[6]; // 0x9900de
    x[1] = x[1] * x[5]; // 0x9900e2
    x[7] = x[7] - x[3]; // 0x9900e6
    x[7] = x[7] * linear[1]; // 0x9900ea
    x[2] = x[2] * x[4]; // 0x9900ef
    x[1] = x[1] - x[2]; // 0x9900f3
    x[1] = x[1] * linear[2]; // 0x9900f7
    x[0] = x[0] - x[7]; // 0x9900fc
    x[0] = x[0] + x[1]; // 0x990100
    warrior_curve_require(std::isfinite(x[0]) && x[0]!=0.f,"Singular source resource basis"); // 0x990104
    x[1] = std::bit_cast<float>(0x3f800000u); // 0x990115
    x[1] = x[1] / x[0]; // 0x99011d
    x[0] = x[4]; // 0x990121
    x[0] = x[0] * linear[8]; // 0x990124
    x[2] = x[5]; // 0x990129
    x[2] = x[2] * x[6]; // 0x99012c
    x[0] = x[0] - x[2]; // 0x990130
    x[0] = x[0] * x[1]; // 0x990134
    inverse[0] = x[0]; // 0x990138
    x[2] = linear[8]; // 0x99013c
    x[2] = x[2] * linear[3]; // 0x990141
    x[0] = linear[6]; // 0x990146
    x[0] = x[0] * linear[5]; // 0x99014b
    x[2] = x[2] - x[0]; // 0x990150
    x[0] = std::bit_cast<float>(0x80000000u); // 0x990154
    x[2] = x[2] * x[1]; // 0x99015c
    x[2] = std::bit_cast<float>(std::bit_cast<std::uint32_t>(x[2]) ^ std::bit_cast<std::uint32_t>(x[0])); // 0x990160
    inverse[3] = x[2]; // 0x990163
    x[2] = linear[7]; // 0x990168
    x[2] = x[2] * linear[3]; // 0x99016d
    x[3] = linear[6]; // 0x990172
    x[3] = x[3] * linear[4]; // 0x990177
    x[2] = x[2] - x[3]; // 0x99017c
    x[2] = x[2] * x[1]; // 0x990180
    inverse[6] = x[2]; // 0x990184
    x[2] = linear[1]; // 0x990189
    x[2] = x[2] * linear[8]; // 0x99018e
    x[3] = linear[7]; // 0x990193
    x[3] = x[3] * linear[2]; // 0x990198
    x[2] = x[2] - x[3]; // 0x99019d
    x[2] = x[2] * x[1]; // 0x9901a1
    x[2] = std::bit_cast<float>(std::bit_cast<std::uint32_t>(x[2]) ^ std::bit_cast<std::uint32_t>(x[0])); // 0x9901a5
    inverse[1] = x[2]; // 0x9901a8
    x[2] = linear[8]; // 0x9901ad
    x[2] = x[2] * linear[0]; // 0x9901b2
    x[3] = linear[6]; // 0x9901b6
    x[3] = x[3] * linear[2]; // 0x9901bb
    x[2] = x[2] - x[3]; // 0x9901c0
    x[2] = x[2] * x[1]; // 0x9901c4
    inverse[4] = x[2]; // 0x9901c8
    x[2] = linear[7]; // 0x9901cd
    x[2] = x[2] * linear[0]; // 0x9901d2
    x[3] = linear[6]; // 0x9901d6
    x[3] = x[3] * linear[1]; // 0x9901db
    x[2] = x[2] - x[3]; // 0x9901e0
    x[2] = x[2] * x[1]; // 0x9901e4
    x[2] = std::bit_cast<float>(std::bit_cast<std::uint32_t>(x[2]) ^ std::bit_cast<std::uint32_t>(x[0])); // 0x9901e8
    inverse[7] = x[2]; // 0x9901eb
    x[2] = linear[1]; // 0x9901f0
    x[2] = x[2] * linear[5]; // 0x9901f5
    x[3] = linear[4]; // 0x9901fa
    x[3] = x[3] * linear[2]; // 0x9901ff
    x[2] = x[2] - x[3]; // 0x990204
    x[2] = x[2] * x[1]; // 0x990208
    inverse[2] = x[2]; // 0x99020c
    x[2] = linear[5]; // 0x990211
    x[2] = x[2] * linear[0]; // 0x990216
    x[3] = linear[2]; // 0x99021a
    x[3] = x[3] * linear[3]; // 0x99021f
    x[2] = x[2] - x[3]; // 0x990224
    x[2] = x[2] * x[1]; // 0x990228
    x[2] = std::bit_cast<float>(std::bit_cast<std::uint32_t>(x[2]) ^ std::bit_cast<std::uint32_t>(x[0])); // 0x99022c
    inverse[5] = x[2]; // 0x99022f
    x[0] = linear[4]; // 0x990234
    x[0] = x[0] * linear[0]; // 0x990239
    x[2] = linear[1]; // 0x99023d
    x[2] = x[2] * linear[3]; // 0x990242
    x[0] = x[0] - x[2]; // 0x990247
    x[0] = x[0] * x[1]; // 0x99024b
    inverse[8] = x[0]; // 0x99024f
    return inverse;
}

inline std::uint32_t warrior_resource_flags(const std::array<float,16>& p) {
    float position=0,orientation=0,shear=0;for(unsigned i=0;i<3;++i){const float delta=p[i]-0.f;position=std::fabs(delta)+position;}
    for(unsigned i=0;i<4;++i){const float delta=p[i+3]-(i==3?1.f:0.f);orientation=std::fabs(delta)+orientation;}
    for(unsigned i=0;i<9;++i){const float delta=p[i+7]-(i%4==0?1.f:0.f);shear=std::fabs(delta)+shear;}
    const float large=std::bit_cast<float>(0x3a83126fu),small=std::bit_cast<float>(0x38d1b717u);
    return (position>=large?1u:0u)|(orientation>=small?2u:0u)|(shear>=large?4u:0u);
}
inline BnwWarriorIdleLocalTransform warrior_resource_local_transform(BnwWarriorIdleLocalTransform out,const BnwWarriorNativeResourceBasis& basis) {
    auto& p=out.position_orientation_scale;const auto& b=basis.linear;const auto& t=basis.translation;const float x=p[0],y=p[1],z=p[2];
    const float a0=b[2]*z,b0=b[1]*y,c0=b[0]*x;const float pair0=a0+b0,all0=pair0+c0;p[0]=all0+t[0];
    const float a1=b[3]*x,b1=b[5]*z,c1=b[4]*y;const float pair1=a1+b1,all1=pair1+c1;p[1]=t[1]+all1;
    const float a2=b[6]*x,b2=b[8]*z,c2=b[7]*y;const float pair2=a2+b2,all2=pair2+c2;p[2]=t[2]+all2;
    const auto q=warrior_resource_quaternion({p[3],p[4],p[5],p[6]},b,basis.inverse);std::copy(q.begin(),q.end(),p.begin()+3);
    std::array<float,9> s;std::copy_n(p.begin()+7,9,s.begin());s=warrior_resource_conjugate(s,b,basis.inverse);std::copy(s.begin(),s.end(),p.begin()+7);
    warrior_curve_require(std::ranges::all_of(p,[](float v){return std::isfinite(v);}),"Source resource transform overflow/nonfinite quaternion");out.flags=warrior_resource_flags(p);return out;
}
inline std::array<float,16> warrior_resource_inverse_bind(std::array<float,16> m,const BnwWarriorNativeResourceBasis& basis) {
    const auto& b=basis.linear;const auto& t=basis.translation;const float x=m[12],y=m[13],z=m[14];
    const float a0=b[2]*z,b0=b[1]*y,c0=b[0]*x;const float pair0=a0+b0,all0=pair0+c0;m[12]=t[0]+all0;
    const float a1=b[5]*z,b1=b[4]*y,c1=b[3]*x;const float pair1=a1+b1,all1=pair1+c1;m[13]=t[1]+all1;
    const float a2=b[8]*z,b2=b[7]*y,c2=b[6]*x;const float pair2=a2+b2,all2=pair2+c2;m[14]=t[2]+all2;
    std::array<float,9> matrix;for(unsigned row=0;row<3;++row)for(unsigned col=0;col<3;++col)matrix[row*3+col]=m[col*4+row];
    matrix=warrior_resource_conjugate(matrix,b,basis.inverse);for(unsigned row=0;row<3;++row)for(unsigned col=0;col<3;++col)m[col*4+row]=matrix[row*3+col];validate_bnw_warrior_native_affine(m);return m;
}
} // namespace warrior_resource_detail
// Current geometry metadata artifact. Animation metadata/curve conversion is a
// separate source99F660 operation; do not apply this to an already sampled pose.
namespace warrior_resource_detail {
inline BnwWarriorNativeResourceBasis compile_resource_basis_recipe(const BnwWarriorGr2ArtToolMetadata& metadata,float compiled_scale) {
    const std::array<float,12> source_words{metadata.origin[0],metadata.origin[1],metadata.origin[2],metadata.right[0],metadata.right[1],metadata.right[2],metadata.up[0],metadata.up[1],metadata.up[2],metadata.back[0],metadata.back[1],metadata.back[2]};
    const std::array<std::uint32_t,12> expected_words{0,0,0,0x3f800000u,0,0,0,0,0x3f800000u,0,0xbf800000u,0};
    for(unsigned i=0;i<12;++i)warrior_curve_require(std::bit_cast<std::uint32_t>(source_words[i])==expected_words[i],"Native resource actual metadata float words");
    const auto old=compile_bnw_warrior_gr2_native_basis(metadata,compiled_scale);const auto inv=warrior_resource_detail::warrior_resource_invert(old.linear);
    warrior_curve_require(std::ranges::all_of(inv,[](float v){return std::isfinite(v);}),"Native resource inverse overflow");
    return {{std::bit_cast<std::uint32_t>(metadata.units_per_meter),std::bit_cast<std::uint32_t>(compiled_scale)},old.linear,inv,{0,0,0}};
}
} // namespace warrior_resource_detail
inline BnwWarriorNativeResourceBasis compile_bnw_warrior_native_resource_basis(const BnwWarriorGr2ArtToolMetadata& metadata,std::string_view actual_metadata_identity,float compiled_scale) {
    warrior_curve_require(actual_metadata_identity=="78ec5c7873c3cf59ec1d140dfca5990c3e0011076da434aeedb4a6bea56f49ac","Foreign actual Warrior resource metadata identity");
    return warrior_resource_detail::compile_resource_basis_recipe(metadata,compiled_scale);
}
inline void validate_bnw_warrior_native_resource_basis(const BnwWarriorNativeResourceBasis& basis) {
    const BnwWarriorGr2ArtToolMetadata metadata{std::bit_cast<float>(basis.key.source_units_bits),{0,0,0},{1,0,0},{0,0,1},{0,-1,0}};
    const auto expected=compile_bnw_warrior_native_resource_basis(metadata,"78ec5c7873c3cf59ec1d140dfca5990c3e0011076da434aeedb4a6bea56f49ac",std::bit_cast<float>(basis.key.compiled_scale_bits));
    auto equal=[](auto a,auto b){for(unsigned i=0;i<a.size();++i)if(std::bit_cast<std::uint32_t>(a[i])!=std::bit_cast<std::uint32_t>(b[i]))return false;return true;};
    warrior_curve_require(equal(basis.linear,expected.linear)&&equal(basis.inverse,expected.inverse)&&equal(basis.translation,expected.translation),"Foreign/corrupt native resource basis values");
}
struct BnwWarriorNativeResourceSkeleton {
    BnwWarriorNativeResourceBasisKey basis_key;
    std::array<BnwWarriorIdleLocalTransform,23> bind_local;
    std::array<BnwWarriorNativeSkeletonBone,23> skeleton;
};
inline BnwWarriorNativeResourceSkeleton convert_bnw_warrior_native_resource_skeleton(std::span<const BnwWarriorIdleLocalTransform> raw_bind_local,std::span<const BnwWarriorNativeSkeletonBone> raw_skeleton,std::string_view actual_raw_skeleton_identity,const BnwWarriorNativeResourceBasis& basis) {
    warrior_curve_require(actual_raw_skeleton_identity=="7eed0924e1559e030c8d4f14746d1ad93c4f3e87550789a96fc80f8feb713d7e","Foreign actual selected raw skeleton identity");
    warrior_curve_require(raw_bind_local.size()==23&&raw_skeleton.size()==23,"Native resource raw skeleton cardinality");validate_bnw_warrior_native_resource_basis(basis);BnwWarriorNativeResourceSkeleton out{};out.basis_key=basis.key;
    for(unsigned i=0;i<23;++i){const auto& bone=raw_skeleton[i];warrior_curve_require(!bone.name.empty()&&std::count_if(raw_skeleton.begin(),raw_skeleton.end(),[&](const auto& v){return v.name==bone.name;})==1&&bone.parent>=-1&&bone.parent<int(i),"Native resource source names/parents");validate_bnw_warrior_native_affine(bone.inverse_bind);warrior_curve_require(raw_bind_local[i].flags<=3&&std::ranges::all_of(raw_bind_local[i].position_orientation_scale,[](float v){return std::isfinite(v);}),"Native resource supported raw bind pose");out.bind_local[i]=warrior_resource_detail::warrior_resource_local_transform(raw_bind_local[i],basis);out.skeleton[i]=bone;out.skeleton[i].inverse_bind=warrior_resource_detail::warrior_resource_inverse_bind(bone.inverse_bind,basis);}
    return out;
}
} // namespace civ5::graphics
