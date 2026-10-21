// Input-only U32 count / signed rebase / jitter / prior constant blocks.
#include "civ5/materials/material_shadows.hpp"
#include "material_blend_fixture.hpp"
#include <iostream>
using namespace blend_validation;
int main(int argc, char **argv) {
    try {
        require(argc == 3, "Usage: civ5_material_shadow_oracle input.bin fresh-output.bin");
        require(!std::filesystem::exists(argv[2]), "Fresh shadow output required");
        const auto data = read(argv[1]);
        Reader r{data};
        const auto magic = r.take(8);
        require(std::string_view(reinterpret_cast<const char *>(magic.data()), 8) == "C5MSIN01",
                "Shadow input schema");
        const auto records = r.word();
        require(records > 0 && records <= 256, "Shadow input count");
        const std::string_view out_magic = "C5MSOU01";
        Bytes output(out_magic.begin(), out_magic.end());
        generation_validation::word(output, records);
        for (unsigned record = 0; record < records; ++record) {
            const auto count = r.word();
            const auto blocks = r.word();
            require(blocks <= 256 && (count != 0 || blocks == 0), "Invalid shadow draw block count");
            const std::array<std::int32_t, 2> rebase{std::bit_cast<std::int32_t>(r.word()),
                                                     std::bit_cast<std::int32_t>(r.word())};
            generation_validation::word(output,
                                        std::bit_cast<std::uint32_t>(material_shadow_sample_weight(count)));
            generation_validation::word(output, blocks);
            for (unsigned i = 0; i < blocks; ++i) {
                const std::array<float, 2> jitter{std::bit_cast<float>(r.word()),
                                                  std::bit_cast<float>(r.word())};
                const auto prior = r.take(0x4c0);
                const auto result = material_shadow_draw_block(
                    count, rebase, jitter, std::span<const std::uint8_t, 0x4c0>(prior.data(), 0x4c0));
                output.insert(output.end(), result.begin(), result.end());
            }
        }
        r.end();
        write(argv[2], output);
        std::cout << "PASS shadow parameter records=" << records << '\n';
    } catch (const std::exception &e) {
        std::cerr << "FAIL " << e.what() << '\n';
        return 1;
    }
}
