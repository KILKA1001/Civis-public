#include "civ5/graphics/gl_material_shadows.hpp"
#include "civ5/graphics/gl_material_blend.hpp"
#include "gl_sampled_input.hpp"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <exception>
#include <stdexcept>

namespace civ5::graphics {
std::string_view material_shadow_vertex_shader() {
    return R"(#version 460 core
layout(location=0) in vec3 aPosition;layout(location=1) in vec2 aSample;
uniform vec2 uTexelOffsets,uPixelCenter;out vec2 vSample;
void main(){
    precise vec2 clip=aPosition.xy+uTexelOffsets;
    gl_Position=vec4(vec2(clip.x,-clip.y)+uPixelCenter,aPosition.z*2.0-1.0,1.0);
    vSample=aSample;
}
)";
}
std::string material_shadow_fragment_shader() {
    return std::string(R"(#version 460 core
layout(binding=0) uniform sampler2D uHeight;
uniform vec3 uWorldScales,uSunDir;uniform vec2 uHeightScale,uRebase;uniform vec4 uJitter;
in vec2 vSample;layout(location=0) out vec4 color;
)") + std::string(material_sampling_glsl()) +
           R"(
// Keep repeated ray/UV updates in Float32. The water threshold has a separate
// measured rounding path: replacing it with this FMA changes stored shadows.
float shadowMad(float a,float b,float c){precise float value=fma(a,b,c);return value;}
vec2 shadowMad(vec2 a,vec2 b,vec2 c){precise vec2 value=fma(a,b,c);return value;}
float shadowWaterMad(float a,float b,float c){precise double value=double(a)*double(b)+double(c);return float(value);}
void main(){
    precise float inv=1.0/max(abs(uSunDir.x),abs(uSunDir.y));
    precise vec2 stepDir=inv*uSunDir.xy;
    precise vec2 origin=(uRebase+vSample)*uHeightScale;
    precise float initial=nativeMaterialImplicit(uHeight,origin).a*uWorldScales.z;
    precise float ray=shadowMad(initial,255.0,uJitter.z);
    precise vec2 worldStep=stepDir*uWorldScales.xy;
    precise float square=worldStep.x*worldStep.x;
    square=shadowMad(worldStep.y,worldStep.y,square);
    float inverseLength=inversesqrt(square);
    precise float length=1.0/inverseLength;
    ray=shadowMad(length,0.314485461,ray);
    precise vec2 uv=(stepDir+vSample+uRebase)*uHeightScale;
    precise float visibility=1.0;
    for(int i=0;i<40;i++){
        uv=shadowMad(stepDir,uHeightScale,uv);
        ray=shadowMad(length,0.314485461,ray);
        float alpha=nativeMaterialImplicit(uHeight,uv).a;
        precise float scaled=alpha*255.0;
        // Measured original HAL folds c3.w into the preceding MAD constants.
        // Rounding the level before this scale crosses native UNORM8 boundaries.
        precise float water=1.0-clamp(shadowWaterMad(alpha,255.0*0.010416667,-64.0*0.010416667),0.0,1.0);
        precise float difference=shadowMad(scaled,-uWorldScales.z,ray);
        // This driver rounds the product before adding the ray. Its nonzero
        // result preserves the comparison sign; cancellation can erase the
        // original negative residual. Recover only that boundary, keeping
        // the common 40-step path in Float32.
        if(difference==0.0){
            precise double residual=double(scaled)*double(-uWorldScales.z)+double(ray);
            difference=float(residual);
        }
        if(difference<0.0)visibility=min(water,visibility);
    }
    precise float shadow=(1.0-visibility)*uJitter.z;color=vec4(shadow);
}
)";
}
namespace {
void require(bool ok, const char *why) {
    if (!ok)
        throw std::invalid_argument(why);
}
} // namespace
struct GlMaterialShadowRenderer::Impl {
    std::shared_ptr<GlContext> context;
    GlRenderApi gl;
    GLuint program = 0, vao = 0, vb = 0, ib = 0, sampler = 0;
    GLsizei count;
    GLint offsets = -1, center = -1, world = -1, sun = -1, height_scale = -1, rebase = -1, jitter = -1;
    Impl(std::shared_ptr<GlContext> c, const materials::MaterialPatchMesh &mesh)
        : context(std::move(c)), gl(*context), count(static_cast<GLsizei>(mesh.indices().size())) {
        try {
            context->check("Before macro-shadow initialization");
            program = gl.CreateProgram();
            const auto fragment = material_shadow_fragment_shader();
            for (auto [kind, source] : {std::pair{GL_VERTEX_SHADER, material_shadow_vertex_shader()},
                                        {GL_FRAGMENT_SHADER, std::string_view(fragment)}}) {
                const auto shader = gl.CreateShader(kind);
                const char *text = source.data();
                const auto length = static_cast<GLint>(source.size());
                gl.ShaderSource(shader, 1, &text, &length);
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
            gl.LinkProgram(program);
            GLint ok = 0;
            gl.GetProgramiv(program, GL_LINK_STATUS, &ok);
            if (!ok) {
                char log[8192]{};
                gl.GetProgramInfoLog(program, sizeof(log), nullptr, log);
                throw std::runtime_error(log);
            }
            offsets = gl.GetUniformLocation(program, "uTexelOffsets");
            center = gl.GetUniformLocation(program, "uPixelCenter");
            world = gl.GetUniformLocation(program, "uWorldScales");
            sun = gl.GetUniformLocation(program, "uSunDir");
            height_scale = gl.GetUniformLocation(program, "uHeightScale");
            rebase = gl.GetUniformLocation(program, "uRebase");
            jitter = gl.GetUniformLocation(program, "uJitter");
            require(std::min({offsets, center, world, sun, height_scale, rebase, jitter}) >= 0,
                    "Missing macro-shadow uniforms");
            gl.CreateVertexArrays(1, &vao);
            gl.CreateBuffers(1, &vb);
            gl.CreateBuffers(1, &ib);
            gl.NamedBufferData(vb, mesh.vertices().size_bytes(), mesh.vertices().data(), GL_STATIC_DRAW);
            gl.NamedBufferData(ib, mesh.indices().size_bytes(), mesh.indices().data(), GL_STATIC_DRAW);
            gl.VertexArrayVertexBuffer(vao, 0, vb, 0, 48);
            gl.VertexArrayElementBuffer(vao, ib);
            for (GLuint i = 0; i < 2; ++i) {
                gl.EnableVertexArrayAttrib(vao, i);
                gl.VertexArrayAttribBinding(vao, i, 0);
                gl.VertexArrayAttribFormat(vao, i, i == 0 ? 3 : 2, GL_FLOAT, GL_FALSE, i == 0 ? 0 : 12);
            }
            gl.CreateSamplers(1, &sampler);
            gl.SamplerParameteri(sampler, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
            gl.SamplerParameteri(sampler, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            for (auto wrap : {GL_TEXTURE_WRAP_S, GL_TEXTURE_WRAP_T, GL_TEXTURE_WRAP_R})
                gl.SamplerParameteri(sampler, wrap, GL_REPEAT);
            gl.SamplerParameterf(sampler, GL_TEXTURE_LOD_BIAS, 0);
            gl.SamplerParameterf(sampler, GL_TEXTURE_MIN_LOD, -1000);
            gl.SamplerParameterf(sampler, GL_TEXTURE_MAX_LOD, 1000);
            gl.SamplerParameterf(sampler, GL_TEXTURE_MAX_ANISOTROPY, 1);
            context->check("Macro-shadow program and material48/TEXCOORD0 geometry");
        } catch (...) {
            release();
            throw;
        }
    }
    void release() noexcept {
        try {
            const auto &api = context->api();
            gl.DeleteBuffers(1, &vb);
            gl.DeleteBuffers(1, &ib);
            gl.DeleteVertexArrays(1, &vao);
            gl.DeleteSamplers(1, &sampler);
            if (program) {
                GLint current = 0;
                api.GetIntegerv(GL_CURRENT_PROGRAM, &current);
                if (static_cast<GLuint>(current) == program)
                    gl.UseProgram(0);
                gl.DeleteProgram(program);
            }
            program = vao = vb = ib = sampler = 0;
        } catch (...) {
            std::terminate();
        }
    }
    ~Impl() { release(); }
    void prepare(GLuint texture, int width, int height, const std::array<float, 2> &offset) {
        require(width > 0 && height > 0 && std::isfinite(offset[0]) && std::isfinite(offset[1]),
                "Invalid macro-shadow target dimensions or offset");
        detail::validate_sampled_2d(*context, gl, texture);
        const std::array<float, 2> pixel_center{1.f / width, 1.f / height};
        gl.ProgramUniform2fv(program, offsets, 1, offset.data());
        gl.ProgramUniform2fv(program, center, 1, pixel_center.data());
        gl.UseProgram(program);
        gl.BindVertexArray(vao);
        context->api().BindTextureUnit(0, texture);
        gl.BindSampler(0, sampler);
    }
    void draw(const materials::MaterialShadowParameters &p) {
        gl.ProgramUniform3f(program, world, p.world_scales[0], p.world_scales[1], p.world_scales[2]);
        gl.ProgramUniform3f(program, sun, p.sun_direction[0], p.sun_direction[1], p.sun_direction[2]);
        gl.ProgramUniform2fv(program, height_scale, 1, p.height_texture_scale.data());
        gl.ProgramUniform2fv(program, rebase, 1, p.world_rebase.data());
        gl.ProgramUniform4fv(program, jitter, 1, p.jitter_add.data());
        gl.DrawElements(GL_TRIANGLES, count, GL_UNSIGNED_SHORT, nullptr);
        context->check("Draw original40-step macro shadow");
    }
};
GlMaterialShadowRenderer::GlMaterialShadowRenderer(std::shared_ptr<GlContext> context,
                                                   const materials::MaterialPatchMesh &mesh) {
    require(bool(context), "Macro-shadow renderer requires owned context");
    require(mesh.vertices().size() == static_cast<std::size_t>(mesh.columns() * mesh.rows() * 48) &&
                mesh.indices().size() ==
                    static_cast<std::size_t>((mesh.columns() - 1) * (mesh.rows() - 1) * 6),
            "Moved-from macro-shadow mesh");
    for (std::size_t at = 0; at < mesh.vertices().size(); at += 48) {
        std::array<float, 2> sample;
        std::memcpy(sample.data(), mesh.vertices().data() + at + 12, 8);
        require(std::isfinite(sample[0]) && std::isfinite(sample[1]), "Nonfinite macro-shadow TEXCOORD0");
    }
    impl_ = std::make_unique<Impl>(std::move(context), mesh);
}
GlMaterialShadowRenderer::~GlMaterialShadowRenderer() = default;
void GlMaterialShadowRenderer::draw(GLuint texture, const materials::MaterialShadowParameters &p, int width,
                                    int height, const std::array<float, 2> &offset) {
    p.validate();
    impl_->prepare(texture, width, height, offset);
    impl_->draw(p);
}
void GlMaterialShadowRenderer::draw_accumulated(GLuint texture,
                                                std::span<const materials::MaterialShadowParameters> samples,
                                                int width, int height) {
    for (const auto &p : samples)
        p.validate();
    auto &s = *impl_;
    s.prepare(texture, width, height, {0, 0});
    s.gl.Enable(GL_BLEND);
    s.gl.BlendEquationSeparate(GL_FUNC_ADD, GL_FUNC_ADD);
    s.gl.BlendFuncSeparate(GL_ONE, GL_ONE, GL_ONE, GL_ONE);
    s.gl.ColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    try {
        for (const auto &p : samples)
            s.draw(p);
    } catch (...) {
        s.gl.Disable(GL_BLEND);
        throw;
    }
    s.gl.Disable(GL_BLEND);
    s.context->check("End macro-shadow accumulation");
}
} // namespace civ5::graphics
