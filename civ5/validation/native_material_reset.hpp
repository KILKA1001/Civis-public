#pragma once
// Validation fixture only. Real Reset on the existing D3D9 device; no original
// game process, no simulated GPU result and no restoration from captured pixels.
namespace {
struct NativeMaterialResetResult {
    unsigned textures = 0, surfaces = 0, aliases = 0, managed = 0;
    HRESULT result = E_FAIL;
};

NativeMaterialResetResult reset_native_material(NativeWorld &world, unsigned width, unsigned height) {
    auto &chain = world.cache;
    auto *device = chain.device.value;
    check(!world.distance_fog, "Recovery prototype owns only the material/LAND fixture");
    struct Texture {
        IDirect3DTexture9 **slot;
        IDirect3DTexture9 *old;
        D3DSURFACE_DESC descriptor;
    };
    struct Surface {
        IDirect3DSurface9 **slot;
        D3DSURFACE_DESC descriptor;
    };
    std::vector<Texture> textures;
    std::vector<Surface> surfaces;
    const auto texture = [&](Com<IDirect3DTexture9> &owner) {
        check(owner.value != nullptr, "Expected owned DEFAULT texture before Reset");
        D3DSURFACE_DESC descriptor{};
        hr(owner->GetLevelDesc(0, &descriptor), "Recovery texture descriptor");
        check(descriptor.Pool == D3DPOOL_DEFAULT && (descriptor.Usage & D3DUSAGE_RENDERTARGET),
              "Only generated DEFAULT render textures may be released here");
        textures.push_back({&owner.value, owner.value, descriptor});
    };
    const auto surface = [&](Com<IDirect3DSurface9> &owner) {
        check(owner.value != nullptr, "Expected owned DEFAULT target before Reset");
        D3DSURFACE_DESC descriptor{};
        hr(owner->GetDesc(&descriptor), "Recovery target descriptor");
        check(descriptor.Pool == D3DPOOL_DEFAULT && (descriptor.Usage & D3DUSAGE_RENDERTARGET) &&
                  descriptor.MultiSampleType == D3DMULTISAMPLE_NONE,
              "Expected ordinary DEFAULT color target");
        surfaces.push_back({&owner.value, descriptor});
    };
    for (auto &value : chain.cache)
        texture(value);
    for (auto &value : chain.extra)
        texture(value);
    for (auto &[index, value] : world.finals) {
        (void)index;
        texture(value);
    }
    for (auto &value : chain.targets)
        surface(value);
    for (auto &value : chain.float_surfaces)
        surface(value);
    surface(chain.shadows.target);
    for (auto &value : chain.lights.targets)
        surface(value);
    surface(world.land.target);
    check(surfaces.size() == 8 && textures.size() == world.finals.size() + 6,
          "Complete material fixture DEFAULT inventory");
    const auto managed_snapshot = [&] {
        std::vector<IDirect3DBaseTexture9 *> values;
        const auto append = [&](auto &owner) {
            if (owner.value)
                values.push_back(owner.value);
        };
        for (auto &value : chain.authored_textures)
            append(value);
        for (auto &group : chain.float_textures)
            for (auto &value : group)
                append(value);
        append(chain.world_height);
        for (auto &[index, value] : world.heights) {
            (void)index;
            append(value);
        }
        for (auto &value : world.neutral)
            append(value);
        for (auto &value : world.varied)
            append(value);
        for (auto &value : chain.lights.cubes)
            append(value);
        append(chain.lights.lut);
        if (world.color_keys)
            for (auto &value : world.color_keys->volumes)
                append(value);
        for (auto *value : values) {
            D3DPOOL pool;
            if (value->GetType() == D3DRTYPE_TEXTURE) {
                D3DSURFACE_DESC d{};
                hr(static_cast<IDirect3DTexture9 *>(value)->GetLevelDesc(0, &d), "Managed 2D descriptor");
                pool = d.Pool;
            } else if (value->GetType() == D3DRTYPE_CUBETEXTURE) {
                D3DSURFACE_DESC d{};
                hr(static_cast<IDirect3DCubeTexture9 *>(value)->GetLevelDesc(0, &d), "Managed cube descriptor");
                pool = d.Pool;
            } else {
                check(value->GetType() == D3DRTYPE_VOLUMETEXTURE, "Known material source texture type");
                D3DVOLUME_DESC d{};
                hr(static_cast<IDirect3DVolumeTexture9 *>(value)->GetLevelDesc(0, &d), "Managed volume descriptor");
                pool = d.Pool;
            }
            check(pool == D3DPOOL_MANAGED, "Source texture must survive Reset in MANAGED pool");
        }
        return values;
    };
    const auto managed = managed_snapshot();
    const auto bindings = chain.resources;
    for (unsigned slot = 0; slot < 16; ++slot)
        hr(device->SetTexture(slot, nullptr), "Reset unbind pixel texture");
    for (unsigned slot = 0; slot < 4; ++slot)
        hr(device->SetTexture(D3DVERTEXTEXTURESAMPLER0 + slot, nullptr), "Reset unbind vertex texture");
    for (unsigned slot = 1; slot < 4; ++slot)
        hr(device->SetRenderTarget(slot, nullptr), "Reset unbind secondary target");
    hr(device->SetDepthStencilSurface(nullptr), "Reset unbind scene depth");
    hr(device->SetStreamSource(0, nullptr, 0, 0), "Reset unbind geometry stream");
    hr(device->SetIndices(nullptr), "Reset unbind index buffer");
    {
        Com<IDirect3DSurface9> backbuffer;
        hr(device->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &backbuffer.value), "Reset backbuffer");
        hr(device->SetRenderTarget(0, backbuffer.value), "Reset release final bound offscreen target");
    }
    // Every alias must be released before its parent texture. Never keep a
    // GetSurfaceLevel/GetBackBuffer reference across Reset.
    for (auto &alias : chain.surfaces) {
        check(alias.value != nullptr, "Expected cache surface alias");
        std::exchange(alias.value, nullptr)->Release();
    }
    for (auto &record : surfaces)
        std::exchange(*record.slot, nullptr)->Release();
    for (auto &record : textures)
        std::exchange(*record.slot, nullptr)->Release();
    // Logical bindings cannot temporarily expose released COM objects.
    for (auto &[handle, value] : chain.resources) {
        (void)handle;
        for (const auto &record : textures)
            if (value == record.old)
                value = nullptr;
    }
    D3DPRESENT_PARAMETERS parameters{};
    parameters.BackBufferWidth = width;
    parameters.BackBufferHeight = height;
    parameters.BackBufferFormat = D3DFMT_X8R8G8B8;
    parameters.BackBufferCount = 1;
    parameters.SwapEffect = D3DSWAPEFFECT_DISCARD;
    parameters.hDeviceWindow = chain.window.value;
    parameters.Windowed = TRUE;
    parameters.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;
    const HRESULT result = device->Reset(&parameters);
    hr(result, "Actual material D3D9 Reset");
    hr(device->TestCooperativeLevel(), "Restored D3D9 cooperative state");
    {
        Com<IDirect3DSurface9> backbuffer;
        D3DSURFACE_DESC descriptor{};
        hr(device->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &backbuffer.value), "Restored backbuffer");
        hr(backbuffer->GetDesc(&descriptor), "Restored backbuffer descriptor");
        check(descriptor.Width == width && descriptor.Height == height &&
                  descriptor.Format == D3DFMT_X8R8G8B8,
              "Actual Reset backbuffer dimensions differ from the request");
    }
    for (const auto &record : textures) {
        const auto &d = record.descriptor;
        // Original fixture constructors request levels=0 and usage=0x401,
        // including formats where the driver reports NOAUTOGEN support.
        hr(device->CreateTexture(d.Width, d.Height, 0,
                                  D3DUSAGE_RENDERTARGET | D3DUSAGE_AUTOGENMIPMAP,
                                  d.Format, D3DPOOL_DEFAULT, record.slot, nullptr),
           "Recreate original material cache allocation");
    }
    for (const auto &record : surfaces) {
        const auto &d = record.descriptor;
        hr(device->CreateRenderTarget(d.Width, d.Height, d.Format, d.MultiSampleType,
                                       d.MultiSampleQuality, FALSE, record.slot, nullptr),
           "Recreate material observer target");
    }
    for (unsigned i = 0; i < chain.surfaces.size(); ++i)
        hr(chain.cache[i]->GetSurfaceLevel(0, &chain.surfaces[i].value), "Recreate cache surface alias");
    for (auto &[handle, value] : chain.resources) {
        value = bindings.at(handle);
        for (const auto &record : textures)
            if (value == record.old) {
                value = *record.slot;
                break;
            }
        check(value != nullptr, "Restored material source binding");
    }
    check(managed_snapshot() == managed && world.land.device.value == device &&
              chain.shadows.device.value == device && chain.lights.device.value == device,
          "Reset must retain the existing device and every managed source owner");
    for (const auto &record : textures) {
        D3DSURFACE_DESC d{};
        hr((*record.slot)->GetLevelDesc(0, &d), "Restored texture descriptor");
        check(d.Width == record.descriptor.Width && d.Height == record.descriptor.Height &&
                  d.Format == record.descriptor.Format && d.Pool == D3DPOOL_DEFAULT,
              "Restored texture storage descriptor differs");
    }
    return {static_cast<unsigned>(textures.size()), static_cast<unsigned>(surfaces.size()),
            static_cast<unsigned>(chain.surfaces.size()), static_cast<unsigned>(managed.size()), result};
}
} // namespace
