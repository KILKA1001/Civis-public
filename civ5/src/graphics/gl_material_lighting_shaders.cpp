#include "civ5/graphics/gl_material_blend.hpp"
#include "civ5/graphics/gl_material_lighting.hpp"

namespace civ5::graphics {
// Specialization of the retained key4B translation: only ordinary and measured
// UNORM32 modes. Fitted transition/corner hypotheses and prefix diagnostics are
// not runtime branches. Full-original-shader and old-corpus comparisons remain
// mandatory; this is not universal precision or invented zero-vector behavior.
std::string_view material_lighting_vertex_shader() {
    return R"GLSL(#version 460 core
layout(location=0) in vec4 aNormal;layout(location=1) in vec2 aUv;
uniform vec2 uPixelCenter;out vec3 vNormal;out vec2 vUv;
void main(){
    precise vec2 clip=aUv*vec2(2.0,-2.0)+vec2(-1.0,1.0);
    gl_Position=vec4(vec2(clip.x,-clip.y)+uPixelCenter,0.0,1.0);
    vNormal=aNormal.xyz;vUv=aUv;
})GLSL";
}
std::string material_lighting_fragment_shader(bool specular) {
    std::string source;
    source += R"GLSL(#version 460 core
layout(binding=0) uniform sampler2D uNormal;layout(binding=1) uniform sampler2D uAlbedo;
layout(binding=2) uniform sampler2D uShadow;layout(binding=3) uniform sampler2D uSpec;
layout(binding=4) uniform samplerCube uSun;layout(binding=5) uniform samplerCube uShade;
layout(binding=6) uniform samplerCube uCubeCoordinateProbe;uniform bool uCubeTapCoordinates;
layout(binding=8) uniform sampler3D uColorCorrect;uniform vec3 uHalf;uniform bool uCubeTapFilter;
in vec3 vNormal;in vec2 vUv;layout(location=0) out vec4 diffuse;
)GLSL";
    if (specular)
        source += "layout(location=1) out vec4 specular;\n";
    source += R"GLSL(
)GLSL";
    source += material_sampling_glsl();
    source += R"GLSL(
float nativeDp2Squared(float first,float second){
    precise float roundedProduct=first*first;
    // GLSL fma on this GL driver rounds differently from the original
    // D3D9 DP2ADD. Preserve its measured product/sum rounding explicitly.
    precise double sum=double(second)*double(second)+double(roundedProduct);
    return float(sum);
}
vec3 nativeMad(vec3 a,vec3 b,vec3 c){
    // Diagnostic explicit intermediate precision; checked against
    // the original prefix, not assumed equivalent for all floats.
    precise dvec3 sum=dvec3(a)*dvec3(b)+dvec3(c);
    return vec3(sum);
}

