#include "civ5/assets/dds.hpp"
#include "civ5/graphics/gl_render_api.hpp"
#include "civ5/graphics/gl_texture.hpp"
#include "civ5/ui/minimap_projection.hpp"
#include "civ5/ui/minimap_ice_fixed_raster.hpp"
#include "civ5/ui/minimap_ice_probe_mesh.hpp"
#include "civ5/ui/minimap_native_ice_records.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
std::vector<std::uint8_t> read(const std::filesystem::path &path) {
    std::ifstream stream(path, std::ios::binary | std::ios::ate);
    if (!stream) throw std::runtime_error("Ice DDS input is missing");
    const auto size = stream.tellg();
    if (size != 128 + 128 * 128 * 4)
        throw std::runtime_error("Original Ice DDS length changed");
    std::vector<std::uint8_t> bytes(static_cast<std::size_t>(size));
    stream.seekg(0);
    stream.read(reinterpret_cast<char *>(bytes.data()), size);
    if (!stream) throw std::runtime_error("Ice DDS input is truncated");
    return bytes;
}

void save_ppm(const civ5::graphics::GlTexture &texture,
              const std::filesystem::path &path) {
    const auto raw = texture.read_mip_raw(0);
    if (raw.size() != 320 * 190 * 4)
        throw std::runtime_error("Ice GPU readback extent changed");
    std::ofstream image(path, std::ios::binary);
    image << "P6\n320 190\n255\n";
    for (int y = 189; y >= 0; --y)
        for (int x = 0; x < 320; ++x) {
            const auto at = (static_cast<std::size_t>(y) * 320 + x) * 4;
            const char rgb[3]{static_cast<char>(raw[at + 2]),
                              static_cast<char>(raw[at + 1]),
                              static_cast<char>(raw[at])};
            image.write(rgb, sizeof(rgb));
        }
    if (!image) throw std::runtime_error("Ice GPU PPM write failed");
}

void checked_shader(civ5::graphics::GlRenderApi &gl, GLuint shader) {
    GLint status = 0;
    gl.GetShaderiv(shader, GL_COMPILE_STATUS, &status);
    if (!status) throw std::runtime_error("Minimap ice GL shader compile failed");
}

std::vector<civ5::ui::MinimapPoint2> read_ice_pixels(const std::filesystem::path &path) {
    std::ifstream stream(path, std::ios::binary | std::ios::ate);
    if (!stream) throw std::runtime_error("Ice fixed-edge pixel list is missing");
    const auto size = stream.tellg();
    if (size < 12 || size > 12 + 320 * 190 * 8)
        throw std::runtime_error("Invalid ice fixed-edge pixel list length");
    stream.seekg(0);
    std::array<char, 12> header{};
    stream.read(header.data(), header.size());
    std::uint32_t count = 0;
    std::memcpy(&count, header.data() + 8, sizeof(count));
    if (!stream || std::memcmp(header.data(), "C5IPXL01", 8) ||
        !count || count > 320 * 190 || size != 12 + 8ull * count)
        throw std::runtime_error("Invalid ice fixed-edge pixel list header");
    std::vector<civ5::ui::MinimapPoint2> points(count);
    stream.read(reinterpret_cast<char *>(points.data()), 8ull * count);
    if (!stream) throw std::runtime_error("Truncated ice fixed-edge pixel list");
    for (const auto &point : points)
        if (!std::isfinite(point.x) || !std::isfinite(point.y) ||
            point.x < 3 || point.x > 315 || point.y < 0 || point.y > 189 ||
            point.x != std::floor(point.x) || point.y != std::floor(point.y))
            throw std::runtime_error("Invalid ice fixed-edge pixel coordinate");
    return points;
}
} // namespace

