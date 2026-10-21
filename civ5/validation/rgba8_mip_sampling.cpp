// Own API shaders isolate readback side effects; original game shaders are
// exercised separately by civ5_gl_terrain_decal.
#include "texture_sampling_native.hpp"
#include <fstream>
#include <random>
#include <vector>

namespace sv = texture_sampling_validation;
using namespace civ5::assets;
using namespace civ5::graphics;
namespace {
using Sample = std::array<float, 4>;
DdsData constants() {
    Bytes bytes(128);
    const auto set = [&](unsigned at, std::uint32_t value) { std::memcpy(bytes.data() + at, &value, 4); };
    for (auto [at, value] : std::initializer_list<std::pair<unsigned, unsigned>>{
             {0, 0x20534444}, {4, 124}, {8, 0x2100f}, {12, 8}, {16, 8}, {20, 32}, {28, 4},
             {76, 32}, {80, 0x41}, {88, 32}, {92, 0xff0000}, {96, 0xff00}, {100, 0xff},
             {104, 0xff000000}, {108, 0x401008}})
        set(at, value);
    const std::array<std::array<std::uint8_t, 4>, 4> colors{
        {{13, 71, 191, 255}, {127, 128, 17, 71}, {255, 0, 128, 127}, {0, 255, 64, 128}}};
    for (unsigned mip = 0; mip < 4; ++mip)
        for (unsigned i = 0; i < (8u >> mip) * (8u >> mip); ++i)
            bytes.insert(bytes.end(), colors[mip].begin(), colors[mip].end());
    return DdsData::parse(std::move(bytes));
}
Bytes read(const char *path) {
    std::ifstream in(path, std::ios::binary | std::ios::ate);
    sv::require(bool(in), "Missing original zero DDS");
    const auto size = in.tellg();
    sv::require(size > 0 && size < 1024 * 1024, "Unexpected zero DDS length");
    Bytes bytes(static_cast<std::size_t>(size));
    in.seekg(0);
    sv::require(bool(in.read(reinterpret_cast<char *>(bytes.data()), bytes.size())), "Truncated zero DDS");
    return bytes;
}
std::vector<Sample> coordinates() {
    // UV/LOD captured from the original decal's first differing cache pixel.
    std::vector<Sample> result{{1.6590909957885742f, 2.825000047683716f, 0, 2.67578125f}};
    for (float lod : {-.5f, 0.f, .125f, .25f, .5f, .75f, .875f, 1.f, 1.5f, 2.1f,
                      2.25f, 3.125f, 3.5f, 4.375f, 5.25f, 6.f, 8.f})
        for (const auto &uv : std::array<std::array<float, 2>, 5>{
                 {{0, 0}, {.03125f, .125f}, {.18f, .23f}, {-.03f, 1.42f}, {2.2205882f, 1.2613636f}}})
            result.push_back({uv[0], uv[1], 0, lod});
    std::mt19937 rng(107);
    for (unsigned i = 0; i < 64; ++i) {
        const auto unit = [&] { return float(rng() & 65535) / 65536.f; };
        result.push_back({unit() * 4 - 1, unit() * 4 - 1, 0, unit() * 8 - 1});
    }
    return result;
}
void read_mips(GlContext &context, const GlTexture &texture, const DdsData &data,
               bool rgba_upload, bool direct) {
    for (const auto &level : data.levels()) {
        Bytes actual;
        if (direct) {
            actual.resize(level.length);
            context.api().GetTextureImage(texture.id(), level.mip, GL_BGRA, GL_UNSIGNED_BYTE,
                                          static_cast<GLsizei>(actual.size()), actual.data());
            context.check("Direct-readback negative control");
        } else {
            actual = texture.read_mip_raw(level.mip);
            if (rgba_upload)
                for (std::size_t i = 0; i < actual.size(); i += 4)
                    std::swap(actual[i], actual[i + 2]);
        }
        const auto expected = data.payload(0, level.mip);
        sv::require(actual.size() == expected.size() &&
                        std::equal(actual.begin(), actual.end(), expected.begin()),
                    "Readback changed source mip bytes");
    }
}
} // namespace
int main(int argc, char **argv) {
    try {
        sv::require(argc == 3 || argc == 4,
                    "Usage: civ5_rgba8_mip_sampling original-resource-zero.dds fresh-output.tsv "
                    "[--direct-readback-control]");
        const bool direct = argc == 4;
        sv::require(!direct || std::string_view(argv[3]) == "--direct-readback-control", "Unknown option");
        sv::require(!std::filesystem::exists(argv[2]), "Fresh sampler output required");
        std::ofstream out(argv[2]);
        sv::require(bool(out), "Sampler output open");
        out << "upload\ttexture\tsrgb\tu\tv\tlod";
        for (const char *group : {"native", "before", "after", "repeat"})
            for (char c : std::string_view("rgba")) out << '\t' << group << '_' << c;
        out << '\n' << std::hexfloat;
        sv::NativeSampler native;
        GlContext context;
        sv::GlSampler sampler(context);
        for (auto [state, value] : std::initializer_list<std::pair<D3DSAMPLERSTATETYPE, DWORD>>{
                 {D3DSAMP_ADDRESSU, D3DTADDRESS_WRAP}, {D3DSAMP_ADDRESSV, D3DTADDRESS_WRAP},
                 {D3DSAMP_ADDRESSW, D3DTADDRESS_WRAP}, {D3DSAMP_MINFILTER, D3DTEXF_LINEAR},
                 {D3DSAMP_MAGFILTER, D3DTEXF_LINEAR}, {D3DSAMP_MIPFILTER, D3DTEXF_LINEAR}})
            sv::checked(native.device->SetSamplerState(0, state, value), "Linear repeat native sampler");
        sampler.SamplerParameteri(sampler.sampler, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        sampler.SamplerParameteri(sampler.sampler, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        for (auto axis : {GL_TEXTURE_WRAP_S, GL_TEXTURE_WRAP_T, GL_TEXTURE_WRAP_R})
            sampler.SamplerParameteri(sampler.sampler, axis, GL_REPEAT);
        std::array<DdsData, 2> data{DdsData::parse(read(argv[1])), constants()};
        sv::require(data[0].width() == 256 && data[0].height() == 256 && data[0].mip_count() == 7,
                    "Unexpected original resource-zero dimensions");
        const auto probes = coordinates();
        unsigned count = 0, mismatches = 0, changed = 0;
        for (unsigned upload = 0; upload < 2; ++upload)
            for (unsigned source = 0; source < data.size(); ++source) {
                auto texture = native.upload(data[source], D3DFMT_A8R8G8B8);
                for (unsigned srgb = 0; srgb < 2; ++srgb) {
                    std::vector<Rgba8Level> levels;
                    for (const auto &level : data[source].levels()) {
                        const auto bgra = data[source].payload(0, level.mip);
                        Bytes rgba(bgra.begin(), bgra.end());
                        for (std::size_t i = 0; i < rgba.size(); i += 4) std::swap(rgba[i], rgba[i + 2]);
                        levels.push_back({level.width, level.height, std::move(rgba)});
                    }
                    const auto color = srgb ? DdsSamplingColor::srgb : DdsSamplingColor::linear;
                    const auto make_texture = [&] {
                        return upload ? GlTexture::rgba8(context, levels, color)
                                      : GlTexture(context, data[source], color);
                    };
                    GlTexture pristine = make_texture();
                    sv::checked(native.device->SetSamplerState(0, D3DSAMP_SRGBTEXTURE, srgb), "Color decode");
                    std::vector<Sample> expected, before, after, repeat;
                    for (const auto &p : probes) {
                        expected.push_back(native.sample(texture.value, false, p));
                        before.push_back(sampler.sample(pristine, false, p));
                    }
                    // The defect depends on reading before the first sample.
                    // Use a fresh sibling: warming this texture first masks it.
                    GlTexture gl = make_texture();
                    read_mips(context, gl, data[source], upload != 0, direct);
                    for (const auto &p : probes) after.push_back(sampler.sample(gl, false, p));
                    read_mips(context, gl, data[source], upload != 0, direct);
                    for (const auto &p : probes) repeat.push_back(sampler.sample(gl, false, p));
                    for (std::size_t i = 0; i < probes.size(); ++i) {
                        out << upload << '\t' << source << '\t' << srgb << '\t' << probes[i][0] << '\t'
                            << probes[i][1] << '\t' << probes[i][3];
                        for (const auto &sample : {expected[i], before[i], after[i], repeat[i]})
                            for (float v : sample) out << '\t' << v;
                        out << '\n';
                        for (unsigned c = 0; c < 4; ++c) {
                            const auto bits = [](float v) { return std::bit_cast<std::uint32_t>(v); };
                            for (const auto &sample : {before[i], after[i], repeat[i]})
                                mismatches += bits(expected[i][c]) != bits(sample[c]);
                            changed += bits(before[i][c]) != bits(after[i][c]);
                        }
                        ++count;
                    }
                }
            }
        out.close();
        sv::require(bool(out), "Sampler output write");
        std::cout << (mismatches ? "FAIL " : "PASS ") << count << " RGBA8 probes; nativeMismatchChannels="
                  << mismatches << "; changedByReadback=" << changed << "; directControl=" << direct
                  << "; " << context.description() << '\n';
        return mismatches ? 1 : 0;
    } catch (const std::exception &e) {
        std::cerr << "FAIL " << e.what() << '\n';
        return 1;
    }
}
