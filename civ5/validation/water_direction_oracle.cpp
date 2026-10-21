#include "civ5/materials/water_reflection.hpp"
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <vector>
using namespace civ5::materials;
namespace {
void require(bool ok, const char *why) {
    if (!ok)
        throw std::invalid_argument(why);
}
} // namespace
int main(int argc, char **argv) {
    try {
        require(argc == 3, "Usage: water_direction_oracle primitive-input fresh-output");
        std::ifstream input(argv[1], std::ios::binary | std::ios::ate);
        require(bool(input), "Open direction input");
        const auto size = input.tellg();
        require(size >= 12 && size <= 1024 * 1024, "Bounded direction input");
        std::vector<std::uint8_t> bytes(static_cast<std::size_t>(size));
        input.seekg(0);
        input.read(reinterpret_cast<char *>(bytes.data()), size);
        require(bool(input) && std::memcmp(bytes.data(), "C5WDIR01", 8) == 0, "Direction input schema");
        std::size_t at = 8;
        const auto word = [&] {
            require(at <= bytes.size() && bytes.size() - at >= 4, "Truncated direction primitive");
            std::uint32_t result = 0;
            for (unsigned i = 0; i < 4; ++i)
                result |= std::uint32_t(bytes[at++]) << (8 * i);
            return result;
        };
        const auto count = word();
        require(count > 0 && count <= 4096, "Direction case count");
        std::vector<std::uint8_t> output;
        const auto save = [&](const auto &value) {
            const auto *p = reinterpret_cast<const std::uint8_t *>(&value);
            output.insert(output.end(), p, p + sizeof(value));
        };
        save(count);
        for (unsigned i = 0; i < count; ++i) {
            const auto pc = word();
            require(pc == 24 || pc == 53, "Explicit direction precision");
            const WaterReflectionAngles angles{std::bit_cast<float>(word()), std::bit_cast<float>(word()),
                                               std::bit_cast<float>(word()), std::bit_cast<float>(word())};
            const auto result =
                water_reflection_directions(angles, pc == 24 ? civ5::terrain::X87Precision::single
                                                             : civ5::terrain::X87Precision::double_precision);
            save(result.light);
            save(result.reflection);
        }
        require(at == bytes.size(), "Trailing direction primitive bytes");
        require(!std::filesystem::exists(argv[2]), "Fresh direction output required");
        std::ofstream out(argv[2], std::ios::binary);
        out.write(reinterpret_cast<const char *>(output.data()), output.size());
        out.close();
        require(bool(out), "Complete direction output");
        std::cout << "PASS " << count << " original angle-to-direction cases\n";
        return 0;
    } catch (const std::exception &e) {
        std::cerr << "FAIL " << e.what() << '\n';
        return 1;
    }
}
