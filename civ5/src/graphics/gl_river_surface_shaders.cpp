#include "civ5/graphics/gl_river_surface.hpp"
#include <stdexcept>
namespace civ5::graphics {
namespace {
constexpr std::string_view vertex=R"GLSL(#version 460 core
out vec2 waterT0;out vec2 waterT1;out vec2 waterT2;out vec2 waterT3;out vec3 waterWorld;out vec3 waterClip;out vec4 waterT6;
layout(location=0) in vec3 aPosition;layout(location=1) in vec2 aUv0;layout(location=2) in vec2 aUv1;uniform vec4 uV[17];out vec4 tfClip;

vec4 sm3Mad(vec4 a,vec4 b,vec4 c){precise dvec4 v=dvec4(a)*dvec4(b)+dvec4(c);return vec4(v);}
vec4 sm3Lrp(vec4 a,vec4 b,vec4 c){precise vec4 x=b-c;return sm3Mad(a,x,c);}
float sm3Log(float a){return a==0.0 ? -3.402823466e38 : log2(abs(a));}
vec4 sm3Log(vec4 a){return vec4(sm3Log(a.x),sm3Log(a.y),sm3Log(a.z),sm3Log(a.w));}
vec4 sm3Dp3(vec4 a,vec4 b){precise float v=a.x*b.x;v=float(double(a.y)*double(b.y)+double(v));v=float(double(a.z)*double(b.z)+double(v));return vec4(v);}

vec4 sm3Dp2Add(vec4 a,vec4 b,vec4 c){precise float v=a.x*b.x;v=float(double(a.y)*double(b.y)+double(v));v=v+c.x;return vec4(v);}
vec4 sm3Dp4(vec4 a,vec4 b){precise float v=a.x*b.x;v=float(double(a.y)*double(b.y)+double(v));v=float(double(a.z)*double(b.z)+double(v));v=float(double(a.w)*double(b.w)+double(v));return vec4(v);}

void main(){
const vec4 c13=vec4(0.5,-0.5,0.0,0.0);
vec4 v0=vec4(aPosition,1.0);
vec4 v1=vec4(aUv0,0.0,1.0);
vec4 v2=vec4(aUv1,0.0,1.0);
precise vec4 o0;
precise vec4 o1;
precise vec4 o2;
precise vec4 o3;
precise vec4 o4;
precise vec4 o5;
precise vec4 o6;
precise vec4 o7;
precise vec4 r0;
precise vec4 r1;
// mul o1.xy, c14, v1
o1.xy=(uV[14].xyzw*v1.xyzw).xy;
// add r0, c12, v1.xyxy
r0=(uV[12].xyzw+v1.xyxy).xyzw;
// mul r0, r0, c14.xyxy
r0=(r0.xyzw*uV[14].xyxy).xyzw;
// mul o2.xy, r0, c14.z
o2.xy=(r0.xyzw*uV[14].zzzz).xy;
// mul o3.xy, r0.zwzw, c14.w
o3.xy=(r0.zwzw*uV[14].wwww).xy;
// mul r0.xyz, c1, v0.y
r0.xyz=(uV[1].xyzw*v0.yyyy).xyz;
// mad r0.xyz, v0.x, c0, r0
r0.xyz=(sm3Mad(v0.xxxx,uV[0].xyzw,r0.xyzw)).xyz;
// mad r0.xyz, v0.z, c2, r0
r0.xyz=(sm3Mad(v0.zzzz,uV[2].xyzw,r0.xyzw)).xyz;
// add r0.xyz, r0, c3
r0.xyz=(r0.xyzw+uV[3].xyzw).xyz;
// mul r1.xy, r0.y, c9
r1.xy=(r0.yyyy*uV[9].xyzw).xy;
// mad r1.xy, r0.x, c8, r1
r1.xy=(sm3Mad(r0.xxxx,uV[8].xyzw,r1.xyzw)).xy;
// mad r1.xy, r0.z, c10, r1
r1.xy=(sm3Mad(r0.zzzz,uV[10].xyzw,r1.xyzw)).xy;
// add r1.xy, r1, c11
r1.xy=(r1.xyzw+uV[11].xyzw).xy;
// mad o7.xy, r1, c13, c13.y
o7.xy=(sm3Mad(r1.xyzw,c13.xyzw,c13.yyyy)).xy;
// mul r1, r0.y, c5
r1=(r0.yyyy*uV[5].xyzw).xyzw;
// mad r1, r0.x, c4, r1
r1=(sm3Mad(r0.xxxx,uV[4].xyzw,r1.xyzw)).xyzw;
// mad r1, r0.z, c6, r1
r1=(sm3Mad(r0.zzzz,uV[6].xyzw,r1.xyzw)).xyzw;
// mov o5.xyz, r0
o5.xyz=(r0.xyzw).xyz;
// add r0, r1, c7
r0=(r1.xyzw+uV[7].xyzw).xyzw;
// mov o0, r0
o0=(r0.xyzw).xyzw;
// mov o6.xyz, r0.xyww
o6.xyz=(r0.xyww).xyz;
// mov o4.xy, v2
o4.xy=(v2.xyzw).xy;
// mov o7.zw, c13.z
o7.zw=(c13.zzzz).zw;
waterT0=o1.xy;waterT1=o2.xy;waterT2=o3.xy;waterT3=o4.xy;waterWorld=o5.xyz;waterClip=o6.xyz;waterT6=o7;tfClip=o0;
// API Y conversion; draw supplies native depth and pixel-center viewport.
gl_Position=vec4(o0.x,-o0.y,o0.z,o0.w);
}
)GLSL";
constexpr std::string_view pixel1=R"GLSL(#version 460 core
in vec2 waterT0;in vec2 waterT1;in vec2 waterT2;in vec2 waterT3;in vec3 waterWorld;in vec3 waterClip;in vec4 waterT6;
uniform vec4 uP[30];layout(location=0) out vec4 outColor;
layout(binding=5) uniform sampler2D s5;
layout(binding=6) uniform sampler2D s6;
layout(binding=8) uniform sampler2D s8;
layout(binding=9) uniform sampler2D s9;
layout(binding=10) uniform sampler2D s10;
layout(binding=11) uniform sampler3D s11;
layout(binding=12) uniform sampler3D s12;
// Measured original D3D9: LEAN, river color and alpha interpolate authored mip samples in float.
vec4 riverSample(sampler2D s,vec2 uv){float last=float(textureQueryLevels(s)-1);float level=clamp(textureQueryLod(s,uv).y,0.0,last);float low=floor(level);vec4 first=textureLod(s,uv,low);vec4 second=textureLod(s,uv,min(low+1.0,last));precise vec4 result=first*(1.0-(level-low))+second*(level-low);return result;}
vec4 riverLeanSample(sampler2D s,vec2 uv){float last=float(textureQueryLevels(s)-1);float level=clamp(textureQueryLod(s,uv).y,0.0,last);float low=floor(level);vec4 first=textureLod(s,uv,low);vec4 second=textureLod(s,uv,min(low+1.0,last));precise vec4 delta=second-first;precise dvec4 result=dvec4(delta)*double(level-low)+dvec4(first);return vec4(result);}

vec4 sm3Mad(vec4 a,vec4 b,vec4 c){precise dvec4 v=dvec4(a)*dvec4(b)+dvec4(c);return vec4(v);}
vec4 sm3Lrp(vec4 a,vec4 b,vec4 c){precise vec4 x=b-c;return sm3Mad(a,x,c);}
float sm3Log(float a){return a==0.0 ? -3.402823466e38 : log2(abs(a));}
vec4 sm3Log(vec4 a){return vec4(sm3Log(a.x),sm3Log(a.y),sm3Log(a.z),sm3Log(a.w));}
vec4 sm3Dp3(vec4 a,vec4 b){precise float v=a.x*b.x;v=float(double(a.y)*double(b.y)+double(v));v=float(double(a.z)*double(b.z)+double(v));return vec4(v);}

vec4 sm3Dp2Add(vec4 a,vec4 b,vec4 c){precise float v=a.x*b.x;v=float(double(a.y)*double(b.y)+double(v));v=v+c.x;return vec4(v);}
vec4 sm3Dp4(vec4 a,vec4 b){precise float v=a.x*b.x;v=float(double(a.y)*double(b.y)+double(v));v=float(double(a.z)*double(b.z)+double(v));v=float(double(a.w)*double(b.w)+double(v));return vec4(v);}

void main(){
const vec4 c0=vec4(-0.400000006,0.400000006,0.5,1.44269502);
const vec4 c1=vec4(-1.0,-0.0,0.454545468,1.0);
vec4 v0=vec4(waterT3,0.0,0.0);
vec4 v1=vec4(waterWorld,0.0);
vec4 v2=vec4(waterClip,0.0);
vec4 v3=waterT6;
precise vec4 r0;
precise vec4 r1;
precise vec4 r2;
precise vec4 r3;
precise vec4 oC0;
// texld r0, v3, s10
r0=(texture(s10,(v3.xyzw).xy)).xyzw;
// cmp r1, -r0_abs.x, c1.x, c1.y
r1=(mix(c1.yyyy,c1.xxxx,greaterThanEqual(-abs(r0.xxxx),vec4(0.0)))).xyzw;
// texkill r1
if(any(lessThan(r1.xyz,vec3(0.0))))discard;
// texld r1, v0, s6
r1=(riverSample(s6,(v0.xyzw).xy)).xyzw;
// texld r2, v0, s5
r2=(riverSample(s5,(v0.xyzw).xy)).xyzw;
// log r2.x, r1.x
r2.x=(sm3Log(r1.xxxx)).x;
// log r2.y, r1.y
r2.y=(sm3Log(r1.yyyy)).y;
// log r2.z, r1.z
r2.z=(sm3Log(r1.zzzz)).z;
// mul r1.xyz, r2, c1.z
r1.xyz=(r2.xyzw*c1.zzzz).xyz;
// exp r2.x, r1.x
r2.x=(exp2(r1.xxxx)).x;
// exp r2.y, r1.y
r2.y=(exp2(r1.yyyy)).y;
// exp r2.z, r1.z
r2.z=(exp2(r1.zzzz)).z;
// texld r1, r2, s12
r1=(texture(s12,(r2.xyzw).xyz)).xyzw;
// texld r3, v3, s9
r3=(texture(s9,(v3.xyzw).xy)).xyzw;
// add r0.y, -r3.w, c1.w
r0.y=(-r3.wwww+c1.wwww).y;
// mad r1.xyz, r1, r0.y, r3
r1.xyz=(sm3Mad(r1.xyzw,r0.yyyy,r3.xyzw)).xyz;
// add r1.xyz, r1, c0.x
r1.xyz=(r1.xyzw+c0.xxxx).xyz;
// mad r1.xyz, r0.x, r1, c0.y
r1.xyz=(sm3Mad(r0.xxxx,r1.xyzw,c0.yyyy)).xyz;
// texld r3, r1, s11
r3=(texture(s11,(r1.xyzw).xyz)).xyzw;
// mad r0.x, r0.z, -r0.x, r0.x
r0.x=(sm3Mad(r0.zzzz,-r0.xxxx,r0.xxxx)).x;
// lrp r2.xyz, r0.x, r3, r1
r2.xyz=(sm3Lrp(r0.xxxx,r3.xyzw,r1.xyzw)).xyz;
// rcp r0.x, v2.z
r0.x=(vec4(1.0)/v2.zzzz).x;
// mul r0.x, r0.x, v2.y
r0.x=(r0.xxxx*v2.yyyy).x;
// mad r0.x, r0.x, c0.z, c0.z
r0.x=(sm3Mad(r0.xxxx,c0.zzzz,c0.zzzz)).x;
// mov r0.y, c11.w
r0.y=(uP[11].wwww).y;
// texld r0, r0, s8
r0=(texture(s8,(r0.xyzw).xy)).xyzw;
// add r0.yzw, -c12.xxyz, v1.xxyz
r0.yzw=(-uP[12].xxyz+v1.xxyz).yzw;
// dp3 r0.y, r0.yzww, r0.yzww
r0.y=(sm3Dp3(r0.yzww,r0.yzww)).y;
// mul r0.yz, r0.xyww, c11.xyxw
r0.yz=(r0.xyww*uP[11].xyxw).yz;
// mul r0.z, r0.z, c0.w
r0.z=(r0.zzzz*c0.wwww).z;
// exp r0.z, r0.z
r0.z=(exp2(r0.zzzz)).z;
// add r0.z, -r0.z, c1.w
r0.z=(-r0.zzzz+c1.wwww).z;
// mul r0.y, r0.z, r0.y
r0.y=(r0.zzzz*r0.yyyy).y;
// rcp r0.z, r0.w
r0.z=(vec4(1.0)/r0.wwww).z;
// mul r0.y, r0.z, r0.y
r0.y=(r0.zzzz*r0.yyyy).y;
// mul r0.y, r0.y, c0.w
r0.y=(r0.yyyy*c0.wwww).y;
// exp_sat r0.y, r0.y
r0.y=(clamp(exp2(r0.yyyy),0.0,1.0)).y;
// add r0.y, -r0.y, c1.w
r0.y=(-r0.yyyy+c1.wwww).y;
// mul r0.x, r0.y, r0.x
r0.x=(r0.yyyy*r0.xxxx).x;
// add r0.yzw, -r2.xxyz, c26.xxyz
r0.yzw=(-r2.xxyz+uP[26].xxyz).yzw;
// mad oC0.xyz, r0.x, r0.yzww, r2
oC0.xyz=(sm3Mad(r0.xxxx,r0.yzww,r2.xyzw)).xyz;
// mov oC0.w, r2.w
oC0.w=(r2.wwww).w;
outColor=oC0;
}
)GLSL";
constexpr std::string_view pixel4=R"GLSL(#version 460 core
in vec2 waterT0;in vec2 waterT1;in vec2 waterT2;in vec2 waterT3;in vec3 waterWorld;in vec3 waterClip;in vec4 waterT6;
uniform vec4 uP[30];layout(location=0) out vec4 outColor;
layout(binding=0) uniform sampler2D s0;
layout(binding=1) uniform sampler2D s1;
layout(binding=2) uniform sampler2D s2;
layout(binding=3) uniform sampler2D s3;
layout(binding=4) uniform sampler2D s4;
layout(binding=5) uniform sampler2D s5;
layout(binding=6) uniform sampler2D s6;
layout(binding=8) uniform sampler2D s8;
layout(binding=9) uniform sampler2D s9;
layout(binding=10) uniform sampler2D s10;
layout(binding=11) uniform sampler3D s11;
layout(binding=12) uniform sampler3D s12;
// Measured original D3D9: LEAN, river color and alpha interpolate authored mip samples in float.
vec4 riverSample(sampler2D s,vec2 uv){float last=float(textureQueryLevels(s)-1);float level=clamp(textureQueryLod(s,uv).y,0.0,last);float low=floor(level);vec4 first=textureLod(s,uv,low);vec4 second=textureLod(s,uv,min(low+1.0,last));precise vec4 result=first*(1.0-(level-low))+second*(level-low);return result;}

