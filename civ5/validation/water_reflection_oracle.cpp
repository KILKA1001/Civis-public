#include "civ5/materials/water_reflection.hpp"
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <vector>
using namespace civ5::materials;
using namespace civ5::terrain;
namespace {
void require(bool ok, const char *why) {
    if (!ok)
        throw std::invalid_argument(why);
}
struct Reader {
    std::vector<std::uint8_t> bytes;
    std::size_t at = 0;
    std::uint32_t word() {
        require(at <= bytes.size() && bytes.size() - at >= 4, "Truncated reflection primitive");
        std::uint32_t result = 0;
        for (unsigned i = 0; i < 4; ++i)
            result |= std::uint32_t(bytes[at++]) << (8 * i);
        return result;
    }
    void floats(auto &array) {
        for (auto &f : array)
            f = std::bit_cast<float>(word());
    }
    void matrices(WaterReflectionMatrices &values) {
        for (auto &array : {&values.shadow, &values.reflection, &values.reflection_view})
            for (auto &matrix : *array)
                floats(matrix);
        floats(values.reflection_projection);
    }
};
}
int main(int argc, char **argv) {
    try {
        require(argc == 3, "Usage: water_reflection_oracle primitive-input fresh-output");
        std::ifstream input(argv[1], std::ios::binary | std::ios::ate);
        require(bool(input), "Open reflection input");
        const auto size = input.tellg();
        require(size >= 16 && size <= 16 * 1024 * 1024, "Bounded reflection input");
        Reader reader{std::vector<std::uint8_t>(static_cast<std::size_t>(size))};
        input.seekg(0);
        input.read(reinterpret_cast<char *>(reader.bytes.data()), size);
        require(bool(input) && std::memcmp(reader.bytes.data(), "C5WRFL01", 8) == 0, "Reflection input schema");
        reader.at = 8;
        const auto resources = reader.word(), matrices = reader.word();
        require(resources <= 1024 && matrices <= 1024 && resources + matrices > 0, "Reflection case count");
        std::vector<std::uint8_t> output;
        const auto save = [&](const auto &value) {
            const auto *p = reinterpret_cast<const std::uint8_t *>(&value);
            output.insert(output.end(), p, p + sizeof(value));
        };
        save(resources); save(matrices);
        for (unsigned i = 0; i < resources; ++i) {
            const auto quality = reader.word(), reflections = reader.word();
            const auto plan = water_reflection_resource(quality, reflections);
            save(plan.water_mode); save(plan.width); save(plan.height);
            save(std::uint32_t(plan.render_target));
            if (!plan.render_target) {
                const auto fallback = water_reflection_fallback();
                require(fallback.width == plan.width && fallback.height == plan.height, "Fallback resource dimensions");
                save(std::uint32_t(fallback.rgba.size()));
                output.insert(output.end(), fallback.rgba.begin(), fallback.rgba.end());
            } else
                save(std::uint32_t(0));
        }
        for (unsigned i = 0; i < matrices; ++i) {
            const auto precision = reader.word();
            require(precision == 24 || precision == 53, "Explicit reflection precision");
            WaterReflectionFrame frame;
            frame.view_count = reader.word();
            reader.floats(frame.bounds_low); reader.floats(frame.bounds_high);
            reader.floats(frame.shadow_direction); reader.floats(frame.reflection_direction);
            reader.floats(frame.base_view_inverse);
            for (auto &m : frame.extra_views)
                reader.floats(m);
            WaterReflectionMatrices prior;
            reader.matrices(prior);
            std::array<std::array<unsigned, 2>, 4> estimates;
            for (auto &pair : estimates) {
                pair[0] = reader.word(); pair[1] = reader.word();
            }
            unsigned cursor = 0;
            const auto result = water_reflection_matrices(frame, prior,
                precision == 24 ? X87Precision::single : X87Precision::double_precision,
                [&](float squared) {
                    require(cursor < estimates.size() && std::bit_cast<unsigned>(squared) == estimates[cursor][0],
                            "Reflection RSQRT input differs from independent original");
                    return std::bit_cast<float>(estimates[cursor++][1]);
                });
            require(cursor == estimates.size(), "Incomplete reflection normalizations");
            static_assert(sizeof(result) == 832);
            save(result);
        }
        require(reader.at == reader.bytes.size(), "Trailing reflection primitive bytes");
        require(!std::filesystem::exists(argv[2]), "Fresh reflection output required");
        std::ofstream out(argv[2], std::ios::binary);
        out.write(reinterpret_cast<const char *>(output.data()), output.size());
        out.close();
        require(bool(out), "Complete reflection output");
        std::cout << "PASS " << resources << " reflection resource plans, " << matrices << " matrix cases\n";
        return 0;
    } catch (const std::exception &e) {
        std::cerr << "FAIL " << e.what() << '\n';
        return 1;
    }
}
