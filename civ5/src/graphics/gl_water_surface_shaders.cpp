#include "civ5/graphics/gl_water_surface.hpp"
#include <stdexcept>
namespace civ5::graphics {
namespace {
constexpr std::string_view vertex=R"GLSL(#version 460 core
out vec2 waterT0;out vec2 waterT1;out vec2 waterT2;out vec2 waterT3;out vec3 waterWorld;out vec3 waterClip;out vec4 waterT6;
layout(location=0) in vec4 aPosition;uniform vec4 uV[17];out vec4 tfClip;

vec4 sm3Mad(vec4 a,vec4 b,vec4 c){precise dvec4 v=dvec4(a)*dvec4(b)+dvec4(c);return vec4(v);}
vec4 sm3Lrp(vec4 a,vec4 b,vec4 c){precise vec4 x=b-c;return sm3Mad(a,x,c);}
float sm3Log(float a){return a==0.0 ? -3.402823466e38 : log2(abs(a));}
vec4 sm3Log(vec4 a){return vec4(sm3Log(a.x),sm3Log(a.y),sm3Log(a.z),sm3Log(a.w));}
vec4 sm3Dp3(vec4 a,vec4 b){precise float v=a.x*b.x;v=float(double(a.y)*double(b.y)+double(v));v=float(double(a.z)*double(b.z)+double(v));return vec4(v);}

vec4 sm3Dp2Add(vec4 a,vec4 b,vec4 c){precise float v=a.x*b.x;v=float(double(a.y)*double(b.y)+double(v));v=v+c.x;return vec4(v);}
vec4 sm3Dp4(vec4 a,vec4 b){precise float v=a.x*b.x;v=float(double(a.y)*double(b.y)+double(v));v=float(double(a.z)*double(b.z)+double(v));v=float(double(a.w)*double(b.w)+double(v));return vec4(v);}

void main(){
const vec4 c0=vec4(1.0,0.0,-1.0,0.0);
const vec4 c1=vec4(0.5,-0.5,0.0,0.0);
vec4 v0=aPosition;
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
// mov r0, c13
r0=(uV[13].xyzw).xyzw;
// mad o1.xy, v0, r0.x, c15.x
o1.xy=(sm3Mad(v0.xyzw,r0.xxxx,uV[15].xxxx)).xy;
// mad r1, r0.y, c0.xyyx, c0.yxzy
r1=(sm3Mad(r0.yyyy,c0.xyyx,c0.yxzy)).xyzw;
// mul r1.xy, r1, v0
r1.xy=(r1.xyzw*v0.xyzw).xy;
// mad o2.xy, r1, r1.zwzw, c15.x
o2.xy=(sm3Mad(r1.xyzw,r1.zwzw,uV[15].xxxx)).xy;
// mad o3.xy, v0, -r0.z, c15.x
o3.xy=(sm3Mad(v0.xyzw,-r0.zzzz,uV[15].xxxx)).xy;
// mad r0, r0.ywwy, c0.yxxy, c0.xyyz
r0=(sm3Mad(r0.ywwy,c0.yxxy,c0.xyyz)).xyzw;
// mul r0.xy, r0, v0
r0.xy=(r0.xyzw*v0.xyzw).xy;
// mad o4.xy, r0, r0.zwzw, c15.x
o4.xy=(sm3Mad(r0.xyzw,r0.zwzw,uV[15].xxxx)).xy;
// mul r0.xy, c9, v0.y
r0.xy=(uV[9].xyzw*v0.yyyy).xy;
// mad r0.xy, v0.x, c8, r0
r0.xy=(sm3Mad(v0.xxxx,uV[8].xyzw,r0.xyzw)).xy;
// mov r1.x, c16.x
r1.x=(uV[16].xxxx).x;
// mad r0.xy, r1.x, c10, r0
r0.xy=(sm3Mad(r1.xxxx,uV[10].xyzw,r0.xyzw)).xy;
// add r0.xy, r0, c11
r0.xy=(r0.xyzw+uV[11].xyzw).xy;
// mad o7.xy, r0, c1, c1.y
o7.xy=(sm3Mad(r0.xyzw,c1.xyzw,c1.yyyy)).xy;
// mul r0, c5, v0.y
r0=(uV[5].xyzw*v0.yyyy).xyzw;
// mad r0, v0.x, c4, r0
r0=(sm3Mad(v0.xxxx,uV[4].xyzw,r0.xyzw)).xyzw;
// mad r0, r1.x, c6, r0
r0=(sm3Mad(r1.xxxx,uV[6].xyzw,r0.xyzw)).xyzw;
// add r0, r0, c7
r0=(r0.xyzw+uV[7].xyzw).xyzw;
// mov o0, r0
o0=(r0.xyzw).xyzw;
// mov o6.xyz, r0.xyww
o6.xyz=(r0.xyww).xyz;
// mov o5.xy, v0
o5.xy=(v0.xyzw).xy;
// mov o5.z, c16.x
o5.z=(uV[16].xxxx).z;
// mov o7.zw, v0
o7.zw=(v0.xyzw).zw;
waterT0=o1.xy;waterT1=o2.xy;waterT2=o3.xy;waterT3=o4.xy;waterWorld=o5.xyz;waterClip=o6.xyz;waterT6=o7;tfClip=o0;
// API Y conversion; draw supplies native depth and pixel-center viewport.
gl_Position=vec4(o0.x,-o0.y,o0.z,o0.w);
}
)GLSL";
constexpr std::string_view pixel0=R"GLSL(#version 460 core
in vec2 waterT0;in vec2 waterT1;in vec2 waterT2;in vec2 waterT3;in vec3 waterWorld;in vec3 waterClip;in vec4 waterT6;
uniform vec4 uP[30];layout(location=0) out vec4 outColor;
layout(binding=0) uniform sampler2D s0;
layout(binding=5) uniform sampler2D s5;
layout(binding=6) uniform sampler2D s6;
layout(binding=7) uniform sampler2D s7;
layout(binding=8) uniform sampler2D s8;
layout(binding=9) uniform sampler2D s9;
layout(binding=10) uniform sampler2D s10;
layout(binding=11) uniform sampler3D s11;
layout(binding=13) uniform sampler2D s13;

vec4 sm3Mad(vec4 a,vec4 b,vec4 c){precise dvec4 v=dvec4(a)*dvec4(b)+dvec4(c);return vec4(v);}
vec4 sm3Lrp(vec4 a,vec4 b,vec4 c){precise vec4 x=b-c;return sm3Mad(a,x,c);}
float sm3Log(float a){return a==0.0 ? -3.402823466e38 : log2(abs(a));}
vec4 sm3Log(vec4 a){return vec4(sm3Log(a.x),sm3Log(a.y),sm3Log(a.z),sm3Log(a.w));}
vec4 sm3Dp3(vec4 a,vec4 b){precise float v=a.x*b.x;v=float(double(a.y)*double(b.y)+double(v));v=float(double(a.z)*double(b.z)+double(v));return vec4(v);}

vec4 sm3Dp2Add(vec4 a,vec4 b,vec4 c){precise float v=a.x*b.x;v=float(double(a.y)*double(b.y)+double(v));v=v+c.x;return vec4(v);}
vec4 sm3Dp4(vec4 a,vec4 b){precise float v=a.x*b.x;v=float(double(a.y)*double(b.y)+double(v));v=float(double(a.z)*double(b.z)+double(v));v=float(double(a.w)*double(b.w)+double(v));return vec4(v);}

void main(){
const vec4 c0=vec4(-1.0,-0.0,4.0,1.0);
const vec4 c1=vec4(2.0,-1.0,0.5,-0.5);
const vec4 c2=vec4(-0.670000017,0.454545468,-0.400000006,0.400000006);
const vec4 c3=vec4(1.44269502,0.0,0.0,0.0);
vec4 v0=vec4(waterT0,0.0,0.0);
vec4 v1=vec4(waterT1,0.0,0.0);
vec4 v2=vec4(waterWorld,0.0);
vec4 v3=vec4(waterClip,0.0);
vec4 v4=waterT6;
precise vec4 r0;
precise vec4 r1;
precise vec4 r2;
precise vec4 r3;
precise vec4 oC0;
// texld r0, v4, s10
r0=(texture(s10,(v4.xyzw).xy)).xyzw;
// cmp r1, -r0_abs.x, c0.x, c0.y
r1=(mix(c0.yyyy,c0.xxxx,greaterThanEqual(-abs(r0.xxxx),vec4(0.0)))).xyzw;
// texkill r1
if(any(lessThan(r1.xyz,vec3(0.0))))discard;
// mul r0.yw, c22.xxzy, v2.y
r0.yw=(uP[22].xxzy*v2.yyyy).yw;
// mad r0.yw, v2.x, c21.xxzy, r0
r0.yw=(sm3Mad(v2.xxxx,uP[21].xxzy,r0.xyzw)).yw;
// mad r0.yw, v2.z, c23.xxzy, r0
r0.yw=(sm3Mad(v2.zzzz,uP[23].xxzy,r0.xyzw)).yw;
// add r0.yw, r0, c24.xxzy
r0.yw=(r0.xyzw+uP[24].xxzy).yw;
#ifndef CIV5_LITERAL_WATER_CURRENT_AFFINE
// Measured original HAL: T -> FMA(X) -> FMA(Z) -> FMA(Y).
precise vec4 waterProjected=sm3Mad(v2.xxxx,uP[21],uP[24]);
waterProjected=sm3Mad(v2.zzzz,uP[23],waterProjected);
waterProjected=sm3Mad(v2.yyyy,uP[22],waterProjected);
r0.yw=waterProjected.xy;
#endif
// texld r1, v4.zwzw, s7
r1=(texture(s7,(v4.zwzw).xy)).xyzw;
// mul_sat r1.x, r1.w, c0.z
r1.x=(clamp(r1.wwww*c0.zzzz,0.0,1.0)).x;
// add r1.x, -r1.x, c0.w
r1.x=(-r1.xxxx+c0.wwww).x;
// mov r1.yzw, -c0.y
r1.yzw=(-c0.yyyy).yzw;
// texldl r1, r1, s6
r1=(textureLod(s6,(r1.xyzw).xy ,(r1.xyzw).w)).xyzw;
// texld r2, r0.ywzw, s5
r2=(texture(s5,(r0.ywzw).xy)).xyzw;
precise float waterWeightSum=r2.x+r2.y;
precise float waterWeightY=r2.y;
// mad r0.yw, r2.xxzy, c1.x, c1.y
r0.yw=(sm3Mad(r2.xxzy,c1.xxxx,c1.yyyy)).yw;
// dp2add r2.x, -c0.xwzw, r0.ywzw, -c0.y
r2.x=(sm3Dp2Add(-c0.xwzw,r0.ywzw,-c0.yyyy)).x;
// dp2add r2.y, -c0.w, r0.ywzw, -c0.y
r2.y=2.0-2.0*waterWeightSum;
// add r0.yw, r2.xxzy, c1.x
r0.yw=(r2.xxzy+c1.xxxx).yw;
// dp2add r2.x, -c0.x, r0.ywzw, -c0.y
r2.x=6.0-4.0*waterWeightY;
// rcp r2.x, r2.x
r2.x=(vec4(1.0)/r2.xxxx).x;
// mul r0.yw, r0, r2.x
r0.yw=(r0.xyzw*r2.xxxx).yw;
// rcp r2.x, c4.x
r2.x=(vec4(1.0)/uP[4].xxxx).x;
// mul r0.yw, r0, r2.x
r0.yw=(r0.xyzw*r2.xxxx).yw;
// texld r2, v0, s0
r2=(texture(s0,(v0.xyzw).xy)).xyzw;
// mad r2.xy, r2, c1.x, c1.y
r2.xy=(sm3Mad(r2.xyzw,c1.xxxx,c1.yyyy)).xy;
// texld r3, v1, s0
r3=(texture(s0,(v1.xyzw).xy)).xyzw;
// add r2.zw, r3.xyxy, r3.xyxy
r2.zw=(r3.xyxy+r3.xyxy).zw;
// mad r2.zw, r2, c0.xyxw, c0.xywx
r2.zw=(sm3Mad(r2.xyzw,c0.xyxw,c0.xywx)).zw;
// mul r2.zw, r0.w, r2
r2.zw=(r0.wwww*r2.xyzw).zw;
// mad r2.xy, r2, r0.y, r2.zwzw
r2.xy=(sm3Mad(r2.xyzw,r0.yyyy,r2.zwzw)).xy;
// mov r2.z, c0.w
r2.z=(c0.wwww).z;
// dp3_sat r0.y, c5, r2
precise vec4 waterDiffuse=sm3Mad(r2.xxxx,uP[5].xxxx,uP[5].zzzz);
r0.y=clamp(sm3Mad(r2.yyyy,uP[5].yyyy,waterDiffuse).x,0.0,1.0);
// mul r1.xyz, r0.y, r1
r1.xyz=(r0.yyyy*r1.xyzw).xyz;
// mul r2.xyz, c18.xyww, v2.y
r2.xyz=(uP[18].xyww*v2.yyyy).xyz;
// mad r2.xyz, v2.x, c17.xyww, r2
r2.xyz=(sm3Mad(v2.xxxx,uP[17].xyww,r2.xyzw)).xyz;
// mad r2.xyz, v2.z, c19.xyww, r2
r2.xyz=(sm3Mad(v2.zzzz,uP[19].xyww,r2.xyzw)).xyz;
// add r2.xyz, r2, c20.xyww
r2.xyz=(r2.xyzw+uP[20].xyww).xyz;
// rcp r0.y, r2.z
r0.y=(vec4(1.0)/r2.zzzz).y;
// mul r0.yw, r0.y, r2.xxzy
r0.yw=(r0.yyyy*r2.xxzy).yw;
// mad r0.yw, r0, c1.xzzw, c1.w
r0.yw=(sm3Mad(r0.xyzw,c1.xzzw,c1.wwww)).yw;
// texld r2, r0.ywzw, s13
r2=(texture(s13,(r0.ywzw).xy)).xyzw;
// mov_sat r2.x, r2.x
r2.x=(clamp(r2.xxxx,0.0,1.0)).x;
// mul r0.y, r2.x, c25.x
r0.y=(r2.xxxx*uP[25].xxxx).y;
// mul r2.xyz, r0.y, r1
r2.xyz=(r0.yyyy*r1.xyzw).xyz;
// mad r1.xyz, r2, c2.x, r1
r1.xyz=(sm3Mad(r2.xyzw,c2.xxxx,r1.xyzw)).xyz;
// mov_sat oC0.w, r1.w
oC0.w=(clamp(r1.wwww,0.0,1.0)).w;
// log r2.x, r1.x
r2.x=(sm3Log(r1.xxxx)).x;
// log r2.y, r1.y
r2.y=(sm3Log(r1.yyyy)).y;
// log r2.z, r1.z
r2.z=(sm3Log(r1.zzzz)).z;
// mul r1.xyz, r2, c2.y
r1.xyz=(r2.xyzw*c2.yyyy).xyz;
// exp r2.x, r1.x
r2.x=(exp2(r1.xxxx)).x;
// exp r2.y, r1.y
r2.y=(exp2(r1.yyyy)).y;
// exp r2.z, r1.z
r2.z=(exp2(r1.zzzz)).z;
// texld r1, v4, s9
r1=(texture(s9,(v4.xyzw).xy)).xyzw;
// add r0.y, -r1.w, c0.w
r0.y=(-r1.wwww+c0.wwww).y;
// mad r1.xyz, r2, r0.y, r1
r1.xyz=(sm3Mad(r2.xyzw,r0.yyyy,r1.xyzw)).xyz;
// add r1.xyz, r1, c2.z
r1.xyz=(r1.xyzw+c2.zzzz).xyz;
// mad r1.xyz, r0.x, r1, c2.w
r1.xyz=(sm3Mad(r0.xxxx,r1.xyzw,c2.wwww)).xyz;
// texld r2, r1, s11
r2=(texture(s11,(r1.xyzw).xyz)).xyzw;
// mad r0.x, r0.z, -r0.x, r0.x
r0.x=(sm3Mad(r0.zzzz,-r0.xxxx,r0.xxxx)).x;
// lrp r3.xyz, r0.x, r2, r1
r3.xyz=(sm3Lrp(r0.xxxx,r2.xyzw,r1.xyzw)).xyz;
// rcp r0.x, v3.z
r0.x=(vec4(1.0)/v3.zzzz).x;
// mul r0.x, r0.x, v3.y
r0.x=(r0.xxxx*v3.yyyy).x;
// mad r0.x, r0.x, c1.z, c1.z
r0.x=(sm3Mad(r0.xxxx,c1.zzzz,c1.zzzz)).x;
// mov r0.y, c11.w
r0.y=(uP[11].wwww).y;
// texld r0, r0, s8
r0=(texture(s8,(r0.xyzw).xy)).xyzw;
// add r0.yzw, -c12.xxyz, v2.xxyz
r0.yzw=(-uP[12].xxyz+v2.xxyz).yzw;
// dp3 r0.y, r0.yzww, r0.yzww
r0.y=(sm3Dp3(r0.yzww,r0.yzww)).y;
// mul r0.yz, r0.xyww, c11.xyxw
r0.yz=(r0.xyww*uP[11].xyxw).yz;
// mul r0.z, r0.z, c3.x
r0.z=(r0.zzzz*c3.xxxx).z;
// exp r0.z, r0.z
r0.z=(exp2(r0.zzzz)).z;
// add r0.z, -r0.z, c0.w
r0.z=(-r0.zzzz+c0.wwww).z;
// mul r0.y, r0.z, r0.y
r0.y=(r0.zzzz*r0.yyyy).y;
// rcp r0.z, r0.w
r0.z=(vec4(1.0)/r0.wwww).z;
// mul r0.y, r0.z, r0.y
r0.y=(r0.zzzz*r0.yyyy).y;
// mul r0.y, r0.y, c3.x
r0.y=(r0.yyyy*c3.xxxx).y;
// exp_sat r0.y, r0.y
r0.y=(clamp(exp2(r0.yyyy),0.0,1.0)).y;
// add r0.y, -r0.y, c0.w
r0.y=(-r0.yyyy+c0.wwww).y;
// mul r0.x, r0.y, r0.x
r0.x=(r0.yyyy*r0.xxxx).x;
// add r0.yzw, -r3.xxyz, c26.xxyz
r0.yzw=(-r3.xxyz+uP[26].xxyz).yzw;
// mad oC0.xyz, r0.x, r0.yzww, r3
oC0.xyz=(sm3Mad(r0.xxxx,r0.yzww,r3.xyzw)).xyz;
outColor=oC0;
}
)GLSL";
constexpr std::string_view pixel2=R"GLSL(#version 460 core
in vec2 waterT0;in vec2 waterT1;in vec2 waterT2;in vec2 waterT3;in vec3 waterWorld;in vec3 waterClip;in vec4 waterT6;
uniform vec4 uP[30];layout(location=0) out vec4 outColor;
layout(binding=0) uniform sampler2D s0;
layout(binding=1) uniform sampler2D s1;
layout(binding=4) uniform sampler2D s4;
layout(binding=5) uniform sampler2D s5;
layout(binding=6) uniform sampler2D s6;
layout(binding=7) uniform sampler2D s7;
layout(binding=8) uniform sampler2D s8;
layout(binding=9) uniform sampler2D s9;
layout(binding=10) uniform sampler2D s10;
layout(binding=11) uniform sampler3D s11;
layout(binding=13) uniform sampler2D s13;

