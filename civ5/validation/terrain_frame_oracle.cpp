#include "civ5/materials/terrain_frame.hpp"
#include <cstring>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <vector>
#include <xmmintrin.h>
using namespace civ5::materials;
using namespace civ5::terrain;
namespace {
void require(bool ok, const char *why) {
    if (!ok)
        throw std::invalid_argument(why);
}
struct Reader {
    std::vector<std::uint8_t> bytes;
    std::size_t at = 8;
    std::uint32_t word() {
        require(at <= bytes.size() && bytes.size() - at >= 4, "Truncated terrain frame primitive");
        std::uint32_t result = 0;
        for (unsigned i = 0; i < 4; ++i)
            result |= std::uint32_t(bytes[at++]) << (8 * i);
        return result;
    }
    float number() { return std::bit_cast<float>(word()); }
    void floats(auto &values) {
        for (auto &v : values)
            v = number();
    }
    void copy(auto &value) {
        require(at <= bytes.size() && bytes.size() - at >= sizeof(value), "Truncated terrain prior state");
        std::memcpy(&value, bytes.data() + at, sizeof(value));
        at += sizeof(value);
    }
};
struct Writer {
    std::vector<std::uint8_t> bytes;
    void value(const auto &v) {
        const auto *p = reinterpret_cast<const std::uint8_t *>(&v);
        bytes.insert(bytes.end(), p, p + sizeof(v));
    }
    void lighting(const TerrainLighting &s) {
        value(s.angles);
        value(s.scalar);
        value(s.shadow);
        value(s.sun);
        value(s.reflection);
    }
};
} // namespace
int main(int argc, char **argv) {
    try {
        require(argc == 3, "Usage: terrain_frame_oracle primitive-input fresh-output");
        if (std::string(argv[1]) == "--rsqrt-operands") {
            require(!std::filesystem::exists(argv[2]), "Fresh host instruction observations required");
            unsigned count = 0;
            std::cin >> count;
            require(bool(std::cin) && count > 0 && count <= 64, "Bounded RSQRT operand count");
            Writer out;
            out.value(count);
            for (unsigned i = 0; i < count; ++i) {
                std::uint32_t bits = 0;
                std::cin >> bits;
                const auto operand = std::bit_cast<float>(bits);
                require(bool(std::cin) && std::isfinite(operand) && operand > 0,
                        "Positive finite original RSQRT operand");
                out.value(bits);
                out.value(_mm_cvtss_f32(_mm_rsqrt_ss(_mm_set_ss(operand))));
            }
            std::string trailing;
            require(!(std::cin >> trailing), "No trailing RSQRT operands");
            std::ofstream output(argv[2], std::ios::binary);
            output.write(reinterpret_cast<const char *>(out.bytes.data()), out.bytes.size());
            output.close();
            require(bool(output), "Write complete host instruction observations");
            return 0;
        }
        if (std::string(argv[1]) == "--scene-estimate") {
            require(!std::filesystem::exists(argv[2]), "Fresh scene estimate required");
            Writer out;
            unsigned calls = 0;
            terrain_cache_lighting(terrain_lighting_defaults(X87Precision::double_precision),
                                   [&](float squared) {
                                       require(calls++ == 0, "One scene normalization estimate");
                                       const float seed = _mm_cvtss_f32(_mm_rsqrt_ss(_mm_set_ss(squared)));
                                       out.value(squared);
                                       out.value(seed);
                                       return seed;
                                   });
            require(calls == 1 && out.bytes.size() == 8, "Complete scene estimate");
            std::ofstream output(argv[2], std::ios::binary);
            output.write(reinterpret_cast<const char *>(out.bytes.data()), out.bytes.size());
            output.close();
            require(bool(output), "Write host scene normalization estimate");
            return 0;
        }
        std::ifstream input(argv[1], std::ios::binary | std::ios::ate);
        require(bool(input), "Open terrain frame input");
        const auto size = input.tellg();
        require(size >= 12 && size <= 16 * 1024 * 1024, "Bounded terrain frame input");
        Reader r{std::vector<std::uint8_t>(static_cast<std::size_t>(size))};
        input.seekg(0);
        input.read(reinterpret_cast<char *>(r.bytes.data()), size);
        require(bool(input) && std::memcmp(r.bytes.data(), "C5TFRM01", 8) == 0, "Terrain frame schema");
        const auto count = r.word();
        require(count > 0 && count <= 4096, "Terrain frame case count");
        Writer output;
        output.value(count);
        for (unsigned i = 0; i < count; ++i) {
            const auto pc = r.word(), update = r.word();
            require((pc == 24 || pc == 53) && update <= 1, "Terrain lighting mode");
            const auto precision = pc == 24 ? X87Precision::single : X87Precision::double_precision;
            auto lighting = terrain_lighting_defaults(precision);
            output.lighting(lighting);
            if (update)
                lighting = terrain_lighting_update(lighting, precision);
            output.lighting(lighting);
            TerrainFrameInput in{};
            for (auto *matrix :
                 {&in.view, &in.inverse_view, &in.projection, &in.shadow_projection, &in.overlay_projection})
                r.floats(*matrix);
            r.floats(in.world_scales);
            in.shadow_direction = lighting.shadow;
            in.sun_direction = lighting.sun;
            in.dimensions = {r.word(), r.word()};
            in.screen = {r.word(), r.word()};
            r.floats(in.overlay);
            r.floats(in.fog_color);
            in.fog_density = r.number();
            in.fog_strength = r.number();
            in.fog_distance = r.number();
            in.texture_extent = r.number();
            in.control = r.number();
            in.level = r.word();
            for (auto &v : in.options)
                v = r.word();
            TerrainFrameState prior;
            r.copy(prior.constants);
            prior.minimum_level = r.word();
            prior.maximum_level = r.word();
            const auto squared_bits = r.word(), seed_bits = r.word();
            unsigned calls = 0;
            const auto result = terrain_frame(in, prior, precision, [&](float squared) {
                require(calls++ == 0 && std::bit_cast<std::uint32_t>(squared) == squared_bits,
                        "Original terrain half-vector RSQRT input differs");
                return std::bit_cast<float>(seed_bits);
            });
            require(calls == 1, "Original terrain half-vector normalization count");
            output.value(result.constants);
            output.value(result.fog);
            output.value(result.minimum_level);
            output.value(result.maximum_level);
        }
        require(r.at == r.bytes.size(), "Trailing terrain frame input");
        require(!std::filesystem::exists(argv[2]), "Fresh terrain frame output required");
        std::ofstream out(argv[2], std::ios::binary);
        out.write(reinterpret_cast<const char *>(output.bytes.data()), output.bytes.size());
        out.close();
        require(bool(out), "Complete terrain frame output");
        std::cout << "PASS " << count << " original terrain frame cases\n";
        return 0;
    } catch (const std::exception &error) {
        std::cerr << "FAIL " << error.what() << '\n';
        return 1;
    }
}
