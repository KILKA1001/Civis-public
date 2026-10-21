#include "civ5/materials/material_cache_projection.hpp"
#include "material_cache_acquisition_validation.hpp"

using namespace cache_acquisition_validation;
using namespace civ5::materials;
using namespace civ5::terrain;
namespace {
void floating(Bytes &out, float value) { word(out, std::bit_cast<std::uint32_t>(value)); }
} // namespace
int main(int argc, char **argv) {
    try {
        require(argc == 3, "Projection input and fresh output required");
        Reader reader(argv[1]);
        require(reader.bytes(8) == Bytes{'C', '5', 'P', 'R', 'O', 'J', 'I', '1'}, "Projection input schema");
        const auto count = reader.word();
        Bytes output{'C', '5', 'P', 'R', 'O', 'J', 'O', '1'};
        word(output, count);
        for (unsigned i = 0; i < count; ++i) {
            const auto pc = reader.word(), dispatch = reader.word(), width = reader.word(),
                       base = reader.word();
            require((pc == 24 || pc == 53) && dispatch <= 1, "Projection arithmetic profile");
            std::array<float, 6> bounds;
            Matrix4 matrix;
            for (auto &value : bounds)
                value = std::bit_cast<float>(reader.word());
            for (auto &value : matrix)
                value = std::bit_cast<float>(reader.word());
            const auto precision = pc == 24 ? X87Precision::single : X87Precision::double_precision;
            const auto path = dispatch ? Vec4Path::sse : Vec4Path::legacy_x87;
            MaterialCacheProjectionTrace trace;
            const auto result =
                material_cache_projection(bounds, matrix, width, base, precision, path, &trace);
            const auto ordinary = material_cache_projection(bounds, matrix, width, base, precision, path);
            require(std::bit_cast<std::uint64_t>(ordinary.metric) ==
                            std::bit_cast<std::uint64_t>(result.metric) &&
                        ordinary.selected_size == result.selected_size,
                    "Trace observations changed production projection output");
            const auto bits = std::bit_cast<std::uint64_t>(result.metric);
            word(output, static_cast<std::uint32_t>(bits));
            word(output, static_cast<std::uint32_t>(bits >> 32));
            floating(output, static_cast<float>(result.metric));
            word(output, result.selected_size);
            for (unsigned pair = 0; pair < 4; ++pair) {
                for (const auto &corner : trace.transformed[pair])
                    for (auto coordinate : corner)
                        floating(output, coordinate);
                for (const auto &corner : trace.normalized[pair])
                    for (auto coordinate : corner)
                        floating(output, coordinate);
                floating(output, trace.half_diagonals[pair]);
            }
        }
        require(reader.at == reader.data.size(), "Trailing projection input");
        require(!std::filesystem::exists(argv[2]), "Fresh projection output required");
        std::ofstream stream(argv[2], std::ios::binary);
        stream.write(reinterpret_cast<const char *>(output.data()), output.size());
        require(bool(stream), "Projection output failed");
        std::cout << "PASS production projected cache size " << count << " cases, with and without trace\n";
        return 0;
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
