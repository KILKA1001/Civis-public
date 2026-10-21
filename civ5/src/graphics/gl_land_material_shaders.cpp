#include "civ5/graphics/gl_land_material.hpp"
#include <stdexcept>
namespace civ5::graphics {
namespace {
constexpr std::string_view displaced=R"GLSL(#version 460 core
layout(location=0) in vec3 aPosition;layout(location=1) in vec4 aNormal;layout(location=2) in vec2 aUv;
uniform vec4 uV[40];
out vec4 landT0;out vec4 landT1;out vec3 landWorld;out vec2 landNdc;out vec4 tfClip;out vec4 tfWorld;
layout(binding=10) uniform sampler2D vss0;

vec4 sm3Mad(vec4 a,vec4 b,vec4 c){precise dvec4 v=dvec4(a)*dvec4(b)+dvec4(c);return vec4(v);}
vec4 sm3Lrp(vec4 a,vec4 b,vec4 c){precise vec4 x=b-c;return sm3Mad(a,x,c);}
float sm3Log(float a){return a==0.0 ? -3.402823466e38 : log2(abs(a));}
vec4 sm3Log(vec4 a){return vec4(sm3Log(a.x),sm3Log(a.y),sm3Log(a.z),sm3Log(a.w));}
vec4 sm3Dp3(vec4 a,vec4 b){precise float v=a.x*b.x;v=float(double(a.y)*double(b.y)+double(v));v=float(double(a.z)*double(b.z)+double(v));return vec4(v);}

void main(){
const vec4 c8=vec4(0.5,0.0,255.0,0.0);
const vec4 c10=vec4(0.5,-0.5,1.0,0.0);
vec4 v0=vec4(aPosition,1.0);
vec4 v1=aNormal;
vec4 v2=vec4(aUv,0.0,1.0);
precise vec4 o0;
precise vec4 o1;
precise vec4 o2;
precise vec4 o3;
precise vec4 o4;
precise vec4 o5;
precise vec4 r0;
precise vec4 r1;
precise vec4 r2;
// add r0.xy, c39, v0
r0.xy=(uV[39].xyzw+v0.xyzw).xy;
// add r0.zw, r0.xyxy, c39
r0.zw=(r0.xyxy+uV[39].xyzw).zw;
// mul r0.xy, r0, c16
r0.xy=(r0.xyzw*uV[16].xyzw).xy;
// add r0.zw, r0, c8.x
r0.zw=(r0.xyzw+c8.xxxx).zw;
// mul r1.xy, r0.zwzw, c22
r1.xy=(r0.zwzw*uV[22].xyzw).xy;
// mov r1.zw, c8.y
r1.zw=(c8.yyyy).zw;
// texldl r1, r1, s0
r1=(textureLod(vss0,(r1.xyzw).xy ,(r1.xyzw).w)).xyzw;
// mul r0.z, r1.w, c16.z
r0.z=(r1.wwww*uV[16].zzzz).z;
// mul r0.z, r0.z, c8.z
r0.z=(r0.zzzz*c8.zzzz).z;
// mul r1, r0.y, c36
r1=(r0.yyyy*uV[36].xyzw).xyzw;
// mad r1, r0.x, c35, r1
r1=(sm3Mad(r0.xxxx,uV[35].xyzw,r1.xyzw)).xyzw;
// mad r0, r0.z, c37, r1
r0=(sm3Mad(r0.zzzz,uV[37].xyzw,r1.xyzw)).xyzw;
// add r0, r0, c38
r0=(r0.xyzw+uV[38].xyzw).xyzw;
// add r1.xy, r0, c9.zwzw
r1.xy=(r0.xyzw+uV[9].zwzw).xy;
// mul o2.zw, r1.xyxy, c9.xyxy
o2.zw=(r1.xyxy*uV[9].xyxy).zw;
// mul r1, r0.y, c1
r1=(r0.yyyy*uV[1].xyzw).xyzw;
// mad r1, r0.x, c0, r1
r1=(sm3Mad(r0.xxxx,uV[0].xyzw,r1.xyzw)).xyzw;
// mad r1, r0.z, c2, r1
r1=(sm3Mad(r0.zzzz,uV[2].xyzw,r1.xyzw)).xyzw;
// mad r1, r0.w, c3, r1
r1=(sm3Mad(r0.wwww,uV[3].xyzw,r1.xyzw)).xyzw;
// rcp r2.x, r1.w
r2.x=(vec4(1.0)/r1.wwww).x;
// mul o5.xy, r1, r2.x
o5.xy=(r1.xyzw*r2.xxxx).xy;
// mov o0, r1
o0=(r1.xyzw).xyzw;
// mov o1.xyz, v1
o1.xyz=(v1.xyzw).xyz;
// mov o2.xy, v2
o2.xy=(v2.xyzw).xy;
// mul r1, r0.y, c5
r1=(r0.yyyy*uV[5].xyzw).xyzw;
// mad r1, r0.x, c4, r1
r1=(sm3Mad(r0.xxxx,uV[4].xyzw,r1.xyzw)).xyzw;
// mad r1, r0.z, c6, r1
r1=(sm3Mad(r0.zzzz,uV[6].xyzw,r1.xyzw)).xyzw;
// mad r1, r0.w, c7, r1
r1=(sm3Mad(r0.wwww,uV[7].xyzw,r1.xyzw)).xyzw;
// mov o4.xyz, r0
o4.xyz=(r0.xyzw).xyz;
// mad o3, r1, c10.xyzz, c10.xxww
o3=(sm3Mad(r1.xyzw,c10.xyzz,c10.xxww)).xyzw;
landT0=o2;landT1=o3;landWorld=o4.xyz;landNdc=o5.xy;tfClip=o0;tfWorld=vec4(o4.xyz,1.0);
// API Y conversion; draw supplies native depth and pixel-center viewport.
gl_Position=vec4(o0.x,-o0.y,o0.z,o0.w);
}
)GLSL";
constexpr std::string_view positioned=R"GLSL(#version 460 core
layout(location=0) in vec3 aPosition;layout(location=1) in vec4 aNormal;layout(location=2) in vec2 aUv;
uniform vec4 uV[40];
out vec4 landT0;out vec4 landT1;out vec3 landWorld;out vec2 landNdc;out vec4 tfClip;out vec4 tfWorld;

vec4 sm3Mad(vec4 a,vec4 b,vec4 c){precise dvec4 v=dvec4(a)*dvec4(b)+dvec4(c);return vec4(v);}
vec4 sm3Lrp(vec4 a,vec4 b,vec4 c){precise vec4 x=b-c;return sm3Mad(a,x,c);}
float sm3Log(float a){return a==0.0 ? -3.402823466e38 : log2(abs(a));}
vec4 sm3Log(vec4 a){return vec4(sm3Log(a.x),sm3Log(a.y),sm3Log(a.z),sm3Log(a.w));}
vec4 sm3Dp3(vec4 a,vec4 b){precise float v=a.x*b.x;v=float(double(a.y)*double(b.y)+double(v));v=float(double(a.z)*double(b.z)+double(v));return vec4(v);}

void main(){
const vec4 c8=vec4(0.5,-0.5,1.0,0.0);
vec4 v0=vec4(aPosition,1.0);
vec4 v1=aNormal;
vec4 v2=vec4(aUv,0.0,1.0);
precise vec4 o0;
precise vec4 o1;
precise vec4 o2;
precise vec4 o3;
precise vec4 o4;
precise vec4 o5;
precise vec4 r0;
precise vec4 r1;
precise vec4 r2;
// mul r0, c36, v0.y
r0=(uV[36].xyzw*v0.yyyy).xyzw;
// mad r0, v0.x, c35, r0
r0=(sm3Mad(v0.xxxx,uV[35].xyzw,r0.xyzw)).xyzw;
// mad r0, v0.z, c37, r0
r0=(sm3Mad(v0.zzzz,uV[37].xyzw,r0.xyzw)).xyzw;
// add r0, r0, c38
r0=(r0.xyzw+uV[38].xyzw).xyzw;
// add r1.xy, r0, c9.zwzw
r1.xy=(r0.xyzw+uV[9].zwzw).xy;
// mul o2.zw, r1.xyxy, c9.xyxy
o2.zw=(r1.xyxy*uV[9].xyxy).zw;
// mul r1, r0.y, c1
r1=(r0.yyyy*uV[1].xyzw).xyzw;
// mad r1, r0.x, c0, r1
r1=(sm3Mad(r0.xxxx,uV[0].xyzw,r1.xyzw)).xyzw;
// mad r1, r0.z, c2, r1
r1=(sm3Mad(r0.zzzz,uV[2].xyzw,r1.xyzw)).xyzw;
// mad r1, r0.w, c3, r1
r1=(sm3Mad(r0.wwww,uV[3].xyzw,r1.xyzw)).xyzw;
// rcp r2.x, r1.w
r2.x=(vec4(1.0)/r1.wwww).x;
// mul o5.xy, r1, r2.x
o5.xy=(r1.xyzw*r2.xxxx).xy;
// mov o0, r1
o0=(r1.xyzw).xyzw;
// mov o1.xyz, v1
o1.xyz=(v1.xyzw).xyz;
// mov o2.xy, v2
o2.xy=(v2.xyzw).xy;
// mul r1, r0.y, c5
r1=(r0.yyyy*uV[5].xyzw).xyzw;
// mad r1, r0.x, c4, r1
r1=(sm3Mad(r0.xxxx,uV[4].xyzw,r1.xyzw)).xyzw;
// mad r1, r0.z, c6, r1
r1=(sm3Mad(r0.zzzz,uV[6].xyzw,r1.xyzw)).xyzw;
// mad r1, r0.w, c7, r1
r1=(sm3Mad(r0.wwww,uV[7].xyzw,r1.xyzw)).xyzw;
// mov o4.xyz, r0
o4.xyz=(r0.xyzw).xyz;
// mad o3, r1, c8.xyzz, c8.xxww
o3=(sm3Mad(r1.xyzw,c8.xyzz,c8.xxww)).xyzw;
landT0=o2;landT1=o3;landWorld=o4.xyz;landNdc=o5.xy;tfClip=o0;tfWorld=vec4(o4.xyz,1.0);
// API Y conversion; draw supplies native depth and pixel-center viewport.
gl_Position=vec4(o0.x,-o0.y,o0.z,o0.w);
}
)GLSL";
constexpr std::string_view specular=R"GLSL(#version 460 core
uniform vec4 uP[23];in vec4 landT0;in vec4 landT1;in vec3 landWorld;in vec2 landNdc;layout(location=0) out vec4 outColor;
layout(binding=0) uniform sampler2D s0;
layout(binding=1) uniform sampler2D s1;
layout(binding=2) uniform sampler2D s2;
layout(binding=3) uniform sampler2D s3;
layout(binding=4) uniform sampler2D s4;
layout(binding=5) uniform sampler2D s5;
layout(binding=6) uniform sampler3D s6;
layout(binding=7) uniform sampler3D s7;
layout(binding=8) uniform sampler2D s8;
layout(binding=9) uniform sampler2D s9;

