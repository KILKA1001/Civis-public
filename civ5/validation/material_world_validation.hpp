#pragma once
// Shared validation fixtures and original D3D9 world controls.
#include "civ5/graphics/gl_material_world.hpp"
#include "civ5/graphics/gl_material_blend.hpp"
#include "civ5/graphics/gl_distance_fog.hpp"
#include "civ5/graphics/gl_fog_of_war_map.hpp"
#include "civ5/graphics/gl_render_api.hpp"
#include "civ5/graphics/gl_water_world.hpp"
#include "civ5/graphics/gl_river_world.hpp"
#include "river_surface_native.hpp"
#include "civ5/materials/water_animation.hpp"
#include "civ5/materials/water_sky.hpp"
#include "color_key_assets.hpp"
#include "color_key_native.hpp"
#include "distance_fog_native.hpp"
#include "fog_of_war_native.hpp"
#include "land_material_native.hpp"
#include "material_cache_chain_native.hpp"
#include "material_mip_native.hpp"
#include "material_world_input.hpp"
#include "water_surface_native.hpp"
#include <iostream>
#include <map>
#include <sstream>
using namespace cache_chain_validation;
using namespace land_material_validation;
using namespace civ5::graphics;
#include "land_material_validation_inputs.hpp"
#include "land_material_probe.hpp"
#include "water_surface_probe.hpp"
using water_validation::checked;
#include "water_precision_target.hpp"
#include "material_cache_chain_io.hpp"