// Original HAL UNORM16: 8-bit spatial weights, unrounded bilinear values,
// separately rounded mip contributions, then a half-up sum.
vec4 riverUnormRound(dvec4 value){
    precise vec4 nearest=vec4(value);
    precise vec4 upper=uintBitsToFloat(floatBitsToUint(nearest)+uvec4(1u));
    precise dvec4 midpoint=(dvec4(nearest)+dvec4(upper))*0.5;
    return mix(nearest,upper,equal(value,midpoint));
}
dvec4 riverUnormTexel(sampler2D s,ivec2 p,int mip,ivec2 size){
    p=(p%size+size)%size;
    return dvec4(roundEven(texelFetch(s,p,mip)*65535.0));
}
dvec4 riverUnormBilinear(sampler2D s,vec2 uv,int mip){
    ivec2 size=textureSize(s,mip);
    precise vec2 position=uv*vec2(size)-0.5;
    ivec2 low=ivec2(floor(position));
    precise vec2 fraction=floor((position-vec2(low))*256.0+0.5)/256.0;
    precise dvec4 a=riverUnormTexel(s,low,mip,size);
    precise dvec4 b=riverUnormTexel(s,low+ivec2(1,0),mip,size);
    precise dvec4 c=riverUnormTexel(s,low+ivec2(0,1),mip,size);
    precise dvec4 d=riverUnormTexel(s,low+ivec2(1,1),mip,size);
    precise dvec4 top=a*(1.0-double(fraction.x))+b*double(fraction.x);
    precise dvec4 bottom=c*(1.0-double(fraction.x))+d*double(fraction.x);
    return top*(1.0-double(fraction.y))+bottom*double(fraction.y);
}
vec4 riverLeanSample(sampler2D s,vec2 uv){
    float last=float(textureQueryLevels(s)-1);
    float level=clamp(textureQueryLod(s,uv).y,0.0,last);
    int low=int(floor(level));
    precise double fraction=double(level-float(low));
    const float unorm=1.0/65535.0;
    precise dvec4 first=riverUnormBilinear(s,uv,low)*double(unorm)*(1.0-fraction);
    precise dvec4 second=riverUnormBilinear(s,uv,min(low+1,int(last)))*double(unorm)*fraction;
    precise vec4 a=riverUnormRound(first);
    precise vec4 b=riverUnormRound(second);
    return riverUnormRound(dvec4(a)+dvec4(b));
}


