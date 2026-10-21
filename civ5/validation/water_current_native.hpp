#pragma once
// Validation only: unchanged original shader pair and independently executed quad.
#include "civ5/materials/water_current.hpp"
#include "water_surface_native.hpp"
namespace water_current_validation {
using water_validation::read;
using texture_sampling_validation::Com;
using texture_sampling_validation::require;
using texture_sampling_validation::checked;
using texture_sampling_validation::NativeSampler;
using civ5::assets::Bytes;
using civ5::materials::WaterCurrentConstants;
namespace fs = std::filesystem;
constexpr unsigned extent = 256;
struct NativeCurrent : NativeSampler {
    Com<IDirect3DVertexShader9> vs;
    Com<IDirect3DPixelShader9> ps;
    Com<IDirect3DVertexDeclaration9> declaration;
    std::array<Com<IDirect3DTexture9>, 2> maps;
    std::array<Com<IDirect3DSurface9>, 2> color, pixels;
    std::array<float, 32> vertices{};
    std::array<std::uint32_t, 6> indices{};
    NativeCurrent(const fs::path &shaders, const fs::path &fixture, IDirect3DDevice9 *shared = nullptr)
        : NativeSampler(shared) {
        const auto vertex = read(shaders / "current-vs.bin"), pixel = read(shaders / "current-ps.bin");
        require(vertex.size() == 0xcc && pixel.size() == 0x758, "Complete original current shaders");
        std::vector<DWORD> v(vertex.size() / 4), p(pixel.size() / 4);
        std::memcpy(v.data(), vertex.data(), vertex.size());
        std::memcpy(p.data(), pixel.data(), pixel.size());
        checked(device->CreateVertexShader(v.data(), &vs.value), "Original current VS");
        checked(device->CreatePixelShader(p.data(), &ps.value), "Original current PS3");
        const auto vb = read(fixture / "vertices.bin"), ib = read(fixture / "indices.bin"),
                   decl = read(fixture / "declaration.bin");
        require(vb.size() == sizeof(vertices) && ib.size() == sizeof(indices) && decl.size() == 32,
                "Executed original shared-quad streams");
        std::memcpy(vertices.data(), vb.data(), vb.size());
        std::memcpy(indices.data(), ib.data(), ib.size());
        std::array<D3DVERTEXELEMENT9, 4> elements{};
        std::memcpy(elements.data(), decl.data(), decl.size());
        checked(device->CreateVertexDeclaration(elements.data(), &declaration.value), "Original layout1");
        for (unsigned i = 0; i < 2; ++i) {
            const auto format = i == 0 ? D3DFMT_A32B32G32R32F : D3DFMT_A8R8G8B8;
            checked(device->CreateTexture(extent, extent, 1, D3DUSAGE_RENDERTARGET, format,
                                           D3DPOOL_DEFAULT, &maps[i].value, nullptr), "Current target texture");
            checked(maps[i]->GetSurfaceLevel(0, &color[i].value), "Current target surface");
            checked(device->CreateOffscreenPlainSurface(extent, extent, format, D3DPOOL_SYSTEMMEM,
                                                        &pixels[i].value, nullptr), "Current readback");
        }
    }
    void upload(IDirect3DTexture9 *texture, unsigned level, std::span<const std::uint8_t> bytes,
                unsigned width, unsigned height, unsigned bpp) {
        require(bytes.size() == std::size_t(width) * height * bpp, "Current source byte extent");
        for (bool verify : {false, true}) {
            D3DLOCKED_RECT lock{};
            checked(texture->LockRect(level, &lock, nullptr, verify ? D3DLOCK_READONLY : 0), "Current input lock");
            bool valid = lock.pBits && lock.Pitch >= int(width * bpp);
            if (valid)
                for (unsigned y = 0; y < height; ++y) {
                    auto *dst = static_cast<char *>(lock.pBits) + std::size_t(y) * lock.Pitch;
                    const auto *src = bytes.data() + std::size_t(y) * width * bpp;
                    if (verify)
                        valid = valid && std::memcmp(dst, src, width * bpp) == 0;
                    else
                        std::memcpy(dst, src, width * bpp);
                }
            checked(texture->UnlockRect(level), "Current input unlock");
            require(valid, "Current source storage identity");
        }
    }
    Bytes render(const WaterCurrentConstants &c, IDirect3DBaseTexture9 *source, unsigned format,
                 IDirect3DPixelShader9 *diagnostic = nullptr) {
        checked(device->SetRenderTarget(0, color[format].value), "Current target binding");
        checked(device->SetDepthStencilSurface(nullptr), "Current no depth control");
        D3DVIEWPORT9 viewport{0, 0, extent, extent, 0, 1};
        checked(device->SetViewport(&viewport), "Current viewport");
        for (auto [state, value] : std::initializer_list<std::pair<D3DRENDERSTATETYPE, DWORD>>{
                 {D3DRS_ZENABLE, 0}, {D3DRS_CULLMODE, D3DCULL_NONE}, {D3DRS_ALPHABLENDENABLE, 0},
                 {D3DRS_ALPHATESTENABLE, 0}, {D3DRS_SCISSORTESTENABLE, 0}, {D3DRS_DITHERENABLE, 0},
                 {D3DRS_SRGBWRITEENABLE, 0}, {D3DRS_COLORWRITEENABLE, 15}})
            checked(device->SetRenderState(state, value), "Explicit current raster control");
        for (auto [state, value] : std::initializer_list<std::pair<D3DSAMPLERSTATETYPE, DWORD>>{
                 {D3DSAMP_ADDRESSU, D3DTADDRESS_WRAP}, {D3DSAMP_ADDRESSV, D3DTADDRESS_WRAP},
                 {D3DSAMP_ADDRESSW, D3DTADDRESS_WRAP}, {D3DSAMP_MINFILTER, D3DTEXF_LINEAR},
                 {D3DSAMP_MAGFILTER, D3DTEXF_LINEAR}, {D3DSAMP_MIPFILTER, D3DTEXF_LINEAR},
                 {D3DSAMP_MAXMIPLEVEL, 0}, {D3DSAMP_MAXANISOTROPY, 1}, {D3DSAMP_MIPMAPLODBIAS, 0},
                 {D3DSAMP_SRGBTEXTURE, 0}})
            checked(device->SetSamplerState(0, state, value), "Original sampler0");
        checked(device->SetVertexDeclaration(declaration.value), "Original current declaration");
        checked(device->SetVertexShader(vs.value), "Original current VS binding");
        checked(device->SetPixelShader(diagnostic ? diagnostic : ps.value), "Current PS binding");
        checked(device->SetPixelShaderConstantF(0, c.data(), 4), "Original current CTAB");
        checked(device->SetTexture(0, source), "Current source binding");
        checked(device->Clear(0, nullptr, D3DCLEAR_TARGET, 0xffbb3377, 1, 0), "Current sentinel");
        checked(device->BeginScene(), "Current begin");
        const auto draw = device->DrawIndexedPrimitiveUP(D3DPT_TRIANGLELIST, 0, 4, 2, indices.data(),
                                                         D3DFMT_INDEX32, vertices.data(), 32);
        checked(device->EndScene(), "Current end");
        checked(draw, "Original current quad");
        checked(device->GetRenderTargetData(color[format].value, pixels[format].value), "Current readback");
        const unsigned bpp = format == 0 ? 16 : 4;
        Bytes result(extent * extent * bpp);
        D3DLOCKED_RECT lock{};
        checked(pixels[format]->LockRect(&lock, nullptr, D3DLOCK_READONLY), "Current read lock");
        const bool valid = lock.pBits && lock.Pitch >= int(extent * bpp);
        if (valid)
            for (unsigned y = 0; y < extent; ++y)
                std::memcpy(result.data() + y * extent * bpp,
                            static_cast<char *>(lock.pBits) + std::size_t(y) * lock.Pitch, extent * bpp);
        checked(pixels[format]->UnlockRect(), "Current read unlock");
        require(valid, "Current output pitch");
        if (format == 1)
            for (std::size_t i = 0; i < result.size(); i += 4)
                std::swap(result[i], result[i + 2]);
        return result;
    }
};
} // namespace water_current_validation
