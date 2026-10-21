#pragma once
#include "water_surface_native.hpp"
#include "civ5/terrain/river_surface.hpp"
#include <tuple>
namespace river_validation {
using namespace water_validation;
struct NativeRiver : NativeSampler {
    Com<IDirect3DVertexShader9> vs;
    std::array<Com<IDirect3DPixelShader9>, 5> ps;
    Com<IDirect3DVertexDeclaration9> declaration;
    Com<IDirect3DSurface9> color, pixels, depth;
    Com<IDirect3DVertexBuffer9> vertices;
    Com<IDirect3DIndexBuffer9> indices;
    unsigned vertex_capacity = 0, triangles = 0;
    explicit NativeRiver(const std::filesystem::path &shaders, const std::filesystem::path &out,
                         IDirect3DDevice9 *shared_device = nullptr)
        : NativeSampler(shared_device) {
        for (auto [name, size, selector] :
             {std::tuple{"river-vs", 820u, 0u}, {"river-1-ps", 1400u, 1u}, {"river-4-ps", 4828u, 4u}}) {
            const auto raw = read(shaders / (std::string(name) + ".bin"));
            require(raw.size() == size, "Complete original river shader length");
            std::vector<DWORD> words(raw.size() / 4);
            std::memcpy(words.data(), raw.data(), raw.size());
            if (selector)
                checked(device->CreatePixelShader(words.data(), &ps[selector].value),
                        "Complete original river PS");
            else
                checked(device->CreateVertexShader(words.data(), &vs.value), "Complete original river VS");
            write(out / (std::string(name) + ".bin"), raw);
        }
        const D3DVERTEXELEMENT9 elements[]{
            {0, 0, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
            {0, 12, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0},
            {0, 20, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 1},
            {0, 28, D3DDECLTYPE_UBYTE4N, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_COLOR, 0},
            D3DDECL_END()};
        checked(device->CreateVertexDeclaration(elements, &declaration.value), "Original river format10");
        checked(device->CreateRenderTarget(64, 64, D3DFMT_A32B32G32R32F, D3DMULTISAMPLE_NONE, 0, FALSE,
                                           &color.value, nullptr),
                "River color");
        checked(device->CreateOffscreenPlainSurface(64, 64, D3DFMT_A32B32G32R32F, D3DPOOL_SYSTEMMEM,
                                                    &pixels.value, nullptr),
                "River readback");
        checked(device->CreateDepthStencilSurface(64, 64, D3DFMT_D24S8, D3DMULTISAMPLE_NONE, 0, TRUE,
                                                  &depth.value, nullptr),
                "River depth");
    }
    void mesh(const civ5::terrain::RiverSurfaceMesh &mesh) {
        if (vertices.value) {
            vertices.value->Release();
            vertices.value = nullptr;
        }
        if (indices.value) {
            indices.value->Release();
            indices.value = nullptr;
        }
        triangles = unsigned(mesh.indices.size() / 3);
        if (mesh.vertices.empty() || mesh.indices.empty()) {
            triangles = 0;
            vertex_capacity = 0;
            return;
        }
        const auto vb = (unsigned(mesh.vertices.size()) * 32 + 8191) & ~8191u;
        const auto ib = (unsigned(mesh.indices.size()) * 4 + 4095) & ~4095u;
        vertex_capacity = vb / 32;
        checked(device->CreateVertexBuffer(vb, 0, 0, D3DPOOL_MANAGED, &vertices.value, nullptr),
                "Original rounded river VB");
        checked(device->CreateIndexBuffer(ib, 0, D3DFMT_INDEX32, D3DPOOL_MANAGED, &indices.value, nullptr),
                "Original rounded river IB");
        void *data = nullptr;
        checked(vertices->Lock(0, 0, &data, 0), "River VB lock");
        std::memset(data, 0xa5, vb);
        std::memcpy(data, mesh.vertices.data(), mesh.vertices.size() * 32);
        checked(vertices->Unlock(), "River VB unlock");
        checked(indices->Lock(0, 0, &data, 0), "River IB lock");
        std::memset(data, 0xa5, ib);
        std::memcpy(data, mesh.indices.data(), mesh.indices.size() * 4);
        checked(indices->Unlock(), "River IB unlock");
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
               const std::array<IDirect3DBaseTexture9 *, 13> &textures, float clear_depth = 1) {
        checked(device->SetRenderTarget(0, color.value), "River target");
        checked(device->SetDepthStencilSurface(depth.value), "River inherited depth");
        D3DVIEWPORT9 viewport{0, 0, 64, 64, 0, 1};
        checked(device->SetViewport(&viewport), "River viewport");
        bind(selector, c, textures);
        checked(device->Clear(0, nullptr, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, 0x00bb3377, clear_depth, 0),
                "River sentinel and depth");
        checked(device->BeginScene(), "River begin");
    }
    // Submit to the caller's live target/depth/viewport without clearing it.
    void bind(unsigned selector, const WaterConstantBlock &c,
              const std::array<IDirect3DBaseTexture9 *, 13> &textures) {
        // Values recorded from original blend8/depth4 and scheduled-pass consumers.
        for (auto [state, value] : std::initializer_list<std::pair<unsigned, DWORD>>{
                 {8, 3},   {57, 0},  {152, 0}, {15, 0},  {27, 1},   {206, 1}, {19, 5}, {20, 6},
                 {171, 1}, {207, 2}, {208, 2}, {209, 1}, {168, 15}, {193, 0}, {7, 1},  {14, 1},
                 {23, 4},  {22, 2},  {195, 0}, {52, 0},  {174, 0},  {176, 0}})
            checked(device->SetRenderState(D3DRENDERSTATETYPE(state), value), "Original river pass state");
        checked(device->SetVertexDeclaration(declaration.value), "River vertex declaration");
        checked(device->SetVertexShader(vs.value), "River VS bind");
        checked(device->SetPixelShader(ps[selector].value), "River PS bind");
        for (unsigned slot = 0; slot < 13; ++slot) {
            checked(device->SetTexture(slot, textures[slot]), "River texture");
            const DWORD wrap =
                slot == 5 || slot == 6 || slot == 8 || slot >= 11 ? D3DTADDRESS_CLAMP : D3DTADDRESS_WRAP;
            for (auto [state, value] : std::initializer_list<std::pair<D3DSAMPLERSTATETYPE, DWORD>>{
                     {D3DSAMP_ADDRESSU, wrap},
                     {D3DSAMP_ADDRESSV, wrap},
                     {D3DSAMP_ADDRESSW, wrap},
                     {D3DSAMP_MINFILTER, 2},
                     {D3DSAMP_MAGFILTER, 2},
                     {D3DSAMP_MIPFILTER, 2},
                     {D3DSAMP_MAXANISOTROPY, 1},
                     {D3DSAMP_SRGBTEXTURE, slot == 6 ? 1u : 0u},
                     {D3DSAMP_MAXMIPLEVEL, 0},
                     {D3DSAMP_MIPMAPLODBIAS, 0}})
                checked(device->SetSamplerState(slot, state, value), "Original river sampler");
        }
        // Independent original binding spans, not the GL helper.
        std::array<float, 148> source{};
        std::copy(c.begin(), c.end(), source.begin());
        const auto world = civ5::terrain::identity_matrix();
        const std::array<float, 16> unused{};
        checked(device->SetVertexShaderConstantF(4, source.data(), 13), "River global VS");
        checked(device->SetPixelShaderConstantF(4, source.data() + 52, 24), "River global PS");
        checked(device->SetVertexShaderConstantF(0, world.data(), 4), "River World");
        checked(device->SetPixelShaderConstantF(0, unused.data(), 4), "River local PS");
    }
    void draw() {
        if (!triangles)
            return;
        checked(device->SetStreamSource(0, vertices.value, 0, 32), "River stream");
        checked(device->SetIndices(indices.value), "River indices");
        checked(device->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, 0, vertex_capacity, 0, triangles),
                "Complete original river draw");
    }
    std::vector<float> end() {
        checked(device->EndScene(), "River end");
        checked(device->GetRenderTargetData(color.value, pixels.value), "River output");
        std::vector<float> result(64 * 64 * 4);
        D3DLOCKED_RECT lock{};
        checked(pixels->LockRect(&lock, nullptr, D3DLOCK_READONLY), "River output lock");
        const bool valid = lock.Pitch >= 64 * 16;
        if (valid)
            for (unsigned y = 0; y < 64; ++y)
                std::memcpy(result.data() + y * 256, static_cast<char *>(lock.pBits) + y * lock.Pitch, 1024);
        checked(pixels->UnlockRect(), "River output unlock");
        require(valid, "River output pitch");
        return result;
    }
};
} // namespace river_validation