namespace {
using WorldReader = Reader;
struct PrimitiveInput {
    civ5::assets::AssetKey piece_key, coast_key;
    std::vector<WorldRequest> worlds;
    std::vector<MaterialRequest> materials;
};
struct ObservedFinalMaterialFields {
    std::vector<std::uint16_t> heights;
    std::vector<std::uint8_t> materials;
};
PrimitiveInput primitive_input(const fs::path &path) {
    const auto bytes = input_bytes(path);
    Reader r{bytes};
    const auto magic = r.take(8);
    check(std::string(magic.begin(), magic.end()) == "C5MWIN01", "Wrong world GPU primitive schema");
    Reader wr{r.blob()};
    const auto world_magic = wr.take(8);
    check(std::string(world_magic.begin(), world_magic.end()) == "C5TWLD01", "Wrong embedded world schema");
    const auto count = wr.word();
    check(count == 2, "Two independently controlled material worlds required");
    PrimitiveInput input{civ5::assets::AssetKey(key(wr)), civ5::assets::AssetKey(key(wr)), {}, {}};
    for (unsigned i = 0; i < count; ++i) {
        Reader source{wr.blob()};
        input.worlds.push_back(request(source));
        check(input.worlds.back().keys.empty(), "World GPU input must not contain an expected palette");
    }
    wr.end();
    check(r.word() == count, "World GPU material request count differs");
    for (unsigned i = 0; i < count; ++i) {
        Reader source{r.blob()};
        input.materials.push_back(material_request(source));
    }
    r.end();
    return input;
}
std::shared_ptr<const MaterialWorld> build_world(const fs::path &root, PrimitiveInput &input,
                                                 unsigned index,
                                                 const TerrainMarshStamp *marsh_stamp = nullptr,
                                                 std::span<const TerrainFeature2Source> marsh_sources = {},
                                                 const TerrainMarshWriteObserver &marsh_write = {},
                                                 bool saved_river_edges = false,
                                                 bool marsh_before_coast = false,
                                                 bool low_texture_quality = false,
                                                 const ObservedFinalMaterialFields *observed_final = nullptr) {
    auto &q = input.worlds.at(index);
    auto &m = input.materials.at(index);
    q.input.cooked_to_primary = q.mapping;
    q.input.expanded_logical_cells = q.cells;
    q.input.coast_palette = {};
    check(q.input.map_width == m.map_width && q.input.map_height == m.map_height && q.input.mode == m.mode,
          "Material GPU source map identity");
    const civ5::assets::LooseSource loose(root);
    const auto pieces = TerrainPieceLibrary::load(loose, input.piece_key);
    const civ5::assets::AssetEnvironment environment(root);
    const auto models = archive(environment, "Resource/Common/TerrainModels.fpk");
    const auto textures = archive(environment, "Resource/DX9/TerrainTextures.fpk");
    const auto low_textures = low_texture_quality
        ? archive(environment, "Resource/DX9_Low/TerrainTexturesDX9Low.fpk")
        : nullptr;
    const auto coasts = CoastLibrary::load(*models, input.coast_key, X87Precision::double_precision);
    const auto definition = models->find(civ5::assets::AssetKey("TerrainDef.csv"));
    check(std::holds_alternative<civ5::assets::AssetHandle>(definition), "World GPU provider unavailable");
    auto assets =
        MaterialAssets::load_terrain_provider(std::get<civ5::assets::AssetHandle>(definition),
                                              *textures, low_textures.get());
    const LandStyleGrid prior(q.prior);
    unsigned reciprocal_calls = 0;
    check(!marsh_before_coast || marsh_stamp, "Loading marsh order requires a stamp");
    std::optional<MaterialWorldLoadingMarsh> loading_marsh;
    if (marsh_before_coast)
        loading_marsh.emplace(MaterialWorldLoadingMarsh{marsh_sources, marsh_stamp,
            X87Precision::double_precision, Vec4Path::sse, marsh_write});
    auto world = MaterialWorld::build(
        pieces, coasts, std::move(assets), q.input,
        {prior, q.masks, nullptr, nullptr, [&](int n) { return Bytes(n, std::uint8_t(q.fill)); }},
        [&](float determinant) {
            check(reciprocal_calls < 2 &&
                      std::bit_cast<std::uint32_t>(determinant) == q.reciprocal[reciprocal_calls * 2],
                  "World GPU reciprocal input differs");
            return std::bit_cast<float>(q.reciprocal[reciprocal_calls++ * 2 + 1]);
        },
        [&](unsigned, std::span<const CoastDispatchJob> phase) {
            std::vector<std::size_t> order(phase.size());
            std::iota(order.begin(), order.end(), 0);
            if (q.order)
                std::reverse(order.begin(), order.end());
            return order;
        }, {}, loading_marsh ? &*loading_marsh : nullptr);
    check(reciprocal_calls == 2, "World GPU reciprocal inputs unused");
    // The fixture's RSQRT transcript belongs to its own river topology.
    // Saved edges use the host instruction; m.seed is still an explicit
    // controlled entering cursor, not a recovered saved loading cursor.
    auto complete = std::move(world).with_rivers(
        m.edges, m.seed, [&](float squared) {
            return saved_river_edges ? _mm_cvtss_f32(_mm_rsqrt_ss(_mm_set_ss(squared)))
                                     : m.rivers.take(squared);
        }, m.precision);
    if (!saved_river_edges) m.rivers.end();
    if (marsh_stamp && !marsh_before_coast)
        complete = std::move(complete).with_marshes(marsh_sources, *marsh_stamp,
            X87Precision::double_precision, Vec4Path::sse, marsh_write);
    // The observed R is already after original coast and river writes. The
    // controlled build above supplies non-field descriptors only; overwrite
    // its final pages once, after every controlled terrain mutation.
    if (observed_final)
        complete = std::move(complete).with_observed_final_fields(
            observed_final->heights, observed_final->materials);
    return std::make_shared<const MaterialWorld>(std::move(complete));
}
FloatImage identity_volume() {
    FloatImage f{16, true, std::vector<float>(16 * 16 * 16 * 4)};
    for (unsigned z = 0; z < 16; ++z)
        for (unsigned y = 0; y < 16; ++y)
            for (unsigned x = 0; x < 16; ++x) {
                const auto at = ((z * 16 + y) * 16 + x) * 4;
                f.values[at] = x / 15.f;
                f.values[at + 1] = y / 15.f;
                f.values[at + 2] = z / 15.f;
                f.values[at + 3] = 1;
            }
    return f;
}
std::array<FloatImage, 10> neutral_scene() {
    std::array<FloatImage, 10> result;
    for (unsigned i = 0; i < 10; ++i) {
        if (i == 6 || i == 7) {
            result[i] = identity_volume();
            continue;
        }
        result[i] = {1, false, std::vector<float>(4)};
        if (i == 2)
            result[i].values = {1, 1, 1, 1};
        if (i == 4)
            result[i].values = {1, 0, 1, 1};
    }
    return result;
}
LandMaterialConstants patch_frame(const TerrainWorld &world, std::size_t index, unsigned variant) {
    const auto &layout = world.land();
    const auto patch =
        layout.patch(static_cast<int>(index / layout.rows()), static_cast<int>(index % layout.rows()));
    auto c = constants(patch.source_samples_x(), patch.source_samples_y(), variant);
    const auto bounds = patch.culling_bounds();
    const float sx = 2.f / (patch.source_samples_x() - 1),
                sy = -2.f / ((patch.source_samples_y() - 1) * .866f);
    c.view_projection = {
        {{sx, 0, 0, 0}, {0, sy, 0, 0}, {0, 0, .01f, 0}, {-1 - bounds[0] * sx, 1 - bounds[1] * sy, 0, 1}}};
    c.camera = {bounds[0] + 4, bounds[1] - 2, 110, 1};
    return c;
}
LandMaterialConstants overview_frame() {
    LandMaterialConstants c;
    c.view_projection = {{{1.f / 1408, 0, 0, 0},
                          {0, .75f / 768, .75f / 3072, 0},
                          {0, .75f / 768, -.75f / 3072, 0},
                          {0, 0, .5f, 1}}};
    c.composite_projection = identity();
    c.shadow_projection = identity();
    c.camera = {0, -1500, 1500, 1};
    c.apply_diffuse = 1;
    return c;
}
Bytes frame_bytes(const LandMaterialConstants &c) {
    Writer out;
    for (auto p : {&c.view_projection, &c.composite_projection, &c.world, &c.shadow_projection})
        for (const auto &row : *p)
            for (auto f : row)
                out.scalar(f);
    for (auto p :
         {&c.fow_scale, &c.world_scales, &c.height_texture_scale, &c.offset, &c.camera, &c.fog_color, &c.fog})
        for (auto f : *p)
            out.scalar(f);
    out.scalar(c.shadow_fade);
    out.scalar(c.apply_specular);
    out.scalar(c.apply_diffuse);
    return out.bytes;
}
struct SceneTarget {
    std::shared_ptr<GlContext> context;
    GlRenderApi gl;
    std::shared_ptr<GlTexture> color_storage, depth_storage;
    GLuint color = 0, depth = 0, fbo = 0;
    unsigned width, height;
    SceneTarget(std::shared_ptr<GlContext> c, unsigned w, unsigned h)
        : context(std::move(c)), gl(*context), width(w), height(h) {
        color_storage = std::make_unique<GlTexture>(
            GlTexture::render_target(*context, width, height, 1, RenderTextureFormat::rgba32_float));
        depth_storage = std::make_unique<GlTexture>(
            GlTexture::render_target(*context, width, height, 1, RenderTextureFormat::depth24_stencil8));
        color = color_storage->id();
        depth = depth_storage->id();
        gl.CreateFramebuffers(1, &fbo);
        gl.NamedFramebufferTexture(fbo, GL_COLOR_ATTACHMENT0, color, 0);
        gl.NamedFramebufferTexture(fbo, GL_DEPTH_STENCIL_ATTACHMENT, depth, 0);
        const GLenum output = GL_COLOR_ATTACHMENT0;
        gl.NamedFramebufferDrawBuffers(fbo, 1, &output);
        require(gl.CheckNamedFramebufferStatus(fbo, GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE,
                "World scene target");
    }
    SceneTarget(std::shared_ptr<GlContext> c,
        std::pair<std::shared_ptr<GlTexture>,std::shared_ptr<GlTexture>> actual)
        :context(std::move(c)),gl(*context),color_storage(std::move(actual.first)),depth_storage(std::move(actual.second)),
         width(color_storage?color_storage->levels().at(0).width:0),height(color_storage?color_storage->levels().at(0).height:0){
        require(color_storage&&depth_storage&&width&&height&&color_storage->belongs_to(*context)&&depth_storage->belongs_to(*context)&&
            color_storage->id()!=depth_storage->id()&&depth_storage->levels().at(0).width==width&&depth_storage->levels().at(0).height==height,
            "Actual Camera target source/context/extent differs");
        color=color_storage->id();depth=depth_storage->id();gl.CreateFramebuffers(1,&fbo);
        gl.NamedFramebufferTexture(fbo,GL_COLOR_ATTACHMENT0,color,0);gl.NamedFramebufferTexture(fbo,GL_DEPTH_STENCIL_ATTACHMENT,depth,0);
        const GLenum output=GL_COLOR_ATTACHMENT0;gl.NamedFramebufferDrawBuffers(fbo,1,&output);
        require(gl.CheckNamedFramebufferStatus(fbo,GL_FRAMEBUFFER)==GL_FRAMEBUFFER_COMPLETE,"Actual Camera framebuffer incomplete");
    }
    SceneTarget(const SceneTarget&)=delete;SceneTarget& operator=(const SceneTarget&)=delete;
    ~SceneTarget() {
        context->api();
        gl.BindFramebuffer(GL_FRAMEBUFFER, 0);
        gl.DeleteFramebuffers(1, &fbo);
    }
    void begin() {
        context->api();
        gl.BindFramebuffer(GL_FRAMEBUFFER, fbo);
        gl.Viewport(0, 0, width, height);
        for (auto flag : {GL_CULL_FACE, GL_BLEND, GL_SCISSOR_TEST, GL_DITHER, GL_FRAMEBUFFER_SRGB,
                          GL_RASTERIZER_DISCARD, GL_STENCIL_TEST})
            gl.Disable(flag);
        gl.Enable(GL_DEPTH_TEST);
        gl.DepthMask(GL_TRUE);
        gl.DepthFunc(GL_LEQUAL);
        gl.ColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
        const float zero[4]{}, one = 1;
        gl.ClearNamedFramebufferfv(fbo, GL_COLOR, 0, zero);
        gl.ClearNamedFramebufferfv(fbo, GL_DEPTH, 0, &one);
    }
    std::vector<float> pixels() {
        std::vector<float> values(std::size_t(width) * height * 4);
        context->api().GetTextureImage(color, 0, GL_RGBA, GL_FLOAT, static_cast<GLsizei>(values.size() * 4),
                                       values.data());
        context->check("World scene readback");
        return values;
    }
};
struct NativeWorld {
    NativeCacheChain cache;
    NativeLand land;
    std::map<unsigned, Com<IDirect3DTexture9>> heights, finals;
    std::array<Com<IDirect3DBaseTexture9>, 10> neutral, varied;
    std::uint32_t height_id, final_id;
    std::unique_ptr<color_key_validation::NativeColorKeys> color_keys;
    std::unique_ptr<distance_fog_validation::NativeDistanceFog> distance_fog;
    std::vector<unsigned> color_styles;
    std::vector<float> original_terrain_frame;
    NativeWorld(const std::array<fs::path, 4> &shaders, const fs::path &land_shaders,
                const MaterialAssets &assets, const LightingCubeAssets &cubes, unsigned size,
                std::span<const std::uint8_t> geometry, std::span<const std::uint8_t> height,
                const MaterialCachePassSource &source, const fs::path &color_key_reference = {},
                std::span<const std::uint32_t> styles = {}, const fs::path &terrain_reference = {})
        : cache(shaders, assets, cubes, identity_volume().values, size, geometry, height, source,
                source.textures[0] + 13, source.textures[0] + 14, source.textures[0] + 30),
          land(land_shaders, cache.device.value), height_id(source.textures[0] + 13),
          final_id(source.textures[0] + 14) {
        if (!terrain_reference.empty()) {
            const auto bytes = blend_validation::read(terrain_reference / "frame.bin");
            check(bytes.size() == 592, "Original terrain frame reference extent");
            original_terrain_frame.resize(148);
            std::memcpy(original_terrain_frame.data(), bytes.data(), bytes.size());
        }
        auto a = neutral_scene(), b = fixtures();
        for (unsigned i = 1; i < 10; ++i) {
            auto first = land.image(a[i]), second = land.image(b[i]);
            neutral[i].value = std::exchange(first.value, nullptr);
            varied[i].value = std::exchange(second.value, nullptr);
        }
        if (!color_key_reference.empty()) {
            check(!styles.empty(), "Original ColorKey LAND source styles required");
            for (auto mask : styles)
                color_styles.push_back(material_lighting_style(mask));
            color_keys = std::make_unique<color_key_validation::NativeColorKeys>(cache.device.value,
                                                                                 color_key_reference);
            for (unsigned style = 0; style < 5; ++style)
                cache.resources.at(source.lighting.color_correct[style]) = color_keys->at(0, style);
        }
    }
    void execute(std::size_t index, const GlMaterialWorldPrepared &job, std::span<const std::uint8_t> a8) {
        execute(index, job, a8, cache.size);
    }
    void execute(std::size_t index, const GlMaterialWorldPrepared &job, std::span<const std::uint8_t> a8,
                 unsigned final_size) {
        auto h = heights.find(job.height_table_slot);
        if (h == heights.end())
            h = heights.emplace(job.height_table_slot, cache.shadows.texture(1024, a8, true)).first;
        cache.resources[height_id] = h->second.value;
        Com<IDirect3DTexture9> target;
        hr(cache.device->CreateTexture(final_size, final_size, 0,
                                       D3DUSAGE_RENDERTARGET | D3DUSAGE_AUTOGENMIPMAP, D3DFMT_R5G6B5,
                                       D3DPOOL_DEFAULT, &target.value, nullptr),
           "Native world final texture");
        cache.resources[final_id] = target.value;
        const auto material = job.patch.geometry.mesh();
        const LightingPatchMesh normal(material.columns(), material.rows(),
                                       job.patch.geometry.streams.normal24, material.indices());
        if (!original_terrain_frame.empty()) {
            check(std::equal(job.frame.world_scales.begin(), job.frame.world_scales.end(),
                             original_terrain_frame.begin() + 64) &&
                      std::equal(job.frame.sun_direction.begin(), job.frame.sun_direction.end(),
                                 original_terrain_frame.begin() + 68) &&
                      std::equal(job.frame.half_vector.begin(), job.frame.half_vector.end(),
                                 original_terrain_frame.begin() + 84) &&
                      std::equal(job.frame.height_texture_scale.begin(), job.frame.height_texture_scale.end(),
                                 original_terrain_frame.begin() + 88),
                  "C++ terrain lighting differs from independently executed original frame");
        }
        cache.execute(job.plan, material, normal, job.frame, true, nullptr, original_terrain_frame);
        finals.erase(static_cast<unsigned>(index));
        finals.emplace(static_cast<unsigned>(index), std::move(target));
    }
    std::array<IDirect3DBaseTexture9 *, 10> images(std::size_t index, bool varied_inputs) {
        std::array<IDirect3DBaseTexture9 *, 10> pointers{};
        pointers[0] = finals.at(static_cast<unsigned>(index)).value;
        for (unsigned i = 1; i < 10; ++i)
            pointers[i] = (varied_inputs ? varied : neutral)[i].value;
        if (color_keys) {
            const auto style = color_styles.at(index);
            pointers[6] = color_keys->at(1, style);
            pointers[7] = color_keys->at(2, style);
        }
        if (distance_fog)
            pointers[2] = distance_fog->ramp.value;
        return pointers;
    }
    Bytes final_pixels(std::size_t index) {
        Com<IDirect3DSurface9> surface;
        hr(finals.at(static_cast<unsigned>(index))->GetSurfaceLevel(0, &surface.value),
           "Native world final readback surface");
        return cache.surface_pixels(surface.value);
    }
};
struct NativeSceneTarget {
    NativeWorld &native;
    Com<IDirect3DSurface9> color, depth;
    unsigned width, height;
    NativeSceneTarget(NativeWorld &n, unsigned w, unsigned h) : native(n), width(w), height(h) {
        hr(n.cache.device->CreateRenderTarget(width, height, D3DFMT_A32B32G32R32F, D3DMULTISAMPLE_NONE, 0,
                                              FALSE, &color.value, nullptr),
           "Native world scene color");
        hr(n.cache.device->CreateDepthStencilSurface(width, height, D3DFMT_D24S8, D3DMULTISAMPLE_NONE, 0,
                                                     FALSE, &depth.value, nullptr),
           "Native world scene depth24");
    }
    void begin() {
        auto *device = native.cache.device.value;
        native.land.begin_target();
        hr(device->SetRenderTarget(0, color.value), "Native world scene target");
        hr(device->SetDepthStencilSurface(depth.value), "Native world scene depth");
        const D3DVIEWPORT9 viewport{0, 0, width, height, 0, 1};
        hr(device->SetViewport(&viewport), "Native world scene viewport");
        hr(device->SetRenderState(D3DRS_ZENABLE, TRUE), "Native world scene depth enabled");
        hr(device->SetRenderState(D3DRS_ZWRITEENABLE, TRUE), "Native world scene depth writes");
        hr(device->SetRenderState(D3DRS_ZFUNC, D3DCMP_LESSEQUAL), "Native world scene depth function");
        hr(device->SetRenderState(D3DRS_STENCILENABLE, FALSE), "Native world scene stencil disabled");
        hr(device->Clear(0, nullptr, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, 0, 1, 0),
           "Native world scene clear");
    }
    Bytes pixels() { return native.cache.surface_pixels(color.value); }
};
void save_edges(Writer &out, std::size_t index, int columns, int rows, std::span<const float> values) {
    check(values.size() == std::size_t(columns) * rows * 21, "World LAND feedback extent");
    out.word(static_cast<std::uint32_t>(index));
    out.word(columns);
    out.word(rows);
    for (int side = 0; side < 4; ++side) {
        const int count = side < 2 ? rows : columns;
        out.word(count);
        for (int i = 0; i < count; ++i) {
            const int v = side == 0   ? i * columns
                          : side == 1 ? i * columns + columns - 1
                          : side == 2 ? i
                                      : (rows - 1) * columns + i;
            for (auto f : values.subspan(v * 21, 21))
                out.scalar(f);
        }
    }
}
void draw_ordinary_mip_control(std::shared_ptr<GlContext> context, const MaterialWorld &world,
                               GlMaterialWorldRenderer &owner, const LandMaterialConstants &frame,
                               GlLandMaterialInputs images) {
    std::map<std::pair<int, int>, std::unique_ptr<GlLandMaterialRenderer>> renderers;
    for (std::size_t index = 0; index < world.cache().patches.size(); ++index) {
        const auto p = world.terrain().land().patch(index / world.cache().rows, index % world.cache().rows);
        const auto key = std::pair{p.source_samples_x(), p.source_samples_y()};
        if (!renderers.contains(key)) {
            const LandGrid grid(key.first, key.second, 1);
            const LandMaterialMesh mesh(LandVertexInput::local_grid, grid.vertices(), grid.indices());
            renderers.emplace(key, std::make_unique<GlLandMaterialRenderer>(
                                       context, mesh, 1, LandShadowArithmetic::measured_fma_t_x_z_y,
                                       LandMipSampling::ordinary));
        }
        images.pixel[0] = owner.final_texture(index).id();
        images.height = owner.height_texture(index).id();
        images = owner.color_key_inputs(index, images);
        renderers.at(key)->draw(owner.land_constants(index, frame), images, 1280, 960);
    }
}
} // namespace
