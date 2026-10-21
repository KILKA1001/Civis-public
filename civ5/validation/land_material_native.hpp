#pragma once
#include "civ5/materials/land_material.hpp"
#include "material_blend_native.hpp"
#include <cmath>
namespace land_material_validation {
using namespace blend_validation;
struct FloatImage {
    unsigned size;
    bool volume;
    std::vector<float> values;
};
struct NativeLand {
    OwnWindow window;
    Com<IDirect3D9> api{Direct3DCreate9(D3D_SDK_VERSION)};
    Com<IDirect3DDevice9> device;
    std::array<Com<IDirect3DVertexShader9>, 2> vs;
    std::array<Com<IDirect3DPixelShader9>, 3> ps;
    Com<IDirect3DVertexDeclaration9> declaration;
    Com<IDirect3DSurface9> target, readback;
    std::uint64_t uploaded = 0;
    explicit NativeLand(const std::filesystem::path &shaders, IDirect3DDevice9 *shared_device = nullptr) {
        require(api.value != nullptr, "Own LAND D3D9 API");
        D3DPRESENT_PARAMETERS p{};
        p.BackBufferWidth = p.BackBufferHeight = 64;
        p.BackBufferFormat = D3DFMT_X8R8G8B8;
        p.BackBufferCount = 1;
        p.SwapEffect = D3DSWAPEFFECT_DISCARD;
        p.hDeviceWindow = window.value;
        p.Windowed = TRUE;
        p.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;
        if (shared_device) {
            shared_device->AddRef();
            device.value = shared_device;
        } else {
            hr(api->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, window.value,
                                 D3DCREATE_HARDWARE_VERTEXPROCESSING, &p, &device.value),
               "LAND original-shader HAL/HWVP");
        }
        const std::array<const char *, 5> files{"land-displaced-vs.bin", "land-positioned-vs.bin",
                                                "land-specular-ps.bin", "land-diffuse-ps.bin",
                                                "land-overlay-ps.bin"};
        const std::array<std::size_t, 5> sizes{1112, 748, 2256, 1728, 192};
        for (unsigned i = 0; i < 5; ++i) {
            const auto bytes = read(shaders / files[i]);
            require(bytes.size() == sizes[i], "Complete original LAND shader length");
            std::vector<DWORD> code(bytes.size() / 4);
            std::memcpy(code.data(), bytes.data(), bytes.size());
            if (i < 2)
                hr(device->CreateVertexShader(code.data(), &vs[i].value), "Original LAND VS");
            else
                hr(device->CreatePixelShader(code.data(), &ps[i - 2].value), "Original LAND PS");
        }
        const D3DVERTEXELEMENT9 elements[]{
            {0, 0, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
            {0, 12, D3DDECLTYPE_FLOAT16_4, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_NORMAL, 0},
            {0, 20, D3DDECLTYPE_FLOAT16_2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0},
            D3DDECL_END()};
        hr(device->CreateVertexDeclaration(elements, &declaration.value), "LAND format10 declaration");
        hr(device->SetVertexDeclaration(declaration.value), "LAND declaration binding");
        hr(device->CreateRenderTarget(64, 64, D3DFMT_A32B32G32R32F, D3DMULTISAMPLE_NONE, 0, FALSE,
                                      &target.value, nullptr),
           "LAND explicit float target");
        hr(device->CreateOffscreenPlainSurface(64, 64, D3DFMT_A32B32G32R32F, D3DPOOL_SYSTEMMEM,
                                               &readback.value, nullptr),
           "LAND readback");
        begin_target();
    }
    void begin_target() {
        hr(device->SetVertexDeclaration(declaration.value), "LAND declaration for shared device");
        hr(device->SetRenderTarget(0, target.value), "LAND target");
        hr(device->SetDepthStencilSurface(nullptr), "LAND no depth");
        D3DVIEWPORT9 view{0, 0, 64, 64, 0, 1};
        hr(device->SetViewport(&view), "LAND viewport");
        for (auto [state, value] :
             std::initializer_list<std::pair<D3DRENDERSTATETYPE, DWORD>>{{D3DRS_ZENABLE, 0},
                                                                         {D3DRS_CULLMODE, D3DCULL_NONE},
                                                                         {D3DRS_ALPHABLENDENABLE, 0},
                                                                         {D3DRS_ALPHATESTENABLE, 0},
                                                                         {D3DRS_SCISSORTESTENABLE, 0},
                                                                         {D3DRS_DITHERENABLE, 0},
                                                                         {D3DRS_LIGHTING, 0},
                                                                         {D3DRS_SRGBWRITEENABLE, 0},
                                                                         {D3DRS_COLORWRITEENABLE, 15}})
            hr(device->SetRenderState(state, value), "LAND raster state");
        for (DWORD stage : {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 257}) {
            const bool nearest = stage == 257, repeat = nearest || (stage >= 3 && stage <= 5);
            for (auto [state, value] : std::initializer_list<std::pair<D3DSAMPLERSTATETYPE, DWORD>>{
                     {D3DSAMP_ADDRESSU, repeat ? D3DTADDRESS_WRAP : D3DTADDRESS_CLAMP},
                     {D3DSAMP_ADDRESSV, repeat ? D3DTADDRESS_WRAP : D3DTADDRESS_CLAMP},
                     {D3DSAMP_ADDRESSW, repeat ? D3DTADDRESS_WRAP : D3DTADDRESS_CLAMP},
                     {D3DSAMP_MAGFILTER, nearest ? D3DTEXF_POINT : D3DTEXF_LINEAR},
                     {D3DSAMP_MINFILTER, nearest ? D3DTEXF_POINT : D3DTEXF_LINEAR},
                     {D3DSAMP_MIPFILTER, nearest ? D3DTEXF_POINT : D3DTEXF_LINEAR},
                     {D3DSAMP_MAXMIPLEVEL, 0},
                     {D3DSAMP_MAXANISOTROPY, 1},
                     {D3DSAMP_SRGBTEXTURE, 0}})
                hr(device->SetSamplerState(stage, state, value), "Original LAND sampler5/0/9");
        }
    }
    Com<IDirect3DBaseTexture9> image(const FloatImage &source) {
        const auto size = source.size;
        require(source.values.size() == std::size_t(size) * size * (source.volume ? size : 1) * 4,
                "LAND float source extent");
        if (source.volume) {
            Com<IDirect3DVolumeTexture9> texture;
            hr(device->CreateVolumeTexture(size, size, size, 1, 0, D3DFMT_A32B32G32R32F, D3DPOOL_MANAGED,
                                           &texture.value, nullptr),
               "LAND explicit LUT");
            for (bool verify : {false, true}) {
                D3DLOCKED_BOX box{};
                hr(texture->LockBox(0, &box, nullptr, verify ? D3DLOCK_READONLY : 0), "LAND volume lock");
                bool valid =
                    box.RowPitch >= int(size * 16) && box.SlicePitch >= std::int64_t(box.RowPitch) * size;
                if (valid)
                    for (unsigned z = 0; z < size; ++z)
                        for (unsigned y = 0; y < size; ++y) {
                            auto *dst = static_cast<char *>(box.pBits) + std::size_t(z) * box.SlicePitch +
                                        std::size_t(y) * box.RowPitch;
                            const auto *src = source.values.data() + (std::size_t(z) * size + y) * size * 4;
                            if (verify)
                                valid = valid && std::memcmp(dst, src, size * 16) == 0;
                            else
                                std::memcpy(dst, src, size * 16);
                        }
                hr(texture->UnlockBox(0), "LAND volume unlock");
                require(valid, "LAND volume upload identity");
            }
            uploaded += source.values.size() * 4;
            return Com<IDirect3DBaseTexture9>(std::exchange(texture.value, nullptr));
        }
        Com<IDirect3DTexture9> texture;
        hr(device->CreateTexture(size, size, 1, 0, D3DFMT_A32B32G32R32F, D3DPOOL_MANAGED, &texture.value,
                                 nullptr),
           "LAND explicit 2D");
        for (bool verify : {false, true}) {
            D3DLOCKED_RECT lock{};
            hr(texture->LockRect(0, &lock, nullptr, verify ? D3DLOCK_READONLY : 0), "LAND image lock");
            bool valid = lock.Pitch >= int(size * 16);
            if (valid)
                for (unsigned y = 0; y < size; ++y) {
                    auto *dst = static_cast<char *>(lock.pBits) + std::size_t(y) * lock.Pitch;
                    const auto *src = source.values.data() + std::size_t(y) * size * 4;
                    if (verify)
                        valid = valid && std::memcmp(dst, src, size * 16) == 0;
                    else
                        std::memcpy(dst, src, size * 16);
                }
            hr(texture->UnlockRect(0), "LAND image unlock");
            require(valid, "LAND image upload identity");
        }
        uploaded += source.values.size() * 4;
        return Com<IDirect3DBaseTexture9>(std::exchange(texture.value, nullptr));
    }
    Com<IDirect3DTexture9> height(std::span<const std::uint8_t> bytes, unsigned size) {
        require(bytes.size() == std::size_t(size) * size, "LAND height extent");
        Com<IDirect3DTexture9> texture;
        hr(device->CreateTexture(size, size, 1, 0, D3DFMT_A8, D3DPOOL_MANAGED, &texture.value, nullptr),
           "LAND original A8");
        for (bool verify : {false, true}) {
            D3DLOCKED_RECT lock{};
            hr(texture->LockRect(0, &lock, nullptr, verify ? D3DLOCK_READONLY : 0), "LAND A8 lock");
            bool valid = lock.Pitch >= int(size);
            if (valid)
                for (unsigned y = 0; y < size; ++y) {
                    auto *dst = static_cast<char *>(lock.pBits) + std::size_t(y) * lock.Pitch;
                    const auto *src = bytes.data() + std::size_t(y) * size;
                    if (verify)
                        valid = valid && std::memcmp(dst, src, size) == 0;
                    else
                        std::memcpy(dst, src, size);
                }
            hr(texture->UnlockRect(0), "LAND A8 unlock");
            require(valid, "LAND A8 upload identity");
        }
        uploaded += bytes.size();
        return texture;
    }
    void bind(const LandMaterialMesh &mesh, unsigned selector, const LandMaterialConstants &c,
              const std::array<IDirect3DBaseTexture9 *, 10> &images, IDirect3DTexture9 *height,
              IDirect3DPixelShader9 *diagnostic = nullptr) {
        c.validate();
        const auto technique = land_technique(selector);
        require(mesh.input() == technique.input, "Native LAND position convention mismatch");
        hr(device->SetVertexShader(vs[selector < 3 ? 0 : 1].value), "Original LAND selected VS");
        hr(device->SetPixelShader(diagnostic ? diagnostic
                                             : ps[selector == 0                    ? 0
                                                  : selector == 1 || selector == 3 ? 1
                                                                                   : 2]
                                                   .value),
           "Original LAND selected PS");
        for (unsigned i = 0; i < 10; ++i)
            hr(device->SetTexture(i, images[i]), "LAND ten pixel resources");
        hr(device->SetTexture(257, selector < 3 ? height : nullptr), "LAND displacement binding");
        for (auto [start, value] : std::initializer_list<std::pair<unsigned, const LandMatrix *>>{
                 {0, &c.view_projection}, {4, &c.composite_projection}, {35, &c.world}})
            hr(device->SetVertexShaderConstantF(start, (*value)[0].data(), 4), "LAND VS matrices");
        for (auto [start, value] : std::initializer_list<std::pair<unsigned, const LandVector *>>{
                 {9, &c.fow_scale}, {16, &c.world_scales}, {22, &c.height_texture_scale}, {39, &c.offset}})
            hr(device->SetVertexShaderConstantF(start, value->data(), 1), "LAND VS vectors");
        hr(device->SetPixelShaderConstantF(15, c.shadow_projection[0].data(), 4), "LAND shadow matrix");
        for (auto [start, value] : std::initializer_list<std::pair<unsigned, const LandVector *>>{
                 {3, &c.camera}, {21, &c.fog_color}, {22, &c.fog}})
            hr(device->SetPixelShaderConstantF(start, value->data(), 1), "LAND PS vectors");
        for (auto [start, value] : std::initializer_list<std::pair<unsigned, float>>{
                 {9, c.shadow_fade}, {11, c.apply_specular}, {12, c.apply_diffuse}}) {
            const LandVector word{value, 0, 0, 0};
            hr(device->SetPixelShaderConstantF(start, word.data(), 1), "LAND PS toggles");
        }
    }
    void submit(const LandMaterialMesh &mesh) {
        hr(device->BeginScene(), "LAND begin");
        const auto result =
            device->DrawIndexedPrimitiveUP(D3DPT_TRIANGLELIST, 0, static_cast<UINT>(mesh.vertices().size()),
                                           static_cast<UINT>(mesh.indices().size() / 3),
                                           mesh.indices().data(), D3DFMT_INDEX16, mesh.vertices().data(), 24);
        const auto end = device->EndScene();
        hr(result, "Complete original LAND draw");
        hr(end, "LAND end");
    }
    std::vector<float> draw(const LandMaterialMesh &mesh, unsigned selector, const LandMaterialConstants &c,
                            const std::array<IDirect3DBaseTexture9 *, 10> &images, IDirect3DTexture9 *height,
                            IDirect3DPixelShader9 *diagnostic = nullptr) {
        bind(mesh, selector, c, images, height, diagnostic);
        hr(device->Clear(0, nullptr, D3DCLEAR_TARGET, 0xffbb3377, 1, 0), "LAND sentinel clear");
        submit(mesh);
        hr(device->GetRenderTargetData(target.value, readback.value), "LAND readback transfer");
        std::vector<float> out(64 * 64 * 4);
        D3DLOCKED_RECT lock{};
        hr(readback->LockRect(&lock, nullptr, D3DLOCK_READONLY), "LAND readback lock");
        const bool valid = lock.Pitch >= 1024;
        if (valid)
            for (unsigned y = 0; y < 64; ++y)
                std::memcpy(out.data() + y * 256,
                            static_cast<char *>(lock.pBits) + std::size_t(y) * lock.Pitch, 1024);
        hr(readback->UnlockRect(), "LAND readback unlock");
        require(valid, "LAND output pitch");
        return out;
    }
};
} // namespace land_material_validation