vec4 sm3Mad(vec4 a,vec4 b,vec4 c){precise dvec4 v=dvec4(a)*dvec4(b)+dvec4(c);return vec4(v);}
vec4 sm3Lrp(vec4 a,vec4 b,vec4 c){precise vec4 x=b-c;return sm3Mad(a,x,c);}
float sm3Log(float a){return a==0.0 ? -3.402823466e38 : log2(abs(a));}
vec4 sm3Log(vec4 a){return vec4(sm3Log(a.x),sm3Log(a.y),sm3Log(a.z),sm3Log(a.w));}
vec4 sm3Dp3(vec4 a,vec4 b){precise float v=a.x*b.x;v=float(double(a.y)*double(b.y)+double(v));v=float(double(a.z)*double(b.z)+double(v));return vec4(v);}

vec4 sm3Dp2Add(vec4 a,vec4 b,vec4 c){precise float v=a.x*b.x;v=float(double(a.y)*double(b.y)+double(v));v=v+c.x;return vec4(v);}
vec4 sm3Dp4(vec4 a,vec4 b){precise float v=a.x*b.x;v=float(double(a.y)*double(b.y)+double(v));v=float(double(a.z)*double(b.z)+double(v));v=float(double(a.w)*double(b.w)+double(v));return vec4(v);}

