#include "civ5/graphics/gl_material_cache_renderer.hpp"
#include "civ5/graphics/gl_material_blend.hpp"
#include "civ5/graphics/gl_material_normals.hpp"
#include "civ5/graphics/gl_material_shadows.hpp"
#include "civ5/graphics/gl_render_api.hpp"
#include "civ5/materials/material_normals.hpp"
#include <algorithm>
#include <bit>
#include <cmath>
#include <map>
#include <stdexcept>

namespace civ5::graphics {
namespace {
using namespace materials;
void require(bool condition, const char *why) {
    if (!condition)
        throw std::invalid_argument(why);
}
float lane(std::span<const std::uint8_t> bytes, unsigned at) {
    require(at <= bytes.size() && bytes.size() - at >= 4, "Truncated cache draw constants");
    const auto bits = std::uint32_t(bytes[at]) | (std::uint32_t(bytes[at + 1]) << 8) |
                      (std::uint32_t(bytes[at + 2]) << 16) | (std::uint32_t(bytes[at + 3]) << 24);
    return std::bit_cast<float>(bits);
}
struct PreparedDraw {
    std::array<GLuint, 16> textures{};
    const GlTexture *sun = nullptr, *shade = nullptr;
    GLuint volume = 0;
    std::array<float, 2> offset{};
    MaterialShadowParameters shadow{};
};
struct PreparedPass {
    std::vector<GlMaterialCacheTarget> targets;
    std::vector<PreparedDraw> draws;
    TerrainDecalDrawList decals;
    std::map<std::pair<std::uint16_t, DdsSamplingColor>, const GlTexture *> decal_textures;
    int width, height;
};
} // namespace
struct GlMaterialCacheRenderer::Impl {
    std::shared_ptr<GlContext> context;
    MaterialPatchMesh material;
    LightingPatchMesh lighting_mesh;
    GlRenderApi gl;
    GlMaterialCacheRaster raster;
    GlMaterialBlendRenderer blend;
    GlMaterialNormalRenderer normal;
    GlMaterialShadowRenderer shadow;
    GlMaterialLightingRenderer lighting;
    MaterialCachePassSource bindings;
    std::uint32_t primitives;
    LightingCubeSampling sampling;
    Impl(std::shared_ptr<GlContext> c, const MaterialPatchMesh &m, const LightingPatchMesh &l,
         const MaterialCachePassSource &b, LightingCubeSampling mode)
        : context(std::move(c)), material(m), lighting_mesh(l), gl(*context), raster(context),
          blend(context, m, MaterialBlendCoordinates::d3d9_cache), normal(context), shadow(context, m),
          lighting(context, l, false, mode), bindings(b),
          primitives(static_cast<std::uint32_t>(m.indices().size() / 3)), sampling(mode) {}

