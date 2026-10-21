#include "civ5/graphics/gl_fog_of_war.hpp"
#include "gl_sampled_input.hpp"
#include "gl_native_clip.hpp"
#include <algorithm>
#include <cmath>
#include <exception>
#include <stdexcept>

namespace civ5::graphics {
namespace {
void require(bool ok, const char *message) {
    if (!ok)
        throw std::invalid_argument(message);
}
} // namespace
struct GlFogOfWarRenderer::Impl {
    std::shared_ptr<GlContext> context;
    GlRenderApi gl;
    std::array<GLuint, 6> programs{};
    std::array<GLint, 6> constants{};
    std::array<GLuint, 2> vaos{}, vertices{}, indices{};
    GLuint sampler = 0;
    explicit Impl(std::shared_ptr<GlContext> c) : context(std::move(c)), gl(*context) {
        try {
            for (unsigned index = 0; index < 6; ++index) {
                const auto program = programs[index] = gl.CreateProgram();
                for (auto [stage, text] : {std::pair{GL_VERTEX_SHADER, fog_of_war_vertex_shader(index)},
                                           {GL_FRAGMENT_SHADER, fog_of_war_fragment_shader(index)}}) {
                    const auto shader = gl.CreateShader(stage);
                    const char *source = text.data();
                    const auto size = static_cast<GLint>(text.size());
                    gl.ShaderSource(shader, 1, &source, &size);
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
                constants[index] = gl.GetUniformLocation(program, "uV[0]");
                require(constants[index] >= 0, "Missing FOW shader constants");
            }
            const auto geometry = materials::fog_of_war_geometry();
            gl.CreateVertexArrays(2, vaos.data());
            gl.CreateBuffers(2, vertices.data());
            gl.CreateBuffers(2, indices.data());
            gl.NamedBufferData(vertices[0], sizeof(geometry.mask_vertices), geometry.mask_vertices.data(),
                               GL_STATIC_DRAW);
            gl.NamedBufferData(indices[0], sizeof(geometry.mask_indices), geometry.mask_indices.data(),
                               GL_STATIC_DRAW);
            gl.NamedBufferData(vertices[1], sizeof(geometry.layer_vertices), geometry.layer_vertices.data(),
                               GL_STATIC_DRAW);
            gl.NamedBufferData(indices[1], sizeof(geometry.layer_indices), geometry.layer_indices.data(),
                               GL_STATIC_DRAW);
            for (unsigned kind = 0; kind < 2; ++kind) {
                gl.VertexArrayVertexBuffer(vaos[kind], 0, vertices[kind], 0, kind ? 32 : 40);
                gl.VertexArrayElementBuffer(vaos[kind], indices[kind]);
                for (unsigned attribute = 0; attribute < 3; ++attribute) {
                    gl.EnableVertexArrayAttrib(vaos[kind], attribute);
                    gl.VertexArrayAttribBinding(vaos[kind], attribute, 0);
                    gl.VertexArrayAttribFormat(vaos[kind], attribute, attribute == 2 ? 2 : 3, GL_FLOAT,
                                               GL_FALSE, attribute == 2 ? (kind ? 24 : 32) : attribute * 12);
                }
            }
            gl.CreateSamplers(1, &sampler);
            for (auto axis : {GL_TEXTURE_WRAP_S, GL_TEXTURE_WRAP_T, GL_TEXTURE_WRAP_R})
                gl.SamplerParameteri(sampler, axis, GL_REPEAT);
            gl.SamplerParameteri(sampler, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
            gl.SamplerParameteri(sampler, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            gl.SamplerParameterf(sampler, GL_TEXTURE_MAX_ANISOTROPY, 1);
            gl.SamplerParameterf(sampler, GL_TEXTURE_LOD_BIAS, 0);
            gl.SamplerParameterf(sampler, GL_TEXTURE_MIN_LOD, -1000);
            gl.SamplerParameterf(sampler, GL_TEXTURE_MAX_LOD, 1000);
            context->check("FOW original geometry, shaders and sampler0");
        } catch (...) {
            release();
            throw;
        }
    }
    void release() noexcept {
        try {
            context->api();
            gl.DeleteVertexArrays(2, vaos.data());
            gl.DeleteBuffers(2, vertices.data());
            gl.DeleteBuffers(2, indices.data());
            gl.DeleteSamplers(1, &sampler);
            GLint current = 0;
            context->api().GetIntegerv(GL_CURRENT_PROGRAM, &current);
            for (auto program : programs)
                if (program) {
                    if (static_cast<GLuint>(current) == program)
                        gl.UseProgram(0);
                    gl.DeleteProgram(program);
                }
            vaos = {};
            vertices = {};
            indices = {};
            programs = {};
            sampler = 0;
        } catch (...) {
            std::terminate();
        }
    }
    ~Impl() { release(); }
};
GlFogOfWarRenderer::GlFogOfWarRenderer(std::shared_ptr<GlContext> context) {
    require(bool(context), "FOW renderer requires an owned context");
    impl_ = std::make_unique<Impl>(std::move(context));
}
GlFogOfWarRenderer::~GlFogOfWarRenderer() = default;
void GlFogOfWarRenderer::draw(const materials::FogOfWarDraw &draw, int width, int height,
                              GLuint cloud_texture, GLuint mask_texture) {
    auto &s = *impl_;
    const bool cloud = draw.geometry >= 2 && draw.geometry <= 4;
    require(width > 0 && height > 0 && draw.geometry <= 5 &&
                ((draw.geometry < 2 && draw.selector <= 2) || (cloud && draw.selector == 0) ||
                 (draw.geometry == 5 && (draw.selector == 3 || draw.selector == 4))),
            "FOW draw geometry/selector domain");
    require(draw.constants.size() == (cloud ? 197 : 96), "FOW constant batch size");
    for (const auto &v : draw.constants)
        require(std::all_of(v.begin(), v.end(), [](float x) { return std::isfinite(x); }),
                "Nonfinite FOW draw constant");
    const auto technique = cloud ? 5 : draw.selector, program = s.programs[technique];
    if (cloud || technique == 4) {
        detail::validate_sampled_2d(*s.context, s.gl, mask_texture);
        s.context->api().BindTextureUnit(1, mask_texture);
        s.gl.BindSampler(1, s.sampler);
    }
    if (cloud) {
        detail::validate_sampled_2d(*s.context, s.gl, cloud_texture);
        s.context->api().BindTextureUnit(0, cloud_texture);
        s.gl.BindSampler(0, s.sampler);
    }
    s.context->api();
    s.gl.ProgramUniform4fv(program, s.constants[technique], static_cast<GLsizei>(draw.constants.size()),
                           draw.constants[0].data());
    s.gl.UseProgram(program);
    s.gl.BindVertexArray(s.vaos[draw.geometry < 2 ? 0 : 1]);
    const detail::NativeClipScope clip(*s.context, s.gl);
    s.gl.DrawElements(GL_TRIANGLES, draw.geometry < 2 ? 1152 : 144, GL_UNSIGNED_INT,
                      reinterpret_cast<const void *>(std::uintptr_t(draw.geometry == 1 ? 1152 * 4 : 0)));
    s.context->check("Original FOW draw");
}
} // namespace civ5::graphics
