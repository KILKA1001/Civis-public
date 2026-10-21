#pragma once
// Empty object scene with each API's own original reflection target/clear.
namespace {
struct WaterReflectionScene {
    water_validation::Com<IDirect3DTexture9> map;
    water_validation::Com<IDirect3DSurface9> color, depth, readback;
    WaterReflectionScene(std::shared_ptr<GlContext> context, GlWaterWorldRenderer &water,
                         IDirect3DDevice9 *device, const fs::path &fixture, const fs::path &folder) {
        auto shared_depth = std::make_shared<GlTexture>(GlTexture::render_target(*context, 2048, 2048, 1,
                                                         RenderTextureFormat::depth24_stencil8));
        water.prepare_reflection(2, 1, shared_depth);
        const auto raw = water_validation::read(fixture / "raster.bin");
        check(raw.size() == 236 && std::memcmp(raw.data(), "C5WRRS01", 8) == 0,
              "Original scene reflection raster fixture");
        D3DVIEWPORT9 viewport{};
        std::array<std::array<unsigned, 2>, 20> states{};
        std::array<unsigned, 6> clear{};
        std::array<float, 4> plane{};
        unsigned count = 0;
        std::memcpy(&viewport, raw.data()+8, 24); std::memcpy(&count, raw.data()+32, 4);
        check(count == 20, "Complete reflection scene states");
        std::memcpy(states.data(), raw.data()+36, 160); std::memcpy(clear.data(), raw.data()+196, 24);
        std::memcpy(plane.data(), raw.data()+220, 16);
        hr(device->CreateTexture(1024, 1024, 1, D3DUSAGE_RENDERTARGET, D3DFMT_A8R8G8B8,
                                D3DPOOL_DEFAULT, &map.value, nullptr), "Original scene reflection target");
        hr(map->GetSurfaceLevel(0, &color.value), "Original scene reflection surface");
        hr(device->CreateDepthStencilSurface(2048, 2048, D3DFMT_D24S8, D3DMULTISAMPLE_NONE, 0, TRUE,
                                            &depth.value, nullptr), "Explicit scene shared depth");
        hr(device->CreateOffscreenPlainSurface(1024, 1024, D3DFMT_A8R8G8B8, D3DPOOL_SYSTEMMEM,
                                              &readback.value, nullptr), "Scene reflection readback");
        for (unsigned i = 0; i < 4; ++i)
            hr(device->SetRenderTarget(i, i == 0 ? color.value : nullptr), "Original scene reflection binding");
        for (unsigned i = 0; i < 16; ++i)
            hr(device->SetTexture(i, nullptr), "Original scene reflection texture unbind");
        for (auto state : {D3DRS_SCISSORTESTENABLE, D3DRS_DITHERENABLE, D3DRS_SRGBWRITEENABLE})
            hr(device->SetRenderState(state, FALSE), "Explicit reflection scene device control");
        hr(device->SetDepthStencilSurface(depth.value), "Original scene shared depth binding");
        check(clear[0] == 0 && clear[1] == 0, "Original whole scene reflection clear");
        hr(device->Clear(0, nullptr, clear[2], clear[3], std::bit_cast<float>(clear[4]), clear[5]),
           "Original scene reflection clear");
        hr(device->SetViewport(&viewport), "Original reflection scene viewport");
        for (unsigned i = 0; i < states.size(); ++i) {
            hr(device->SetRenderState(static_cast<D3DRENDERSTATETYPE>(states[i][0]), states[i][1]),
               "Original reflection scene state");
            if (i == 2) hr(device->SetClipPlane(0, plane.data()), "Original scene disabled clip plane");
        }
        hr(device->GetRenderTargetData(color.value, readback.value), "Original reflection scene transfer");
        D3DLOCKED_RECT lock{};
        hr(readback->LockRect(&lock, nullptr, D3DLOCK_READONLY), "Original reflection scene lock");
        const bool valid = lock.pBits && lock.Pitch >= 4096;
        civ5::assets::Bytes pixels(1024*1024*4);
        if (valid)
            for (unsigned y = 0; y < 1024; ++y)
                std::memcpy(pixels.data()+y*4096, static_cast<char *>(lock.pBits)+y*lock.Pitch, 4096);
        hr(readback->UnlockRect(), "Original reflection scene unlock");
        check(valid && pixels == water.reflection_map().texture().read_mip_raw(0),
              "Scene reflection targets differ across APIs");
        fs::create_directories(folder / "water-reflection");
        water_validation::write(folder / "water-reflection/gl.bgra8", water.reflection_map().texture().read_mip_raw(0));
        water_validation::write(folder / "water-reflection/d3d9.bgra8", pixels);
        water_validation::write(folder / "water-reflection/raster.bin", raw);
        blend_validation::text(folder / "water-reflection/scope.txt",
            "Owned original 1024x1024 ARGB8 reflection target, blend8/depth0, original clear calls.\n"
            "Shared 2048x2048 D24S8 depth allocation and empty object scene are explicit controls.\n"
            "Original object draws, reflection camera and complete device initialization remain open.\n");
    }
};
} // namespace