void main(){
const vec4 c0=vec4(-1.0,-0.0,4.0,1.0);
const vec4 c1=vec4(2.0,-1.0,1.0,0.5);
const vec4 c2=vec4(0.5,-0.5,6.0,0.25);
const vec4 c3=vec4(2.0,-2.0,6.0,2.20000005);
const vec4 c28=vec4(-0.670000017,0.454545468,-0.400000006,0.400000006);
const vec4 c29=vec4(-16.0,-1.44269502,16.0,0.00499999989);
vec4 v0=vec4(waterT0,0.0,0.0);
vec4 v1=vec4(waterT1,0.0,0.0);
vec4 v2=vec4(waterWorld,0.0);
vec4 v3=vec4(waterClip,0.0);
vec4 v4=waterT6;
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
// texld r0, v4, s10
r0=(texture(s10,(v4.xyzw).xy)).xyzw;
// cmp r1, -r0_abs.x, c0.x, c0.y
r1=(mix(c0.yyyy,c0.xxxx,greaterThanEqual(-abs(r0.xxxx),vec4(0.0)))).xyzw;
// texkill r1
if(any(lessThan(r1.xyz,vec3(0.0))))discard;
// mul r0.yw, c22.xxzy, v2.y
r0.yw=(uP[22].xxzy*v2.yyyy).yw;
// mad r0.yw, v2.x, c21.xxzy, r0
r0.yw=(sm3Mad(v2.xxxx,uP[21].xxzy,r0.xyzw)).yw;
// mad r0.yw, v2.z, c23.xxzy, r0
r0.yw=(sm3Mad(v2.zzzz,uP[23].xxzy,r0.xyzw)).yw;
// add r0.yw, r0, c24.xxzy
r0.yw=(r0.xyzw+uP[24].xxzy).yw;
#ifndef CIV5_LITERAL_WATER_CURRENT_AFFINE
// Measured original HAL: T -> FMA(X) -> FMA(Z) -> FMA(Y).
precise vec4 waterProjected=sm3Mad(v2.xxxx,uP[21],uP[24]);
waterProjected=sm3Mad(v2.zzzz,uP[23],waterProjected);
waterProjected=sm3Mad(v2.yyyy,uP[22],waterProjected);
r0.yw=waterProjected.xy;
#endif
// texld r1, v4.zwzw, s7
r1=(texture(s7,(v4.zwzw).xy)).xyzw;
// mul_sat r1.x, r1.w, c0.z
r1.x=(clamp(r1.wwww*c0.zzzz,0.0,1.0)).x;
// add r2.x, -r1.x, c0.w
r2.x=(-r1.xxxx+c0.wwww).x;
// mov r2.yzw, -c0.y
r2.yzw=(-c0.yyyy).yzw;
// texldl r2, r2, s6
r2=(textureLod(s6,(r2.xyzw).xy ,(r2.xyzw).w)).xyzw;
// texld r3, r0.ywzw, s5
r3=(texture(s5,(r0.ywzw).xy)).xyzw;
precise float waterWeightSum=r3.x+r3.y;
precise float waterWeightY=r3.y;
// mad r0.yw, r3.xxzy, c1.x, c1.y
r0.yw=(sm3Mad(r3.xxzy,c1.xxxx,c1.yyyy)).yw;
// dp2add r3.x, -c0.xwzw, r0.ywzw, -c0.y
r3.x=(sm3Dp2Add(-c0.xwzw,r0.ywzw,-c0.yyyy)).x;
// dp2add r3.y, -c0.w, r0.ywzw, -c0.y
r3.y=2.0-2.0*waterWeightSum;
// add r0.yw, r3.xxzy, c1.x
r0.yw=(r3.xxzy+c1.xxxx).yw;
// dp2add r1.y, -c0.x, r0.ywzw, -c0.y
r1.y=6.0-4.0*waterWeightY;
// rcp r1.y, r1.y
r1.y=(vec4(1.0)/r1.yyyy).y;
// mul r0.yw, r0, r1.y
r0.yw=(r0.xyzw*r1.yyyy).yw;
// rcp r1.y, c4.x
r1.y=(vec4(1.0)/uP[4].xxxx).y;
// mul r0.yw, r0, r1.y
r0.yw=(r0.xyzw*r1.yyyy).yw;
// mul r1.yz, r0.xyww, r0.xyww
r1.yz=(r0.xyww*r0.xyww).yz;
// texld r3, v0, s0
r3=(texture(s0,(v0.xyzw).xy)).xyzw;
// texld r4, v0, s1
r4=(texture(s1,(v0.xyzw).xy)).xyzw;
// mad r3.xy, r3, c1.x, c1.y
r3.xy=(sm3Mad(r3.xyzw,c1.xxxx,c1.yyyy)).xy;
// mul r4.yz, r0.y, r3.xxyw
r4.yz=(r0.yyyy*r3.xxyw).yz;
// mad r5.z, r4.x, c1.x, c1.y
r5.z=(sm3Mad(r4.xxxx,c1.xxxx,c1.yyyy)).z;
// mov r5.xy, r3.zwzw
r5.xy=(r3.zwzw).xy;
// texld r6, v1, s0
r6=(texture(s0,(v1.xyzw).xy)).xyzw;
// texld r7, v1, s1
r7=(texture(s1,(v1.xyzw).xy)).xyzw;
// add r3.zw, r6.xyyx, r6.xyyx
r3.zw=(r6.xyyx+r6.xyyx).zw;
// mad r3.zw, r3, c0.xywx, c0.xyxw
r3.zw=(sm3Mad(r3.xyzw,c0.xywx,c0.xyxw)).zw;
// mul r3.zw, r0.w, r3
r3.zw=(r0.wwww*r3.xyzw).zw;
// mad r8.xy, r3, r0.y, r3.wzzw
r8.xy=r4.yz+r3.wz;
// mad r7.z, r7.x, -c1.x, c1.z
r7.z=(sm3Mad(r7.xxxx,-c1.xxxx,c1.zzzz)).z;
// mov r7.xy, r6.zwzw
r7.xy=(r6.zwzw).xy;
// mul r6.xyz, r1.z, r7
r6.xyz=(r1.zzzz*r7.xyzw).xyz;
// mad r1.yzw, r5.xxyz, r1.y, r6.xxyz
precise vec4 waterMoments0=r5.xxyz;precise vec4 waterMoments1=r7.xxyz;
precise vec2 waterMomentSquares=r1.yz;
r1.yzw=(sm3Mad(r5.xxyz,r1.yyyy,r6.xxyz)).yzw;
// mul r0.yw, r4.xyzz, r3.xwzz
r0.yw=(r4.xyzz*r3.xwzz).yw;
// add r5.xy, r0.ywzw, r0.ywzw
r5.xy=(r0.ywzw+r0.ywzw).xy;
// dp2add r5.z, r4.yzzw, r3.zwzw, -c0.y
r5.z=(sm3Dp2Add(r4.yzzw,r3.zwzw,-c0.yyyy)).z;
// add r1.yzw, r1, r5.xxyz
r1.yzw=sm3Mad(waterMoments1,vec4(waterMomentSquares.y),sm3Mad(waterMoments0,vec4(waterMomentSquares.x),r5.xxyz)).yzw;
// mad r1.yzw, r8.xxyx, -r8.xxyy, r1
r1.yzw=(sm3Mad(r8.xxyx,-r8.xxyy,r1.xyzw)).yzw;
// mov r8.z, c0.w
r8.z=(c0.wwww).z;
// dp3_sat r0.y, c5, r8
precise vec4 waterDiffuse=sm3Mad(r8.xxxx,uP[5].xxxx,uP[5].zzzz);
r0.y=clamp(sm3Mad(r8.yyyy,uP[5].yyyy,waterDiffuse).x,0.0,1.0);
// add r3.xyz, c12, -v2
r3.xyz=(uP[12].xyzw+-v2.xyzw).xyz;
// dp3 r0.w, r3, r3
r0.w=(sm3Dp3(r3.xyzw,r3.xyzw)).w;
// rsq r0.w, r0.w
r0.w=(inversesqrt(abs(r0.wwww))).w;
// mul r4.xyz, r0.w, r3
r4.xyz=(r0.wwww*r3.xyzw).xyz;
precise float waterViewHeight=r3.z;
precise float waterViewInverse=r0.w;
// mad r5.xyz, r3, r0.w, -c9
r5.xyz=(sm3Mad(r3.xyzw,r0.wwww,-uP[9].xyzw)).xyz;
r5.xy=r4.xy-uP[9].xy;
// mad r5.xyz, c9.w, r5, c9
r5.xyz=(sm3Mad(uP[9].wwww,r5.xyzw,uP[9].xyzw)).xyz;
// dp3 r3.w, r5, r5
r3.w=(sm3Dp3(r5.xyzw,r5.xyzw)).w;
// rsq r3.w, r3.w
r3.w=(inversesqrt(abs(r3.wwww))).w;
// mad r5.xyz, r5, r3.w, c5
r5.xyz=(sm3Mad(r5.xyzw,r3.wwww,uP[5].xyzw)).xyz;
// rcp r3.w, r5.z
r3.w=(vec4(1.0)/r5.zzzz).w;
// mad r5.xy, r5, r3.w, -r8
r5.xy=(sm3Mad(r5.xyzw,r3.wwww,-r8.xyzw)).xy;
// rcp r3.w, c7.w
r3.w=(vec4(1.0)/uP[7].wwww).w;
// add r5.zw, r1.xyyz, r3.w
r5.zw=(r1.xyyz+r3.wwww).zw;
// mul r4.w, r1.w, r1.w
r4.w=(r1.wwww*r1.wwww).w;
// mad r6.x, r5.z, r5.w, -r4.w
r6.x=(sm3Mad(r5.zzzz,r5.wwww,-r4.wwww)).x;
// mul r6.y, r5.x, r5.x
r6.y=(r5.xxxx*r5.xxxx).y;
// add r1.w, r1.w, r1.w
r1.w=(r1.wwww+r1.wwww).w;
// mul r5.x, r5.x, r1.w
r5.x=(r5.xxxx*r1.wwww).x;
// mad r5.z, r5.z, r5.y, -r5.x
r5.z=(sm3Mad(r5.zzzz,r5.yyyy,-r5.xxxx)).z;
// mul r5.z, r5.y, r5.z
r5.z=(r5.yyyy*r5.zzzz).z;
// mad r5.z, r6.y, r5.w, r5.z
r5.z=(sm3Mad(r6.yyyy,r5.wwww,r5.zzzz)).z;
// mul r5.z, r5.z, c1.w
r5.z=(r5.zzzz*c1.wwww).z;
// rcp r5.w, r6.x
r5.w=(vec4(1.0)/r6.xxxx).w;
// mul r6.z, r5.w, r5.z
r6.z=(r5.wwww*r5.zzzz).z;
// cmp r6.w, -r6.x, -c0.x, -c0.y
r6.w=(mix(-c0.yyyy,-c0.xxxx,greaterThanEqual(-r6.xxxx,vec4(0.0)))).w;
// mad r5.z, r5.z, r5.w, c29.x
r5.z=(sm3Mad(r5.zzzz,r5.wwww,c29.xxxx)).z;
// cmp r5.z, r5.z, -c0.x, -c0.y
r5.z=(mix(-c0.yyyy,-c0.xxxx,greaterThanEqual(r5.zzzz,vec4(0.0)))).z;
// mul r5.w, r6.z, c29.y
r5.w=(r6.zzzz*c29.yyyy).w;
// exp r5.w, r5.w
r5.w=(exp2(r5.wwww)).w;
// rsq r6.x, r6.x
r6.x=(inversesqrt(abs(r6.xxxx))).x;
// mul r5.w, r5.w, r6.x
r5.w=(r5.wwww*r6.xxxx).w;
// mad r6.xz, r3.w, c29.z, r1.yyzw
r6.xz=(sm3Mad(r3.wwww,c29.zzzz,r1.yyzw)).xz;
// mad r3.w, r6.x, r6.z, -r4.w
r3.w=(sm3Mad(r6.xxxx,r6.zzzz,-r4.wwww)).w;
// mad r5.x, r6.x, r5.y, -r5.x
r5.x=(sm3Mad(r6.xxxx,r5.yyyy,-r5.xxxx)).x;
// mul r5.x, r5.y, r5.x
r5.x=(r5.yyyy*r5.xxxx).x;
// mad r5.x, r6.y, r6.z, r5.x
r5.x=(sm3Mad(r6.yyyy,r6.zzzz,r5.xxxx)).x;
// mul r5.x, r5.x, c1.w
r5.x=(r5.xxxx*c1.wwww).x;
// rcp r5.y, r3.w
r5.y=(vec4(1.0)/r3.wwww).y;
// mul r6.x, r5.y, r5.x
r6.x=(r5.yyyy*r5.xxxx).x;
// cmp r6.y, -r3.w, -c0.x, -c0.y
r6.y=(mix(-c0.yyyy,-c0.xxxx,greaterThanEqual(-r3.wwww,vec4(0.0)))).y;
// mad r5.x, r5.x, r5.y, c29.x
r5.x=(sm3Mad(r5.xxxx,r5.yyyy,c29.xxxx)).x;
// cmp r5.x, r5.x, -c0.x, -c0.y
r5.x=(mix(-c0.yyyy,-c0.xxxx,greaterThanEqual(r5.xxxx,vec4(0.0)))).x;
// add r5.xz, r5, r6.yyww
r5.xz=(r5.xyzw+r6.yyww).xz;
// mul r5.y, r6.x, c29.y
r5.y=(r6.xxxx*c29.yyyy).y;
// exp r5.y, r5.y
r5.y=(exp2(r5.yyyy)).y;
// rsq r3.w, r3.w
r3.w=(inversesqrt(abs(r3.wwww))).w;
// mul r3.w, r3.w, r5.y
r3.w=(r3.wwww*r5.yyyy).w;
// mad r3.xyz, r3, r0.w, -c10
r3.xyz=(sm3Mad(r3.xyzw,r0.wwww,-uP[10].xyzw)).xyz;
r3.xy=r4.xy-uP[10].xy;
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
// mad r3.xy, r3, r0.w, -r8
r3.xy=(sm3Mad(r3.xyzw,r0.wwww,-r8.xyzw)).xy;
// rcp r0.w, c8.w
r0.w=(vec4(1.0)/uP[8].wwww).w;
// add r6.xy, r0.w, r1.yzzw
r6.xy=(r0.wwww+r1.yzzw).xy;
// mad r0.w, r6.x, r6.y, -r4.w
r0.w=(sm3Mad(r6.xxxx,r6.yyyy,-r4.wwww)).w;
// mul r3.z, r3.x, r3.x
r3.z=(r3.xxxx*r3.xxxx).z;
// mul r1.w, r1.w, r3.x
r1.w=(r1.wwww*r3.xxxx).w;
// mad r1.w, r6.x, r3.y, -r1.w
r1.w=(sm3Mad(r6.xxxx,r3.yyyy,-r1.wwww)).w;
// mul r1.w, r3.y, r1.w
r1.w=(r3.yyyy*r1.wwww).w;
// mad r1.w, r3.z, r6.y, r1.w
r1.w=(sm3Mad(r3.zzzz,r6.yyyy,r1.wwww)).w;
// mul r1.w, r1.w, c1.w
r1.w=(r1.wwww*c1.wwww).w;
// rcp r3.x, r0.w
r3.x=(vec4(1.0)/r0.wwww).x;
// mul r3.y, r1.w, r3.x
r3.y=(r1.wwww*r3.xxxx).y;
// cmp r3.z, -r0.w, -c0.x, -c0.y
r3.z=(mix(-c0.yyyy,-c0.xxxx,greaterThanEqual(-r0.wwww,vec4(0.0)))).z;
// mad r1.w, r1.w, r3.x, c29.x
r1.w=(sm3Mad(r1.wwww,r3.xxxx,c29.xxxx)).w;
// cmp r1.w, r1.w, -c0.x, -c0.y
r1.w=(mix(-c0.yyyy,-c0.xxxx,greaterThanEqual(r1.wwww,vec4(0.0)))).w;
// add r1.w, r1.w, r3.z
r1.w=(r1.wwww+r3.zzzz).w;
// mul r3.xw, r3.yyzw, c29.yyzw
r3.xw=(r3.yyzw*c29.yyzw).xw;
// exp r3.x, r3.x
r3.x=(exp2(r3.xxxx)).x;
// rsq r3.z, r0.w
r3.z=(inversesqrt(abs(r0.wwww))).z;
// mul r3.x, r3.z, r3.x
r3.x=(r3.zzzz*r3.xxxx).x;
// mul r6.xyz, c14.xyww, v2.y
r6.xyz=(uP[14].xyww*v2.yyyy).xyz;
// mad r6.xyz, v2.x, c13.xyww, r6
r6.xyz=(sm3Mad(v2.xxxx,uP[13].xyww,r6.xyzw)).xyz;
// mad r6.xyz, v2.z, c15.xyww, r6
r6.xyz=(sm3Mad(v2.zzzz,uP[15].xyww,r6.xyzw)).xyz;
// add r6.xyz, r6, c16.xyww
r6.xyz=(r6.xyzw+uP[16].xyww).xyz;
// rcp r3.z, r6.z
r3.z=(vec4(1.0)/r6.zzzz).z;
// mul r6.xy, r3.z, r6
r6.xy=(r3.zzzz*r6.xyzw).xy;
// mad r6.xy, r6, c2, c2.y
r6.xy=(sm3Mad(r6.xyzw,c2.xyzw,c2.yyyy)).xy;
// mad r1.yz, r1, c2.z, r6.xxyw
r1.yz=(sm3Mad(r1.xyzw,c2.zzzz,r6.xxyw)).yz;
// mad r6.xy, r0.w, c1.x, r1.yzzw
r6.xy=(sm3Mad(r0.wwww,c1.xxxx,r1.yzzw)).xy;
// texld r6, r6, s4
r6=(texture(s4,(r6.xyzw).xy)).xyzw;
// mad r7.xy, r0.w, c3, r1.yzzw
r7.xy=(sm3Mad(r0.wwww,c3.xyzw,r1.yzzw)).xy;
// texld r7, r7, s4
r7=(texture(s4,(r7.xyzw).xy)).xyzw;
// mul r7, r7, c2.w
r7=(r7.xyzw*c2.wwww).xyzw;
// mad r6, r6, c2.w, r7
r6=(sm3Mad(r6.xyzw,c2.wwww,r7.xyzw)).xyzw;
// mad r7.xy, r0.w, c3.yzzw, r1.yzzw
r7.xy=(sm3Mad(r0.wwww,c3.yzzw,r1.yzzw)).xy;
// texld r7, r7, s4
r7=(texture(s4,(r7.xyzw).xy)).xyzw;
// mad r6, r7, c2.w, r6
r6=(sm3Mad(r7.xyzw,c2.wwww,r6.xyzw)).xyzw;
// mad r1.yz, r0.w, -c1.x, r1
r1.yz=(sm3Mad(r0.wwww,-c1.xxxx,r1.xyzw)).yz;
// texld r7, r1.yzzw, s4
r7=(texture(s4,(r1.yzzw).xy)).xyzw;
// mad r6, r7, c2.w, r6
r6=(sm3Mad(r7.xyzw,c2.wwww,r6.xyzw)).xyzw;
// log r7.x, r6.x
r7.x=(sm3Log(r6.xxxx)).x;
// log r7.y, r6.y
r7.y=(sm3Log(r6.yyyy)).y;
// log r7.z, r6.z
r7.z=(sm3Log(r6.zzzz)).z;
// mul r6.xyz, r7, c3.w
r6.xyz=(r7.xyzw*c3.wwww).xyz;
// exp r7.x, r6.x
r7.x=(exp2(r6.xxxx)).x;
// exp r7.y, r6.y
r7.y=(exp2(r6.yyyy)).y;
// exp r7.z, r6.z
r7.z=(exp2(r6.zzzz)).z;
// dp3 r0.w, r8, r4
r0.w=sm3Mad(vec4(waterViewHeight),vec4(waterViewInverse),sm3Mad(r8.yyyy,r4.yyyy,r8.xxxx*r4.xxxx)).x;
// add r0.w, -r0.w, c0.w
r0.w=(-r0.wwww+c0.wwww).w;
// mul r1.y, r0.w, r0.w
r1.y=(r0.wwww*r0.wwww).y;
// mul r1.y, r1.y, r1.y
r1.y=(r1.yyyy*r1.yyyy).y;
// mul r0.w, r0.w, r1.y
r0.w=(r0.wwww*r1.yyyy).w;
// mul r4.xyz, r5.w, c7
r4.xyz=(r5.wwww*uP[7].xyzw).xyz;
// cmp r4.xyz, -r5.z, r4, -c0.y
r4.xyz=(mix(-c0.yyyy,r4.xyzw,greaterThanEqual(-r5.zzzz,vec4(0.0)))).xyz;
// mul r5.yzw, r3.x, c8.xxyz
r5.yzw=(r3.xxxx*uP[8].xxyz).yzw;
// cmp r1.yzw, -r1.w, r5, -c0.y
r1.yzw=(mix(-c0.yyyy,r5.xyzw,greaterThanEqual(-r1.wwww,vec4(0.0)))).yzw;
// add r1.yzw, r1, r4.xxyz
r1.yzw=(r1.xyzw+r4.xxyz).yzw;
// mad r1.yzw, r1, r0.w, r2.xxyz
r1.yzw=(sm3Mad(r1.xyzw,r0.wwww,r2.xxyz)).yzw;
// mul r4.xyz, r2, c7
r4.xyz=(r2.xyzw*uP[7].xyzw).xyz;
// mul r3.xzw, r3.w, r4.xyyz
r3.xzw=(r3.wwww*r4.xyyz).xzw;
// cmp r3.xzw, -r5.x, r3, -c0.y
r3.xzw=(mix(-c0.yyyy,r3.xyzw,greaterThanEqual(-r5.xxxx,vec4(0.0)))).xzw;
// mad r1.yzw, r1, r0.y, r3.xxzw
r1.yzw=(sm3Mad(r1.xyzw,r0.yyyy,r3.xxzw)).yzw;
// mad r1.yzw, r7.xxyz, c27.x, r1
r1.yzw=(sm3Mad(r7.xxyz,uP[27].xxxx,r1.xyzw)).yzw;
// mul r0.y, r3.y, r3.y
r0.y=(r3.yyyy*r3.yyyy).y;
// mul r0.y, r3.y, r0.y
r0.y=(r3.yyyy*r0.yyyy).y;
// mul r0.y, r1.x, r0.y
r0.y=(r1.xxxx*r0.yyyy).y;
// mad r1.xyz, r0.y, r2, r1.yzww
r1.xyz=(sm3Mad(r0.yyyy,r2.xyzw,r1.yzww)).xyz;
// mul r2.xyz, c18.xyww, v2.y
r2.xyz=(uP[18].xyww*v2.yyyy).xyz;
// mad r2.xyz, v2.x, c17.xyww, r2
r2.xyz=(sm3Mad(v2.xxxx,uP[17].xyww,r2.xyzw)).xyz;
// mad r2.xyz, v2.z, c19.xyww, r2
r2.xyz=(sm3Mad(v2.zzzz,uP[19].xyww,r2.xyzw)).xyz;
// add r2.xyz, r2, c20.xyww
r2.xyz=(r2.xyzw+uP[20].xyww).xyz;
// rcp r0.y, r2.z
r0.y=(vec4(1.0)/r2.zzzz).y;
// mul r0.yw, r0.y, r2.xxzy
r0.yw=(r0.yyyy*r2.xxzy).yw;
// mad r0.yw, r0, c2.xxzy, c2.y
r0.yw=(sm3Mad(r0.xyzw,c2.xxzy,c2.yyyy)).yw;
// texld r3, r0.ywzw, s13
r3=(texture(s13,(r0.ywzw).xy)).xyzw;
// mov_sat r3.x, r3.x
r3.x=(clamp(r3.xxxx,0.0,1.0)).x;
// mul r0.y, r3.x, c25.x
r0.y=(r3.xxxx*uP[25].xxxx).y;
// mul r2.xyz, r0.y, r1
r2.xyz=(r0.yyyy*r1.xyzw).xyz;
// mad r1.xyz, r2, c28.x, r1
precise float waterShadowScale=r0.y*c28.x;
r1.xyz=sm3Mad(r1,vec4(waterShadowScale),r1).xyz;
// mad_sat oC0.w, r6.w, r2.w, r2.w
oC0.w=(clamp(sm3Mad(r6.wwww,r2.wwww,r2.wwww),0.0,1.0)).w;
// log r2.x, r1.x
r2.x=(sm3Log(r1.xxxx)).x;
// log r2.y, r1.y
r2.y=(sm3Log(r1.yyyy)).y;
// log r2.z, r1.z
r2.z=(sm3Log(r1.zzzz)).z;
// mul r1.xyz, r2, c28.y
r1.xyz=(r2.xyzw*c28.yyyy).xyz;
// exp r2.x, r1.x
r2.x=(exp2(r1.xxxx)).x;
// exp r2.y, r1.y
r2.y=(exp2(r1.yyyy)).y;
// exp r2.z, r1.z
r2.z=(exp2(r1.zzzz)).z;
// texld r1, v4, s9
r1=(texture(s9,(v4.xyzw).xy)).xyzw;
// add r0.y, -r1.w, c0.w
r0.y=(-r1.wwww+c0.wwww).y;
// mad r1.xyz, r2, r0.y, r1
r1.xyz=(sm3Mad(r2.xyzw,r0.yyyy,r1.xyzw)).xyz;
// add r1.xyz, r1, c28.z
r1.xyz=(r1.xyzw+c28.zzzz).xyz;
// mad r1.xyz, r0.x, r1, c28.w
r1.xyz=(sm3Mad(r0.xxxx,r1.xyzw,c28.wwww)).xyz;
// texld r2, r1, s11
r2=(texture(s11,(r1.xyzw).xyz)).xyzw;
// mad r0.x, r0.z, -r0.x, r0.x
r0.x=(sm3Mad(r0.zzzz,-r0.xxxx,r0.xxxx)).x;
// lrp r3.xyz, r0.x, r2, r1
r3.xyz=(sm3Lrp(r0.xxxx,r2.xyzw,r1.xyzw)).xyz;
// rcp r0.x, v3.z
r0.x=(vec4(1.0)/v3.zzzz).x;
// mul r0.x, r0.x, v3.y
r0.x=(r0.xxxx*v3.yyyy).x;
// mad r0.x, r0.x, c1.w, c1.w
r0.x=(sm3Mad(r0.xxxx,c1.wwww,c1.wwww)).x;
// mov r0.y, c11.w
r0.y=(uP[11].wwww).y;
// texld r0, r0, s8
r0=(texture(s8,(r0.xyzw).xy)).xyzw;
// add r0.yzw, -c12.xxyz, v2.xxyz
r0.yzw=(-uP[12].xxyz+v2.xxyz).yzw;
// dp3 r0.y, r0.yzww, r0.yzww
r0.y=(sm3Dp3(r0.yzww,r0.yzww)).y;
// mul r0.yz, r0.xyww, c11.xyxw
r0.yz=(r0.xyww*uP[11].xyxw).yz;
// mul r0.z, r0.z, -c29.y
r0.z=(r0.zzzz*-c29.yyyy).z;
// exp r0.z, r0.z
r0.z=(exp2(r0.zzzz)).z;
// add r0.z, -r0.z, c0.w
r0.z=(-r0.zzzz+c0.wwww).z;
// mul r0.y, r0.z, r0.y
r0.y=(r0.zzzz*r0.yyyy).y;
// rcp r0.z, r0.w
r0.z=(vec4(1.0)/r0.wwww).z;
// mul r0.y, r0.z, r0.y
r0.y=(r0.zzzz*r0.yyyy).y;
// mul r0.y, r0.y, -c29.y
r0.y=(r0.yyyy*-c29.yyyy).y;
// exp_sat r0.y, r0.y
r0.y=(clamp(exp2(r0.yyyy),0.0,1.0)).y;
// add r0.y, -r0.y, c0.w
r0.y=(-r0.yyyy+c0.wwww).y;
// mul r0.x, r0.y, r0.x
r0.x=(r0.yyyy*r0.xxxx).x;
// add r0.yzw, -r3.xxyz, c26.xxyz
r0.yzw=(-r3.xxyz+uP[26].xxyz).yzw;
// mad oC0.xyz, r0.x, r0.yzww, r3
oC0.xyz=(sm3Mad(r0.xxxx,r0.yzww,r3.xyzw)).xyz;
outColor=oC0;
}
)GLSL";
constexpr std::string_view pixel3=R"GLSL(#version 460 core
in vec2 waterT0;in vec2 waterT1;in vec2 waterT2;in vec2 waterT3;in vec3 waterWorld;in vec3 waterClip;in vec4 waterT6;
uniform vec4 uP[30];layout(location=0) out vec4 outColor;
layout(binding=0) uniform sampler2D s0;
layout(binding=1) uniform sampler2D s1;
layout(binding=4) uniform sampler2D s4;
layout(binding=5) uniform sampler2D s5;
layout(binding=6) uniform sampler2D s6;
layout(binding=7) uniform sampler2D s7;
layout(binding=8) uniform sampler2D s8;
layout(binding=9) uniform sampler2D s9;
layout(binding=10) uniform sampler2D s10;
layout(binding=11) uniform sampler3D s11;
layout(binding=13) uniform sampler2D s13;

