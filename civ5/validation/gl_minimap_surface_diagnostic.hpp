#pragma once

#include "civ5/graphics/gl_render_api.hpp"
#include "civ5/graphics/gl_texture.hpp"
#include "civ5/ui/minimap_mountain_markers.hpp"
#include "civ5/ui/minimap_projection.hpp"
#include "civ5/ui/minimap_saved_ice_plots.hpp"
#include "civ5/ui/minimap_ice_fixed_raster.hpp"
#include "civ5/ui/minimap_ice_regions.hpp"
#include "civ5/ui/minimap_native_ice_records.hpp"
#include "civ5/ui/minimap_render_meshes.hpp"
#include "civ5/ui/bnw_minimap_fog_preparation.hpp"
#include <windows.h>
#include <limits>
#include <thread>
#include <algorithm>
#include <array>
#include <climits>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <memory>
#include <optional>
#include <span>
#include <stdexcept>
#include <vector>

// A bounded GPU diagnostic for the recovered first surface, river and fog groups.
// It deliberately does not stand in for the complete native minimap pass list.
struct GlMinimapKind5Marker {
    float center_x = 0, center_y = 0, radius = 0;
    std::uint32_t argb = 0;
    float fade_min = civ5::ui::minimap_city_fade.x;
    float fade_max = civ5::ui::minimap_city_fade.y;
    std::uint32_t segment = 0;
};

struct GlMinimapCultureColor {
    std::uint8_t payload = 0;
    std::array<std::uint8_t, 3> rgb{};
};

struct GlMinimapSurfaceDiagnostic {
  private:
    struct AppliedIdentity {};
  public:
    // Caller-declared correspondence to the selected scene, independently
    // checked against the real preparation owner. Not native-base proof.
    struct PrefogSource {
        civ5::graphics::BnwRendererSceneReceipt scene;
        civ5::ui::BnwMinimapFogGeometry geometry;
    };
    class AppliedFogReceipt {
      public:
        bool own_gl_applied() const noexcept { return true; }
        bool independent_native_base_proof() const noexcept { return false; }
        bool captured_original_pixel_parity() const noexcept { return false; }
        const auto &prepared_identity() const noexcept { return packet_->identity(); }
        const std::string &prefog_sha256() const noexcept { return prefog_sha256_; }
        std::uint64_t write_revision() const noexcept { return revision_; }
        GLuint target_id() const noexcept { return target_; }
        const civ5::ui::MinimapVisibleBounds &render_bounds() const noexcept { return bounds_; }
      private:
        friend struct GlMinimapSurfaceDiagnostic;
        AppliedFogReceipt(std::shared_ptr<const AppliedIdentity> owner,
            std::shared_ptr<const civ5::ui::BnwMinimapFogPacket> packet,
            const civ5::ui::BnwMinimapFogPreparationOwner *preparation,
            const GlTexture *texture, GLuint target, std::uint64_t revision,
            std::string prefog_sha256, civ5::ui::MinimapVisibleBounds bounds)
            : owner_(std::move(owner)), packet_(std::move(packet)), preparation_(preparation),
              texture_(texture), target_(target), revision_(revision),
              prefog_sha256_(std::move(prefog_sha256)), bounds_(std::move(bounds)) {}
        std::shared_ptr<const AppliedIdentity> owner_;
        std::shared_ptr<const civ5::ui::BnwMinimapFogPacket> packet_;
        const civ5::ui::BnwMinimapFogPreparationOwner *preparation_;
        const GlTexture *texture_;
        GLuint target_;
        std::uint64_t revision_;
        std::string prefog_sha256_;
        civ5::ui::MinimapVisibleBounds bounds_;
    };

    static constexpr int width = 320, height = 190;
    GlRenderApi gl;
    std::shared_ptr<GlContext> context;
    GlTexture output, water, coast, land, fog, mountain, ice, ice_stencil;
    std::optional<GlTexture> culture;
    GLuint framebuffer = 0, program = 0, vertex_array = 0, vertex_buffer = 0;
    struct IceGpuRegion {
        GLuint array = 0, vertices = 0, indices = 0;
        GLsizei count = 0;
    };
    std::vector<IceGpuRegion> ice_gpu_regions;
    GLuint sampler = 0, sprite_sampler = 0;
    GLint bounds_location = -1, tiling_location = -1, lod_location = -1, mode_location = -1;
    GLint fog_half_pixel_location = -1, clip_pixel_shift_location = -1;
    GLint sprite_location = -1, city_color_location = -1, city_fade_location = -1;
    GLint culture_color_location = -1;
    GLint ice_box_location = -1, ice_offset_location = -1;
    GLint ice_projection_location = -1, ice_pixel_viewport_location = -1;

