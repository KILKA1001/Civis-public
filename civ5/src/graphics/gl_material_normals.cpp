#include "civ5/graphics/gl_material_normals.hpp"
#include "civ5/graphics/gl_material_blend.hpp"
#include "civ5/graphics/gl_render_api.hpp"
#include "civ5/materials/material_normals.hpp"
#include "gl_sampled_input.hpp"
#include <algorithm>
#include <cmath>
#include <exception>
#include <stdexcept>

namespace civ5::graphics {
std::string_view material_normal_vertex_shader() {
    return R"(#version 460 core
uniform vec4 uCorners;out vec2 vUv;
void main(){vec2 p=vec2((gl_VertexID<<1)&2,gl_VertexID&2);
 gl_Position=vec4(p*2.0-1.0,0.0,1.0);
 vUv=uCorners.xy+p*(uCorners.zw-uCorners.xy);}
)";
}
std::string material_normal_fragment_shader() {
    return std::string(R"(#version 460 core
layout(binding=0) uniform sampler2D uHeight;
uniform vec3 uTexelSize; uniform float uHeightScale;
in vec2 vUv; layout(location=0) out vec4 color;
)") + std::string(material_sampling_glsl()) +
           R"(
float heightAt(vec2 offset){return nativeMaterialImplicit(uHeight,vUv+offset).r;}
void main(){
 vec2 d=uTexelSize.xy;
 float l=heightAt(vec2(-d.x,uTexelSize.z));
 float tl=heightAt(-d),tr=heightAt(vec2(d.x,-d.y));
 precise float left=(tl-tr)+2.0*l;
 float r=heightAt(vec2(d.x,uTexelSize.z));
 float bl=heightAt(vec2(-d.x,d.y)),br=heightAt(d);
 precise float right=(bl-br)-2.0*r;
 precise float gx=right+left;
 precise float halfScale=0.5*uHeightScale;
 precise float sx=halfScale*gx;
 float t=heightAt(vec2(uTexelSize.z,-d.y));
 precise float top=(tl+tr)+2.0*t;
 float b=heightAt(vec2(uTexelSize.z,d.y));
 precise float bottom=(-bl-br)-2.0*b;
 precise float gy=bottom+top;
 precise float sy=halfScale*gy;
 precise vec3 n=vec3(-(sx*2.0),-(sy*-2.0),4.0);
 precise float magnitude=(n.x*n.x+n.y*n.y)+n.z*n.z;
 float inverse=inversesqrt(magnitude);
 precise vec2 xy=n.xy*inverse;
 color=vec4(xy*0.5+0.5,inverse*2.0+0.5,0.0);
}
)";
}
namespace {
void require(bool ok, const char *why) {
    if (!ok)
        throw std::invalid_argument(why);
}
} // namespace
struct GlMaterialNormalRenderer::Impl {
    std::shared_ptr<GlContext> context;
    GlRenderApi gl;
    GLuint program = 0, vao = 0, sampler = 0;
    GLint corners = -1, texel_size = -1, height_scale = -1;
    explicit Impl(std::shared_ptr<GlContext> c) : context(std::move(c)), gl(*context) {
        try {
            context->check("Before material-normal initialization");
            program = gl.CreateProgram();
            const auto fragment = material_normal_fragment_shader();
            for (auto [kind, source] : {std::pair{GL_VERTEX_SHADER, material_normal_vertex_shader()},
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
            corners = gl.GetUniformLocation(program, "uCorners");
            texel_size = gl.GetUniformLocation(program, "uTexelSize");
            height_scale = gl.GetUniformLocation(program, "uHeightScale");
            require(corners >= 0 && texel_size >= 0 && height_scale >= 0, "Missing material-normal uniform");
            gl.CreateVertexArrays(1, &vao);
            gl.CreateSamplers(1, &sampler);
            gl.SamplerParameteri(sampler, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
            gl.SamplerParameteri(sampler, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            for (auto wrap : {GL_TEXTURE_WRAP_S, GL_TEXTURE_WRAP_T, GL_TEXTURE_WRAP_R})
                gl.SamplerParameteri(sampler, wrap, GL_CLAMP_TO_EDGE);
            gl.SamplerParameterf(sampler, GL_TEXTURE_LOD_BIAS, 0);
            gl.SamplerParameterf(sampler, GL_TEXTURE_MIN_LOD, -1000);
            gl.SamplerParameterf(sampler, GL_TEXTURE_MAX_LOD, 1000);
            gl.SamplerParameterf(sampler, GL_TEXTURE_MAX_ANISOTROPY, 1);
            context->check("Material-normal program and sampler5 initialized");
        } catch (...) {
            release();
            throw;
        }
    }
    void release() noexcept {
        try {
            const auto &api = context->api();
            gl.DeleteVertexArrays(1, &vao);
            gl.DeleteSamplers(1, &sampler);
            if (program) {
                GLint current = 0;
                api.GetIntegerv(GL_CURRENT_PROGRAM, &current);
                if (static_cast<GLuint>(current) == program)
                    gl.UseProgram(0);
                gl.DeleteProgram(program);
            }
            program = vao = sampler = 0;
        } catch (...) {
            std::terminate();
        }
    }
    ~Impl() { release(); }
};
GlMaterialNormalRenderer::GlMaterialNormalRenderer(std::shared_ptr<GlContext> context) {
    require(bool(context), "Material-normal renderer requires owned context");
    impl_ = std::make_unique<Impl>(std::move(context));
}
GlMaterialNormalRenderer::~GlMaterialNormalRenderer() = default;
void GlMaterialNormalRenderer::draw_cache(GLuint height, std::int32_t resolution) {
    (void)materials::material_normal_parameters(resolution);
    const float origin = -.5f / resolution;
    draw(height, resolution, {origin, origin, 1.f + origin, 1.f + origin});
}
void GlMaterialNormalRenderer::draw(GLuint height, std::int32_t resolution, const std::array<float, 4> &uv) {
    const auto parameters = materials::material_normal_parameters(resolution);
    require(std::all_of(uv.begin(), uv.end(), [](float v) { return std::isfinite(v); }),
            "Nonfinite material-normal corners");
    auto &s = *impl_;
    const auto &api = s.context->api();
    detail::validate_sampled_2d(*s.context, s.gl, height);
    s.context->check("Before material-normal draw");
    s.gl.ProgramUniform4fv(s.program, s.corners, 1, uv.data());
    s.gl.ProgramUniform3f(s.program, s.texel_size, parameters[0], parameters[1], parameters[2]);
    s.gl.ProgramUniform1f(s.program, s.height_scale, parameters[4]);
    s.gl.UseProgram(s.program);
    s.gl.BindVertexArray(s.vao);
    api.BindTextureUnit(0, height);
    s.gl.BindSampler(0, s.sampler);
    s.gl.DrawArrays(GL_TRIANGLES, 0, 3);
    s.context->check("Draw material-height RED to cache normal");
}
} // namespace civ5::graphics
