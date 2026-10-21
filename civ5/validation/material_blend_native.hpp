#pragma once
// Own x64 D3D9 device executes COMPLETE original VS/PS bytecode supplied by a
// separate hash-pinning exporter. No original process or bytecode modification.
#include "material_blend_fixture.hpp"
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <d3d9.h>
#include <utility>

namespace blend_validation {
inline void hr(HRESULT value, const char *why) {
    if (FAILED(value))
        throw std::runtime_error(std::string(why) +
                                 " HRESULT=" + std::to_string(static_cast<std::uint32_t>(value)));
}
template <class T> struct Com {
    T *value = nullptr;
    Com() = default;
    explicit Com(T *p) : value(p) {}
    ~Com() {
        if (value)
            value->Release();
    }
    Com(const Com &) = delete;
    Com &operator=(const Com &) = delete;
    Com(Com &&other) noexcept : value(std::exchange(other.value, nullptr)) {}
    T *operator->() const { return value; }
};
struct OwnWindow {
    HWND value = CreateWindowExW(0, L"STATIC", L"Own x64 Civ5 material shader control", WS_POPUP, 0, 0, 64,
                                 64, nullptr, nullptr, nullptr, nullptr);
    OwnWindow() { require(value != nullptr, "Own D3D9 window failed"); }
    ~OwnWindow() { DestroyWindow(value); }
};
struct NativeBlend {
    OwnWindow window;
    Com<IDirect3D9> api{Direct3DCreate9(D3D_SDK_VERSION)};
    Com<IDirect3DDevice9> device;
    Com<IDirect3DVertexShader9> vertex_shader;
    Com<IDirect3DPixelShader9> pixel_shader;
    Com<IDirect3DVertexDeclaration9> declaration;
    std::array<Com<IDirect3DSurface9>, 2> targets, readbacks;
    std::vector<Com<IDirect3DTexture9>> authored_textures;
    std::array<std::array<Com<IDirect3DTexture9>, 16>, 2> float_textures;
    NativeBlend(const std::filesystem::path &shader_directory, const MaterialAssets &assets) {
        require(api.value != nullptr, "Own D3D9 API failed");
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
           "Own HAL/HWVP device");
        for (unsigned i = 0; i < 2; ++i) {
            hr(device->CreateRenderTarget(64, 64, D3DFMT_A32B32G32R32F, D3DMULTISAMPLE_NONE, 0, FALSE,
                                          &targets[i].value, nullptr),
               "Native float target");
            hr(device->CreateOffscreenPlainSurface(64, 64, D3DFMT_A32B32G32R32F, D3DPOOL_SYSTEMMEM,
                                                   &readbacks[i].value, nullptr),
               "Native float readback");
            hr(device->SetRenderTarget(i, targets[i].value), "Native MRT binding");
        }
        hr(device->SetDepthStencilSurface(nullptr), "No depth surface");
        D3DVIEWPORT9 viewport{0, 0, 64, 64, 0, 1};
        hr(device->SetViewport(&viewport), "Native viewport");
        for (auto [state, value] :
             std::initializer_list<std::pair<D3DRENDERSTATETYPE, DWORD>>{{D3DRS_ZENABLE, FALSE},
                                                                         {D3DRS_CULLMODE, D3DCULL_NONE},
                                                                         {D3DRS_ALPHABLENDENABLE, FALSE},
                                                                         {D3DRS_ALPHATESTENABLE, FALSE},
                                                                         {D3DRS_LIGHTING, FALSE},
                                                                         {D3DRS_SCISSORTESTENABLE, FALSE},
                                                                         {D3DRS_DITHERENABLE, FALSE},
                                                                         {D3DRS_SRGBWRITEENABLE, FALSE},
                                                                         {D3DRS_COLORWRITEENABLE, 15},
                                                                         {D3DRS_COLORWRITEENABLE1, 15}})
            hr(device->SetRenderState(state, value), "Native raster state");
        const auto vs = read(shader_directory / "original-vs.bin"),
                   ps = read(shader_directory / "original-ps.bin");
        require(vs.size() == 0x824 && ps.size() == 0x60c, "Unexpected original shader byte lengths");
        // memcpy into DWORD-aligned storage; no x86 executable is loaded/executed.
        std::vector<DWORD> vwords(vs.size() / 4), pwords(ps.size() / 4);
        std::memcpy(vwords.data(), vs.data(), vs.size());
        std::memcpy(pwords.data(), ps.data(), ps.size());
        hr(device->CreateVertexShader(vwords.data(), &vertex_shader.value), "Complete original VS");
        hr(device->CreatePixelShader(pwords.data(), &pixel_shader.value), "Complete original PS");
        const D3DVERTEXELEMENT9 elements[]{
            {0, 0, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
            {0, 20, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 1},
            {0, 32, D3DDECLTYPE_UBYTE4N, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 2},
            {0, 36, D3DDECLTYPE_UBYTE4N, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 3},
            {0, 40, D3DDECLTYPE_UBYTE4, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_BLENDINDICES, 0},
            {0, 44, D3DDECLTYPE_UBYTE4, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_BLENDINDICES, 1},
            D3DDECL_END()};
        hr(device->CreateVertexDeclaration(elements, &declaration.value), "Original material48 declaration");
        hr(device->SetVertexDeclaration(declaration.value), "Native declaration binding");
        hr(device->SetVertexShader(vertex_shader.value), "Native VS binding");
        hr(device->SetPixelShader(pixel_shader.value), "Native PS binding");
        authored_textures.resize(assets.textures().size() + 1);
        const auto zero = material_resource_zero();
        hr(device->CreateTexture(256, 256, 7, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED,
                                 &authored_textures[0].value, nullptr),
           "Original resource0 texture");
        for (unsigned level = 0; level < 7; ++level) {
            auto bgra = zero[level].rgba;
            for (std::size_t i = 0; i < bgra.size(); i += 4)
                std::swap(bgra[i], bgra[i + 2]);
            upload(*authored_textures[0].value, level, bgra, zero[level].width * 4, zero[level].height);
        }
        for (std::size_t i = 0; i < assets.textures().size(); ++i) {
            const auto &data = assets.textures()[i].data;
            require(data.format() == DdsFormat::dxt5 && data.face_count() == 1,
                    "Native authored profile requires raw DXT5");
            auto &texture = authored_textures[i + 1];
            hr(device->CreateTexture(data.width(), data.height(), data.mip_count(), 0, D3DFMT_DXT5,
                                     D3DPOOL_MANAGED, &texture.value, nullptr),
               "Native authored BC3 texture");
            for (const auto &level : data.levels())
                upload(*texture.value, level.mip, data.payload(0, level.mip),
                       static_cast<unsigned>(level.row_pitch), (level.height + 3) / 4);
        }
        for (unsigned pattern = 0; pattern < 2; ++pattern)
            for (unsigned layer = 0; layer < 16; ++layer) {
                auto &texture = float_textures[pattern][layer];
                hr(device->CreateTexture(4, 4, 1, 0, D3DFMT_A32B32G32R32F, D3DPOOL_MANAGED, &texture.value,
                                         nullptr),
                   "Float control texture");
                const auto data = float_texture(layer, pattern != 0);
                upload(*texture.value, 0, {reinterpret_cast<const std::uint8_t *>(data.data()), sizeof(data)},
                       64, 4);
            }
    }
    void upload(IDirect3DTexture9 &texture, unsigned mip, std::span<const std::uint8_t> data, unsigned pitch,
                unsigned rows) {
        require(data.size() == std::size_t(pitch) * rows, "Native upload extent");
        D3DLOCKED_RECT lock{};
        hr(texture.LockRect(mip, &lock, nullptr, 0), "Native texture lock");
        const bool enough = lock.Pitch >= 0 && static_cast<unsigned>(lock.Pitch) >= pitch;
        if (enough)
            for (unsigned y = 0; y < rows; ++y)
                std::memcpy(static_cast<char *>(lock.pBits) + std::size_t(y) * lock.Pitch,
                            data.data() + std::size_t(y) * pitch, pitch);
        hr(texture.UnlockRect(mip), "Native texture unlock");
        require(enough, "Native texture pitch too small");
        hr(texture.LockRect(mip, &lock, nullptr, D3DLOCK_READONLY), "Native upload verification lock");
        bool same = lock.Pitch >= 0 && static_cast<unsigned>(lock.Pitch) >= pitch;
        if (same)
            for (unsigned y = 0; y < rows; ++y)
                same = same && std::memcmp(static_cast<char *>(lock.pBits) + std::size_t(y) * lock.Pitch,
                                           data.data() + std::size_t(y) * pitch, pitch) == 0;
        hr(texture.UnlockRect(mip), "Native verification unlock");
        require(same, "Native uploaded bytes differ");
    }
    std::vector<float> draw(const MaterialPatchMesh &mesh, std::span<const float, 384> coefficients,
                            const std::array<std::uint16_t, 16> &ids, int texture_set, bool linear,
                            std::array<float, 2> offset = {0, 0}) {
        // DX9 window centers are integer, GL centers half-integer. This explicit
        // API conversion aligns the same geometry; caller offsets remain separate.
        const std::array<float, 4> native_offset{offset[0] - 1.f / 64, offset[1] + 1.f / 64, 0, 0};
        hr(device->SetVertexShaderConstantF(35, native_offset.data(), 1), "Original c35 texel offset");
        hr(device->SetVertexShaderConstantF(109, coefficients.data(), 96), "Original c109..204 coefficients");
        for (unsigned slot = 0; slot < 16; ++slot) {
            auto *texture = texture_set >= 0 ? float_textures[texture_set][slot].value
                                             : authored_textures.at(ids[slot]).value;
            hr(device->SetTexture(slot, texture), "Native sixteen texture slots");
            for (auto [state, value] : std::initializer_list<std::pair<D3DSAMPLERSTATETYPE, DWORD>>{
                     {D3DSAMP_ADDRESSU, D3DTADDRESS_WRAP},
                     {D3DSAMP_ADDRESSV, D3DTADDRESS_WRAP},
                     {D3DSAMP_ADDRESSW, D3DTADDRESS_WRAP},
                     {D3DSAMP_MINFILTER, linear ? D3DTEXF_LINEAR : D3DTEXF_POINT},
                     {D3DSAMP_MAGFILTER, linear ? D3DTEXF_LINEAR : D3DTEXF_POINT},
                     {D3DSAMP_MIPFILTER, linear ? D3DTEXF_LINEAR : D3DTEXF_NONE},
                     {D3DSAMP_MAXANISOTROPY, 1},
                     {D3DSAMP_MIPMAPLODBIAS, 0},
                     {D3DSAMP_MAXMIPLEVEL, 0},
                     {D3DSAMP_SRGBTEXTURE, FALSE}})
                hr(device->SetSamplerState(slot, state, value), "Native explicit sampler");
        }
        hr(device->Clear(0, nullptr, D3DCLEAR_TARGET, 0xffaa3377, 1, 0), "Clear native targets");
        hr(device->BeginScene(), "Native BeginScene");
        const auto drawn = device->DrawIndexedPrimitiveUP(
            D3DPT_TRIANGLELIST, 0, mesh.columns() * mesh.rows(), static_cast<UINT>(mesh.indices().size() / 3),
            mesh.indices().data(), D3DFMT_INDEX16, mesh.vertices().data(), 48);
        const auto ended = device->EndScene();
        hr(drawn, "Complete original material draw");
        hr(ended, "Native EndScene");
        std::vector<float> result(64 * 64 * 8);
        for (unsigned target = 0; target < 2; ++target) {
            hr(device->GetRenderTargetData(targets[target].value, readbacks[target].value),
               "Native MRT readback");
            D3DLOCKED_RECT lock{};
            hr(readbacks[target]->LockRect(&lock, nullptr, D3DLOCK_READONLY), "Native readback lock");
            const bool enough = lock.Pitch >= 64 * 16;
            if (enough)
                for (unsigned y = 0; y < 64; ++y)
                    std::memcpy(result.data() + target * 64 * 64 * 4 + (63 - y) * 64 * 4,
                                static_cast<char *>(lock.pBits) + std::size_t(y) * lock.Pitch, 64 * 16);
            hr(readbacks[target]->UnlockRect(), "Native readback unlock");
            require(enough, "Native readback pitch too small");
        }
        return result; // API row-order conversion only; texture bytes never flipped.
    }
};
} // namespace blend_validation