vec4 sm3Mad(vec4 a,vec4 b,vec4 c){precise dvec4 v=dvec4(a)*dvec4(b)+dvec4(c);return vec4(v);}
vec4 sm3Lrp(vec4 a,vec4 b,vec4 c){precise vec4 x=b-c;return sm3Mad(a,x,c);}
float sm3Log(float a){return a==0.0 ? -3.402823466e38 : log2(abs(a));}
vec4 sm3Log(vec4 a){return vec4(sm3Log(a.x),sm3Log(a.y),sm3Log(a.z),sm3Log(a.w));}
vec4 sm3Dp3(vec4 a,vec4 b){precise float v=a.x*b.x;v=float(double(a.y)*double(b.y)+double(v));v=float(double(a.z)*double(b.z)+double(v));return vec4(v);}

void main(){
const vec4 c0=vec4(0.5,-0.5,0.0,2.20000005);
const vec4 c1=vec4(0.454545468,1.0,1.44269502,0.0);
vec4 v0=landT0;
vec4 v1=landT1;
vec4 v2=vec4(landWorld,0.0);
vec4 v3=vec4(landNdc,0.0,0.0);
precise vec4 r0;
precise vec4 r1;
precise vec4 r2;
precise vec4 r3;
precise vec4 r4;
precise vec4 oC0;
// mad r0.xy, v3, c0.x, c0.x
r0.xy=(sm3Mad(v3.xyzw,c0.xxxx,c0.xxxx)).xy;
// texld r0, r0, s9
r0=(texture(s9,(r0.xyzw).xy)).xyzw;
// texld r1, v0, s8
r1=(texture(s8,(v0.xyzw).xy)).xyzw;
// log r2.x, r1.x
r2.x=(sm3Log(r1.xxxx)).x;
// log r2.y, r1.y
r2.y=(sm3Log(r1.yyyy)).y;
// log r2.z, r1.z
r2.z=(sm3Log(r1.zzzz)).z;
// mul r1.xyz, r2, c0.w
r1.xyz=(r2.xyzw*c0.wwww).xyz;
// exp r2.x, r1.x
r2.x=(exp2(r1.xxxx)).x;
// exp r2.y, r1.y
r2.y=(exp2(r1.yyyy)).y;
// exp r2.z, r1.z
r2.z=(exp2(r1.zzzz)).z;
// texld r1, v0, s0
r1=(texture(s0,(v0.xyzw).xy)).xyzw;
// mov r3.z, c0.z
r3.z=(c0.zzzz).z;
// cmp r1.xyz, -c12.x, r3.z, r1
r1.xyz=(mix(r1.xyzw,r3.zzzz,greaterThanEqual(-uP[12].xxxx,vec4(0.0)))).xyz;
// log r3.x, r1.x
r3.x=(sm3Log(r1.xxxx)).x;
// log r3.y, r1.y
r3.y=(sm3Log(r1.yyyy)).y;
// log r3.z, r1.z
r3.z=(sm3Log(r1.zzzz)).z;
// mul r3.xyz, r3, c0.w
r3.xyz=(r3.xyzw*c0.wwww).xyz;
// exp r4.x, r3.x
r4.x=(exp2(r3.xxxx)).x;
// exp r4.y, r3.y
r4.y=(exp2(r3.yyyy)).y;
// exp r4.z, r3.z
r4.z=(exp2(r3.zzzz)).z;
// mad r0.xyz, r2, r0, r4
r0.xyz=(sm3Mad(r2.xyzw,r0.xyzw,r4.xyzw)).xyz;
// log r2.x, r0.x
r2.x=(sm3Log(r0.xxxx)).x;
// log r2.y, r0.y
r2.y=(sm3Log(r0.yyyy)).y;
// log r2.z, r0.z
r2.z=(sm3Log(r0.zzzz)).z;
// mul r0.xyz, r2, c1.x
r0.xyz=(r2.xyzw*c1.xxxx).xyz;
// exp r2.x, r0.x
r2.x=(exp2(r0.xxxx)).x;
// exp r2.y, r0.y
r2.y=(exp2(r0.yyyy)).y;
// exp r2.z, r0.z
r2.z=(exp2(r0.zzzz)).z;
// cmp r0.xyz, -c11.x, r1, r2
r0.xyz=(mix(r2.xyzw,r1.xyzw,greaterThanEqual(-uP[11].xxxx,vec4(0.0)))).xyz;
// texld r1, r0, s7
r1=(texture(s7,(r0.xyzw).xyz)).xyzw;
// mul r2.xyz, c16.xyww, v2.y
r2.xyz=(uP[16].xyww*v2.yyyy).xyz;
// mad r2.xyz, v2.x, c15.xyww, r2
r2.xyz=(sm3Mad(v2.xxxx,uP[15].xyww,r2.xyzw)).xyz;
// mad r2.xyz, v2.z, c17.xyww, r2
r2.xyz=(sm3Mad(v2.zzzz,uP[17].xyww,r2.xyzw)).xyz;
// add r2.xyz, r2, c18.xyww
r2.xyz=(r2.xyzw+uP[18].xyww).xyz;
#ifdef CIV5_MEASURED_SHADOW_AFFINE
// Device-measured T -> FMA(X) -> FMA(Z) -> FMA(Y); not native matrix layout.
precise vec4 shadowAffine=sm3Mad(v2.xxxx,uP[15],uP[18]);
shadowAffine=sm3Mad(v2.zzzz,uP[17],shadowAffine);
shadowAffine=sm3Mad(v2.yyyy,uP[16],shadowAffine);
r2.xyz=shadowAffine.xyw;
#endif
// rcp r0.w, r2.z
r0.w=(vec4(1.0)/r2.zzzz).w;
// mul r2.xy, r0.w, r2
r2.xy=(r0.wwww*r2.xyzw).xy;
// mad r2.xy, r2, c0, c0.y
r2.xy=(sm3Mad(r2.xyzw,c0.xyzw,c0.yyyy)).xy;
// texld r2, r2, s3
r2=(texture(s3,(r2.xyzw).xy)).xyzw;
// mov_sat r2.x, r2.x
r2.x=(clamp(r2.xxxx,0.0,1.0)).x;
// mul r0.w, r2.x, c9.x
r0.w=(r2.xxxx*uP[9].xxxx).w;
// lrp r2.xyz, r0.w, r1, r0
r2.xyz=(sm3Lrp(r0.wwww,r1.xyzw,r0.xyzw)).xyz;
// texld r0, v1, s1
r0=(texture(s1,(v1.xyzw).xy)).xyzw;
// add r0.w, -r0.w, c1.y
r0.w=(-r0.wwww+c1.yyyy).w;
// mad r0.xyz, r2, r0.w, r0
r0.xyz=(sm3Mad(r2.xyzw,r0.wwww,r0.xyzw)).xyz;
// texld r1, v0.zwzw, s5
r1=(texture(s5,(v0.zwzw).xy)).xyzw;
// texld r2, v1, s4
r2=(texture(s4,(v1.xyzw).xy)).xyzw;
// lrp r3.xyz, r2.x, r0, r1
r3.xyz=(sm3Lrp(r2.xxxx,r0.xyzw,r1.xyzw)).xyz;
// mad r0.x, r2.z, -r2.x, r2.x
r0.x=(sm3Mad(r2.zzzz,-r2.xxxx,r2.xxxx)).x;
// texld r1, r3, s6
r1=(texture(s6,(r3.xyzw).xyz)).xyzw;
// lrp r2.xyz, r0.x, r1, r3
r2.xyz=(sm3Lrp(r0.xxxx,r1.xyzw,r3.xyzw)).xyz;
// add r0.xyz, -r2, c21
r0.xyz=(-r2.xyzw+uP[21].xyzw).xyz;
// add r1.xyz, -c3, v2
r1.xyz=(-uP[3].xyzw+v2.xyzw).xyz;
// mul r0.w, r1.z, c22.x
r0.w=(r1.zzzz*uP[22].xxxx).w;
// mul r0.w, r0.w, c1.z
r0.w=(r0.wwww*c1.zzzz).w;
// exp r0.w, r0.w
r0.w=(exp2(r0.wwww)).w;
// add r0.w, -r0.w, c1.y
r0.w=(-r0.wwww+c1.yyyy).w;
// dp3 r1.x, r1, r1
r1.x=(sm3Dp3(r1.xyzw,r1.xyzw)).x;
// rcp r1.y, r1.z
r1.y=(vec4(1.0)/r1.zzzz).y;
// mul r1.x, r1.x, c22.y
r1.x=(r1.xxxx*uP[22].yyyy).x;
// mul r0.w, r0.w, r1.x
r0.w=(r0.wwww*r1.xxxx).w;
// mul r0.w, r1.y, r0.w
r0.w=(r1.yyyy*r0.wwww).w;
// mul r0.w, r0.w, c1.z
r0.w=(r0.wwww*c1.zzzz).w;
// exp_sat r0.w, r0.w
r0.w=(clamp(exp2(r0.wwww),0.0,1.0)).w;
// add r0.w, -r0.w, c1.y
r0.w=(-r0.wwww+c1.yyyy).w;
// mad r1.x, v3.y, c0.x, c0.x
r1.x=(sm3Mad(v3.yyyy,c0.xxxx,c0.xxxx)).x;
// mov r1.y, c22.w
r1.y=(uP[22].wwww).y;
// texld r1, r1, s2
r1=(texture(s2,(r1.xyzw).xy)).xyzw;
// mul r0.w, r0.w, r1.x
r0.w=(r0.wwww*r1.xxxx).w;
// mad oC0.xyz, r0.w, r0, r2
oC0.xyz=(sm3Mad(r0.wwww,r0.xyzw,r2.xyzw)).xyz;
// mov oC0.w, c1.y
oC0.w=(c1.yyyy).w;
outColor=oC0;
}
)GLSL";
constexpr std::string_view diffuse=R"GLSL(#version 460 core
uniform vec4 uP[23];in vec4 landT0;in vec4 landT1;in vec3 landWorld;in vec2 landNdc;layout(location=0) out vec4 outColor;
layout(binding=0) uniform sampler2D s0;
layout(binding=1) uniform sampler2D s1;
layout(binding=2) uniform sampler2D s2;
layout(binding=3) uniform sampler2D s3;
layout(binding=4) uniform sampler2D s4;
layout(binding=5) uniform sampler2D s5;
layout(binding=6) uniform sampler3D s6;
layout(binding=7) uniform sampler3D s7;

