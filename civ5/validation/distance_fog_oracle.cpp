#include "civ5/materials/distance_fog.hpp"
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
        throw std::runtime_error("Cannot write DistanceFog output");
}
template <typename T> void append(Bytes &bytes, const T &value) {
    const auto raw = std::bit_cast<std::array<std::uint8_t, sizeof(T)>>(value);
    bytes.insert(bytes.end(), raw.begin(), raw.end());
}
} // namespace
int wmain(int argc, wchar_t **argv) {
    try {
        if (argc != 3)
            throw std::invalid_argument("Expected asset directory and fresh output directory");
        const fs::path root(argv[1]), output(argv[2]);
        if (fs::exists(output))
            throw std::invalid_argument("Fresh DistanceFog output required");
        const LooseSource source(root);
        const auto assets = DistanceFogAssets::load([&](const char *path) {
            const auto key = fs::path(path).filename().string();
            const auto result = source.find(AssetKey(key));
            if (!std::holds_alternative<AssetHandle>(result))
                throw std::invalid_argument("Missing or ambiguous DistanceFog asset");
            return std::get<AssetHandle>(result);
        });
        auto state = assets.state();
        Bytes startup;
        append(startup, state.density);
        append(startup, state.strength);
        append(startup, state.color);
        fs::create_directories(output);
        write(output / "startup.bin", startup);
        const auto &dds = assets.ramp();
        write(output / "ramp.dds", dds.original_bytes());
        for (unsigned mip = 0; mip < dds.mip_count(); ++mip)
            write(output / ("ramp-mip" + std::to_string(mip) + ".bin"), dds.payload(0, mip));
        const DistanceFogFrame prior{std::bit_cast<civ5::terrain::Vector4>(std::array<std::uint32_t, 4>{
                                         0x7fc00013, 0x80000000, 0x3dcccccd, 0x4bffffff}),
                                     std::bit_cast<civ5::terrain::Vector4>(std::array<std::uint32_t, 4>{
                                         0x7fc00114, 0xbf800000, 0x80000000, 0x01234567})};
        Bytes frames;
        for (bool loaded : {false, true})
            for (bool enabled : {false, true})
                for (float zoom : {0.f, .125f, .5f, .875f, 1.f, -.125f, 1.125f}) {
                    state.loaded = loaded;
                    state.enabled = enabled;
                    const auto frame = distance_fog_frame(state, zoom, prior);
                    append(frames, frame.color);
                    append(frames, frame.parameters);
                }
        write(output / "frames.bin", frames);
        std::cout << "PASS C++ DistanceFog startup, authored mip chain and 28 "
                     "frame cases\n";
    } catch (const std::exception &e) {
        std::cerr << "FAIL " << e.what() << '\n';
        return 1;
    }
}
