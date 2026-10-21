#include "civ5/graphics/gl_water_current.hpp"
namespace civ5::graphics {
namespace {
constexpr std::string_view vertex = R"GLSL(#version 460 core
out vec2 currentUv;layout(location=0) in vec3 aPosition;layout(location=1) in vec2 aUv;uniform vec2 uPixelCenter;

vec4 sm3Mad(vec4 a,vec4 b,vec4 c){precise dvec4 v=dvec4(a)*dvec4(b)+dvec4(c);return vec4(v);}
vec4 sm3Lrp(vec4 a,vec4 b,vec4 c){precise vec4 x=b-c;return sm3Mad(a,x,c);}
float sm3Log(float a){return a==0.0 ? -3.402823466e38 : log2(abs(a));}
vec4 sm3Log(vec4 a){return vec4(sm3Log(a.x),sm3Log(a.y),sm3Log(a.z),sm3Log(a.w));}
vec4 sm3Dp3(vec4 a,vec4 b){precise float v=a.x*b.x;v=float(double(a.y)*double(b.y)+double(v));v=float(double(a.z)*double(b.z)+double(v));return vec4(v);}

vec4 sm3Dp2Add(vec4 a,vec4 b,vec4 c){precise float v=a.x*b.x;v=float(double(a.y)*double(b.y)+double(v));v=v+c.x;return vec4(v);}
vec4 sm3Dp4(vec4 a,vec4 b){precise float v=a.x*b.x;v=float(double(a.y)*double(b.y)+double(v));v=float(double(a.z)*double(b.z)+double(v));v=float(double(a.w)*double(b.w)+double(v));return vec4(v);}

void main(){
const vec4 c0=vec4(1.0,0.0,0.0,0.0);
vec4 v0=vec4(aPosition,1.0);
vec4 v1=vec4(aUv,0.0,0.0);
precise vec4 o0;
precise vec4 o1;
// mad o0, v0.xyzx, c0.xxxy, c0.yyyx
o0=(sm3Mad(v0.xyzx,c0.xxxy,c0.yyyx)).xyzw;
// mov o1.xy, v1
o1.xy=(v1.xyzw).xy;
currentUv=o1.xy;
// API pixel-center/Y/depth conversion only; varyings retain native clip.
gl_Position=vec4(o0.x,-o0.y,2.0*o0.z-o0.w,o0.w);gl_Position.xy+=uPixelCenter*o0.w;
}
)GLSL";
constexpr std::string_view pixel = R"GLSL(#version 460 core
in vec2 currentUv;uniform vec4 uP[4];layout(location=0) out vec4 outColor;
layout(binding=0) uniform sampler2D s0;

vec4 sm3Mad(vec4 a,vec4 b,vec4 c){precise dvec4 v=dvec4(a)*dvec4(b)+dvec4(c);return vec4(v);}
vec4 sm3Lrp(vec4 a,vec4 b,vec4 c){precise vec4 x=b-c;return sm3Mad(a,x,c);}
float sm3Log(float a){return a==0.0 ? -3.402823466e38 : log2(abs(a));}
vec4 sm3Log(vec4 a){return vec4(sm3Log(a.x),sm3Log(a.y),sm3Log(a.z),sm3Log(a.w));}
vec4 sm3Dp3(vec4 a,vec4 b){precise float v=a.x*b.x;v=float(double(a.y)*double(b.y)+double(v));v=float(double(a.z)*double(b.z)+double(v));return vec4(v);}

vec4 sm3Dp2Add(vec4 a,vec4 b,vec4 c){precise float v=a.x*b.x;v=float(double(a.y)*double(b.y)+double(v));v=v+c.x;return vec4(v);}
vec4 sm3Dp4(vec4 a,vec4 b){precise float v=a.x*b.x;v=float(double(a.y)*double(b.y)+double(v));v=float(double(a.z)*double(b.z)+double(v));v=float(double(a.w)*double(b.w)+double(v));return vec4(v);}

void main(){
const vec4 c4=vec4(2.0,3.0,-6.0,6.0);
const vec4 c5=vec4(0.0,1.0,17.0,3721.0);
const vec4 c6=vec4(13.0,1860.5,-0.5,930.25);
const vec4 c7=vec4(1.0,-1.0,2.0,-2.0);
const vec4 c8=vec4(-1.0,1.0,0.0,0.5);
vec4 v0=vec4(currentUv,0.0,0.0);
precise vec4 r0;
precise vec4 r1;
precise vec4 r2;
precise vec4 r3;
precise vec4 r4;
precise vec4 r5;
precise vec4 oC0;
// rcp r0.x, c2.x
r0.x=(vec4(1.0)/uP[2].xxxx).x;
// mul r0, r0.x, c0.yyxx
r0=(r0.xxxx*uP[0].yyxx).xyzw;
// frc r1, r0.yyww
r1=(fract(r0.yyww)).xyzw;
// add r0, r0, -r1
r0=(r0.xyzw+-r1.xyzw).xyzw;
// rcp r1.x, r0.y
r1.x=(vec4(1.0)/r0.yyyy).x;
// mul r1.yz, r0.xyww, v0.xyxw
r1.yz=(r0.xyww*v0.xyxw).yz;
// frc r2.xy, r1.zyzw
r2.xy=(fract(r1.zyzw)).xy;
// mad r3, v0.yyxx, r0, -r2.yyxx
r3=(sm3Mad(v0.yyxx,r0.xyzw,-r2.yyxx)).xyzw;
// rcp r0.x, r0.w
r0.x=(vec4(1.0)/r0.wwww).x;
// add r3, r3, c5.xyxy
r3=(r3.xyzw+c5.xyxy).xyzw;
// mul r4, r3.xyxy, c5.z
r4=(r3.xyxy*c5.zzzz).xyzw;
// mul r1, r1.x, r4
r1=(r1.xxxx*r4.xyzw).xyzw;
// frc r1, r1
r1=(fract(r1.xyzw)).xyzw;
// mul r1, r1, r1
r1=(r1.xyzw*r1.xyzw).xyzw;
// mad r1, r1, c5.w, r3.zzww
r1=(sm3Mad(r1.xyzw,c5.wwww,r3.zzww)).xyzw;
// Measured D3D9 current-noise scale grouping; preserve the unscaled hash.
precise vec4 currentNoiseHash=r1;
// mul r1, r1, c6.x
r1=(r1.xyzw*c6.xxxx).xyzw;
precise vec4 currentNoiseScale=r0.xxxx*c6.xxxx;
// mul r0, r0.x, r1
r0=(currentNoiseHash*currentNoiseScale).xyzw;
// frc r0, r0
r0=(fract(r0.xyzw)).xyzw;
// mul r0, r0, r0
r0=(r0.xyzw*r0.xyzw).xyzw;
// mul r1, r0, c6.w
r1=(r0.xyzw*c6.wwww).xyzw;
// mul r0, r0, c6.y
r0=(r0.xyzw*c6.yyyy).xyzw;
// frc r0, r0
r0=(fract(r0.xyzw)).xyzw;
// add r0, r0, c6.z
r0=(r0.xyzw+c6.zzzz).xyzw;
// frc r1, r1
r1=(fract(r1.xyzw)).xyzw;
// add r1, r1, c6.z
r1=(r1.xyzw+c6.zzzz).xyzw;
// cmp r3, -r1, c5.x, c5.y
r3=(mix(c5.yyyy,c5.xxxx,greaterThanEqual(-r1.xyzw,vec4(0.0)))).xyzw;
// cmp r1, r1, -c5.x, -c5.y
r1=(mix(-c5.yyyy,-c5.xxxx,greaterThanEqual(r1.xyzw,vec4(0.0)))).xyzw;
// add r1, r1, r3
r1=(r1.xyzw+r3.xyzw).xyzw;
// add r3, r2.xxyy, -c5.xyxy
r3=(r2.xxyy+-c5.xyxy).xyzw;
// mul r4, r1, r3.zwzw
r4=(r1.xyzw*r3.zwzw).xyzw;
// cmp r5, -r0, c5.x, c5.y
r5=(mix(c5.yyyy,c5.xxxx,greaterThanEqual(-r0.xyzw,vec4(0.0)))).xyzw;
// cmp r0, r0, -c5.x, -c5.y
r0=(mix(-c5.yyyy,-c5.xxxx,greaterThanEqual(r0.xyzw,vec4(0.0)))).xyzw;
// add r0, r0, r5
r0=(r0.xyzw+r5.xyzw).xyzw;
// mad r3, r3.xxyy, r0, r4
r3=(sm3Mad(r3.xxyy,r0.xyzw,r4.xyzw)).xyzw;
// add r3, -r3.xyxz, r3.zwyw
r3=(-r3.xyxz+r3.zwyw).xyzw;
// mad r2.zw, r2.xyxy, c4.z, c4.w
r2.zw=(sm3Mad(r2.xyxy,c4.zzzz,c4.wwww)).zw;
// mul r4, r2.xyxy, r2
r4=(r2.xyxy*r2.xyzw).xyzw;
// mad r2, r2.xyxx, -c4.x, c4.y
r2=(sm3Mad(r2.xyxx,-c4.xxxx,c4.yyyy)).xyzw;
// mul r2, r2, r4.xyxx
r2=(r2.xyzw*r4.xyxx).xyzw;
// lrp r5.xy, r2.w, r0.zwzw, r0
r5.xy=(sm3Lrp(r2.wwww,r0.zwzw,r0.xyzw)).xy;
// lrp r5.zw, r2.y, r1.xyyw, r1.xyxz
r5.zw=(sm3Lrp(r2.yyyy,r1.xyyw,r1.xyxz)).zw;
// mad r0, r3, r4.zzww, r5
r0=(sm3Mad(r3.xyzw,r4.zzww,r5.xyzw)).xyzw;
// lrp r1, r2, r0.wyww, r0.zxzz
r1=(sm3Lrp(r2.xyzw,r0.wyww,r0.zxzz)).xyzw;
// mul r0, r1, c1.x
r0=(r1.xyzw*uP[1].xxxx).xyzw;
// mul r0, r0, -c6.z
r0=(r0.xyzw*-c6.zzzz).xyzw;
// mad r0, r0, c8.xyzz, c8.wwzy
r0=(sm3Mad(r0.xyzw,c8.xyzz,c8.wwzy)).xyzw;
// mov r1.w, c5.x
r1.w=(c5.xxxx).w;
// rcp r1.x, c0.x
r1.x=(vec4(1.0)/uP[0].xxxx).x;
// add r2.xy, r1.xwzw, v0
r2.xy=(r1.xwzw+v0.xyzw).xy;
// texld r2, r2, s0
r2=(texture(s0,(r2.xyzw).xy)).xyzw;
// mov r1.y, -r1.x
r1.y=(-r1.xxxx).y;
// mov r3.yw, r1.xyzx
r3.yw=(r1.xyzx).yw;
// rcp r3.x, c0.y
r3.x=(vec4(1.0)/uP[0].yyyy).x;
// add r4.xy, r3.yxzw, v0
r4.xy=(r3.yxzw+v0.xyzw).xy;
// add r3.yw, r3.xwzx, v0.xxzy
r3.yw=(r3.xwzx+v0.xxzy).yw;
// texld r5, r3.ywzw, s0
r5=(texture(s0,(r3.ywzw).xy)).xyzw;
// mov r2.z, r5.x
r2.z=(r5.xxxx).z;
// texld r4, r4, s0
r4=(texture(s0,(r4.xyzw).xy)).xyzw;
// mov r2.y, r4.x
r2.y=(r4.xxxx).y;
// dp3 r2.x, r2.yzxw, c7.xyww
r2.x=(sm3Dp3(r2.yzxw,c7.xyww)).x;
// add r3.yw, r1, v0.xxzy
r3.yw=(r1.xyzw+v0.xxzy).yw;
// texld r4, r3.ywzw, s0
r4=(texture(s0,(r3.ywzw).xy)).xyzw;
// mov r4.z, r4.x
r4.z=(r4.xxxx).z;
// mov r1.z, -r3.x
r1.z=(-r3.xxxx).z;
// add r3.yw, r1.xxzz, v0.xxzy
r3.yw=(r1.xxzz+v0.xxzy).yw;
// texld r5, r3.ywzw, s0
r5=(texture(s0,(r3.ywzw).xy)).xyzw;
// mov r4.y, r5.x
r4.y=(r5.xxxx).y;
// add r1.xy, r1.yzzw, v0
r1.xy=(r1.yzzw+v0.xyzw).xy;
// add r1.zw, r1.xywz, v0.xyxy
r1.zw=(r1.xywz+v0.xyxy).zw;
// texld r5, r1.zwzw, s0
r5=(texture(s0,(r1.zwzw).xy)).xyzw;
// mov r4.w, r5.x
r4.w=(r5.xxxx).w;
// texld r1, r1, s0
r1=(texture(s0,(r1.xyzw).xy)).xyzw;
// mov r4.x, r1.x
r4.x=(r1.xxxx).x;
// dp3 r1.x, r4, c7
r1.x=(sm3Dp3(r4.xyzw,c7.xyzw)).x;
// dp3 r1.y, r4.xyww, c7.xxzw
r1.y=(sm3Dp3(r4.xyww,c7.xxzw)).y;
// add r4.x, r2.x, r1.x
r4.x=(r2.xxxx+r1.xxxx).x;
// mov r3.z, c5.x
r3.z=(c5.xxxx).z;
// add r1.xz, r3.zyxw, v0.xyyw
r1.xz=(r3.zyxw+v0.xyyw).xz;
// texld r3, r1.xzzw, s0
r3=(texture(s0,(r1.xzzw).xy)).xyzw;
// mov r2.w, r3.x
r2.w=(r3.xxxx).w;
// dp3 r1.x, r2.yzww, c7.yyww
r1.x=(sm3Dp3(r2.yzww,c7.yyww)).x;
// add r4.y, r1.x, r1.y
r4.y=(r1.xxxx+r1.yyyy).y;
// mul r1.xy, r4, c3.x
r1.xy=(r4.xyzw*uP[3].xxxx).xy;
// mul r2.xyz, r1.yxyw, -c6.z
r2.xyz=(r1.yxyw*-c6.zzzz).xyz;
// dp2add r1.x, r1, r1, c5.x
r1.x=(sm3Dp2Add(r1.xyzw,r1.xyzw,c5.xxxx)).x;
// rsq r1.x, r1.x
r1.x=(inversesqrt(abs(r1.xxxx))).x;
// rcp r1.x, r1.x
r1.x=(vec4(1.0)/r1.xxxx).x;
// mad r1.yzw, r2.xxyz, c8.xxyz, c8.xwwz
r1.yzw=(sm3Mad(r2.xxyz,c8.xxyz,c8.xwwz)).yzw;
// add r2, -r0, r1.yzwx
r2=(-r0.xyzw+r1.yzwx).xyzw;
// mov_sat r1.x, r1.x
r1.x=(clamp(r1.xxxx,0.0,1.0)).x;
// mad oC0, r1.x, r2, r0
oC0=(sm3Mad(r1.xxxx,r2.xyzw,r0.xyzw)).xyzw;
outColor=oC0;
}
)GLSL";
}
std::string_view water_current_vertex_shader() { return vertex; }
std::string_view water_current_fragment_shader() { return pixel; }
}
