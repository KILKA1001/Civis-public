#include "civ5/materials/fog_of_war.hpp"
#include <bit>
#include <fstream>
#include <iostream>

using namespace civ5::materials;
using namespace civ5::assets;
namespace fs = std::filesystem;
namespace {
void write(const fs::path &path, std::span<const std::uint8_t> bytes) {
    std::ofstream file(path, std::ios::binary);
    file.write(reinterpret_cast<const char *>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
    if (!file)
        throw std::runtime_error("Cannot write fog of war output");
}
template <typename T> void object(const fs::path &path, const T &value) {
    write(path, std::bit_cast<std::array<std::uint8_t, sizeof(T)>>(value));
}
} // namespace
int wmain(int argc, wchar_t **argv) {
    try {
        if (argc != 4)
            throw std::invalid_argument("Expected asset directory, quality and fresh output directory");
        const fs::path root(argv[1]), output(argv[3]);
        if (fs::exists(output))
            throw std::invalid_argument("Fresh fog of war output required");
        const auto quality = static_cast<std::uint32_t>(std::stoul(argv[2]));
        const LooseSource source(root);
        const auto assets = FogOfWarAssets::load(quality, [&](const char *path) {
            const auto result = source.find(AssetKey(path));
            if (!std::holds_alternative<AssetHandle>(result))
                throw std::invalid_argument("Missing or ambiguous cloud asset");
            return std::get<AssetHandle>(result);
        });
        const auto geometry = fog_of_war_geometry();
        const auto records = fog_of_war_draw_records(quality, {1, 2, 3, 4, 1, {2, 3, 4}});
        fs::create_directories(output);
        object(output / "startup.bin", assets.state());
        object(output / "1-index.bin", geometry.mask_indices);
        object(output / "2-vertex.bin", geometry.mask_vertices);
        object(output / "3-index.bin", geometry.layer_indices);
        object(output / "4-vertex.bin", geometry.layer_vertices);
        for (unsigned i = 0; i < records.size(); ++i)
            write(output / ("draw" + std::to_string(i + 1) + ".bin"), records[i]);
        if (assets.state().quality == 0)
            write(output / "fallback.bgra", FogOfWarAssets::fallback_base);
        for (unsigned i = 0; i < assets.clouds().size(); ++i) {
            const auto &dds = assets.clouds()[i];
            write(output / ("cloud" + std::to_string(i) + ".dds"), dds.original_bytes());
            for (unsigned mip = 0; mip < dds.mip_count(); ++mip)
                write(output / ("cloud" + std::to_string(i) + "-mip" + std::to_string(mip) + ".bin"),
                      dds.payload(0, mip));
        }
        std::cout << "PASS C++ fog of war startup, geometry, records and assets\n";
    } catch (const std::exception &e) {
        std::cerr << "FAIL " << e.what() << '\n';
        return 1;
    }
}
