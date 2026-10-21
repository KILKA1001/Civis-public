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
    Com() = default;
    ~Com() { if (value) value->Release(); }
    Com(const Com &) = delete;
    Com &operator=(const Com &) = delete;
    T *operator->() const { return value; }
};
void checked(HRESULT code, const char *operation) {
    if (FAILED(code))
        throw std::runtime_error(std::string(operation) + ": HRESULT " +
                                 std::to_string(static_cast<std::uint32_t>(code)));
}
std::vector<std::uint8_t> read(const std::filesystem::path &path) {
    std::ifstream stream(path, std::ios::binary | std::ios::ate);
    if (!stream) throw std::runtime_error("Kind-8 probe input is missing");
    const auto size = stream.tellg();
    if (size < 8 || size > 65536)
        throw std::runtime_error("Kind-8 probe input length is invalid");
    std::vector<std::uint8_t> bytes(static_cast<std::size_t>(size));
    stream.seekg(0);
    stream.read(reinterpret_cast<char *>(bytes.data()), size);
    if (!stream) throw std::runtime_error("Kind-8 probe input is truncated");
    return bytes;
}
std::vector<std::uint8_t> background_ppm(const std::filesystem::path &path) {
    std::ifstream stream(path, std::ios::binary);
    std::string magic, extent, range;
    if (!stream || !std::getline(stream, magic) || !std::getline(stream, extent) ||
        !std::getline(stream, range) || magic != "P6" || extent != "320 190" || range != "255")
        throw std::runtime_error("Kind-8 background PPM header changed");
    std::vector<std::uint8_t> pixels(320 * 190 * 3);
    stream.read(reinterpret_cast<char *>(pixels.data()), pixels.size());
    char extra = 0;
    if (!stream || stream.get(extra))
        throw std::runtime_error("Kind-8 background PPM extent changed");
    return pixels;
}
template <class T> T at(const std::vector<std::uint8_t> &bytes, std::size_t offset) {
    if (offset + sizeof(T) > bytes.size()) throw std::runtime_error("Kind-8 probe field is truncated");
    T value{};
    std::memcpy(&value, bytes.data() + offset, sizeof(T));
    return value;
}
std::vector<DWORD> shader(const std::filesystem::path &path, DWORD version) {
    const auto bytes = read(path);
    if (bytes.size() % 4 || at<DWORD>(bytes, 0) != version)
        throw std::runtime_error("Original kind-8 shader version changed");
    std::vector<DWORD> words(bytes.size() / 4);
    std::memcpy(words.data(), bytes.data(), bytes.size());
    return words;
}
struct Window {
    HWND value = CreateWindowExW(0, L"STATIC", L"Civ5 kind-8 DX9 control", WS_POPUP,
                                 0, 0, 32, 32, nullptr, nullptr, nullptr, nullptr);
    Window() { if (!value) throw std::runtime_error("DX9 control window failed"); }
    ~Window() { DestroyWindow(value); }
};
} // namespace

