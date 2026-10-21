#include "civ5/assets/dds.hpp"
#include <array>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <d3d9.h>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
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
    HWND value = CreateWindowExW(0, L"STATIC", L"Civ5 minimap DX9 surface control",
                                 WS_POPUP, 0, 0, 32, 32, nullptr, nullptr, nullptr, nullptr);
    Window() { if (!value) throw std::runtime_error("DX9 minimap window creation failed"); }
    ~Window() { DestroyWindow(value); }
};
struct Point { float x, y; };
struct SpriteVertex {
    float x, y;
    std::int16_t u, v;
    std::uint32_t color;
};
static_assert(sizeof(SpriteVertex) == 16);
struct MountainMarkerList {
    float radius = 0;
    std::vector<Point> first_segment;
};
MountainMarkerList mountain_markers(const std::filesystem::path &path) {
    const auto bytes = read(path);
    if (bytes.size() < 16 || std::memcmp(bytes.data(), "MMM1", 4))
        throw std::runtime_error("Captured mountain marker stream changed");
    MountainMarkerList result;
    std::uint32_t first = 0, second = 0;
    std::memcpy(&result.radius, bytes.data() + 4, 4);
    std::memcpy(&first, bytes.data() + 8, 4);
    std::memcpy(&second, bytes.data() + 12, 4);
    if (!(std::isfinite(result.radius) && result.radius > 0) ||
        first == 0 || first > 4096 || second != 0 ||
        bytes.size() != 16 + std::size_t(first + second) * 16)
        throw std::runtime_error("Captured first-segment mountain extent changed");
    result.first_segment.reserve(first);
    for (std::size_t index = 0; index < first; ++index) {
        Point center{};
        std::memcpy(&center, bytes.data() + 16 + index * 16 + 8, sizeof(center));
        if (!std::isfinite(center.x) || !std::isfinite(center.y))
            throw std::runtime_error("Captured mountain position is invalid");
        result.first_segment.push_back(center);
    }
    return result;
}
void upload_ppm_background(IDirect3DDevice9 *device, IDirect3DSurface9 *target,
                           const std::filesystem::path &path) {
    const auto bytes = read(path);
    constexpr char header[] = "P6\n320 190\n255\n";
    constexpr std::size_t prefix = sizeof(header) - 1;
    if (bytes.size() != prefix + 320u * 190u * 3u ||
        std::memcmp(bytes.data(), header, prefix))
        throw std::runtime_error("Mountain background PPM changed");
    Com<IDirect3DSurface9> source;
    checked(device->CreateOffscreenPlainSurface(320, 190, D3DFMT_A8R8G8B8,
                                                D3DPOOL_SYSTEMMEM, &source.value, nullptr),
            "Mountain background upload surface");
    D3DLOCKED_RECT lock{};
    checked(source->LockRect(&lock, nullptr, 0), "Mountain background upload lock");
    for (std::size_t y = 0; y < 190; ++y) {
        auto *row = static_cast<std::uint8_t *>(lock.pBits) + y * lock.Pitch;
        for (std::size_t x = 0; x < 320; ++x) {
            const auto at = prefix + (y * 320 + x) * 3;
            row[x * 4] = bytes[at + 2];
            row[x * 4 + 1] = bytes[at + 1];
            row[x * 4 + 2] = bytes[at];
            row[x * 4 + 3] = 255;
        }
    }
    checked(source->UnlockRect(), "Mountain background upload unlock");
    checked(device->UpdateSurface(source.value, nullptr, target, nullptr),
            "Mountain background GPU upload");
}
std::array<std::vector<Point>, 3> triangles(const std::filesystem::path &path) {
    const auto bytes = read(path);
    if (bytes.size() < 4 || std::memcmp(bytes.data(), "MMS1", 4))
        throw std::runtime_error("Selected-save minimap triangle stream changed");
    std::array<std::vector<Point>, 3> layers;
    std::size_t cursor = 4;
    for (auto &layer : layers) {
        if (cursor + 4 > bytes.size()) throw std::runtime_error("Minimap triangle count is truncated");
        std::uint32_t count = 0;
        std::memcpy(&count, bytes.data() + cursor, 4);
        cursor += 4;
        if (!count || count > 100000 || cursor + std::size_t(count) * 3 * sizeof(Point) > bytes.size())
            throw std::runtime_error("Minimap triangle extent is invalid");
        layer.resize(std::size_t(count) * 3);
        std::memcpy(layer.data(), bytes.data() + cursor, layer.size() * sizeof(Point));
        cursor += layer.size() * sizeof(Point);
    }
    if (cursor != bytes.size()) throw std::runtime_error("Minimap triangle stream has trailing bytes");
    return layers;
}
std::vector<Point> river_triangles(const std::filesystem::path &path) {
    const auto bytes = read(path);
    if (bytes.size() < 8 || std::memcmp(bytes.data(), "MMR1", 4))
        throw std::runtime_error("Captured minimap river triangle stream changed");
    std::uint32_t count = 0;
    std::memcpy(&count, bytes.data() + 4, 4);
    if (!count || count > 100000 || bytes.size() != 8 + std::size_t(count) * 3 * sizeof(Point))
        throw std::runtime_error("Captured minimap river triangle extent is invalid");
    std::vector<Point> points(std::size_t(count) * 3);
    std::memcpy(points.data(), bytes.data() + 8, points.size() * sizeof(Point));
    return points;
}
void bind_texture(IDirect3DDevice9 *device, const std::filesystem::path &path,
                  DWORD slot, Com<IDirect3DTexture9> &texture,
                  std::uint32_t expected_size = 256, std::uint32_t expected_mips = 9,
                  D3DTEXTUREADDRESS address = D3DTADDRESS_WRAP) {
    auto dds = civ5::assets::DdsData::parse(read(path));
    if (dds.width() != expected_size || dds.height() != expected_size ||
        dds.mip_count() != expected_mips ||
        dds.bits_per_pixel() != 32 || dds.face_count() != 1)
        throw std::runtime_error("Retail minimap DDS layout changed");
    checked(device->CreateTexture(expected_size, expected_size, expected_mips,
                                  0, D3DFMT_A8R8G8B8,
                                  D3DPOOL_MANAGED, &texture.value, nullptr),
            "Original minimap mip texture");
    for (const auto &level : dds.levels()) {
        D3DLOCKED_RECT lock{};
        checked(texture->LockRect(level.mip, &lock, nullptr, 0), "Minimap mip lock");
        const auto bytes = dds.payload(0, level.mip);
        if (lock.Pitch < static_cast<INT>(level.row_pitch))
            throw std::runtime_error("Minimap mip row pitch is too small");
        for (std::size_t y = 0; y < level.height; ++y)
            std::memcpy(static_cast<std::uint8_t *>(lock.pBits) + y * lock.Pitch,
                        bytes.data() + y * level.row_pitch,
                        static_cast<std::size_t>(level.row_pitch));
        checked(texture->UnlockRect(level.mip), "Minimap mip unlock");
    }
    checked(device->SetTexture(slot, texture.value), "Original minimap texture slot");
    for (const auto state : {D3DSAMP_ADDRESSU, D3DSAMP_ADDRESSV, D3DSAMP_ADDRESSW})
        checked(device->SetSamplerState(slot, state, address), "Native sampler address");
    for (const auto state : {D3DSAMP_MINFILTER, D3DSAMP_MAGFILTER, D3DSAMP_MIPFILTER})
        checked(device->SetSamplerState(slot, state, D3DTEXF_LINEAR), "Native sampler filtering");
    checked(device->SetSamplerState(slot, D3DSAMP_MAXANISOTROPY, 1), "Native sampler anisotropy");
    checked(device->SetSamplerState(slot, D3DSAMP_SRGBTEXTURE, TRUE), "Native sampler sRGB decode");
}
} // namespace