int cubeFace(vec3 direction){
    vec3 m=abs(direction);
    int axis=m.x>=m.y && m.x>=m.z ? 0 : m.y>=m.z ? 1 : 2;
    return axis*2+(direction[axis]<0.0 ? 1 : 0);
}
vec2 cubeFaceNumerator(vec3 direction,int face){
    if(face==0)return vec2(-direction.z,-direction.y);
    if(face==1)return vec2(direction.z,-direction.y);
    if(face==2)return vec2(direction.x,direction.z);
    if(face==3)return vec2(direction.x,-direction.z);
    if(face==4)return vec2(direction.x,-direction.y);
    return vec2(-direction.x,-direction.y);
}
vec2 cubeFaceUv(vec3 direction,int face){
    if(face==0)return vec2(-direction.z,-direction.y)/abs(direction.x);
    if(face==1)return vec2(direction.z,-direction.y)/abs(direction.x);
    if(face==2)return vec2(direction.x,direction.z)/abs(direction.y);
    if(face==3)return vec2(direction.x,-direction.z)/abs(direction.y);
    if(face==4)return vec2(direction.x,-direction.y)/abs(direction.z);
    return vec2(-direction.x,-direction.y)/abs(direction.z);
}
vec3 cubeFaceDirection(vec2 uv,int face){
    if(face==0)return vec3(1.0,-uv.y,-uv.x);
    if(face==1)return vec3(-1.0,-uv.y,uv.x);
    if(face==2)return vec3(uv.x,1.0,uv.y);
    if(face==3)return vec3(uv.x,-1.0,-uv.y);
    if(face==4)return vec3(uv.x,-uv.y,1.0);
    return vec3(-uv.x,-uv.y,-1.0);
}
vec4 cubePoint(samplerCube source,ivec2 texel,int face,int level){
    float size=float(textureSize(source,level).x);
    vec3 direction=cubeFaceDirection((vec2(texel)+.5)*(2.0/size)-1.0,face);
    if(any(lessThan(texel,ivec2(0))) || any(greaterThanEqual(texel,ivec2(int(size))))) {
        int adjacent=cubeFace(direction);
        vec2 mapped=floor((cubeFaceUv(direction,adjacent)+1.0)*(.5*size));
        direction=cubeFaceDirection((mapped+.5)*(2.0/size)-1.0,adjacent);
    }
    // Exact dyadic centers retain original compressed/sRGB GPU decode.
    return textureLod(source,direction,float(level));
}
dvec4 cubeTapLevel(samplerCube source,vec3 direction,int level,float mipWeight,out dvec4 cornerParts[3]){
    for(int i=0;i<3;i++)cornerParts[i]=dvec4(0.0);
    int face=cubeFace(direction),major=face/2;
    float size=float(textureSize(source,level).x);
    precise vec2 location=(cubeFaceUv(direction,face)+1.0)*(.5*size)-.5;
    ivec2 low=ivec2(floor(location));
    vec2 weight=roundEven(fract(location)*256.0)/256.0;
    if(uCubeTapCoordinates) {
        // A binary auxiliary cube observes this GPU's spatial weights
        // independently of authored colors. Cross-API synthetic probes
        // validate the coordinate path; this remains opt-in for the full
        // original pass and does not assert universal driver precision.
        vec2 numerator=cubeFaceNumerator(direction,face);float majorValue=abs(direction[major]);
        dvec2 exactLocation=(dvec2(numerator)/double(majorValue)+1.0)*double(.5*size)-.5;
        low=ivec2(floor(exactLocation));
        vec2 binary=vec2(textureLod(uCubeCoordinateProbe,vec3(numerator.x,0.0,majorValue),float(level)).r,
                         textureLod(uCubeCoordinateProbe,vec3(numerator.y,0.0,majorValue),float(level)).r);
        weight=mix(binary,1.0-binary,notEqual(low&ivec2(1),ivec2(0)));
        vec2 quantized=vec2(low)+weight;
        low=ivec2(floor(quantized));weight=fract(quantized);
    }
    precise dvec4 sum=dvec4(0.0);
    for(int y=0;y<2;y++)for(int x=0;x<2;x++) {
        ivec2 texel=low+ivec2(x,y);
        precise double spatial=double(x==0 ? 1.0-weight.x : weight.x)*double(y==0 ? 1.0-weight.y : weight.y);
        if(spatial==0.0 || mipWeight==0.0)continue;
        bool outsideX=texel.x<0 || texel.x>=int(size),outsideY=texel.y<0 || texel.y>=int(size);
        if(outsideX && outsideY) {
            int horizontal=major==0 ? 2 : 0,vertical=major==1 ? 2 : 1;
            int last=texel.y>=int(size) ? vertical : texel.x>=int(size) ? horizontal : major;
            vec3 s=sign(direction);float center=1.0-1.0/size;
            vec3 dx=s*center,dy=dx,dz=dx;dx.x=s.x;dy.y=s.y;dz.z=s.z;
            vec4 colors[3]=vec4[3](textureLod(source,dx,float(level)),textureLod(source,dy,float(level)),textureLod(source,dz,float(level)));
            float third=roundEven(mipWeight*(256.0/3.0))/256.0;
            bool left=texel.x<0,top=texel.y<0;
            int order[3]=top ? int[3](vertical,left ? horizontal : major,left ? major : horizontal) : int[3](left ? horizontal : major,left ? major : horizontal,vertical);
            for(int rank=0;rank<3;rank++) {
                int i=order[rank];
                vec4 color=roundEven(colors[i]*65535.0);
                dvec4 part=dvec4(color)*(spatial*double(i==last ? mipWeight-2.0*third : third));
                cornerParts[rank]=part;
            }
        } else {
            vec4 color=cubePoint(source,texel,face,level);
            color=roundEven(color*65535.0);
            sum+=dvec4(color)*(spatial*double(mipWeight));
        }
    }
    return sum;
}
vec4 roundPositiveFloat32(dvec4 value){
    vec4 rounded=vec4(value);
    vec4 successor=uintBitsToFloat(floatBitsToUint(rounded)+uvec4(1));
    dvec4 midpoint=(dvec4(rounded)+dvec4(successor))*.5;
    return mix(rounded,successor,equal(value,midpoint));
}
vec4 normalizeTapInteger(dvec4 value){
    // Measured synthetic U8/sRGB-U16 filter output: extend the
    // UNORM word to 32 bits, then round positive halfway ties up.
    // This remains an opt-in hypothesis for original BC1 sampling.
    precise double multiplier=double(65537)/double(4294967296.0);
    precise dvec4 expanded=value*multiplier;
    return roundPositiveFloat32(expanded);
}
vec4 accumulateTapLevel(vec4 initial,dvec4 ordinary,dvec4 cornerParts[3]){
    vec4 value=roundPositiveFloat32(dvec4(initial)+dvec4(normalizeTapInteger(ordinary)));
    // Independent six-face corner probe: ordinary taps normalize
    // together; the three virtual corner samples normalize and add
    // separately in surviving row-major order, each with ties up.
    for(int i=0;i<3;i++)value=roundPositiveFloat32(dvec4(value)+dvec4(normalizeTapInteger(cornerParts[i])));
    return value;
}
vec4 cubeLight(samplerCube source,vec3 direction){
    float level=clamp(textureQueryLod(source,direction).y,0.0,float(textureQueryLevels(source)-1));
    if(uCubeTapFilter) {
        if(all(equal(direction,vec3(0.0))))return textureLod(source,direction,level);
        int low=int(floor(level)),high=min(low+1,textureQueryLevels(source)-1);
        float weight=fract(level);
        dvec4 lowerCorner[3],upperCorner[3];
        precise dvec4 lower=cubeTapLevel(source,direction,low,1.0-weight,lowerCorner);
        precise dvec4 upper=cubeTapLevel(source,direction,high,weight,upperCorner);
        vec4 value=accumulateTapLevel(vec4(0.0),lower,lowerCorner);
        return accumulateTapLevel(value,upper,upperCorner);
    }
    float low=floor(level);vec4 a=textureLod(source,direction,low);
    vec4 b=textureLod(source,direction,min(low+1.0,float(textureQueryLevels(source)-1)));
    precise vec4 result=a*(1.0-(level-low))+b*(level-low);
    return result;
}
void main(){
    precise vec3 t=vec3(-vNormal.z,0.0,vNormal.x);
    // Original DP2ADD on the measured D3D9 HAL rounds the first
    // product, then fuses the second product with that sum.
    precise float tt=nativeDp2Squared(t.x,t.z);
    t=inversesqrt(tt)*t;
    precise vec3 b=vec3(0.0,vNormal.z,-vNormal.y);
    precise float bb=nativeDp2Squared(b.y,b.z);b=inversesqrt(bb)*b;
    precise vec3 n=nativeMaterialImplicit(uNormal,vUv).xyz*2.0-1.0;
    precise vec3 world=b*n.y;world=nativeMad(vec3(n.x),t,world);world=nativeMad(vec3(n.z),vNormal,world);
    vec3 shade=cubeLight(uShade,world).xyz;vec3 sun=cubeLight(uSun,world).xyz;
    precise vec3 base=sun*1.70000005;precise vec3 delta=nativeMad(shade,vec3(1.70000005),-base);
    float shadow=nativeMaterialImplicit(uShadow,vUv).r;
    precise vec3 light=nativeMad(vec3(shadow),delta,base);
    precise vec3 pre=nativeMad(nativeMaterialImplicit(uAlbedo,vUv).xyz,light,vec3(.00100000005));
    precise vec3 gamma=log2(pre)*.454545468;
    precise vec3 lookup=exp2(gamma);
    diffuse=vec4(texture(uColorCorrect,lookup).xyz,1.0);
    )GLSL";
    if (specular)
        source += R"GLSL(        vec4 s=nativeMaterialImplicit(uSpec,vUv);
        precise float dotProduct=(uHalf.x*world.x+uHalf.y*world.y)+uHalf.z*world.z;
        precise float exponent=s.a*64.0+1.0;
        float power=pow(clamp(dotProduct,0.0,1.0),exponent);
        precise float factor=exponent*power;factor=clamp(1.0-shadow,0.0,1.0)*factor;
        precise vec4 value=s*factor;precise vec3 specGamma=log2(value.xyz)*.454545468;
        specular=vec4(texture(uColorCorrect,exp2(specGamma)).xyz,value.a);)GLSL";
    source += R"GLSL(
})GLSL";
    return source;
}
} // namespace civ5::graphics