vec4 sm3Mad(vec4 a,vec4 b,vec4 c){precise dvec4 v=dvec4(a)*dvec4(b)+dvec4(c);return vec4(v);}
vec4 sm3Lrp(vec4 a,vec4 b,vec4 c){precise vec4 x=b-c;return sm3Mad(a,x,c);}
float sm3Log(float a){return a==0.0 ? -3.402823466e38 : log2(abs(a));}
vec4 sm3Log(vec4 a){return vec4(sm3Log(a.x),sm3Log(a.y),sm3Log(a.z),sm3Log(a.w));}
vec4 sm3Dp3(vec4 a,vec4 b){precise float v=a.x*b.x;v=float(double(a.y)*double(b.y)+double(v));v=float(double(a.z)*double(b.z)+double(v));return vec4(v);}

vec4 sm3Dp2Add(vec4 a,vec4 b,vec4 c){precise float v=a.x*b.x;v=float(double(a.y)*double(b.y)+double(v));v=v+c.x;return vec4(v);}
vec4 sm3Dp4(vec4 a,vec4 b){precise float v=a.x*b.x;v=float(double(a.y)*double(b.y)+double(v));v=float(double(a.z)*double(b.z)+double(v));v=float(double(a.w)*double(b.w)+double(v));return vec4(v);}

