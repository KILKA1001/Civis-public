#pragma once
#include "river_surface_probe.hpp"
namespace {
RiverWorldAssets load_river_world_assets(const fs::path &root) {
    const auto resource = [&](const char *path, const char *name) {
        const auto opened = civ5::assets::FpkSource::open(root / path, path);
        check(std::holds_alternative<std::shared_ptr<civ5::assets::FpkSource>>(opened),
              "River scene archive");
        const auto found =
            std::get<std::shared_ptr<civ5::assets::FpkSource>>(opened)->find(civ5::assets::AssetKey(name));
        check(std::holds_alternative<civ5::assets::AssetHandle>(found), "River scene authored resource");
        return std::get<civ5::assets::AssetHandle>(found).read_bytes();
    };
    using namespace civ5::assets;
    return {{LeanData::parse(resource("Resource/Common/TerrainModels.fpk", "riversmooth16.lean")),
             LeanData::parse(resource("Resource/Common/Upgrade1Models.fpk", "riverrough_new16.lean"))},
            {DdsData::parse(resource("Resource/DX9/Upgrade1Textures.fpk", "river_alpha_new.dds")),
             DdsData::parse(resource("Resource/DX9/Upgrade1Textures.fpk", "river_diffuse_new.dds")),
             DdsData::parse(resource("Resource/DX9/Upgrade1Textures.fpk", "waterdepthcolor_new.dds"))}};
}
struct RiverWorldScene {
    RiverWorldAssets assets;
    GlRiverWorldRenderer gl;
    std::unique_ptr<river_validation::NativeRiver> native;
    GlRiverSurfaceInputs inputs;
    std::array<IDirect3DBaseTexture9 *, 13> pointers{};
    std::vector<water_validation::Com<IDirect3DBaseTexture9>> textures;
    fs::path probes;
    RiverWorldScene(std::shared_ptr<GlContext> context, std::shared_ptr<const TerrainWorld> terrain,
                    const fs::path &root, const fs::path &shaders, const fs::path &reference,
                    const fs::path &output, IDirect3DDevice9 *device, const GlColorKeys &colors,
                    const color_key_validation::NativeColorKeys &native_colors, GLuint reflection,
                    GLuint overlay, const std::array<IDirect3DBaseTexture9 *, 14> &water_images)
        : assets(load_river_world_assets(root)), gl(context, terrain, assets),
          probes(shaders / "scene-probes") {
        fs::create_directory(output);
        native = std::make_unique<river_validation::NativeRiver>(shaders, output, device);
        blend_validation::text(output / "river-vs.glsl", river_surface_vertex_shader());
        blend_validation::text(output / "river-1-ps.glsl", river_surface_fragment_shader(1));
        blend_validation::text(output / "river-4-ps.glsl", river_surface_fragment_shader(4));
        for (unsigned slot = 0; slot < 4; ++slot) {
            auto texture = native->lean(assets.lean[slot / 2], slot % 2 ? civ5::assets::LeanMap::luminance16
                                                                        : civ5::assets::LeanMap::rgba16);
            textures.emplace_back(std::exchange(texture.value, nullptr));
            pointers[slot] = textures.back().value;
            const auto map = slot % 2 ? civ5::assets::LeanMap::luminance16 : civ5::assets::LeanMap::rgba16;
            for (const auto &level : assets.lean[slot / 2].levels(map)) {
                const auto raw = assets.lean[slot / 2].payload(map, level.mip);
                check(gl.texture(slot).read_level_raw(0, level.mip) == Bytes(raw.begin(), raw.end()),
                      "Connected river LEAN mip storage identity");
            }
        }
        for (unsigned i = 0; i < 3; ++i) {
            auto texture = native->upload(assets.maps[i], i == 0 ? D3DFMT_DXT5 : D3DFMT_A8R8G8B8);
            textures.push_back(std::move(texture));
            pointers[i == 2 ? 8 : 5 + i] = textures.back().value;
            for (const auto &level : assets.maps[i].levels()) {
                const auto raw = assets.maps[i].payload(0, level.mip);
                check(gl.texture(i == 2 ? 8 : 5 + i).read_level_raw(0, level.mip) ==
                          Bytes(raw.begin(), raw.end()),
                      "Connected river authored mip storage identity");
                D3DLOCKED_RECT lock{};
                auto *image = static_cast<IDirect3DTexture9 *>(textures.back().value);
                hr(image->LockRect(level.mip, &lock, nullptr, D3DLOCK_READONLY), "River scene DDS readback");
                bool valid = lock.pBits && lock.Pitch >= int(level.row_pitch);
                if (valid)
                    for (std::size_t y = 0; y < level.length / level.row_pitch; ++y)
                        valid = valid && std::memcmp(static_cast<char *>(lock.pBits) + y * lock.Pitch,
                                                     raw.data() + y * level.row_pitch, level.row_pitch) == 0;
                hr(image->UnlockRect(level.mip), "River scene DDS unlock");
                check(valid, "Connected original river DDS mip storage identity");
            }
        }
        save(output / "riversmooth16.lean", assets.lean[0].original_bytes());
        save(output / "riverrough_new16.lean", assets.lean[1].original_bytes());
        for (unsigned i = 0; i < 3; ++i)
            save(output /
                     std::array{"river_alpha_new.dds", "river_diffuse_new.dds", "waterdepthcolor_new.dds"}[i],
                 assets.maps[i].original_bytes());
        inputs.pixel[4] = reflection;
        inputs.pixel[9] = overlay;
        inputs.pixel[11] = colors.at(ColorKeySystem::terrain_fow, 0).id();
        inputs.pixel[12] = colors.at(ColorKeySystem::rivers, 0).id();
        pointers[4] = water_images[4];
        pointers[9] = water_images[9];
        pointers[11] = native_colors.at(1, 0);
        pointers[12] = native_colors.at(4, 0);
        save(output / "terrain-fow-color-key.bgra",
             colors.at(ColorKeySystem::terrain_fow, 0).read_mip_raw(0));
        save(output / "river-color-key.bgra", colors.at(ColorKeySystem::rivers, 0).read_mip_raw(0));
        const auto vertices = blend_validation::read(reference / "vertices.bin");
        const auto indices = blend_validation::read(reference / "indices.bin");
        check(vertices.size() % 32 == 0 && indices.size() % 4 == 0, "Original river scene geometry framing");
        RiverSurfaceMesh original;
        original.vertices.resize(vertices.size() / 32);
        original.indices.resize(indices.size() / 4);
        std::memcpy(original.vertices.data(), vertices.data(), vertices.size());
        std::memcpy(original.indices.data(), indices.data(), indices.size());
        native->mesh(original);
        save(output / "vertices-d3d9.bin", vertices);
        save(output / "indices-d3d9.bin", indices);
        water_validation::save(output / "vertices-gl.bin", gl.mesh().vertices);
        water_validation::save(output / "indices-gl.bin", gl.mesh().indices);
        for (auto &vertex : original.vertices)
            vertex[7] = 0; // Original declaration includes unconsumed COLOR0.
        check(original.vertices == gl.mesh().vertices && original.indices == gl.mesh().indices,
              "River surface from this world differs from independent original generation");
        blend_validation::text(
            output / "scope.txt",
            "Own world graph/curves/heights -> surface; independent original surface -> D3D9.\n"
            "Original authored river maps and Rivers/Generic + TerrainFOW/Generic ColorKeys.\n"
            "Shared camera/overlay/water constants and each API's own reflection/FOW/depth targets.\n");
    }
    void draw_gl(const WaterConstantBlock &constants, GLuint mask) {
        inputs.pixel[10] = mask;
        gl.draw(constants, inputs, 1280, 960);
    }
    void draw_native(const WaterConstantBlock &constants, IDirect3DTexture9 *mask) {
        pointers[10] = mask;
        native->bind(4, constants, pointers);
        native->draw();
    }
    void diagnose(std::shared_ptr<GlContext> context, SceneTarget &target, NativeSceneTarget &native_target,
                  const std::vector<WaterConstantBlock> &cpp, const std::vector<WaterConstantBlock> &original,
                  GLuint mask, IDirect3DTexture9 *native_mask, const fs::path &output) {
        if (!fs::is_directory(probes))
            return;
        bool selected = output.filename() == "60" || fs::exists(probes / "all-frames");
        if (fs::is_regular_file(probes / "steps.u32")) {
            const auto bytes = blend_validation::read(probes / "steps.u32");
            check(!bytes.empty() && bytes.size() % 4 == 0 && bytes.size() <= 323 * 4,
                  "Bounded river diagnostic steps");
            selected = false;
            for (std::size_t at = 0; at < bytes.size(); at += 4) {
                std::uint32_t step;
                std::memcpy(&step, bytes.data() + at, 4);
                check(step < 323, "River diagnostic step range");
                selected = selected || output.filename() == std::to_string(step);
            }
        }
        if (!selected)
            return;
        const auto directory = output / "river-probes";
        fs::create_directory(directory);
        std::vector<fs::path> files;
        for (const auto &entry : fs::directory_iterator(probes))
            if (entry.path().extension() == ".bin")
                files.push_back(entry.path());
        std::sort(files.begin(), files.end());
        check(!files.empty() && files.size() <= 32, "Bounded river scene observations");
        for (const auto &path : files) {
            auto source = path;
            source.replace_extension(".glsl");
            const auto code = blend_validation::read(path), text = blend_validation::read(source);
            check(code.size() % 4 == 0, "River scene diagnostic bytecode alignment");
            std::vector<DWORD> words(code.size() / 4);
            std::memcpy(words.data(), code.data(), code.size());
            Com<IDirect3DPixelShader9> shader;
            hr(native->device->CreatePixelShader(words.data(), &shader.value),
               "River scene observation shader");
            RiverProbe program(*context, gl.mesh(),
                               {reinterpret_cast<const char *>(text.data()), text.size()});
            native_target.begin();
            std::unique_ptr<WaterPrecisionTarget> observed;
            if (path.stem().string().ends_with("-mrt"))
                observed = std::make_unique<WaterPrecisionTarget>(*context, target.gl, target.fbo,
                                                                  native->device.value, 1280, 960);
            target.begin();
            for (const auto &c : cpp)
                draw_gl(c, mask); // Establish the complete original state and authored inputs.
            const float zero[4]{}, one = 1;
            target.gl.ClearNamedFramebufferfv(target.fbo, GL_COLOR, 0, zero);
            target.gl.ClearNamedFramebufferfv(target.fbo, GL_DEPTH, 0, &one);
            if (observed)
                observed->clear();
            target.gl.Disable(GL_BLEND);
            for (const auto &c : cpp)
                program.draw(c, 1280, 960);
            blend_validation::write(directory / (path.stem().string() + "-gl.f32"), target.pixels());
            if (observed)
                blend_validation::write(directory / (path.stem().string() + "-observed-gl.f32"),
                                        observed->gl_output());
            native_target.begin();
            hr(native->device->BeginScene(), "Original river scene observation begin");
            pointers[10] = native_mask;
            for (const auto &c : original) {
                native->bind(4, c, pointers);
                hr(native->device->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE),
                   "River scene observation blend");
                hr(native->device->SetPixelShader(shader.value), "River scene observation bind");
                native->draw();
            }
            hr(native->device->EndScene(), "Original river scene observation end");
            blend_validation::write(directory / (path.stem().string() + "-d3d9.f32"), native_target.pixels());
            if (observed)
                blend_validation::write(directory / (path.stem().string() + "-observed-d3d9.f32"),
                                        observed->native_output());
            save(directory / path.filename(), code);
            save(directory / source.filename(), text);
        }
    }
};
} // namespace
