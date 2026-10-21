#include "civ5/assets/dds.hpp"
#include "civ5/ui/minimap_ice_probe_mesh.hpp"
#include <array>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <d3d9.h>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>
#include <windows.h>

namespace {
template <class T> struct Com {
    T *value = nullptr;
    ~Com() { if (value) value->Release(); }
    Com(const Com &) = delete;
    Com &operator=(const Com &) = delete;
    Com() = default;
    T *operator->() const { return value; }
};
void checked(HRESULT result, const char *operation) {
    if (FAILED(result))
        throw std::runtime_error(std::string(operation) + ": HRESULT " +
                                 std::to_string(static_cast<std::uint32_t>(result)));
}
std::vector<std::uint8_t> read(const std::filesystem::path &path) {
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file) throw std::runtime_error("DX9 minimap input is missing");
    const auto length = file.tellg();
    if (length <= 0 || length > 1024 * 1024)
        throw std::runtime_error("DX9 minimap input extent is invalid");
    std::vector<std::uint8_t> data(static_cast<std::size_t>(length));
    file.seekg(0);
    file.read(reinterpret_cast<char *>(data.data()), length);
    if (!file) throw std::runtime_error("DX9 minimap input is truncated");
    return data;
}
std::vector<DWORD> shader(const std::filesystem::path &path, DWORD version) {
    const auto bytes = read(path);
    if (bytes.size() < 8 || bytes.size() % sizeof(DWORD) ||
        std::uint32_t(bytes[0]) + (std::uint32_t(bytes[1]) << 8) +
                (std::uint32_t(bytes[2]) << 16) + (std::uint32_t(bytes[3]) << 24) != version)
        throw std::runtime_error("Original minimap shader version is invalid");
    std::vector<DWORD> words(bytes.size() / sizeof(DWORD));
    std::memcpy(words.data(), bytes.data(), bytes.size());
    return words;
}
struct Window {
    HWND value = CreateWindowExW(0, L"STATIC", L"Civ5 minimap DX9 water control",
                                 WS_POPUP, 0, 0, 32, 32, nullptr, nullptr, nullptr, nullptr);
    Window() { if (!value) throw std::runtime_error("DX9 minimap window creation failed"); }
    ~Window() { DestroyWindow(value); }
};
} // namespace