int main(int argc, char **argv) {
    try {
        const bool ice_fixed = (argc == 11 || argc == 12) &&
                               std::string(argv[1]) == "--ice-mesh-fixed";
        const bool ice_mesh = ((argc == 10 || argc == 11 || argc == 12) &&
                               std::string(argv[1]) == "--ice-mesh") || ice_fixed;
        const bool ice_pixels = argc == 10 && std::string(argv[1]) == "--ice-pixels";
        const bool ice_record_fixed = (argc == 11 || argc == 12) &&
                                      std::string(argv[1]) == "--ice-record-fixed";
        const bool ice_record = (argc == 11 || argc == 12) &&
                                (std::string(argv[1]) == "--ice-record" ||
                                 std::string(argv[1]) == "--ice-record-float") ||
                                ice_record_fixed;
        const bool fixed_mode = ice_fixed || ice_record_fixed;
        const bool indexed_float = ice_record && std::string(argv[1]) == "--ice-record-float";
        if (argc != 12 && !ice_mesh && !ice_record && !ice_pixels)
            throw std::invalid_argument("Usage: civ5_gl_minimap_ice [--ice-mesh|--ice-mesh-fixed|--ice-record|--ice-record-float|--ice-record-fixed|--ice-pixels] input-folder output.ppm viewMinX viewMinY viewMaxX viewMaxY [iceMinX iceMinY iceMaxX iceMaxY|mesh.bin|records.bin region|pixels.bin] pixelShift [iceOffsetX|fixedPixels.bin [iceOffsetX]|--snap-grid N|--separate-mad|--dx9-point-cull]");
        const int base = ice_mesh || ice_record || ice_pixels ? 1 : 0;
        const std::filesystem::path input(argv[1 + base]), output(argv[2 + base]);
        const std::array<float, 4> view{std::stof(argv[3 + base]), std::stof(argv[4 + base]),
                                        std::stof(argv[5 + base]), std::stof(argv[6 + base])};
        const auto probe = ice_mesh ? std::optional(
            civ5::ui::read_minimap_ice_probe_mesh(argv[8])) : std::nullopt;
        std::vector<civ5::ui::MinimapPoint2> pixel_points;
        const auto records = ice_record ? std::optional(
            civ5::ui::read_minimap_native_ice_records(argv[8])) : std::nullopt;
        const int region_index = ice_record ? std::stoi(argv[9]) : 0;
        if (records && (region_index < 0 ||
                        static_cast<std::size_t>(region_index) >= records->regions.size()))
            throw std::invalid_argument("Ice record region index is outside the stream");
        const auto *record = records ? &records->regions[region_index] : nullptr;
        const std::array<float, 4> box = ice_pixels ? view : probe ? probe->box : record ?
            records->shader_box.value_or(record->box) :
            std::array<float, 4>{std::stof(argv[7]), std::stof(argv[8]),
                                 std::stof(argv[9]), std::stof(argv[10])};
        const float pixel_shift = std::stof(argv[ice_record ? 10 :
                                                  ice_mesh || ice_pixels ? 9 : 11]);
        const bool snap_trial = ice_mesh && !ice_fixed && argc == 12 &&
                                std::string(argv[10]) == "--snap-grid";
        const bool separate_mad_trial = ice_mesh && !ice_fixed && argc == 11 &&
                                        std::string(argv[10]) == "--separate-mad";
        const bool point_cull_trial = ice_mesh && !ice_fixed && argc == 11 &&
                                      std::string(argv[10]) == "--dx9-point-cull";
        if (ice_mesh && !ice_fixed && argc == 12 && !snap_trial)
            throw std::invalid_argument("Unknown ice mesh raster trial");
        const float snap_grid = snap_trial ? std::stof(argv[11]) : 0.0f;
        const float ice_offset_x = ice_fixed && argc == 12 ? std::stof(argv[11]) :
                                   ice_record && argc == 12 ? std::stof(argv[11]) :
                                   ice_mesh && argc == 11 && !separate_mad_trial &&
                                   !point_cull_trial && !ice_fixed ?
                                       std::stof(argv[10]) : 0.0f;
        if (!(view[2] > view[0] && view[3] > view[1] &&
              box[2] > box[0] && box[3] > box[1] &&
              std::isfinite(pixel_shift) && pixel_shift >= 0 && pixel_shift <= 1 &&
              std::isfinite(ice_offset_x) && std::isfinite(snap_grid) &&
              snap_grid >= 0.0f && snap_grid <= 4096.0f))
            throw std::invalid_argument("Invalid minimap ice GPU bounds or shift");
        if (ice_record_fixed)
            pixel_points = civ5::ui::raster_minimap_ice_dx9_pixels(
                *record, box, view, ice_offset_x);
        else if (ice_pixels || ice_fixed)
            pixel_points = read_ice_pixels(argv[ice_fixed ? 10 : 8]);
        auto source = civ5::assets::DdsData::parse(read(input / "minimapice.dds"));
        if (source.width() != 128 || source.height() != 128 || source.mip_count() != 1 ||
            source.bits_per_pixel() != 32 || source.face_count() != 1)
            throw std::runtime_error("Original Ice DDS layout changed");

        auto context = std::make_shared<civ5::graphics::GlContext>();
        civ5::graphics::GlRenderApi gl(*context);
        auto target = civ5::graphics::GlTexture::render_target(
            *context, 320, 190, 1, civ5::graphics::RenderTextureFormat::rgba8);
        auto stencil = fixed_mode ? std::make_unique<civ5::graphics::GlTexture>(
            civ5::graphics::GlTexture::render_target(
                *context, 320, 190, 1,
                civ5::graphics::RenderTextureFormat::depth24_stencil8)) : nullptr;
        auto ice = civ5::graphics::GlTexture(
            *context, source, civ5::graphics::DdsSamplingColor::srgb);
        GLuint framebuffer = 0;
        gl.CreateFramebuffers(1, &framebuffer);
        gl.NamedFramebufferTexture(framebuffer, GL_COLOR_ATTACHMENT0, target.id(), 0);
        if (stencil)
            gl.NamedFramebufferTexture(framebuffer, GL_DEPTH_STENCIL_ATTACHMENT,
                                       stencil->id(), 0);
        const GLenum attachment = GL_COLOR_ATTACHMENT0;
        gl.NamedFramebufferDrawBuffers(framebuffer, 1, &attachment);
        if (gl.CheckNamedFramebufferStatus(framebuffer, GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
            throw std::runtime_error("Ice diagnostic framebuffer is incomplete");
        const char *vs_source =
            "#version 460 core\n"
            "layout(location=0) in vec2 normalized; uniform vec4 view_scale_offset,ice_box,view_bounds;"
            "uniform float ice_offset_x;"
            "uniform vec2 clip_shift; uniform float snap_grid; uniform bool separate_mad,pixel_mode; out vec2 world;"
            "void main(){if(pixel_mode){"
            "world=vec2(view_bounds.x+(normalized.x-3.)*(view_bounds.z-view_bounds.x)/313.,"
            "view_bounds.w-normalized.y*(view_bounds.w-view_bounds.y)/190.);"
            "vec2 point_clip=vec2((normalized.x-3.+.5)/156.5-1.,1.-(normalized.y+.5)/95.);"
            "gl_Position=vec4(point_clip,0.,1.);gl_PointSize=1.;return;}"
            "vec2 unit=fma(normalized,vec2(0.5),vec2(0.5));"
            "world=fma(unit,ice_box.zw-ice_box.xy,ice_box.xy);"
            "world.x+=ice_offset_x;"
            "precise vec2 product=world*view_scale_offset.xy;"
            "vec2 clip=separate_mad?product-view_scale_offset.zw:"
            "fma(world,view_scale_offset.xy,-view_scale_offset.zw);"
            "if(snap_grid>0.){vec2 screen=vec2(3.+(clip.x+1.)*156.5,(1.-clip.y)*95.);"
            "screen=floor(screen*snap_grid+vec2(.5))/snap_grid;"
            "clip=vec2((screen.x-3.)/156.5-1.,1.-screen.y/95.);}"
            "gl_Position=vec4(clip+clip_shift,0.,1.);}";
        const char *ps_source =
            "#version 460 core\n"
            "layout(binding=0) uniform sampler2D source; uniform vec2 tiling;"
            "in vec2 world; out vec4 color;"
            "void main(){vec2 uv=world*tiling.x+tiling.y;"
            "vec3 texel=texture(source,uv).rgb;"
            "color=vec4(pow(max(texel,vec3(0.)),vec3(0.454545468)),1.);}";
        const auto vs = gl.CreateShader(GL_VERTEX_SHADER), ps = gl.CreateShader(GL_FRAGMENT_SHADER);
        gl.ShaderSource(vs, 1, &vs_source, nullptr);
        gl.ShaderSource(ps, 1, &ps_source, nullptr);
        gl.CompileShader(vs);
        gl.CompileShader(ps);
        checked_shader(gl, vs);
        checked_shader(gl, ps);
        const auto program = gl.CreateProgram();
        gl.AttachShader(program, vs);
        gl.AttachShader(program, ps);
        gl.LinkProgram(program);
        gl.DeleteShader(vs);
        gl.DeleteShader(ps);
        GLint linked = 0;
        gl.GetProgramiv(program, GL_LINK_STATUS, &linked);
        if (!linked) throw std::runtime_error("Minimap ice GL program link failed");
        const auto view_location = gl.GetUniformLocation(program, "view_scale_offset");
        const auto view_bounds_location = gl.GetUniformLocation(program, "view_bounds");
        const auto pixel_mode_location = gl.GetUniformLocation(program, "pixel_mode");
        const auto box_location = gl.GetUniformLocation(program, "ice_box");
        const auto shift_location = gl.GetUniformLocation(program, "clip_shift");
        const auto snap_location = gl.GetUniformLocation(program, "snap_grid");
        const auto separate_mad_location = gl.GetUniformLocation(program, "separate_mad");
        const auto ice_offset_location = gl.GetUniformLocation(program, "ice_offset_x");
        const auto tiling_location = gl.GetUniformLocation(program, "tiling");
        if (view_location < 0 || view_bounds_location < 0 || pixel_mode_location < 0 ||
            box_location < 0 || shift_location < 0 ||
            snap_location < 0 || separate_mad_location < 0 ||
            ice_offset_location < 0 ||
            tiling_location < 0)
            throw std::runtime_error("Minimap ice GL uniforms are incomplete");
        GLuint array = 0, buffer = 0, index_buffer = 0, sampler = 0;
        GLuint pixel_array = 0, pixel_buffer = 0;
        gl.CreateVertexArrays(1, &array);
        gl.CreateBuffers(1, &buffer);
        constexpr std::array<float, 8> quad{-1, -1, 1, -1, -1, 1, 1, 1};
        std::vector<civ5::ui::MinimapPoint2> decoded;
        std::vector<civ5::ui::MinimapPoint2> point_culled;
        if (point_cull_trial) {
            point_culled.reserve(probe->normalized_triangles.size());
            const auto fixed_point = [&](const civ5::ui::MinimapPoint2 &position) {
                const double world_x = box[0] + (double(position.x) * 0.5 + 0.5) *
                    (double(box[2]) - box[0]);
                const double world_y = box[1] + (double(position.y) * 0.5 + 0.5) *
                    (double(box[3]) - box[1]);
                const double x = 3.0 + (world_x - view[0]) * 313.0 /
                    (double(view[2]) - view[0]);
                const double y = (view[3] - world_y) * 190.0 /
                    (double(view[3]) - view[1]);
                return std::array<std::int64_t, 2>{
                    static_cast<std::int64_t>(std::floor(x * 16.0 + 0.5)),
                    static_cast<std::int64_t>(std::floor(y * 16.0 + 0.5))};
            };
            for (std::size_t i = 0; i < probe->normalized_triangles.size(); i += 3) {
                const auto &vertices = probe->normalized_triangles;
                const auto a = fixed_point(vertices[i]);
                const auto b = fixed_point(vertices[i + 1]);
                const auto c = fixed_point(vertices[i + 2]);
                if (a == b && b == c) continue;
                point_culled.insert(point_culled.end(), vertices.begin() + i,
                                    vertices.begin() + i + 3);
            }
        }
        if (record && indexed_float) {
            decoded.reserve(record->vertices.size());
            for (const auto &vertex : record->vertices)
                decoded.push_back({std::max(-1.0f, float(vertex[0]) / 32767.0f),
                                   std::max(-1.0f, float(vertex[1]) / 32767.0f)});
        }
        const auto *positions = record && !indexed_float
            ? static_cast<const void *>(record->vertices.data())
            : record ? static_cast<const void *>(decoded.data())
            : probe ? static_cast<const void *>((point_cull_trial ? point_culled :
                                                  probe->normalized_triangles).data())
            : ice_pixels ? static_cast<const void *>(pixel_points.data())
            : static_cast<const void *>(quad.data());
        const auto position_bytes = record && !indexed_float
            ? record->vertices.size() * sizeof(record->vertices[0])
            : record ? decoded.size() * sizeof(decoded[0])
            : probe ? (point_cull_trial ? point_culled.size() :
                       probe->normalized_triangles.size()) * sizeof(civ5::ui::MinimapPoint2)
            : ice_pixels ? pixel_points.size() * sizeof(civ5::ui::MinimapPoint2)
            : sizeof(quad);
        gl.NamedBufferData(buffer, position_bytes, positions, GL_STATIC_DRAW);
        gl.VertexArrayVertexBuffer(array, 0, buffer, 0, 2 * sizeof(float));
        gl.EnableVertexArrayAttrib(array, 0);
        gl.VertexArrayAttribFormat(array, 0, 2,
                                   record && !indexed_float ? GL_SHORT : GL_FLOAT,
                                   record && !indexed_float ? GL_TRUE : GL_FALSE, 0);
        gl.VertexArrayAttribBinding(array, 0, 0);
        if (fixed_mode) {
            gl.CreateVertexArrays(1, &pixel_array);
            gl.CreateBuffers(1, &pixel_buffer);
            gl.NamedBufferData(pixel_buffer,
                               pixel_points.size() * sizeof(civ5::ui::MinimapPoint2),
                               pixel_points.data(), GL_STATIC_DRAW);
            gl.VertexArrayVertexBuffer(pixel_array, 0, pixel_buffer, 0,
                                       sizeof(civ5::ui::MinimapPoint2));
            gl.EnableVertexArrayAttrib(pixel_array, 0);
            gl.VertexArrayAttribFormat(pixel_array, 0, 2, GL_FLOAT, GL_FALSE, 0);
            gl.VertexArrayAttribBinding(pixel_array, 0, 0);
        }
        if (record) {
            gl.CreateBuffers(1, &index_buffer);
            gl.NamedBufferData(index_buffer, record->indices.size() * sizeof(record->indices[0]),
                               record->indices.data(), GL_STATIC_DRAW);
            gl.VertexArrayElementBuffer(array, index_buffer);
        }
        gl.CreateSamplers(1, &sampler);
        gl.SamplerParameteri(sampler, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        gl.SamplerParameteri(sampler, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        gl.SamplerParameteri(sampler, GL_TEXTURE_WRAP_S, GL_REPEAT);
        gl.SamplerParameteri(sampler, GL_TEXTURE_WRAP_T, GL_REPEAT);
        gl.BindFramebuffer(GL_FRAMEBUFFER, framebuffer);
        gl.ClipControl(GL_LOWER_LEFT, GL_NEGATIVE_ONE_TO_ONE);
        for (auto flag : {GL_DEPTH_TEST, GL_CULL_FACE, GL_BLEND, GL_SCISSOR_TEST,
                          GL_FRAMEBUFFER_SRGB, GL_DITHER})
            gl.Disable(flag);
        if (ice_pixels || fixed_mode) gl.Enable(GL_PROGRAM_POINT_SIZE);
        const float black[4]{0, 0, 0, 1};
        gl.ClearNamedFramebufferfv(framebuffer, GL_COLOR, 0, black);
        gl.Viewport(3, 0, 313, 190);
        gl.UseProgram(program);
        const float sx = 2.0f / (view[2] - view[0]);
        const float sy = 2.0f / (view[3] - view[1]);
        const std::array<float, 4> view_scale_offset{
            sx, sy, (view[0] + view[2]) * 0.5f * sx,
            (view[1] + view[3]) * 0.5f * sy};
        gl.ProgramUniform4fv(program, view_location, 1, view_scale_offset.data());
        gl.ProgramUniform4fv(program, view_bounds_location, 1, view.data());
        gl.ProgramUniform1i(program, pixel_mode_location, ice_pixels);
        gl.ProgramUniform4fv(program, box_location, 1, box.data());
        gl.ProgramUniform1f(program, ice_offset_location, ice_offset_x);
        const std::array<float, 2> shift{2 * pixel_shift / 313, -2 * pixel_shift / 190};
        gl.ProgramUniform2fv(program, shift_location, 1, shift.data());
        gl.ProgramUniform1f(program, snap_location, snap_grid);
        gl.ProgramUniform1i(program, separate_mad_location, separate_mad_trial);
        const std::array<float, 2> tiling{
            civ5::ui::minimap_ice_texture_tiling.x,
            civ5::ui::minimap_ice_texture_tiling.y};
        gl.ProgramUniform2fv(program, tiling_location, 1, tiling.data());
        gl.BindVertexArray(array);
        gl.BindSampler(0, sampler);
        ice.bind(0);
        if (fixed_mode) {
            gl.ClearStencil(0);
            gl.StencilMask(0xff);
            gl.Clear(GL_STENCIL_BUFFER_BIT);
            gl.Enable(GL_STENCIL_TEST);
            gl.StencilFunc(GL_ALWAYS, 1, 0xff);
            gl.StencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
            gl.ColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
            gl.ProgramUniform1i(program, pixel_mode_location, 1);
            gl.BindVertexArray(pixel_array);
            gl.DrawArrays(GL_POINTS, 0, static_cast<GLsizei>(pixel_points.size()));
            gl.ColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
            gl.ProgramUniform1i(program, pixel_mode_location, 0);
            gl.BindVertexArray(array);
            gl.StencilFunc(GL_LEQUAL, 1, 0xff);
            gl.StencilOp(GL_KEEP, GL_KEEP, GL_INCR);
        }
        if (record)
            gl.DrawElements(GL_TRIANGLES, static_cast<GLsizei>(record->indices.size()),
                            GL_UNSIGNED_SHORT, nullptr);
        else
            gl.DrawArrays(ice_pixels ? GL_POINTS : probe ? GL_TRIANGLES : GL_TRIANGLE_STRIP, 0,
                          ice_pixels ? static_cast<GLsizei>(pixel_points.size()) :
                          probe ? static_cast<GLsizei>(point_cull_trial ? point_culled.size() :
                                                      probe->normalized_triangles.size()) : 4);
        if (fixed_mode) {
            gl.StencilFunc(GL_EQUAL, 1, 0xff);
            gl.StencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
            gl.ProgramUniform1i(program, pixel_mode_location, 1);
            gl.BindVertexArray(pixel_array);
            gl.DrawArrays(GL_POINTS, 0, static_cast<GLsizei>(pixel_points.size()));
            gl.Disable(GL_STENCIL_TEST);
        }
        gl.BindFramebuffer(GL_FRAMEBUFFER, 0);
        save_ppm(target, output);
        gl.DeleteSamplers(1, &sampler);
        if (pixel_buffer) gl.DeleteBuffers(1, &pixel_buffer);
        if (pixel_array) gl.DeleteVertexArrays(1, &pixel_array);
        if (index_buffer) gl.DeleteBuffers(1, &index_buffer);
        gl.DeleteBuffers(1, &buffer);
        gl.DeleteVertexArrays(1, &array);
        gl.DeleteProgram(program);
        gl.DeleteFramebuffers(1, &framebuffer);
        context->check("Kind-9 GL ice control");
        std::cout << "GL minimap kind-9 ice control: 320x190 shift=" << pixel_shift
                  << " snapGrid=" << snap_grid
                  << " iceOffsetX=" << ice_offset_x
                  << " vertices=" << (record ? record->vertices.size() :
                                      probe ? (point_cull_trial ? point_culled.size() :
                                               probe->normalized_triangles.size()) : 4)
                  << " indices=" << (record ? record->indices.size() : 0) << '\n';
        return 0;
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
