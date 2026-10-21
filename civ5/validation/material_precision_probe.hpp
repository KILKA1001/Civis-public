#pragma once
// Validation only. The preceding production draw supplies its checked geometry,
// textures and samplers; only the fragment program is instrumented here.
struct MaterialPrecisionProbe {
    civ5::graphics::GlContext &context;
    civ5::graphics::GlRenderApi gl;
    GLuint program = 0;
    MaterialPrecisionProbe(civ5::graphics::GlContext &c, std::string_view fragment)
        : context(c), gl(c) {
        try {
            program = gl.CreateProgram();
            const auto vertex = civ5::graphics::material_blend_cache_vertex_shader();
            for (const auto &[stage, source] :
                 {std::pair{GL_VERTEX_SHADER, std::string_view(vertex)}, {GL_FRAGMENT_SHADER, fragment}}) {
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
            context.check("Material precision probe initialization");
        } catch (...) {
            gl.DeleteProgram(program);
            throw;
        }
    }
    ~MaterialPrecisionProbe() {
        gl.UseProgram(0);
        gl.DeleteProgram(program);
    }
    void draw(std::span<const float, 384> coefficients, std::array<float, 2> offset,
              unsigned size, GLsizei count, const std::array<GLuint, 16> &textures) {
        auto location = gl.GetUniformLocation(program, "uCoefficients[0]");
        if (location >= 0)
            gl.ProgramUniform4fv(program, location, 96, coefficients.data());
        location = gl.GetUniformLocation(program, "uTexelOffsets");
        if (location >= 0)
            gl.ProgramUniform2fv(program, location, 1, offset.data());
        location = gl.GetUniformLocation(program, "uPixelCenter");
        const std::array<float, 2> center{1.f / size, 1.f / size};
        if (location >= 0)
            gl.ProgramUniform2fv(program, location, 1, center.data());
        location = gl.GetUniformLocation(program, "uHardwareMipMask");
        if (location >= 0) {
            GLint mask = 0;
            for (unsigned slot = 0; slot < 16; ++slot) {
                GLint base = 0, format = 0;
                context.api().GetTextureParameteriv(textures[slot], GL_TEXTURE_BASE_LEVEL, &base);
                context.api().GetTextureLevelParameteriv(textures[slot], base, GL_TEXTURE_INTERNAL_FORMAT, &format);
                if (format == 0x83f3)
                    mask |= 1 << slot;
            }
            gl.ProgramUniform1i(program, location, mask);
        }
        gl.UseProgram(program);
        gl.DrawElements(GL_TRIANGLES, count, GL_UNSIGNED_SHORT, nullptr);
        context.check("Material precision probe draw");
    }
};
