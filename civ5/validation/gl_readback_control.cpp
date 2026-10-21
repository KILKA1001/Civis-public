// Reproducible API witnesses. Unexpected DSA errors are recorded, not hidden.
#include "civ5/graphics/gl_texture.hpp"
#include <algorithm>
#include <iostream>
#include <stdexcept>

using namespace civ5::assets;
using namespace civ5::graphics;
namespace fs = std::filesystem;
void require(bool ok, const char *reason) {
    if (!ok)
        throw std::runtime_error(reason);
}
DdsData fixture(bool cube) {
    Bytes data(128);
    auto put = [&](unsigned at, unsigned value) {
        for (unsigned i = 0; i < 4; ++i)
            data.at(at + i) = static_cast<std::uint8_t>(value >> (8 * i));
    };
    put(0, 0x20534444);
    put(4, 124);
    put(8, 0x20000);
    put(12, cube ? 4 : 3);
    put(16, cube ? 4 : 5);
    put(28, 1);
    put(76, 32);
    put(80, 4);
    put(84, 0x31545844);
    put(112, cube ? 0xfe00 : 0);
    for (unsigned i = 0; i < (cube ? 48u : 16u); ++i)
        data.push_back(static_cast<std::uint8_t>(i * 3));
    return DdsData::parse(std::move(data));
}
int main(int argc, char **argv) {
    try {
        require(argc == 2, "Usage: civ5_gl_readback_control fresh-output.tsv");
        const fs::path output(argv[1]);
        require(!fs::exists(output), "Refusing output overwrite");
        GlContext context;
        const auto &gl = context.api();
        std::ofstream out(output);
        require(bool(out), "Output open failed");
        std::cout << context.description() << std::endl;
        out << "cube\twidth\theight\tqueryTargetError\tdsaSubImageError\tdsaWholeImageError\tcoreFaceRawExact"
               "\n";
        for (bool cube : {false, true}) {
            auto data = fixture(cube);
            GlTexture texture(context, data);
            Bytes expected;
            for (unsigned face = 0; face < data.face_count(); ++face) {
                auto payload = data.payload(face, 0);
                expected.insert(expected.end(), payload.begin(), payload.end());
            }
            GLint target = 0;
            gl.GetTextureParameteriv(texture.id(), GL_TEXTURE_TARGET, &target);
            const auto target_error = gl.GetError();
            if (target_error == 0)
                require(target == (cube ? GL_TEXTURE_CUBE_MAP : GL_TEXTURE_2D),
                        "Successful DSA query gave wrong target");
            auto first = data.payload(0, 0);
            Bytes sub(first.size(), 0xcd);
            gl.GetCompressedTextureSubImage(texture.id(), 0, 0, 0, 0, data.width(), data.height(), 1,
                                            static_cast<GLsizei>(sub.size()), sub.data());
            const auto sub_error = gl.GetError();
            if (sub_error == 0)
                require(std::equal(sub.begin(), sub.end(), first.begin()),
                        "Successful DSA subimage changed bytes");
            Bytes whole(expected.size(), 0xcd);
            gl.GetCompressedTextureImage(texture.id(), 0, static_cast<GLsizei>(whole.size()), whole.data());
            const auto whole_error = gl.GetError();
            if (whole_error == 0)
                require(whole == expected, "Successful DSA whole image changed bytes");
            const auto native = texture.read_mip_raw(0);
            require(native == expected, "Core full-face readback changed bytes");
            out << cube << '\t' << data.width() << '\t' << data.height() << '\t' << target_error << '\t'
                << sub_error << '\t' << whole_error << "\t1\n";
            std::cout << "API CONTROL cube=" << cube << " targetError=" << target_error
                      << " subError=" << sub_error << " wholeError=" << whole_error << " coreFaceRawExact=1"
                      << std::endl;
        }
        out.close();
        require(bool(out), "Output close failed");
        return 0;
    } catch (const std::exception &e) {
        std::cerr << "FAIL " << e.what() << '\n';
        return 1;
    }
}
