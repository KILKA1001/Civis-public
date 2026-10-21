#pragma once

#include "civ5/graphics/gl_render_api.hpp"
#include "civ5/ui/minimap_viewport_ring.hpp"
#include <cmath>
#include <cstddef>
#include <limits>
#include <memory>
#include <span>
#include <stdexcept>
#include <string>
#include <utility>

// Kind-8 viewport-frame pass. The caller owns the target and supplies the
// original camera-derived quads through build_minimap_viewport_ring().
struct GlMinimapViewportRingPass {
    std::shared_ptr<civ5::graphics::GlContext> context;
    civ5::graphics::GlRenderApi gl;
    GLuint program = 0, vertex_array = 0, vertex_buffer = 0, index_buffer = 0;
    GLint scale_location = -1, offset_location = -1, shift_location = -1;

    explicit GlMinimapViewportRingPass(std::shared_ptr<civ5::graphics::GlContext> owner)
        : context(std::move(owner)), gl(*context) {
        const char *vertex_source =
            "#version 460 core\n"
            "layout(location=0) in vec3 position; layout(location=1) in vec4 vertexColor;"
            "uniform vec2 scale; uniform vec2 offset; uniform vec2 clipShift;"
            "out float shade;"
            "void main(){gl_Position=vec4(position.xy*scale-offset+clipShift,0.,1.);"
            "shade=vertexColor.r;}";
        const char *fragment_source =
            "#version 460 core\n"
            "in float shade; out vec4 pixel;"
            "void main(){pixel=vec4(vec3(shade),sqrt(max(shade,0.)));}";
        program = gl.CreateProgram();
        for (const auto &[stage, source] :
             {std::pair{GL_VERTEX_SHADER, vertex_source},
              std::pair{GL_FRAGMENT_SHADER, fragment_source}}) {
            const auto shader = gl.CreateShader(stage);
            gl.ShaderSource(shader, 1, &source, nullptr);
            gl.CompileShader(shader);
            GLint compiled = 0;
            gl.GetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
            if (!compiled) {
                char log[4096]{};
                gl.GetShaderInfoLog(shader, sizeof(log), nullptr, log);
                gl.DeleteShader(shader);
                throw std::runtime_error(std::string("Kind-8 GL shader: ") + log);
            }
            gl.AttachShader(program, shader);
            gl.DeleteShader(shader);
        }
        gl.LinkProgram(program);
        GLint linked = 0;
        gl.GetProgramiv(program, GL_LINK_STATUS, &linked);
        if (!linked) {
            char log[4096]{};
            gl.GetProgramInfoLog(program, sizeof(log), nullptr, log);
            throw std::runtime_error(std::string("Kind-8 GL program: ") + log);
        }
        scale_location = gl.GetUniformLocation(program, "scale");
        offset_location = gl.GetUniformLocation(program, "offset");
        shift_location = gl.GetUniformLocation(program, "clipShift");
        if (scale_location < 0 || offset_location < 0 || shift_location < 0)
            throw std::runtime_error("Kind-8 GL projection uniforms are missing");
        gl.CreateVertexArrays(1, &vertex_array);
        gl.CreateBuffers(1, &vertex_buffer);
        gl.CreateBuffers(1, &index_buffer);
        static_assert(sizeof(civ5::ui::MinimapViewportVertex) == 16);
        gl.VertexArrayVertexBuffer(vertex_array, 0, vertex_buffer, 0,
                                   sizeof(civ5::ui::MinimapViewportVertex));
        gl.VertexArrayElementBuffer(vertex_array, index_buffer);
        gl.EnableVertexArrayAttrib(vertex_array, 0);
        gl.VertexArrayAttribFormat(vertex_array, 0, 3, GL_FLOAT, GL_FALSE,
                                   offsetof(civ5::ui::MinimapViewportVertex, position));
        gl.VertexArrayAttribBinding(vertex_array, 0, 0);
        gl.EnableVertexArrayAttrib(vertex_array, 1);
        gl.VertexArrayAttribFormat(vertex_array, 1, 4, GL_UNSIGNED_BYTE, GL_TRUE,
                                   offsetof(civ5::ui::MinimapViewportVertex, color));
        gl.VertexArrayAttribBinding(vertex_array, 1, 0);
        context->check("Create GL minimap viewport-frame pass");
    }

