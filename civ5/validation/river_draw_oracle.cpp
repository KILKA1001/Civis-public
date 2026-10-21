#include "civ5/materials/river_surface_draw.hpp"
#include <algorithm>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>
using namespace civ5;
namespace fs = std::filesystem;
namespace {
template <class T> void save(const fs::path &path, const T &data) {
    std::ofstream out(path, std::ios::binary);
    out.write(reinterpret_cast<const char *>(data.data()), data.size() * sizeof(data[0]));
    out.close();
    if (!out)
        throw std::runtime_error("River draw output failed");
}
int run(const fs::path &input, const fs::path &output) {
    if (fs::exists(output))
        throw std::invalid_argument("Fresh river draw output required");
    std::ifstream file(input, std::ios::binary);
    if (!file)
        throw std::invalid_argument("Missing river draw input");
    const std::vector<char> bytes{std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()};
    if (bytes.size() < 80 || std::string_view(bytes.data(), 8) != "C5RDRW01")
        throw std::invalid_argument("Invalid river draw input magic/size");
    std::array<std::uint32_t, 18> words{};
    std::memcpy(words.data(), bytes.data() + 8, 72);
    const auto nv = words[0], ni = words[1];
    if (nv > 1048576 || ni > 1048576 || bytes.size() != 80ull + nv * 32ull + ni * 4ull)
        throw std::invalid_argument("Invalid river draw stream extent");
    terrain::RiverSurfaceMesh mesh;
    mesh.vertices.resize(nv);
    mesh.indices.resize(ni);
    if (nv)
        std::memcpy(mesh.vertices.data(), bytes.data() + 80, nv * 32ull);
    if (ni)
        std::memcpy(mesh.indices.data(), bytes.data() + 80 + nv * 32ull, ni * 4ull);
    materials::RiverSurfaceResources resources{words[3], words[4], {}};
    std::copy_n(words.begin() + 5, 13, resources.pixel_textures.begin());
    const auto upload = materials::river_surface_upload(mesh);
    const auto draw = materials::river_surface_draw(mesh, resources, words[2]);
    const std::array<std::uint32_t, 5> layout{upload.active, upload.vertex_count, upload.index_count,
                                              upload.vertex_bytes, upload.index_bytes};
    std::array<std::uint8_t, 100> packet{};
    std::array<std::uint32_t, 9> command{};
    std::array<float, 16> local{};
    if (draw) {
        packet = draw->packet;
        command = {draw->selector,
                   draw->blend_state,
                   draw->depth_state,
                   draw->primitive,
                   0,
                   draw->vertex_capacity,
                   0,
                   0,
                   draw->triangle_count};
        local = draw->world;
    }
    fs::create_directories(output);
    save(output / "input.bin", bytes);
    save(output / "layout.bin", layout);
    save(output / "packet.bin", packet);
    save(output / "draw.bin", command);
    save(output / "world.bin", local);
    std::cout << "PASS river upload and draw " << nv << " vertices / " << ni << " indices\n";
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
            throw std::invalid_argument("Usage: civ5_river_draw_oracle input.bin fresh-output-directory");
        return run(argv[1], argv[2]);
    } catch (const std::exception &e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
