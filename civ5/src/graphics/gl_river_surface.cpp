#include "civ5/graphics/gl_river_surface.hpp"
#include "gl_native_clip.hpp"
#include "gl_sampled_input.hpp"
#include <algorithm>
#include <cmath>
#include <exception>
#include <limits>
#include <stdexcept>

namespace civ5::graphics {
namespace {
// EXT_texture_compression_s3tc_srgb, absent from the OpenGL Core header.
constexpr GLint srgb_alpha_dxt5 = 0x8c4f;
void require(bool ok, const char *why) {
    if (!ok)
        throw std::invalid_argument(why);
}
void validate(const materials::WaterConstantBlock &c) {
    // Original river VS reads VP/overlay/phase/scale. The pixel variants read
    // lighting, camera, reflection, fog and ColorKey inputs in these ranges.
    for (unsigned i = 0; i < 145; ++i) {
        const bool consumed = i < 36 || (i >= 40 && i < 44) || i == 52 || (i >= 56 && i < 59) ||
                              (i >= 60 && i < 87) || (i >= 88 && i < 104) || i >= 140;
        if (consumed)
            require(std::isfinite(c[i]), "Nonfinite river shader constant");
    }
}
void validate_mesh(const terrain::RiverSurfaceMesh &mesh) {
    materials::river_surface_upload(mesh);
    require(mesh.vertices.size() <= std::numeric_limits<GLsizei>::max() &&
                mesh.indices.size() <= std::numeric_limits<GLsizei>::max(),
            "River mesh exceeds GL draw extent");
}
} // namespace
struct GlRiverSurfaceRenderer::Impl {
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
    Impl(std::shared_ptr<GlContext> c, const terrain::RiverSurfaceMesh &mesh, unsigned selected,
         std::shared_ptr<Pipeline> shared = {})
        : context(std::move(c)), gl(*context), selector(selected),
          vertices(static_cast<GLsizei>(mesh.vertices.size())),
          indices(static_cast<GLsizei>(mesh.indices.size())) {
        try {
            context->check("Before river surface initialization");
            if (shared) {
                pipeline = std::move(shared);
                program = pipeline->program;
                samplers = pipeline->samplers;
            } else {
                program = gl.CreateProgram();
                const auto fragment = river_surface_fragment_shader(selector);
                for (auto [stage, text] : {std::pair{GL_VERTEX_SHADER, river_surface_vertex_shader()},
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
            require(vs >= 0 && ps >= 0, "Missing river constants");
            gl.CreateVertexArrays(1, &vao);
            gl.CreateBuffers(1, &vb);
            gl.CreateBuffers(1, &ib);
            gl.NamedBufferData(vb, mesh.vertices.size() * 32, mesh.vertices.data(), GL_STATIC_DRAW);
            gl.NamedBufferData(ib, mesh.indices.size() * 4, mesh.indices.data(), GL_STATIC_DRAW);
            gl.VertexArrayVertexBuffer(vao, 0, vb, 0, 32);
            gl.VertexArrayElementBuffer(vao, ib);
            for (unsigned attribute = 0; attribute < 3; ++attribute) {
                gl.EnableVertexArrayAttrib(vao, attribute);
                gl.VertexArrayAttribBinding(vao, attribute, 0);
                gl.VertexArrayAttribFormat(vao, attribute, attribute ? 2 : 3, GL_FLOAT, GL_FALSE,
                                           attribute == 0   ? 0
                                           : attribute == 1 ? 12
                                                            : 20);
            }
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
            context->check("River surface program, geometry and original samplers");
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
        require(width > 0 && height > 0, "Positive river target dimensions required");
        const auto registers = materials::water_shader_registers(c);
        gl.ProgramUniform4fv(program, vs, 17, registers.vertex.data());
        if (ps >= 0)
            gl.ProgramUniform4fv(program, ps, 30, registers.pixel.data());
    }
};
GlRiverSurfaceRenderer::GlRiverSurfaceRenderer(std::shared_ptr<GlContext> context,
                                               const terrain::RiverSurfaceMesh &mesh, unsigned selector) {
    require(context != nullptr, "River renderer requires a GL context");
    require(bool(context), "River renderer requires owned context");
    river_surface_fragment_shader(selector);
    validate_mesh(mesh);
    impl_ = std::make_unique<Impl>(std::move(context), mesh, selector);
}
GlRiverSurfaceRenderer::GlRiverSurfaceRenderer(const GlRiverSurfaceRenderer &source,
                                               const terrain::RiverSurfaceMesh &mesh) {
    validate_mesh(mesh);
    const auto &s = *source.impl_;
    impl_ = std::make_unique<Impl>(s.context, mesh, s.selector, s.pipeline);
}
GlRiverSurfaceRenderer::~GlRiverSurfaceRenderer() = default;
void GlRiverSurfaceRenderer::draw(const materials::WaterConstantBlock &c, const GlRiverSurfaceInputs &inputs,
                                  int width, int height) {
    auto &s = *impl_;
    validate(c);
    require(width > 0 && height > 0, "Positive river target dimensions required");
    if (!s.vertices || !s.indices)
        return;
    for (unsigned slot = 0; slot < 13; ++slot) {
        if (slot == 7 || (s.selector == 1 && slot < 5))
            continue;
        detail::validate_sampled_texture(*s.context, s.gl, inputs.pixel[slot],
                                         slot >= 11 ? GL_TEXTURE_3D : GL_TEXTURE_2D);
        if (slot < 4 || slot == 6) {
            GLint format = 0;
            s.context->api().GetTextureLevelParameteriv(inputs.pixel[slot], 0, GL_TEXTURE_INTERNAL_FORMAT,
                                                        &format);
            if (slot < 4)
                require(format == (slot % 2 ? GL_R16 : GL_RGBA16),
                        "River LEAN filtering requires original L16 or RGBA16 UNORM storage");
            else
                require(format == GL_SRGB8_ALPHA8 || format == srgb_alpha_dxt5,
                        "River WaterColor requires original BC3 or RGBA8 sRGB decode");
        }
    }
    for (unsigned slot = 0; slot < 13; ++slot) {
        if (slot == 7 || (s.selector == 1 && slot < 5))
            continue;
        s.context->api().BindTextureUnit(slot, inputs.pixel[slot]);
        s.gl.BindSampler(slot, s.samplers[slot == 5 || slot == 6 || slot == 8 || slot >= 11 ? 1 : 0]);
    }
    s.constants(c, width, height);
    s.gl.Enable(GL_BLEND);
    s.gl.BlendEquationSeparate(GL_FUNC_ADD, GL_FUNC_ADD);
    s.gl.BlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE);
    s.gl.ColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    s.gl.Disable(GL_FRAMEBUFFER_SRGB);
    s.gl.Enable(GL_DEPTH_TEST);
    s.gl.DepthMask(GL_TRUE);
    s.gl.DepthFunc(GL_LEQUAL);
    s.gl.Enable(GL_CULL_FACE);
    // The vertex shader flips Y; D3DCULL_CW therefore keeps GL-clockwise faces.
    s.gl.FrontFace(GL_CW);
    s.gl.CullFace(GL_BACK);
    s.gl.PolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    s.gl.Disable(GL_POLYGON_OFFSET_FILL);
    s.gl.Disable(GL_STENCIL_TEST);
    for (unsigned i = 0; i < 8; ++i)
        s.gl.Disable(GL_CLIP_DISTANCE0 + i);
    s.gl.UseProgram(s.program);
    s.gl.BindVertexArray(s.vao);
    const detail::NativeClipScope clip(*s.context, s.gl);
    s.gl.DrawElements(GL_TRIANGLES, s.indices, GL_UNSIGNED_INT, nullptr);
    s.context->check("Complete key5B river surface draw");
}
std::vector<float> GlRiverSurfaceRenderer::capture(const materials::WaterConstantBlock &c, bool indexed) {
    auto &s = *impl_;
    validate(c);
    GLint active = 0, previous = 0;
    s.context->api().GetIntegerv(GL_TRANSFORM_FEEDBACK_ACTIVE, &active);
    require(!active, "Nested river transform feedback rejected");
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
            s.gl.DrawElements(GL_TRIANGLES, count, GL_UNSIGNED_INT, nullptr);
        else
            s.gl.DrawArrays(GL_POINTS, 0, count);
        s.gl.EndTransformFeedback();
        started = false;
        s.gl.GetNamedBufferSubData(buffer, 0, out.size() * 4, out.data());
        s.context->check("Original river varyings capture");
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