vec4 sm3Mad(vec4 a,vec4 b,vec4 c){precise dvec4 v=dvec4(a)*dvec4(b)+dvec4(c);return vec4(v);}
vec4 sm3Lrp(vec4 a,vec4 b,vec4 c){precise vec4 x=b-c;return sm3Mad(a,x,c);}
float sm3Log(float a){return a==0.0 ? -3.402823466e38 : log2(abs(a));}
vec4 sm3Log(vec4 a){return vec4(sm3Log(a.x),sm3Log(a.y),sm3Log(a.z),sm3Log(a.w));}
vec4 sm3Dp3(vec4 a,vec4 b){precise float v=a.x*b.x;v=float(double(a.y)*double(b.y)+double(v));v=float(double(a.z)*double(b.z)+double(v));return vec4(v);}

vec4 sm3Dp2Add(vec4 a,vec4 b,vec4 c){precise float v=a.x*b.x;v=float(double(a.y)*double(b.y)+double(v));v=v+c.x;return vec4(v);}
vec4 sm3Dp4(vec4 a,vec4 b){precise float v=a.x*b.x;v=float(double(a.y)*double(b.y)+double(v));v=float(double(a.z)*double(b.z)+double(v));v=float(double(a.w)*double(b.w)+double(v));return vec4(v);}


