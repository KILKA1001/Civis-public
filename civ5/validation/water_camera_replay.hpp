#pragma once
// Diagnostic only: reconstruct source fields from primitives, then replay each
// API's OWN previously generated cache bases. No production cache preparation
// accepts these files. The canonical full scene still regenerates all caches.
namespace {
struct CameraReplayLand {
    std::shared_ptr<GlContext> context;
    std::shared_ptr<const MaterialWorld> world;
    GlMaterialWorldRenderer source;
    std::vector<GlTexture> textures;
    std::map<std::pair<int, int>, std::unique_ptr<GlLandMaterialRenderer>> renderers;
    LandMipSampling mip_sampling;
    CameraReplayLand(std::shared_ptr<GlContext> c, std::shared_ptr<const MaterialWorld> w,
                     const LightingCubeAssets &cubes, const fs::path &previous,
                     std::shared_ptr<const GlColorKeys> color_keys = {},
                     LandMipSampling mip_sampling = LandMipSampling::d3d9_trilinear)
        : context(c), world(w),
          source(c, w, cubes, {false, true, true}, LightingCubeSampling::ordinary,
                 LandShadowArithmetic::instruction_order, LandMipSampling::ordinary, std::move(color_keys)),
          mip_sampling(mip_sampling) {
        const auto bytes = input_bytes(previous / "0/scene-cache-bases-gl.bin");
        WorldReader reader{bytes};
        for (std::size_t index = 0; index < world->cache().patches.size(); ++index) {
            const auto base = reader.blob();
            check(base.size() == 8192, "Camera replay GL cache extent");
            auto texture = GlTexture::render_target(*context, 64, 64, 7, RenderTextureFormat::rgb565);
            context->api().TextureSubImage2D(texture.id(), 0, 0, 0, 64, 64, texture.plan().transfer_format,
                                             texture.plan().transfer_type, base.data());
            const auto actual = texture.read_mip_raw(0);
            check(std::equal(actual.begin(), actual.end(), base.begin(), base.end()),
                  "Camera replay GL cache transfer");
            texture.generate_mip_chain();
            textures.push_back(std::move(texture));
        }
        reader.end();
    }
    const GlTexture &height_texture(std::size_t index) const { return source.height_texture(index); }
    LandMaterialConstants land_constants(std::size_t index, const LandMaterialConstants &frame) const {
        return source.land_constants(index, frame);
    }
    void draw(std::size_t index, const LandMaterialConstants &frame, GlLandMaterialInputs images, int width,
              int height) {
        const auto patch =
            world->terrain().land().patch(index / world->cache().rows, index % world->cache().rows);
        const auto key = std::pair{patch.source_samples_x(), patch.source_samples_y()};
        if (!renderers.contains(key)) {
            const LandGrid grid(key.first, key.second, 1);
            const LandMaterialMesh mesh(LandVertexInput::local_grid, grid.vertices(), grid.indices());
            renderers.emplace(
                key, std::make_unique<GlLandMaterialRenderer>(
                         context, mesh, 1, LandShadowArithmetic::measured_fma_t_x_z_y, mip_sampling));
        }
        images.pixel[0] = textures.at(index).id();
        images.height = height_texture(index).id();
        images = source.color_key_inputs(index, images);
        renderers.at(key)->draw(land_constants(index, frame), images, width, height);
    }
};

void camera_replay_native_caches(NativeWorld &native, const MaterialWorld &world, const fs::path &previous) {
    const auto field = world.terrain().height_field();
    for (int x = 0; x < field.domain().texture_columns(); ++x)
        for (int y = 0; y < field.domain().texture_rows(); ++y)
            native.heights.emplace(x + 16 * y, native.land.height(field.extract_a8_texture(x, y), 1024));
    const auto bytes = input_bytes(previous / "0/scene-cache-bases-d3d9.bin");
    WorldReader reader{bytes};
    for (std::size_t index = 0; index < world.cache().patches.size(); ++index) {
        const auto base = reader.blob();
        check(base.size() == 8192, "Camera replay native cache extent");
        Com<IDirect3DTexture9> upload, texture;
        auto *device = native.cache.device.value;
        hr(device->CreateTexture(64, 64, 1, 0, D3DFMT_R5G6B5, D3DPOOL_SYSTEMMEM, &upload.value, nullptr),
           "Camera replay native staging");
        D3DLOCKED_RECT lock{};
        hr(upload->LockRect(0, &lock, nullptr, 0), "Camera replay cache lock");
        check(lock.Pitch >= 128, "Camera replay cache row extent");
        for (unsigned y = 0; y < 64; ++y)
            std::memcpy(static_cast<char *>(lock.pBits) + std::size_t(y) * lock.Pitch, base.data() + y * 128,
                        128);
        hr(upload->UnlockRect(0), "Camera replay cache unlock");
        hr(device->CreateTexture(64, 64, 0, D3DUSAGE_RENDERTARGET | D3DUSAGE_AUTOGENMIPMAP, D3DFMT_R5G6B5,
                                 D3DPOOL_DEFAULT, &texture.value, nullptr),
           "Camera replay native final texture");
        Com<IDirect3DSurface9> from, to;
        hr(upload->GetSurfaceLevel(0, &from.value), "Camera replay staging surface");
        hr(texture->GetSurfaceLevel(0, &to.value), "Camera replay final surface");
        hr(device->UpdateSurface(from.value, nullptr, to.value, nullptr), "Camera replay cache transfer");
        texture->GenerateMipSubLevels();
        native.finals.emplace(static_cast<unsigned>(index), std::move(texture));
        const auto actual = native.final_pixels(index);
        check(std::equal(actual.begin(), actual.end(), base.begin(), base.end()),
              "Camera replay native cache identity");
    }
    reader.end();
}
} // namespace