int main(int argc, char **argv) {
    try {
        if (argc != 3 && argc != 4)
            throw std::invalid_argument("Usage: civ5_minimap_dx9_viewport <input-folder> <output.ppm> [background.ppm]");
        const std::filesystem::path input(argv[1]), output(argv[2]);
        const auto payload = read(input / "viewport.bin");
        if (std::memcmp(payload.data(), "MMV1", 4))
            throw std::runtime_error("Kind-8 vertex format changed");
        const std::array<float, 4> bounds{at<float>(payload, 4), at<float>(payload, 8),
                                          at<float>(payload, 12), at<float>(payload, 16)};
        const auto vertex_count = at<std::uint32_t>(payload, 20);
        const auto index_count = at<std::uint32_t>(payload, 24);
        if (!(std::isfinite(bounds[0]) && std::isfinite(bounds[1]) &&
              std::isfinite(bounds[2]) && std::isfinite(bounds[3]) &&
              bounds[2] > bounds[0] && bounds[3] > bounds[1]) ||
            vertex_count == 0 || vertex_count > 36 || vertex_count % 12 ||
            index_count != vertex_count * 4 ||
            payload.size() != 28 + std::size_t(vertex_count) * 40 + std::size_t(index_count) * 2)
            throw std::runtime_error("Kind-8 vertex/index extent changed");
        std::vector<std::uint16_t> indices(index_count);
        std::memcpy(indices.data(), payload.data() + 28 + std::size_t(vertex_count) * 40,
                    index_count * sizeof(indices[0]));
        for (std::size_t n = 0; n < index_count; ++n)
            if (indices[n] >= vertex_count) throw std::runtime_error("Kind-8 index is out of range");

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
        auto result = api->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, window.value,
                                        D3DCREATE_HARDWARE_VERTEXPROCESSING, &pp, &device.value);
        if (FAILED(result)) {
            software_vertex_processing = true;
            result = api->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, window.value,
                                       D3DCREATE_SOFTWARE_VERTEXPROCESSING, &pp, &device.value);
        }
        checked(result, "Original DX9 HAL device");
        Com<IDirect3DSurface9> target, readback;
        checked(device->CreateRenderTarget(320, 190, D3DFMT_A8R8G8B8, D3DMULTISAMPLE_NONE,
                                           0, FALSE, &target.value, nullptr), "Kind-8 target");
        checked(device->CreateOffscreenPlainSurface(320, 190, D3DFMT_A8R8G8B8,
                                                    D3DPOOL_SYSTEMMEM, &readback.value, nullptr),
                "Kind-8 readback");
        checked(device->SetRenderTarget(0, target.value), "Kind-8 target binding");
        checked(device->SetDepthStencilSurface(nullptr), "Kind-8 depth surface disable");
        checked(device->SetRenderState(D3DRS_ZENABLE, FALSE), "Kind-8 depth disable");
        checked(device->SetRenderState(D3DRS_ZWRITEENABLE, FALSE), "Kind-8 depth writes disable");
        checked(device->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE), "Kind-8 culling disable");
        checked(device->SetRenderState(D3DRS_SRGBWRITEENABLE, FALSE), "Kind-8 sRGB write disable");
        checked(device->SetRenderState(D3DRS_COLORWRITEENABLE, 15), "Kind-8 color write mask");
        checked(device->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE), "Kind-8 blending enable");
        checked(device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA), "Kind-8 source blend");
        checked(device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA),
                "Kind-8 destination blend");
        checked(device->SetRenderState(D3DRS_BLENDOP, D3DBLENDOP_ADD), "Kind-8 blend operation");
        checked(device->SetRenderState(D3DRS_SEPARATEALPHABLENDENABLE, TRUE),
                "Kind-8 separate alpha blend");
        checked(device->SetRenderState(D3DRS_SRCBLENDALPHA, D3DBLEND_ONE),
                "Kind-8 source alpha blend");
        checked(device->SetRenderState(D3DRS_DESTBLENDALPHA, D3DBLEND_ONE),
                "Kind-8 destination alpha blend");
        checked(device->SetRenderState(D3DRS_BLENDOPALPHA, D3DBLENDOP_ADD),
                "Kind-8 alpha blend operation");
        if (argc == 4) {
            const auto background = background_ppm(argv[3]);
            Com<IDirect3DSurface9> source;
            checked(device->CreateOffscreenPlainSurface(320, 190, D3DFMT_A8R8G8B8,
                                                        D3DPOOL_SYSTEMMEM, &source.value, nullptr),
                    "Kind-8 background upload surface");
            D3DLOCKED_RECT fill{};
            checked(source->LockRect(&fill, nullptr, 0), "Kind-8 background upload lock");
            for (int y = 0; y < 190; ++y) {
                auto *row = static_cast<std::uint8_t *>(fill.pBits) + y * fill.Pitch;
                for (int x = 0; x < 320; ++x) {
                    const auto src = (y * 320 + x) * 3;
                    row[x * 4] = background[src + 2];
                    row[x * 4 + 1] = background[src + 1];
                    row[x * 4 + 2] = background[src];
                    row[x * 4 + 3] = 255;
                }
            }
            checked(source->UnlockRect(), "Kind-8 background upload unlock");
            checked(device->UpdateSurface(source.value, nullptr, target.value, nullptr),
                    "Kind-8 background GPU upload");
        } else {
            checked(device->Clear(0, nullptr, D3DCLEAR_TARGET, 0xFF203040u, 1.0f, 0),
                    "Kind-8 fixed background clear");
        }
        const D3DVIEWPORT9 viewport{3, 0, 313, 190, 0.0f, 1.0f};
        checked(device->SetViewport(&viewport), "Kind-8 active minimap viewport");

        const auto vs = shader(input / "kind8-vertex.bin", 0xFFFE0300u);
        const auto ps = shader(input / "kind8-pixel.bin", 0xFFFF0300u);
        Com<IDirect3DVertexShader9> vertex_shader;
        Com<IDirect3DPixelShader9> pixel_shader;
        checked(device->CreateVertexShader(vs.data(), &vertex_shader.value), "Original kind-8 VS");
        checked(device->CreatePixelShader(ps.data(), &pixel_shader.value), "Original kind-8 PS");
        const D3DVERTEXELEMENT9 elements[] = {
            {0, 0, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
            {0, 24, D3DDECLTYPE_D3DCOLOR, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_COLOR, 0},
            D3DDECL_END()};
        Com<IDirect3DVertexDeclaration9> declaration;
        checked(device->CreateVertexDeclaration(elements, &declaration.value),
                "Original kind-8 vertex declaration");
        checked(device->SetVertexDeclaration(declaration.value), "Kind-8 declaration binding");
        checked(device->SetVertexShader(vertex_shader.value), "Kind-8 VS binding");
        checked(device->SetPixelShader(pixel_shader.value), "Kind-8 PS binding");
        const float sx = 2.0f / (bounds[2] - bounds[0]);
        const float sy = 2.0f / (bounds[3] - bounds[1]);
        const float constants[2][4]{{(bounds[0] + bounds[2]) * 0.5f * sx,
                                     (bounds[1] + bounds[3]) * 0.5f * sy, 0, 0},
                                    {sx, sy, 0, 0}};
        checked(device->SetVertexShaderConstantF(3, constants[0], 2),
                "Original kind-8 projection constants");
        checked(device->BeginScene(), "Begin kind-8 pass");
        checked(device->DrawIndexedPrimitiveUP(D3DPT_TRIANGLELIST, 0, vertex_count,
                                               index_count / 3, indices.data(), D3DFMT_INDEX16,
                                               payload.data() + 28, 40),
                "Original kind-8 indexed draw");
        checked(device->EndScene(), "End kind-8 pass");
        checked(device->GetRenderTargetData(target.value, readback.value), "Kind-8 GPU readback");
        D3DLOCKED_RECT lock{};
        checked(readback->LockRect(&lock, nullptr, D3DLOCK_READONLY), "Kind-8 readback lock");
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
        checked(readback->UnlockRect(), "Kind-8 readback unlock");
        if (!image) throw std::runtime_error("Kind-8 GPU image write failed");
        auto report_path = output;
        report_path.replace_extension(".json");
        std::ofstream report(report_path);
        report << std::setprecision(9)
               << "{\"schema\":\"civ5-minimap-kind8-native-dx9-gpu-v1\",\"bounds\":["
               << bounds[0] << ',' << bounds[1] << ',' << bounds[2] << ',' << bounds[3]
               << "],\"viewport\":[3,0,313,190],\"vertexCount\":" << vertex_count
               << ",\"indexCount\":" << index_count
               << ",\"backgroundPpm\":" << (argc == 4 ? "true" : "false")
               << ",\"originalShaderBytecodeExecuted\":true,\"originalGameProcessAccess\":false,"
               << "\"softwareVertexProcessing\":"
               << (software_vertex_processing ? "true" : "false") << "}\n";
        if (!report) throw std::runtime_error("Kind-8 GPU report write failed");
        std::cout << "Original DX9 kind-8 shader: " << vertex_count << " vertices, "
                  << index_count / 3 << " triangles\n";
        return 0;
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
