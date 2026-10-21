#pragma once
#include "civ5/materials/lighting_mesh.hpp"
#include "civ5/materials/lighting_sources.hpp"
#include "material_blend_native.hpp"
#include <cmath>

namespace lighting_validation {
using namespace blend_validation;
// Own device, complete original key4B VS and both PS. Not original-process
// injection, generated HLSL, a shader prefix, or a full world/format claim.
struct NativeLighting {
    OwnWindow window;
    Com<IDirect3D9> api{Direct3DCreate9(D3D_SDK_VERSION)};
    Com<IDirect3DDevice9> device;
    Com<IDirect3DVertexShader9> vs;
    std::array<Com<IDirect3DPixelShader9>, 2> ps;
    Com<IDirect3DVertexDeclaration9> declaration;
    std::array<Com<IDirect3DSurface9>, 2> targets, readbacks;
    std::vector<Com<IDirect3DCubeTexture9>> cubes;
    Com<IDirect3DVolumeTexture9> lut;
    std::uint64_t uploaded = 0;
    NativeLighting(const std::filesystem::path &shaders, const LightingCubeAssets &assets,
                   std::span<const float> lut_values, IDirect3DDevice9 *shared_device = nullptr) {
        require(api.value != nullptr, "Own lighting D3D9 API");
        D3DPRESENT_PARAMETERS pp{};
        pp.BackBufferWidth = pp.BackBufferHeight = 64;
        pp.BackBufferFormat = D3DFMT_X8R8G8B8;
        pp.BackBufferCount = 1;
        pp.SwapEffect = D3DSWAPEFFECT_DISCARD;
        pp.hDeviceWindow = window.value;
        pp.Windowed = TRUE;
        pp.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;
        if (shared_device) {
            shared_device->AddRef();
            device.value = shared_device;
        } else {
            hr(api->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, window.value,
                                 D3DCREATE_HARDWARE_VERTEXPROCESSING, &pp, &device.value),
               "Lighting HAL/HWVP");
        }
        const std::array<const char *, 3> names{"lighting-vs.bin", "lighting-diffuse-ps.bin",
                                                "lighting-specular-ps.bin"};
        const std::array<std::size_t, 3> lengths{0x154, 0x430, 0x5b4};
        for (unsigned i = 0; i < 3; ++i) {
            const auto bytes = read(shaders / names[i]);
            require(bytes.size() == lengths[i], "Original lighting shader length");
            std::vector<DWORD> words(bytes.size() / 4);
            std::memcpy(words.data(), bytes.data(), bytes.size());
            if (i == 0)
                hr(device->CreateVertexShader(words.data(), &vs.value), "Original lighting VS");
            else
                hr(device->CreatePixelShader(words.data(), &ps[i - 1].value), "Original lighting PS");
        }
        hr(device->SetVertexShader(vs.value), "Bind lighting VS");
        const D3DVERTEXELEMENT9 elements[]{
            {0, 12, D3DDECLTYPE_FLOAT16_4, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_NORMAL, 0},
            {0, 20, D3DDECLTYPE_FLOAT16_2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0},
            D3DDECL_END()};
        hr(device->CreateVertexDeclaration(elements, &declaration.value),
           "Explicit native normal24 declaration");
        hr(device->SetVertexDeclaration(declaration.value), "Bind normal24 declaration");
        for (unsigned i = 0; i < 2; ++i) {
            hr(device->CreateRenderTarget(64, 64, D3DFMT_A32B32G32R32F, D3DMULTISAMPLE_NONE, 0, FALSE,
                                          &targets[i].value, nullptr),
               "Lighting float target");
            hr(device->CreateOffscreenPlainSurface(64, 64, D3DFMT_A32B32G32R32F, D3DPOOL_SYSTEMMEM,
                                                   &readbacks[i].value, nullptr),
               "Lighting readback");
        }
        hr(device->SetDepthStencilSurface(nullptr), "No lighting depth");
        D3DVIEWPORT9 view{0, 0, 64, 64, 0, 1};
        hr(device->SetViewport(&view), "Lighting viewport");
        for (auto [state, value] :
             std::initializer_list<std::pair<D3DRENDERSTATETYPE, DWORD>>{{D3DRS_ZENABLE, 0},
                                                                         {D3DRS_CULLMODE, D3DCULL_NONE},
                                                                         {D3DRS_ALPHABLENDENABLE, 0},
                                                                         {D3DRS_LIGHTING, 0},
                                                                         {D3DRS_COLORWRITEENABLE, 15},
                                                                         {D3DRS_COLORWRITEENABLE1, 15},
                                                                         {D3DRS_SRGBWRITEENABLE, 0}})
            hr(device->SetRenderState(state, value), "Lighting explicit raster state");
        for (DWORD stage : {0, 1, 2, 3, 4, 5, 8}) {
            const bool cube = stage == 4 || stage == 5;
            for (auto [state, value] : std::initializer_list<std::pair<D3DSAMPLERSTATETYPE, DWORD>>{
                     {D3DSAMP_ADDRESSU, cube ? D3DTADDRESS_WRAP : D3DTADDRESS_CLAMP},
                     {D3DSAMP_ADDRESSV, cube ? D3DTADDRESS_WRAP : D3DTADDRESS_CLAMP},
                     {D3DSAMP_ADDRESSW, cube ? D3DTADDRESS_WRAP : D3DTADDRESS_CLAMP},
                     {D3DSAMP_MAGFILTER, D3DTEXF_LINEAR},
                     {D3DSAMP_MINFILTER, D3DTEXF_LINEAR},
                     {D3DSAMP_MIPFILTER, D3DTEXF_LINEAR},
                     {D3DSAMP_MAXMIPLEVEL, 0},
                     {D3DSAMP_MAXANISOTROPY, 1},
                     {D3DSAMP_SRGBTEXTURE, cube ? 1u : 0u}})
                hr(device->SetSamplerState(stage, state, value), "Original lighting sampler2/5");
        }
        for (const auto &asset : assets.cubes()) {
            const auto &data = asset.data;
            require(data.width() == 128 && data.mip_count() == 8 && data.format() == DdsFormat::dxt1,
                    "Original BC1 cube profile");
            Com<IDirect3DCubeTexture9> cube;
            hr(device->CreateCubeTexture(128, 8, 0, D3DFMT_DXT1, D3DPOOL_MANAGED, &cube.value, nullptr),
               "Original-format cube");
            for (const auto &level : data.levels()) {
                const auto bytes = data.payload(level.face, level.mip);
                const auto rows = (level.height + 3) / 4;
                const auto row_bytes = (level.width + 3) / 4 * 8;
                D3DLOCKED_RECT lock{};
                const auto face = static_cast<D3DCUBEMAP_FACES>(level.face);
                hr(cube->LockRect(face, level.mip, &lock, nullptr, 0), "Cube upload lock");
                bool valid = lock.Pitch >= static_cast<int>(row_bytes);
                if (valid)
                    for (unsigned y = 0; y < rows; ++y)
                        std::memcpy(static_cast<char *>(lock.pBits) + std::size_t(y) * lock.Pitch,
                                    bytes.data() + y * row_bytes, row_bytes);
                hr(cube->UnlockRect(face, level.mip), "Cube upload unlock");
                require(valid, "Cube upload pitch");
                hr(cube->LockRect(face, level.mip, &lock, nullptr, D3DLOCK_READONLY), "Cube readback lock");
                valid = lock.Pitch >= static_cast<int>(row_bytes);
                if (valid)
                    for (unsigned y = 0; y < rows; ++y)
                        valid = valid &&
                                std::memcmp(static_cast<char *>(lock.pBits) + std::size_t(y) * lock.Pitch,
                                            bytes.data() + y * row_bytes, row_bytes) == 0;
                hr(cube->UnlockRect(face, level.mip), "Cube readback unlock");
                require(valid, "Native cube bytes changed");
                uploaded += bytes.size();
            }
            cubes.push_back(std::move(cube));
        }
        require(lut_values.size() == 16 * 16 * 16 * 4, "Explicit LUT extent");
        hr(device->CreateVolumeTexture(16, 16, 16, 1, 0, D3DFMT_A32B32G32R32F, D3DPOOL_MANAGED, &lut.value,
                                       nullptr),
           "Explicit ColorCorrect LUT");
        for (bool verify : {false, true}) {
            D3DLOCKED_BOX box{};
            hr(lut->LockBox(0, &box, nullptr, verify ? D3DLOCK_READONLY : 0), "LUT lock");
            bool valid = box.RowPitch >= 256 && box.SlicePitch >= std::int64_t(box.RowPitch) * 16;
            if (valid)
                for (unsigned z = 0; z < 16; ++z)
                    for (unsigned y = 0; y < 16; ++y) {
                        auto *at = static_cast<char *>(box.pBits) + std::size_t(z) * box.SlicePitch +
                                   std::size_t(y) * box.RowPitch;
                        const auto *source = lut_values.data() + (z * 16 + y) * 16 * 4;
                        if (verify)
                            valid = valid && std::memcmp(at, source, 256) == 0;
                        else
                            std::memcpy(at, source, 256);
                    }
            hr(lut->UnlockBox(0), "LUT unlock");
            require(valid, "LUT upload identity/pitch");
        }
        uploaded += lut_values.size_bytes();
    }
    Com<IDirect3DTexture9> image(std::span<const float> values) {
        require(values.size() == 64 * 64 * 4, "Lighting source extent");
        Com<IDirect3DTexture9> result;
        hr(device->CreateTexture(64, 64, 1, 0, D3DFMT_A32B32G32R32F, D3DPOOL_MANAGED, &result.value, nullptr),
           "Lighting image");
        for (bool verify : {false, true}) {
            D3DLOCKED_RECT lock{};
            hr(result->LockRect(0, &lock, nullptr, verify ? D3DLOCK_READONLY : 0), "Lighting image lock");
            bool valid = lock.Pitch >= 1024;
            if (valid)
                for (unsigned y = 0; y < 64; ++y) {
                    auto *at = static_cast<char *>(lock.pBits) + std::size_t(y) * lock.Pitch;
                    const auto *source = values.data() + y * 64 * 4;
                    if (verify)
                        valid = valid && std::memcmp(at, source, 1024) == 0;
                    else
                        std::memcpy(at, source, 1024);
                }
            hr(result->UnlockRect(0), "Lighting image unlock");
            require(valid, "Lighting image upload identity/pitch");
        }
        uploaded += values.size_bytes();
        return result;
    }
    std::vector<float> draw(const LightingPatchMesh &mesh, const std::array<IDirect3DTexture9 *, 4> &images,
                            unsigned style, bool specular, const std::array<float, 3> &half) {
        require(style < 5, "Lighting native style");
        for (unsigned i = 0; i < 4; ++i)
            hr(device->SetTexture(i, images[i]), "Lighting source binding");
        hr(device->SetTexture(4, cubes.at(style * 2).value), "Lighting sunlight");
        hr(device->SetTexture(5, cubes.at(style * 2 + 1).value), "Lighting shadowlight");
        hr(device->SetTexture(8, lut.value), "Lighting ColorCorrect binding");
        const std::array<float, 4> hv{half[0], half[1], half[2], 0};
        hr(device->SetPixelShaderConstantF(5, hv.data(), 1), "Original PS5 half vector");
        hr(device->SetPixelShader(ps[specular ? 1 : 0].value), "Original lighting selector");
        hr(device->SetRenderTarget(0, targets[0].value), "Lighting MRT0");
        hr(device->SetRenderTarget(1, specular ? targets[1].value : nullptr), "Lighting MRT1");
        hr(device->Clear(0, nullptr, D3DCLEAR_TARGET, 0xffaa3377, 1, 0), "Lighting sentinel clear");
        hr(device->BeginScene(), "Lighting BeginScene");
        const auto result = device->DrawIndexedPrimitiveUP(
            D3DPT_TRIANGLELIST, 0, mesh.columns() * mesh.rows(), static_cast<UINT>(mesh.indices().size() / 3),
            mesh.indices().data(), D3DFMT_INDEX16, mesh.vertices().data(), 24);
        const auto end = device->EndScene();
        hr(result, "Complete original lighting draw");
        hr(end, "Lighting EndScene");
        std::vector<float> output;
        for (unsigned i = 0; i < (specular ? 2u : 1u); ++i) {
            hr(device->GetRenderTargetData(targets[i].value, readbacks[i].value), "Lighting native readback");
            D3DLOCKED_RECT lock{};
            hr(readbacks[i]->LockRect(&lock, nullptr, D3DLOCK_READONLY), "Lighting readback lock");
            const bool valid = lock.Pitch >= 1024;
            const auto start = output.size();
            output.resize(start + 64 * 64 * 4);
            if (valid)
                for (unsigned y = 0; y < 64; ++y)
                    std::memcpy(output.data() + start + y * 64 * 4,
                                static_cast<char *>(lock.pBits) + std::size_t(y) * lock.Pitch, 1024);
            hr(readbacks[i]->UnlockRect(), "Lighting readback unlock");
            require(valid, "Lighting readback pitch");
        }
        for (std::size_t i = 0; i < output.size(); ++i)
            require(std::isfinite(output[i]) && (i >= 16384 || i % 4 != 3 || output[i] == 1),
                    "Invalid/uncovered original lighting output");
        return output;
    }
};
} // namespace lighting_validation
