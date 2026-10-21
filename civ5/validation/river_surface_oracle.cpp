#include "civ5/terrain/river_surface.hpp"
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>
using namespace civ5::terrain;
namespace fs = std::filesystem;
namespace {
struct Reader {
    std::vector<char> bytes;
    std::size_t at = 0;
    std::uint32_t word() {
        if (at > bytes.size() || bytes.size() - at < 4)
            throw std::invalid_argument("Truncated river surface input");
        std::uint32_t v;
        std::memcpy(&v, bytes.data() + at, 4);
        at += 4;
        return v;
    }
    float scalar() { return std::bit_cast<float>(word()); }
};
template <typename T> void save(const fs::path &path, const std::vector<T> &v) {
    std::ofstream file(path, std::ios::binary);
    file.write(reinterpret_cast<const char *>(v.data()), v.size() * sizeof(T));
    file.close();
    if (!file)
        throw std::runtime_error("River surface output failed");
}
int run(const fs::path &input, const fs::path &output) {
    if (fs::exists(output))
        throw std::invalid_argument("Fresh river surface output required");
    std::ifstream file(input, std::ios::binary);
    if (!file)
        throw std::invalid_argument("Missing river surface input");
    Reader r{{std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()}};
    if (r.bytes.size() < 8 || std::string_view(r.bytes.data(), 8) != "C5RSRF01")
        throw std::invalid_argument("Invalid river surface input magic");
    r.at = 8;
    RiverSurfaceSettings settings;
    const auto pc = r.word(), quality = r.word();
    if ((pc != 24 && pc != 53) || quality > 1)
        throw std::invalid_argument("Invalid river surface precision/quality");
    settings.precision = pc == 24 ? X87Precision::single : X87Precision::double_precision;
    settings.high_quality = quality;
    for (auto &f : settings.offset)
        f = r.scalar();
    settings.height_scale = r.scalar();
    const auto count = r.word();
    if (count > 65536)
        throw std::invalid_argument("Excessive river surface family count");
    RiverPolylines centers;
    std::array<RiverPolylines, 2> banks;
    RiverHeights heights;
    for (unsigned family = 0; family < 4; ++family)
        for (std::uint32_t i = 0; i < count; ++i) {
            const auto n = r.word(), flag = r.word();
            if (n > 1048576 || flag > 1 || (family == 3 && flag != 0))
                throw std::invalid_argument("Invalid river surface line extent/flag");
            if (family == 3) {
                std::vector<float> line;
                for (std::uint32_t j = 0; j < n; ++j)
                    line.push_back(r.scalar());
                heights.push_back(std::move(line));
            } else {
                RiverPolyline line;
                line.terminal = flag;
                for (std::uint32_t j = 0; j < n; ++j) {
                    const auto x = r.scalar(), y = r.scalar(), z = r.scalar();
                    line.points.push_back({x, y, z});
                }
                (family == 2 ? centers : banks[family]).push_back(std::move(line));
            }
        }
    if (r.at != r.bytes.size())
        throw std::invalid_argument("Trailing river surface input bytes");
    const auto mesh = build_river_surface(centers, banks, heights, settings);
    fs::create_directories(output);
    save(output / "input.bin", r.bytes);
    save(output / "vertices.bin", mesh.vertices);
    save(output / "indices.bin", mesh.indices);
    std::cout << "PASS river surface " << mesh.vertices.size() << " vertices / " << mesh.indices.size()
              << " indices\n";
    return 0;
}
} // namespace
#ifdef _WIN32
int wmain(int argc, wchar_t **argv) {
#else
int main(int argc, char **argv) {
#endif
    try {
        if (argc != 3)
            throw std::invalid_argument("Usage: civ5_river_surface_oracle input.bin fresh-output-directory");
        return run(argv[1], argv[2]);
    } catch (const std::exception &e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