// Complete original HAL: sRGB decoding feeds U16 values to the mip filter.
// Retain bilinear precision; round each mip contribution and the sum half-up.
dvec4 riverColorBilinear(sampler2D s,vec2 uv,int mip){
    ivec2 size=textureSize(s,mip);
    precise vec2 position=uv*vec2(size)-0.5;
    ivec2 low=ivec2(floor(position));
    precise vec2 fraction=floor((position-vec2(low))*256.0+0.5)/256.0;
    precise dvec4 a=dvec4(roundEven(texelFetch(s,clamp(low,ivec2(0),size-1),mip)*65535.0));
    precise dvec4 b=dvec4(roundEven(texelFetch(s,clamp(low+ivec2(1,0),ivec2(0),size-1),mip)*65535.0));
    precise dvec4 c=dvec4(roundEven(texelFetch(s,clamp(low+ivec2(0,1),ivec2(0),size-1),mip)*65535.0));
    precise dvec4 d=dvec4(roundEven(texelFetch(s,clamp(low+ivec2(1,1),ivec2(0),size-1),mip)*65535.0));
    precise dvec4 top=a*(1.0-double(fraction.x))+b*double(fraction.x);
    precise dvec4 bottom=c*(1.0-double(fraction.x))+d*double(fraction.x);
    const float unorm=1.0/65535.0;
    return (top*(1.0-double(fraction.y))+bottom*double(fraction.y))*double(unorm);
}
vec4 riverColorSample(sampler2D s,vec2 uv){
    int last=textureQueryLevels(s)-1;
    float level=clamp(textureQueryLod(s,uv).y,0.0,float(last));
    int low=int(floor(level));
    precise double fraction=double(level-float(low));
    precise dvec4 first=riverColorBilinear(s,uv,low)*(1.0-fraction);
    precise dvec4 second=riverColorBilinear(s,uv,min(low+1,last))*fraction;
    precise vec4 a=riverUnormRound(first);
    precise vec4 b=riverUnormRound(second);
    return riverUnormRound(dvec4(a)+dvec4(b));
}

