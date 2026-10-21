#include "civ5/graphics/gl_land_height.hpp"
#include "civ5/graphics/gl_render_api.hpp"
#include "civ5/graphics/gl_texture.hpp"
#include "civ5/terrain/land_grid.hpp"
#include <algorithm>
#include <cmath>
#include <exception>
#include <map>
#include <stdexcept>

namespace civ5::graphics {
namespace {
constexpr std::array<float, 16> identity{1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};
void require(bool ok, const char *reason) {
    if (!ok)
        throw std::invalid_argument(reason);
}
} // namespace
std::string_view land_height_vertex_shader() {
    // Position path only of05D97348; same GLSL operations as frozen Kotlin port.
    return R"(#version 460 core
layout(location=0) in vec3 aPosition;
uniform sampler2D uHeight;
uniform vec4 uOffset;
uniform vec3 uWorldScales;
uniform vec4 uWorld[4];
uniform mat4 uViewProjection;
out vec4 tfWorld;
out vec2 tfHeightUv;
void main() {
    precise vec2 source = uOffset.xy + aPosition.xy;
    precise vec2 address = source + uOffset.zw;
    address = address + vec2(0.5);
    precise vec2 uv = address * vec2(0.0009765625,-0.0009765625);
    precise vec2 xy = source * uWorldScales.xy;
    precise float z = textureLod(uHeight,uv,0.0).a * uWorldScales.z;
    z = z * 255.0;
    precise vec4 world = xy.y * uWorld[1];
    world = xy.x * uWorld[0] + world;
    world = z * uWorld[2] + world;
    world = world + uWorld[3];
    tfWorld = world;
    tfHeightUv = uv;
    gl_Position = uViewProjection * world;
})";
}
std::string_view land_height_fragment_shader() {
    return R"(#version 460 core
in vec4 tfWorld;
layout(location=0) out vec4 outColor;
void main() { outColor = vec4(vec3(tfWorld.z / 51.0),1.0); })";
}
struct GlLandHeightRenderer::Impl {
    struct Mesh {
        GLuint vao = 0, vb = 0, ib = 0;
        GLsizei vertices = 0, indices = 0;
    };
    std::shared_ptr<GlContext> context;
    std::shared_ptr<const terrain::HeightField> field;
    terrain::LandLayout layout;
    int step;
    GlRenderApi gl;
    GLuint program = 0, sampler = 0;
    std::array<GLint, 5> uniforms{};
    std::map<std::pair<int, int>, Mesh> meshes;
    std::map<int, GlTexture> textures;
    Impl(std::shared_ptr<GlContext> c, std::shared_ptr<const terrain::HeightField> f, int s)
        : context(std::move(c)), field(std::move(f)), layout(field->domain()), step(s), gl(*context) {
        try {
            context->check("Before LAND renderer initialization");
            program = gl.CreateProgram();
            for (const auto [kind, source] : {std::pair{GL_VERTEX_SHADER, land_height_vertex_shader()},
                                              {GL_FRAGMENT_SHADER, land_height_fragment_shader()}}) {
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
            const char *varyings[] = {"tfWorld", "tfHeightUv"};
            gl.TransformFeedbackVaryings(program, 2, varyings, GL_INTERLEAVED_ATTRIBS);
            gl.LinkProgram(program);
            GLint ok = 0;
            gl.GetProgramiv(program, GL_LINK_STATUS, &ok);
            if (!ok) {
                char log[8192]{};
                gl.GetProgramInfoLog(program, sizeof(log), nullptr, log);
                throw std::runtime_error(log);
            }
            const char *names[] = {"uViewProjection", "uWorld[0]", "uOffset", "uWorldScales", "uHeight"};
            for (std::size_t i = 0; i < uniforms.size(); ++i) {
                uniforms[i] = gl.GetUniformLocation(program, names[i]);
                require(uniforms[i] >= 0, "Missing LAND uniform");
            }
            gl.ProgramUniform1i(program, uniforms[4], 0);
            gl.CreateSamplers(1, &sampler);
            gl.SamplerParameteri(sampler, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            gl.SamplerParameteri(sampler, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            gl.SamplerParameteri(sampler, GL_TEXTURE_WRAP_S, GL_REPEAT);
            gl.SamplerParameteri(sampler, GL_TEXTURE_WRAP_T, GL_REPEAT);
            context->check("Create native LAND height program/sampler");
        } catch (...) {
            release();
            throw;
        }
    }
    void release_mesh(const Mesh &m) noexcept {
        gl.DeleteVertexArrays(1, &m.vao);
        gl.DeleteBuffers(1, &m.vb);
        gl.DeleteBuffers(1, &m.ib);
    }
    void release() noexcept {
        try {
            context->api();
            textures.clear();
            for (const auto &[key, m] : meshes)
                release_mesh(m);
            meshes.clear();
            if (sampler) {
                gl.DeleteSamplers(1, &sampler);
                sampler = 0;
            }
            if (program) {
                GLint active = 0;
                context->api().GetIntegerv(GL_CURRENT_PROGRAM, &active);
                if (static_cast<GLuint>(active) == program)
                    gl.UseProgram(0);
                gl.DeleteProgram(program);
                program = 0;
            }
        } catch (...) {
            std::terminate();
        }
    }
    ~Impl() { release(); }
    const Mesh &prepare(int column, int row, const std::array<float, 16> &vp) {
        require(std::all_of(vp.begin(), vp.end(), [](float v) { return std::isfinite(v); }),
                "Nonfinite LAND camera");
        const auto patch = layout.patch(column, row);
        const auto &api = context->api();
        context->check("Before LAND draw");
        const auto key = std::pair{patch.source_samples_x(), patch.source_samples_y()};
        auto mesh = meshes.find(key);
        if (mesh == meshes.end()) {
            const terrain::LandGrid source(key.first, key.second, step);
            Mesh m;
            m.vertices = static_cast<GLsizei>(source.vertices().size());
            m.indices = static_cast<GLsizei>(source.indices().size());
            try {
                gl.CreateVertexArrays(1, &m.vao);
                gl.CreateBuffers(1, &m.vb);
                gl.CreateBuffers(1, &m.ib);
                gl.NamedBufferData(m.vb, source.vertices().size_bytes(), source.vertices().data(),
                                   GL_STATIC_DRAW);
                gl.NamedBufferData(m.ib, source.indices().size_bytes(), source.indices().data(),
                                   GL_STATIC_DRAW);
                gl.VertexArrayVertexBuffer(m.vao, 0, m.vb, 0, sizeof(terrain::LandGridVertex));
                gl.VertexArrayElementBuffer(m.vao, m.ib);
                gl.EnableVertexArrayAttrib(m.vao, 0);
                gl.VertexArrayAttribFormat(m.vao, 0, 3, GL_FLOAT, GL_FALSE, 0);
                gl.VertexArrayAttribBinding(m.vao, 0, 0);
                context->check("Native24-byte LAND buffers/VAO");
                mesh = meshes.emplace(key, m).first;
            } catch (...) {
                release_mesh(m);
                throw;
            }
        }
        auto texture = textures.find(patch.texture_table_slot());
        if (texture == textures.end()) {
            const auto bytes =
                field->extract_a8_texture(patch.height_window().tile_x(), patch.height_window().tile_y());
            texture =
                textures.emplace(patch.texture_table_slot(), GlTexture::alpha8(*context, 1024, 1024, bytes))
                    .first;
        }
        auto world = identity;
        const auto translation = patch.world_translation();
        std::copy(translation.begin(), translation.end(), world.begin() + 12);
        gl.ProgramUniformMatrix4fv(program, uniforms[0], 1, GL_FALSE, vp.data());
        gl.ProgramUniform4fv(program, uniforms[1], 4, world.data());
        gl.ProgramUniform4fv(program, uniforms[2], 1, patch.offset_constants().data());
        gl.ProgramUniform3f(program, uniforms[3], terrain::LandLayout::scale_x, terrain::LandLayout::scale_y,
                            terrain::LandLayout::scale_z);
        gl.UseProgram(program);
        gl.BindSampler(0, sampler);
        api.BindTextureUnit(0, texture->second.id());
        gl.BindVertexArray(mesh->second.vao);
        context->check("Prepare native LAND draw");
        return mesh->second;
    }
};
GlLandHeightRenderer::GlLandHeightRenderer(std::shared_ptr<GlContext> context,
                                           std::shared_ptr<const terrain::HeightField> field, int step) {
    require(context && field, "Missing LAND context/field");
    require(step >= 1 && step <= 3, "Invalid explicit LAND index step");
    impl_ = std::make_unique<Impl>(std::move(context), std::move(field), step);
}
GlLandHeightRenderer::~GlLandHeightRenderer() = default;
const terrain::LandLayout &GlLandHeightRenderer::layout() const { return impl_->layout; }
void GlLandHeightRenderer::draw(int column, int row, const std::array<float, 16> &vp) {
    auto &s = *impl_;
    const auto &mesh = s.prepare(column, row, vp);
    s.gl.DrawElements(GL_TRIANGLES, mesh.indices, GL_UNSIGNED_SHORT, nullptr);
    s.context->check("Draw native LAND triangles");
}
std::vector<float> GlLandHeightRenderer::capture(int column, int row, bool indexed) {
    auto &s = *impl_;
    const auto &api = s.context->api();
    GLint active = 0;
    api.GetIntegerv(GL_TRANSFORM_FEEDBACK_ACTIVE, &active);
    require(active == 0, "Cannot capture inside another active transform feedback");
    const auto &mesh = s.prepare(column, row, identity);
    const auto count = indexed ? mesh.indices : mesh.vertices;
    std::vector<float> result(static_cast<std::size_t>(count) * 6);
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
        s.context->check("LAND feedback allocation");
        s.gl.BeginTransformFeedback(indexed ? GL_TRIANGLES : GL_POINTS);
        s.context->check("Begin LAND feedback");
        began = true;
        if (indexed)
            s.gl.DrawElements(GL_TRIANGLES, count, GL_UNSIGNED_SHORT, nullptr);
        else
            s.gl.DrawArrays(GL_POINTS, 0, count);
        s.gl.EndTransformFeedback();
        began = false;
        s.gl.GetNamedBufferSubData(buffer, 0, result.size() * sizeof(float), result.data());
        s.context->check("Read actual LAND vertex output");
    } catch (...) {
        restore();
        throw;
    }
    restore();
    s.context->check("LAND feedback state restoration");
    return result;
}
} // namespace civ5::graphics