int main(int argc, char **argv) {
    try {
        const bool ice_mesh = (argc == 9 || argc == 10) &&
                              std::string(argv[1]) == "--ice-mesh";
        const bool ice = ice_mesh || (argc == 12 && std::string(argv[1]) == "--ice");
        if (!ice && argc != 7)
            throw std::invalid_argument("Usage: civ5_minimap_dx9_water [--ice|--ice-mesh] <input-folder> <output.ppm> <minX> <minY> <maxX> <maxY> [<iceMinX> <iceMinY> <iceMaxX> <iceMaxY>|<mesh.bin> [iceOffsetX]]");
        const int first = ice ? 2 : 1;
        const std::filesystem::path input(argv[first]), output(argv[first + 1]);
        const float min_x = std::stof(argv[first + 2]), min_y = std::stof(argv[first + 3]);
        const float max_x = std::stof(argv[first + 4]), max_y = std::stof(argv[first + 5]);
        if (!(max_x > min_x && max_y > min_y))
            throw std::invalid_argument("Invalid minimap world bounds");
        const auto probe = ice_mesh ? std::optional(
            civ5::ui::read_minimap_ice_probe_mesh(argv[8])) : std::nullopt;
        const float ice_offset_x = ice_mesh && argc == 10 ? std::stof(argv[9]) : 0.0f;
        const std::array<float, 4> ice_box = probe ? probe->box : ice ?
            std::array<float, 4>{std::stof(argv[8]), std::stof(argv[9]),
                                 std::stof(argv[10]), std::stof(argv[11])} :
            std::array<float, 4>{};
        if (ice && (!(ice_box[2] > ice_box[0] && ice_box[3] > ice_box[1]) ||
                    !std::isfinite(ice_offset_x)))
            throw std::invalid_argument("Invalid minimap ice region bounds");
        const auto vs_bytes = shader(input / (ice ? "kind9-vertex.bin" : "kind7-vertex.bin"),
                                     0xFFFE0300u);
        const auto ps_bytes = shader(input / (ice ? "kind9-pixel.bin" : "kind7-pixel.bin"),
                                     0xFFFF0300u);
        auto texture_source = civ5::assets::DdsData::parse(
            read(input / (ice ? "minimapice.dds" : "minimapwater.dds")));
        const int texture_size = ice ? 128 : 256, mip_count = ice ? 1 : 9;
        if (texture_source.width() != texture_size ||
            texture_source.height() != texture_size ||
            texture_source.mip_count() != mip_count ||
            texture_source.bits_per_pixel() != 32 || texture_source.face_count() != 1)
            throw std::runtime_error("Retail minimap texture DDS layout changed");

        Window window;
        Com<IDirect3D9> api;
#if defined(CIV5_MINIMAP_DX9EX_DIAGNOSTIC)
        IDirect3D9Ex *api_ex = nullptr;
        checked(Direct3DCreate9Ex(D3D_SDK_VERSION, &api_ex),
                "D3D9Ex diagnostic API creation");
        api.value = api_ex;
#else
        api.value = Direct3DCreate9(D3D_SDK_VERSION);
#endif
        if (!api.value) throw std::runtime_error("D3D9 API creation failed");
        D3DPRESENT_PARAMETERS pp{};
        pp.BackBufferWidth = 320;
        pp.BackBufferHeight = 190;
        pp.BackBufferFormat = D3DFMT_X8R8G8B8;
        pp.BackBufferCount = 1;
        pp.SwapEffect = D3DSWAPEFFECT_DISCARD;
        pp.hDeviceWindow = window.value;
        pp.Windowed = TRUE;
        pp.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;
        Com<IDirect3DDevice9> device;
        bool software_vertex_processing = false;
        auto device_result = api->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL,
                                               window.value,
                                               D3DCREATE_HARDWARE_VERTEXPROCESSING,
                                               &pp, &device.value);
        if (FAILED(device_result)) {
            software_vertex_processing = true;
            device_result = api->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL,
                                               window.value,
                                               D3DCREATE_SOFTWARE_VERTEXPROCESSING,
                                               &pp, &device.value);
        }
        checked(device_result, "Original DX9 HAL device");
        Com<IDirect3DSurface9> target, readback;
        checked(device->CreateRenderTarget(320, 190, D3DFMT_A8R8G8B8,
                                           D3DMULTISAMPLE_NONE, 0, FALSE, &target.value, nullptr),
                "Minimap DX9 target");
        checked(device->CreateOffscreenPlainSurface(320, 190, D3DFMT_A8R8G8B8,
                                                    D3DPOOL_SYSTEMMEM, &readback.value, nullptr),
                "Minimap DX9 readback");
        checked(device->SetRenderTarget(0, target.value), "Minimap DX9 target binding");
        checked(device->SetDepthStencilSurface(nullptr), "Minimap DX9 depth disable");
        checked(device->SetRenderState(D3DRS_ZENABLE, FALSE), "Minimap DX9 Z state");
        checked(device->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE), "Minimap DX9 culling");
        checked(device->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE), "Minimap DX9 blending");
        checked(device->SetRenderState(D3DRS_SRGBWRITEENABLE, FALSE), "Minimap DX9 sRGB write");
        checked(device->SetRenderState(D3DRS_COLORWRITEENABLE, 15), "Minimap DX9 color mask");
        checked(device->Clear(0, nullptr, D3DCLEAR_TARGET, 0xFF000000u, 1.0f, 0),
                "Minimap DX9 opaque clear");
        const D3DVIEWPORT9 viewport{3, 0, 313, 190, 0.0f, 1.0f};
        checked(device->SetViewport(&viewport), "Minimap DX9 active viewport");

        Com<IDirect3DVertexShader9> vertex;
        Com<IDirect3DPixelShader9> pixel;
        checked(device->CreateVertexShader(vs_bytes.data(), &vertex.value), "Original minimap VS");
        checked(device->CreatePixelShader(ps_bytes.data(), &pixel.value), "Original minimap PS");
        const D3DVERTEXELEMENT9 elements[] = {
            {0, 0, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0},
            D3DDECL_END()};
        Com<IDirect3DVertexDeclaration9> declaration;
        checked(device->CreateVertexDeclaration(elements, &declaration.value),
                "Original kind-7 vertex input declaration");
        checked(device->SetVertexDeclaration(declaration.value), "Minimap DX9 declaration");
        checked(device->SetVertexShader(vertex.value), "Minimap DX9 VS");
        checked(device->SetPixelShader(pixel.value), "Minimap DX9 PS");
        const float sx = 2.0f / (max_x - min_x), sy = 2.0f / (max_y - min_y);
        const float vs_constants[4][4] = {
            {(min_x + max_x) * 0.5f * sx, (min_y + max_y) * 0.5f * sy, 0, 0},
            {sx, sy, 0, 0}, {min_x, min_y, 0, 0}, {max_x, max_y, 0, 0}};
        if (ice) {
            const float ice_constants[5][4]{
                {ice_offset_x, 0, 0, 0},
                {ice_box[0], ice_box[1], 0, 0},
                {ice_box[2], ice_box[3], 0, 0},
                {vs_constants[0][0], vs_constants[0][1], 0, 0},
                {sx, sy, 0, 0}};
            checked(device->SetVertexShaderConstantF(0, ice_constants[0], 5),
                    "Original minimap ice box and visible-bounds constants");
        } else {
            checked(device->SetVertexShaderConstantF(3, vs_constants[0], 4),
                    "Original minimap visible-bounds constants");
        }
        const float ps_constant[4]{0.002000000095f, ice ? 176.0f : 100.0f, 0, 0};
        checked(device->SetPixelShaderConstantF(ice ? 5 : 3, ps_constant, 1),
                "Original minimap texture tiling constants");

        Com<IDirect3DTexture9> texture;
