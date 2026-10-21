#pragma once
#include "../src/graphics/gl_native_clip.hpp"
// Validation-only instrumentation. Never substituted for full original controls.
struct RiverProbe {
    GlContext &context;
    GlRenderApi gl;
    GLuint program = 0, vao = 0, vb = 0, ib = 0;
    GLsizei count;
    RiverProbe(GlContext &c, const RiverSurfaceMesh &mesh, std::string_view fragment)
        : context(c), gl(c), count(static_cast<GLsizei>(mesh.indices.size())) {
        try {
            program = gl.CreateProgram();
            for (auto [stage, source] : {std::pair{GL_VERTEX_SHADER, river_surface_vertex_shader()},
                                         {GL_FRAGMENT_SHADER, fragment}}) {
                const auto shader = gl.CreateShader(stage);
                const auto *data = source.data();
                const auto length = GLint(source.size());
                gl.ShaderSource(shader, 1, &data, &length);
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
            gl.NamedBufferData(vb, mesh.vertices.size() * 32, mesh.vertices.data(), GL_STATIC_DRAW);
            gl.NamedBufferData(ib, mesh.indices.size() * 4, mesh.indices.data(), GL_STATIC_DRAW);
            gl.VertexArrayVertexBuffer(vao, 0, vb, 0, 32);
            gl.VertexArrayElementBuffer(vao, ib);
            gl.EnableVertexArrayAttrib(vao, 0);
            gl.VertexArrayAttribBinding(vao, 0, 0);
            gl.VertexArrayAttribFormat(vao, 0, 3, GL_FLOAT, GL_FALSE, 0);
            for (unsigned attribute = 1; attribute < 3; ++attribute) {
                gl.EnableVertexArrayAttrib(vao, attribute);
                gl.VertexArrayAttribBinding(vao, attribute, 0);
                gl.VertexArrayAttribFormat(vao, attribute, 2, GL_FLOAT, GL_FALSE, attribute == 1 ? 12 : 20);
            }
            context.check("River diagnostic program");
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
    ~RiverProbe() { release(); }
    void draw(const WaterConstantBlock &c, int width = 64, int height = 64, unsigned first = 0,
              unsigned triangles = 0xffffffffu) {
        const auto registers = civ5::materials::water_shader_registers(c);
        auto loc = gl.GetUniformLocation(program, "uV[0]");
        if (loc >= 0)
            gl.ProgramUniform4fv(program, loc, 17, registers.vertex.data());
        loc = gl.GetUniformLocation(program, "uP[0]");
        if (loc >= 0)
            gl.ProgramUniform4fv(program, loc, 30, registers.pixel.data());
        gl.UseProgram(program);
        gl.BindVertexArray(vao);
        const auto drawn = triangles == 0xffffffffu ? unsigned(count) : triangles * 3;
        require(width > 0 && height > 0 && first <= unsigned(count) && drawn <= unsigned(count) - first,
                "River diagnostic draw range");
        const civ5::graphics::detail::NativeClipScope clip(context, gl);
        gl.DrawElements(GL_TRIANGLES, drawn, GL_UNSIGNED_INT,
                        reinterpret_cast<const void *>(std::uintptr_t(first) * 4));
        context.check("River diagnostic observation");
    }
};
