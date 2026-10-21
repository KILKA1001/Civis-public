#pragma once
// Validation only: complete, unmodified, hash-pinned original surface VS/PS.
#include "civ5/materials/water_surface.hpp"
#include "texture_sampling_native.hpp"
#include <filesystem>
#include <fstream>

namespace water_validation {
using namespace texture_sampling_validation;
using namespace civ5::materials;
inline Bytes read(const std::filesystem::path &path) {
    const auto size = std::filesystem::file_size(path);
    require(size <= 32 * 1024 * 1024, "Bounded water input");
    Bytes out(static_cast<std::size_t>(size));
    std::ifstream f(path, std::ios::binary);
    f.read(reinterpret_cast<char *>(out.data()), size);
    require(bool(f), "Read water input");
    return out;
}
inline void write(const std::filesystem::path &path, std::span<const std::uint8_t> bytes) {
    require(!std::filesystem::exists(path), "Fresh water artifact required");
    std::ofstream f(path, std::ios::binary);
    f.write(reinterpret_cast<const char *>(bytes.data()), bytes.size());
    f.close();
    require(bool(f), "Write water artifact");
}
inline void verify_native_2d(IDirect3DBaseTexture9 *base, const DdsData &data) {
    require(data.face_count() == 1, "Water source verification requires 2D");
    auto *texture = static_cast<IDirect3DTexture9 *>(base);
    for (const auto &level : data.levels()) {
        const auto raw = data.payload(0, level.mip);
        D3DLOCKED_RECT lock{};
        checked(texture->LockRect(level.mip, &lock, nullptr, D3DLOCK_READONLY),
                "Water source verification lock");
        bool valid = lock.pBits && lock.Pitch >= int(level.row_pitch);
        if (valid)
            for (unsigned y = 0; y < level.height; ++y)
                valid =
                    valid && std::memcmp(static_cast<char *>(lock.pBits) + std::size_t(y) * lock.Pitch,
                                         raw.data() + std::size_t(y) * level.row_pitch, level.row_pitch) == 0;
        checked(texture->UnlockRect(level.mip), "Water source verification unlock");
        require(valid, "Water source differs in D3D9 storage");
    }
}
template <class T> void save(const std::filesystem::path &path, const T &values) {
    write(path, {reinterpret_cast<const std::uint8_t *>(values.data()), values.size() * sizeof(values[0])});
}
struct NativeWater : NativeSampler {
    Com<IDirect3DVertexShader9> vs;
    std::array<Com<IDirect3DPixelShader9>, 6> ps;
    Com<IDirect3DVertexDeclaration9> decl;
    Com<IDirect3DSurface9> color, pixels;
    explicit NativeWater(const std::filesystem::path &shaders, IDirect3DDevice9 *shared_device = nullptr)
        : NativeSampler(shared_device) {
        for (unsigned selector : {0u, 2u, 3u, 5u, 6u}) {
            const auto name = selector == 6 ? "water-surface-vs.bin"
                                            : "water-surface-" + std::to_string(selector) + "-ps.bin";
            const auto bytes = read(shaders / name);
            const auto size = selector == 6   ? 868
                              : selector == 0 ? 2436
                              : selector == 2 ? 5072
                              : selector == 3 ? 5724
                                              : 156;
            require(bytes.size() == unsigned(size), "Complete original water bytecode length");
            std::vector<DWORD> words(bytes.size() / 4);
            std::memcpy(words.data(), bytes.data(), bytes.size());
            if (selector == 6)
                checked(device->CreateVertexShader(words.data(), &vs.value), "Original water VS");
            else
                checked(device->CreatePixelShader(words.data(), &ps[selector].value), "Original water PS");
        }
        const D3DVERTEXELEMENT9 elements[]{
            {0, 0, D3DDECLTYPE_FLOAT4, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0}, D3DDECL_END()};
        checked(device->CreateVertexDeclaration(elements, &decl.value), "Original layout14 float4");
        checked(device->CreateRenderTarget(64, 64, D3DFMT_A32B32G32R32F, D3DMULTISAMPLE_NONE, 0, FALSE,
                                           &color.value, nullptr),
                "Water float target");
        checked(device->CreateOffscreenPlainSurface(64, 64, D3DFMT_A32B32G32R32F, D3DPOOL_SYSTEMMEM,
                                                    &pixels.value, nullptr),
                "Water readback target");
    }
    Com<IDirect3DTexture9> lean(const LeanData &data, LeanMap map) {
        Com<IDirect3DTexture9> texture;
        checked(device->CreateTexture(data.width(), data.height(), data.mip_count(), 0,
                                      map == LeanMap::rgba16 ? D3DFMT_A16B16G16R16 : D3DFMT_L16,
                                      D3DPOOL_MANAGED, &texture.value, nullptr),
                "Direct LEAN water texture");
        for (const auto &level : data.levels(map)) {
            const auto raw = data.payload(map, level.mip);
            for (bool verify : {false, true}) {
                D3DLOCKED_RECT lock{};
                checked(texture->LockRect(level.mip, &lock, nullptr, verify ? D3DLOCK_READONLY : 0),
                        "LEAN lock");
                bool valid = lock.Pitch >= int(level.row_pitch);
                if (valid)
                    for (unsigned y = 0; y < level.height; ++y) {
                        auto *dst = static_cast<char *>(lock.pBits) + std::size_t(y) * lock.Pitch;
                        const auto *src = raw.data() + std::size_t(y) * level.row_pitch;
                        if (verify)
                            valid = valid && std::memcmp(dst, src, level.row_pitch) == 0;
                        else
                            std::memcpy(dst, src, level.row_pitch);
                    }
                checked(texture->UnlockRect(level.mip), "LEAN unlock");
                require(valid, "LEAN storage identity");
            }
        }
        return texture;
    }
    Com<IDirect3DVolumeTexture9> volume(std::span<const float> values, unsigned size) {
        require(values.size() == std::size_t(size) * size * size * 4, "Volume extent");
        Com<IDirect3DVolumeTexture9> t;
        checked(device->CreateVolumeTexture(size, size, size, 1, 0, D3DFMT_A32B32G32R32F, D3DPOOL_MANAGED,
                                            &t.value, nullptr),
                "Water control LUT");
        for (bool verify : {false, true}) {
            D3DLOCKED_BOX lock{};
            checked(t->LockBox(0, &lock, nullptr, verify ? D3DLOCK_READONLY : 0), "LUT lock");
            bool valid = lock.RowPitch >= int(size * 16) && lock.SlicePitch >= int(size) * lock.RowPitch;
            if (valid)
                for (unsigned z = 0; z < size; ++z)
                    for (unsigned y = 0; y < size; ++y) {
                        auto *dst = static_cast<char *>(lock.pBits) + std::size_t(z) * lock.SlicePitch +
                                    std::size_t(y) * lock.RowPitch;
                        const auto *src = values.data() + (std::size_t(z) * size + y) * size * 4;
                        if (verify)
                            valid = valid && std::memcmp(dst, src, size * 16) == 0;
                        else
                            std::memcpy(dst, src, size * 16);
                    }
            checked(t->UnlockBox(0), "LUT unlock");
            require(valid, "Water LUT identity");
        }
        return t;
    }
    void begin(unsigned selector, const WaterConstantBlock &c,
               const std::array<IDirect3DBaseTexture9 *, 14> &textures) {
        checked(device->SetRenderTarget(0, color.value), "Water color target");
        checked(device->SetDepthStencilSurface(nullptr), "Water explicit no depth");
        D3DVIEWPORT9 view{0, 0, 64, 64, 0, 1};
        checked(device->SetViewport(&view), "Water viewport");
        for (auto [state, value] :
             std::initializer_list<std::pair<D3DRENDERSTATETYPE, DWORD>>{{D3DRS_ZENABLE, 0},
                                                                         {D3DRS_CULLMODE, D3DCULL_NONE},
                                                                         {D3DRS_ALPHABLENDENABLE, 0},
                                                                         {D3DRS_ALPHATESTENABLE, 0},
                                                                         {D3DRS_SCISSORTESTENABLE, 0},
                                                                         {D3DRS_DITHERENABLE, 0},
                                                                         {D3DRS_SRGBWRITEENABLE, 0},
                                                                         {D3DRS_COLORWRITEENABLE, 15}})
            checked(device->SetRenderState(state, value), "Water raster control");
        bind(selector, c, textures);
        checked(device->Clear(0, nullptr, D3DCLEAR_TARGET, 0xffbb3377, 1, 0), "Water sentinel");
        checked(device->BeginScene(), "Water begin scene");
    }
    // Retain caller-owned scene target/depth and existing LAND pixels.
    void bind(unsigned selector, const WaterConstantBlock &c,
              const std::array<IDirect3DBaseTexture9 *, 14> &textures) {
        checked(device->SetVertexDeclaration(decl.value), "Water declaration");
        checked(device->SetVertexShader(vs.value), "Water original VS binding");
        checked(device->SetPixelShader(ps[selector].value), "Water original PS binding");
        for (unsigned slot = 0; slot < 14; ++slot) {
            checked(device->SetTexture(slot, textures[slot]), "Original water resource slot");
            const auto address = slot == 6 || slot == 8 || slot == 11 ? D3DTADDRESS_CLAMP : D3DTADDRESS_WRAP;
            for (auto [state, value] : std::initializer_list<std::pair<D3DSAMPLERSTATETYPE, DWORD>>{
                     {D3DSAMP_ADDRESSU, address},
                     {D3DSAMP_ADDRESSV, address},
                     {D3DSAMP_ADDRESSW, address},
                     {D3DSAMP_MINFILTER, D3DTEXF_LINEAR},
                     {D3DSAMP_MAGFILTER, D3DTEXF_LINEAR},
                     {D3DSAMP_MIPFILTER, D3DTEXF_LINEAR},
                     {D3DSAMP_MAXMIPLEVEL, 0},
                     {D3DSAMP_MAXANISOTROPY, 1},
                     {D3DSAMP_MIPMAPLODBIAS, 0},
                     {D3DSAMP_SRGBTEXTURE, slot == 6 ? 1u : 0u}})
                checked(device->SetSamplerState(slot, state, value), "Original water sampler0/3/5");
        }
        // Original 703FCD/73CB00 binding and 8E2350 consumer, independently
        // executed by emulate_native_water_binding.py. Upload the native spans
        // directly; do not reuse the production register-mapping helper here.
        // Producer stores 145 floats of the 37-vector wrapper; its final three
        // components are unused by the complete original water shaders.
        std::array<float, 148> source{};
        std::copy(c.begin(), c.end(), source.begin());
        checked(device->SetVertexShaderConstantF(4, source.data(), 13), "Original global VS c4..16");
        checked(device->SetPixelShaderConstantF(4, source.data() + 52, 24), "Original global PS c4..27");
        const auto world = civ5::terrain::identity_matrix();
        const std::array<float, 16> unused{};
        checked(device->SetVertexShaderConstantF(0, world.data(), 4), "Local World c0..3");
        checked(device->SetPixelShaderConstantF(0, unused.data(), 4), "Unused local PS c0..3");
    }
    void draw(const WaterSurfaceMesh &mesh, unsigned first, unsigned triangles) {
        checked(device->DrawIndexedPrimitiveUP(D3DPT_TRIANGLELIST, 0, static_cast<UINT>(mesh.vertices.size()),
                                               triangles, mesh.indices.data() + first, D3DFMT_INDEX16,
                                               mesh.vertices.data(), 16),
                "Complete original water draw");
    }
    std::vector<float> end() {
        checked(device->EndScene(), "Water end scene");
        checked(device->GetRenderTargetData(color.value, pixels.value), "Water readback");
        std::vector<float> result(64 * 64 * 4);
        D3DLOCKED_RECT lock{};
        checked(pixels->LockRect(&lock, nullptr, D3DLOCK_READONLY), "Water read lock");
        const bool valid = lock.Pitch >= 1024;
        if (valid)
            for (unsigned y = 0; y < 64; ++y)
                std::memcpy(result.data() + y * 256,
                            static_cast<char *>(lock.pBits) + std::size_t(y) * lock.Pitch, 1024);
        checked(pixels->UnlockRect(), "Water read unlock");
        require(valid, "Water read pitch");
        return result;
    }
};
} // namespace water_validation
