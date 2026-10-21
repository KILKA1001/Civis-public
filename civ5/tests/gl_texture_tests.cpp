#include "civ5/graphics/gl_texture.hpp"
#include <array>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <thread>

using namespace civ5::assets;
using namespace civ5::graphics;
namespace {
int checks = 0;
void check(bool value, const char *reason) {
    ++checks;
    if (!value)
        throw std::runtime_error(reason);
}
template <class F> void rejects(F fn) {
    bool failed = false;
    try {
        fn();
    } catch (const std::exception &) {
        failed = true;
    }
    check(failed, "Expected rejection");
}
void word(Bytes &bytes, std::size_t at, std::uint32_t v) {
    for (unsigned i = 0; i < 4; ++i)
        bytes.at(at + i) = static_cast<std::uint8_t>(v >> (8 * i));
}
DdsData fixture(unsigned code, bool cube = false, unsigned bpp = 0, std::array<std::uint32_t, 4> masks = {}) {
    const bool compressed = code > 0xffffff;
    const unsigned bits = compressed                   ? 0
                          : code == 0                  ? bpp
                          : code == 111                ? 16
                          : code == 112 || code == 114 ? 32
                          : code == 116                ? 128
                                                       : 64;
    Bytes bytes(128);
    word(bytes, 0, 0x20534444);
    word(bytes, 4, 124);
    word(bytes, 8, 0x20000);
    word(bytes, 12, cube ? 4 : 3);
    word(bytes, 16, cube ? 4 : 5);
    word(bytes, 28, 3);
    word(bytes, 76, 32);
    const auto flags = code ? 4 : masks[0] == 0 ? 2 : masks[1] == 0 && masks[2] == 0 ? 0x20000 : 0x40;
    word(bytes, 80, flags);
    word(bytes, 84, code);
    word(bytes, 88, bpp);
    word(bytes, 112, cube ? 0xfe00 : 0);
    for (unsigned i = 0; i < 4; ++i)
        word(bytes, 92 + 4 * i, masks[i]);
    for (unsigned face = 0; face < (cube ? 6u : 1u); ++face)
        for (unsigned mip = 0; mip < 3; ++mip) {
            const auto w = std::max(1u, (cube ? 4u : 5u) >> mip), h = std::max(1u, (cube ? 4u : 3u) >> mip);
            const auto count =
                compressed ? ((w + 3) / 4) * ((h + 3) / 4) * (code == 0x31545844 ? 8 : 16) : w * h * bits / 8;
            // Finite float/half patterns, plus unique face/mip/byte identities.
            for (unsigned i = 0; i < count; ++i)
                bytes.push_back(static_cast<std::uint8_t>((i + face * 17 + mip * 3) % 64));
        }
    return DdsData::parse(std::move(bytes), NumericDdsProfile::legacy_d3d9);
}
void cpu() {
    check(dds_upload_plan(fixture(36)).internal_format == GL_RGBA16, "36 retains 16-bit UNORM channels");
    check(dds_upload_plan(fixture(112)).internal_format == GL_RG16F, "112 retains half floats");
    check(dds_upload_plan(fixture(114)).swizzle == std::array<GLint, 4>{GL_RED, GL_ONE, GL_ONE, GL_ONE},
          "D3D9 missing float channels");
    check(dds_upload_plan(fixture(0, false, 8, {0, 0, 0, 255})).swizzle ==
              std::array<GLint, 4>{GL_ZERO, GL_ZERO, GL_ZERO, GL_RED},
          "Native A8 semantics");
    check(dds_upload_plan(fixture(0, false, 16, {65535, 0, 0, 0})).swizzle ==
              std::array<GLint, 4>{GL_RED, GL_RED, GL_RED, GL_ONE},
          "L16 channel replication");
    check(dds_upload_plan(fixture(0, false, 32, {0xff0000, 0xff00, 0xff, 0})).swizzle[3] == GL_ONE,
          "X8 alpha is one, not its stored unused byte");
    rejects([] { dds_upload_plan(fixture(0, false, 32, {1, 2, 4, 8})); });
    for (auto [code, format] : std::array<std::pair<unsigned, GLenum>, 3>{
             {{0x31545844u, 0x8c4d}, {0x33545844u, 0x8c4e}, {0x35545844u, 0x8c4f}}}) {
        const auto plan = dds_upload_plan(fixture(code, true), DdsSamplingColor::srgb);
        check(plan.internal_format == format && plan.compressed, "Explicit BC sRGB mapping");
        check(plan.swizzle == std::array<GLint, 4>{GL_RED, GL_GREEN, GL_BLUE, GL_ALPHA},
              "sRGB retains alpha");
    }
    rejects([] { dds_upload_plan(fixture(116), DdsSamplingColor::srgb); });
    for (auto masks : {std::array<unsigned, 4>{0xff, 0xff00, 0xff0000, 0xff000000},
                       {0xff0000, 0xff00, 0xff, 0xff000000},
                       {0xff, 0xff00, 0xff0000, 0},
                       {0xff0000, 0xff00, 0xff, 0}}) {
        const auto plan = dds_upload_plan(fixture(0, false, 32, masks), DdsSamplingColor::srgb);
        check(plan.internal_format == GL_SRGB8_ALPHA8 && !plan.compressed &&
                  plan.swizzle[3] == (masks[3] ? GL_ALPHA : GL_ONE),
              "RGBA8 sRGB storage and alpha");
    }
    rejects([] { dds_upload_plan(fixture(0, false, 16, {0xf800, 0x7e0, 0x1f, 0}), DdsSamplingColor::srgb); });
    rejects([] { dds_upload_plan(fixture(0x31545844u), static_cast<DdsSamplingColor>(-1)); });
}
void gpu() {
    GlContext context;
    const auto &gl = context.api();
    std::cout << context.description() << '\n';
    check(context.has_extension("GL_EXT_texture_compression_s3tc"), "Original BC formats require S3TC");
    std::vector<DdsData> cases;
    for (const auto code :
         {36u, 110u, 111u, 112u, 113u, 114u, 115u, 116u, 0x31545844u, 0x33545844u, 0x35545844u})
        for (bool cube : {false, true})
            cases.push_back(fixture(code, cube));
    for (bool cube : {false, true}) {
        cases.push_back(fixture(0, cube, 8, {0, 0, 0, 255}));
        cases.push_back(fixture(0, cube, 8, {255, 0, 0, 0}));
        cases.push_back(fixture(0, cube, 16, {65535, 0, 0, 0}));
        cases.push_back(fixture(0, cube, 16, {255, 0, 0, 0xff00}));
        cases.push_back(fixture(0, cube, 32, {0xff0000, 0xff00, 0xff, 0xff000000}));
        cases.push_back(fixture(0, cube, 32, {0xff0000, 0xff00, 0xff, 0}));
        cases.push_back(fixture(0, cube, 32, {0xff, 0xff00, 0xff0000, 0xff000000}));
        cases.push_back(fixture(0, cube, 32, {0xffff, 0xffff0000, 0, 0}));
        cases.push_back(fixture(0, cube, 24, {0xff0000, 0xff00, 0xff, 0}));
        cases.push_back(fixture(0, cube, 16, {0xf800, 0x7e0, 0x1f, 0}));
        cases.push_back(fixture(0, cube, 16, {0x7c00, 0x3e0, 0x1f, 0x8000}));
        cases.push_back(fixture(0, cube, 16, {0xf00, 0xf0, 0xf, 0xf000}));
    }
    auto create_buffers = reinterpret_cast<PFNGLCREATEBUFFERSPROC>(context.proc_address("glCreateBuffers"));
    auto delete_buffers = reinterpret_cast<PFNGLDELETEBUFFERSPROC>(context.proc_address("glDeleteBuffers"));
    GLuint pbo = 0;
    create_buffers(1, &pbo);
    GLuint previous2d = 0, previous_cube = 0;
    gl.CreateTextures(GL_TEXTURE_2D, 1, &previous2d);
    gl.CreateTextures(GL_TEXTURE_CUBE_MAP, 1, &previous_cube);
    auto active_texture = reinterpret_cast<PFNGLACTIVETEXTUREPROC>(context.proc_address("glActiveTexture"));
    active_texture(GL_TEXTURE3);
    gl.BindTexture(GL_TEXTURE_2D, previous2d);
    gl.BindTexture(GL_TEXTURE_CUBE_MAP, previous_cube);
    gl.BindBuffer(GL_PIXEL_UNPACK_BUFFER, pbo);
    gl.BindBuffer(GL_PIXEL_PACK_BUFFER, pbo);
    const std::array<GLenum, 12> states{GL_UNPACK_ALIGNMENT,   GL_UNPACK_ROW_LENGTH, GL_UNPACK_IMAGE_HEIGHT,
                                        GL_UNPACK_SKIP_PIXELS, GL_UNPACK_SKIP_ROWS,  GL_UNPACK_SKIP_IMAGES,
                                        GL_PACK_ALIGNMENT,     GL_PACK_ROW_LENGTH,   GL_PACK_IMAGE_HEIGHT,
                                        GL_PACK_SKIP_PIXELS,   GL_PACK_SKIP_ROWS,    GL_PACK_SKIP_IMAGES};
    for (auto state : states)
        gl.PixelStorei(state, state == GL_PACK_ALIGNMENT || state == GL_UNPACK_ALIGNMENT ? 8 : 7);
    gl.PixelStorei(GL_PACK_SWAP_BYTES, 1);
    gl.PixelStorei(GL_UNPACK_SWAP_BYTES, 1);
    gl.PixelStorei(GL_PACK_LSB_FIRST, 1);
    gl.PixelStorei(GL_UNPACK_LSB_FIRST, 1);
    const std::array<GLenum, 8> block_fields{
        GL_PACK_COMPRESSED_BLOCK_WIDTH,   GL_PACK_COMPRESSED_BLOCK_HEIGHT,  GL_PACK_COMPRESSED_BLOCK_DEPTH,
        GL_PACK_COMPRESSED_BLOCK_SIZE,    GL_UNPACK_COMPRESSED_BLOCK_WIDTH, GL_UNPACK_COMPRESSED_BLOCK_HEIGHT,
        GL_UNPACK_COMPRESSED_BLOCK_DEPTH, GL_UNPACK_COMPRESSED_BLOCK_SIZE};
    const std::array<GLint, 4> block_values{8, 4, 1, 32};
    for (unsigned i = 0; i < block_fields.size(); ++i)
        gl.PixelStorei(block_fields[i], block_values[i % 4]);
    context.check("Deliberately hostile transfer state");
    // Deterministic TEST pixels only; these are not claimed to be original assets.
    // Seven explicit levels intentionally stop at 4x4 instead of generating 2x2/1x1.
    std::array<Bytes, 7> bgra_mip_bytes;
    std::array<Bgra8Level, 7> bgra_levels{};
    for (unsigned mip = 0; mip < bgra_levels.size(); ++mip) {
        const auto side = 256u >> mip;
        auto &bytes = bgra_mip_bytes[mip];
        bytes.resize(side * side * 4);
        for (unsigned y = 0; y < side; ++y)
            for (unsigned x = 0; x < side; ++x) {
                const auto at = (y * side + x) * 4;
                bytes[at] = static_cast<std::uint8_t>(x * 3 + y * 5 + mip * 17 + 1);
                bytes[at + 1] = static_cast<std::uint8_t>(x * 7 + y * 11 + mip * 29 + 37);
                bytes[at + 2] = static_cast<std::uint8_t>(x * 13 + y * 19 + mip * 43 + 83);
                bytes[at + 3] = static_cast<std::uint8_t>(x * 23 + y * 31 + mip * 61 + 149);
            }
        bgra_levels[mip] = {side, side, std::span<const std::uint8_t>(bytes)};
    }
    GLuint bgra_id = 0;
    {
        auto texture = GlTexture::bgra8(context, bgra_levels);
        bgra_id = texture.id();
        check(texture.belongs_to(context) && gl.IsTexture(bgra_id), "BGRA mip chain real GPU owner");
        check(texture.plan().internal_format == GL_RGBA8 && texture.plan().transfer_format == GL_BGRA &&
                  texture.plan().transfer_type == GL_UNSIGNED_BYTE && !texture.plan().compressed,
              "Explicit BGRA chain defaults to linear byte storage and BGRA transfer");
        for (unsigned mip = 0; mip < bgra_levels.size(); ++mip) {
            check(texture.read_mip_raw(mip) == bgra_mip_bytes[mip],
                  "All seven BGRA mip/channel/alpha payloads roundtrip exactly");
            GLint width = 0, height = 0;
            gl.GetTextureLevelParameteriv(bgra_id, mip, GL_TEXTURE_WIDTH, &width);
            gl.GetTextureLevelParameteriv(bgra_id, mip, GL_TEXTURE_HEIGHT, &height);
            check(width == static_cast<GLint>(bgra_levels[mip].width) &&
                      height == static_cast<GLint>(bgra_levels[mip].height),
                  "All seven explicit BGRA GPU mip extents match 256 through 4");
        }
        GLint count = 0, base = -1, maximum = -1;
        gl.GetTextureParameteriv(bgra_id, GL_TEXTURE_IMMUTABLE_LEVELS, &count);
        gl.GetTextureParameteriv(bgra_id, GL_TEXTURE_BASE_LEVEL, &base);
        gl.GetTextureParameteriv(bgra_id, GL_TEXTURE_MAX_LEVEL, &maximum);
        check(count == 7 && base == 0 && maximum == 6,
              "Incomplete BGRA chain allocates seven levels with BASE0/MAX6");
        rejects([&] { texture.read_mip_raw(7); });
    }
    check(!gl.IsTexture(bgra_id), "BGRA chain GPU ownership released");
    auto invalid_bgra = bgra_levels;
    invalid_bgra[3].bgra = invalid_bgra[3].bgra.first(invalid_bgra[3].bgra.size() - 1);
    rejects([&] { GlTexture::bgra8(context, invalid_bgra); });
    invalid_bgra = bgra_levels;
    invalid_bgra[1] = bgra_levels[2]; // Valid payload size, wrong contiguous mip dimension.
    rejects([&] { GlTexture::bgra8(context, invalid_bgra); });
    for (auto state : states) {
        GLint value = 0;
        gl.GetIntegerv(state, &value);
        check(value == (state == GL_PACK_ALIGNMENT || state == GL_UNPACK_ALIGNMENT ? 8 : 7),
              "BGRA upload/readback/rejection restores hostile pixel-store state");
    }
    for (auto state : {GL_PACK_SWAP_BYTES, GL_UNPACK_SWAP_BYTES, GL_PACK_LSB_FIRST, GL_UNPACK_LSB_FIRST}) {
        GLint value = 0;
        gl.GetIntegerv(state, &value);
        check(value == 1, "BGRA chain restores byte/bit order");
    }
    for (unsigned i = 0; i < block_fields.size(); ++i) {
        GLint value = 0;
        gl.GetIntegerv(block_fields[i], &value);
        check(value == block_values[i % 4], "BGRA chain restores compressed pixel-store fields");
    }
    for (auto state : {GL_PIXEL_PACK_BUFFER_BINDING, GL_PIXEL_UNPACK_BUFFER_BINDING}) {
        GLint value = 0;
        gl.GetIntegerv(state, &value);
        check(value == static_cast<GLint>(pbo), "BGRA chain restores hostile PBO bindings");
    }
    for (auto [state, value] : std::initializer_list<std::pair<GLenum, GLint>>{
             {GL_ACTIVE_TEXTURE, GL_TEXTURE3},
             {GL_TEXTURE_BINDING_2D, static_cast<GLint>(previous2d)},
             {GL_TEXTURE_BINDING_CUBE_MAP, static_cast<GLint>(previous_cube)}}) {
        GLint actual = 0;
        gl.GetIntegerv(state, &actual);
        check(actual == value, "BGRA chain restores active unit and texture bindings");
    }
    context.check("Explicit seven-level BGRA roundtrip and rejection state");
    // Unequal dimensions and a nonuniform alpha expose axis/channel swaps.
    Bytes volume_bytes(7 * 5 * 3 * 4);
    for (unsigned i = 0; i < volume_bytes.size(); ++i)
        volume_bytes[i] = static_cast<std::uint8_t>(i * 37 + i / 11);
    GLuint volume_id = 0;
    {
        auto volume = GlTexture::bgra8_volume(context, 7, 5, 3, volume_bytes);
        volume_id = volume.id();
        check(volume.depth() == 3 && volume.belongs_to(context), "Volume depth and context owner");
        check(volume.plan().internal_format == GL_RGBA8 && volume.plan().transfer_format == GL_BGRA,
              "Original linear A8R8G8B8 volume storage");
        check(volume.read_mip_raw(0) == volume_bytes && volume.read_level_raw(0, 0) == volume_bytes,
              "Entire BGRA volume survives hostile pixel transfer state");
        GLint depth = 0, levels = 0;
        gl.GetTextureLevelParameteriv(volume.id(), 0, GL_TEXTURE_DEPTH, &depth);
        gl.GetTextureParameteriv(volume.id(), GL_TEXTURE_IMMUTABLE_LEVELS, &levels);
        check(depth == 3 && levels == 1, "GPU volume depth and single immutable mip");
        auto moved = std::move(volume);
        check(volume.id() == 0 && moved.depth() == 3, "Volume move construction retains depth");
        volume = std::move(moved);
        check(moved.id() == 0 && volume.depth() == 3 && volume.read_mip_raw(0) == volume_bytes,
              "Volume move assignment retains payload and depth");
        rejects([&] { moved.read_mip_raw(0); });
        rejects([&] { volume.read_level_raw(1, 0); });
        rejects([&] { volume.read_mip_raw(1); });
    }
    check(!gl.IsTexture(volume_id), "Volume GPU ownership released");
    for (const auto extent : {std::array<unsigned, 3>{0, 5, 3},
                              {7, 0, 3},
                              {7, 5, 0},
                              {7, 5, 4},
                              {0xffffffffu, 0xffffffffu, 0xffffffffu}})
        rejects([&] { GlTexture::bgra8_volume(context, extent[0], extent[1], extent[2], volume_bytes); });
    rejects([&] {
        GlTexture::bgra8_volume(context, 7, 5, 3, std::span(volume_bytes).first(volume_bytes.size() - 1));
    });
    for (auto state : states) {
        GLint actual = 0;
        gl.GetIntegerv(state, &actual);
        check(actual == (state == GL_PACK_ALIGNMENT || state == GL_UNPACK_ALIGNMENT ? 8 : 7),
              "Volume preserves hostile pixel store");
    }
    for (auto state : {GL_PIXEL_PACK_BUFFER_BINDING, GL_PIXEL_UNPACK_BUFFER_BINDING}) {
        GLint actual = 0;
        gl.GetIntegerv(state, &actual);
        check(actual == static_cast<GLint>(pbo), "Volume preserves hostile PBO binding");
    }
    for (const auto &data : cases)
        for (auto color : {DdsSamplingColor::linear, DdsSamplingColor::srgb}) {
            if (color == DdsSamplingColor::srgb && data.format() != DdsFormat::dxt1 &&
                data.format() != DdsFormat::dxt3 && data.format() != DdsFormat::dxt5 &&
                !(data.format() == DdsFormat::masked_rgb && data.bits_per_pixel() == 32 &&
                  data.channel_masks()[1] == 0xff00))
                continue;
            std::cout << "GPU storage case format=" << static_cast<unsigned>(data.format())
                      << " bpp=" << data.bits_per_pixel() << " faces=" << data.face_count() << std::endl;
            GLuint id = 0;
            {
                GlTexture texture(context, data, color);
                id = texture.id();
                check(gl.IsTexture(id) != 0, "Real owned GPU texture");
                for (const auto &level : data.levels()) {
                    const auto raw = texture.read_level_raw(level.face, level.mip);
                    const auto expected = data.payload(level.face, level.mip);
                    check(raw.size() == expected.size() &&
                              std::equal(raw.begin(), raw.end(), expected.begin()),
                          "Full GPU raw level roundtrip");
                }
                std::array<GLint, 4> swizzle{};
                gl.GetTextureParameteriv(id, GL_TEXTURE_SWIZZLE_RGBA, swizzle.data());
                check(swizzle == texture.plan().swizzle, "Installed sampling swizzle");
                GLint mips = 0;
                gl.GetTextureParameteriv(id, GL_TEXTURE_IMMUTABLE_LEVELS, &mips);
                check(mips == 3, "Exactly authored mip allocation");
                GLint storage = 0;
                gl.GetTextureLevelParameteriv(id, 0, GL_TEXTURE_INTERNAL_FORMAT, &storage);
                check(static_cast<GLenum>(storage) == texture.plan().internal_format,
                      "Explicit color storage installed");
                for (auto state : states) {
                    GLint actual = 0;
                    gl.GetIntegerv(state, &actual);
                    check(actual == (state == GL_PACK_ALIGNMENT || state == GL_UNPACK_ALIGNMENT ? 8 : 7),
                          "Pixel store restored");
                }
                for (auto state :
                     {GL_PACK_SWAP_BYTES, GL_UNPACK_SWAP_BYTES, GL_PACK_LSB_FIRST, GL_UNPACK_LSB_FIRST}) {
                    GLint actual = 0;
                    gl.GetIntegerv(state, &actual);
                    check(actual == 1, "Byte/bit order restored");
                }
                for (unsigned i = 0; i < block_fields.size(); ++i) {
                    GLint actual = 0;
                    gl.GetIntegerv(block_fields[i], &actual);
                    check(actual == block_values[i % 4], "Compressed pixel store restored");
                }
                for (auto [state, value] : std::initializer_list<std::pair<GLenum, GLint>>{
                         {GL_ACTIVE_TEXTURE, GL_TEXTURE3},
                         {GL_TEXTURE_BINDING_2D, static_cast<GLint>(previous2d)},
                         {GL_TEXTURE_BINDING_CUBE_MAP, static_cast<GLint>(previous_cube)}}) {
                    GLint actual = 0;
                    gl.GetIntegerv(state, &actual);
                    check(actual == value, "Texture bindings / active unit restored");
                }
                for (auto state : {GL_PIXEL_PACK_BUFFER_BINDING, GL_PIXEL_UNPACK_BUFFER_BINDING}) {
                    GLint actual = 0;
                    gl.GetIntegerv(state, &actual);
                    check(actual == static_cast<GLint>(pbo), "PBO binding restored");
                }
                GlTexture moved(std::move(texture));
                check(texture.id() == 0 && moved.id() == id, "Move transfers GPU ownership");
                rejects([&] { texture.bind(0); });
                rejects([&] { moved.read_level_raw(data.face_count(), 0); });
            }
            check(gl.IsTexture(id) == 0, "GPU object released");
        }
    // Producer-owned binary cube uses all six faces and authored mip extents;
    // deliberately hostile pack/unpack state above still applies.
    std::vector<Rgba8CubeLevel> cube_levels;
    for (unsigned mip = 0; mip < 3; ++mip) {
        Rgba8CubeLevel level{std::max(1u, 7u >> mip), {}};
        for (unsigned face = 0; face < 6; ++face) {
            auto &bytes = level.faces[face];
            bytes.resize(level.size * level.size * 4);
            for (unsigned i = 0; i < bytes.size(); ++i)
                bytes[i] = static_cast<std::uint8_t>(i + 31 * face + 17 * mip);
        }
        cube_levels.push_back(std::move(level));
    }
    GLuint cube_id = 0;
    {
        auto cube = GlTexture::rgba8_cube(context, cube_levels);
        cube_id = cube.id();
        check(cube.belongs_to(context), "RGBA8 cube context owner");
        check(cube.plan().internal_format == GL_RGBA8, "Auxiliary cube is linear, not authored sRGB");
        for (unsigned mip = 0; mip < 3; ++mip) {
            Bytes all;
            for (unsigned face = 0; face < 6; ++face) {
                const auto &bytes = cube_levels[mip].faces[face];
                check(cube.read_level_raw(face, mip) == bytes, "RGBA8 cube face/mip exact");
                all.insert(all.end(), bytes.begin(), bytes.end());
            }
            check(cube.read_mip_raw(mip) == all, "RGBA8 cube whole-mip face order");
        }
        auto moved = std::move(cube);
        check(!cube.belongs_to(context) && moved.belongs_to(context), "Cube ownership follows move");
        rejects([&] { cube.read_mip_raw(0); });
        rejects([&] { moved.read_level_raw(6, 0); });
        rejects([&] { moved.read_level_raw(0, 3); });
        {
            GlContext foreign;
            check(!moved.belongs_to(foreign), "Foreign context not same-name ownership");
        }
        context.api();
    }
    check(!gl.IsTexture(cube_id), "RGBA8 cube released");
    rejects([&] { GlTexture::rgba8_cube(context, {}); });
    auto invalid_cube = cube_levels;
    invalid_cube[0].size = 0;
    rejects([&] { GlTexture::rgba8_cube(context, invalid_cube); });
    invalid_cube = cube_levels;
    invalid_cube[1].size = 2;
    rejects([&] { GlTexture::rgba8_cube(context, invalid_cube); });
    invalid_cube = cube_levels;
    invalid_cube.push_back(invalid_cube.back());
    rejects([&] { GlTexture::rgba8_cube(context, invalid_cube); });
    invalid_cube = cube_levels;
    invalid_cube[0].size = 0xffffffffu;
    rejects([&] { GlTexture::rgba8_cube(context, invalid_cube); });
    for (unsigned face = 0; face < 6; ++face) {
        invalid_cube = cube_levels;
        invalid_cube[1].faces[face].pop_back();
        rejects([&] { GlTexture::rgba8_cube(context, invalid_cube); });
    }
    for (auto state : states) {
        GLint value = 0;
        gl.GetIntegerv(state, &value);
        check(value == (state == GL_PACK_ALIGNMENT || state == GL_UNPACK_ALIGNMENT ? 8 : 7),
              "Cube factory preserved pixel state");
    }
    for (auto state : {GL_PIXEL_PACK_BUFFER_BINDING, GL_PIXEL_UNPACK_BUFFER_BINDING}) {
        GLint value = 0;
        gl.GetIntegerv(state, &value);
        check(value == static_cast<GLint>(pbo), "Cube factory preserved PBO");
    }
    // Direct LEAN uploads also have to survive hostile pixel-store/PBO state.
    Bytes lean_bytes(36, 0xcc);
    for (auto [at, value] : std::initializer_list<std::pair<unsigned, unsigned>>{
             {0, 0x4e41454c}, {4, 0x3ea8f5c3}, {8, 8}, {12, 2}, {16, 4}, {20, 2}})
        word(lean_bytes, at, value);
    for (unsigned map = 0; map < 2; ++map) {
        word(lean_bytes, 24 + map * 4, static_cast<unsigned>(lean_bytes.size()));
        for (unsigned mip = 0; mip < 4; ++mip)
            for (unsigned i = 0; i < std::max(1u, 8u >> mip) * std::max(1u, 2u >> mip) * (map ? 2u : 8u); ++i)
                lean_bytes.push_back(static_cast<std::uint8_t>(i * 71 + mip * 53 + map * 17));
    }
    std::optional<GlTexture> lean_retained;
    {
        const auto data = LeanData::parse(lean_bytes);
        for (auto map : {LeanMap::rgba16, LeanMap::luminance16}) {
            auto texture = GlTexture::lean(context, data, map);
            for (const auto &level : data.levels(map)) {
                const auto bytes = data.payload(map, level.mip);
                const auto raw = texture.read_level_raw(0, level.mip);
                check(std::equal(raw.begin(), raw.end(), bytes.begin(), bytes.end()),
                      "Direct LEAN mip bytes");
            }
            check(texture.belongs_to(context), "LEAN context ownership");
            rejects([&] { texture.read_mip_raw(4); });
            rejects([&] { texture.read_level_raw(1, 0); });
            lean_retained = std::move(texture);
            rejects([&] { texture.bind(0); });
        }
        rejects([&] { GlTexture::lean(context, data, static_cast<LeanMap>(9)); });
        check(data.original_bytes() == lean_bytes, "LEAN GPU upload preserves authored header and maps");
    }
    check(lean_retained->read_mip_raw(3).size() == 2, "LEAN texture survives CPU asset release");
    lean_retained.reset();
    for (auto state : states) {
        GLint value = 0;
        gl.GetIntegerv(state, &value);
        check(value == (state == GL_PACK_ALIGNMENT || state == GL_UNPACK_ALIGNMENT ? 8 : 7),
              "LEAN factory restores hostile pixel-store state");
    }
    for (auto state : {GL_PIXEL_PACK_BUFFER_BINDING, GL_PIXEL_UNPACK_BUFFER_BINDING}) {
        GLint value = 0;
        gl.GetIntegerv(state, &value);
        check(value == static_cast<GLint>(pbo), "LEAN factory restores PBO state");
    }
    delete_buffers(1, &pbo);
    gl.DeleteTextures(1, &previous2d);
    gl.DeleteTextures(1, &previous_cube);
    context.check("Texture roundtrip and state tests");
    bool thread_rejected = false;
    std::thread other([&] {
        try {
            context.api();
        } catch (const std::runtime_error &) {
            thread_rejected = true;
        }
    });
    other.join();
    check(thread_rejected, "Cross-thread context access fails explicitly");
    std::optional<GlTexture> retained;
    {
        GlContext short_lived;
        retained.emplace(short_lived, fixture(36));
    }
    check(retained->read_level_raw(0, 0).size() == 120, "Resource retains real context lifetime");
    retained.reset();
    context.api();
    context.check("Restore first independent context");
}
} // namespace
int main(int argc, char **argv) {
    try {
        cpu();
        if (argc == 2 && std::string(argv[1]) == "--gpu")
            gpu();
        else
            check(argc == 1, "Usage: civ5_gl_texture_tests [--gpu]");
        std::cout << "PASS " << checks << " C++ texture checks\n";
        return 0;
    } catch (const std::exception &e) {
        std::cerr << "FAIL " << e.what() << '\n';
        return 1;
    }
}
