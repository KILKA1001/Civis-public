// Input-only parameter transport. Saved original outputs never enter C++.
#include "civ5/materials/material_normals.hpp"
#include "material_blend_fixture.hpp"
#include <iostream>
using namespace blend_validation;
int main(int argc, char **argv) {
    try {
        require(argc == 3, "Usage: civ5_material_normal_oracle input.bin fresh-output.bin");
        require(!std::filesystem::exists(argv[2]), "Fresh normal output required");
        const auto bytes = read(argv[1]);
        Reader reader{bytes};
        const auto magic = reader.take(8);
        require(std::string_view(reinterpret_cast<const char *>(magic.data()), 8) == "C5MNIN01",
                "Normal input schema");
        const auto count = reader.word();
        require(count > 0 && count <= 256, "Normal parameter count");
        const std::string_view out_magic = "C5MNOU01";
        Bytes output(out_magic.begin(), out_magic.end());
        generation_validation::word(output, count);
        for (std::uint32_t i = 0; i < count; ++i) {
            const auto resolution = std::bit_cast<std::int32_t>(reader.word());
            std::array<float, 8> prior;
            const auto raw = reader.take(32);
            std::memcpy(prior.data(), raw.data(), 32);
            const auto result = material_normal_parameters(resolution, prior);
            const auto *data = reinterpret_cast<const std::uint8_t *>(result.data());
            output.insert(output.end(), data, data + 32);
        }
        reader.end();
        write(argv[2], output);
        std::cout << "PASS parameters=" << count << '\n';
    } catch (const std::exception &e) {
        std::cerr << "FAIL " << e.what() << '\n';
        return 1;
    }
}