void main(){
const vec4 c0=vec4(-1.44269502,0.00499999989,0.5,-0.5);
const vec4 c1=vec4(6.0,2.0,-2.0,0.25);
const vec4 c2=vec4(2.20000005,0.800000012,0.454545468,-0.400000006);
const vec4 c3=vec4(-1.0,1.0,0.5,-16.0);
const vec4 c17=vec4(-1.0,-0.0,0.333000004,2.0);
vec4 v0=vec4(waterT0,0.0,0.0);
vec4 v1=vec4(waterT1,0.0,0.0);
vec4 v2=vec4(waterT2,0.0,0.0);
vec4 v3=vec4(waterT3,0.0,0.0);
vec4 v4=vec4(waterWorld,0.0);
vec4 v5=vec4(waterClip,0.0);
vec4 v6=waterT6;
precise vec4 r0;
precise vec4 r1;
precise vec4 r2;
precise vec4 r3;
precise vec4 r4;
precise vec4 r5;
precise vec4 r6;
precise vec4 r7;
precise vec4 r8;
precise vec4 oC0;
// texld r0, v6, s10
r0=(texture(s10,(v6.xyzw).xy)).xyzw;
// cmp r1, -r0_abs.x, c17.x, c17.y
r1=(mix(c17.yyyy,c17.xxxx,greaterThanEqual(-abs(r0.xxxx),vec4(0.0)))).xyzw;
// texkill r1
if(any(lessThan(r1.xyz,vec3(0.0))))discard;
// texld r1, v3, s6
r1=riverColorSample(s6,v3.xy);
// rcp r0.y, c4.x
r0.y=(vec4(1.0)/uP[4].xxxx).y;
// mul r0.y, r0.y, c17.z
r0.y=(r0.yyyy*c17.zzzz).y;
// mul r0.w, r0.y, r0.y
r0.w=(r0.yyyy*r0.yyyy).w;
// texld r2, v0, s0
r2=(riverLeanSample(s0,(v0.xyzw).xy)).xyzw;
// texld r3, v0, s1
r3=(riverLeanSample(s1,(v0.xyzw).xy)).xyzw;
// mad r2.xy, r2, c17.w, c17.x
r2.xy=(sm3Mad(r2.xyzw,c17.wwww,c17.xxxx)).xy;
// mul r3.yz, r0.y, r2.xxyw
r3.yz=(r0.yyyy*r2.xxyw).yz;
precise vec4 riverMeanFirst=r3.yzyz;
// mad r4.z, r3.x, c17.w, c17.x
r4.z=(sm3Mad(r3.xxxx,c17.wwww,c17.xxxx)).z;
// mov r4.xy, r2.zwzw
r4.xy=(r2.zwzw).xy;
// texld r5, v1, s2
r5=(riverLeanSample(s2,(v1.xyzw).xy)).xyzw;
// texld r6, v1, s3
r6=(riverLeanSample(s3,(v1.xyzw).xy)).xyzw;
// add r2.zw, r5.xyxy, r5.xyxy
r2.zw=(r5.xyxy+r5.xyxy).zw;
// mad r2.zw, r2, c3.xyxy, c3.xyyx
r2.zw=(sm3Mad(r2.xyzw,c3.xyxy,c3.xyyx)).zw;
// mul r2.zw, r0.y, r2
r2.zw=(r0.yyyy*r2.xyzw).zw;
precise vec4 riverMeanSecond=r2.zwzw;
// mad r2.xy, r2, r0.y, r2.zwzw
r2.xy=r3.yz+r2.zw;
// mad r6.z, r6.x, -c17_abs.w, c17_abs.x
r6.z=(sm3Mad(r6.xxxx,-abs(c17.wwww),abs(c17.xxxx))).z;
// mov r6.xy, r5.zwzw
r6.xy=(r5.zwzw).xy;
// mul r5.xyz, r0.w, r6
r5.xyz=(r0.wwww*r6.xyzw).xyz;
precise vec4 riverVarianceFirst=r4;
precise vec4 riverVarianceSecond=r6;
precise vec4 riverVarianceSum=riverVarianceFirst+riverVarianceSecond;
precise vec4 riverVarianceScale=r0.wwww;
// mad r4.xyz, r4, r0.w, r5
r4.xyz=(riverVarianceSum*riverVarianceScale).xyz;
// add r3.xw, r3.yyzz, r3.yyzz
r3.xw=(r3.yyzz+r3.yyzz).xw;
// mul r5.xy, r2.zwzw, r3.xwzw
r5.xy=(r2.zwzw*r3.xwzw).xy;
// dp2add r5.z, r3.yzzw, r2.wzzw, -c17.y
r5.z=(sm3Dp2Add(r3.yzzw,r2.wzzw,-c17.yyyy)).z;
// add r4.xyz, r4, r5
r4.xy=sm3Mad(riverVarianceSum,riverVarianceScale,r5).xy;
r4.z=sm3Mad(riverVarianceSecond,riverVarianceScale,
    sm3Mad(riverVarianceFirst,riverVarianceScale,r5)).z;
// texld r5, v2, s2
r5=(riverLeanSample(s2,(v2.xyzw).xy)).xyzw;
// texld r6, v2, s3
r6=(riverLeanSample(s3,(v2.xyzw).xy)).xyzw;
// mad r5.xy, r5.yxzw, c17.w, c17.x
r5.xy=(sm3Mad(r5.yxzw,c17.wwww,c17.xxxx)).xy;
// mul r6.yz, r0.y, r5.xxyw
r6.yz=(r0.yyyy*r5.xxyw).yz;
precise vec4 riverMeanThird=r6.zyzy;
// mad r7.xy, r5.yxzw, r0.y, r2
r7.xy=r6.zy+r2.xy;
// mad r8.z, r6.x, c17.w, c17.x
r8.z=(sm3Mad(r6.xxxx,c17.wwww,c17.xxxx)).z;
// mov r8.xy, r5.zwzw
r8.xy=(r5.zwzw).xy;
// mad r4.xyz, r8, r0.w, r4
r4.xyz=(sm3Mad(r8.xyzw,r0.wwww,r4.xyzw)).xyz;
// mul r5.xy, r3.xwzw, r6.zyzw
r5.xy=(r3.xwzw*r6.zyzw).xy;
// dp2add r5.z, r3.yzzw, r6.yzzw, -c17.y
r5.z=(sm3Dp2Add(r3.yzzw,r6.yzzw,-c17.yyyy)).z;
// add r3.xyz, r4, r5
precise vec4 riverMeanDouble=riverMeanFirst+riverMeanFirst;
r3.xy=sm3Mad(riverMeanDouble,riverMeanThird,r4).xy;
r3.z=sm3Mad(riverMeanFirst.xxxx,riverMeanThird.yyyy,
    sm3Mad(riverMeanFirst.yyyy,riverMeanThird.xxxx,r4.zzzz)).z;
// mul r0.yw, r2.xzzw, r6.xzzy
r0.yw=(r2.xzzw*r6.xzzy).yw;
// add r4.xy, r0.ywzw, r0.ywzw
r4.xy=(r0.ywzw+r0.ywzw).xy;
// dp2add r4.z, r2.zwzw, r6.yzzw, -c17.y
r4.z=(sm3Dp2Add(r2.zwzw,r6.yzzw,-c17.yyyy)).z;
// add r2.xyz, r3, r4
r2.xy=sm3Mad(riverMeanSecond,riverMeanThird,
    sm3Mad(riverMeanSecond,riverMeanThird,r3)).xy;
r2.z=sm3Mad(riverMeanSecond.xxxx,riverMeanThird.yyyy,
    sm3Mad(riverMeanSecond.yyyy,riverMeanThird.xxxx,r3.zzzz)).z;
// mad r2.xyz, r7.xyxw, -r7.xyyw, r2
r2.xyz=(sm3Mad(r7.xyxw,-r7.xyyw,r2.xyzw)).xyz;
// mov r7.z, -c17.x
r7.z=(-c17.xxxx).z;
// dp3_sat r0.y, c5, r7
precise vec4 riverDiffuse=sm3Mad(r7.xxxx,uP[5].xxxx,uP[5].zzzz);
r0.y=clamp(sm3Mad(r7.yyyy,uP[5].yyyy,riverDiffuse).x,0.0,1.0);
// add r3.xyz, c12, -v4
r3.xyz=(uP[12].xyzw+-v4.xyzw).xyz;
// dp3 r0.w, r3, r3
r0.w=(sm3Dp3(r3.xyzw,r3.xyzw)).w;
// rsq r0.w, r0.w
r0.w=(inversesqrt(abs(r0.wwww))).w;
// mul r4.xyz, r0.w, r3
r4.xyz=(r0.wwww*r3.xyzw).xyz;
precise float riverViewHeight=r3.z;
precise float riverViewInverse=r0.w;
// mad r5.xyz, r3, r0.w, -c9
r5.xyz=(sm3Mad(r3.xyzw,r0.wwww,-uP[9].xyzw)).xyz;
// mad r5.xyz, c9.w, r5, c9
r5.xyz=(sm3Mad(uP[9].wwww,r5.xyzw,uP[9].xyzw)).xyz;
// dp3 r1.w, r5, r5
r1.w=(sm3Dp3(r5.xyzw,r5.xyzw)).w;
// rsq r1.w, r1.w
r1.w=(inversesqrt(abs(r1.wwww))).w;
// mad r5.xyz, r5, r1.w, c5
r5.xyz=(sm3Mad(r5.xyzw,r1.wwww,uP[5].xyzw)).xyz;
// rcp r1.w, r5.z
r1.w=(vec4(1.0)/r5.zzzz).w;
// mad r5.xy, r5, r1.w, -r7
r5.xy=(sm3Mad(r5.xyzw,r1.wwww,-r7.xyzw)).xy;
// rcp r1.w, c7.w
r1.w=(vec4(1.0)/uP[7].wwww).w;
// add r5.zw, r1.w, r2.xyxy
r5.zw=(r1.wwww+r2.xyxy).zw;
// mul r2.w, r2.z, r2.z
r2.w=(r2.zzzz*r2.zzzz).w;
// mad r3.w, r5.z, r5.w, -r2.w
r3.w=(sm3Mad(r5.zzzz,r5.wwww,-r2.wwww)).w;
// mul r4.w, r5.x, r5.x
r4.w=(r5.xxxx*r5.xxxx).w;
// add r2.z, r2.z, r2.z
r2.z=(r2.zzzz+r2.zzzz).z;
// mul r5.x, r5.x, r2.z
r5.x=(r5.xxxx*r2.zzzz).x;
// mad r5.z, r5.z, r5.y, -r5.x
r5.z=(sm3Mad(r5.zzzz,r5.yyyy,-r5.xxxx)).z;
// mul r5.z, r5.y, r5.z
r5.z=(r5.yyyy*r5.zzzz).z;
// mad r5.z, r4.w, r5.w, r5.z
r5.z=(sm3Mad(r4.wwww,r5.wwww,r5.zzzz)).z;
// mul r5.z, r5.z, c3.z
r5.z=(r5.zzzz*c3.zzzz).z;
// rcp r5.w, r3.w
r5.w=(vec4(1.0)/r3.wwww).w;
// mul r6.x, r5.w, r5.z
r6.x=(r5.wwww*r5.zzzz).x;
// cmp r6.y, -r3.w, -c17.x, -c17.y
r6.y=(mix(-c17.yyyy,-c17.xxxx,greaterThanEqual(-r3.wwww,vec4(0.0)))).y;
// mad r5.z, r5.z, r5.w, c3.w
r5.z=(sm3Mad(r5.zzzz,r5.wwww,c3.wwww)).z;
// cmp r5.z, r5.z, -c17.x, -c17.y
r5.z=(mix(-c17.yyyy,-c17.xxxx,greaterThanEqual(r5.zzzz,vec4(0.0)))).z;
// add r5.z, r5.z, r6.y
r5.z=(r5.zzzz+r6.yyyy).z;
// mul r5.w, r6.x, c0.x
r5.w=(r6.xxxx*c0.xxxx).w;
// exp r5.w, r5.w
r5.w=(exp2(r5.wwww)).w;
// rsq r3.w, r3.w
r3.w=(inversesqrt(abs(r3.wwww))).w;
// mul r3.w, r3.w, r5.w
r3.w=(r3.wwww*r5.wwww).w;
// mad r6.xy, r1.w, -c3.w, r2
r6.xy=(sm3Mad(r1.wwww,-c3.wwww,r2.xyzw)).xy;
// mad r1.w, r6.x, r6.y, -r2.w
r1.w=(sm3Mad(r6.xxxx,r6.yyyy,-r2.wwww)).w;
// mad r5.x, r6.x, r5.y, -r5.x
r5.x=(sm3Mad(r6.xxxx,r5.yyyy,-r5.xxxx)).x;
// mul r5.x, r5.y, r5.x
r5.x=(r5.yyyy*r5.xxxx).x;
// mad r4.w, r4.w, r6.y, r5.x
r4.w=(sm3Mad(r4.wwww,r6.yyyy,r5.xxxx)).w;
// mul r4.w, r4.w, c3.z
r4.w=(r4.wwww*c3.zzzz).w;
// rcp r5.x, r1.w
r5.x=(vec4(1.0)/r1.wwww).x;
// mul r5.y, r4.w, r5.x
r5.y=(r4.wwww*r5.xxxx).y;
// cmp r5.w, -r1.w, -c17.x, -c17.y
r5.w=(mix(-c17.yyyy,-c17.xxxx,greaterThanEqual(-r1.wwww,vec4(0.0)))).w;
// mad r4.w, r4.w, r5.x, c3.w
r4.w=(sm3Mad(r4.wwww,r5.xxxx,c3.wwww)).w;
// cmp r4.w, r4.w, -c17.x, -c17.y
r4.w=(mix(-c17.yyyy,-c17.xxxx,greaterThanEqual(r4.wwww,vec4(0.0)))).w;
// add r4.w, r4.w, r5.w
r4.w=(r4.wwww+r5.wwww).w;
// mul r5.x, r5.y, c0.x
r5.x=(r5.yyyy*c0.xxxx).x;
// exp r5.x, r5.x
r5.x=(exp2(r5.xxxx)).x;
// rsq r1.w, r1.w
r1.w=(inversesqrt(abs(r1.wwww))).w;
// mul r1.w, r1.w, r5.x
r1.w=(r1.wwww*r5.xxxx).w;
// mul r1.w, r1.w, c0.y
r1.w=(r1.wwww*c0.yyyy).w;
// mad r3.xyz, r3, r0.w, -c10
r3.xyz=(sm3Mad(r3.xyzw,r0.wwww,-uP[10].xyzw)).xyz;
// mad r3.xyz, c10.w, r3, c10
r3.xyz=(sm3Mad(uP[10].wwww,r3.xyzw,uP[10].xyzw)).xyz;
// dp3 r0.w, r3, r3
r0.w=(sm3Dp3(r3.xyzw,r3.xyzw)).w;
// rsq r0.w, r0.w
r0.w=(inversesqrt(abs(r0.wwww))).w;
// mad r3.xyz, r3, r0.w, c6
r3.xyz=(sm3Mad(r3.xyzw,r0.wwww,uP[6].xyzw)).xyz;
// rcp r0.w, r3.z
r0.w=(vec4(1.0)/r3.zzzz).w;
// mad r3.xy, r3, r0.w, -r7
r3.xy=(sm3Mad(r3.xyzw,r0.wwww,-r7.xyzw)).xy;
// rcp r0.w, c8.w
r0.w=(vec4(1.0)/uP[8].wwww).w;
// add r5.xy, r0.w, r2
r5.xy=(r0.wwww+r2.xyzw).xy;
// mad r0.w, r5.x, r5.y, -r2.w
r0.w=(sm3Mad(r5.xxxx,r5.yyyy,-r2.wwww)).w;
// mul r2.w, r3.x, r3.x
r2.w=(r3.xxxx*r3.xxxx).w;
// mul r2.z, r2.z, r3.x
r2.z=(r2.zzzz*r3.xxxx).z;
// mad r2.z, r5.x, r3.y, -r2.z
r2.z=(sm3Mad(r5.xxxx,r3.yyyy,-r2.zzzz)).z;
// mul r2.z, r3.y, r2.z
r2.z=(r3.yyyy*r2.zzzz).z;
// mad r2.z, r2.w, r5.y, r2.z
r2.z=(sm3Mad(r2.wwww,r5.yyyy,r2.zzzz)).z;
// mul r2.z, r2.z, c3.z
r2.z=(r2.zzzz*c3.zzzz).z;
// rcp r2.w, r0.w
r2.w=(vec4(1.0)/r0.wwww).w;
// mul r3.x, r2.w, r2.z
r3.x=(r2.wwww*r2.zzzz).x;
// cmp r3.y, -r0.w, -c17.x, -c17.y
r3.y=(mix(-c17.yyyy,-c17.xxxx,greaterThanEqual(-r0.wwww,vec4(0.0)))).y;
// mad r2.z, r2.z, r2.w, c3.w
r2.z=(sm3Mad(r2.zzzz,r2.wwww,c3.wwww)).z;
// cmp r2.z, r2.z, -c17.x, -c17.y
r2.z=(mix(-c17.yyyy,-c17.xxxx,greaterThanEqual(r2.zzzz,vec4(0.0)))).z;
// add r2.z, r2.z, r3.y
r2.z=(r2.zzzz+r3.yyyy).z;
// mul r2.w, r3.x, c0.x
r2.w=(r3.xxxx*c0.xxxx).w;
// exp r2.w, r2.w
r2.w=(exp2(r2.wwww)).w;
precise float riverSkyIntensity=r2.w*c2.y;
// rsq r3.x, r0.w
r3.x=(inversesqrt(abs(r0.wwww))).x;
// mul r2.w, r2.w, r3.x
r2.w=(r2.wwww*r3.xxxx).w;
riverSkyIntensity=riverSkyIntensity*r3.x;
// mul r3.xyz, c14.xyww, v4.y
r3.xyz=(uP[14].xyww*v4.yyyy).xyz;
// mad r3.xyz, v4.x, c13.xyww, r3
r3.xyz=(sm3Mad(v4.xxxx,uP[13].xyww,r3.xyzw)).xyz;
// mad r3.xyz, v4.z, c15.xyww, r3
r3.xyz=(sm3Mad(v4.zzzz,uP[15].xyww,r3.xyzw)).xyz;
// add r3.xyz, r3, c16.xyww
r3.xyz=(r3.xyzw+uP[16].xyww).xyz;
precise vec4 riverProjection=sm3Mad(v4.xxxx,uP[13],uP[16]);
riverProjection=sm3Mad(v4.zzzz,uP[15],riverProjection);
riverProjection=sm3Mad(v4.yyyy,uP[14],riverProjection);
r3.xyz=riverProjection.xyw;
// rcp r3.z, r3.z
r3.z=(vec4(1.0)/r3.zzzz).z;
// mul r3.xy, r3.z, r3
r3.xy=(r3.zzzz*r3.xyzw).xy;
// mad r3.xy, r3, c0.zwzw, c0.w
r3.xy=(sm3Mad(r3.xyzw,c0.zwzw,c0.wwww)).xy;
// mad r2.xy, r2, c1.x, r3
r2.xy=(sm3Mad(r2.xyzw,c1.xxxx,r3.xyzw)).xy;
// mad r3.xy, r0.w, c17.w, r2
r3.xy=(sm3Mad(r0.wwww,c17.wwww,r2.xyzw)).xy;
// texld r6, r3, s4
r6=(texture(s4,(r3.xyzw).xy)).xyzw;
// mad r3.xy, r0.w, c1.yzzw, r2
r3.xy=(sm3Mad(r0.wwww,c1.yzzw,r2.xyzw)).xy;
// texld r8, r3, s4
r8=(texture(s4,(r3.xyzw).xy)).xyzw;
// mul r8, r8, c1.w
r8=(r8.xyzw*c1.wwww).xyzw;
// mad r6, r6, c1.w, r8
r6=(sm3Mad(r6.xyzw,c1.wwww,r8.xyzw)).xyzw;
// mad r3.xy, r0.w, c1.zxzw, r2
r3.xy=(sm3Mad(r0.wwww,c1.zxzw,r2.xyzw)).xy;
// texld r8, r3, s4
r8=(texture(s4,(r3.xyzw).xy)).xyzw;
// mad r6, r8, c1.w, r6
r6=(sm3Mad(r8.xyzw,c1.wwww,r6.xyzw)).xyzw;
// mad r2.xy, r0.w, -c17.w, r2
r2.xy=(sm3Mad(r0.wwww,-c17.wwww,r2.xyzw)).xy;
// texld r8, r2, s4
r8=(texture(s4,(r2.xyzw).xy)).xyzw;
// mad r6, r8, c1.w, r6
r6=(sm3Mad(r8.xyzw,c1.wwww,r6.xyzw)).xyzw;
// log r3.x, r6.x
r3.x=(sm3Log(r6.xxxx)).x;
// log r3.y, r6.y
r3.y=(sm3Log(r6.yyyy)).y;
// log r3.z, r6.z
r3.z=(sm3Log(r6.zzzz)).z;
// mul r3.xyz, r3, c2.x
r3.xyz=(r3.xyzw*c2.xxxx).xyz;
// exp r6.x, r3.x
r6.x=(exp2(r3.xxxx)).x;
// exp r6.y, r3.y
r6.y=(exp2(r3.yyyy)).y;
// exp r6.z, r3.z
r6.z=(exp2(r3.zzzz)).z;
// dp3 r0.w, r7, r4
r0.w=sm3Mad(vec4(riverViewHeight),vec4(riverViewInverse),sm3Mad(r7.yyyy,r4.yyyy,r7.xxxx*r4.xxxx)).x;
// add r0.w, -r0.w, -c17.x
r0.w=(-r0.wwww+-c17.xxxx).w;
// mul r2.x, r0.w, r0.w
r2.x=(r0.wwww*r0.wwww).x;
// mul r2.x, r2.x, r2.x
r2.x=(r2.xxxx*r2.xxxx).x;
// mul r2.y, r0.w, r2.x
r2.y=(r0.wwww*r2.xxxx).y;
// mul r3.xyz, r3.w, c7
r3.xyz=(r3.wwww*uP[7].xyzw).xyz;
// cmp r3.xyz, -r5.z, r3, -c17.y
r3.xyz=(mix(-c17.yyyy,r3.xyzw,greaterThanEqual(-r5.zzzz,vec4(0.0)))).xyz;
// mul r4.xyz, r2.w, c8
r4.xyz=(r2.wwww*uP[8].xyzw).xyz;
// mul r4.xyz, r4, c2.y
r4.xyz=riverSkyIntensity*uP[8].xyz;
// cmp r4.xyz, -r2.z, r4, -c17.y
r4.xyz=(mix(-c17.yyyy,r4.xyzw,greaterThanEqual(-r2.zzzz,vec4(0.0)))).xyz;
// add r3.xyz, r3, r4
r3.xyz=(r3.xyzw+r4.xyzw).xyz;
// mad r2.yzw, r3.xxyz, r2.y, r1.xxyz
r2.yzw=(sm3Mad(r3.xxyz,r2.yyyy,r1.xxyz)).yzw;
// mul r1.xyz, r1, c7
r1.xyz=(r1.xyzw*uP[7].xyzw).xyz;
// mul r1.xyz, r1.w, r1
r1.xyz=(r1.wwww*r1.xyzw).xyz;
// cmp r1.xyz, -r4.w, r1, -c17.y
r1.xyz=(mix(-c17.yyyy,r1.xyzw,greaterThanEqual(-r4.wwww,vec4(0.0)))).xyz;
// mad r1.xyz, r2.yzww, r0.y, r1
r1.xyz=(sm3Mad(r2.yzww,r0.yyyy,r1.xyzw)).xyz;
// mad r1.xyz, r6, c27.x, r1
r1.xyz=(sm3Mad(r6.xyzw,uP[27].xxxx,r1.xyzw)).xyz;
// texld r3, v3, s5
r3=(riverSample(s5,(v3.xyzw).xy)).xyzw;
// mad r0.y, r0.w, r2.x, c3.z
r0.y=(sm3Mad(r0.wwww,r2.xxxx,c3.zzzz)).y;
// mul r1.xyz, r0.y, r1
r1.xyz=(r0.yyyy*r1.xyzw).xyz;
// add_sat r0.y, r6.w, -c17.x
r0.y=(clamp(r6.wwww+-c17.xxxx,0.0,1.0)).y;
// mul oC0.w, r3.w, r0.y
oC0.w=(r3.wwww*r0.yyyy).w;
// log r2.x, r1.x
r2.x=(sm3Log(r1.xxxx)).x;
// log r2.y, r1.y
r2.y=(sm3Log(r1.yyyy)).y;
// log r2.z, r1.z
r2.z=(sm3Log(r1.zzzz)).z;
// mul r1.xyz, r2, c2.z
r1.xyz=(r2.xyzw*c2.zzzz).xyz;
// exp r2.x, r1.x
r2.x=(exp2(r1.xxxx)).x;
// exp r2.y, r1.y
r2.y=(exp2(r1.yyyy)).y;
// exp r2.z, r1.z
r2.z=(exp2(r1.zzzz)).z;
// texld r1, r2, s12
r1=(texture(s12,(r2.xyzw).xyz)).xyzw;
// texld r2, v6, s9
r2=(texture(s9,(v6.xyzw).xy)).xyzw;
// add r0.y, -r2.w, -c17.x
r0.y=(-r2.wwww+-c17.xxxx).y;
// mad r1.xyz, r1, r0.y, r2
r1.xyz=(sm3Mad(r1.xyzw,r0.yyyy,r2.xyzw)).xyz;
// lrp r2.xyz, r0.x, r1, -c2.w
r2.xyz=(sm3Lrp(r0.xxxx,r1.xyzw,-c2.wwww)).xyz;
// texld r1, r2, s11
r1=(texture(s11,(r2.xyzw).xyz)).xyzw;
// mad r0.x, r0.z, -r0.x, r0.x
r0.x=(sm3Mad(r0.zzzz,-r0.xxxx,r0.xxxx)).x;
// lrp r3.xyz, r0.x, r1, r2
r3.xyz=(sm3Lrp(r0.xxxx,r1.xyzw,r2.xyzw)).xyz;
// rcp r0.x, v5.z
r0.x=(vec4(1.0)/v5.zzzz).x;
// mul r0.x, r0.x, v5.y
r0.x=(r0.xxxx*v5.yyyy).x;
// mad r0.x, r0.x, c3.z, c3.z
r0.x=(sm3Mad(r0.xxxx,c3.zzzz,c3.zzzz)).x;
// mov r0.y, c11.w
r0.y=(uP[11].wwww).y;
// texld r0, r0, s8
r0=(texture(s8,(r0.xyzw).xy)).xyzw;
// add r0.yzw, -c12.xxyz, v4.xxyz
r0.yzw=(-uP[12].xxyz+v4.xxyz).yzw;
// dp3 r0.y, r0.yzww, r0.yzww
r0.y=(sm3Dp3(r0.yzww,r0.yzww)).y;
// mul r0.yz, r0.xyww, c11.xyxw
r0.yz=(r0.xyww*uP[11].xyxw).yz;
// mul r0.z, r0.z, -c0.x
r0.z=(r0.zzzz*-c0.xxxx).z;
// exp r0.z, r0.z
r0.z=(exp2(r0.zzzz)).z;
// add r0.z, -r0.z, -c17.x
r0.z=(-r0.zzzz+-c17.xxxx).z;
// mul r0.y, r0.z, r0.y
r0.y=(r0.zzzz*r0.yyyy).y;
// rcp r0.z, r0.w
r0.z=(vec4(1.0)/r0.wwww).z;
// mul r0.y, r0.z, r0.y
r0.y=(r0.zzzz*r0.yyyy).y;
// mul r0.y, r0.y, -c0.x
r0.y=(r0.yyyy*-c0.xxxx).y;
// exp_sat r0.y, r0.y
r0.y=(clamp(exp2(r0.yyyy),0.0,1.0)).y;
// add r0.y, -r0.y, -c17.x
r0.y=(-r0.yyyy+-c17.xxxx).y;
// mul r0.x, r0.y, r0.x
r0.x=(r0.yyyy*r0.xxxx).x;
// add r0.yzw, -r3.xxyz, c26.xxyz
r0.yzw=(-r3.xxyz+uP[26].xxyz).yzw;
// mad oC0.xyz, r0.x, r0.yzww, r3
oC0.xyz=(sm3Mad(r0.xxxx,r0.yzww,r3.xyzw)).xyz;
outColor=oC0;
}
)GLSL";
}
std::string_view river_surface_vertex_shader(){return vertex;}
std::string_view river_surface_fragment_shader(unsigned selector){
    switch(selector){case 1:return pixel1;case 4:return pixel4;}
    throw std::invalid_argument("Invalid river surface shader selector");
}
} // namespace civ5::graphics