    std::vector<PreparedPass> prepare(const MaterialCachePassPlan &plan,
                                      std::span<const GlMaterialCacheTarget> resources,
                                      std::span<const GlMaterialCacheVolume> volumes,
                                      const GlMaterialCacheFrame &frame,
                                      const GlMaterialCacheDecals *decals) const {
        using Kind = MaterialCachePassKind;
        const bool with_decals = plan.passes.size() >= 6;
        const std::vector<Kind> order = with_decals
            ? std::vector{Kind::height, Kind::decal, Kind::normal, Kind::shadow, Kind::decal_shadow, Kind::lighting}
            : std::vector{Kind::height, Kind::normal, Kind::shadow, Kind::lighting};
        require(plan.passes.size() == order.size() || plan.passes.size() == order.size() + 1,
                "Only complete ordinary uncompressed cache plans are supported");
        require(!with_decals || decals, "Cache decal passes require their packed backend");
        // Retail DX9 leaves cache selectors 1/2 without a usable shader pair
        // (see the pinned native cache-mode gate audit). Keep this rejection
        // explicit so a future alternate material plan cannot fall through the
        // ordinary packet checks and look like a supported cache.
        if (!plan.passes.empty() && !plan.passes.front().draws.empty() &&
            plan.passes.front().draws.front().selector != 0)
            throw std::invalid_argument(
                "Alternate material selector is gated by the retail DX9 shader table");
        require(std::all_of(frame.material_coefficients.begin(), frame.material_coefficients.end(),
                            [](float x) { return std::isfinite(x); }) &&
                    std::all_of(frame.half_vector.begin(), frame.half_vector.end(),
                                [](float x) { return std::isfinite(x); }),
                "Nonfinite cache frame constants");
        for (std::size_t i = 0; i < resources.size(); ++i) {
            require(resources[i].texture && resources[i].texture->belongs_to(*context),
                    "Unavailable or foreign cache resource");
            const auto &texture = *resources[i].texture;
            GLint base = 0, maximum = 0, levels = 0;
            context->api().GetTextureParameteriv(texture.id(), GL_TEXTURE_BASE_LEVEL, &base);
            context->api().GetTextureParameteriv(texture.id(), GL_TEXTURE_MAX_LEVEL, &maximum);
            context->api().GetTextureParameteriv(texture.id(), GL_TEXTURE_IMMUTABLE_LEVELS, &levels);
            const auto expected_levels = texture.levels().size() / texture.face_count();
            require(base == 0 && levels > 0 && static_cast<std::size_t>(levels) == expected_levels &&
                        maximum == levels - 1,
                    "Cache resource mip state differs from its owner");
            for (std::size_t j = 0; j < i; ++j)
                require(resources[i].handle != resources[j].handle, "Duplicate logical cache resource");
        }
        for (std::size_t i = 0; i < volumes.size(); ++i) {
            require(volumes[i].texture && context->api().IsTexture(volumes[i].texture),
                    "Unavailable cache volume");
            for (std::size_t j = 0; j < i; ++j)
                require(volumes[i].handle != volumes[j].handle, "Duplicate logical cache volume");
            GLint previous = 0, width = 0, height = 0, depth = 0;
            context->api().GetIntegerv(GL_TEXTURE_BINDING_3D, &previous);
            context->api().BindTexture(GL_TEXTURE_3D, volumes[i].texture);
            const auto error = context->api().GetError();
            context->api().BindTexture(GL_TEXTURE_3D, static_cast<GLuint>(previous));
            context->check("Restore cache volume preflight binding");
            require(error == GL_NO_ERROR, "Cache ColorCorrect texture dimension");
            context->api().GetTextureLevelParameteriv(volumes[i].texture, 0, GL_TEXTURE_WIDTH, &width);
            context->api().GetTextureLevelParameteriv(volumes[i].texture, 0, GL_TEXTURE_HEIGHT, &height);
            context->api().GetTextureLevelParameteriv(volumes[i].texture, 0, GL_TEXTURE_DEPTH, &depth);
            require(width > 0 && height > 0 && depth > 0, "Cache ColorCorrect needs a complete 3D source");
        }
        const auto resource = [&](std::uint32_t handle) -> const GlTexture & {
            for (const auto &r : resources)
                if (r.handle == handle)
                    return *r.texture;
            throw std::invalid_argument("Unresolved cache resource handle");
        };
        const auto volume = [&](std::uint16_t handle) {
            for (const auto &v : volumes)
                if (v.handle == handle)
                    return v.texture;
            throw std::invalid_argument("Unresolved cache ColorCorrect handle");
        };
        std::vector<PreparedPass> prepared;
        for (std::size_t index = 0; index < plan.passes.size(); ++index) {
            const auto &pass = plan.passes[index];
            const auto kind = index < order.size() ? order[index] : Kind::lighting;
            const bool decal = kind == Kind::decal || kind == Kind::decal_shadow;
            const bool material_target = kind == Kind::height || kind == Kind::decal;
            require(pass.kind == kind && pass.mip == 0, "Unsupported cache pass order or mip");
            require(pass.draws.size() == 1 ||
                        (kind == Kind::shadow && pass.draws.size() <= 32) ||
                        (decal && pass.draws.size() <= 131072),
                    "Unsupported cache draw count");
            PreparedPass p{};
            for (auto handle : pass.targets)
                if (handle)
                    p.targets.push_back({handle, &resource(handle)});
            raster.validate(pass, p.targets);
            require(p.targets.size() == (material_target ? 2u : 1u),
                    "Specular cache production is not implemented");
            p.width = p.targets[0].texture->levels().front().width;
            p.height = p.targets[0].texture->levels().front().height;
            for (const auto &target : p.targets) {
                const auto format = target.texture->plan().internal_format;
                require(material_target ? format == GL_RGBA16F
                        : kind != Kind::lighting ? format == GL_RGBA8
                                                 : format == GL_RGBA8 || format == GL_RGB565,
                        "Unsupported cache pass storage format");
            }
            if (decal) {
                const auto producer = kind == Kind::decal ? 0u : 3u;
                require(pass.targets == plan.passes[producer].targets &&
                            pass.depth_surface == (kind == Kind::decal ? bindings.depth_surface : 0) &&
                            pass.blend_state == (kind == Kind::decal ? 8u : 4u) && pass.flags == 0,
                        "Decal cache layer must retain its producer targets and original blend state");
            }
            for (const auto &draw : pass.draws) {
                const bool inherited = kind == Kind::normal || kind == Kind::lighting;
                require(draw.packet < plan.packets.size() &&
                            (decal ? draw.constant_bytes == 112
                                   : draw.constant_bytes == 0x4c0 ||
                                         (with_decals && inherited && draw.constant_bytes == 112)) &&
                            draw.constant_offset <= plan.constants.size() &&
                            draw.constant_bytes <= plan.constants.size() - draw.constant_offset,
                        "Cache draw packet or constant extent");
                const auto &packet = plan.packets[draw.packet];
                const auto constants = std::span(plan.constants).subspan(draw.constant_offset, draw.constant_bytes);
                if (decal) {
                    require(packet.key == 0x4e && draw.shader_binding == decals->shader_binding &&
                                draw.selector == 0 && packet.pixel_count == 2 && packet.vertex_count == 0 &&
                                packet.pixel_samplers[0] == 2 && packet.pixel_samplers[1] == 0 &&
                                packet.geometry[4] == 0,
                            "Unsupported cache decal shader/packet");
                    TerrainDecalDraw d{};
                    d.index_buffer = packet.geometry[0]; d.vertex_buffer = packet.geometry[1];
                    d.triangles = packet.geometry[2]; d.first_vertex = packet.geometry[3];
                    d.albedo = packet.pixel_textures[0]; d.heightmap = packet.pixel_textures[1];
                    for (unsigned i = 0; i < d.constants.size(); ++i)
                        d.constants[i] = lane(constants, i * 4);
                    p.decals.draws.push_back(d);
                    continue;
                }
                PreparedDraw d{};
                // Normal and lighting inherit the current local block, but
                // their selected shaders do not read its material offsets.
                if (!inherited) {
                    d.offset = {lane(constants, 0), lane(constants, 4)};
                    require(std::isfinite(d.offset[0]) && std::isfinite(d.offset[1]),
                            "Nonfinite cache vertex offset");
                }
                const auto geometry =
                    kind == Kind::normal ? std::array<std::uint32_t, 5>{bindings.normal_index_buffer,
                                                              bindings.normal_vertex_buffer, 2, 0, 0}
                               : std::array<std::uint32_t, 5>{bindings.patch_index_buffer,
                                                              kind == Kind::lighting ? bindings.lighting_vertex_buffer
                                                                                     : bindings.material_vertex_buffer,
                                                              primitives, 0, 0};
                require(packet.geometry == geometry, "Cache geometry requires one complete owned patch");
                const auto bind = [&](unsigned slot, GLenum target = GL_TEXTURE_2D) -> const GlTexture & {
                    const auto &texture = resource(packet.pixel_textures[slot]);
                    require(texture.face_count() == (target == GL_TEXTURE_CUBE_MAP ? 6u : 1u),
                            "Wrong cache input texture dimension");
                    for (const auto &out : p.targets)
                        require(out.texture->id() != texture.id(), "Cache input/output feedback");
                    d.textures[slot] = texture.id();
                    return texture;
                };
                if (kind == Kind::height) {
                    require(packet.key == 0x4c && draw.shader_binding == bindings.shader_bindings[0] &&
                                draw.selector == 0 && packet.pixel_count == 16 && packet.vertex_count == 0,
                            "Unsupported cache material shader/packet");
                    for (unsigned i = 0; i < 16; ++i) {
                        require(packet.pixel_samplers[i] == 0, "Cache material sampler must be original0");
                        bind(i);
                    }
                } else if (kind == Kind::normal) {
                    require(packet.key == 0x4a && draw.shader_binding == bindings.shader_bindings[1] &&
                                draw.selector == 0 && packet.pixel_count == 2 && packet.vertex_count == 0 &&
                                packet.pixel_samplers[0] == 5 && packet.pixel_samplers[1] == 5 &&
                                packet.pixel_textures[0] == packet.pixel_textures[1] && p.width == p.height,
                            "Unsupported cache normal shader/packet");
                    const auto &height = bind(0);
                    require(height.id() == prepared[0].targets[1].texture->id() &&
                                height.levels().front().width == static_cast<unsigned>(p.width) &&
                                height.levels().front().height == static_cast<unsigned>(p.height),
                            "Normal pass must consume its generated material height");
                    const auto expected = material_normal_parameters(p.width, plan.normal_constants);
                    require(expected == plan.normal_constants, "Unverified cache normal constants");
                } else if (kind == Kind::shadow) {
                    require(packet.key == 0x4c && draw.shader_binding == bindings.shader_bindings[0] &&
                                draw.selector == 3 && packet.pixel_count == 1 && packet.vertex_count == 0 &&
                                packet.pixel_samplers[0] == 0,
                            "Unsupported cache shadow shader/packet");
                    bind(0);
                    d.shadow = {frame.world_scales,
                                frame.sun_direction,
                                frame.height_texture_scale,
                                {lane(constants, 0x490), lane(constants, 0x494)},
                                {lane(constants, 0x4b0), lane(constants, 0x4b4), lane(constants, 0x4b8),
                                 lane(constants, 0x4bc)}};
                    d.shadow.validate();
                } else {
                    require(packet.key == 0x4b && draw.shader_binding == bindings.shader_bindings[2] &&
                                draw.selector == 0 && packet.pixel_count == 12 && packet.vertex_count == 1 &&
                                packet.vertex_samplers[0] == 1,
                            "Unsupported cache lighting shader/packet");
                    for (unsigned slot : {0u, 1u, 2u}) {
                        require(packet.pixel_samplers[slot] == 5, "Cache lighting clamp sampler");
                        bind(slot);
                    }
                    require(d.textures[0] == prepared[with_decals ? 2 : 1].targets[0].texture->id() &&
                                d.textures[1] == prepared[0].targets[0].texture->id() &&
                                d.textures[2] == prepared[with_decals ? 3 : 2].targets[0].texture->id(),
                            "Lighting must consume its generated normal/albedo/shadow");
                    require(packet.pixel_samplers[4] == 2 && packet.pixel_samplers[5] == 2 &&
                                packet.pixel_samplers[8] == 5,
                            "Cache lighting cube/LUT sampler");
                    d.sun = &bind(4, GL_TEXTURE_CUBE_MAP);
                    d.shade = &bind(5, GL_TEXTURE_CUBE_MAP);
                    for (const auto *cube : {d.sun, d.shade}) {
                        const auto format = cube->plan().internal_format;
                        require(format == 0x8c4d || format == 0x8c4e || format == 0x8c4f,
                                "Cache lighting requires authored sRGB BC cubes");
                        if (sampling != LightingCubeSampling::ordinary)
                            require(format == 0x8c4d && cube->levels().front().width == 128 &&
                                        cube->levels().size() == 48,
                                    "Measured cache lighting requires original128/8 BC1 cubes");
                    }
                    d.volume = volume(packet.pixel_textures[8]);
                    for (const auto &out : p.targets)
                        require(out.texture->id() != d.volume, "Cache volume/output feedback");
                }
                p.draws.push_back(d);
            }
            if (decal) {
                p.decals.selected = static_cast<std::uint32_t>(p.decals.draws.size());
                std::vector<GLuint> targets;
                for (const auto &target : p.targets) targets.push_back(target.texture->id());
                decals->renderer.validate(p.decals, [&](auto id, auto color) -> const GlTexture & {
                    const auto key = std::pair{id, color};
                    auto found = p.decal_textures.find(key);
                    if (found == p.decal_textures.end()) {
                        require(bool(decals->resolve), "Cache decals require a texture resolver");
                        found = p.decal_textures.emplace(key, &decals->resolve(id, color)).first;
                    }
                    return *found->second;
                }, *context, targets);
            }
            prepared.push_back(std::move(p));
        }
        context->check("Preflight complete material cache plan");
        return prepared;
    }
};
GlMaterialCacheRenderer::GlMaterialCacheRenderer(std::shared_ptr<GlContext> context,
                                                 const MaterialPatchMesh &material,
                                                 const LightingPatchMesh &lighting,
                                                 const MaterialCachePassSource &bindings,
                                                 LightingCubeSampling sampling) {
    require(bool(context), "Cache renderer requires owned context");
    require(material.columns() == lighting.columns() && material.rows() == lighting.rows() &&
                std::equal(material.indices().begin(), material.indices().end(), lighting.indices().begin(),
                           lighting.indices().end()),
            "Material and lighting must use the same patch lattice");
    impl_ = std::make_unique<Impl>(std::move(context), material, lighting, bindings, sampling);
}
GlMaterialCacheRenderer::~GlMaterialCacheRenderer() = default;
void GlMaterialCacheRenderer::rebuild_context(std::shared_ptr<GlContext> context) {
    require(context != nullptr, "Cache context recovery needs a live replacement context");
    require(context != impl_->context, "Cache context recovery requires a different OpenGL context");
    auto replacement = std::make_unique<Impl>(context, impl_->material, impl_->lighting_mesh, impl_->bindings,
                                               impl_->sampling);
    impl_.swap(replacement);
}
void GlMaterialCacheRenderer::execute(const MaterialCachePassPlan &plan,
                                      std::span<const GlMaterialCacheTarget> resources,
                                      std::span<const GlMaterialCacheVolume> volumes,
                                      const GlMaterialCacheFrame &frame, bool conditional_clear,
                                      const GlMaterialCacheDecals *decals) {
    auto &s = *impl_;
    s.context->api();
    s.context->check("Before complete cache plan preflight");
    const auto prepared = s.prepare(plan, resources, volumes, frame, decals);
    try {
        for (std::size_t i = 0; i < prepared.size(); ++i) {
            const auto &p = prepared[i];
            s.raster.begin(plan.passes[i], p.targets, conditional_clear);
            const auto kind = plan.passes[i].kind;
            if (kind == MaterialCachePassKind::decal || kind == MaterialCachePassKind::decal_shadow)
                decals->renderer.draw(p.decals, [&](auto id, auto color) -> const GlTexture & {
                    return *p.decal_textures.at({id, color});
                });
            for (const auto &draw : p.draws) {
                if (kind == MaterialCachePassKind::height)
                    s.blend.draw(frame.material_coefficients, draw.offset, draw.textures,
                                 MaterialSampling::ordinary0, {p.width, p.height});
                else if (kind == MaterialCachePassKind::normal)
                    s.normal.draw_cache(draw.textures[0], p.width);
                else if (kind == MaterialCachePassKind::shadow)
                    s.shadow.draw(draw.textures[0], draw.shadow, p.width, p.height, draw.offset);
                else
                    s.lighting.draw(draw.textures[0], draw.textures[1], draw.textures[2], 0, *draw.sun,
                                    *draw.shade, draw.volume, p.width, p.height, frame.half_vector);
            }
            s.raster.end();
            for (const auto &target : p.targets)
                target.texture->generate_mip_chain();
        }
    } catch (...) {
        s.raster.end();
        throw;
    }
}
} // namespace civ5::graphics