void main(){
const vec4 c0=vec4(0.5,-0.5,6.0,0.25);
const vec4 c1=vec4(2.0,-2.0,6.0,2.20000005);
const vec4 c2=vec4(-1.0,-0.0,4.0,1.0);
const vec4 c3=vec4(-0.670000017,0.454545468,-0.400000006,0.400000006);
const vec4 c28=vec4(2.0,-1.0,1.0,0.5);
const vec4 c29=vec4(-16.0,-1.44269502,16.0,0.00499999989);
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
precise vec4 r9;
precise vec4 oC0;
// texld r0, v6, s10
r0=(texture(s10,(v6.xyzw).xy)).xyzw;
// cmp r1, -r0_abs.x, c2.x, c2.y
r1=(mix(c2.yyyy,c2.xxxx,greaterThanEqual(-abs(r0.xxxx),vec4(0.0)))).xyzw;
// texkill r1
if(any(lessThan(r1.xyz,vec3(0.0))))discard;
// mul r0.yw, c22.xxzy, v4.y
r0.yw=(uP[22].xxzy*v4.yyyy).yw;
// mad r0.yw, v4.x, c21.xxzy, r0
r0.yw=(sm3Mad(v4.xxxx,uP[21].xxzy,r0.xyzw)).yw;
// mad r0.yw, v4.z, c23.xxzy, r0
r0.yw=(sm3Mad(v4.zzzz,uP[23].xxzy,r0.xyzw)).yw;
// add r0.yw, r0, c24.xxzy
r0.yw=(r0.xyzw+uP[24].xxzy).yw;
#ifndef CIV5_LITERAL_WATER_CURRENT_AFFINE
// Measured original HAL: T -> FMA(X) -> FMA(Z) -> FMA(Y).
precise vec4 waterProjected=sm3Mad(v4.xxxx,uP[21],uP[24]);
waterProjected=sm3Mad(v4.zzzz,uP[23],waterProjected);
waterProjected=sm3Mad(v4.yyyy,uP[22],waterProjected);
r0.yw=waterProjected.xy;
#endif
// texld r1, v6.zwzw, s7
r1=(texture(s7,(v6.zwzw).xy)).xyzw;
// mul_sat r1.x, r1.w, c2.z
r1.x=(clamp(r1.wwww*c2.zzzz,0.0,1.0)).x;
// add r2.x, -r1.x, c2.w
r2.x=(-r1.xxxx+c2.wwww).x;
// mov r2.yzw, -c2.y
r2.yzw=(-c2.yyyy).yzw;
// texldl r2, r2, s6
r2=(textureLod(s6,(r2.xyzw).xy ,(r2.xyzw).w)).xyzw;
// texld r3, r0.ywzw, s5
r3=(texture(s5,(r0.ywzw).xy)).xyzw;
precise float waterWeightSum=r3.x+r3.y;
precise float waterWeightY=r3.y;
// mad r0.yw, r3.xxzy, c28.x, c28.y
r0.yw=(sm3Mad(r3.xxzy,c28.xxxx,c28.yyyy)).yw;
// dp2add r3.x, -c2.xwzw, r0.ywzw, -c2.y
r3.x=(sm3Dp2Add(-c2.xwzw,r0.ywzw,-c2.yyyy)).x;
// dp2add r3.y, -c2.w, r0.ywzw, -c2.y
r3.y=2.0-2.0*waterWeightSum;
// mov r3.zw, -r3.xyxy
r3.zw=(-r3.xyxy).zw;
// add r3, r3, c28.x
r3=(r3.xyzw+c28.xxxx).xyzw;
// mul r3, r3, r3
r3=(r3.xyzw*r3.xyzw).xyzw;
// mul r3, r3, r3
r3=(r3.xyzw*r3.xyzw).xyzw;
// dp4 r0.y, c2.w, r3
r0.y=(sm3Dp4(c2.wwww,r3.xyzw)).y;
// rcp r0.y, r0.y
r0.y=(vec4(1.0)/r0.yyyy).y;
// mul r3, r0.y, r3
r3=(r0.yyyy*r3.xyzw).xyzw;
// rcp r0.y, c4.x
r0.y=(vec4(1.0)/uP[4].xxxx).y;
// mul r3, r0.y, r3
r3=(r0.yyyy*r3.xyzw).xyzw;
// mul r4, r3, r3
r4=(r3.xyzw*r3.xyzw).xyzw;
// texld r5, v0, s0
r5=(texture(s0,(v0.xyzw).xy)).xyzw;
// texld r6, v0, s1
r6=(texture(s1,(v0.xyzw).xy)).xyzw;
// mad r0.yw, r5.xxzy, c28.x, c28.y
r0.yw=(sm3Mad(r5.xxzy,c28.xxxx,c28.yyyy)).yw;
// mul r1.yz, r3.x, r0.xyww
r1.yz=(r3.xxxx*r0.xyww).yz;
// mad r6.z, r6.x, c28.x, c28.y
r6.z=(sm3Mad(r6.xxxx,c28.xxxx,c28.yyyy)).z;
// mov r6.xy, r5.zwzw
r6.xy=(r5.zwzw).xy;
// texld r5, v1, s0
r5=(texture(s0,(v1.xyzw).xy)).xyzw;
// texld r7, v1, s1
r7=(texture(s1,(v1.xyzw).xy)).xyzw;
// add r5.xy, r5, r5
r5.xy=(r5.xyzw+r5.xyzw).xy;
// mad r5.xy, r5, c2.xwzw, c2.wxzw
r5.xy=(sm3Mad(r5.xyzw,c2.xwzw,c2.wxzw)).xy;
// mul r5.xy, r3.y, r5
r5.xy=(r3.yyyy*r5.xyzw).xy;
// mad r0.yw, r0, r3.x, r5.xxzy
r0.yw=r1.yz+r5.xy;
// mad r7.z, r7.x, -c28.x, c28.z
r7.z=(sm3Mad(r7.xxxx,-c28.xxxx,c28.zzzz)).z;
// mov r7.xy, r5.zwzw
r7.xy=(r5.zwzw).xy;
// mul r7.xyz, r4.y, r7
precise vec2 waterFirstMomentZ=vec2(r6.z,r7.z);
precise vec2 waterFirstMomentSquares=r4.xy;
r7.xyz=(r4.yyyy*r7.xyzw).xyz;
// mad r6.xyz, r6, r4.x, r7
r6.xyz=(sm3Mad(r6.xyzw,r4.xxxx,r7.xyzw)).xyz;
// add r3.xy, r1.yzzw, r1.yzzw
r3.xy=(r1.yzzw+r1.yzzw).xy;
// mul r7.xy, r5, r3
r7.xy=(r5.xyzw*r3.xyzw).xy;
// dp2add r7.z, r1.yzzw, r5.yxzw, -c2.y
r7.z=(sm3Dp2Add(r1.yzzw,r5.yxzw,-c2.yyyy)).z;
// add r6.xyz, r6, r7
r6.xy=sm3Mad(r5.xyxy,r3.xyxy,r6).xy;
r6.z=sm3Mad(vec4(waterFirstMomentZ.y),vec4(waterFirstMomentSquares.y),sm3Mad(vec4(waterFirstMomentZ.x),vec4(waterFirstMomentSquares.x),r7.zzzz)).x;
// texld r7, v2, s0
r7=(texture(s0,(v2.xyzw).xy)).xyzw;
// texld r8, v2, s1
r8=(texture(s1,(v2.xyzw).xy)).xyzw;
// mad r4.xy, r7, -c28.x, c28.z
r4.xy=(sm3Mad(r7.xyzw,-c28.xxxx,c28.zzzz)).xy;
// mul r5.zw, r3.z, r4.xyxy
r5.zw=(r3.zzzz*r4.xyxy).zw;
// mad r0.yw, r4.xxzy, r3.z, r0
r0.yw=r5.zw+r0.yw;
// mad r8.z, r8.x, c28.x, c28.y
r8.z=(sm3Mad(r8.xxxx,c28.xxxx,c28.yyyy)).z;
// mov r8.xy, r7.zwzw
r8.xy=(r7.zwzw).xy;
// mad r4.xyz, r8, r4.z, r6
r4.xyz=(sm3Mad(r8.xyzw,r4.zzzz,r6.xyzw)).xyz;
// mul r6.xy, r3, r5.zwzw
r6.xy=(r3.xyzw*r5.zwzw).xy;
// dp2add r6.z, r1.yzzw, r5.wzzw, -c2.y
r6.z=(sm3Dp2Add(r1.yzzw,r5.wzzw,-c2.yyyy)).z;
// add r4.xyz, r4, r6
r4.xy=sm3Mad(r3.xyxy,r5.zwzw,r4).xy;
r4.z=sm3Mad(r1.yyyy,r5.wwww,sm3Mad(r1.zzzz,r5.zzzz,r4.zzzz)).x;
// add r6.xy, r5, r5
r6.xy=(r5.xyzw+r5.xyzw).xy;
// mul r7.xy, r5.zwzw, r6
r7.xy=(r5.zwzw*r6.xyzw).xy;
// dp2add r7.z, r5, r5.wzzw, -c2.y
r7.z=(sm3Dp2Add(r5.xyzw,r5.wzzw,-c2.yyyy)).z;
// add r4.xyz, r4, r7
r4.xy=sm3Mad(r6.xyxy,r5.zwzw,r4).xy;
r4.z=sm3Mad(r5.xxxx,r5.wwww,sm3Mad(r5.yyyy,r5.zzzz,r4.zzzz)).x;
// texld r7, v3, s0
r7=(texture(s0,(v3.xyzw).xy)).xyzw;
// texld r8, v3, s1
r8=(texture(s1,(v3.xyzw).xy)).xyzw;
// add r6.zw, r7.xyyx, r7.xyyx
r6.zw=(r7.xyyx+r7.xyyx).zw;
// mad r6.zw, r6, c2.xyxw, c2.xywx
r6.zw=(sm3Mad(r6.xyzw,c2.xyxw,c2.xywx)).zw;
// mul r7.xy, r3.w, r6.zwzw
r7.xy=(r3.wwww*r6.zwzw).xy;
// mad r9.xy, r6.wzzw, r3.w, r0.ywzw
r9.xy=r7.yx+r0.yw;
// mad r8.z, r8.x, -c28.x, c28.z
r8.z=(sm3Mad(r8.xxxx,-c28.xxxx,c28.zzzz)).z;
// mov r8.xy, r7.zwzw
r8.xy=(r7.zwzw).xy;
// mad r4.xyz, r8, r4.w, r4
r4.xyz=(sm3Mad(r8.xyzw,r4.wwww,r4.xyzw)).xyz;
// mul r3.xy, r3, r7.yxzw
r3.xy=(r3.xyzw*r7.yxzw).xy;
// dp2add r3.z, r1.yzzw, r7, -c2.y
r3.z=(sm3Dp2Add(r1.yzzw,r7.xyzw,-c2.yyyy)).z;
// add r1.yzw, r3.xxyz, r4.xxyz
precise vec2 waterNormal0=r1.yz;
r1.yz=sm3Mad(vec4(waterNormal0*2.0,waterNormal0*2.0),r7.yxyx,r4).xy;
r1.w=sm3Mad(vec4(waterNormal0.x),r7.xxxx,sm3Mad(vec4(waterNormal0.y),r7.yyyy,r4.zzzz)).x;
// mul r3.xy, r6, r7.yxzw
r3.xy=(r6.xyzw*r7.yxzw).xy;
// dp2add r3.z, r5, r7, -c2.y
r3.z=(sm3Dp2Add(r5.xyzw,r7.xyzw,-c2.yyyy)).z;
// add r1.yzw, r1, r3.xxyz
r1.yz=sm3Mad(r6.xyxy,r7.yxyx,r1.yzyz).xy;
r1.w=sm3Mad(r5.xxxx,r7.xxxx,sm3Mad(r5.yyyy,r7.yyyy,r1.wwww)).x;
// mul r0.yw, r5.xzzw, r7.xyzx
r0.yw=(r5.xzzw*r7.xyzx).yw;
// add r3.xy, r0.ywzw, r0.ywzw
r3.xy=(r0.ywzw+r0.ywzw).xy;
// dp2add r3.z, r5.zwzw, r7, -c2.y
r3.z=(sm3Dp2Add(r5.zwzw,r7.xyzw,-c2.yyyy)).z;
// add r1.yzw, r1, r3.xxyz
r1.yz=sm3Mad(r5.zwzw,r7.yxyx,sm3Mad(r5.zwzw,r7.yxyx,r1.yzyz)).xy;
r1.w=sm3Mad(r5.zzzz,r7.xxxx,sm3Mad(r5.wwww,r7.yyyy,r1.wwww)).x;
// mad r1.yzw, r9.xxyx, -r9.xxyy, r1
r1.yzw=(sm3Mad(r9.xxyx,-r9.xxyy,r1.xyzw)).yzw;
// mov r9.z, c2.w
r9.z=(c2.wwww).z;
// dp3_sat r0.y, c5, r9
precise vec4 waterDiffuse=sm3Mad(r9.xxxx,uP[5].xxxx,uP[5].zzzz);
r0.y=clamp(sm3Mad(r9.yyyy,uP[5].yyyy,waterDiffuse).x,0.0,1.0);
// add r3.xyz, c12, -v4
r3.xyz=(uP[12].xyzw+-v4.xyzw).xyz;
// dp3 r0.w, r3, r3
r0.w=(sm3Dp3(r3.xyzw,r3.xyzw)).w;
// rsq r0.w, r0.w
r0.w=(inversesqrt(abs(r0.wwww))).w;
// mul r4.xyz, r0.w, r3
r4.xyz=(r0.wwww*r3.xyzw).xyz;
precise float waterViewHeight=r3.z;
precise float waterViewInverse=r0.w;
// mad r5.xyz, r3, r0.w, -c9
r5.xyz=(sm3Mad(r3.xyzw,r0.wwww,-uP[9].xyzw)).xyz;
r5.xy=r4.xy-uP[9].xy;
// mad r5.xyz, c9.w, r5, c9
r5.xyz=(sm3Mad(uP[9].wwww,r5.xyzw,uP[9].xyzw)).xyz;
// dp3 r3.w, r5, r5
r3.w=(sm3Dp3(r5.xyzw,r5.xyzw)).w;
// rsq r3.w, r3.w
r3.w=(inversesqrt(abs(r3.wwww))).w;
// mad r5.xyz, r5, r3.w, c5
r5.xyz=(sm3Mad(r5.xyzw,r3.wwww,uP[5].xyzw)).xyz;
// rcp r3.w, r5.z
r3.w=(vec4(1.0)/r5.zzzz).w;
// mad r5.xy, r5, r3.w, -r9
r5.xy=(sm3Mad(r5.xyzw,r3.wwww,-r9.xyzw)).xy;
// rcp r3.w, c7.w
r3.w=(vec4(1.0)/uP[7].wwww).w;
// add r5.zw, r1.xyyz, r3.w
r5.zw=(r1.xyyz+r3.wwww).zw;
// mul r4.w, r1.w, r1.w
r4.w=(r1.wwww*r1.wwww).w;
// mad r6.x, r5.z, r5.w, -r4.w
r6.x=(sm3Mad(r5.zzzz,r5.wwww,-r4.wwww)).x;
// mul r6.y, r5.x, r5.x
r6.y=(r5.xxxx*r5.xxxx).y;
// add r1.w, r1.w, r1.w
r1.w=(r1.wwww+r1.wwww).w;
// mul r5.x, r5.x, r1.w
r5.x=(r5.xxxx*r1.wwww).x;
// mad r5.z, r5.z, r5.y, -r5.x
r5.z=(sm3Mad(r5.zzzz,r5.yyyy,-r5.xxxx)).z;
// mul r5.z, r5.y, r5.z
r5.z=(r5.yyyy*r5.zzzz).z;
// mad r5.z, r6.y, r5.w, r5.z
r5.z=(sm3Mad(r6.yyyy,r5.wwww,r5.zzzz)).z;
// mul r5.z, r5.z, c28.w
r5.z=(r5.zzzz*c28.wwww).z;
// rcp r5.w, r6.x
r5.w=(vec4(1.0)/r6.xxxx).w;
// mul r6.z, r5.w, r5.z
r6.z=(r5.wwww*r5.zzzz).z;
// cmp r6.w, -r6.x, -c2.x, -c2.y
r6.w=(mix(-c2.yyyy,-c2.xxxx,greaterThanEqual(-r6.xxxx,vec4(0.0)))).w;
// mad r5.z, r5.z, r5.w, c29.x
r5.z=(sm3Mad(r5.zzzz,r5.wwww,c29.xxxx)).z;
// cmp r5.z, r5.z, -c2.x, -c2.y
r5.z=(mix(-c2.yyyy,-c2.xxxx,greaterThanEqual(r5.zzzz,vec4(0.0)))).z;
// mul r5.w, r6.z, c29.y
r5.w=(r6.zzzz*c29.yyyy).w;
// exp r5.w, r5.w
r5.w=(exp2(r5.wwww)).w;
// rsq r6.x, r6.x
r6.x=(inversesqrt(abs(r6.xxxx))).x;
// mul r5.w, r5.w, r6.x
r5.w=(r5.wwww*r6.xxxx).w;
// mad r6.xz, r3.w, c29.z, r1.yyzw
r6.xz=(sm3Mad(r3.wwww,c29.zzzz,r1.yyzw)).xz;
// mad r3.w, r6.x, r6.z, -r4.w
r3.w=(sm3Mad(r6.xxxx,r6.zzzz,-r4.wwww)).w;
// mad r5.x, r6.x, r5.y, -r5.x
r5.x=(sm3Mad(r6.xxxx,r5.yyyy,-r5.xxxx)).x;
// mul r5.x, r5.y, r5.x
r5.x=(r5.yyyy*r5.xxxx).x;
// mad r5.x, r6.y, r6.z, r5.x
r5.x=(sm3Mad(r6.yyyy,r6.zzzz,r5.xxxx)).x;
// mul r5.x, r5.x, c28.w
r5.x=(r5.xxxx*c28.wwww).x;
// rcp r5.y, r3.w
r5.y=(vec4(1.0)/r3.wwww).y;
// mul r6.x, r5.y, r5.x
r6.x=(r5.yyyy*r5.xxxx).x;
// cmp r6.y, -r3.w, -c2.x, -c2.y
r6.y=(mix(-c2.yyyy,-c2.xxxx,greaterThanEqual(-r3.wwww,vec4(0.0)))).y;
// mad r5.x, r5.x, r5.y, c29.x
r5.x=(sm3Mad(r5.xxxx,r5.yyyy,c29.xxxx)).x;
// cmp r5.x, r5.x, -c2.x, -c2.y
r5.x=(mix(-c2.yyyy,-c2.xxxx,greaterThanEqual(r5.xxxx,vec4(0.0)))).x;
// add r5.xz, r5, r6.yyww
r5.xz=(r5.xyzw+r6.yyww).xz;
// mul r5.y, r6.x, c29.y
r5.y=(r6.xxxx*c29.yyyy).y;
// exp r5.y, r5.y
r5.y=(exp2(r5.yyyy)).y;
// rsq r3.w, r3.w
r3.w=(inversesqrt(abs(r3.wwww))).w;
// mul r3.w, r3.w, r5.y
precise float waterScaledSpecularExp=r5.y*c29.w;
r3.w=r3.w*waterScaledSpecularExp;
// mad r3.xyz, r3, r0.w, -c10
r3.xyz=(sm3Mad(r3.xyzw,r0.wwww,-uP[10].xyzw)).xyz;
r3.xy=r4.xy-uP[10].xy;
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
// mad r3.xy, r3, r0.w, -r9
r3.xy=(sm3Mad(r3.xyzw,r0.wwww,-r9.xyzw)).xy;
// rcp r0.w, c8.w
r0.w=(vec4(1.0)/uP[8].wwww).w;
// add r6.xy, r0.w, r1.yzzw
r6.xy=(r0.wwww+r1.yzzw).xy;
// mad r0.w, r6.x, r6.y, -r4.w
r0.w=(sm3Mad(r6.xxxx,r6.yyyy,-r4.wwww)).w;
// mul r3.z, r3.x, r3.x
r3.z=(r3.xxxx*r3.xxxx).z;
// mul r1.w, r1.w, r3.x
r1.w=(r1.wwww*r3.xxxx).w;
// mad r1.w, r6.x, r3.y, -r1.w
r1.w=(sm3Mad(r6.xxxx,r3.yyyy,-r1.wwww)).w;
// mul r1.w, r3.y, r1.w
r1.w=(r3.yyyy*r1.wwww).w;
// mad r1.w, r3.z, r6.y, r1.w
r1.w=(sm3Mad(r3.zzzz,r6.yyyy,r1.wwww)).w;
// mul r1.w, r1.w, c28.w
r1.w=(r1.wwww*c28.wwww).w;
// rcp r3.x, r0.w
r3.x=(vec4(1.0)/r0.wwww).x;
// mul r3.y, r1.w, r3.x
r3.y=(r1.wwww*r3.xxxx).y;
// cmp r3.z, -r0.w, -c2.x, -c2.y
r3.z=(mix(-c2.yyyy,-c2.xxxx,greaterThanEqual(-r0.wwww,vec4(0.0)))).z;
// mad r1.w, r1.w, r3.x, c29.x
r1.w=(sm3Mad(r1.wwww,r3.xxxx,c29.xxxx)).w;
// cmp r1.w, r1.w, -c2.x, -c2.y
r1.w=(mix(-c2.yyyy,-c2.xxxx,greaterThanEqual(r1.wwww,vec4(0.0)))).w;
// add r1.w, r1.w, r3.z
r1.w=(r1.wwww+r3.zzzz).w;
// mul r3.xw, r3.yyzw, c29.yyzw
r3.x=r3.y*c29.y;
// exp r3.x, r3.x
r3.x=(exp2(r3.xxxx)).x;
// rsq r3.z, r0.w
r3.z=(inversesqrt(abs(r0.wwww))).z;
// mul r3.x, r3.z, r3.x
r3.x=(r3.zzzz*r3.xxxx).x;
// mul r6.xyz, c14.xyww, v4.y
r6.xyz=(uP[14].xyww*v4.yyyy).xyz;
// mad r6.xyz, v4.x, c13.xyww, r6
r6.xyz=(sm3Mad(v4.xxxx,uP[13].xyww,r6.xyzw)).xyz;
// mad r6.xyz, v4.z, c15.xyww, r6
r6.xyz=(sm3Mad(v4.zzzz,uP[15].xyww,r6.xyzw)).xyz;
// add r6.xyz, r6, c16.xyww
r6.xyz=(r6.xyzw+uP[16].xyww).xyz;
// rcp r3.z, r6.z
r3.z=(vec4(1.0)/r6.zzzz).z;
// mul r6.xy, r3.z, r6
r6.xy=(r3.zzzz*r6.xyzw).xy;
// mad r6.xy, r6, c0, c0.y
r6.xy=(sm3Mad(r6.xyzw,c0.xyzw,c0.yyyy)).xy;
// mad r1.yz, r1, c0.z, r6.xxyw
r1.yz=(sm3Mad(r1.xyzw,c0.zzzz,r6.xxyw)).yz;
// mad r6.xy, r0.w, c28.x, r1.yzzw
r6.xy=(sm3Mad(r0.wwww,c28.xxxx,r1.yzzw)).xy;
// texld r6, r6, s4
r6=(texture(s4,(r6.xyzw).xy)).xyzw;
// mad r7.xy, r0.w, c1, r1.yzzw
r7.xy=(sm3Mad(r0.wwww,c1.xyzw,r1.yzzw)).xy;
// texld r7, r7, s4
r7=(texture(s4,(r7.xyzw).xy)).xyzw;
// mul r7, r7, c0.w
r7=(r7.xyzw*c0.wwww).xyzw;
// mad r6, r6, c0.w, r7
r6=(sm3Mad(r6.xyzw,c0.wwww,r7.xyzw)).xyzw;
// mad r7.xy, r0.w, c1.yzzw, r1.yzzw
r7.xy=(sm3Mad(r0.wwww,c1.yzzw,r1.yzzw)).xy;
// texld r7, r7, s4
r7=(texture(s4,(r7.xyzw).xy)).xyzw;
// mad r6, r7, c0.w, r6
r6=(sm3Mad(r7.xyzw,c0.wwww,r6.xyzw)).xyzw;
// mad r1.yz, r0.w, -c28.x, r1
r1.yz=(sm3Mad(r0.wwww,-c28.xxxx,r1.xyzw)).yz;
// texld r7, r1.yzzw, s4
r7=(texture(s4,(r1.yzzw).xy)).xyzw;
// mad r6, r7, c0.w, r6
r6=(sm3Mad(r7.xyzw,c0.wwww,r6.xyzw)).xyzw;
// log r7.x, r6.x
r7.x=(sm3Log(r6.xxxx)).x;
// log r7.y, r6.y
r7.y=(sm3Log(r6.yyyy)).y;
// log r7.z, r6.z
r7.z=(sm3Log(r6.zzzz)).z;
// mul r6.xyz, r7, c1.w
r6.xyz=(r7.xyzw*c1.wwww).xyz;
// exp r7.x, r6.x
r7.x=(exp2(r6.xxxx)).x;
// exp r7.y, r6.y
r7.y=(exp2(r6.yyyy)).y;
// exp r7.z, r6.z
r7.z=(exp2(r6.zzzz)).z;
// dp3 r0.w, r9, r4
r0.w=sm3Mad(vec4(waterViewHeight),vec4(waterViewInverse),sm3Mad(r9.yyyy,r4.yyyy,r9.xxxx*r4.xxxx)).x;
// add r0.w, -r0.w, c2.w
r0.w=(-r0.wwww+c2.wwww).w;
// mul r1.y, r0.w, r0.w
r1.y=(r0.wwww*r0.wwww).y;
// mul r1.y, r1.y, r1.y
r1.y=(r1.yyyy*r1.yyyy).y;
// mul r0.w, r0.w, r1.y
r0.w=(r0.wwww*r1.yyyy).w;
// mul r4.xyz, r5.w, c7
r4.xyz=(r5.wwww*uP[7].xyzw).xyz;
// cmp r4.xyz, -r5.z, r4, -c2.y
r4.xyz=(mix(-c2.yyyy,r4.xyzw,greaterThanEqual(-r5.zzzz,vec4(0.0)))).xyz;
// mul r5.yzw, r3.x, c8.xxyz
r5.yzw=(r3.xxxx*uP[8].xxyz).yzw;
// cmp r1.yzw, -r1.w, r5, -c2.y
r1.yzw=(mix(-c2.yyyy,r5.xyzw,greaterThanEqual(-r1.wwww,vec4(0.0)))).yzw;
// add r1.yzw, r1, r4.xxyz
r1.yzw=(r1.xyzw+r4.xxyz).yzw;
// mad r1.yzw, r1, r0.w, r2.xxyz
r1.yzw=(sm3Mad(r1.xyzw,r0.wwww,r2.xxyz)).yzw;
// mul r4.xyz, r2, c7
r4.xyz=(r2.xyzw*uP[7].xyzw).xyz;
// mul r3.xzw, r3.w, r4.xyyz
r3.xzw=(r3.wwww*r4.xyyz).xzw;
// cmp r3.xzw, -r5.x, r3, -c2.y
r3.xzw=(mix(-c2.yyyy,r3.xyzw,greaterThanEqual(-r5.xxxx,vec4(0.0)))).xzw;
// mad r1.yzw, r1, r0.y, r3.xxzw
r1.yzw=(sm3Mad(r1.xyzw,r0.yyyy,r3.xxzw)).yzw;
// mad r1.yzw, r7.xxyz, c27.x, r1
r1.yzw=(sm3Mad(r7.xxyz,uP[27].xxxx,r1.xyzw)).yzw;
// mul r0.y, r3.y, r3.y
r0.y=(r3.yyyy*r3.yyyy).y;
// mul r0.y, r3.y, r0.y
r0.y=(r3.yyyy*r0.yyyy).y;
// mul r0.y, r1.x, r0.y
r0.y=(r1.xxxx*r0.yyyy).y;
// mad r1.xyz, r0.y, r2, r1.yzww
r1.xyz=(sm3Mad(r0.yyyy,r2.xyzw,r1.yzww)).xyz;
// mul r2.xyz, c18.xyww, v4.y
r2.xyz=(uP[18].xyww*v4.yyyy).xyz;
// mad r2.xyz, v4.x, c17.xyww, r2
r2.xyz=(sm3Mad(v4.xxxx,uP[17].xyww,r2.xyzw)).xyz;
// mad r2.xyz, v4.z, c19.xyww, r2
r2.xyz=(sm3Mad(v4.zzzz,uP[19].xyww,r2.xyzw)).xyz;
// add r2.xyz, r2, c20.xyww
r2.xyz=(r2.xyzw+uP[20].xyww).xyz;
// rcp r0.y, r2.z
r0.y=(vec4(1.0)/r2.zzzz).y;
// mul r0.yw, r0.y, r2.xxzy
r0.yw=(r0.yyyy*r2.xxzy).yw;
// mad r0.yw, r0, c0.xxzy, c0.y
r0.yw=(sm3Mad(r0.xyzw,c0.xxzy,c0.yyyy)).yw;
// texld r3, r0.ywzw, s13
r3=(texture(s13,(r0.ywzw).xy)).xyzw;
// mov_sat r3.x, r3.x
r3.x=(clamp(r3.xxxx,0.0,1.0)).x;
// mul r0.y, r3.x, c25.x
r0.y=(r3.xxxx*uP[25].xxxx).y;
// mul r2.xyz, r0.y, r1
r2.xyz=(r0.yyyy*r1.xyzw).xyz;
// mad r1.xyz, r2, c3.x, r1
r1.xyz=(sm3Mad(r2.xyzw,c3.xxxx,r1.xyzw)).xyz;
// mad_sat oC0.w, r6.w, r2.w, r2.w
oC0.w=(clamp(sm3Mad(r6.wwww,r2.wwww,r2.wwww),0.0,1.0)).w;
// log r2.x, r1.x
r2.x=(sm3Log(r1.xxxx)).x;
// log r2.y, r1.y
r2.y=(sm3Log(r1.yyyy)).y;
// log r2.z, r1.z
r2.z=(sm3Log(r1.zzzz)).z;
// mul r1.xyz, r2, c3.y
r1.xyz=(r2.xyzw*c3.yyyy).xyz;
// exp r2.x, r1.x
r2.x=(exp2(r1.xxxx)).x;
// exp r2.y, r1.y
r2.y=(exp2(r1.yyyy)).y;
// exp r2.z, r1.z
r2.z=(exp2(r1.zzzz)).z;
// texld r1, v6, s9
r1=(texture(s9,(v6.xyzw).xy)).xyzw;
// add r0.y, -r1.w, c2.w
r0.y=(-r1.wwww+c2.wwww).y;
// mad r1.xyz, r2, r0.y, r1
r1.xyz=(sm3Mad(r2.xyzw,r0.yyyy,r1.xyzw)).xyz;
// add r1.xyz, r1, c3.z
r1.xyz=(r1.xyzw+c3.zzzz).xyz;
// mad r1.xyz, r0.x, r1, c3.w
r1.xyz=(sm3Mad(r0.xxxx,r1.xyzw,c3.wwww)).xyz;
// texld r2, r1, s11
r2=(texture(s11,(r1.xyzw).xyz)).xyzw;
// mad r0.x, r0.z, -r0.x, r0.x
r0.x=(sm3Mad(r0.zzzz,-r0.xxxx,r0.xxxx)).x;
// lrp r3.xyz, r0.x, r2, r1
r3.xyz=(sm3Lrp(r0.xxxx,r2.xyzw,r1.xyzw)).xyz;
// rcp r0.x, v5.z
r0.x=(vec4(1.0)/v5.zzzz).x;
// mul r0.x, r0.x, v5.y
r0.x=(r0.xxxx*v5.yyyy).x;
// mad r0.x, r0.x, c28.w, c28.w
r0.x=sm3Mad(vec4(1.0)/v5.zzzz,v5.yyyy*c28.wwww,c28.wwww).x;
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
// mul r0.z, r0.z, -c29.y
r0.z=(r0.zzzz*-c29.yyyy).z;
// exp r0.z, r0.z
r0.z=(exp2(r0.zzzz)).z;
// add r0.z, -r0.z, c2.w
r0.z=(-r0.zzzz+c2.wwww).z;
// mul r0.y, r0.z, r0.y
r0.y=(r0.zzzz*r0.yyyy).y;
// rcp r0.z, r0.w
r0.z=(vec4(1.0)/r0.wwww).z;
// mul r0.y, r0.z, r0.y
r0.y=(r0.zzzz*r0.yyyy).y;
// mul r0.y, r0.y, -c29.y
r0.y=(r0.yyyy*-c29.yyyy).y;
// exp_sat r0.y, r0.y
r0.y=(clamp(exp2(r0.yyyy),0.0,1.0)).y;
// add r0.y, -r0.y, c2.w
r0.y=(-r0.yyyy+c2.wwww).y;
// mul r0.x, r0.y, r0.x
r0.x=(r0.yyyy*r0.xxxx).x;
// add r0.yzw, -r3.xxyz, c26.xxyz
r0.yzw=(-r3.xxyz+uP[26].xxyz).yzw;
// mad oC0.xyz, r0.x, r0.yzww, r3
oC0.xyz=(sm3Mad(r0.xxxx,r0.yzww,r3.xyzw)).xyz;
outColor=oC0;
}
)GLSL";
constexpr std::string_view pixel5=R"GLSL(#version 460 core
in vec2 waterT0;in vec2 waterT1;in vec2 waterT2;in vec2 waterT3;in vec3 waterWorld;in vec3 waterClip;in vec4 waterT6;
uniform vec4 uP[30];layout(location=0) out vec4 outColor;

