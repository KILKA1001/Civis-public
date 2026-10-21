#pragma once
#include <array>
#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <string_view>

namespace civ5::graphics {
inline constexpr std::string_view bnw_warrior_native_vs_sha="450c843d3b887c89fe4260de76925903e0e4e05ce6b859c106b442c1a0418a50";
inline constexpr std::string_view bnw_warrior_native_ps_sha="e648d21b1fde0f1a650e6d2677164c7038b2e02adc9390c0529fab3ba61a0712";
inline constexpr unsigned bnw_warrior_native_shader_family=0x54;
using WarriorColor=std::array<float,4>;
// Values are samples/actual frame inputs, never lighting coefficients or defaults.
// This arithmetic kernel proves the main opaque TeamColorS PS only. It does not
// choose a native shader variant, sampler state, live color or animation owner.
struct BnwWarriorMaterialSamples {
    WarriorColor diffuse,sref,irradiance,dull_environment,sharp_environment;
    WarriorColor team_color;
    std::array<float,3> fog_color;
    float fog_factor;
};
inline WarriorColor evaluate_bnw_warrior_material_samples(const BnwWarriorMaterialSamples &s) {
    const auto unorm=[](float v){if(!std::isfinite(v)||v<0||v>1)throw std::invalid_argument("Unknown/out-of-range Warrior material sample");};
    for(const auto *v:{&s.diffuse,&s.sref,&s.irradiance,&s.dull_environment,&s.sharp_environment,&s.team_color})for(float x:*v)unorm(x);
    for(float v:s.fog_color)if(!std::isfinite(v))throw std::invalid_argument("Unknown Warrior fog color");unorm(s.fog_factor);
    // Pinned PS: r4=sref*sref; r1.w=r4.w*r4.w; r1.w=saturate(r1.w*r1.w).
    float gloss=s.sref[3]*s.sref[3];gloss=gloss*gloss;gloss=std::clamp(gloss*gloss,0.f,1.f);
    const float alpha2=s.diffuse[3]*s.diffuse[3];WarriorColor out{};
    for(unsigned i=0;i<3;++i){
        const float dull2=s.dull_environment[i]*s.dull_environment[i];
        const float sharp2=s.sharp_environment[i]*s.sharp_environment[i];
        const float spec=(dull2+gloss*(sharp2-dull2))*(s.sref[i]*s.sref[i]);
        const float team2=s.team_color[i]*s.team_color[i];
        const float tint=team2+alpha2*(1.f-team2);
        const float diffuse=(s.diffuse[i]*s.diffuse[i])*tint;
        const float linear=spec+diffuse*(s.irradiance[i]*s.irradiance[i]);
        // Native RSQ/RCP pair. sqrt is the mathematical CPU equivalent;
        // hardware precision/zero behavior remains an explicit oracle check.
        const float encoded=std::sqrt(linear);
        out[i]=encoded+s.fog_factor*(s.fog_color[i]-encoded);
    }
    out[3]=1.f;return out;
}
// Native input meanings follow VS 05D9CFE8: normal.xyz+uv.x,
// (skinnedWorld-cameraWorld).xyz+uv.y, world.xyz, clip.xyww.
// This stage requires skinned normals; unposed GR2 normals are not valid inputs.
// No source sampler policy, TeamColorS owner or resource is inferred here.
inline constexpr std::string_view bnw_warrior_main_material_fragment=R"GLSL(#version 460 core
in vec4 warriorNormalU;
in vec4 warriorViewV;
in vec3 warriorWorld;
in vec3 warriorClip;
layout(binding=0) uniform sampler2D BaseTextureMap;
layout(binding=1) uniform sampler2D SREFMap;
layout(binding=2) uniform samplerCube IrradianceCubeMap;
layout(binding=3) uniform samplerCube DullEnvironmentCubeMap;
layout(binding=4) uniform samplerCube EnvironmentCubeMap;
layout(binding=5) uniform sampler2D DistanceFogRamp;
uniform vec4 g_FogConsts;
uniform vec3 f3FogColor;
uniform vec3 f3CameraWorld;
uniform vec4 TeamColorS;
layout(location=0) out vec4 warriorPixel;
void main(){
 precise vec3 delta=warriorWorld-f3CameraWorld;
 precise float density=delta.z*g_FogConsts.x;
 density=density*1.44269502;
 density=1.0-exp2(density);
 precise float fog=dot(delta,delta);
 fog=fog*g_FogConsts.y;
 fog=density*fog;
 fog=(1.0/delta.z)*fog;
 fog=fog*1.44269502;
 fog=1.0-clamp(exp2(fog),0.0,1.0);
 precise float rampX=(warriorClip.y/warriorClip.z)*0.5+0.5;
 fog=fog*texture(DistanceFogRamp,vec2(rampX,g_FogConsts.w)).x;
 vec3 viewDirection=normalize(warriorViewV.xyz);
 vec3 normal=normalize(warriorNormalU.xyz);
 precise float twiceDot=2.0*dot(viewDirection,normal);
 precise vec3 reflection=viewDirection-normal*twiceDot;
 vec3 irradiance=texture(IrradianceCubeMap,normal).rgb;
 irradiance=irradiance*irradiance;
 vec3 sharp=texture(EnvironmentCubeMap,reflection).rgb;
 vec3 dull=texture(DullEnvironmentCubeMap,reflection).rgb;
 dull=dull*dull;
 precise vec3 environmentDelta=sharp*sharp-dull;
 vec2 uv=vec2(warriorNormalU.w,warriorViewV.w);
 vec4 sref=texture(SREFMap,uv);sref=sref*sref;
 vec4 diffuse=texture(BaseTextureMap,uv);diffuse=diffuse*diffuse;
 precise float gloss=sref.w*sref.w;gloss=clamp(gloss*gloss,0.0,1.0);
 precise vec3 specular=(dull+gloss*environmentDelta)*sref.rgb;
 precise vec3 teamSquared=TeamColorS.rgb*TeamColorS.rgb;
 precise vec3 tint=teamSquared+diffuse.a*(vec3(1.0)-teamSquared);
 precise vec3 lit=(diffuse.rgb*tint)*irradiance+specular;
 vec3 encoded=1.0/inversesqrt(lit);
 warriorPixel=vec4(encoded+fog*(f3FogColor-encoded),1.0);
}
)GLSL";
} // namespace civ5::graphics
