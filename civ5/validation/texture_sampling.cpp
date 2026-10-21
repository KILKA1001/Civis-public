// Own D3D9 HAL/HLSL versus C++ OpenGL/GLSL point samples. No game process/shader.
#include "texture_sampling_native.hpp"
using namespace civ5::assets;
using namespace civ5::graphics;
using namespace texture_sampling_validation;
namespace fs = std::filesystem;
namespace {
void put(Bytes &b, std::size_t offset, std::uint32_t value) {
    for (unsigned i = 0; i < 4; ++i)
        b.at(offset + i) = static_cast<std::uint8_t>(value >> (8 * i));
}
void append(Bytes &b, std::uint32_t value, unsigned count) {
    for (unsigned i = 0; i < count; ++i)
        b.push_back(static_cast<std::uint8_t>(value >> (8 * i)));
}
struct Case {
    unsigned format;
    bool cube;
    DdsData data;
};
Case make_case(unsigned format, bool cube) {
    const bool compressed = format > 0xffffff;
    unsigned bpp = 0, pf = 4, fourcc = format;
    std::array<unsigned, 4> masks{};
    switch (format) {
    case 21:
    case 22:
        bpp = 32;
        pf = 0x40;
        fourcc = 0;
        masks = {0xff0000, 0xff00, 0xff, format == 21 ? 0xff000000u : 0u};
        break;
    case 28:
        bpp = 8;
        pf = 2;
        fourcc = 0;
        masks = {0, 0, 0, 255};
        break;
    case 50:
        bpp = 8;
        pf = 0x20000;
        fourcc = 0;
        masks = {255, 0, 0, 0};
        break;
    case 81:
        bpp = 16;
        pf = 0x20000;
        fourcc = 0;
        masks = {65535, 0, 0, 0};
        break;
    case 36:
    case 110:
    case 113:
    case 115:
        bpp = 64;
        break;
    case 111:
        bpp = 16;
        break;
    case 112:
    case 114:
        bpp = 32;
        break;
    case 116:
        bpp = 128;
        break;
    default:
        require(compressed, "Unknown explicit native control format");
        break;
    }
    Bytes bytes(128);
    put(bytes, 0, 0x20534444);
    put(bytes, 4, 124);
    put(bytes, 8, 0x20000);
    put(bytes, 12, 4);
    put(bytes, 16, 4);
    put(bytes, 28, 3);
    put(bytes, 76, 32);
    put(bytes, 80, pf);
    put(bytes, 84, fourcc);
    put(bytes, 88, bpp);
    put(bytes, 112, cube ? 0xfe00 : 0);
    for (unsigned i = 0; i < 4; ++i)
        put(bytes, 92 + i * 4, masks[i]);
    for (unsigned face = 0; face < (cube ? 6u : 1u); ++face)
        for (unsigned mip = 0; mip < 3; ++mip) {
            const unsigned count = (4u >> mip) * (4u >> mip), seed = face * 5 + mip * 3;
            if (compressed) {
                if (format == 0x33545844)
                    for (unsigned i = 0; i < 8; ++i)
                        bytes.push_back(static_cast<std::uint8_t>(i * 31 + seed));
                if (format == 0x35545844) {
                    bytes.push_back(13);
                    bytes.push_back(241);
                    for (unsigned i = 0; i < 6; ++i)
                        bytes.push_back(static_cast<std::uint8_t>(i * 37 + seed));
                }
                append(bytes, (seed % 2) == 0 ? 0x001f : 0xf800, 2);
                append(bytes, (seed % 2) == 0 ? 0xf800 : 0x001f, 2);
                append(bytes, 0xe4e4e4e4u + (seed % 4) * 0x11111111u, 4);
            } else if (format >= 111 && format <= 116) {
                const float values[] = {-1, 0, .125f, .5f, 1, 2, .75f};
                const unsigned half[] = {0xbc00, 0, 0x3000, 0x3800, 0x3c00, 0x4000, 0x3a00};
                const auto width = format <= 113 ? 2u : 4u;
                for (unsigned i = 0; i < count * bpp / (8 * width); ++i)
                    append(bytes,
                           width == 2 ? half[(i + seed) % 7]
                                      : std::bit_cast<std::uint32_t>(values[(i + seed) % 7]),
                           width);
            } else if (format == 36 || format == 81 || format == 110) {
                const unsigned values[] = {0, 1, 255, 256, 32767, 32768, 65534, 65535};
                for (unsigned i = 0; i < count * bpp / 16; ++i)
                    append(bytes, values[(i + seed) % 8], 2);
            } else
                for (unsigned i = 0; i < count * bpp / 8; ++i)
                    bytes.push_back(static_cast<std::uint8_t>((i * 17 + seed * 29) % 256));
        }
    return {format, cube, DdsData::parse(std::move(bytes), NumericDdsProfile::legacy_d3d9)};
}
std::array<float, 4> coordinate(bool cube, unsigned face, unsigned mip, unsigned x, unsigned y) {
    const auto size = static_cast<float>(4u >> mip), u = (x + .5f) / size, v = (y + .5f) / size;
    if (!cube)
        return {u, v, 0, static_cast<float>(mip)};
    const auto s = 2 * u - 1, t = 2 * v - 1;
    std::array<float, 3> direction;
    switch (face) {
    case 0:
        direction = {1, -t, -s};
        break;
    case 1:
        direction = {-1, -t, s};
        break;
    case 2:
        direction = {s, 1, t};
        break;
    case 3:
        direction = {s, -1, -t};
        break;
    case 4:
        direction = {s, -t, 1};
        break;
    default:
        direction = {-s, -t, -1};
        break;
    }
    return {direction[0], direction[1], direction[2], static_cast<float>(mip)};
}
} // namespace
int main(int argc, char **argv) {
    try {
        require(argc == 2, "Usage: civ5_texture_sampling fresh-output.tsv");
        const fs::path output(argv[1]);
        require(!fs::exists(output), "Refusing output overwrite");
        NativeSampler native;
        GlContext context;
        GlSampler gl_sampler(context);
        const auto &gl = context.api();
        std::cout << context.description() << std::endl;
        std::ofstream out(output);
        require(bool(out), "Output open failed");
        out << "format\tcube\tface\tmip\tx\ty\td3d_r\td3d_g\td3d_b\td3d_a\tgl_r\tgl_g\tgl_b\tgl_a\tdefault_"
               "r\tdefault_g\tdefault_b\tdefault_a\topaque_r\topaque_g\topaque_b\topaque_a\n"
            << std::hexfloat;
        unsigned total = 0, case_count = 0;
        const std::pair<unsigned, bool> definitions[] = {
            {21, false},  {21, true},          {22, false},        {22, true},          {28, false},
            {50, false},  {81, false},         {36, false},        {114, false},        {113, true},
            {116, true},  {0x31545844, false}, {0x31545844, true}, {0x33545844, false}, {0x35545844, false},
            {111, false}, {112, false},        {115, false},       {110, false}};
        for (const auto [format, cube] : definitions) {
            try {
                auto test = make_case(format, cube);
                auto native_texture = native.upload(test.data, static_cast<D3DFORMAT>(format));
                GlTexture texture(context, test.data);
                const std::array<GLint, 4> standard{GL_RED, GL_GREEN, GL_BLUE, GL_ALPHA};
                auto opaque = texture.plan().swizzle;
                opaque[3] = GL_ONE;
                for (unsigned face = 0; face < (cube ? 6u : 1u); ++face)
                    for (unsigned mip = 0; mip < 3; ++mip)
                        for (unsigned y = 0; y < (4u >> mip); ++y)
                            for (unsigned x = 0; x < (4u >> mip); ++x) {
                                const auto point = coordinate(cube, face, mip, x, y);
                                const auto expected = native.sample(native_texture.value, cube, point);
                                gl.TextureParameteriv(texture.id(), GL_TEXTURE_SWIZZLE_RGBA,
                                                      texture.plan().swizzle.data());
                                const auto actual = gl_sampler.sample(texture, cube, point);
                                gl.TextureParameteriv(texture.id(), GL_TEXTURE_SWIZZLE_RGBA, standard.data());
                                const auto default_channels = gl_sampler.sample(texture, cube, point);
                                gl.TextureParameteriv(texture.id(), GL_TEXTURE_SWIZZLE_RGBA, opaque.data());
                                const auto forced_opaque = gl_sampler.sample(texture, cube, point);
                                out << format << '\t' << cube << '\t' << face << '\t' << mip << '\t' << x
                                    << '\t' << y;
                                for (const auto &values : {expected, actual, default_channels, forced_opaque})
                                    for (float value : values)
                                        out << '\t' << value;
                                out << '\n';
                                ++total;
                            }
                ++case_count;
                std::cout << "SAMPLED native format=" << format << " cube=" << cube << "; total=" << total
                          << std::endl;
            } catch (const std::exception &e) {
                throw std::runtime_error("Format " + std::to_string(format) + (cube ? " cube: " : " 2D: ") +
                                         e.what());
            }
        }
        out.close();
        require(bool(out), "Final sample output failed");
        std::cout << "DONE own D3D9/GLSL sampling: " << case_count << " cases / " << total
                  << " float4 samples; compare all output values" << std::endl;
        return 0;
    } catch (const std::exception &e) {
        std::cerr << "FAIL " << e.what() << '\n';
        return 1;
    }
}
