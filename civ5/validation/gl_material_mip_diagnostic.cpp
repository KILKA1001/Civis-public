// Diagnostic source-upload control for hidden D3D9 automatic mip levels.
#include "civ5/graphics/gl_texture.hpp"
#include "land_material_native.hpp"
#include "material_mip_native.hpp"
#include <iostream>
using namespace mip_validation;
using namespace civ5::graphics;
namespace fs = std::filesystem;
int main(int argc, char **argv) {
    try {
        require(argc == 5,
                "Usage: material_mip_diagnostic RGB565-base64 land-shaders fresh-output fixture-count");
        const fs::path out(argv[3]);
        require(!fs::exists(out), "Fresh mip diagnostic output");
        const unsigned count = std::stoul(argv[4]);
        require(count > 0 && count <= 32, "Bounded mip fixtures");
        auto base = read(argv[1]);
        require(base.size() == 8192, "Base64 RGB565 extent");
        land_material_validation::NativeLand n(argv[2]);
        NativeMipReader observer(n.device.value);
        auto context = std::make_shared<GlContext>();
        fs::create_directories(out);
        for (unsigned test = 0; test < count; ++test) {
            auto values = base;
            if (test)
                for (unsigned y = 0; y < 64; ++y)
                    for (unsigned x = 0; x < 64; ++x) {
                        const unsigned q =
                            test == 1   ? (x + y * 64)
                            : test == 2 ? ((x + y) % 2 ? 65535 : 0)
                                        : ((x * 1664525u + y * 1013904223u + test * 7777u) ^ (x * 1171u * y));
                        const std::uint16_t v = static_cast<std::uint16_t>(q);
                        std::memcpy(values.data() + (y * 64 + x) * 2, &v, 2);
                    }
            const auto folder = out / std::to_string(test);
            fs::create_directory(folder);
            write(folder / "base.bin", values);
            Com<IDirect3DTexture9> native;
            Com<IDirect3DSurface9> upload, surface;
            hr(n.device->CreateTexture(64, 64, 0, D3DUSAGE_RENDERTARGET | D3DUSAGE_AUTOGENMIPMAP,
                                       D3DFMT_R5G6B5, D3DPOOL_DEFAULT, &native.value, nullptr),
               "Diagnostic RGB565 automatic chain");
            hr(native->GetSurfaceLevel(0, &surface.value), "Diagnostic texture surface");
            hr(n.device->CreateOffscreenPlainSurface(64, 64, D3DFMT_R5G6B5, D3DPOOL_SYSTEMMEM, &upload.value,
                                                     nullptr),
               "Diagnostic base upload");
            D3DLOCKED_RECT lock{};
            hr(upload->LockRect(&lock, nullptr, 0), "Diagnostic upload lock");
            require(lock.Pitch >= 128, "Diagnostic source pitch");
            for (unsigned y = 0; y < 64; ++y)
                std::memcpy(static_cast<char *>(lock.pBits) + std::size_t(y) * lock.Pitch,
                            values.data() + y * 128, 128);
            hr(upload->UnlockRect(), "Diagnostic upload unlock");
            hr(n.device->UpdateSurface(upload.value, nullptr, surface.value, nullptr),
               "Diagnostic base transfer");
            native->GenerateMipSubLevels();
            auto gl = GlTexture::render_target(*context, 64, 64, 7, RenderTextureFormat::rgb565);
            context->api().TextureSubImage2D(gl.id(), 0, 0, 0, 64, 64, gl.plan().transfer_format,
                                             gl.plan().transfer_type, values.data());
            require(gl.read_mip_raw(0) == values, "Diagnostic GL base transport");
            gl.generate_mip_chain();
            for (unsigned mip = 0; mip < 7; ++mip) {
                write(folder / (std::to_string(mip) + "-native.f32"), observer.read(native.value, mip));
                write(folder / (std::to_string(mip) + "-gl.bin"), gl.read_mip_raw(mip));
            }
            std::cout << "PASS diagnostic automatic mip fixture " << test << '\n';
        }
    } catch (const std::exception &e) {
        std::cerr << "FAIL " << e.what() << '\n';
        return 1;
    }
}
