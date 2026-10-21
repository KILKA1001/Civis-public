#include "civ5/graphics/gl_water_current.hpp"
#include "gl_sampled_input.hpp"
#include <cmath>
#include <exception>
#include <stdexcept>

namespace civ5::graphics {
namespace {
void require(bool ok, const char *why) {
    if (!ok)
        throw std::invalid_argument(why);
}
}
struct GlWaterCurrentRenderer::Impl {
    std::shared_ptr<GlContext> context;
    GlRenderApi gl;
    GLuint program = 0, vao = 0, vb = 0, ib = 0, sampler = 0;
    GLint constants = -1, center = -1;
    explicit Impl(std::shared_ptr<GlContext> c) : context(std::move(c)), gl(*context) {
        try {
            context->check("Before current-map initialization");
            program = gl.CreateProgram();
            for (auto [stage, text] : {std::pair{GL_VERTEX_SHADER, water_current_vertex_shader()},
                                      {GL_FRAGMENT_SHADER, water_current_fragment_shader()}}) {
                const auto shader = gl.CreateShader(stage);
                const char *source = text.data();
                const auto length = static_cast<GLint>(text.size());
                gl.ShaderSource(shader, 1, &source, &length);
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
            constants = gl.GetUniformLocation(program, "uP[0]");
            center = gl.GetUniformLocation(program, "uPixelCenter");
            require(constants >= 0 && center >= 0, "Missing current-map constants");
            const auto quad = materials::water_current_quad();
            gl.CreateVertexArrays(1, &vao);
            gl.CreateBuffers(1, &vb);
            gl.CreateBuffers(1, &ib);
            gl.NamedBufferData(vb, sizeof(quad.vertices), quad.vertices.data(), GL_STATIC_DRAW);
            gl.NamedBufferData(ib, sizeof(quad.indices), quad.indices.data(), GL_STATIC_DRAW);
            gl.VertexArrayVertexBuffer(vao, 0, vb, 0, 32);
            gl.VertexArrayElementBuffer(vao, ib);
            for (unsigned attribute = 0; attribute < 2; ++attribute) {
                gl.EnableVertexArrayAttrib(vao, attribute);
                gl.VertexArrayAttribBinding(vao, attribute, 0);
                gl.VertexArrayAttribFormat(vao, attribute, attribute == 0 ? 3 : 2, GL_FLOAT,
                                          GL_FALSE, attribute == 0 ? 0 : 24);
            }
            gl.CreateSamplers(1, &sampler);
            for (auto wrap : {GL_TEXTURE_WRAP_S, GL_TEXTURE_WRAP_T, GL_TEXTURE_WRAP_R})
                gl.SamplerParameteri(sampler, wrap, GL_REPEAT);
            gl.SamplerParameteri(sampler, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
            gl.SamplerParameteri(sampler, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            gl.SamplerParameterf(sampler, GL_TEXTURE_MAX_ANISOTROPY, 1);
            gl.SamplerParameterf(sampler, GL_TEXTURE_LOD_BIAS, 0);
            gl.SamplerParameterf(sampler, GL_TEXTURE_MIN_LOD, -1000);
            gl.SamplerParameterf(sampler, GL_TEXTURE_MAX_LOD, 1000);
            context->check("Current-map program, shared quad and sampler0");
        } catch (...) {
            release();
            throw;
        }
    }
    void release() noexcept {
        try {
            // The owner can be destroyed after another context became
            // current during a context replacement. Restore the owner before
            // deleting its names.
            context->api();
            gl.DeleteBuffers(1, &vb);
            gl.DeleteBuffers(1, &ib);
            gl.DeleteVertexArrays(1, &vao);
            gl.DeleteSamplers(1, &sampler);
            if (program) {
                GLint current = 0;
                context->api().GetIntegerv(GL_CURRENT_PROGRAM, &current);
                if (static_cast<GLuint>(current) == program)
                    gl.UseProgram(0);
                gl.DeleteProgram(program);
            }
            vb = ib = vao = sampler = program = 0;
        } catch (...) {
            std::terminate();
        }
    }
    ~Impl() { release(); }
};
GlWaterCurrentRenderer::GlWaterCurrentRenderer(std::shared_ptr<GlContext> context) {
    require(bool(context), "Current renderer requires owned context");
    impl_ = std::make_unique<Impl>(std::move(context));
}
GlWaterCurrentRenderer::~GlWaterCurrentRenderer() = default;
void GlWaterCurrentRenderer::draw(const materials::WaterCurrentConstants &c, GLuint source,
                                 int width, int height) {
    auto &s = *impl_;
    require(width > 0 && height > 0, "Positive current target dimensions required");
    // The remaining components are retained allocation contents and never read.
    for (unsigned index : {0u, 1u, 4u, 8u, 12u})
        require(std::isfinite(c[index]), "Nonfinite current shader constant");
    detail::validate_sampled_2d(*s.context, s.gl, source);
    s.context->api().BindTextureUnit(0, source);
    s.gl.BindSampler(0, s.sampler);
    s.gl.ProgramUniform4fv(s.program, s.constants, 4, c.data());
    const std::array<float, 2> pixel{1.f / width, 1.f / height};
    s.gl.ProgramUniform2fv(s.program, s.center, 1, pixel.data());
    s.gl.UseProgram(s.program);
    s.gl.BindVertexArray(s.vao);
    s.gl.DrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
    s.context->check("Complete key5d current-map draw");
}
} // namespace civ5::graphics