    ~GlMinimapViewportRingPass() {
        gl.BindVertexArray(0);
        gl.UseProgram(0);
        if (index_buffer) gl.DeleteBuffers(1, &index_buffer);
        if (vertex_buffer) gl.DeleteBuffers(1, &vertex_buffer);
        if (vertex_array) gl.DeleteVertexArrays(1, &vertex_array);
        if (program) gl.DeleteProgram(program);
    }

    void draw(const civ5::ui::MinimapViewportRing &ring,
              civ5::ui::MinimapWorldBounds bounds, float pixel_shift = 0.0f,
              int padding_x = 3, int active_width = 313, int height = 190) {
        if (ring.vertices.empty() || ring.vertices.size() > 36 ||
            ring.vertices.size() % 12 || ring.indices.size() != ring.vertices.size() * 4 ||
            ring.indices.size() > static_cast<std::size_t>(std::numeric_limits<GLsizei>::max()) ||
            !(std::isfinite(bounds.min_x) && std::isfinite(bounds.min_y) &&
              std::isfinite(bounds.max_x) && std::isfinite(bounds.max_y) &&
              bounds.max_x > bounds.min_x && bounds.max_y > bounds.min_y &&
              std::isfinite(pixel_shift) && std::abs(pixel_shift) <= 1.0f) ||
            padding_x < 0 || active_width <= 0 || height <= 0)
            throw std::invalid_argument("Invalid kind-8 GL viewport-frame input");
        for (const auto index : ring.indices)
            if (index >= ring.vertices.size())
                throw std::invalid_argument("Kind-8 GL index is out of range");
        const float scale[2]{2.0f / (bounds.max_x - bounds.min_x),
                             2.0f / (bounds.max_y - bounds.min_y)};
        const float offset[2]{(bounds.min_x + bounds.max_x) * 0.5f * scale[0],
                              (bounds.min_y + bounds.max_y) * 0.5f * scale[1]};
        const float shift[2]{2.0f * pixel_shift / active_width,
                             -2.0f * pixel_shift / height};
        gl.ClipControl(GL_LOWER_LEFT, GL_NEGATIVE_ONE_TO_ONE);
        for (auto flag : {GL_DEPTH_TEST, GL_CULL_FACE, GL_SCISSOR_TEST,
                          GL_FRAMEBUFFER_SRGB, GL_DITHER, GL_MULTISAMPLE})
            gl.Disable(flag);
        gl.Viewport(padding_x, 0, active_width, height);
        gl.Enable(GL_BLEND);
        gl.BlendEquationSeparate(GL_FUNC_ADD, GL_FUNC_ADD);
        gl.BlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE);
        gl.NamedBufferData(vertex_buffer, ring.vertices.size() * sizeof(ring.vertices[0]),
                           ring.vertices.data(), GL_STREAM_DRAW);
        gl.NamedBufferData(index_buffer, ring.indices.size() * sizeof(ring.indices[0]),
                           ring.indices.data(), GL_STREAM_DRAW);
        gl.ProgramUniform2fv(program, scale_location, 1, scale);
        gl.ProgramUniform2fv(program, offset_location, 1, offset);
        gl.ProgramUniform2fv(program, shift_location, 1, shift);
        gl.UseProgram(program);
        gl.BindVertexArray(vertex_array);
        gl.DrawElements(GL_TRIANGLES, static_cast<GLsizei>(ring.indices.size()),
                        GL_UNSIGNED_SHORT, nullptr);
        gl.BindVertexArray(0);
        gl.UseProgram(0);
        context->check("Draw GL minimap viewport frame");
    }
};
