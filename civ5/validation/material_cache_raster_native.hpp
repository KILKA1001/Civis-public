#pragma once
// Validation only: one owned D3D9 device, complete original shaders and the
// original quad bytes exported independently by the bounded x86 oracle.
#include "civ5/materials/material_cache.hpp"
#include "material_blend_native.hpp"

namespace cache_raster_validation {
using namespace blend_validation;
struct NativeCacheRaster : NativeBlend {
    unsigned size;
    std::array<Com<IDirect3DTexture9>, 3> cache;
    std::array<Com<IDirect3DSurface9>, 3> surfaces, staging;
    std::array<Com<IDirect3DSurface9>, 2> float_surfaces, float_staging;
    Com<IDirect3DVertexShader9> normal_vs;
    Com<IDirect3DPixelShader9> normal_ps;
    Com<IDirect3DVertexDeclaration9> normal_declaration;
    std::array<float, 32> quad;
    std::array<std::uint32_t, 6> quad_indices;
    std::array<bool, 3> automatic{};
    NativeCacheRaster(const std::filesystem::path &blend_shaders, const std::filesystem::path &normal_shaders,
                      const MaterialAssets &assets, unsigned resolution,
                      std::span<const std::uint8_t> geometry)
        : NativeBlend(blend_shaders, assets), size(resolution) {
        require(geometry.size() == 152, "Original geometry extent");
        std::memcpy(quad.data(), geometry.data(), 128);
        std::memcpy(quad_indices.data(), geometry.data() + 128, 24);
        for (unsigned i = 0; i < 3; ++i) {
            const auto format = i == 2 ? D3DFMT_A8R8G8B8 : D3DFMT_A16B16G16R16F;
            const auto support = api->CheckDeviceFormat(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, D3DFMT_X8R8G8B8,
                                                        D3DUSAGE_RENDERTARGET | D3DUSAGE_AUTOGENMIPMAP,
                                                        D3DRTYPE_TEXTURE, format);
            hr(support, "Original requested cache format");
            automatic[i] = support != D3DOK_NOAUTOGEN;
            hr(device->CreateTexture(size, size, 0, D3DUSAGE_RENDERTARGET | D3DUSAGE_AUTOGENMIPMAP, format,
                                     D3DPOOL_DEFAULT, &cache[i].value, nullptr),
               "Original cache allocation request");
            hr(cache[i]->GetSurfaceLevel(0, &surfaces[i].value), "Original cache surface");
            hr(device->CreateOffscreenPlainSurface(size, size, format, D3DPOOL_SYSTEMMEM, &staging[i].value,
                                                   nullptr),
               "Cache readback allocation");
            if (i < 2) {
                hr(device->CreateRenderTarget(size, size, D3DFMT_A32B32G32R32F, D3DMULTISAMPLE_NONE, 0, FALSE,
                                              &float_surfaces[i].value, nullptr),
                   "Cache preconversion control target");
                hr(device->CreateOffscreenPlainSurface(size, size, D3DFMT_A32B32G32R32F, D3DPOOL_SYSTEMMEM,
                                                       &float_staging[i].value, nullptr),
                   "Cache preconversion readback");
            }
        }
        const auto vertex = read(normal_shaders / "normal-vs.bin"),
                   pixel = read(normal_shaders / "normal-ps.bin");
        require(vertex.size() == 228 && pixel.size() == 968, "Original normal shader extent");
        std::vector<DWORD> v(vertex.size() / 4), p(pixel.size() / 4);
        std::memcpy(v.data(), vertex.data(), vertex.size());
        std::memcpy(p.data(), pixel.data(), pixel.size());
        hr(device->CreateVertexShader(v.data(), &normal_vs.value), "Original cache normal VS");
        hr(device->CreatePixelShader(p.data(), &normal_ps.value), "Original cache normal PS");
        const D3DVERTEXELEMENT9 elements[]{
            {0, 0, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
            {0, 24, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0},
            D3DDECL_END()};
        hr(device->CreateVertexDeclaration(elements, &normal_declaration.value),
           "Original normal vertex32 semantics");
    }
    void target(bool normal, std::uint32_t clear, bool float_control = false) {
        for (unsigned i = 0; i < 16; ++i)
            hr(device->SetTexture(i, nullptr), "Unbind cache inputs before targets");
        for (unsigned i = 0; i < 4; ++i)
            hr(device->SetRenderTarget(i, float_control && i < 2 ? float_surfaces[i].value
                                          : i == 0               ? surfaces[normal ? 2 : 0].value
                                          : !normal && i == 1    ? surfaces[1].value
                                                                 : nullptr),
               "Cache MRT targets");
        hr(device->SetDepthStencilSurface(nullptr), "Cache depth profile9");
        D3DVIEWPORT9 vp{0, 0, size, size, 0, 1};
        hr(device->SetViewport(&vp), "Cache native viewport");
        hr(device->SetRenderState(D3DRS_ZWRITEENABLE, FALSE), "Cache depth write9");
        hr(device->SetRenderState(D3DRS_STENCILENABLE, FALSE), "Cache stencil9");
        hr(device->SetRenderState(D3DRS_FILLMODE, D3DFILL_SOLID), "Cache fill");
        hr(device->Clear(0, nullptr, D3DCLEAR_TARGET, clear, 1, 0), "Cache original clear color");
    }
    void material(const MaterialPatchMesh &mesh, std::span<const float, 384> coefficients,
                  const std::array<std::uint16_t, 16> &ids, std::array<float, 2> offset,
                  bool float_control = false, IDirect3DPixelShader9 *diagnostic = nullptr) {
        target(false, 0xff000000, float_control);
        hr(device->SetVertexDeclaration(declaration.value), "Cache material declaration");
        hr(device->SetVertexShader(vertex_shader.value), "Cache material original VS");
        hr(device->SetPixelShader(diagnostic ? diagnostic : pixel_shader.value), "Cache material PS");
        const std::array<float, 4> native_offset{offset[0], offset[1], 0, 0};
        hr(device->SetVertexShaderConstantF(35, native_offset.data(), 1), "Unmodified native cache offsets");
        hr(device->SetVertexShaderConstantF(109, coefficients.data(), 96), "Cache material coefficients");
        for (unsigned slot = 0; slot < 16; ++slot) {
            hr(device->SetTexture(slot, authored_textures.at(ids[slot]).value), "Cache authored texture");
            sampler(slot, D3DTADDRESS_WRAP);
        }
        hr(device->BeginScene(), "Cache material BeginScene");
        auto drawn = device->DrawIndexedPrimitiveUP(
            D3DPT_TRIANGLELIST, 0, mesh.columns() * mesh.rows(), static_cast<UINT>(mesh.indices().size() / 3),
            mesh.indices().data(), D3DFMT_INDEX16, mesh.vertices().data(), 48);
        const auto ended = device->EndScene();
        hr(drawn, "Native cache material draw");
        hr(ended, "Cache material EndScene");
    }
    void sampler(unsigned slot, D3DTEXTUREADDRESS address) {
        for (auto [state, value] :
             std::initializer_list<std::pair<D3DSAMPLERSTATETYPE, DWORD>>{{D3DSAMP_ADDRESSU, address},
                                                                          {D3DSAMP_ADDRESSV, address},
                                                                          {D3DSAMP_ADDRESSW, address},
                                                                          {D3DSAMP_MINFILTER, D3DTEXF_LINEAR},
                                                                          {D3DSAMP_MAGFILTER, D3DTEXF_LINEAR},
                                                                          {D3DSAMP_MIPFILTER, D3DTEXF_LINEAR},
                                                                          {D3DSAMP_MAXANISOTROPY, 1},
                                                                          {D3DSAMP_MIPMAPLODBIAS, 0},
                                                                          {D3DSAMP_MAXMIPLEVEL, 0},
                                                                          {D3DSAMP_SRGBTEXTURE, FALSE}})
            hr(device->SetSamplerState(slot, state, value), "Original cache sampler profile");
    }
    void normals() {
        target(true, 0xff007f00);
        hr(device->SetVertexDeclaration(normal_declaration.value), "Cache normal declaration");
        hr(device->SetVertexShader(normal_vs.value), "Cache normal original VS");
        hr(device->SetPixelShader(normal_ps.value), "Cache normal original PS");
        const std::array<float, 8> p{1.f / size, 1.f / size, 0, 0, 16, 0, 0, 0};
        hr(device->SetPixelShaderConstantF(0, p.data(), 2), "Cache normal constants");
        hr(device->SetTexture(0, cache[1].value), "Direct native material-height texture");
        sampler(0, D3DTADDRESS_CLAMP);
        hr(device->BeginScene(), "Cache normal BeginScene");
        const auto drawn = device->DrawIndexedPrimitiveUP(D3DPT_TRIANGLELIST, 0, 4, 2, quad_indices.data(),
                                                          D3DFMT_INDEX32, quad.data(), 32);
        const auto ended = device->EndScene();
        hr(drawn, "Complete native cache normal draw");
        hr(ended, "Cache normal EndScene");
    }
    Bytes clear_pixels(D3DFORMAT format, DWORD color) {
        Com<IDirect3DSurface9> surface, readback;
        hr(device->CreateRenderTarget(17, 9, format, D3DMULTISAMPLE_NONE, 0, FALSE, &surface.value, nullptr),
           "Native clear format target");
        hr(device->CreateOffscreenPlainSurface(17, 9, format, D3DPOOL_SYSTEMMEM, &readback.value, nullptr),
           "Native clear readback");
        for (unsigned i = 0; i < 16; ++i)
            hr(device->SetTexture(i, nullptr), "Native clear input reset");
        for (unsigned i = 1; i < 4; ++i)
            hr(device->SetRenderTarget(i, nullptr), "Native clear MRT reset");
        hr(device->SetRenderTarget(0, surface.value), "Native clear target");
        hr(device->SetDepthStencilSurface(nullptr), "Native clear no depth");
        hr(device->Clear(0, nullptr, D3DCLEAR_TARGET, color, 1, 0), "Native unchanged D3DCOLOR Clear");
        hr(device->GetRenderTargetData(surface.value, readback.value), "Native clear pixels");
        D3DLOCKED_RECT lock{};
        hr(readback->LockRect(&lock, nullptr, D3DLOCK_READONLY), "Native clear lock");
        const unsigned pitch = 17 * (format == D3DFMT_A16B16G16R16F ? 8 : format == D3DFMT_A8R8G8B8 ? 4 : 2);
        Bytes output(pitch * 9);
        const bool valid = lock.Pitch >= static_cast<int>(pitch);
        if (valid)
            for (unsigned y = 0; y < 9; ++y)
                std::memcpy(output.data() + y * pitch, static_cast<char *>(lock.pBits) + y * lock.Pitch,
                            pitch);
        hr(readback->UnlockRect(), "Native clear unlock");
        require(valid, "Native clear pitch");
        return output;
    }
    Bytes pixels(unsigned index, bool float_control = false) {
        auto *source = float_control ? float_surfaces.at(index).value : surfaces.at(index).value;
        auto *destination = float_control ? float_staging.at(index).value : staging.at(index).value;
        hr(device->GetRenderTargetData(source, destination), "Cache GPU readback");
        D3DLOCKED_RECT lock{};
        hr(destination->LockRect(&lock, nullptr, D3DLOCK_READONLY), "Cache readback lock");
        const unsigned pitch = size * (float_control ? 16 : index == 2 ? 4 : 8);
        Bytes result(std::size_t(pitch) * size);
        const bool valid = lock.Pitch >= static_cast<int>(pitch);
        if (valid)
            for (unsigned y = 0; y < size; ++y)
                std::memcpy(result.data() + std::size_t(y) * pitch,
                            static_cast<char *>(lock.pBits) + std::size_t(y) * lock.Pitch, pitch);
        hr(destination->UnlockRect(), "Cache readback unlock");
        require(valid, "Cache readback pitch");
        // RGBA16F word order / BGRA8 byte order, matching GlTexture raw transfers.
        return result; // Physical increasing D3D rows, no flip or upload roundtrip.
    }
};
} // namespace cache_raster_validation