vec4 sm3Mad(vec4 a,vec4 b,vec4 c){precise dvec4 v=dvec4(a)*dvec4(b)+dvec4(c);return vec4(v);}
vec4 sm3Lrp(vec4 a,vec4 b,vec4 c){precise vec4 x=b-c;return sm3Mad(a,x,c);}
float sm3Log(float a){return a==0.0 ? -3.402823466e38 : log2(abs(a));}
vec4 sm3Log(vec4 a){return vec4(sm3Log(a.x),sm3Log(a.y),sm3Log(a.z),sm3Log(a.w));}
vec4 sm3Dp3(vec4 a,vec4 b){precise float v=a.x*b.x;v=float(double(a.y)*double(b.y)+double(v));v=float(double(a.z)*double(b.z)+double(v));return vec4(v);}

vec4 sm3Dp2Add(vec4 a,vec4 b,vec4 c){precise float v=a.x*b.x;v=float(double(a.y)*double(b.y)+double(v));v=v+c.x;return vec4(v);}
vec4 sm3Dp4(vec4 a,vec4 b){precise float v=a.x*b.x;v=float(double(a.y)*double(b.y)+double(v));v=float(double(a.z)*double(b.z)+double(v));v=float(double(a.w)*double(b.w)+double(v));return vec4(v);}

void main(){
const vec4 c0=vec4(1.0,0.0,0.0,0.0);
vec4 v0=vec4(waterT0,0.0,0.0);
precise vec4 oC0;
// mad oC0, v0.x, c0.xxyy, c0.yyyx
oC0=(sm3Mad(v0.xxxx,c0.xxyy,c0.yyyx)).xyzw;
outColor=oC0;
}
)GLSL";
}
std::string_view water_surface_vertex_shader(){return vertex;}
std::string_view water_surface_fragment_shader(unsigned selector){
    switch(selector){case 0:return pixel0;case 2:return pixel2;case 3:return pixel3;case 5:return pixel5;}
    throw std::invalid_argument("Invalid water surface shader selector");
}
} // namespace civ5::graphics
