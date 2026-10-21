#pragma once
#include "material_blend_native.hpp"
#include "civ5/materials/fog_of_war_frame.hpp"
#include <functional>

namespace fow_validation {
using namespace blend_validation;
inline FogOfWarFrame load_frame(const std::filesystem::path &folder) {
    const auto commands = read(folder / "commands.bin"), constants = read(folder / "constants.bin");
    Reader r{commands}, c{constants};
    const auto passes = r.word(), draws = r.word();
    require((passes == 1 || passes == 3) && draws <= 8192, "FOW fixture queue extent");
    FogOfWarFrame frame;
    for (unsigned i = 0; i < passes; ++i) {
        FogOfWarPass pass{static_cast<FogOfWarPassKind>(r.word()), r.word(), r.word(), {}};
        const auto bytes = r.take(16);
        std::memcpy(pass.clear_color.data(), bytes.data(), 16);
        require(static_cast<unsigned>(pass.kind) == i && pass.end <= draws, "FOW fixture pass");
        frame.passes.push_back(pass);
    }
    for (unsigned i = 0; i < draws; ++i) {
        FogOfWarDraw draw{r.word(), r.word(), {}};
        const auto count = r.word();
        require(count == 96 || count == 197, "FOW fixture constants");
        draw.constants.resize(count);
        const auto bytes = c.take(count * 16);
        std::memcpy(draw.constants.data(), bytes.data(), bytes.size());
        frame.draws.push_back(std::move(draw));
    }
    r.end();
    c.end();
    return frame;
}
struct NativeFow {
    OwnWindow window;
    Com<IDirect3D9> api{Direct3DCreate9(D3D_SDK_VERSION)};
    Com<IDirect3DDevice9> device;
    std::array<Com<IDirect3DVertexShader9>, 6> vs;
    std::array<Com<IDirect3DPixelShader9>, 6> ps;
    std::array<Com<IDirect3DVertexDeclaration9>, 2> declaration;
    std::array<Com<IDirect3DVertexBuffer9>, 2> vertices;
    std::array<Com<IDirect3DIndexBuffer9>, 2> indices;
    std::array<Com<IDirect3DTexture9>, 3> clouds;
    Com<IDirect3DTexture9> mask;
    Com<IDirect3DSurface9> mask_surface, scene, depth;
    std::array<std::vector<std::array<unsigned, 4>>, 8> states;
    unsigned mask_extent, width, height;
    D3DFORMAT scene_format;
    NativeFow(const std::filesystem::path &shaders, const std::filesystem::path &fixture,
              const std::filesystem::path &assets, unsigned quality, unsigned w, unsigned h,
              bool half = false, bool single = false, IDirect3DDevice9 *shared_device = nullptr,
              IDirect3DSurface9 *shared_color = nullptr, IDirect3DSurface9 *shared_depth = nullptr)
        : mask_extent(quality == 2 ? 1024 : 512), width(w), height(h),
          scene_format(single ? D3DFMT_A32B32G32R32F
                       : half ? D3DFMT_A16B16G16R16F
                              : D3DFMT_A8R8G8B8) {
        require(api.value != nullptr, "Own FOW D3D9 API");
        D3DPRESENT_PARAMETERS pp{};
        pp.BackBufferWidth = pp.BackBufferHeight = 64;
        pp.BackBufferFormat = D3DFMT_X8R8G8B8;
        pp.BackBufferCount = 1;
        pp.SwapEffect = D3DSWAPEFFECT_DISCARD;
        pp.hDeviceWindow = window.value;
        pp.Windowed = TRUE;
        pp.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;
        if (shared_device) {
            device.value = shared_device;
            device->AddRef();
        } else {
            hr(api->CreateDevice(0, D3DDEVTYPE_HAL, window.value, D3DCREATE_HARDWARE_VERTEXPROCESSING, &pp,
                                 &device.value),
               "Own FOW HAL/HWVP");
        }
        require(bool(shared_color) == bool(shared_depth) && (!shared_color || shared_device),
                "Complete shared FOW device and targets required");
        for (unsigned i = 0; i < 6; ++i) {
            const auto stem = i == 5 ? std::string("k53-s0-") : "k52-s" + std::to_string(i) + "-";
            const auto v = read(shaders / (stem + "vertex.bin")), p = read(shaders / (stem + "pixel.bin"));
            require(v.size() % 4 == 0 && p.size() % 4 == 0 && v.size() >= 8 && p.size() >= 8,
                    "Original FOW shader extent");
            std::vector<DWORD> vw(v.size() / 4), pw(p.size() / 4);
            std::memcpy(vw.data(), v.data(), v.size());
            std::memcpy(pw.data(), p.data(), p.size());
            hr(device->CreateVertexShader(vw.data(), &vs[i].value), "Complete original FOW VS");
            hr(device->CreatePixelShader(pw.data(), &ps[i].value), "Complete original FOW PS");
        }
        for (unsigned i = 0; i < 2; ++i) {
            const auto decl = read(fixture / (i ? "declaration1.bin" : "declaration5.bin"));
            require(decl.size() % 8 == 0 && decl.size() >= 16 && decl.size() <= 136,
                    "Original FOW declaration extent");
            std::vector<D3DVERTEXELEMENT9> elements(decl.size() / 8);
            std::memcpy(elements.data(), decl.data(), decl.size());
            hr(device->CreateVertexDeclaration(elements.data(), &declaration[i].value),
               "Executed original FOW declaration");
            const auto vb = read(fixture / (i ? "4-vertex.bin" : "2-vertex.bin"));
            const auto ib = read(fixture / (i ? "3-index.bin" : "1-index.bin"));
            require(vb.size() == (i ? 3072 : 23040) && ib.size() == (i ? 576 : 9216),
                    "Original FOW geometry byte count");
            hr(device->CreateVertexBuffer(vb.size(), 0, 0, D3DPOOL_MANAGED, &vertices[i].value, nullptr),
               "Original FOW VB");
            hr(device->CreateIndexBuffer(ib.size(), 0, D3DFMT_INDEX32, D3DPOOL_MANAGED, &indices[i].value,
                                         nullptr),
               "Original FOW IB");
            void *p = nullptr;
            hr(vertices[i]->Lock(0, vb.size(), &p, 0), "FOW vertex lock");
            std::memcpy(p, vb.data(), vb.size());
            hr(vertices[i]->Unlock(), "FOW vertex unlock");
            hr(indices[i]->Lock(0, ib.size(), &p, 0), "FOW index lock");
            std::memcpy(p, ib.data(), ib.size());
            hr(indices[i]->Unlock(), "FOW index unlock");
        }
        const auto state_bytes = read(fixture / "gpu-states.bin");
        Reader r{state_bytes};
        auto magic = r.take(8);
        require(std::string_view(reinterpret_cast<const char *>(magic.data()), 8) == "C5FOWGP1",
                "FOW GPU state schema");
        for (auto &group : states) {
            const auto count = r.word();
            require(count <= 32, "Original FOW state count");
            for (unsigned i = 0; i < count; ++i)
                group.push_back({r.word(), r.word(), r.word(), r.word()});
        }
        r.end();
        hr(device->CreateTexture(mask_extent, mask_extent, 1, D3DUSAGE_RENDERTARGET, D3DFMT_A8R8G8B8,
                                 D3DPOOL_DEFAULT, &mask.value, nullptr),
           "FOW mask target");
        hr(mask->GetSurfaceLevel(0, &mask_surface.value), "FOW mask surface");
        if (shared_color) {
            scene.value = shared_color;
            depth.value = shared_depth;
            scene->AddRef();
            depth->AddRef();
            D3DSURFACE_DESC c{}, d{};
            hr(scene->GetDesc(&c), "Shared FOW color descriptor");
            hr(depth->GetDesc(&d), "Shared FOW depth descriptor");
            require(c.Width == width && c.Height == height && c.Format == scene_format && d.Width == width &&
                        d.Height == height && d.Format == D3DFMT_D24S8,
                    "Shared FOW target format/extent");
        } else {
            hr(device->CreateRenderTarget(width, height, scene_format, D3DMULTISAMPLE_NONE, 0, FALSE,
                                          &scene.value, nullptr),
               "FOW scene color");
            hr(device->CreateDepthStencilSurface(width, height, D3DFMT_D24S8, D3DMULTISAMPLE_NONE, 0, TRUE,
                                                 &depth.value, nullptr),
               "FOW scene depth");
        }
        HMODULE library = LoadLibraryExW(L"d3dx9_42.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
        require(library != nullptr, "System x64 D3DX for FOW DDS");
        try {
            using Create = HRESULT(WINAPI *)(IDirect3DDevice9 *, LPCVOID, UINT, IDirect3DTexture9 **);
            const auto create =
                reinterpret_cast<Create>(GetProcAddress(library, "D3DXCreateTextureFromFileInMemory"));
            require(create != nullptr, "D3DX FOW DDS entry");
            for (unsigned i = 0; i < (quality ? 3 : 0); ++i) {
                const auto bytes = read(assets / normalize_asset_name(FogOfWarAssets::cloud_paths[i]));
                hr(create(device.value, bytes.data(), bytes.size(), &clouds[i].value),
                   "Original authored FOW DDS");
            }
        } catch (...) {
            FreeLibrary(library);
            throw;
        }
        FreeLibrary(library);
    }
    void state(unsigned index) {
        for (auto [kind, a, b, c] : states[index]) {
            if (kind == 0)
                hr(device->SetRenderState(static_cast<D3DRENDERSTATETYPE>(a), b),
                   "Original FOW render state");
            else if (kind == 1)
                hr(device->SetSamplerState(a, static_cast<D3DSAMPLERSTATETYPE>(b), c),
                   "Original FOW sampler state");
            else if (kind == 2)
                hr(device->SetTextureStageState(a, static_cast<D3DTEXTURESTAGESTATETYPE>(b), c),
                   "Original FOW texture stage");
            else
                throw std::runtime_error("Unknown original FOW state call");
        }
    }
    Bytes texture_level(unsigned index, unsigned mip) {
        D3DSURFACE_DESC desc{};
        hr(clouds[index]->GetLevelDesc(mip, &desc), "Native FOW DDS descriptor");
        require(desc.Format == D3DFMT_DXT1 || desc.Format == D3DFMT_DXT5, "Original FOW BC format");
        const unsigned pitch = ((desc.Width + 3) / 4) * (desc.Format == D3DFMT_DXT1 ? 8 : 16),
                       rows = (desc.Height + 3) / 4;
        D3DLOCKED_RECT lock{};
        hr(clouds[index]->LockRect(mip, &lock, nullptr, D3DLOCK_READONLY), "Native FOW DDS read");
        Bytes bytes(pitch * rows);
        const bool valid = lock.pBits && lock.Pitch >= static_cast<INT>(pitch);
        if (valid)
            for (unsigned row = 0; row < rows; ++row)
                std::memcpy(bytes.data() + row * pitch, static_cast<char *>(lock.pBits) + row * lock.Pitch,
                            pitch);
        hr(clouds[index]->UnlockRect(mip), "Native FOW DDS unlock");
        require(valid, "Native FOW DDS pitch");
        return bytes;
    }
    void render(const FogOfWarFrame &frame, bool opaque_cloud_probe = false,
                const std::function<void(IDirect3DTexture9 *)> &draw_scene = {}) {
        // Each pass has its own BeginScene/EndScene, so the intervening
        // LAND/water renderers can submit through their own scene boundaries.
        for (const auto &pass : frame.passes) {
            for (unsigned i = 0; i < 16; ++i)
                hr(device->SetTexture(i, nullptr), "FOW texture reset");
            for (auto [s, v] :
                 std::initializer_list<std::pair<D3DRENDERSTATETYPE, DWORD>>{{D3DRS_LIGHTING, 0},
                                                                             {D3DRS_SCISSORTESTENABLE, 0},
                                                                             {D3DRS_DITHERENABLE, 0},
                                                                             {D3DRS_SRGBWRITEENABLE, 0},
                                                                             {D3DRS_COLORWRITEENABLE, 15},
                                                                             {D3DRS_ZWRITEENABLE, 1},
                                                                             {D3DRS_CLIPPLANEENABLE, 0},
                                                                             {D3DRS_FILLMODE, D3DFILL_SOLID}})
                hr(device->SetRenderState(s, v), "FOW explicit initial raster control");
            if (pass.kind == FogOfWarPassKind::mask) {
                hr(device->SetRenderTarget(0, scene.value), "Initial FOW scene target");
                hr(device->SetDepthStencilSurface(depth.value), "Initial FOW scene depth");
                hr(device->Clear(0, nullptr, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER | D3DCLEAR_STENCIL,
                                 D3DCOLOR_ARGB(0, 187, 51, 119), 1, 0),
                   "FOW scene sentinel");
            }
            state(6);
            state(7);
            hr(device->BeginScene(), "FOW BeginScene");
            const auto kind = static_cast<unsigned>(pass.kind);
            hr(device->SetTexture(1, nullptr), "Unbind FOW mask before pass");
            hr(device->SetDepthStencilSurface(nullptr), "Unbind FOW depth before target change");
            hr(device->SetRenderTarget(0, kind ? scene.value : mask_surface.value), "FOW pass target");
            if (kind)
                hr(device->SetDepthStencilSurface(depth.value), "FOW pass shared depth");
            D3DVIEWPORT9 viewport{0, 0, kind ? width : mask_extent, kind ? height : mask_extent, 0, 1};
            hr(device->SetViewport(&viewport), "FOW pass viewport");
            state(kind);
            state(kind + 3);
            if (kind == 2 && opaque_cloud_probe)
                hr(device->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE),
                   "Explicit unblended cloud diagnostic");
            if (kind == 0)
                hr(device->Clear(0, nullptr, D3DCLEAR_TARGET,
                                 D3DCOLOR_COLORVALUE(pass.clear_color[0], pass.clear_color[1],
                                                     pass.clear_color[2], pass.clear_color[3]),
                                 1, 0),
                   "Original FOW mask clear");
            for (unsigned index = pass.begin; index < pass.end; ++index) {
                const auto &draw = frame.draws[index];
                const unsigned group = draw.geometry < 2 ? 0 : 1;
                const unsigned technique = kind == 2 ? 5 : draw.selector;
                hr(device->SetVertexDeclaration(declaration[group].value), "FOW input declaration");
                hr(device->SetStreamSource(0, vertices[group].value, 0, group ? 32 : 40), "FOW stream");
                hr(device->SetIndices(indices[group].value), "FOW indices");
                hr(device->SetVertexShader(vs[technique].value), "Original FOW VS binding");
                hr(device->SetPixelShader(ps[technique].value), "Original FOW PS binding");
                hr(device->SetVertexShaderConstantF(0, draw.constants[0].data(), draw.constants.size()),
                   "Original FOW constants");
                if (kind == 2) {
                    hr(device->SetTexture(0, clouds[draw.geometry - 2].value), "FOW authored cloud");
                    hr(device->SetTexture(1, mask.value), "FOW generated mask");
                }
                hr(device->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, 0, group ? 96 : 576,
                                                draw.geometry == 1 ? 1152 : 0, group ? 48 : 384),
                   "Complete original FOW draw");
            }
            hr(device->EndScene(), "FOW EndScene");
            if (pass.kind == FogOfWarPassKind::mask && draw_scene)
                draw_scene(mask.value);
        }
    }
    Bytes pixels(bool mask_image) {
        const unsigned w = mask_image ? mask_extent : width, h = mask_image ? mask_extent : height;
        const auto format = mask_image ? D3DFMT_A8R8G8B8 : scene_format;
        const unsigned stride = format == D3DFMT_A8R8G8B8 ? 4 : format == D3DFMT_A16B16G16R16F ? 8 : 16;
        Com<IDirect3DSurface9> copy;
        hr(device->CreateOffscreenPlainSurface(w, h, format, D3DPOOL_SYSTEMMEM, &copy.value, nullptr),
           "FOW readback surface");
        hr(device->GetRenderTargetData(mask_image ? mask_surface.value : scene.value, copy.value),
           "FOW GPU readback");
        D3DLOCKED_RECT lock{};
        hr(copy->LockRect(&lock, nullptr, D3DLOCK_READONLY), "FOW readback lock");
        Bytes bytes(w * h * stride);
        const bool valid = lock.pBits && lock.Pitch >= static_cast<INT>(w * stride);
        if (valid)
            for (unsigned row = 0; row < h; ++row)
                std::memcpy(bytes.data() + row * w * stride,
                            static_cast<char *>(lock.pBits) + row * lock.Pitch, w * stride);
        hr(copy->UnlockRect(), "FOW readback unlock");
        require(valid, "FOW readback pitch");
        if (stride == 4)
            for (std::size_t i = 0; i < bytes.size(); i += 4)
                std::swap(bytes[i], bytes[i + 2]);
        return bytes;
    }
};
} // namespace fow_validation
