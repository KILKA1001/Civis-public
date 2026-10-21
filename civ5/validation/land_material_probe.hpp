#pragma once
#include "../src/graphics/gl_native_clip.hpp"
#include <tuple>
// Validation-only instrumented prefixes. Never linked into production libraries.
struct LandProbeProgram {
    civ5::graphics::GlContext &context;
    civ5::graphics::GlRenderApi gl;
    GLuint program = 0, vao = 0, vb = 0, ib = 0;
    GLsizei count;
    LandProbeProgram(civ5::graphics::GlContext &c, const LandMaterialMesh &mesh, std::string_view fragment,
                     bool measured = false)
        : context(c), gl(c), count(static_cast<GLsizei>(mesh.indices().size())) {
        try {
            program = gl.CreateProgram();
            std::string prepared(fragment);
            if (measured)
                prepared.insert(prepared.find('\n', prepared.find("#version")) + 1,
                                "#define CIV5_MEASURED_SHADOW_AFFINE 1\n");
            for (auto [stage, source] :
                 {std::pair{GL_VERTEX_SHADER, land_material_vertex_shader(mesh.input())},
                  {GL_FRAGMENT_SHADER, std::string_view(prepared)}}) {
                const auto shader = gl.CreateShader(stage);
                const auto *data = source.data();
                const auto size = static_cast<GLint>(source.size());
                gl.ShaderSource(shader, 1, &data, &size);
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
            gl.CreateVertexArrays(1, &vao);
            gl.CreateBuffers(1, &vb);
            gl.CreateBuffers(1, &ib);
            gl.NamedBufferData(vb, mesh.vertices().size_bytes(), mesh.vertices().data(), GL_STATIC_DRAW);
            gl.NamedBufferData(ib, mesh.indices().size_bytes(), mesh.indices().data(), GL_STATIC_DRAW);
            gl.VertexArrayVertexBuffer(vao, 0, vb, 0, 24);
            gl.VertexArrayElementBuffer(vao, ib);
            for (GLuint i = 0; i < 3; ++i) {
                gl.EnableVertexArrayAttrib(vao, i);
                gl.VertexArrayAttribBinding(vao, i, 0);
                gl.VertexArrayAttribFormat(vao, i,
                                           i == 0   ? 3
                                           : i == 1 ? 4
                                                    : 2,
                                           i == 0 ? GL_FLOAT : GL_HALF_FLOAT, GL_FALSE,
                                           i == 0   ? 0
                                           : i == 1 ? 12
                                                    : 20);
            }
            context.check("Validation-only LAND prefix initialization");
        } catch (...) {
            release();
            throw;
        }
    }
    void release() noexcept {
        gl.UseProgram(0);
        gl.DeleteProgram(program);
        gl.DeleteBuffers(1, &vb);
        gl.DeleteBuffers(1, &ib);
        gl.DeleteVertexArrays(1, &vao);
    }
    ~LandProbeProgram() { release(); }
    void draw(const LandMaterialConstants &c, float diagnostic_patch = -1) {
        // Literal same registers; caller first runs full production draw to bind
        // textures/samplers. This path changes only program and owned geometry.
        std::array<LandVector, 40> v{};
        std::array<LandVector, 23> p{};
        std::copy(c.view_projection.begin(), c.view_projection.end(), v.begin());
        std::copy(c.composite_projection.begin(), c.composite_projection.end(), v.begin() + 4);
        v[9] = c.fow_scale;
        v[16] = c.world_scales;
        v[22] = c.height_texture_scale;
        std::copy(c.world.begin(), c.world.end(), v.begin() + 35);
        v[39] = c.offset;
        p[3] = c.camera;
        p[9][0] = c.shadow_fade;
        p[11][0] = c.apply_specular;
        p[12][0] = c.apply_diffuse;
        std::copy(c.shadow_projection.begin(), c.shadow_projection.end(), p.begin() + 15);
        p[21] = c.fog_color;
        p[22] = c.fog;
        if (diagnostic_patch >= 0)
            p[13].fill(diagnostic_patch);
        for (auto [name, count, data] : {std::tuple{"uV[0]", 40, v[0].data()}, {"uP[0]", 23, p[0].data()}}) {
            const auto loc = gl.GetUniformLocation(program, name);
            if (loc >= 0)
                gl.ProgramUniform4fv(program, loc, count, data);
        }
        gl.UseProgram(program);
        gl.BindVertexArray(vao);
        const civ5::graphics::detail::NativeClipScope clip(context, gl);
        gl.DrawElements(GL_TRIANGLES, count, GL_UNSIGNED_SHORT, nullptr);
        context.check("Validation-only LAND prefix draw");
    }
};
