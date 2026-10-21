#include "civ5/graphics/gl_water_surface.hpp"
#include "gl_native_clip.hpp"
#include "gl_sampled_input.hpp"
#include <algorithm>
#include <cmath>
#include <exception>
#include <limits>
#include <stdexcept>

namespace civ5::graphics {
namespace {
void require(bool ok, const char *why) {
    if (!ok)
        throw std::invalid_argument(why);
}
void validate(const materials::WaterConstantBlock &c) {
    // Untouched components of the original block may contain arbitrary bits.
    // Only the shader's actual vectors/scalars are consumed below.
    for (unsigned i = 0; i < 145; ++i) {
        if ((i >= 32 && i < 36) || (i >= 40 && i < 44) || (i >= 45 && i < 48) || (i >= 49 && i < 52) ||
            (i >= 53 && i < 56) || i == 59 || i == 63 || i == 87 || (i >= 137 && i < 140))
            continue;
        require(std::isfinite(c[i]), "Nonfinite water shader constant");
    }
}
void validate_mesh(const materials::WaterSurfaceMesh &mesh) {
    require(mesh.vertices.size() <= std::numeric_limits<GLsizei>::max() &&
                mesh.indices.size() <= std::numeric_limits<GLsizei>::max() && mesh.indices.size() % 3 == 0,
            "Invalid water mesh extent");
    for (const auto &v : mesh.vertices)
        for (float f : v)
            require(std::isfinite(f), "Nonfinite water vertex");
    for (auto index : mesh.indices)
        require(index < mesh.vertices.size(), "Water index outside vertex stream");
}
} // namespace
struct GlWaterSurfaceRenderer::Impl {
    struct Pipeline {
        std::shared_ptr<GlContext> context;
        GlRenderApi gl;
        GLuint program;
        std::array<GLuint, 2> samplers;
        Pipeline(std::shared_ptr<GlContext> c, GLuint p, std::array<GLuint, 2> s)
            : context(std::move(c)), gl(*context), program(p), samplers(s) {}
        ~Pipeline() {
            try {
                GLint current = 0;
                context->api().GetIntegerv(GL_CURRENT_PROGRAM, &current);
                if (static_cast<GLuint>(current) == program)
                    gl.UseProgram(0);
                gl.DeleteProgram(program);
                gl.DeleteSamplers(2, samplers.data());
            } catch (...) {
                std::terminate();
            }
        }
    };
    std::shared_ptr<GlContext> context;
    std::shared_ptr<Pipeline> pipeline;
    GlRenderApi gl;
    GLuint program = 0, vao = 0, vb = 0, ib = 0;
    std::array<GLuint, 2> samplers{};
    unsigned selector;
    GLsizei vertices, indices;
    GLint vs = -1, ps = -1;
    Impl(std::shared_ptr<GlContext> c, const materials::WaterSurfaceMesh &mesh, unsigned selected,
         std::shared_ptr<Pipeline> shared = {})
        : context(std::move(c)), gl(*context), selector(selected),
          vertices(static_cast<GLsizei>(mesh.vertices.size())),
          indices(static_cast<GLsizei>(mesh.indices.size())) {
        try {
            context->check("Before water surface initialization");
            if (shared) {
                pipeline = std::move(shared);
                program = pipeline->program;
                samplers = pipeline->samplers;
            } else {
                program = gl.CreateProgram();
                const auto fragment = water_surface_fragment_shader(selector);
                for (auto [stage, text] : {std::pair{GL_VERTEX_SHADER, water_surface_vertex_shader()},
                                           {GL_FRAGMENT_SHADER, std::string_view(fragment)}}) {
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
                const char *varyings[]{"tfClip",  "waterT0",    "waterT1",   "waterT2",
                                       "waterT3", "waterWorld", "waterClip", "waterT6"};
                gl.TransformFeedbackVaryings(program, 8, varyings, GL_INTERLEAVED_ATTRIBS);
                gl.LinkProgram(program);
                GLint ok = 0;
                gl.GetProgramiv(program, GL_LINK_STATUS, &ok);
                if (!ok) {
                    char log[8192]{};
                    gl.GetProgramInfoLog(program, sizeof(log), nullptr, log);
                    throw std::runtime_error(log);
                }
            }
            vs = gl.GetUniformLocation(program, "uV[0]");
            ps = gl.GetUniformLocation(program, "uP[0]");
            require(vs >= 0 && (selector == 5 || ps >= 0), "Missing water constants");
            gl.CreateVertexArrays(1, &vao);
            gl.CreateBuffers(1, &vb);
            gl.CreateBuffers(1, &ib);
            gl.NamedBufferData(vb, mesh.vertices.size() * 16, mesh.vertices.data(), GL_STATIC_DRAW);
            gl.NamedBufferData(ib, mesh.indices.size() * 2, mesh.indices.data(), GL_STATIC_DRAW);
            gl.VertexArrayVertexBuffer(vao, 0, vb, 0, 16);
            gl.VertexArrayElementBuffer(vao, ib);
            gl.EnableVertexArrayAttrib(vao, 0);
            gl.VertexArrayAttribBinding(vao, 0, 0);
            gl.VertexArrayAttribFormat(vao, 0, 4, GL_FLOAT, GL_FALSE, 0);
            if (!pipeline) {
                gl.CreateSamplers(2, samplers.data());
                for (unsigned i = 0; i < 2; ++i) {
                    for (auto wrap : {GL_TEXTURE_WRAP_S, GL_TEXTURE_WRAP_T, GL_TEXTURE_WRAP_R})
                        gl.SamplerParameteri(samplers[i], wrap, i == 0 ? GL_REPEAT : GL_CLAMP_TO_EDGE);
                    gl.SamplerParameteri(samplers[i], GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
                    gl.SamplerParameteri(samplers[i], GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                    gl.SamplerParameterf(samplers[i], GL_TEXTURE_MAX_ANISOTROPY, 1);
                    gl.SamplerParameterf(samplers[i], GL_TEXTURE_LOD_BIAS, 0);
                    gl.SamplerParameterf(samplers[i], GL_TEXTURE_MIN_LOD, -1000);
                    gl.SamplerParameterf(samplers[i], GL_TEXTURE_MAX_LOD, 1000);
                }
                pipeline = std::make_shared<Pipeline>(context, program, samplers);
            }
            context->check("Water surface program, geometry and original samplers");
        } catch (...) {
            release();
            throw;
        }
    }
    void release() noexcept {
        try {
            context->api();
            gl.DeleteBuffers(1, &vb);
            gl.DeleteBuffers(1, &ib);
            gl.DeleteVertexArrays(1, &vao);
            if (!pipeline) {
                gl.DeleteSamplers(2, samplers.data());
                if (program) {
                    GLint current = 0;
                    context->api().GetIntegerv(GL_CURRENT_PROGRAM, &current);
                    if (static_cast<GLuint>(current) == program)
                        gl.UseProgram(0);
                    gl.DeleteProgram(program);
                }
            }
            pipeline.reset();
            vb = ib = vao = program = 0;
            samplers = {};
        } catch (...) {
            std::terminate();
        }
    }
    ~Impl() { release(); }
    void constants(const materials::WaterConstantBlock &c, int width, int height) {
        validate(c);
        require(width > 0 && height > 0, "Positive water target dimensions required");
        const auto registers = materials::water_shader_registers(c);
        gl.ProgramUniform4fv(program, vs, 17, registers.vertex.data());
        if (ps >= 0)
            gl.ProgramUniform4fv(program, ps, 30, registers.pixel.data());
    }
};
GlWaterSurfaceRenderer::GlWaterSurfaceRenderer(std::shared_ptr<GlContext> context,
                                               const materials::WaterSurfaceMesh &mesh, unsigned selector) {
    require(bool(context), "Water renderer requires owned context");
    water_surface_fragment_shader(selector);
    validate_mesh(mesh);
    impl_ = std::make_unique<Impl>(std::move(context), mesh, selector);
}
GlWaterSurfaceRenderer::GlWaterSurfaceRenderer(const GlWaterSurfaceRenderer &source,
                                               const materials::WaterSurfaceMesh &mesh) {
    validate_mesh(mesh);
    const auto &s = *source.impl_;
    impl_ = std::make_unique<Impl>(s.context, mesh, s.selector, s.pipeline);
}
GlWaterSurfaceRenderer::~GlWaterSurfaceRenderer() = default;
void GlWaterSurfaceRenderer::draw(const materials::WaterConstantBlock &c, const GlWaterSurfaceInputs &inputs,
                                  std::uint32_t first, std::uint32_t triangles, int width, int height) {
    auto &s = *impl_;
    validate(c);
    require(width > 0 && height > 0, "Positive water target dimensions required");
    require(first % 3 == 0 && std::uint64_t(first) + std::uint64_t(triangles) * 3 <= unsigned(s.indices),
            "Water packet index range outside owned mesh");
    if (!triangles)
        return;
    if (s.selector != 5) {
        for (unsigned slot : {0u, 1u, 4u, 5u, 6u, 7u, 8u, 9u, 10u, 11u, 13u}) {
            if (s.selector == 0 && (slot == 1 || slot == 4))
                continue;
            detail::validate_sampled_texture(*s.context, s.gl, inputs.pixel[slot],
                                             slot == 11 ? GL_TEXTURE_3D : GL_TEXTURE_2D);
            if (slot == 6) {
                GLint format = 0;
                s.context->api().GetTextureLevelParameteriv(inputs.pixel[slot], 0, GL_TEXTURE_INTERNAL_FORMAT,
                                                            &format);
                require(format == GL_SRGB8_ALPHA8, "WaterColor requires the original RGBA8 sRGB decode");
            }
        }
        for (unsigned slot : {0u, 1u, 4u, 5u, 6u, 7u, 8u, 9u, 10u, 11u, 13u}) {
            if (s.selector == 0 && (slot == 1 || slot == 4))
                continue;
            s.context->api().BindTextureUnit(slot, inputs.pixel[slot]);
            s.gl.BindSampler(slot, s.samplers[slot == 6 || slot == 8 || slot == 11 ? 1 : 0]);
        }
    }
    s.constants(c, width, height);
    s.gl.UseProgram(s.program);
    s.gl.BindVertexArray(s.vao);
    const detail::NativeClipScope clip(*s.context, s.gl);
    s.gl.DrawElements(GL_TRIANGLES, static_cast<GLsizei>(triangles * 3), GL_UNSIGNED_SHORT,
                      reinterpret_cast<const void *>(std::uintptr_t(first) * 2));
    s.context->check("Complete key5b water surface draw");
}
std::vector<float> GlWaterSurfaceRenderer::capture(const materials::WaterConstantBlock &c, bool indexed) {
    auto &s = *impl_;
    validate(c);
    GLint active = 0, previous = 0;
    s.context->api().GetIntegerv(GL_TRANSFORM_FEEDBACK_ACTIVE, &active);
    require(!active, "Nested water transform feedback rejected");
    s.context->api().GetIntegerv(GL_TRANSFORM_FEEDBACK_BINDING, &previous);
    const auto discard = s.gl.IsEnabled(GL_RASTERIZER_DISCARD);
    s.constants(c, 64, 64);
    const auto count = indexed ? s.indices : s.vertices;
    std::vector<float> out(std::size_t(count) * 22);
    if (!count)
        return out;
    GLuint buffer = 0, feedback = 0;
    const auto cleanup = [&] {
        s.gl.BindTransformFeedback(GL_TRANSFORM_FEEDBACK, previous);
        if (!discard)
            s.gl.Disable(GL_RASTERIZER_DISCARD);
        s.gl.DeleteTransformFeedbacks(1, &feedback);
        s.gl.DeleteBuffers(1, &buffer);
    };
    bool started = false;
    try {
        s.gl.CreateBuffers(1, &buffer);
        s.gl.NamedBufferData(buffer, out.size() * 4, nullptr, GL_STREAM_READ);
        s.gl.CreateTransformFeedbacks(1, &feedback);
        s.gl.TransformFeedbackBufferBase(feedback, 0, buffer);
        s.gl.BindTransformFeedback(GL_TRANSFORM_FEEDBACK, feedback);
        s.gl.Enable(GL_RASTERIZER_DISCARD);
        s.gl.UseProgram(s.program);
        s.gl.BindVertexArray(s.vao);
        s.gl.BeginTransformFeedback(indexed ? GL_TRIANGLES : GL_POINTS);
        started = true;
        if (indexed)
            s.gl.DrawElements(GL_TRIANGLES, count, GL_UNSIGNED_SHORT, nullptr);
        else
            s.gl.DrawArrays(GL_POINTS, 0, count);
        s.gl.EndTransformFeedback();
        started = false;
        s.gl.GetNamedBufferSubData(buffer, 0, out.size() * 4, out.data());
        s.context->check("Original water varyings capture");
        cleanup();
    } catch (...) {
        if (started)
            s.gl.EndTransformFeedback();
        cleanup();
        throw;
    }
    return out;
}
} // namespace civ5::graphics
