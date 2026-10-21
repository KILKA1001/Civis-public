#include "civ5/graphics/gl_material_blend.hpp"
#include "civ5/graphics/gl_render_api.hpp"
#include <algorithm>
#include <cmath>
#include <exception>
#include <stdexcept>

namespace civ5::graphics {
std::string_view material_blend_vertex_shader() {
    return R"(#version 460 core
layout(location=0) in vec3 aPosition;
layout(location=1) in vec3 aWorld;
layout(location=2) in vec4 aFactors0;
layout(location=3) in vec4 aFactors1;
layout(location=4) in vec4 aIndices0;
layout(location=5) in vec4 aIndices1;
uniform vec4 uCoefficients[96];
uniform vec2 uTexelOffsets;
out vec4 tfPosition;
out vec2 vUv[8];
out vec4 vFactors[2];
vec2 project(int index) {
    vec4 c=uCoefficients[index];
    precise vec2 xy=aWorld.xy*c.x;
    precise vec2 uv=xy*0.330000013;
    if (c.y==1.0) { xy=aWorld.xz*c.x;uv=xy*0.660000026; }
    if (c.y==2.0) { xy=aWorld.yz*c.x;uv=xy*0.660000026; }
    return uv;
}
void main() {
    precise vec2 clip=aPosition.xy+uTexelOffsets;
    gl_Position=vec4(clip,aPosition.z,1.0);
    tfPosition=gl_Position;
    for (int i=0;i<4;i++) {
        vUv[i]=project(int(aIndices0[i]));
        vUv[i+4]=project(int(aIndices1[i]));
    }
    vFactors[0]=aFactors0;vFactors[1]=aFactors1;
})";
}
std::string_view material_sampling_glsl() {
    return R"(vec4 nativeMaterialLod(sampler2D source,vec2 uv,float lod) {
    float last=float(textureQueryLevels(source)-1);
    float level=clamp(lod,0.0,last);
    float low=floor(level);
    vec4 first=textureLod(source,uv,low);
    vec4 second=textureLod(source,uv,min(low+1.0,last));
    precise vec4 result=first*(1.0-(level-low))+second*(level-low);
    return result;
}
vec4 nativeMaterialImplicit(sampler2D source,vec2 uv) {
    // .y is computed relative LOD, not the selected-mip result in .x.
    return nativeMaterialLod(source,uv,textureQueryLod(source,uv).y);
}
)";
}
std::string material_blend_cache_vertex_shader() {
    std::string source(material_blend_vertex_shader());
    source.insert(source.find("uniform vec2 uTexelOffsets;"), "uniform vec2 uPixelCenter;\n");
    const std::string original = "gl_Position=vec4(clip,aPosition.z,1.0);";
    source.replace(source.find(original), original.size(),
                   "gl_Position=vec4(vec2(clip.x,-clip.y)+uPixelCenter,aPosition."
                   "z*2.0-1.0,1.0);");
    return source;
}
std::string material_blend_fragment_shader() {
    return std::string(R"(#version 460 core
layout(binding=0) uniform sampler2D uAlbedo[8];
layout(binding=8) uniform sampler2D uHeight[8];
in vec2 vUv[8];
in vec4 vFactors[2];
layout(location=0) out vec4 outDiffuse;
layout(location=1) out vec4 outHeight;
uniform bool uLinearMip;
uniform int uHardwareMipMask;
)") + std::string(material_sampling_glsl()) +
           R"(vec4 materialSample(sampler2D source,vec2 uv,int slot) {
    if (uLinearMip && (uHardwareMipMask & (1<<slot))==0) return nativeMaterialImplicit(source,uv);
    return texture(source,uv);
}
vec4 materialMad(vec4 a,vec4 b,vec4 c) {
    // Preserve the single rounding observed at original SM3 MAD boundaries.
    precise dvec4 value=dvec4(a)*dvec4(b)+dvec4(c);return vec4(value);
}
float materialMad(float a,float b,float c) {
    precise double value=double(a)*double(b)+double(c);return float(value);
}
void main() {
    precise vec4 squared[8];precise float weight[8];
    for (int i=0;i<8;i++) {
        vec4 sampleValue=materialSample(uAlbedo[i],vUv[i],i);
        squared[i]=sampleValue*sampleValue;
        precise float fourth=squared[i].a*squared[i].a;
        precise float eighth=fourth*fourth;
        weight[i]=eighth*vFactors[i/4][i%4];
    }
    precise vec4 color=squared[1]*weight[1];
    color=materialMad(vec4(weight[0]),squared[0],color);
    for (int i=2;i<8;i++) color=materialMad(vec4(weight[i]),squared[i],color);
    precise float sum0=((weight[0]+weight[1])+weight[2])+weight[3];
    precise float sum1=((weight[4]+weight[5])+weight[6])+weight[7];
    precise float inverse=1.0/(sum1+sum0);
    outDiffuse=color*inverse;
    precise float height=materialSample(uHeight[1],vUv[1],9).r*weight[1];
    height=materialMad(weight[0],materialSample(uHeight[0],vUv[0],8).r,height);
    for (int i=2;i<8;i++) height=materialMad(weight[i],materialSample(uHeight[i],vUv[i],i+8).r,height);
    outHeight=vec4(inverse*height);
})";
}
namespace {
void require(bool ok, const char *reason) {
    if (!ok)
        throw std::invalid_argument(reason);
}
} // namespace
struct GlMaterialBlendRenderer::Impl {
    std::shared_ptr<GlContext> context;
    GlRenderApi gl;
    GLuint program = 0, vao = 0, vb = 0, ib = 0;
    std::array<GLuint, 2> samplers{};
    GLsizei vertex_count, index_count;
    GLint coefficients = -1, offsets = -1, linear_mip = -1, hardware_mip = -1;
    GLint pixel_center = -1;
    MaterialBlendCoordinates coordinates;
    Impl(std::shared_ptr<GlContext> c, const materials::MaterialPatchMesh &mesh,
         MaterialBlendCoordinates coords)
        : context(std::move(c)), gl(*context), vertex_count(mesh.columns() * mesh.rows()),
          index_count(static_cast<GLsizei>(mesh.indices().size())), coordinates(coords) {
        try {
            context->check("Before material blend initialization");
            program = gl.CreateProgram();
            const auto fragment = material_blend_fragment_shader();
            const auto vertex = coords == MaterialBlendCoordinates::d3d9_cache
                                    ? material_blend_cache_vertex_shader()
                                    : std::string(material_blend_vertex_shader());
            for (auto [kind, source] : {std::pair{GL_VERTEX_SHADER, std::string_view(vertex)},
                                        {GL_FRAGMENT_SHADER, std::string_view(fragment)}}) {
                const GLuint shader = gl.CreateShader(kind);
                const char *text = source.data();
                const auto size = static_cast<GLint>(source.size());
                gl.ShaderSource(shader, 1, &text, &size);
                gl.CompileShader(shader);
                GLint ok = 0;
                gl.GetShaderiv(shader, GL_COMPILE_STATUS, &ok);
                if (!ok) {
                    char log[8192]{};
                    gl.GetShaderInfoLog(shader, sizeof(log), nullptr, log);
                    gl.DeleteShader(shader);
                    throw std::runtime_error(log);
                }
                gl.AttachShader(program, shader);
                gl.DeleteShader(shader);
            }
            const char *varyings[] = {"tfPosition", "vUv[0]", "vUv[1]", "vUv[2]",      "vUv[3]",     "vUv[4]",
                                      "vUv[5]",     "vUv[6]", "vUv[7]", "vFactors[0]", "vFactors[1]"};
            gl.TransformFeedbackVaryings(program, 11, varyings, GL_INTERLEAVED_ATTRIBS);
            gl.LinkProgram(program);
            GLint ok = 0;
            gl.GetProgramiv(program, GL_LINK_STATUS, &ok);
            if (!ok) {
                char log[8192]{};
                gl.GetProgramInfoLog(program, sizeof(log), nullptr, log);
                throw std::runtime_error(log);
            }
            coefficients = gl.GetUniformLocation(program, "uCoefficients[0]");
            offsets = gl.GetUniformLocation(program, "uTexelOffsets");
            linear_mip = gl.GetUniformLocation(program, "uLinearMip");
            hardware_mip = gl.GetUniformLocation(program, "uHardwareMipMask");
            pixel_center = gl.GetUniformLocation(program, "uPixelCenter");
            require(coords != MaterialBlendCoordinates::d3d9_cache || pixel_center >= 0,
                    "Missing cache pixel center");
            require(coefficients >= 0 && offsets >= 0 && linear_mip >= 0 && hardware_mip >= 0,
                    "Missing material uniform");
            gl.CreateVertexArrays(1, &vao);
            gl.CreateBuffers(1, &vb);
            gl.CreateBuffers(1, &ib);
            gl.NamedBufferData(vb, mesh.vertices().size_bytes(), mesh.vertices().data(), GL_STATIC_DRAW);
            gl.NamedBufferData(ib, mesh.indices().size_bytes(), mesh.indices().data(), GL_STATIC_DRAW);
            gl.VertexArrayVertexBuffer(vao, 0, vb, 0, 48);
            gl.VertexArrayElementBuffer(vao, ib);
            const std::array<GLuint, 6> at{0, 20, 32, 36, 40, 44};
            for (GLuint location = 0; location < 6; ++location) {
                gl.EnableVertexArrayAttrib(vao, location);
                gl.VertexArrayAttribBinding(vao, location, 0);
                gl.VertexArrayAttribFormat(vao, location, location < 2 ? 3 : 4,
                                           location < 2 ? GL_FLOAT : GL_UNSIGNED_BYTE,
                                           location == 2 || location == 3, at[location]);
            }
            gl.CreateSamplers(2, samplers.data());
            for (unsigned i = 0; i < 2; ++i) {
                gl.SamplerParameteri(samplers[i], GL_TEXTURE_MIN_FILTER,
                                     i ? GL_LINEAR_MIPMAP_LINEAR : GL_NEAREST);
                gl.SamplerParameteri(samplers[i], GL_TEXTURE_MAG_FILTER, i ? GL_LINEAR : GL_NEAREST);
                for (auto wrap : {GL_TEXTURE_WRAP_S, GL_TEXTURE_WRAP_T, GL_TEXTURE_WRAP_R})
                    gl.SamplerParameteri(samplers[i], wrap, GL_REPEAT);
                gl.SamplerParameterf(samplers[i], GL_TEXTURE_LOD_BIAS, 0);
                gl.SamplerParameterf(samplers[i], GL_TEXTURE_MIN_LOD, -1000);
                gl.SamplerParameterf(samplers[i], GL_TEXTURE_MAX_LOD, 1000);
                // Anisotropy became Core in4.6, the required backend context.
                gl.SamplerParameterf(samplers[i], GL_TEXTURE_MAX_ANISOTROPY, 1);
            }
            context->check("Material48 program/geometry/samplers initialized");
        } catch (...) {
            release();
            throw;
        }
    }
    void release() noexcept {
        try {
            const auto &api = context->api();
            gl.DeleteVertexArrays(1, &vao);
            gl.DeleteBuffers(1, &vb);
            gl.DeleteBuffers(1, &ib);
            gl.DeleteSamplers(2, samplers.data());
            if (program) {
                GLint current = 0;
                api.GetIntegerv(GL_CURRENT_PROGRAM, &current);
                if (static_cast<GLuint>(current) == program)
                    gl.UseProgram(0);
                gl.DeleteProgram(program);
                program = 0;
            }
            vao = vb = ib = 0;
            samplers = {};
        } catch (...) {
            std::terminate();
        }
    }
    ~Impl() { release(); }
    void prepare(std::span<const float, 384> values, const std::array<float, 2> &texel_offsets) {
        require(std::all_of(values.begin(), values.end(), [](float v) { return std::isfinite(v); }) &&
                    std::all_of(texel_offsets.begin(), texel_offsets.end(),
                                [](float v) { return std::isfinite(v); }),
                "Nonfinite material constants/offset");
        context->api();
        context->check("Before material draw");
        gl.ProgramUniform4fv(program, coefficients, 96, values.data());
        gl.ProgramUniform2fv(program, offsets, 1, texel_offsets.data());
        gl.UseProgram(program);
        gl.BindVertexArray(vao);
    }
};
GlMaterialBlendRenderer::GlMaterialBlendRenderer(std::shared_ptr<GlContext> context,
                                                 const materials::MaterialPatchMesh &mesh,
                                                 MaterialBlendCoordinates coordinates) {
    require(bool(context), "Material renderer requires owned context");
    require(coordinates == MaterialBlendCoordinates::gl_control ||
                coordinates == MaterialBlendCoordinates::d3d9_cache,
            "Unverified material coordinate convention");
    require(mesh.vertices().size() == static_cast<std::size_t>(mesh.columns() * mesh.rows() * 48) &&
                mesh.indices().size() ==
                    static_cast<std::size_t>((mesh.columns() - 1) * (mesh.rows() - 1) * 6),
            "Moved-from material mesh cannot initialize a renderer");
    impl_ = std::make_unique<Impl>(std::move(context), mesh, coordinates);
}
GlMaterialBlendRenderer::~GlMaterialBlendRenderer() = default;
void GlMaterialBlendRenderer::draw(std::span<const float, 384> coefficients,
                                   const std::array<float, 2> &offset, const std::array<GLuint, 16> &textures,
                                   MaterialSampling sampling, std::array<std::int32_t, 2> target_size) {
    auto &s = *impl_;
    const auto &api = s.context->api();
    require(sampling == MaterialSampling::ordinary0 || sampling == MaterialSampling::point_control,
            "Unverified material sampler");
    if (s.coordinates == MaterialBlendCoordinates::d3d9_cache) {
        require(target_size[0] > 0 && target_size[1] > 0, "Native cache requires target dimensions");
        const std::array<float, 2> center{1.f / target_size[0], 1.f / target_size[1]};
        s.gl.ProgramUniform2fv(s.program, s.pixel_center, 1, center.data());
    }
    for (auto texture : textures)
        require(texture != 0 && api.IsTexture(texture), "Material draw requires16 real textures");
    s.prepare(coefficients, offset);
    const bool linear = sampling == MaterialSampling::ordinary0;
    s.gl.ProgramUniform1i(s.program, s.linear_mip, linear);
    GLint hardware_mask = 0;
    for (GLuint slot = 0; slot < 16; ++slot) {
        if (linear) {
            GLint base = 0, format = 0;
            api.GetTextureParameteriv(textures[slot], GL_TEXTURE_BASE_LEVEL, &base);
            api.GetTextureLevelParameteriv(textures[slot], base, GL_TEXTURE_INTERNAL_FORMAT, &format);
            // Original BC3 TEXLD prefixes match hardware filtering exactly.
            // The authored RGBA8 resource0 retains its independently measured
            // explicit interpolation of integer mip levels.
            if (format == 0x83f3) // GL_COMPRESSED_RGBA_S3TC_DXT5_EXT
                hardware_mask |= 1 << slot;
        }
        api.BindTextureUnit(slot, textures[slot]);
        s.gl.BindSampler(slot, s.samplers[linear ? 1 : 0]);
    }
    s.gl.ProgramUniform1i(s.program, s.hardware_mip, hardware_mask);
    s.gl.DrawElements(GL_TRIANGLES, s.index_count, GL_UNSIGNED_SHORT, nullptr);
    s.context->check("Draw native eight-layer material cache");
}
std::vector<float> GlMaterialBlendRenderer::capture(std::span<const float, 384> coefficients,
                                                    const std::array<float, 2> &offset, bool indexed) {
    auto &s = *impl_;
    require(s.coordinates == MaterialBlendCoordinates::gl_control,
            "Capture uses unconverted native shader coordinates");
    const auto &api = s.context->api();
    GLint active = 0;
    api.GetIntegerv(GL_TRANSFORM_FEEDBACK_ACTIVE, &active);
    require(active == 0, "Nested material transform feedback");
    s.prepare(coefficients, offset);
    const auto count = indexed ? s.index_count : s.vertex_count;
    std::vector<float> result(static_cast<std::size_t>(count) * 28);
    GLuint buffer = 0, feedback = 0;
    GLint previous = 0;
    api.GetIntegerv(GL_TRANSFORM_FEEDBACK_BINDING, &previous);
    const bool discard = s.gl.IsEnabled(GL_RASTERIZER_DISCARD) != 0;
    bool began = false;
    const auto restore = [&] {
        if (began)
            s.gl.EndTransformFeedback();
        s.gl.BindTransformFeedback(GL_TRANSFORM_FEEDBACK, previous);
        if (!discard)
            s.gl.Disable(GL_RASTERIZER_DISCARD);
        s.gl.DeleteTransformFeedbacks(1, &feedback);
        s.gl.DeleteBuffers(1, &buffer);
    };
    try {
        s.gl.CreateBuffers(1, &buffer);
        s.gl.CreateTransformFeedbacks(1, &feedback);
        s.gl.NamedBufferData(buffer, result.size() * sizeof(float), nullptr, GL_STREAM_READ);
        s.gl.TransformFeedbackBufferBase(feedback, 0, buffer);
        s.gl.BindTransformFeedback(GL_TRANSFORM_FEEDBACK, feedback);
        s.gl.Enable(GL_RASTERIZER_DISCARD);
        s.context->check("Material feedback allocation");
        s.gl.BeginTransformFeedback(indexed ? GL_TRIANGLES : GL_POINTS);
        s.context->check("Begin material feedback");
        began = true;
        if (indexed)
            s.gl.DrawElements(GL_TRIANGLES, count, GL_UNSIGNED_SHORT, nullptr);
        else
            s.gl.DrawArrays(GL_POINTS, 0, count);
        s.gl.EndTransformFeedback();
        began = false;
        s.gl.GetNamedBufferSubData(buffer, 0, result.size() * sizeof(float), result.data());
        s.context->check("Material vertex output readback");
    } catch (...) {
        restore();
        throw;
    }
    restore();
    s.context->check("Material feedback state restored");
    return result;
}
} // namespace civ5::graphics
