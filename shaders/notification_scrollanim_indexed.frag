#version 460 core

// Literal register-level port of pinned ps_3_0 blob raw 0x93b598..0x93bec8.
// Uniform vec4 values are supplied native c1..c8; no dimension-derived defaults.
layout(binding = 0) uniform sampler2D BaseIndexTexture;
layout(binding = 1) uniform sampler2D BaseBlockTexture;
layout(binding = 2) uniform sampler2D MaskIndexTexture;
layout(binding = 3) uniform sampler2D MaskBlockTexture;

uniform vec4 BlockSize; // native c1
uniform vec4 IndexScale; // native c2
uniform vec4 IndexTextureSize; // native c3
uniform vec4 BlockTextureSize; // native c4
uniform vec4 MaskBlockSize; // native c5
uniform vec4 MaskIndexScale; // native c6
uniform vec4 MaskIndexTextureSize; // native c7
uniform vec4 MaskBlockTextureSize; // native c8

layout(location = 0) in vec2 baseUV;
layout(location = 1) in vec2 maskUV;
layout(location = 2) in vec4 color0;
layout(location = 0) precise out vec4 color;

void main() {
    precise vec4 r0;
    precise vec4 r1;
    precise vec4 r2;
    precise vec4 r3;
    const vec4 c0 = vec4(0.5, 0.0, 1024.0, 255.0); // 0x93b81c DEF
    const vec4 c9 = vec4(256.0, 0.0, 0.0, 0.0); // 0x93b834 DEF
    r0.xz = c0.xz; // 0x93b8a0 MOV
    if (r0.x < BlockSize.x) { // 0x93b8ac IFC_LT
        r0.yw = IndexTextureSize.xy * baseUV.xy; // 0x93b8b8 MUL
        r1.xy = fract(r0.yw); // 0x93b8c8 FRC
        r0.yw = r0.yw + -r1.xy; // 0x93b8d4 ADD
        r1.xy = (baseUV.xy * IndexTextureSize.xy) + -r0.yw; // 0x93b8e4 MAD
        r0.yw = r0.yw + c0.xx; // 0x93b8f8 ADD
        r2.x = 1.0 / IndexTextureSize.x; // 0x93b908 RCP
        r2.y = 1.0 / IndexTextureSize.y; // 0x93b914 RCP
        r2.xy = r0.yw * r2.xy; // 0x93b920 MUL
        r2.zw = c0.yy; // 0x93b930 MOV
        r2.xyzw = textureLod(BaseIndexTexture, r2.xy, r2.w); // 0x93b93c TEXLDL
        r0.y = r0.z + -IndexScale.x; // 0x93b94c ADD
        r0.w = r2.x * c0.w; // 0x93b95c MUL
        r1.z = (r2.w * c0.w) + c0.x; // 0x93b96c MAD
        r1.w = fract(r1.z); // 0x93b980 FRC
        r1.z = -r1.w + r1.z; // 0x93b98c ADD
        r0.w = (r1.z * c9.x) + r0.w; // 0x93b99c MAD
        r0.w = r0.w + c0.x; // 0x93b9b0 ADD
        r1.z = fract(r0.w); // 0x93b9c0 FRC
        r0.w = r0.w + -r1.z; // 0x93b9cc ADD
        r0.w = r0.w + c0.x; // 0x93b9dc ADD
        r1.z = (IndexScale.x * r2.x) + r0.x; // 0x93b9ec MAD
        r1.w = fract(r1.z); // 0x93ba00 FRC
        r1.z = -r1.w + r1.z; // 0x93ba0c ADD
        r1.z = r1.z + c0.x; // 0x93ba1c ADD
        r0.y = (r0.y >= 0.0) ? r1.z : r0.w; // 0x93ba2c CMP
        r0.w = 1.0 / BlockSize.x; // 0x93ba40 RCP
        r0.w = (BlockTextureSize.x * r0.w) + r0.x; // 0x93ba4c MAD
        r1.z = fract(r0.w); // 0x93ba60 FRC
        r0.w = r0.w + -r1.z; // 0x93ba6c ADD
        r1.z = r0.w * r0.y; // 0x93ba7c MUL
        r1.z = (r1.z >= 0.0) ? r0.w : -r0.w; // 0x93ba8c CMP
        r1.w = 1.0 / r1.z; // 0x93baa0 RCP
        r1.w = r0.y * r1.w; // 0x93baac MUL
        r1.w = fract(r1.w); // 0x93babc FRC
        r2.x = r1.w * r1.z; // 0x93bac8 MUL
        r0.w = 1.0 / r0.w; // 0x93bad8 RCP
        r2.y = r0.w * r0.y; // 0x93bae4 MUL
        r0.yw = fract(r2.xy); // 0x93baf4 FRC
        r0.yw = -r0.yw + r2.xy; // 0x93bb00 ADD
        r0.yw = r1.xy + r0.yw; // 0x93bb10 ADD
        r0.yw = r0.yw * BlockSize.xx; // 0x93bb20 MUL
        r1.xy = fract(r0.yw); // 0x93bb30 FRC
        r0.yw = r0.yw + -r1.xy; // 0x93bb3c ADD
        r0.yw = r0.yw + c0.xx; // 0x93bb4c ADD
        r1.x = 1.0 / BlockTextureSize.x; // 0x93bb5c RCP
        r1.y = 1.0 / BlockTextureSize.y; // 0x93bb68 RCP
        r1.xy = r0.yw * r1.xy; // 0x93bb74 MUL
        r1.zw = c0.yy; // 0x93bb84 MOV
        r1.xyzw = textureLod(BaseBlockTexture, r1.xy, r1.w); // 0x93bb90 TEXLDL
    } else { // 0x93bba0 ELSE
        r1.xyzw = texture(BaseBlockTexture, baseUV.xy); // 0x93bba4 TEXLD
    } // 0x93bbb4 ENDIF
    r1.xyzw = r1.xyzw * color0.xyzw; // 0x93bbb8 MUL
    if (r0.x < MaskBlockSize.x) { // 0x93bbc8 IFC_LT
        r0.yw = MaskIndexTextureSize.xy * maskUV.xy; // 0x93bbd4 MUL
        r2.xy = fract(r0.yw); // 0x93bbe4 FRC
        r0.yw = r0.yw + -r2.xy; // 0x93bbf0 ADD
        r2.xy = (maskUV.xy * MaskIndexTextureSize.xy) + -r0.yw; // 0x93bc00 MAD
        r0.yw = r0.yw + c0.xx; // 0x93bc14 ADD
        r3.x = 1.0 / MaskIndexTextureSize.x; // 0x93bc24 RCP
        r3.y = 1.0 / MaskIndexTextureSize.y; // 0x93bc30 RCP
        r3.xy = r0.yw * r3.xy; // 0x93bc3c MUL
        r3.zw = c0.yy; // 0x93bc4c MOV
        r3.xyzw = textureLod(MaskIndexTexture, r3.xy, r3.w); // 0x93bc58 TEXLDL
        r0.y = r0.z + -MaskIndexScale.x; // 0x93bc68 ADD
        r0.z = r3.x * c0.w; // 0x93bc78 MUL
        r0.w = (r3.w * c0.w) + c0.x; // 0x93bc88 MAD
        r2.z = fract(r0.w); // 0x93bc9c FRC
        r0.w = r0.w + -r2.z; // 0x93bca8 ADD
        r0.z = (r0.w * c9.x) + r0.z; // 0x93bcb8 MAD
        r0.z = r0.z + c0.x; // 0x93bccc ADD
        r0.w = fract(r0.z); // 0x93bcdc FRC
        r0.z = -r0.w + r0.z; // 0x93bce8 ADD
        r0.w = (MaskIndexScale.x * r3.x) + r0.x; // 0x93bcf8 MAD
        r2.z = fract(r0.w); // 0x93bd0c FRC
        r0.w = r0.w + -r2.z; // 0x93bd18 ADD
        r0.zw = r0.zw + c0.xx; // 0x93bd28 ADD
        r0.y = (r0.y >= 0.0) ? r0.w : r0.z; // 0x93bd38 CMP
        r0.z = 1.0 / MaskBlockSize.x; // 0x93bd4c RCP
        r0.x = (MaskBlockTextureSize.x * r0.z) + r0.x; // 0x93bd58 MAD
        r0.z = fract(r0.x); // 0x93bd6c FRC
        r0.x = -r0.z + r0.x; // 0x93bd78 ADD
        r0.z = r0.x * r0.y; // 0x93bd88 MUL
        r0.z = (r0.z >= 0.0) ? r0.x : -r0.x; // 0x93bd98 CMP
        r0.w = 1.0 / r0.z; // 0x93bdac RCP
        r0.w = r0.w * r0.y; // 0x93bdb8 MUL
        r0.w = fract(r0.w); // 0x93bdc8 FRC
        r0.x = 1.0 / r0.x; // 0x93bdd4 RCP
        r3.xy = r0.wx * r0.zy; // 0x93bde0 MUL
        r0.xy = fract(r3.xy); // 0x93bdf0 FRC
        r0.xy = -r0.xy + r3.xy; // 0x93bdfc ADD
        r0.xy = r2.xy + r0.xy; // 0x93be0c ADD
        r0.xy = r0.xy * MaskBlockSize.xx; // 0x93be1c MUL
        r0.zw = fract(r0.xy); // 0x93be2c FRC
        r0.xy = -r0.zw + r0.xy; // 0x93be38 ADD
        r0.xy = r0.xy + c0.xx; // 0x93be48 ADD
        r2.x = 1.0 / MaskBlockTextureSize.x; // 0x93be58 RCP
        r2.y = 1.0 / MaskBlockTextureSize.y; // 0x93be64 RCP
        r0.xy = r0.xy * r2.xy; // 0x93be70 MUL
        r0.zw = c0.yy; // 0x93be80 MOV
        r0.xyzw = textureLod(MaskBlockTexture, r0.xy, r0.w); // 0x93be8c TEXLDL
    } else { // 0x93be9c ELSE
        r0.xyzw = texture(MaskBlockTexture, maskUV.xy); // 0x93bea0 TEXLD
    } // 0x93beb0 ENDIF
    color.xyzw = r0.xyzw * r1.xyzw; // 0x93beb4 MUL
}
