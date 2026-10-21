#pragma once
#include "civ5/graphics/gl_water_current_map.hpp"
#include "civ5/materials/material_sources.hpp"
#include "water_current_native.hpp"

namespace {
// Independent API producers. Their own render-target textures are sampled
// directly by their ocean pass; readback exists solely for offline comparison.
struct WaterCurrentScene {
    std::shared_ptr<GlContext> context;
    water_current_validation::NativeCurrent native;
    WaterCurrentScene(std::shared_ptr<GlContext> c, const GlWaterCurrentMap &map, IDirect3DDevice9 *device,
                  const fs::path &shaders, const fs::path &fixture, const fs::path &folder)
        : context(std::move(c)), native(shaders, fixture, device) {
        const auto output = folder / "water-current";
        fs::create_directory(output);
        const auto &constants = map.constants();
        water_validation::save(output / "constants.bin", constants);
        const auto original = water_validation::read(fixture / "constants0.bin");
        check(original.size() == 64, "Original current scene constant extent");
        WaterCurrentConstants reference{};
        std::memcpy(reference.data(), original.data(), original.size());
        for (unsigned at : {0u, 1u, 4u, 8u, 12u})
            check(std::bit_cast<unsigned>(constants[at]) == std::bit_cast<unsigned>(reference[at]),
                  "Current scene must use independently verified original parameters");
        water_validation::save(output / "native-constants.bin", reference);
        const auto levels = material_resource_zero();
        water_validation::Com<IDirect3DTexture9> input;
        water_validation::checked(native.device->CreateTexture(256, 256, 7, 0, D3DFMT_A8R8G8B8,
            D3DPOOL_MANAGED, &input.value, nullptr), "Current scene original resource0");
        for (unsigned mip = 0; mip < 7; ++mip) {
            auto bgra = levels[mip].rgba;
            check(map.source_texture().read_level_raw(0, mip) == bgra, "Current scene GL resource0 identity");
            water_validation::write(output / ("resource0-mip" + std::to_string(mip) + ".rgba"), bgra);
            for (std::size_t at = 0; at < bgra.size(); at += 4) std::swap(bgra[at], bgra[at + 2]);
            native.upload(input.value, mip, bgra, levels[mip].width, levels[mip].height, 4);
        }
        const auto dx = native.render(reference, input.value, 1);
        water_validation::write(output / "d3d9.rgba8", dx);
        // GlTexture's raw render-target export preserves native BGRA storage.
        // The cross-API diagnostic file explicitly promises RGBA byte order.
        civ5::assets::Bytes pixels(256 * 256 * 4);
        context->api().GetTextureImage(map.texture().id(), 0, GL_RGBA, GL_UNSIGNED_BYTE,
                                       static_cast<GLsizei>(pixels.size()), pixels.data());
        context->check("Current scene RGBA readback");
        water_validation::write(output / "gl.rgba8", pixels);
        blend_validation::text(output / "scope.txt",
            "Original current shader pair/key5d, original resource0/sampler0, worker-grid dimensions.\n"
            "Original 256x256 single-level A8R8G8B8 render-target descriptor.\n"
            "GL and D3D9 each sample their own produced texture directly in ocean slot5.\n"
            "Packet0x22 selects shared[0]/D91114; slot4 is reflection D91110.\n"
            "GL current target is owned by the world renderer with recovered isolated raster state.\n");
    }
};
}
