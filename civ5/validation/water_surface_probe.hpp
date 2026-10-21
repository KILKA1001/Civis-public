#pragma once
#include "../src/graphics/gl_native_clip.hpp"
// Instrumented programs are validation-only. They cannot replace the full draw
// oracle.
inline std::string water_arithmetic_diagnostic_source(unsigned selector, unsigned profile) {
    if (profile == 3) {
        std::string source = water_arithmetic_diagnostic_source(selector, 0);
        source.insert(source.find('\n') + 1, material_sampling_glsl());
        for (unsigned slot : {0u, 1u}) {
            const auto token = "texture(s" + std::to_string(slot) + ",";
            const auto replacement = "nativeMaterialImplicit(s" + std::to_string(slot) + ",";
            for (auto at = source.find(token); at != std::string::npos;
                 at = source.find(token, at + replacement.size()))
                source.replace(at, token.size(), replacement);
        }
        return source;
    }
    std::string source(water_surface_fragment_shader(selector));
    const auto replace = [&](const std::string &token, const std::string &replacement) {
        auto at = source.find(token);
        require(at != std::string::npos, "Arithmetic probe token");
        source.replace(at, token.size(), replacement);
    };
    if (profile == 0 || profile == 1) {
        replace(
            "vec4 sm3Mad(vec4 a,vec4 b,vec4 c){precise dvec4 v=dvec4(a)*dvec4(b)+dvec4(c);return vec4(v);}",
            profile == 0 ? "vec4 sm3Mad(vec4 a,vec4 b,vec4 c){return a*b+c;}"
                         : "vec4 sm3Mad(vec4 a,vec4 b,vec4 c){return fma(a,b,c);}");
        replace("vec4 sm3Dp3(vec4 a,vec4 b){precise float "
                "v=a.x*b.x;v=float(double(a.y)*double(b.y)+double(v));v=float(double(a.z)*double(b.z)+double("
                "v));return vec4(v);}",
                "vec4 sm3Dp3(vec4 a,vec4 b){return vec4(dot(a.xyz,b.xyz));}");
        replace("vec4 sm3Dp2Add(vec4 a,vec4 b,vec4 c){precise float "
                "v=a.x*b.x;v=float(double(a.y)*double(b.y)+double(v));v=v+c.x;return vec4(v);}",
                "vec4 sm3Dp2Add(vec4 a,vec4 b,vec4 c){return vec4(dot(a.xy,b.xy)+c.x);}");
        replace("vec4 sm3Dp4(vec4 a,vec4 b){precise float "
                "v=a.x*b.x;v=float(double(a.y)*double(b.y)+double(v));v=float(double(a.z)*double(b.z)+double("
                "v));v=float(double(a.w)*double(b.w)+double(v));return vec4(v);}",
                "vec4 sm3Dp4(vec4 a,vec4 b){return vec4(dot(a,b));}");
        replace("vec4 sm3Lrp(vec4 a,vec4 b,vec4 c){precise vec4 x=b-c;return sm3Mad(a,x,c);}",
                "vec4 sm3Lrp(vec4 a,vec4 b,vec4 c){return mix(c,b,a);}");
    }
    if (profile == 0 || profile == 2) {
        for (auto at = source.find("precise "); at != std::string::npos; at = source.find("precise "))
            source.erase(at, 8);
    }
    return source;
}
inline std::string water_mip_diagnostic_source(unsigned selector) {
    std::string source(water_surface_fragment_shader(selector));
    source.insert(source.find('\n') + 1, material_sampling_glsl());
    for (unsigned slot : {0u, 1u, 4u, 5u, 7u, 8u, 9u, 10u, 13u}) {
        const auto token = "texture(s" + std::to_string(slot) + ",";
        const auto replacement = "nativeMaterialImplicit(s" + std::to_string(slot) + ",";
        for (auto at = source.find(token); at != std::string::npos;
             at = source.find(token, at + replacement.size()))
            source.replace(at, token.size(), replacement);
    }
    return source;
}
struct WaterProbe {
    GlContext &context;
    GlRenderApi gl;
    GLuint program = 0, vao = 0, vb = 0, ib = 0;
    GLsizei count;
    WaterProbe(GlContext &c, const WaterSurfaceMesh &mesh, std::string_view fragment)
        : context(c), gl(c), count(static_cast<GLsizei>(mesh.indices.size())) {
        try {
            program = gl.CreateProgram();
            for (auto [stage, source] : {std::pair{GL_VERTEX_SHADER, water_surface_vertex_shader()},
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
            gl.NamedBufferData(vb, mesh.vertices.size() * 16, mesh.vertices.data(), GL_STATIC_DRAW);
            gl.NamedBufferData(ib, mesh.indices.size() * 2, mesh.indices.data(), GL_STATIC_DRAW);
            gl.VertexArrayVertexBuffer(vao, 0, vb, 0, 16);
            gl.VertexArrayElementBuffer(vao, ib);
            gl.EnableVertexArrayAttrib(vao, 0);
            gl.VertexArrayAttribBinding(vao, 0, 0);
            gl.VertexArrayAttribFormat(vao, 0, 4, GL_FLOAT, GL_FALSE, 0);
            context.check("Water diagnostic program");
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
    ~WaterProbe() { release(); }
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
                "Water diagnostic draw range");
        const civ5::graphics::detail::NativeClipScope clip(context, gl);
        gl.DrawElements(GL_TRIANGLES, drawn, GL_UNSIGNED_SHORT,
                        reinterpret_cast<const void *>(std::uintptr_t(first) * 2));
        context.check("Water diagnostic observation");
    }
};