#if defined(CIV5_MINIMAP_DX9EX_DIAGNOSTIC)
        constexpr DWORD texture_usage = D3DUSAGE_DYNAMIC;
        constexpr D3DPOOL texture_pool = D3DPOOL_DEFAULT;
#else
        constexpr DWORD texture_usage = 0;
        constexpr D3DPOOL texture_pool = D3DPOOL_MANAGED;
#endif
        checked(device->CreateTexture(texture_size, texture_size, mip_count,
                                      texture_usage, D3DFMT_A8R8G8B8,
                                      texture_pool, &texture.value, nullptr),
                "Original minimap texture");
        for (const auto &level : texture_source.levels()) {
            D3DLOCKED_RECT lock{};
            checked(texture->LockRect(level.mip, &lock, nullptr, 0), "Minimap mip lock");
            const auto bytes = texture_source.payload(0, level.mip);
            if (lock.Pitch < static_cast<INT>(level.row_pitch))
                throw std::runtime_error("Water mip row pitch is too small");
            for (std::size_t y = 0; y < level.height; ++y)
                std::memcpy(static_cast<std::uint8_t *>(lock.pBits) + y * lock.Pitch,
                            bytes.data() + y * level.row_pitch,
                            static_cast<std::size_t>(level.row_pitch));
            checked(texture->UnlockRect(level.mip), "Minimap mip unlock");
        }
        const DWORD sampler_slot = ice ? 6 : 2;
        checked(device->SetTexture(sampler_slot, texture.value), "Original minimap texture slot");
        for (const auto state : {D3DSAMP_ADDRESSU, D3DSAMP_ADDRESSV, D3DSAMP_ADDRESSW})
            checked(device->SetSamplerState(sampler_slot, state, D3DTADDRESS_WRAP), "Native sampler address");
        for (const auto state : {D3DSAMP_MINFILTER, D3DSAMP_MAGFILTER, D3DSAMP_MIPFILTER})
            checked(device->SetSamplerState(sampler_slot, state, D3DTEXF_LINEAR), "Native sampler filtering");
        checked(device->SetSamplerState(sampler_slot, D3DSAMP_MAXANISOTROPY, 1), "Native sampler anisotropy");
        checked(device->SetSamplerState(sampler_slot, D3DSAMP_SRGBTEXTURE, TRUE), "Native sampler sRGB decode");
        const float quad[4][2]{{ice ? -1.0f : 0.0f, ice ? -1.0f : 0.0f},
                               {1.0f, ice ? -1.0f : 0.0f},
                               {ice ? -1.0f : 0.0f, 1.0f}, {1.0f, 1.0f}};
        checked(device->BeginScene(), "Begin original minimap water pass");
        const auto drawn = probe ?
            device->DrawPrimitiveUP(D3DPT_TRIANGLELIST,
                                    static_cast<UINT>(probe->normalized_triangles.size() / 3),
                                    probe->normalized_triangles.data(),
                                    sizeof(civ5::ui::MinimapPoint2)) :
            device->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, quad, sizeof(quad[0]));
        checked(device->EndScene(), "End original minimap water pass");
        checked(drawn, "Draw original minimap water pass");
        checked(device->GetRenderTargetData(target.value, readback.value),
                "Original minimap water GPU readback");
        D3DLOCKED_RECT lock{};
        checked(readback->LockRect(&lock, nullptr, D3DLOCK_READONLY), "Minimap readback lock");
        std::ofstream image(output, std::ios::binary);
        image << "P6\n320 190\n255\n";
        for (int y = 0; y < 190; ++y) {
            const auto *row = static_cast<const std::uint8_t *>(lock.pBits) + y * lock.Pitch;
            for (int x = 0; x < 320; ++x) {
                const char rgb[3]{static_cast<char>(row[x * 4 + 2]),
                                  static_cast<char>(row[x * 4 + 1]),
                                  static_cast<char>(row[x * 4])};
                image.write(rgb, 3);
            }
        }
        checked(readback->UnlockRect(), "Minimap readback unlock");
        if (!image) throw std::runtime_error("Original minimap water image write failed");
        auto report_path = output;
        report_path.replace_extension(".json");
        std::ofstream report(report_path);
        report << std::setprecision(9)
               << "{\"schema\":\"civ5-minimap-native-dx9-" << (ice ? "ice" : "water")
               << "-v1\",\"bounds\":["
               << min_x << ',' << min_y << ',' << max_x << ',' << max_y
               << "],\"viewport\":[3,0,313,190],\"originalShaderBytecodeExecuted\":true,"
                  "\"completeSurface\":false,\"softwareVertexProcessing\":"
               << (software_vertex_processing ? "true" : "false");
        if (ice)
            report << ",\"iceBox\":[" << ice_box[0] << ',' << ice_box[1] << ','
                   << ice_box[2] << ',' << ice_box[3] << ']';
        if (probe)
            report << ",\"iceMeshVertexCount\":"
                   << probe->normalized_triangles.size()
                   << ",\"iceOffsetX\":" << ice_offset_x
                   << ",\"nativeIceGeometryCaptured\":false";
        report << ",\"d3d9ExDiagnosticApi\":"
#if defined(CIV5_MINIMAP_DX9EX_DIAGNOSTIC)
               << "true";
#else
               << "false";
#endif
        report << "}\n";
        if (!report) throw std::runtime_error("Original minimap water report write failed");
        std::cout << "Original DX9 minimap kind-" << (ice ? 9 : 7)
                  << " texture pass: 320x190, viewport 3,0,313,190\n";
        return 0;
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
