#pragma once
#include "civ5/materials/material_shadows.hpp"
#include "material_blend_native.hpp"
#include <cmath>

namespace shadow_validation {
using namespace blend_validation;
// Own validation device. Executes the unchanged original pair with material48,
// explicit float/A8 sources, original sampler0 and profile4 state calls.
struct NativeShadows {
    OwnWindow window;
    Com<IDirect3D9> api{Direct3DCreate9(D3D_SDK_VERSION)};
    Com<IDirect3DDevice9> device;
    Com<IDirect3DVertexShader9> vs;
    Com<IDirect3DPixelShader9> ps;
    Com<IDirect3DVertexDeclaration9> declaration;
    Com<IDirect3DSurface9> target, readback;
    explicit NativeShadows(const std::filesystem::path &shaders, IDirect3DDevice9 *shared_device = nullptr) {
        require(api.value != nullptr, "Own shadow D3D9 API");
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
               "Own shadow HAL/HWVP");
        }
        const auto vertex = read(shaders / "shadow-vs.bin"), pixel = read(shaders / "shadow-ps.bin");
        require(vertex.size() == 404 && pixel.size() == 1068, "Original shadow shader lengths");
        std::vector<DWORD> vwords(vertex.size() / 4), pwords(pixel.size() / 4);
        std::memcpy(vwords.data(), vertex.data(), vertex.size());
        std::memcpy(pwords.data(), pixel.data(), pixel.size());
        hr(device->CreateVertexShader(vwords.data(), &vs.value), "Original shadow VS");
        hr(device->CreatePixelShader(pwords.data(), &ps.value), "Original shadow PS");
        hr(device->SetVertexShader(vs.value), "Bind shadow VS");
        hr(device->SetPixelShader(ps.value), "Bind shadow PS");
        const D3DVERTEXELEMENT9 elements[]{
            {0, 0, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
            {0, 12, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0},
            D3DDECL_END()};
        hr(device->CreateVertexDeclaration(elements, &declaration.value),
           "Shadow explicit material48 declaration");
        hr(device->SetVertexDeclaration(declaration.value), "Bind shadow declaration");
        hr(device->CreateRenderTarget(64, 64, D3DFMT_A32B32G32R32F, D3DMULTISAMPLE_NONE, 0, FALSE,
                                      &target.value, nullptr),
           "Shadow target");
        hr(device->CreateOffscreenPlainSurface(64, 64, D3DFMT_A32B32G32R32F, D3DPOOL_SYSTEMMEM,
                                               &readback.value, nullptr),
           "Shadow readback");
        hr(device->SetRenderTarget(0, target.value), "Bind shadow target");
        hr(device->SetDepthStencilSurface(nullptr), "No shadow depth");
        D3DVIEWPORT9 view{0, 0, 64, 64, 0, 1};
        hr(device->SetViewport(&view), "Shadow viewport");
        for (auto [state, value] :
             std::initializer_list<std::pair<D3DRENDERSTATETYPE, DWORD>>{{D3DRS_ZENABLE, FALSE},
                                                                         {D3DRS_CULLMODE, D3DCULL_NONE},
                                                                         {D3DRS_LIGHTING, FALSE},
                                                                         {D3DRS_SCISSORTESTENABLE, FALSE},
                                                                         {D3DRS_DITHERENABLE, FALSE},
                                                                         {D3DRS_SRGBWRITEENABLE, FALSE}})
            hr(device->SetRenderState(state, value), "Explicit shadow raster state");
        for (auto [state, value] : std::initializer_list<std::pair<D3DSAMPLERSTATETYPE, DWORD>>{
                 {D3DSAMP_ADDRESSU, D3DTADDRESS_WRAP},
                 {D3DSAMP_ADDRESSV, D3DTADDRESS_WRAP},
                 {D3DSAMP_ADDRESSW, D3DTADDRESS_WRAP},
                 {D3DSAMP_MINFILTER, D3DTEXF_LINEAR},
                 {D3DSAMP_MAGFILTER, D3DTEXF_LINEAR},
                 {D3DSAMP_MIPFILTER, D3DTEXF_LINEAR},
                 {D3DSAMP_MAXANISOTROPY, 1},
                 {D3DSAMP_MIPMAPLODBIAS, 0},
                 {D3DSAMP_MAXMIPLEVEL, 0},
                 {D3DSAMP_SRGBTEXTURE, FALSE}})
            hr(device->SetSamplerState(0, state, value), "Original shadow sampler0");
    }
    Com<IDirect3DTexture9> texture(unsigned size, std::span<const std::uint8_t> data, bool alpha8) {
        const unsigned pitch = size * (alpha8 ? 1 : 16);
        require((size == 64 || size == 1024) && data.size() == std::size_t(size) * pitch,
                "Shadow source extent");
        Com<IDirect3DTexture9> result;
        hr(device->CreateTexture(size, size, 1, 0, alpha8 ? D3DFMT_A8 : D3DFMT_A32B32G32R32F, D3DPOOL_MANAGED,
                                 &result.value, nullptr),
           "Shadow source allocation");
        D3DLOCKED_RECT lock{};
        hr(result->LockRect(0, &lock, nullptr, 0), "Shadow source lock");
        bool valid = lock.Pitch >= 0 && static_cast<unsigned>(lock.Pitch) >= pitch;
        if (valid)
            for (unsigned y = 0; y < size; ++y)
                std::memcpy(static_cast<char *>(lock.pBits) + std::size_t(y) * lock.Pitch,
                            data.data() + std::size_t(y) * pitch, pitch);
        hr(result->UnlockRect(0), "Shadow source unlock");
        require(valid, "Shadow source pitch");
        hr(result->LockRect(0, &lock, nullptr, D3DLOCK_READONLY), "Shadow source verification");
        valid = lock.Pitch >= 0 && static_cast<unsigned>(lock.Pitch) >= pitch;
        if (valid)
            for (unsigned y = 0; y < size; ++y)
                valid = valid && std::memcmp(static_cast<char *>(lock.pBits) + std::size_t(y) * lock.Pitch,
                                             data.data() + std::size_t(y) * pitch, pitch) == 0;
        hr(result->UnlockRect(0), "Shadow verification unlock");
        require(valid, "Shadow input upload differs");
        return result;
    }
    std::vector<float> draw(const MaterialPatchMesh &mesh, IDirect3DTexture9 *source,
                            std::span<const MaterialShadowParameters> samples, bool accumulated,
                            const std::array<float, 2> &offset = {0, 0}) {
        require(!samples.empty(), "Native shadow test needs samples");
        hr(device->SetTexture(0, source), "Bind shadow height source");
        const std::array<float, 4> offsets{offset[0], offset[1], 0, 0};
        hr(device->SetVertexShaderConstantF(35, offsets.data(), 1), "Shadow VS35 offsets");
        // Complete original blend profile4 (193=BLENDFACTOR, not sRGB write).
        for (auto [state, value] : std::initializer_list<std::pair<D3DRENDERSTATETYPE, DWORD>>{
                 {D3DRS_ALPHATESTENABLE, 0},
                 {D3DRS_ALPHABLENDENABLE, accumulated ? 1u : 0u},
                 {D3DRS_SEPARATEALPHABLENDENABLE, accumulated ? 1u : 0u},
                 {D3DRS_SRCBLEND, D3DBLEND_ONE},
                 {D3DRS_DESTBLEND, D3DBLEND_ONE},
                 {D3DRS_BLENDOP, D3DBLENDOP_ADD},
                 {D3DRS_SRCBLENDALPHA, D3DBLEND_ONE},
                 {D3DRS_DESTBLENDALPHA, D3DBLEND_ONE},
                 {D3DRS_BLENDOPALPHA, D3DBLENDOP_ADD},
                 {D3DRS_COLORWRITEENABLE, 15},
                 {D3DRS_BLENDFACTOR, 0}})
            hr(device->SetRenderState(state, value), "Original shadow blend profile4");
        hr(device->Clear(0, nullptr, D3DCLEAR_TARGET, accumulated ? 0 : 0xffaa3377, 1, 0),
           "Shadow black/poison clear");
        hr(device->BeginScene(), "Shadow BeginScene");
        try {
            for (const auto &p : samples) {
                p.validate();
                const auto uniform = [&](unsigned at, std::span<const float> values) {
                    std::array<float, 4> data{};
                    std::copy(values.begin(), values.end(), data.begin());
                    hr(device->SetPixelShaderConstantF(at, data.data(), 1), "Original shadow PS constants");
                };
                uniform(0, p.world_scales);
                uniform(1, p.sun_direction);
                uniform(6, p.height_texture_scale);
                uniform(21, p.world_rebase);
                uniform(23, p.jitter_add);
                hr(device->DrawIndexedPrimitiveUP(D3DPT_TRIANGLELIST, 0, mesh.columns() * mesh.rows(),
                                                  static_cast<UINT>(mesh.indices().size() / 3),
                                                  mesh.indices().data(), D3DFMT_INDEX16,
                                                  mesh.vertices().data(), 48),
                   "Original macro-shadow draw");
            }
        } catch (...) {
            device->EndScene();
            throw;
        }
        hr(device->EndScene(), "Shadow EndScene");
        hr(device->GetRenderTargetData(target.value, readback.value), "Shadow native readback");
        D3DLOCKED_RECT lock{};
        hr(readback->LockRect(&lock, nullptr, D3DLOCK_READONLY), "Shadow readback lock");
        std::vector<float> output(64 * 64 * 4);
        const bool valid = lock.Pitch >= 64 * 16;
        if (valid)
            for (unsigned y = 0; y < 64; ++y)
                std::memcpy(output.data() + y * 64 * 4,
                            static_cast<char *>(lock.pBits) + std::size_t(y) * lock.Pitch, 64 * 16);
        hr(readback->UnlockRect(), "Shadow readback unlock");
        require(valid, "Shadow readback pitch");
        for (std::size_t i = 0; i < output.size(); i += 4)
            require(std::isfinite(output[i]) && output[i] == output[i + 1] && output[i] == output[i + 2] &&
                        output[i] == output[i + 3],
                    "Uncovered native shadow output");
        return output; // Native top-row sequence; GL shadow VS implements this local convention.
    }
};
} // namespace shadow_validation
