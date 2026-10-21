#pragma once
#include "civ5/graphics/bnw_warrior_native_vertex_conversion.hpp"
#include <cmath>
namespace civ5::graphics {
inline constexpr std::array<int,20> bnw_warrior_native_palette{3,11,14,22,12,13,21,4,5,15,16,17,18,7,8,9,10,19,20,1};
// Binary expansion only. Float32 -> Real16 rounding belongs to98B0C0.
inline float bnw_warrior_expand_real16(std::uint16_t h) {
    const float sign=(h&0x8000)?-1.f:1.f;
    const unsigned exponent=(h>>10)&31,mantissa=h&1023;
    if(exponent==31)throw std::invalid_argument("Nonfinite Warrior format4 component");
    if(exponent==0)return sign*std::ldexp(float(mantissa),-24);
    return sign*std::ldexp(float(1024+mantissa),int(exponent)-25);
}
inline void validate_bnw_warrior_native_format4(std::span<const std::uint8_t> bytes,std::size_t count) {
    if(count==0||count>std::numeric_limits<std::size_t>::max()/28||bytes.size()!=count*28)
        throw std::invalid_argument("Warrior format4 consumer cardinality");
    for(std::size_t v=0;v<count;++v){const auto* p=bytes.data()+v*28;
        // The native producer leaves sum0 unchanged. The three-input shader
        // cannot represent fourth255: refuse before allocating/publishing GL.
        if(p[11]!=0||(unsigned(p[8])+p[9]+p[10])==0)
            throw std::invalid_argument("Unsupported zero-divisor/fourth Warrior influence");
        for(unsigned j=0;j<4;++j)if(p[12+j]>=20)throw std::invalid_argument("Warrior format4 mesh binding index");
        for(unsigned offset:{0u,2u,4u,6u,16u,18u,20u,22u,24u,26u}){
            std::uint16_t h;std::memcpy(&h,p+offset,2);(void)bnw_warrior_expand_real16(h);
        }
        if(p[6]||p[7]||p[22]||p[23])throw std::invalid_argument("Warrior format4 source default component differs");
    }
}
using BnwWarriorNativePaletteRows=std::array<float,20*3*4>;
struct BnwWarriorNativeVertexOutput {std::array<float,3> world{},raw_normal{};};
// Source05D9CFE8: v0.w is ignored; position=(xyz,1), normal DP3.
// BlendWeights arrive as D3D UBYTE4N / GL normalized U8, not inferred residual.
inline BnwWarriorNativeVertexOutput consume_bnw_warrior_native_vertex(
    std::span<const std::uint8_t,28> p,const BnwWarriorNativePaletteRows& rows) {
    validate_bnw_warrior_native_format4(p,1);BnwWarriorNativeVertexOutput result;
    std::array<float,3> position{},normal{};
    for(unsigned c=0;c<3;++c){std::uint16_t h;std::memcpy(&h,p.data()+c*2,2);position[c]=bnw_warrior_expand_real16(h);std::memcpy(&h,p.data()+16+c*2,2);normal[c]=bnw_warrior_expand_real16(h);}
    for(float f:rows)if(!std::isfinite(f))throw std::invalid_argument("Nonfinite Warrior palette row");
    for(unsigned influence=0;influence<3;++influence){
        if(influence>0&&p[9]==0)break; // Third guard is nested under second.
        if(influence==2&&p[10]==0)break;
        const float weight=float(p[8+influence])/255.f;
        for(unsigned r=0;r<3;++r){const auto at=unsigned(p[12+influence])*12+r*4;
            float point=position[0]*rows[at],n=normal[0]*rows[at];
            for(unsigned c=1;c<3;++c){point+=position[c]*rows[at+c];n+=normal[c]*rows[at+c];}
            point+=rows[at+3];result.world[r]+=weight*point;result.raw_normal[r]+=weight*n;
        }
    }
    return result;
}
// Arithmetic/branch semantics of pinned mainVS. CPU supplied rows use the
// existing SDK skin pose composed with the SAME member world transform. This
// is explicitly not evidence of the native animation/matrix producer.
inline constexpr const char* bnw_warrior_native_format4_vertex_shader=R"GLSL(#version 460 core
layout(location=0) in vec4 position;
layout(location=1) in vec4 rawNormal;
layout(location=2) in vec2 sourceUV;
layout(location=3) in vec4 sourceWeights;
layout(location=4) in vec4 sourceIndices;
uniform vec4 boneRows[60];
uniform mat4 view,projection;
uniform vec3 f3CameraWorld;
out vec4 warriorNormalU,warriorViewV;
out vec3 warriorWorld,warriorClip;
vec3 point3(int b,vec4 p){return vec3(dot(p,boneRows[b]),dot(p,boneRows[b+1]),dot(p,boneRows[b+2]));}
vec3 normal3(int b){return vec3(dot(rawNormal.xyz,boneRows[b].xyz),dot(rawNormal.xyz,boneRows[b+1].xyz),dot(rawNormal.xyz,boneRows[b+2].xyz));}
void main(){
 vec4 p=vec4(position.xyz,1);
 int b=3*int(sourceIndices.x);
 vec3 world=point3(b,p)*sourceWeights.x;
 vec3 n=normal3(b)*sourceWeights.x;
 if(sourceWeights.y>0){
   b=3*int(sourceIndices.y);world+=point3(b,p)*sourceWeights.y;n+=normal3(b)*sourceWeights.y;
   if(sourceWeights.z>0){b=3*int(sourceIndices.z);world+=point3(b,p)*sourceWeights.z;n+=normal3(b)*sourceWeights.z;}
 }
 vec4 clip=projection*view*vec4(world,1);gl_Position=clip;
 warriorNormalU=vec4(n,sourceUV.x);warriorViewV=vec4(world-f3CameraWorld,sourceUV.y);
 warriorWorld=world;warriorClip=vec3(clip.x,clip.y,clip.w);
}
)GLSL";
} // namespace civ5::graphics