vec4 sm3Mad(vec4 a,vec4 b,vec4 c){precise dvec4 v=dvec4(a)*dvec4(b)+dvec4(c);return vec4(v);}
vec4 sm3Lrp(vec4 a,vec4 b,vec4 c){precise vec4 x=b-c;return sm3Mad(a,x,c);}
float sm3Log(float a){return a==0.0 ? -3.402823466e38 : log2(abs(a));}
vec4 sm3Log(vec4 a){return vec4(sm3Log(a.x),sm3Log(a.y),sm3Log(a.z),sm3Log(a.w));}
vec4 sm3Dp3(vec4 a,vec4 b){precise float v=a.x*b.x;v=float(double(a.y)*double(b.y)+double(v));v=float(double(a.z)*double(b.z)+double(v));return vec4(v);}

void main(){
const vec4 c0=vec4(0.5,-0.5,0.0,1.0);
const vec4 c1=vec4(1.44269502,0.0,0.0,0.0);
vec4 v0=landT0;
vec4 v1=landT1;
vec4 v2=vec4(landWorld,0.0);
vec4 v3=vec4(landNdc,0.0,0.0);
precise vec4 r0;
precise vec4 r1;
precise vec4 r2;
precise vec4 r3;
precise vec4 oC0;
// add r0.xyz, -c3, v2
r0.xyz=(-uP[3].xyzw+v2.xyzw).xyz;
// mul r0.w, r0.z, c22.x
r0.w=(r0.zzzz*uP[22].xxxx).w;
// mul r0.w, r0.w, c1.x
r0.w=(r0.wwww*c1.xxxx).w;
// exp r0.w, r0.w
r0.w=(exp2(r0.wwww)).w;
// add r0.w, -r0.w, c0.w
r0.w=(-r0.wwww+c0.wwww).w;
// dp3 r0.x, r0, r0
r0.x=(sm3Dp3(r0.xyzw,r0.xyzw)).x;
// rcp r0.y, r0.z
r0.y=(vec4(1.0)/r0.zzzz).y;
// mul r0.x, r0.x, c22.y
r0.x=(r0.xxxx*uP[22].yyyy).x;
// mul r0.x, r0.w, r0.x
r0.x=(r0.wwww*r0.xxxx).x;
// mul r0.x, r0.y, r0.x
r0.x=(r0.yyyy*r0.xxxx).x;
// mul r0.x, r0.x, c1.x
r0.x=(r0.xxxx*c1.xxxx).x;
// exp_sat r0.x, r0.x
r0.x=(clamp(exp2(r0.xxxx),0.0,1.0)).x;
// add r0.x, -r0.x, c0.w
r0.x=(-r0.xxxx+c0.wwww).x;
// mad r1.x, v3.y, c0.x, c0.x
r1.x=(sm3Mad(v3.yyyy,c0.xxxx,c0.xxxx)).x;
// mov r1.y, c22.w
r1.y=(uP[22].wwww).y;
// texld r1, r1, s2
r1=(texture(s2,(r1.xyzw).xy)).xyzw;
// mul r0.x, r0.x, r1.x
r0.x=(r0.xxxx*r1.xxxx).x;
// mul r0.yzw, c16.xxyw, v2.y
r0.yzw=(uP[16].xxyw*v2.yyyy).yzw;
// mad r0.yzw, v2.x, c15.xxyw, r0
r0.yzw=(sm3Mad(v2.xxxx,uP[15].xxyw,r0.xyzw)).yzw;
// mad r0.yzw, v2.z, c17.xxyw, r0
r0.yzw=(sm3Mad(v2.zzzz,uP[17].xxyw,r0.xyzw)).yzw;
// add r0.yzw, r0, c18.xxyw
r0.yzw=(r0.xyzw+uP[18].xxyw).yzw;
#ifdef CIV5_MEASURED_SHADOW_AFFINE
// Device-measured T -> FMA(X) -> FMA(Z) -> FMA(Y); not native matrix layout.
precise vec4 shadowAffine=sm3Mad(v2.xxxx,uP[15],uP[18]);
shadowAffine=sm3Mad(v2.zzzz,uP[17],shadowAffine);
shadowAffine=sm3Mad(v2.yyyy,uP[16],shadowAffine);
r0.yzw=shadowAffine.xyw;
#endif
// rcp r0.w, r0.w
r0.w=(vec4(1.0)/r0.wwww).w;
// mul r0.yz, r0.w, r0
r0.yz=(r0.wwww*r0.xyzw).yz;
// mad r0.yz, r0, c0.xxyw, c0.y
r0.yz=(sm3Mad(r0.xyzw,c0.xxyw,c0.yyyy)).yz;
// texld r1, r0.yzzw, s3
r1=(texture(s3,(r0.yzzw).xy)).xyzw;
// mov_sat r1.x, r1.x
r1.x=(clamp(r1.xxxx,0.0,1.0)).x;
// mul r0.y, r1.x, c9.x
r0.y=(r1.xxxx*uP[9].xxxx).y;
// texld r1, v0, s0
r1=(texture(s0,(v0.xyzw).xy)).xyzw;
// mov r0.z, c0.z
r0.z=(c0.zzzz).z;
// cmp r1.xyz, -c12.x, r0.z, r1
r1.xyz=(mix(r1.xyzw,r0.zzzz,greaterThanEqual(-uP[12].xxxx,vec4(0.0)))).xyz;
// texld r2, r1, s7
r2=(texture(s7,(r1.xyzw).xyz)).xyzw;
// lrp r3.xyz, r0.y, r2, r1
r3.xyz=(sm3Lrp(r0.yyyy,r2.xyzw,r1.xyzw)).xyz;
// texld r1, v1, s1
r1=(texture(s1,(v1.xyzw).xy)).xyzw;
// add r0.y, -r1.w, c0.w
r0.y=(-r1.wwww+c0.wwww).y;
// mad r0.yzw, r3.xxyz, r0.y, r1.xxyz
r0.yzw=(sm3Mad(r3.xxyz,r0.yyyy,r1.xxyz)).yzw;
// texld r1, v0.zwzw, s5
r1=(texture(s5,(v0.zwzw).xy)).xyzw;
// texld r2, v1, s4
r2=(texture(s4,(v1.xyzw).xy)).xyzw;
// lrp r3.xyz, r2.x, r0.yzww, r1
r3.xyz=(sm3Lrp(r2.xxxx,r0.yzww,r1.xyzw)).xyz;
// mad r0.y, r2.z, -r2.x, r2.x
r0.y=(sm3Mad(r2.zzzz,-r2.xxxx,r2.xxxx)).y;
// texld r1, r3, s6
r1=(texture(s6,(r3.xyzw).xyz)).xyzw;
// lrp r2.xyz, r0.y, r1, r3
r2.xyz=(sm3Lrp(r0.yyyy,r1.xyzw,r3.xyzw)).xyz;
// add r0.yzw, -r2.xxyz, c21.xxyz
r0.yzw=(-r2.xxyz+uP[21].xxyz).yzw;
// mad oC0.xyz, r0.x, r0.yzww, r2
oC0.xyz=(sm3Mad(r0.xxxx,r0.yzww,r2.xyzw)).xyz;
// mov oC0.w, c0.w
oC0.w=(c0.wwww).w;
outColor=oC0;
}
)GLSL";
constexpr std::string_view overlay=R"GLSL(#version 460 core
uniform vec4 uP[23];in vec4 landT0;in vec4 landT1;in vec3 landWorld;in vec2 landNdc;layout(location=0) out vec4 outColor;
layout(binding=1) uniform sampler2D s1;

