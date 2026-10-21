#pragma once

#include "civ5/graphics/gl_render_api.hpp"
#include "civ5/ui/minimap_notification_pulse.hpp"
#include <array>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <memory>
#include <span>
#include <stdexcept>
#include <string>
#include <utility>

// A presentation sink for the owned minimap notification queue. The host binds
// its 320x190 minimap render target before draw(). The BNW SDK confirms the
// event ID and map coordinate but does not expose the native dot's shader,
// color, radius or timing; callers must supply those visual parameters.
struct GlMinimapNotificationPulseStyle {
    float radius_pixels = 0;
    std::array<float, 4> rgba{};
    std::chrono::milliseconds flash_period{0};
};

struct GlMinimapNotificationPulsePass {
    std::shared_ptr<civ5::graphics::GlContext> context;
    civ5::graphics::GlRenderApi gl;
    GLuint program = 0, vertex_array = 0;
    GLint sprite_location = -1, color_location = -1;

    explicit GlMinimapNotificationPulsePass(
        std::shared_ptr<civ5::graphics::GlContext> owner)
        : context(std::move(owner)), gl(*context) {
        const char *vertex_source =
            "#version 460 core\n"
            "uniform vec4 sprite; out vec2 radial;"
            "void main(){const vec2 q[6]=vec2[6](vec2(-1.,-1.),vec2(1.,-1.),"
            "vec2(1.,1.),vec2(-1.,-1.),vec2(1.,1.),vec2(-1.,1.));"
            "radial=q[gl_VertexID]; vec2 p=sprite.xy+radial*sprite.z;"
            "gl_Position=vec4(p.x/160.-1.,1.-p.y/95.,0.,1.);}";
        const char *fragment_source =
            "#version 460 core\n"
            "in vec2 radial; uniform vec4 dotColor; out vec4 pixel;"
            "void main(){float edge=1.-smoothstep(.72,1.,length(radial));"
            "pixel=vec4(dotColor.rgb,dotColor.a*edge);}";
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
                throw std::runtime_error(std::string("Minimap notification shader: ") + log);
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
            throw std::runtime_error(std::string("Minimap notification program: ") + log);
        }
        sprite_location = gl.GetUniformLocation(program, "sprite");
        color_location = gl.GetUniformLocation(program, "dotColor");
        if (sprite_location < 0 || color_location < 0)
            throw std::runtime_error("Minimap notification uniforms are missing");
        gl.CreateVertexArrays(1, &vertex_array);
        context->check("Create GL minimap notification pass");
    }

    ~GlMinimapNotificationPulsePass() {
        gl.BindVertexArray(0);
        gl.UseProgram(0);
        if (vertex_array) gl.DeleteVertexArrays(1, &vertex_array);
        if (program) gl.DeleteProgram(program);
    }

    std::size_t draw(std::span<const civ5::ui::MinimapNotificationPulse> pulses,
                     std::chrono::steady_clock::time_point now,
                     const GlMinimapNotificationPulseStyle &style) {
        if (!std::isfinite(style.radius_pixels) || style.radius_pixels <= 0 ||
            style.radius_pixels > 32 || style.flash_period.count() <= 0)
            throw std::invalid_argument("Minimap notification style is invalid");
        for (float channel : style.rgba)
            if (!std::isfinite(channel) || channel < 0 || channel > 1)
                throw std::invalid_argument("Minimap notification color is invalid");
        if (style.rgba[3] == 0)
            throw std::invalid_argument("Minimap notification opacity is zero");
        GLint bound_framebuffer = 0;
        context->api().GetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &bound_framebuffer);
        if (bound_framebuffer == 0)
            throw std::invalid_argument("Minimap notification target framebuffer is not bound");
        gl.ClipControl(GL_LOWER_LEFT, GL_NEGATIVE_ONE_TO_ONE);
        for (auto flag : {GL_DEPTH_TEST, GL_CULL_FACE, GL_SCISSOR_TEST,
                          GL_FRAMEBUFFER_SRGB, GL_DITHER, GL_MULTISAMPLE})
            gl.Disable(flag);
        gl.Viewport(0, 0, 320, 190);
        gl.Enable(GL_BLEND);
        gl.BlendEquationSeparate(GL_FUNC_ADD, GL_FUNC_ADD);
        gl.BlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE);
        gl.UseProgram(program);
        gl.BindVertexArray(vertex_array);
        std::size_t drawn = 0;
        for (const auto &pulse : pulses) {
            if (!pulse.inside_view || now < pulse.started_at || now >= pulse.expires_at)
                continue;
            const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                now - pulse.started_at).count();
            const auto phase = static_cast<float>(elapsed % style.flash_period.count()) /
                               static_cast<float>(style.flash_period.count());
            const float opacity = 0.5f + 0.5f *
                std::cos(phase * 6.2831853071795864769f);
            const float sprite[4]{pulse.texture_pixel.x, pulse.texture_pixel.y,
                                  style.radius_pixels, 0};
            const float color[4]{style.rgba[0], style.rgba[1], style.rgba[2],
                                 style.rgba[3] * opacity};
            gl.ProgramUniform4fv(program, sprite_location, 1, sprite);
            gl.ProgramUniform4fv(program, color_location, 1, color);
            gl.DrawArrays(GL_TRIANGLES, 0, 6);
            ++drawn;
        }
        gl.BindVertexArray(0);
        gl.UseProgram(0);
        context->check("Draw GL minimap notification pulses");
        return drawn;
    }
};