    GlMinimapSurfaceDiagnostic(std::shared_ptr<GlContext> owner,
                               const std::array<civ5::assets::DdsData, 6> &sources,
                               const civ5::assets::DdsData *culture_source = nullptr)
        : gl(*owner), context(std::move(owner)),
          output(GlTexture::render_target(*context, width, height, 1, RenderTextureFormat::rgba8)),
          water(*context, sources[0], DdsSamplingColor::srgb),
          coast(*context, sources[1], DdsSamplingColor::srgb),
          land(*context, sources[2], DdsSamplingColor::srgb),
          fog(*context, sources[3], DdsSamplingColor::srgb),
          mountain(*context, sources[4], DdsSamplingColor::srgb),
          ice(*context, sources[5], DdsSamplingColor::srgb),
          ice_stencil(GlTexture::render_target(*context, width, height, 1,
                                              RenderTextureFormat::depth24_stencil8)) {
        native_context_ = wglGetCurrentContext();
        native_dc_ = wglGetCurrentDC();
        for (const auto &dds : std::span(sources).first(4))
            if (dds.width() != 256 || dds.height() != 256 || dds.mip_count() != 9)
                throw std::invalid_argument("Original minimap DDS dimensions changed");
        if (sources[4].width() != 128 || sources[4].height() != 128 ||
            sources[4].mip_count() != 8)
            throw std::invalid_argument("Original minimap mountain DDS dimensions changed");
        if (sources[5].width() != 128 || sources[5].height() != 128 ||
            sources[5].mip_count() != 1)
            throw std::invalid_argument("Original minimap ice DDS dimensions changed");
        if (culture_source) {
            if (culture_source->width() != 256 || culture_source->height() != 256 ||
                culture_source->mip_count() != 9)
                throw std::invalid_argument("Original minimap culture DDS dimensions changed");
            culture.emplace(*context, *culture_source, DdsSamplingColor::srgb);
        }
        gl.CreateFramebuffers(1, &framebuffer);
        gl.NamedFramebufferTexture(framebuffer, GL_COLOR_ATTACHMENT0, output.id(), 0);
        gl.NamedFramebufferTexture(framebuffer, GL_DEPTH_STENCIL_ATTACHMENT,
                                   ice_stencil.id(), 0);
        const GLenum attachment = GL_COLOR_ATTACHMENT0;
        gl.NamedFramebufferDrawBuffers(framebuffer, 1, &attachment);
        if (gl.CheckNamedFramebufferStatus(framebuffer, GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
            throw std::runtime_error("Minimap diagnostic framebuffer is incomplete");
        const char *vertex_source =
            "#version 460 core\n"
            "layout(location=0) in vec2 position; uniform vec4 bounds,ice_box;"
            "uniform float ice_offset_x; uniform vec4 ice_projection,ice_pixel_viewport; uniform int mode;"
            "uniform vec2 clip_pixel_shift; out vec2 world;"
            "void main(){if(mode==6){"
            "world=vec2(bounds.x+(position.x-ice_pixel_viewport.x)*"
            "(bounds.z-bounds.x)/ice_pixel_viewport.y,"
            "bounds.w-position.y*(bounds.w-bounds.y)/ice_pixel_viewport.z);"
            "vec2 clip=vec2((position.x-ice_pixel_viewport.x+.5)*2./"
            "ice_pixel_viewport.y-1.,1.-(position.y+.5)*2./ice_pixel_viewport.z);"
            "gl_Position=vec4(clip,0.,1.);gl_PointSize=1.;return;}"
            "world=mode==4?"
            "fma(fma(position,vec2(.5),vec2(.5)),ice_box.zw-ice_box.xy,ice_box.xy):position;"
            "if(mode==4)world.x+=ice_offset_x;"
            "vec2 q=(world-bounds.xy)/(bounds.zw-bounds.xy);"
            "vec2 clip=mode==4?fma(world,ice_projection.xy,-ice_projection.zw):q*2.-1.;"
            "gl_Position=vec4(clip+clip_pixel_shift,0.,1.);}";
        const char *fragment_source =
            "#version 460 core\n"
            "layout(binding=0) uniform sampler2D source; uniform vec2 tiling;"
            "uniform float lod; uniform int mode; uniform vec4 bounds; uniform vec4 sprite;"
            "uniform vec2 fog_half_pixel,city_fade; uniform vec4 city_color,culture_color;"
            "in vec2 world; out vec4 color;"
            "void main(){if(mode==5){"
            "vec2 uv=vec2((world.x-sprite.x)/(2.*sprite.z),"
            "(sprite.y-world.y)/(2.*sprite.z));"
            "float d=2.*length(uv);"
            "float t=clamp((d-city_fade.x)/(city_fade.y-city_fade.x),0.,1.);"
            "float a=(1.-3.*t*t+2.*t*t*t)*city_color.a;"
            "vec3 linear=pow(max(city_color.rgb,vec3(0.)),vec3(2.20000005));"
            "color=vec4(pow(max(linear,vec3(0.)),vec3(.454545468)),a);return;}"
            "vec2 uv=mode==1?vec2((world.x-bounds.x)/(bounds.z-bounds.x),"
            "(bounds.w-world.y)/(bounds.w-bounds.y)):mode==2?"
            "vec2((world.x-sprite.x)/(2.*sprite.z)+.5,(sprite.y-world.y)/(2.*sprite.z)+.5):"
            "world*tiling.x+tiling.y;"
            "if(mode==1) uv-=fog_half_pixel;"
            "vec4 sampled=mode>=2?texture(source,uv):textureLod(source,uv,lod);"
            "if(mode==7){color=vec4(pow(max(sampled.rgb*culture_color.rgb,vec3(0.)),"
            "vec3(.454545468)),1.);return;}"
            "color=vec4(pow(max(sampled.rgb,vec3(0.)),vec3(0.454545468)),"
            "mode==4||mode==6?1.:sampled.a);}";
        const auto vs = gl.CreateShader(GL_VERTEX_SHADER), ps = gl.CreateShader(GL_FRAGMENT_SHADER);
        gl.ShaderSource(vs, 1, &vertex_source, nullptr);
        gl.CompileShader(vs);
        gl.ShaderSource(ps, 1, &fragment_source, nullptr);
        gl.CompileShader(ps);
        GLint compiled_vs = 0, compiled_ps = 0;
        gl.GetShaderiv(vs, GL_COMPILE_STATUS, &compiled_vs);
        gl.GetShaderiv(ps, GL_COMPILE_STATUS, &compiled_ps);
        if (!compiled_vs || !compiled_ps)
            throw std::runtime_error("Minimap diagnostic shader compilation failed");
        program = gl.CreateProgram();
        gl.AttachShader(program, vs);
        gl.AttachShader(program, ps);
        gl.LinkProgram(program);
        gl.DeleteShader(vs);
        gl.DeleteShader(ps);
        GLint linked = 0;
        gl.GetProgramiv(program, GL_LINK_STATUS, &linked);
        if (!linked)
            throw std::runtime_error("Minimap diagnostic shader link failed");
        bounds_location = gl.GetUniformLocation(program, "bounds");
        tiling_location = gl.GetUniformLocation(program, "tiling");
        lod_location = gl.GetUniformLocation(program, "lod");
        mode_location = gl.GetUniformLocation(program, "mode");
        fog_half_pixel_location = gl.GetUniformLocation(program, "fog_half_pixel");
        clip_pixel_shift_location = gl.GetUniformLocation(program, "clip_pixel_shift");
        sprite_location = gl.GetUniformLocation(program, "sprite");
        city_color_location = gl.GetUniformLocation(program, "city_color");
        culture_color_location = gl.GetUniformLocation(program, "culture_color");
        city_fade_location = gl.GetUniformLocation(program, "city_fade");
        ice_box_location = gl.GetUniformLocation(program, "ice_box");
        ice_offset_location = gl.GetUniformLocation(program, "ice_offset_x");
        ice_projection_location = gl.GetUniformLocation(program, "ice_projection");
        ice_pixel_viewport_location = gl.GetUniformLocation(program, "ice_pixel_viewport");
        if (bounds_location < 0 || tiling_location < 0 || lod_location < 0 ||
            mode_location < 0 || fog_half_pixel_location < 0 || clip_pixel_shift_location < 0 ||
            sprite_location < 0 || city_color_location < 0 || culture_color_location < 0 ||
            city_fade_location < 0 ||
            ice_box_location < 0 || ice_offset_location < 0 ||
            ice_projection_location < 0 || ice_pixel_viewport_location < 0)
            throw std::runtime_error("Minimap diagnostic shader uniforms are incomplete");
        gl.CreateVertexArrays(1, &vertex_array);
        gl.CreateBuffers(1, &vertex_buffer);
        gl.VertexArrayVertexBuffer(vertex_array, 0, vertex_buffer, 0, sizeof(civ5::ui::MinimapPoint2));
        gl.EnableVertexArrayAttrib(vertex_array, 0);
        gl.VertexArrayAttribFormat(vertex_array, 0, 2, GL_FLOAT, GL_FALSE, 0);
        gl.VertexArrayAttribBinding(vertex_array, 0, 0);
        gl.CreateSamplers(1, &sampler);
        gl.SamplerParameteri(sampler, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        gl.SamplerParameteri(sampler, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        gl.SamplerParameteri(sampler, GL_TEXTURE_WRAP_S, GL_REPEAT);
        gl.SamplerParameteri(sampler, GL_TEXTURE_WRAP_T, GL_REPEAT);
        gl.CreateSamplers(1, &sprite_sampler);
        gl.SamplerParameteri(sprite_sampler, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        gl.SamplerParameteri(sprite_sampler, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        gl.SamplerParameteri(sprite_sampler, GL_TEXTURE_WRAP_S, GL_REPEAT);
        gl.SamplerParameteri(sprite_sampler, GL_TEXTURE_WRAP_T, GL_REPEAT);
    }

    ~GlMinimapSurfaceDiagnostic() {
        gl.BindFramebuffer(GL_FRAMEBUFFER, 0);
        clear_ice_gpu_regions();
        if (sprite_sampler) gl.DeleteSamplers(1, &sprite_sampler);
        if (sampler) gl.DeleteSamplers(1, &sampler);
        if (vertex_buffer) gl.DeleteBuffers(1, &vertex_buffer);
        if (vertex_array) gl.DeleteVertexArrays(1, &vertex_array);
        if (program) gl.DeleteProgram(program);
        if (framebuffer) gl.DeleteFramebuffers(1, &framebuffer);
    }

    void clear_ice_gpu_regions() {
        for (const auto &region : ice_gpu_regions) {
            if (region.indices) gl.DeleteBuffers(1, &region.indices);
            if (region.vertices) gl.DeleteBuffers(1, &region.vertices);
            if (region.array) gl.DeleteVertexArrays(1, &region.array);
        }
        ice_gpu_regions.clear();
    }

    void upload_ice_gpu_regions(const civ5::ui::MinimapNativeIceRecords &source) {
        clear_ice_gpu_regions();
        ice_gpu_regions.reserve(source.regions.size());
        for (const auto &mesh : source.regions) {
            IceGpuRegion region;
            gl.CreateVertexArrays(1, &region.array);
            gl.CreateBuffers(1, &region.vertices);
            gl.CreateBuffers(1, &region.indices);
            ice_gpu_regions.push_back(region);
            auto &owned = ice_gpu_regions.back();
            owned.count = static_cast<GLsizei>(mesh.indices.size());
            gl.NamedBufferData(owned.vertices,
                               mesh.vertices.size() * sizeof(mesh.vertices[0]),
                               mesh.vertices.data(), GL_STATIC_DRAW);
            gl.NamedBufferData(owned.indices,
                               mesh.indices.size() * sizeof(mesh.indices[0]),
                               mesh.indices.data(), GL_STATIC_DRAW);
            gl.VertexArrayVertexBuffer(owned.array, 0, owned.vertices, 0,
                                       sizeof(std::array<std::int16_t, 4>));
            gl.VertexArrayElementBuffer(owned.array, owned.indices);
            gl.EnableVertexArrayAttrib(owned.array, 0);
            gl.VertexArrayAttribFormat(owned.array, 0, 2, GL_SHORT, GL_TRUE, 0);
            gl.VertexArrayAttribBinding(owned.array, 0, 0);
        }
    }

    void render(const civ5::ui::MinimapRenderMeshes &meshes,
                const std::vector<civ5::ui::MinimapPoint2> &river_shallows,
                const civ5::ui::MinimapMountainMarkers &mountains,
                const std::vector<civ5::ui::MinimapSavedIcePlot> &ice_plots,
                float world_span_x,
                const civ5::ui::MinimapNativeIceRecords *native_ice = nullptr,
                const std::filesystem::path &water_capture = {},
                const std::filesystem::path &coast_capture = {},
                const std::filesystem::path &land_capture = {},
                const std::filesystem::path &ice_capture = {},
                const std::filesystem::path &river_capture = {},
                const std::filesystem::path &mountain_capture = {},
                float surface_pixel_shift = 0.0f,
                const std::filesystem::path &surface_fog_capture = {},
                const std::filesystem::path &first_mountain_before_capture = {},
                const std::filesystem::path &first_mountain_after_capture = {},
                std::span<const GlMinimapKind5Marker> city_markers = {},
                std::span<const GlMinimapCultureColor> culture_colors = {},
                const std::filesystem::path &culture_capture = {},
                bool retain_prefog = false, const PrefogSource *prefog_source = nullptr) {
        if (consumer_busy_) throw std::logic_error("Reentrant minimap surface producer");
        ConsumerBusy busy(consumer_busy_);
        invalidate_applied_fog();
        prefog_source_.reset(); prefog_bgra_.clear(); prefog_sha256_.clear(); prefog_target_ = 0;
        if (retain_prefog && (!prefog_source || meshes.visible.segment_count != 1 ||
            prefog_source->geometry.edge_subdivisions != meshes.visible.edge_subdivisions ||
            meshes.visible.texture.active_width == 0 ||
            std::uint64_t(meshes.visible.texture.padding_x) + meshes.visible.texture.active_width > width ||
            !std::isfinite(meshes.visible.texture.active_aspect) || meshes.visible.texture.active_aspect <= 0))
            throw std::invalid_argument("Pre-fog retention requires exact declared source geometry");
        if (meshes.visible.segment_count != 1 && meshes.visible.segment_count != 2)
            throw std::invalid_argument("Unsupported minimap diagnostic segment count");
        const auto &b = meshes.visible.segments[0];
        const auto &geometry = meshes.visible.texture;
        if (!(b.max_x > b.min_x && b.max_y > b.min_y) ||
            geometry.padding_x + geometry.active_width > width ||
            !std::isfinite(world_span_x) || world_span_x <= 0 ||
            !std::isfinite(surface_pixel_shift) ||
            surface_pixel_shift < 0.0f || surface_pixel_shift > 1.0f)
            throw std::invalid_argument("Invalid minimap diagnostic viewport");
        for (const auto &marker : city_markers)
            if (!(std::isfinite(marker.center_x) && std::isfinite(marker.center_y) &&
                  std::isfinite(marker.radius) && marker.radius > 0 &&
                  std::isfinite(marker.fade_min) && std::isfinite(marker.fade_max) &&
                  marker.fade_max > marker.fade_min &&
                  marker.segment < meshes.visible.segment_count))
                throw std::invalid_argument("Invalid minimap kind-5 marker");
        if (native_ice) upload_ice_gpu_regions(*native_ice);
        else clear_ice_gpu_regions();
        gl.BindFramebuffer(GL_FRAMEBUFFER, framebuffer);
        gl.ClipControl(GL_LOWER_LEFT, GL_NEGATIVE_ONE_TO_ONE);
        for (auto flag : {GL_DEPTH_TEST, GL_CULL_FACE, GL_BLEND, GL_SCISSOR_TEST,
                          GL_STENCIL_TEST, GL_PROGRAM_POINT_SIZE,
                          GL_FRAMEBUFFER_SRGB, GL_DITHER})
            gl.Disable(flag);
        const float black[4]{0, 0, 0, 1};
        gl.ClearNamedFramebufferfv(framebuffer, GL_COLOR, 0, black);
        gl.Viewport(geometry.padding_x, 0, geometry.active_width, height);
        gl.UseProgram(program);
        gl.BindVertexArray(vertex_array);
        gl.BindSampler(0, sampler);
        const std::array<float, 4> bounds{b.min_x, b.min_y, b.max_x, b.max_y};
        gl.ProgramUniform4fv(program, bounds_location, 1, bounds.data());
        // Retail kind-9 VS uses MAD(world, c4 scale, -c3 offset), where c3
        // is rounded on the CPU before upload. Its order matters at ice edges.
        const float ice_sx = 2.0f / (bounds[2] - bounds[0]);
        const float ice_sy = 2.0f / (bounds[3] - bounds[1]);
        const std::array<float, 4> ice_projection{
            ice_sx, ice_sy, (bounds[0] + bounds[2]) * 0.5f * ice_sx,
            (bounds[1] + bounds[3]) * 0.5f * ice_sy};
        gl.ProgramUniform4fv(program, ice_projection_location, 1,
                             ice_projection.data());
        const std::array<float, 4> ice_pixel_viewport{
            float(geometry.padding_x), float(geometry.active_width), float(height), 0};
        gl.ProgramUniform4fv(program, ice_pixel_viewport_location, 1,
                             ice_pixel_viewport.data());
        // The fog UV correction is reduced by the same screen-space shift
        // applied to its geometry, avoiding a double half-pixel correction.
        const std::array<float, 2> fog_half_pixel{
            (0.5f - surface_pixel_shift) / geometry.active_width,
            (0.5f - surface_pixel_shift) / height};
        gl.ProgramUniform2fv(program, fog_half_pixel_location, 1, fog_half_pixel.data());
        const std::array<float, 2> no_shift{0.0f, 0.0f};
        gl.ProgramUniform2fv(program, clip_pixel_shift_location, 1, no_shift.data());
        const auto draw = [&](const GlTexture &texture,
                              const std::vector<civ5::ui::MinimapPoint2> &triangles,
                              const std::array<float, 2> &tiling, float lod, int mode) {
            draw_layer(texture, triangles, tiling, lod, mode);
        };
        const float span_x = b.max_x - b.min_x, span_y = b.max_y - b.min_y;
        const auto projected_lod = [&](float scale) {
            return std::clamp(std::log2(std::max(span_x * scale * 256.0f / geometry.active_width,
                                                  span_y * scale * 256.0f / height)), 0.0f, 8.0f);
        };
        constexpr float water_scale = 0.002000000095f;
        const std::array<civ5::ui::MinimapPoint2, 6> quad{{
            {b.min_x, b.min_y}, {b.max_x, b.min_y}, {b.max_x, b.max_y},
            {b.min_x, b.min_y}, {b.max_x, b.max_y}, {b.min_x, b.max_y}}};
        draw(water, {quad.begin(), quad.end()}, {water_scale, 100.0f},
             projected_lod(water_scale), 0);
        if (!water_capture.empty()) save_ppm(water_capture);
        const std::array<float, 2> surface_shift{
            2.0f * surface_pixel_shift / geometry.active_width,
            -2.0f * surface_pixel_shift / height};
        gl.ProgramUniform2fv(program, clip_pixel_shift_location, 1, surface_shift.data());
        // The original kind-4 coast and kind-1 land pixel shaders write the
        // sampled texture alpha to oC0.w. Civ5 enables SRCALPHA/INVSRCALPHA
        // blending for those two passes, then disables blending for fog.
        gl.Enable(GL_BLEND);
        gl.BlendEquationSeparate(GL_FUNC_ADD, GL_FUNC_ADD);
        gl.BlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA,
                             GL_ONE, GL_ONE);
        draw(coast, meshes.coast.triangles, {water_scale, 100.0f},
             projected_lod(water_scale), 0);
        if (!coast_capture.empty()) save_ppm(coast_capture);
        draw(land, meshes.land.triangles, {0.800000012f, 1.0f}, 8.0f, 0);
        if (!land_capture.empty()) save_ppm(land_capture);
        const auto draw_culture = [&] {
            if (!meshes.culture.empty() && !culture)
                throw std::invalid_argument("Minimap culture mesh has no original texture");
            for (const auto &layer : meshes.culture) {
                const auto found = std::find_if(culture_colors.begin(), culture_colors.end(),
                    [&](const auto &color) { return color.payload == layer.payload; });
                if (found == culture_colors.end())
                    throw std::invalid_argument("Minimap culture group has no captured color");
                std::array<float, 4> linear{};
                for (std::size_t channel = 0; channel < 3; ++channel)
                    linear[channel] = std::pow(float(found->rgb[channel]) /
                                               255.0f, 2.20000005f);
                linear[3] = 1.0f;
                gl.ProgramUniform4fv(program, culture_color_location, 1, linear.data());
                draw(*culture, layer.mesh.triangles,
                     {civ5::ui::minimap_culture_texture_tiling.x,
                      civ5::ui::minimap_culture_texture_tiling.y},
                     projected_lod(civ5::ui::minimap_culture_texture_tiling.x), 7);
            }
        };
        draw_culture();
        if (!culture_capture.empty()) save_ppm(culture_capture);
        if (!surface_fog_capture.empty()) {
            // Capture the four original surface passes without later ice,
            // river and marker draws, then restore the normal draw order.
            gl.Disable(GL_BLEND);
            draw(fog, meshes.fog.triangles, {1.0f, 0.0f},
                 std::log2(std::max(256.0f / geometry.active_width, 256.0f / height)), 1);
            save_ppm(surface_fog_capture);
            gl.ClearNamedFramebufferfv(framebuffer, GL_COLOR, 0, black);
            gl.ProgramUniform2fv(program, clip_pixel_shift_location, 1, no_shift.data());
            draw(water, {quad.begin(), quad.end()}, {water_scale, 100.0f},
                 projected_lod(water_scale), 0);
            gl.ProgramUniform2fv(program, clip_pixel_shift_location, 1, surface_shift.data());
            gl.Enable(GL_BLEND);
            draw(coast, meshes.coast.triangles, {water_scale, 100.0f},
                 projected_lod(water_scale), 0);
            draw(land, meshes.land.triangles, {0.800000012f, 1.0f}, 8.0f, 0);
            draw_culture();
        }
        // Keep the historical river-only control independent of the new
        // marker layer. Replay the base passes after taking that diagnostic
        // image so the final draw still follows native segment ordering.
        if (!river_capture.empty() &&
            (!mountains.source.empty() || !ice_plots.empty() || native_ice)) {
            for (std::uint32_t segment = 0; segment < meshes.visible.segment_count; ++segment) {
                const auto &current = meshes.visible.segments[segment];
                const std::array<float, 4> river_bounds{
                    current.min_x, current.min_y, current.max_x, current.max_y};
                gl.ProgramUniform4fv(program, bounds_location, 1, river_bounds.data());
                draw(coast, river_shallows, {water_scale, 100.0f},
                     projected_lod(water_scale), 0);
            }
            save_ppm(river_capture);
            gl.ClearNamedFramebufferfv(framebuffer, GL_COLOR, 0, black);
            gl.ProgramUniform4fv(program, bounds_location, 1, bounds.data());
            gl.ProgramUniform2fv(program, clip_pixel_shift_location, 1, no_shift.data());
            draw(water, {quad.begin(), quad.end()}, {water_scale, 100.0f},
                 projected_lod(water_scale), 0);
            gl.ProgramUniform2fv(program, clip_pixel_shift_location, 1, surface_shift.data());
            draw(coast, meshes.coast.triangles, {water_scale, 100.0f},
                 projected_lod(water_scale), 0);
            draw(land, meshes.land.triangles, {0.800000012f, 1.0f}, 8.0f, 0);
            draw_culture();
        }
        // The offline retail region stream is an explicit diagnostic input.
        // Without it, retain the saved FEATURE_ICE candidate for other maps.
        std::vector<civ5::ui::MinimapIceRegion> ice_regions;
        ice_regions.reserve(native_ice ? native_ice->regions.size() : ice_plots.size());
        if (native_ice) {
            for (const auto &mesh : native_ice->regions)
                ice_regions.push_back({{mesh.box[0], mesh.box[1], mesh.box[2], mesh.box[3]}});
        } else {
            for (const auto &plot : ice_plots) ice_regions.push_back({plot.bounds});
        }
        const auto ice_draws = civ5::ui::plan_minimap_ice_draws(
            b, world_span_x, ice_regions);
        if (native_ice && !native_ice->shader_box)
            throw std::runtime_error("Native ice records need the original component shader bounds");
        for (const auto &ice_draw : ice_draws) {
            if (native_ice) {
                const auto &native_region = native_ice->regions.at(ice_draw.region_index);
                const std::array<float, 4> ice_view{
                    b.min_x, b.min_y, b.max_x, b.max_y};
                const auto pixels = civ5::ui::raster_minimap_ice_dx9_pixels(
                    native_region, *native_ice->shader_box, ice_view,
                    ice_draw.vertex_offset_x, width, height,
                    geometry.padding_x, geometry.active_width);
                if (pixels.empty()) continue;
                gl.ProgramUniform4fv(program, ice_box_location, 1,
                                     native_ice->shader_box->data());
                gl.ProgramUniform1f(program, ice_offset_location, ice_draw.vertex_offset_x);
                const std::array<float, 2> tiling{
                    civ5::ui::minimap_ice_texture_tiling.x,
                    civ5::ui::minimap_ice_texture_tiling.y};
                gl.ProgramUniform2fv(program, tiling_location, 1, tiling.data());
                const auto &gpu = ice_gpu_regions.at(ice_draw.region_index);
                ice.bind(0);
                gl.Enable(GL_PROGRAM_POINT_SIZE);
                gl.StencilMask(0xff);
                gl.ClearStencil(0);
                gl.Clear(GL_STENCIL_BUFFER_BIT);
                gl.Enable(GL_STENCIL_TEST);
                gl.StencilFunc(GL_ALWAYS, 1, 0xff);
                gl.StencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
                gl.ColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
                gl.NamedBufferData(vertex_buffer,
                                   pixels.size() * sizeof(civ5::ui::MinimapPoint2),
                                   pixels.data(), GL_STREAM_DRAW);
                gl.BindVertexArray(vertex_array);
                gl.ProgramUniform1i(program, mode_location, 6);
                gl.DrawArrays(GL_POINTS, 0, static_cast<GLsizei>(pixels.size()));
                gl.ColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
                gl.StencilFunc(GL_LEQUAL, 1, 0xff);
                gl.StencilOp(GL_KEEP, GL_KEEP, GL_INCR);
                gl.ProgramUniform1i(program, mode_location, 4);
                gl.BindVertexArray(gpu.array);
                gl.DrawElements(GL_TRIANGLES, gpu.count, GL_UNSIGNED_SHORT, nullptr);
                gl.StencilFunc(GL_EQUAL, 1, 0xff);
                gl.StencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
                gl.BindVertexArray(vertex_array);
                gl.ProgramUniform1i(program, mode_location, 6);
                gl.DrawArrays(GL_POINTS, 0, static_cast<GLsizei>(pixels.size()));
                gl.Disable(GL_STENCIL_TEST);
                gl.Disable(GL_PROGRAM_POINT_SIZE);
                continue;
            }
            const auto &plot = ice_plots.at(ice_draw.region_index);
            std::vector<civ5::ui::MinimapPoint2> triangles;
            triangles.reserve(18);
            for (std::size_t corner = 0; corner < plot.corners.size(); ++corner) {
                const auto next = (corner + 1) % plot.corners.size();
                triangles.push_back({plot.center.x + ice_draw.vertex_offset_x,
                                     plot.center.y});
                triangles.push_back({plot.corners[corner].x + ice_draw.vertex_offset_x,
                                     plot.corners[corner].y});
                triangles.push_back({plot.corners[next].x + ice_draw.vertex_offset_x,
                                     plot.corners[next].y});
            }
            draw(ice, triangles, {civ5::ui::minimap_ice_texture_tiling.x,
                                  civ5::ui::minimap_ice_texture_tiling.y}, 0, 3);
        }
        if (!ice_capture.empty()) save_ppm(ice_capture);
        // Retail 0x004194D0..0x00419572 projects the river-surface VB/IB once
        // per visible X segment and submits kind 3 before the final fog pass.
        for (std::uint32_t segment = 0; segment < meshes.visible.segment_count; ++segment) {
            const auto &current = meshes.visible.segments[segment];
            if (!(current.max_x > current.min_x && current.max_y > current.min_y))
                throw std::invalid_argument("Invalid wrapped minimap river bounds");
            const std::array<float, 4> river_bounds{
                current.min_x, current.min_y, current.max_x, current.max_y};
            gl.ProgramUniform4fv(program, bounds_location, 1, river_bounds.data());
            draw(coast, river_shallows, {water_scale, 100.0f},
                 projected_lod(water_scale), 0);
            gl.BindSampler(0, sprite_sampler);
            for (const auto &marker : city_markers) {
                if (marker.segment != segment) continue;
                const float x = marker.center_x, y = marker.center_y, r = marker.radius;
                const std::array<float, 4> sprite{x, y, r, 0};
                const std::array<float, 4> color{
                    ((marker.argb >> 16) & 255) / 255.0f,
                    ((marker.argb >> 8) & 255) / 255.0f,
                    (marker.argb & 255) / 255.0f,
                    ((marker.argb >> 24) & 255) / 255.0f};
                const std::array<float, 2> fade{marker.fade_min, marker.fade_max};
                gl.ProgramUniform4fv(program, sprite_location, 1, sprite.data());
                gl.ProgramUniform4fv(program, city_color_location, 1, color.data());
                gl.ProgramUniform2fv(program, city_fade_location, 1, fade.data());
                const std::array<civ5::ui::MinimapPoint2, 6> marker_quad{{
                    {x - r, y + r}, {x + r, y + r}, {x + r, y - r},
                    {x - r, y + r}, {x + r, y - r}, {x - r, y - r}}};
                draw(mountain, {marker_quad.begin(), marker_quad.end()}, {0, 0}, 0, 5);
            }
            if (segment == 0 && !first_mountain_before_capture.empty())
                save_ppm(first_mountain_before_capture);
            for (const auto marker_index : mountains.visible_indices[segment]) {
                const auto &marker = mountains.source.at(marker_index);
                // Retail kind 6 writes ARGB 0xFFFFFFFF to each marker record
                // at 0x004D80D9, so the sprite shader's vertex color is white.
                const float x = marker.center.x, y = marker.center.y, r = mountains.radius;
                const std::array<float, 4> sprite{x, y, r, 0};
                gl.ProgramUniform4fv(program, sprite_location, 1, sprite.data());
                const std::array<civ5::ui::MinimapPoint2, 6> marker_quad{{
                    {x - r, y + r}, {x + r, y + r}, {x + r, y - r},
                    {x - r, y + r}, {x + r, y - r}, {x - r, y - r}}};
                draw(mountain, {marker_quad.begin(), marker_quad.end()}, {0, 0}, 0, 2);
            }
            if (segment == 0 && !first_mountain_after_capture.empty())
                save_ppm(first_mountain_after_capture);
            gl.BindSampler(0, sampler);
        }
        if (!river_capture.empty() && mountains.source.empty() && ice_plots.empty() && !native_ice)
            save_ppm(river_capture);
        if (!mountain_capture.empty()) save_ppm(mountain_capture);
        if (retain_prefog) {
            require_owned_gl_current();
            // Exact post-static/pre-final-fog target bytes, never a post-fog image.
            prefog_bgra_ = output.read_mip_raw(0);
            if (prefog_bgra_.size() != width * height * 4u)
                throw std::runtime_error("Pre-fog GPU base extent changed");
            prefog_source_ = *prefog_source;
            prefog_visible_ = meshes.visible;
            prefog_pixel_shift_ = surface_pixel_shift;
            prefog_target_ = output.id();
            prefog_sha256_ = civ5::runtime::identify_bytes(prefog_bgra_).sha256;
            require_target(output);
        }
        gl.ProgramUniform4fv(program, bounds_location, 1, bounds.data());
        gl.Disable(GL_BLEND);
        draw(fog, meshes.fog.triangles, {1.0f, 0.0f},
             std::log2(std::max(256.0f / geometry.active_width, 256.0f / height)), 1);
        gl.BindSampler(0, 0);
        gl.BindVertexArray(0);
        gl.UseProgram(0);
        gl.BindFramebuffer(GL_FRAMEBUFFER, 0);
        context->check("GPU minimap surface diagnostic");
    }

    static void save_ppm(const GlTexture &texture, const std::filesystem::path &path) {
        const auto bgra = texture.read_mip_raw(0);
        if (bgra.size() != width * height * 4)
            throw std::runtime_error("Minimap GPU readback extent changed");
        std::ofstream file(path, std::ios::binary);
        file << "P6\n" << width << ' ' << height << "\n255\n";
        for (int y = height - 1; y >= 0; --y)
            for (int x = 0; x < width; ++x) {
                const auto at = (static_cast<std::size_t>(y) * width + x) * 4;
                const char rgb[3]{static_cast<char>(bgra[at + 2]),
                                  static_cast<char>(bgra[at + 1]),
                                  static_cast<char>(bgra[at])};
                file.write(rgb, sizeof(rgb));
            }
        if (!file) throw std::runtime_error("Minimap GPU diagnostic PPM write failed");
    }

    void save_ppm(const std::filesystem::path &path) const { save_ppm(output, path); }

    // Invalidates before a possible target mutation, including failed writes.
    // Root's optional viewport-ring overlay needs its own final composition
    // revision/camera fence: this receipt attests the fog pass, not later draws.
    void invalidate_applied_fog() {
        applied_.reset();
        if (write_revision_ == std::numeric_limits<std::uint64_t>::max())
            throw std::overflow_error("Minimap write revision exhausted");
        ++write_revision_;
    }

    AppliedFogReceipt apply_current_fog(
        const civ5::ui::BnwMinimapFogPreparationOwner &preparation,
        const std::shared_ptr<const civ5::ui::BnwMinimapFogPacket> &packet,
        GlTexture &destination) {
        // Borrowed owner/target and the shared_ptr storage object outlive
        // the call; callbacks may reset/replace the stored handle.
        const auto packet_pin = packet;
        if (consumer_busy_) throw std::logic_error("Reentrant minimap GPU consumer");
        ConsumerBusy busy(consumer_busy_);
        invalidate_applied_fog();
        const auto revision = write_revision_;
        require_owned_gl_current(); // check() otherwise silently switches WGL.
        if (!prefog_source_ || prefog_bgra_.size() != width * height * 4u ||
            !packet_pin || packet != packet_pin || !preparation.current(packet_pin) ||
            !preparation.source_context_matches(prefog_source_->scene, prefog_source_->geometry) ||
            packet != packet_pin)
            throw std::invalid_argument("Minimap packet/base source is missing or stale");
        require_owned_gl_current();
        require_target(destination);
        if (write_revision_ != revision)
            throw std::invalid_argument("Minimap write changed before restore");
        const auto &api = context->api();
        // Reset CPU upload interpretation, including a previously bound PBO.
        const std::array<GLenum, 4> fields{GL_UNPACK_ALIGNMENT, GL_UNPACK_ROW_LENGTH,
            GL_UNPACK_SKIP_ROWS, GL_UNPACK_SKIP_PIXELS};
        std::array<GLint, 4> previous{};
        GLint previous_buffer = 0;
        api.GetIntegerv(GL_PIXEL_UNPACK_BUFFER_BINDING, &previous_buffer);
        for (std::size_t i = 0; i < fields.size(); ++i) {
            api.GetIntegerv(fields[i], &previous[i]);
            api.PixelStorei(fields[i], i == 0 ? 4 : 0);
        }
        api.BindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
        api.TextureSubImage2D(destination.id(), 0, 0, 0, width, height,
            GL_BGRA, GL_UNSIGNED_BYTE, prefog_bgra_.data());
        for (std::size_t i = 0; i < fields.size(); ++i)
            api.PixelStorei(fields[i], previous[i]);
        api.BindBuffer(GL_PIXEL_UNPACK_BUFFER, static_cast<GLuint>(previous_buffer));
        context->check("Restore genuine pre-fog minimap base");

        gl.BindFramebuffer(GL_FRAMEBUFFER, framebuffer);
        const GLenum color = GL_COLOR_ATTACHMENT0;
        gl.NamedFramebufferDrawBuffers(framebuffer, 1, &color);
        gl.ClipControl(GL_LOWER_LEFT, GL_NEGATIVE_ONE_TO_ONE);
        for (auto flag : {GL_DEPTH_TEST, GL_CULL_FACE, GL_BLEND, GL_SCISSOR_TEST,
            GL_STENCIL_TEST, GL_PROGRAM_POINT_SIZE, GL_FRAMEBUFFER_SRGB, GL_DITHER,
            GL_RASTERIZER_DISCARD, GL_COLOR_LOGIC_OP, GL_MULTISAMPLE,
            GL_SAMPLE_ALPHA_TO_COVERAGE, GL_SAMPLE_COVERAGE}) gl.Disable(flag);
        gl.ColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
        gl.PolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        const auto &geometry = prefog_visible_.texture;
        const auto &b = prefog_visible_.segments[0];
        gl.Viewport(geometry.padding_x, 0, geometry.active_width, height);
        gl.UseProgram(program);
        gl.BindVertexArray(vertex_array);
        gl.BindSampler(0, sampler);
        const float bounds[]{b.min_x, b.min_y, b.max_x, b.max_y};
        const float correction[]{(0.5f - prefog_pixel_shift_) / geometry.active_width,
                                  (0.5f - prefog_pixel_shift_) / height};
        const float shift[]{2.0f * prefog_pixel_shift_ / geometry.active_width,
                            -2.0f * prefog_pixel_shift_ / height};
        gl.ProgramUniform4fv(program, bounds_location, 1, bounds);
        gl.ProgramUniform2fv(program, fog_half_pixel_location, 1, correction);
        gl.ProgramUniform2fv(program, clip_pixel_shift_location, 1, shift);
        // Empty fog still restores the base. No triangle submission is needed.
        draw_layer(fog, packet_pin->fog_mesh().triangles, {1.0f, 0.0f},
            std::log2(std::max(256.0f / geometry.active_width, 256.0f / height)), 1);
        gl.BindSampler(0, 0);
        gl.BindVertexArray(0);
        gl.UseProgram(0);
        gl.BindFramebuffer(GL_FRAMEBUFFER, 0);
        context->check("Apply current prepared minimap fog on own GL target");
        if (write_revision_ != revision || !preparation.current(packet_pin) ||
            !preparation.source_context_matches(prefog_source_->scene, prefog_source_->geometry) ||
            packet != packet_pin)
            throw std::invalid_argument("Minimap source changed during actual fog application");
        require_owned_gl_current();
        require_target(destination);
        context->check("Validate applied minimap fog target");
        AppliedFogReceipt result(applied_identity_, packet_pin, &preparation, &destination,
            destination.id(), revision, prefog_sha256_, prefog_visible_);
        applied_ = result; // Private constructor reached only after actual writes.
        return result;
    }

    bool current(const AppliedFogReceipt &receipt,
        const civ5::ui::BnwMinimapFogPreparationOwner &preparation,
        const std::shared_ptr<const civ5::ui::BnwMinimapFogPacket> &packet,
        const GlTexture &destination) const noexcept {
        // receipt may live inside caller optional storage reset by a
        // source callback. Never read that borrowed object after this copy.
        const auto packet_pin = packet;
        if (consumer_busy_) return false;
        ConsumerBusy busy(consumer_busy_);
        try {
            const auto receipt_pin = receipt;
            if (!applied_ || receipt_pin.owner_ != applied_identity_ ||
                receipt_pin.revision_ != write_revision_ || receipt_pin.packet_ != packet_pin ||
                applied_->packet_ != packet_pin || receipt_pin.preparation_ != &preparation ||
                receipt_pin.texture_ != &destination || receipt_pin.target_ != destination.id() ||
                receipt_pin.prefog_sha256_ != prefog_sha256_ || !prefog_source_)
                return false;
            require_owned_gl_current();
            require_target(destination);
            if (!preparation.current(packet_pin) ||
                !preparation.source_context_matches(prefog_source_->scene, prefog_source_->geometry) ||
                packet != packet_pin)
                return false;
            require_owned_gl_current();
            require_target(destination);
            context->check("Validate current own GL minimap fog receipt");
            if (!preparation.current(packet_pin) ||
                !preparation.source_context_matches(prefog_source_->scene, prefog_source_->geometry) ||
                packet != packet_pin)
                return false;
            require_owned_gl_current();
            require_target(destination);
            return packet == packet_pin && applied_ &&
                applied_->owner_ == receipt_pin.owner_ &&
                applied_->packet_ == packet_pin &&
                applied_->preparation_ == &preparation &&
                applied_->texture_ == &destination &&
                applied_->target_ == destination.id() &&
                applied_->revision_ == receipt_pin.revision_ &&
                receipt_pin.revision_ == write_revision_;
        } catch (...) { return false; }
    }

  private:
    struct ConsumerBusy {
        bool &flag;
        explicit ConsumerBusy(bool &value) : flag(value) { flag = true; }
        ~ConsumerBusy() { flag = false; }
    };
    bool owned_gl_current() const noexcept {
        return std::this_thread::get_id() == render_thread_ && native_context_ && native_dc_ &&
            wglGetCurrentContext() == native_context_ && wglGetCurrentDC() == native_dc_ &&
            WindowFromDC(native_dc_) == context->native_window_handle();
    }
    void require_owned_gl_current() const {
        if (!owned_gl_current()) throw std::invalid_argument("Wrong current minimap WGL context");
        context->check("Before minimap GPU operation");
    }
    void require_target(const GlTexture &destination) const {
        require_owned_gl_current();
        if (!destination.belongs_to(*context) || destination.id() != prefog_target_ ||
            destination.face_count() != 1 || destination.depth() != 1 ||
            destination.levels().size() != 1 || destination.plan().internal_format != GL_RGBA8 ||
            !context->api().IsTexture(destination.id()))
            throw std::invalid_argument("Foreign or replaced minimap fog target");
        GLint actual_width = 0, actual_height = 0, format = 0;
        context->api().GetTextureLevelParameteriv(destination.id(), 0, GL_TEXTURE_WIDTH, &actual_width);
        context->api().GetTextureLevelParameteriv(destination.id(), 0, GL_TEXTURE_HEIGHT, &actual_height);
        context->api().GetTextureLevelParameteriv(destination.id(), 0, GL_TEXTURE_INTERNAL_FORMAT, &format);
        GLint attachment = 0, type = 0, level = -1;
        gl.GetNamedFramebufferAttachmentParameteriv(framebuffer, GL_COLOR_ATTACHMENT0,
            GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME, &attachment);
        gl.GetNamedFramebufferAttachmentParameteriv(framebuffer, GL_COLOR_ATTACHMENT0,
            GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE, &type);
        gl.GetNamedFramebufferAttachmentParameteriv(framebuffer, GL_COLOR_ATTACHMENT0,
            GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_LEVEL, &level);
        if (actual_width != width || actual_height != height || format != GL_RGBA8 ||
            type != GL_TEXTURE || static_cast<GLuint>(attachment) != destination.id() || level != 0 ||
            gl.CheckNamedFramebufferStatus(framebuffer, GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
            throw std::invalid_argument("Minimap native target extent/attachment differs");
        // Unequal attachment extents can clip framebuffer rasterization even
        // though this fog pass disables depth/stencil testing.
        if (!ice_stencil.belongs_to(*context) || !context->api().IsTexture(ice_stencil.id()))
            throw std::invalid_argument("Minimap depth/stencil target is foreign");
        GLint stencil_width = 0, stencil_height = 0, stencil_format = 0;
        context->api().GetTextureLevelParameteriv(ice_stencil.id(), 0, GL_TEXTURE_WIDTH, &stencil_width);
        context->api().GetTextureLevelParameteriv(ice_stencil.id(), 0, GL_TEXTURE_HEIGHT, &stencil_height);
        context->api().GetTextureLevelParameteriv(ice_stencil.id(), 0, GL_TEXTURE_INTERNAL_FORMAT, &stencil_format);
        gl.GetNamedFramebufferAttachmentParameteriv(framebuffer, GL_DEPTH_STENCIL_ATTACHMENT,
            GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME, &attachment);
        gl.GetNamedFramebufferAttachmentParameteriv(framebuffer, GL_DEPTH_STENCIL_ATTACHMENT,
            GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE, &type);
        gl.GetNamedFramebufferAttachmentParameteriv(framebuffer, GL_DEPTH_STENCIL_ATTACHMENT,
            GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_LEVEL, &level);
        if (stencil_width != width || stencil_height != height || stencil_format != GL_DEPTH24_STENCIL8 ||
            type != GL_TEXTURE || static_cast<GLuint>(attachment) != ice_stencil.id() || level != 0)
            throw std::invalid_argument("Minimap native depth/stencil extent/attachment differs");
        context->check("Validate native minimap target layout");
    }
    void draw_layer(const GlTexture &texture,
        const std::vector<civ5::ui::MinimapPoint2> &triangles,
        const std::array<float, 2> &tiling, float lod, int mode) {
        if (triangles.empty()) return;
        if (triangles.size() % 3 || triangles.size() > static_cast<std::size_t>(INT_MAX))
            throw std::invalid_argument("Invalid minimap triangle stream");
        gl.NamedBufferData(vertex_buffer, triangles.size() * sizeof(triangles[0]),
            triangles.data(), GL_STREAM_DRAW);
        gl.ProgramUniform2fv(program, tiling_location, 1, tiling.data());
        gl.ProgramUniform1f(program, lod_location, lod);
        gl.ProgramUniform1i(program, mode_location, mode);
        texture.bind(0);
        gl.DrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(triangles.size()));
    }
    std::shared_ptr<const AppliedIdentity> applied_identity_ = std::make_shared<const AppliedIdentity>();
    std::thread::id render_thread_ = std::this_thread::get_id();
    HGLRC native_context_ = nullptr;
    HDC native_dc_ = nullptr;
    std::uint64_t write_revision_ = 0;
    mutable bool consumer_busy_ = false;
    mutable std::optional<AppliedFogReceipt> applied_;
    std::optional<PrefogSource> prefog_source_;
    civ5::ui::MinimapVisibleBounds prefog_visible_{};
    float prefog_pixel_shift_ = 0;
    GLuint prefog_target_ = 0;
    std::vector<std::uint8_t> prefog_bgra_;
    std::string prefog_sha256_;

};