vec4 sm3Mad(vec4 a,vec4 b,vec4 c){precise dvec4 v=dvec4(a)*dvec4(b)+dvec4(c);return vec4(v);}
vec4 sm3Lrp(vec4 a,vec4 b,vec4 c){precise vec4 x=b-c;return sm3Mad(a,x,c);}
float sm3Log(float a){return a==0.0 ? -3.402823466e38 : log2(abs(a));}
vec4 sm3Log(vec4 a){return vec4(sm3Log(a.x),sm3Log(a.y),sm3Log(a.z),sm3Log(a.w));}
vec4 sm3Dp3(vec4 a,vec4 b){precise float v=a.x*b.x;v=float(double(a.y)*double(b.y)+double(v));v=float(double(a.z)*double(b.z)+double(v));return vec4(v);}

void main(){
vec4 v0=landT1;
precise vec4 oC0;
// texld oC0, v0, s1
oC0=(texture(s1,(v0.xyzw).xy)).xyzw;
outColor=oC0;
}
)GLSL";
}
std::string_view land_material_vertex_shader(materials::LandVertexInput input) {
    switch(input){case materials::LandVertexInput::local_grid:return displaced;case materials::LandVertexInput::source_positioned:return positioned;}
    throw std::invalid_argument("Invalid LAND vertex program");
}
std::string_view land_material_fragment_shader(materials::LandPixelProgram input) {
    switch(input){case materials::LandPixelProgram::cached_specular:return specular;case materials::LandPixelProgram::cached_diffuse:return diffuse;case materials::LandPixelProgram::overlay:return overlay;}
    throw std::invalid_argument("Invalid LAND pixel program");
}
} // namespace civ5::graphics
