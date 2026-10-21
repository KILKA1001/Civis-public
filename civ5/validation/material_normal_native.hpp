#pragma once
// Validation only. Reuses COM lifetime helpers, NOT NativeBlend's device/state.
#include "civ5/materials/material_normals.hpp"
#include "material_blend_native.hpp"
namespace normal_validation {
using namespace blend_validation;
struct NativeNormals {
    OwnWindow window;
    Com<IDirect3D9> api{Direct3DCreate9(D3D_SDK_VERSION)};
    Com<IDirect3DDevice9> device;
    Com<IDirect3DVertexShader9> vs;
    Com<IDirect3DPixelShader9> ps;
    Com<IDirect3DVertexDeclaration9> declaration;
    Com<IDirect3DTexture9> source;
    Com<IDirect3DSurface9> target, readback;
    explicit NativeNormals(const std::filesystem::path &shaders) {
        require(api.value != nullptr, "Own normal D3D9 API");
        D3DPRESENT_PARAMETERS pp{};
        pp.BackBufferWidth = pp.BackBufferHeight = 64;
        pp.BackBufferFormat = D3DFMT_X8R8G8B8;
        pp.BackBufferCount = 1;
        pp.SwapEffect = D3DSWAPEFFECT_DISCARD;
        pp.hDeviceWindow = window.value;
        pp.Windowed = TRUE;
        pp.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;
        hr(api->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, window.value,
                             D3DCREATE_HARDWARE_VERTEXPROCESSING, &pp, &device.value),
           "Own normal HAL/HWVP");
        const auto vertex = read(shaders / "normal-vs.bin"), pixel = read(shaders / "normal-ps.bin");
        require(vertex.size() == 228 && pixel.size() == 968, "Original normal shader lengths");
        std::vector<DWORD> vwords(vertex.size() / 4), pwords(pixel.size() / 4);
        std::memcpy(vwords.data(), vertex.data(), vertex.size());
        std::memcpy(pwords.data(), pixel.data(), pixel.size());
        hr(device->CreateVertexShader(vwords.data(), &vs.value), "Complete original normal VS");
        hr(device->CreatePixelShader(pwords.data(), &ps.value), "Complete original normal PS");
        hr(device->SetVertexShader(vs.value), "Bind normal VS");
        hr(device->SetPixelShader(ps.value), "Bind normal PS");
        const D3DVERTEXELEMENT9 elements[]{
            {0, 0, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
            {0, 12, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0},
            D3DDECL_END()};
        hr(device->CreateVertexDeclaration(elements, &declaration.value), "Explicit normal quad declaration");
        hr(device->SetVertexDeclaration(declaration.value), "Bind normal declaration");
        hr(device->CreateTexture(64, 64, 1, 0, D3DFMT_A32B32G32R32F, D3DPOOL_MANAGED, &source.value, nullptr),
           "Normal source");
        hr(device->CreateRenderTarget(64, 64, D3DFMT_A32B32G32R32F, D3DMULTISAMPLE_NONE, 0, FALSE,
                                      &target.value, nullptr),
           "Normal target");
        hr(device->CreateOffscreenPlainSurface(64, 64, D3DFMT_A32B32G32R32F, D3DPOOL_SYSTEMMEM,
                                               &readback.value, nullptr),
           "Normal readback");
        hr(device->SetRenderTarget(0, target.value), "Bind normal target");
        hr(device->SetDepthStencilSurface(nullptr), "No normal depth");
        D3DVIEWPORT9 vp{0, 0, 64, 64, 0, 1};
        hr(device->SetViewport(&vp), "Normal viewport");
        for (auto [state, value] :
             std::initializer_list<std::pair<D3DRENDERSTATETYPE, DWORD>>{{D3DRS_ZENABLE, FALSE},
                                                                         {D3DRS_CULLMODE, D3DCULL_NONE},
                                                                         {D3DRS_ALPHABLENDENABLE, FALSE},
                                                                         {D3DRS_ALPHATESTENABLE, FALSE},
                                                                         {D3DRS_LIGHTING, FALSE},
                                                                         {D3DRS_SCISSORTESTENABLE, FALSE},
                                                                         {D3DRS_DITHERENABLE, FALSE},
                                                                         {D3DRS_SRGBWRITEENABLE, FALSE},
                                                                         {D3DRS_COLORWRITEENABLE, 15}})
            hr(device->SetRenderState(state, value), "Normal raster state");
        for (auto [state, value] : std::initializer_list<std::pair<D3DSAMPLERSTATETYPE, DWORD>>{
                 {D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP},
                 {D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP},
                 {D3DSAMP_ADDRESSW, D3DTADDRESS_CLAMP},
                 {D3DSAMP_MINFILTER, D3DTEXF_LINEAR},
                 {D3DSAMP_MAGFILTER, D3DTEXF_LINEAR},
                 {D3DSAMP_MIPFILTER, D3DTEXF_LINEAR},
                 {D3DSAMP_MAXANISOTROPY, 1},
                 {D3DSAMP_MIPMAPLODBIAS, 0},
                 {D3DSAMP_MAXMIPLEVEL, 0},
                 {D3DSAMP_SRGBTEXTURE, FALSE}})
            hr(device->SetSamplerState(0, state, value), "Native sampler5");
        const auto parameters = material_normal_parameters(64);
        hr(device->SetPixelShaderConstantF(0, parameters.data(), 2), "Native normal parameters");
    }
    std::vector<float> draw(std::span<const float> values, const std::array<float, 4> &corners) {
        require(values.size() == 64 * 64 * 4, "Native normal source extent");
        D3DLOCKED_RECT lock{};
        hr(source->LockRect(0, &lock, nullptr, 0), "Normal source lock");
        bool valid = lock.Pitch >= 64 * 16;
        if (valid)
            for (unsigned y = 0; y < 64; ++y)
                std::memcpy(static_cast<char *>(lock.pBits) + std::size_t(y) * lock.Pitch,
                            values.data() + y * 64 * 4, 64 * 16);
        hr(source->UnlockRect(0), "Normal source unlock");
        require(valid, "Normal input pitch");
        hr(source->LockRect(0, &lock, nullptr, D3DLOCK_READONLY), "Normal upload verification");
        valid = lock.Pitch >= 64 * 16;
        if (valid)
            for (unsigned y = 0; y < 64; ++y)
                valid = valid && std::memcmp(static_cast<char *>(lock.pBits) + std::size_t(y) * lock.Pitch,
                                             values.data() + y * 64 * 4, 64 * 16) == 0;
        hr(source->UnlockRect(0), "Normal verify unlock");
        require(valid, "Normal uploaded bytes differ");
        hr(device->SetTexture(0, source.value), "Normal source binding");
        const auto a = corners[0], b = corners[1], e = corners[2], f = corners[3];
        // VS passes XYZ/UV through. Half-pixel conversion lives in this explicit
        // quad fixture, NOT a shader modification or recovered native geometry.
        const float d = 1.f / 64;
        const std::array<float, 20> vertices{-1 - d, 1 + d,  0, a, b, 1 - d, 1 + d,  0, e, b,
                                             -1 - d, -1 + d, 0, a, f, 1 - d, -1 + d, 0, e, f};
        hr(device->Clear(0, nullptr, D3DCLEAR_TARGET, 0xffff00ff, 1, 0), "Poison native normal target");
        hr(device->BeginScene(), "Normal BeginScene");
        const auto drawn = device->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, vertices.data(), 20);
        const auto ended = device->EndScene();
        hr(drawn, "Original normal VS/PS draw");
        hr(ended, "Normal EndScene");
        hr(device->GetRenderTargetData(target.value, readback.value), "Normal GPU readback");
        hr(readback->LockRect(&lock, nullptr, D3DLOCK_READONLY), "Normal readback lock");
        std::vector<float> output(64 * 64 * 4);
        valid = lock.Pitch >= 64 * 16;
        if (valid)
            for (unsigned y = 0; y < 64; ++y)
                std::memcpy(output.data() + y * 64 * 4,
                            static_cast<char *>(lock.pBits) + std::size_t(y) * lock.Pitch, 64 * 16);
        hr(readback->UnlockRect(), "Normal readback unlock");
        require(valid, "Normal output pitch");
        // No row flip: explicit native top-to-bottom UV traversal matches GL's
        // bottom-to-top traversal in the exported UV-domain sequence.
        return output;
    }
};
} // namespace normal_validation
