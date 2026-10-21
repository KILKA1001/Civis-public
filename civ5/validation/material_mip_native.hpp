#pragma once
#include "material_blend_native.hpp"
#include <d3dcompiler.h>

namespace mip_validation {
using namespace blend_validation;
// Diagnostic shader only: sample each hidden automatic mip at texel centers.
// This is separate from every unmodified original-shader image comparison.
struct NativeMipReader {
    IDirect3DDevice9 *device;
    Com<IDirect3DPixelShader9> shader;
    explicit NativeMipReader(IDirect3DDevice9 *d) : device(d) {
        const std::string source = "float4 level:register(c0);sampler2D s:register(s0);"
                                   "float4 main(float2 pixel:VPOS):COLOR0{return "
                                   "tex2Dlod(s,float4((pixel+.5)*level.xy,0,level.z));}";
        Com<ID3DBlob> code, error;
        const auto result = D3DCompile(source.data(), source.size(), "own_hidden_mip_reader", nullptr,
                                       nullptr, "main", "ps_3_0", 0, 0, &code.value, &error.value);
        if (FAILED(result) && error.value)
            throw std::runtime_error(static_cast<const char *>(error->GetBufferPointer()));
        hr(result, "Compile diagnostic mip reader");
        hr(device->CreatePixelShader(static_cast<const DWORD *>(code->GetBufferPointer()), &shader.value),
           "Create mip reader");
    }
    std::vector<float> read(IDirect3DTexture9 *texture, unsigned mip) {
        D3DSURFACE_DESC base{};
        hr(texture->GetLevelDesc(0, &base), "Hidden mip source description");
        const auto width = std::max(1u, base.Width >> mip), height = std::max(1u, base.Height >> mip);
        Com<IDirect3DSurface9> target, staging;
        hr(device->CreateRenderTarget(width, height, D3DFMT_A32B32G32R32F, D3DMULTISAMPLE_NONE, 0, FALSE,
                                      &target.value, nullptr),
           "Hidden mip float observer");
        hr(device->CreateOffscreenPlainSurface(width, height, D3DFMT_A32B32G32R32F, D3DPOOL_SYSTEMMEM,
                                               &staging.value, nullptr),
           "Hidden mip observer readback");
        hr(device->SetDepthStencilSurface(nullptr), "Hidden mip no depth");
        for (unsigned i = 1; i < 4; ++i)
            hr(device->SetRenderTarget(i, nullptr), "Hidden mip one target");
        hr(device->SetRenderTarget(0, target.value), "Hidden mip target");
        const D3DVIEWPORT9 viewport{0, 0, width, height, 0, 1};
        hr(device->SetViewport(&viewport), "Hidden mip viewport");
        for (auto [state, value] :
             std::initializer_list<std::pair<D3DRENDERSTATETYPE, DWORD>>{{D3DRS_ZENABLE, FALSE},
                                                                         {D3DRS_CULLMODE, D3DCULL_NONE},
                                                                         {D3DRS_ALPHABLENDENABLE, FALSE},
                                                                         {D3DRS_ALPHATESTENABLE, FALSE},
                                                                         {D3DRS_SCISSORTESTENABLE, FALSE},
                                                                         {D3DRS_DITHERENABLE, FALSE},
                                                                         {D3DRS_SRGBWRITEENABLE, FALSE},
                                                                         {D3DRS_COLORWRITEENABLE, 15}})
            hr(device->SetRenderState(state, value), "Hidden mip raster state");
        for (auto [state, value] : std::initializer_list<std::pair<D3DSAMPLERSTATETYPE, DWORD>>{
                 {D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP},
                 {D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP},
                 {D3DSAMP_MINFILTER, D3DTEXF_POINT},
                 {D3DSAMP_MAGFILTER, D3DTEXF_POINT},
                 {D3DSAMP_MIPFILTER, D3DTEXF_POINT},
                 {D3DSAMP_MAXMIPLEVEL, 0},
                 {D3DSAMP_SRGBTEXTURE, FALSE}})
            hr(device->SetSamplerState(0, state, value), "Hidden mip exact point sampling");
        hr(device->SetVertexShader(nullptr), "Hidden mip fixed pretransformed VS");
        hr(device->SetFVF(D3DFVF_XYZRHW), "Hidden mip pretransformed vertex");
        hr(device->SetPixelShader(shader.value), "Hidden mip diagnostic PS");
        hr(device->SetTexture(0, texture), "Hidden mip input");
        const float constants[]{1.f / width, 1.f / height, static_cast<float>(mip), 0};
        hr(device->SetPixelShaderConstantF(0, constants, 1), "Hidden mip coordinate/Lod");
        const float quad[]{-.5f, -.5f,         0, 1, width - .5f, -.5f,         0, 1,
                           -.5f, height - .5f, 0, 1, width - .5f, height - .5f, 0, 1};
        hr(device->BeginScene(), "Hidden mip begin");
        const auto draw = device->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, quad, 16);
        const auto end = device->EndScene();
        hr(draw, "Hidden mip point read");
        hr(end, "Hidden mip end");
        hr(device->GetRenderTargetData(target.value, staging.value), "Hidden mip download");
        D3DLOCKED_RECT locked{};
        hr(staging->LockRect(&locked, nullptr, D3DLOCK_READONLY), "Hidden mip lock");
        std::vector<float> output(std::size_t(width) * height * 4);
        require(locked.Pitch >= static_cast<int>(width * 16), "Hidden mip readback pitch");
        for (unsigned y = 0; y < height; ++y)
            std::memcpy(output.data() + std::size_t(y) * width * 4,
                        static_cast<const std::uint8_t *>(locked.pBits) + std::size_t(y) * locked.Pitch,
                        width * 16);
        hr(staging->UnlockRect(), "Hidden mip unlock");
        return output;
    }
};
} // namespace mip_validation
