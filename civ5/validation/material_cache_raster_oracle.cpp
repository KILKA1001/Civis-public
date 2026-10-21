#include "civ5/materials/material_cache_raster.hpp"
#include "material_blend_fixture.hpp"
#include <iostream>
using namespace blend_validation;
using generation_validation::word;
int main(int argc, char **argv) {
    try {
        require(argc == 3, "Usage: civ5_material_cache_raster_oracle input fresh-output");
        const auto input = read(argv[1]);
        Reader r{input};
        const auto magic = r.take(8);
        require(std::string_view(reinterpret_cast<const char *>(magic.data()), 8) == "C5CRIN01",
                "Cache raster input magic");
        const auto count = r.word();
        require(count > 0 && count <= 10000, "Cache raster input count");
        Bytes output;
        const auto vertices = material_cache_normal_vertices();
        const auto indices = material_cache_normal_indices();
        const auto append = [&](const auto &values) {
            const auto *p = reinterpret_cast<const std::uint8_t *>(values.data());
            output.insert(output.end(), p, p + sizeof(values));
        };
        append(vertices);
        append(indices);
        for (unsigned i = 0; i < count; ++i) {
            const auto flags = r.word(), conditional = r.word();
            require(conditional <= 1, "Invalid clear control");
            std::array<float, 4> color;
            const auto raw = r.take(16);
            std::memcpy(color.data(), raw.data(), 16);
            const auto packed = material_cache_clear_argb(color);
            word(output, material_cache_clear_target(flags, conditional != 0));
            word(output, packed);
            const auto rgba = material_cache_clear_rgba(packed);
            append(rgba);
        }
        r.end();
        write(argv[2], output);
        std::cout << "PASS cache raster primitives=" << count << '\n';
    } catch (const std::exception &e) {
        std::cerr << "FAIL " << e.what() << '\n';
        return 1;
    }
}
