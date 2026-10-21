#include "civ5/materials/material_cache_ordering.hpp"
#include "material_cache_acquisition_validation.hpp"

using namespace cache_acquisition_validation;
using namespace civ5::materials;
namespace {
MaterialCachePriority priority(Reader &reader) {
    return {std::bit_cast<std::int32_t>(reader.word()), std::bit_cast<std::int32_t>(reader.word()),
            reader.word(), std::bit_cast<float>(reader.word())};
}
} // namespace
int main(int argc, char **argv) {
    try {
        require(argc == 3, "Material ordering input and fresh output required");
        Reader reader(argv[1]);
        const auto magic = reader.bytes(8);
        const bool pairs = magic == Bytes{'C', '5', 'C', 'M', 'P', 'A', 'R', 'I'};
        require(pairs || magic == Bytes{'C', '5', 'O', 'R', 'D', 'E', 'R', 'I'}, "Material ordering schema");
        const auto count = reader.word();
        auto output = magic;
        output[7] = 'O';
        word(output, count);
        for (unsigned i = 0; i < count; ++i) {
            if (pairs) {
                const auto a = priority(reader), b = priority(reader);
                word(output, std::bit_cast<std::uint32_t>(material_cache_compare_priority(a, b)));
                continue;
            }
            const auto size = reader.word();
            require(size <= 1024 * 1024, "Material order fixture exceeds validation capacity");
            std::vector<MaterialCachePriority> keys;
            keys.reserve(size);
            for (unsigned j = 0; j < size; ++j)
                keys.push_back(priority(reader));
            std::vector<std::uint32_t> order(size);
            for (auto &index : order)
                index = reader.word();
            auto ordinary = order;
            MaterialCacheOrderTrace trace;
            material_cache_order(keys, order, &trace);
            material_cache_order(keys, ordinary);
            require(ordinary == order, "Comparison observations changed material order");
            word(output, size);
            for (const auto index : order)
                word(output, index);
            word(output, static_cast<std::uint32_t>(trace.size()));
            for (const auto &comparison : trace) {
                word(output, comparison.first);
                word(output, comparison.second);
                word(output, std::bit_cast<std::uint32_t>(comparison.result));
            }
        }
        require(reader.at == reader.data.size(), "Trailing material ordering input");
        require(!std::filesystem::exists(argv[2]), "Fresh material ordering output required");
        std::ofstream stream(argv[2], std::ios::binary);
        stream.write(reinterpret_cast<const char *>(output.data()), output.size());
        require(bool(stream), "Material ordering output failed");
        std::cout << "PASS production material ordering " << count << (pairs ? " pairs\n" : " sorts\n");
        return 0;
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