int main(int argc, char **argv) {
    try {
        if (argc != 7 && argc != 8 && argc != 10 && argc != 11 && argc != 13 &&
            argc != 14 && argc != 15)
            throw std::invalid_argument("Usage: civ5_minimap_dx9_surface input output.ppm minX minY maxX maxY [layers 0..3 | --mountain centerX centerY radius | --city-marker centerX centerY radius ARGB fadeMin fadeMax | --city-marker-on base.ppm centerX centerY radius ARGB fadeMin fadeMax | --mountain-list/--mountain-list-clamp base.ppm markers.bin | --river/--river-no-fog river.bin secondMinX secondMinY secondMaxX secondMaxY]");
        const std::filesystem::path input(argv[1]), output(argv[2]);
        const float min_x = std::stof(argv[3]), min_y = std::stof(argv[4]);
        const float max_x = std::stof(argv[5]), max_y = std::stof(argv[6]);
        if (!(max_x > min_x && max_y > min_y))
            throw std::invalid_argument("Invalid minimap world bounds");
        const bool river_mode = argc == 13;
        const bool mountain_single = argc == 11 && std::string(argv[7]) == "--mountain";
        const bool city_marker_on = argc == 15 && std::string(argv[7]) == "--city-marker-on";
        const bool city_marker = (argc == 14 && std::string(argv[7]) == "--city-marker") ||
                                 city_marker_on;
        const int city_arg_shift = city_marker_on ? 1 : 0;
        const bool mountain_list_clamp = argc == 10 &&
            std::string(argv[7]) == "--mountain-list-clamp";
        const bool mountain_list_mode = argc == 10 &&
            (std::string(argv[7]) == "--mountain-list" || mountain_list_clamp);
        const bool mountain_mode = mountain_single || mountain_list_mode;
        const bool sprite_mode = mountain_mode || city_marker;
        const bool river_no_fog = river_mode && std::string(argv[7]) == "--river-no-fog";
        if (river_mode && !river_no_fog && std::string(argv[7]) != "--river")
            throw std::invalid_argument("Unknown minimap river draw mode");
        if ((argc == 10 || argc == 11) && !mountain_mode)
            throw std::invalid_argument("Unknown minimap mountain draw mode");
        if (argc == 14 && !city_marker)
            throw std::invalid_argument("Unknown minimap kind-5 draw mode");
        if (argc == 15 && !city_marker_on)
            throw std::invalid_argument("Unknown minimap kind-5 background mode");
        const int surface_layers = argc == 8 ? std::stoi(argv[7]) : sprite_mode ? 0 : 3;
        if (surface_layers < 0 || surface_layers > 3)
            throw std::invalid_argument("Invalid minimap surface-layer count");
        const auto mountain_list = mountain_list_mode ? mountain_markers(argv[9])
                                                      : MountainMarkerList{};
        const float mountain_x = mountain_single || city_marker ?
            std::stof(argv[8 + city_arg_shift]) : 0.0f;
        const float mountain_y = mountain_single || city_marker ?
            std::stof(argv[9 + city_arg_shift]) : 0.0f;
        const float mountain_radius = mountain_list_mode ? mountain_list.radius :
                                      mountain_single || city_marker ?
                                      std::stof(argv[10 + city_arg_shift]) : 0.0f;
        const auto color_word = city_marker ?
            std::stoull(argv[11 + city_arg_shift], nullptr, 0) : 0xFFFFFFFFull;
        if (color_word > 0xFFFFFFFFull)
            throw std::invalid_argument("Invalid kind-5 vertex color");
        const auto marker_color = static_cast<std::uint32_t>(color_word);
        const float city_fade_min = city_marker ?
            std::stof(argv[12 + city_arg_shift]) : 0.0f;
        const float city_fade_max = city_marker ?
            std::stof(argv[13 + city_arg_shift]) : 1.0f;
        if ((mountain_single || city_marker) &&
            !(std::isfinite(mountain_x) && std::isfinite(mountain_y) &&
              std::isfinite(mountain_radius) && mountain_radius > 0.0f))
            throw std::invalid_argument("Invalid minimap marker center or radius");
        if (city_marker && !(std::isfinite(city_fade_min) &&
                             std::isfinite(city_fade_max) &&
                             city_fade_max > city_fade_min))
            throw std::invalid_argument("Invalid kind-5 fade constants");
        const auto mesh = triangles(input / "triangles.bin");
        const auto river_mesh = river_mode ? river_triangles(input / argv[8])
                                           : std::vector<Point>{};
        const std::array<float, 4> second_bounds = river_mode
            ? std::array<float, 4>{std::stof(argv[9]), std::stof(argv[10]),
                                   std::stof(argv[11]), std::stof(argv[12])}
            : std::array<float, 4>{};
        if (river_mode && !(second_bounds[2] > second_bounds[0] &&
                            second_bounds[3] > second_bounds[1]))
            throw std::invalid_argument("Invalid second minimap river bounds");

        Window window;
        Com<IDirect3D9> api;
        api.value = Direct3DCreate9(D3D_SDK_VERSION);
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
        auto device_result = api->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, window.value,
                                                D3DCREATE_HARDWARE_VERTEXPROCESSING,
                                                &pp, &device.value);
        if (FAILED(device_result)) {
            software_vertex_processing = true;
            device_result = api->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, window.value,
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
        if (mountain_list_mode || city_marker_on)
            upload_ppm_background(device.value, target.value, argv[8]);

        constexpr int kinds[7]{7, 4, 1, 0, 3, 6, 5};
        Com<IDirect3DVertexShader9> vertices[7];
        Com<IDirect3DPixelShader9> pixels[7];
        for (int i = 0; i < (city_marker ? 7 : mountain_mode ? 6 : river_mode ? 5 : 4); ++i) {
            const auto prefix = "kind" + std::to_string(kinds[i]);
            const auto vs = shader(input / (prefix + "-vertex.bin"), 0xFFFE0300u);
            const auto ps = shader(input / (prefix + "-pixel.bin"), 0xFFFF0300u);
            checked(device->CreateVertexShader(vs.data(), &vertices[i].value), "Original minimap VS");
            checked(device->CreatePixelShader(ps.data(), &pixels[i].value), "Original minimap PS");
        }
        const D3DVERTEXELEMENT9 water_elements[] = {
            {0, 0, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0},
            D3DDECL_END()};
        const D3DVERTEXELEMENT9 surface_elements[] = {
            {0, 0, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
            D3DDECL_END()};
        const D3DVERTEXELEMENT9 sprite_elements[] = {
            {0, 0, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
            {0, 8, D3DDECLTYPE_SHORT2N, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0},
            {0, 12, D3DDECLTYPE_D3DCOLOR, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_COLOR, 0},
            D3DDECL_END()};
        Com<IDirect3DVertexDeclaration9> water_declaration, surface_declaration,
            sprite_declaration;
        checked(device->CreateVertexDeclaration(water_elements, &water_declaration.value),
                "Original kind-7 vertex input declaration");
        checked(device->CreateVertexDeclaration(surface_elements, &surface_declaration.value),
                "Original minimap surface vertex input declaration");
        if (sprite_mode)
            checked(device->CreateVertexDeclaration(sprite_elements, &sprite_declaration.value),
                    "Original minimap sprite input declaration");
        const auto set_bounds = [&](float x0, float y0, float x1, float y1) {
            const float sx = 2.0f / (x1 - x0), sy = 2.0f / (y1 - y0);
            const float constants[4][4] = {
                {(x0 + x1) * 0.5f * sx, (y0 + y1) * 0.5f * sy, 0, 0},
                {sx, sy, 0, 0}, {x0, y0, 0, 0}, {x1, y1, 0, 0}};
            checked(device->SetVertexShaderConstantF(3, constants[0], 4),
                    "Original minimap visible-bounds constants");
        };
        set_bounds(min_x, min_y, max_x, max_y);
        const float water_constant[4]{0.002000000095f, 100.0f, 0, 0};
        checked(device->SetPixelShaderConstantF(3, water_constant, 1),
                "Original minimap water tiling constants");
        const float land_constant[4]{0.800000012f, 1.0f, 0, 0};
        checked(device->SetPixelShaderConstantF(2, land_constant, 1),
                "Original minimap land tiling constants");
        const float fog_alpha[4]{1.0f, 0, 0, 0};
        checked(device->SetPixelShaderConstantF(6, fog_alpha, 1),
                "Original minimap fog alpha");
        if (city_marker) {
            const float fade[4]{city_fade_min, city_fade_max, 0, 0};
            checked(device->SetPixelShaderConstantF(1, fade, 1),
                    "Original kind-5 fade constants");
        }
        Com<IDirect3DTexture9> textures[5];
        constexpr DWORD slots[4]{2, 5, 1, 3};
        const char *names[4]{"minimapwater.dds", "minimapshallowwater.dds",
                             "minimapgrass.dds", "minimapfog.dds"};
        for (int i = 0; i < 4; ++i)
            bind_texture(device.value, input / names[i], slots[i], textures[i]);
        if (mountain_mode)
            bind_texture(device.value, input / "minimapmountain.dds", 7, textures[4], 128, 8,
                         mountain_list_clamp ? D3DTADDRESS_CLAMP : D3DTADDRESS_WRAP);
        const float quad[4][2]{{0, 0}, {1, 0}, {0, 1}, {1, 1}};
        checked(device->BeginScene(), "Begin original minimap surface passes");
        checked(device->SetVertexDeclaration(water_declaration.value), "Minimap water declaration");
        checked(device->SetVertexShader(vertices[0].value), "Original minimap water VS");
        checked(device->SetPixelShader(pixels[0].value), "Original minimap water PS");
        if (!mountain_list_mode && !city_marker_on)
            checked(device->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, quad, sizeof(quad[0])),
                    "Draw original minimap water pass");
        if (sprite_mode) {
            checked(device->SetVertexDeclaration(sprite_declaration.value),
                    "Original minimap sprite declaration");
            checked(device->SetVertexShader(vertices[city_marker ? 6 : 5].value),
                    "Original minimap sprite VS");
            checked(device->SetPixelShader(pixels[city_marker ? 6 : 5].value),
                    "Original minimap sprite PS");
            checked(device->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE),
                    "Kind-6 sprite blend enable");
            checked(device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA),
                    "Kind-6 sprite RGB source blend");
            checked(device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA),
                    "Kind-6 sprite RGB destination blend");
            checked(device->SetRenderState(D3DRS_BLENDOP, D3DBLENDOP_ADD),
                    "Kind-6 sprite RGB blend operation");
            checked(device->SetRenderState(D3DRS_SEPARATEALPHABLENDENABLE, TRUE),
                    "Kind-6 sprite separate alpha blend");
            checked(device->SetRenderState(D3DRS_SRCBLENDALPHA, D3DBLEND_ONE),
                    "Kind-6 sprite alpha source blend");
            checked(device->SetRenderState(D3DRS_DESTBLENDALPHA, D3DBLEND_ONE),
                    "Kind-6 sprite alpha destination blend");
            checked(device->SetRenderState(D3DRS_BLENDOPALPHA, D3DBLENDOP_ADD),
                    "Kind-6 sprite alpha blend operation");
            const auto draw_sprite = [&](float x, float y) {
                const float r = mountain_radius;
                constexpr std::int16_t uv_max = 0x7FFF;
                const SpriteVertex sprite[6]{
                    {x - r, y + r, 0, 0, marker_color},
                    {x + r, y + r, uv_max, 0, marker_color},
                    {x + r, y - r, uv_max, uv_max, marker_color},
                    {x - r, y + r, 0, 0, marker_color},
                    {x + r, y - r, uv_max, uv_max, marker_color},
                    {x - r, y - r, 0, uv_max, marker_color}};
                checked(device->DrawPrimitiveUP(D3DPT_TRIANGLELIST, 2,
                                                sprite, sizeof(SpriteVertex)),
                        "Draw original minimap marker sprite");
            };
            if (mountain_list_mode)
                for (const auto &center : mountain_list.first_segment)
                    draw_sprite(center.x, center.y);
            else
                draw_sprite(mountain_x, mountain_y);
        }
        checked(device->SetVertexDeclaration(surface_declaration.value),
                "Minimap surface declaration");
        checked(device->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE), "Surface blend enable");
        checked(device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA), "Surface source alpha");
        checked(device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA),
                "Surface destination alpha");
        checked(device->SetRenderState(D3DRS_BLENDOP, D3DBLENDOP_ADD), "Surface blend operation");
        for (int i = 0; i < surface_layers; ++i) {
            if (i == 2 && river_mode) {
                checked(device->SetVertexShader(vertices[4].value), "Original kind-3 river VS");
                checked(device->SetPixelShader(pixels[4].value), "Original kind-3 river PS");
                checked(device->DrawPrimitiveUP(D3DPT_TRIANGLELIST,
                                                static_cast<UINT>(river_mesh.size() / 3),
                                                river_mesh.data(), sizeof(Point)),
                        "Draw original minimap river pass, first segment");
                set_bounds(second_bounds[0], second_bounds[1],
                           second_bounds[2], second_bounds[3]);
                checked(device->DrawPrimitiveUP(D3DPT_TRIANGLELIST,
                                                static_cast<UINT>(river_mesh.size() / 3),
                                                river_mesh.data(), sizeof(Point)),
                        "Draw original minimap river pass, second segment");
                set_bounds(min_x, min_y, max_x, max_y);
                if (river_no_fog) break;
            }
            if (i == 2)
                checked(device->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE),
                        "Fog blend disable");
            checked(device->SetVertexShader(vertices[i + 1].value), "Original minimap surface VS");
            checked(device->SetPixelShader(pixels[i + 1].value), "Original minimap surface PS");
            checked(device->DrawPrimitiveUP(D3DPT_TRIANGLELIST,
                                             static_cast<UINT>(mesh[i].size() / 3),
                                             mesh[i].data(), sizeof(Point)),
                    "Draw original minimap surface pass");
        }
        checked(device->EndScene(), "End original minimap surface passes");
        checked(device->GetRenderTargetData(target.value, readback.value),
                "Original minimap surface GPU readback");
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
        if (!image) throw std::runtime_error("Original minimap surface image write failed");
        auto report_path = output;
        report_path.replace_extension(".json");
        std::ofstream report(report_path);
        report << std::setprecision(9)
               << "{\"schema\":\"civ5-minimap-native-dx9-surface-v1\",\"bounds\":["
               << min_x << ',' << min_y << ',' << max_x << ',' << max_y
               << "],\"viewport\":[3,0,313,190],\"originalShaderBytecodeExecuted\":true,"
                  "\"layers\":[\""
               << (mountain_list_mode || city_marker_on ? "captured-base" : "water") << '"'
               << (mountain_mode ? ",\"mountain\"" : city_marker ? ",\"city-marker\"" : "")
               << (surface_layers >= 1 ? ",\"coast\"" : "")
               << (surface_layers >= 2 ? ",\"land\"" : "")
               << (river_mode ? ",\"river\"" : "")
               << (surface_layers >= 3 && !river_no_fog ? ",\"fog\"" : "")
               << "],\"surfaceLayers\":" << surface_layers << ','
               << "\"riverKind3\":" << (river_mode ? "true" : "false") << ','
               << "\"mountainKind6\":" << (mountain_mode ? "true" : "false") << ','
               << "\"cityKind5\":" << (city_marker ? "true" : "false") << ','
               << "\"mountainListCount\":"
               << (mountain_list_mode ? mountain_list.first_segment.size() : 0) << ','
               << "\"mountainSamplerAddress\":\""
               << (mountain_list_clamp ? "clamp" : "wrap") << "\","
               << "\"backgroundFromPpm\":"
               << (mountain_list_mode || city_marker_on ? "true" : "false") << ','
               << "\"mountainCenter\":[" << mountain_x << ',' << mountain_y << "],"
               << "\"mountainRadius\":" << mountain_radius << ','
               << "\"markerColorArgb\":" << marker_color << ','
               << "\"cityFade\":[" << city_fade_min << ',' << city_fade_max << "],"
               << "\"riverTriangles\":" << river_mesh.size() / 3 << ','
               << "\"riverSegments\":" << (river_mode ? 2 : 0) << ','
               << "\"softwareVertexProcessing\":"
               << (software_vertex_processing ? "true" : "false")
               << ",\"completeSurface\":false}\n";
        if (!report) throw std::runtime_error("Original minimap surface report write failed");
        std::cout << "Original DX9 minimap "
                  << (mountain_list_mode || city_marker_on ? "captured base" : "water")
                  << " plus " << surface_layers
                  << " surface layers: 320x190, viewport 3,0,313,190\n";
        return 0;
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
