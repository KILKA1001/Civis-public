#include "civ5/terrain/terrain_source_rivers.hpp"
#include <bit>
#include <fstream>
#include <iostream>
#include <stdexcept>

using namespace civ5::terrain;
namespace {
std::size_t checks = 0;
void check(bool ok, const char *message) {
    ++checks;
    if (!ok) throw std::runtime_error(message);
}
template <class F> void rejects(F action) {
    bool failed = false;
    try { action(); } catch (const std::exception &) { failed = true; }
    check(failed, "Expected invalid river source to be rejected");
}
std::uint32_t word(std::istream &in) {
    unsigned char b[4];
    if (!in.read(reinterpret_cast<char *>(b), 4)) throw std::runtime_error("Truncated river oracle");
    return std::uint32_t(b[0]) | (std::uint32_t(b[1]) << 8) |
           (std::uint32_t(b[2]) << 16) | (std::uint32_t(b[3]) << 24);
}
std::int32_t integer(std::istream &in) { return std::bit_cast<std::int32_t>(word(in)); }
void guards() {
    const auto grid = build_world_grid_layout(3, 5, 0, {}).source_grid;
    std::vector<TerrainRiverPlot> plots(15);
    plots[0] = {true, true, true, true, 0, 1, 5};
    const auto edges = build_terrain_river_edges(grid, plots);
    check(edges == std::vector<RiverEdge>{{1, 0, -1, 1}, {0, 0, 0, 8}, {0, 4, -4, 6}},
          "Boundary wrapping or river side mapping changed");
    rejects([&] { build_terrain_river_edges(grid, plots, 2); });
    plots[0].is_river = false;
    check(build_terrain_river_edges(grid, plots, 0).empty(), "Native IsRiver gate was bypassed");
    plots.pop_back();
    rejects([&] { build_terrain_river_edges(grid, plots); });
    auto bad = grid;
    bad.width = 0;
    rejects([&] { build_terrain_river_edges(bad, plots); });
    check(game_world_grid_mode(false) == 0 && game_world_grid_mode(true) == 1,
          "Retail game caller world mode changed");
}
std::uint32_t oracle(const char *path) {
    std::ifstream in(path, std::ios::binary);
    char magic[8];
    check(bool(in.read(magic, 8)) && std::string_view(magic, 8) == "C5SRIV01", "Invalid river oracle");
    const auto count = word(in);
    check(count > 0 && count < 100000, "Invalid river oracle case count");
    std::size_t edge_count = 0;
    for (std::uint32_t n = 0; n < count; ++n) {
        const auto width = integer(in), height = integer(in);
        const auto size = word(in);
        check(width > 0 && width <= 512 && height > 0 && height <= 512 &&
                  size <= std::uint32_t(width*height*3), "Invalid native river dimensions/count");
        std::vector<TerrainRiverPlot> plots(std::size_t(width)*height);
        for (auto &plot : plots) {
            const auto gate = integer(in), w = integer(in), nw = integer(in), ne = integer(in);
            plot = {gate != 0, w != 0, nw != 0, ne != 0, integer(in), integer(in), integer(in)};
            (void)integer(in); // Feature ID: auxiliary append transport is independently checked by oracle.
        }
        std::vector<RiverEdge> expected(size);
        for (auto &edge : expected) edge = {integer(in), integer(in), integer(in), integer(in)};
        const auto grid = build_world_grid_layout(width, height, 0, {}).source_grid;
        const auto actual = build_terrain_river_edges(grid, plots);
        if (actual != expected) {
            for (std::size_t i = 0; i < std::min(actual.size(), expected.size()); ++i)
                if (actual[i] != expected[i]) {
                    const auto a = actual[i], e = expected[i];
                    std::cerr << "Edge " << i << ": actual " << a.x << ',' << a.y << ',' << a.z << ',' << a.direction
                              << " expected " << e.x << ',' << e.y << ',' << e.z << ',' << e.direction << '\n';
                    break;
                }
            throw std::runtime_error("Native river source mismatch at case " + std::to_string(n));
        }
        ++checks;
        edge_count += size;
    }
    check(in.peek() == std::char_traits<char>::eof(), "Trailing river oracle bytes");
    std::cout << "Compared " << edge_count << " original river edges\n";
    return count;
}
} // namespace
int main(int argc, char **argv) {
    try {
        if (argc > 2) throw std::runtime_error("Usage: civ5_terrain_source_rivers_tests [oracle.bin]");
        guards();
        const auto cases = argc == 2 ? oracle(argv[1]) : 0;
        std::cout << "PASS source rivers: " << checks << " checks, " << cases << " native cases\n";
        return 0;
    } catch (const std::exception &e) {
        std::cerr << "FAIL source rivers: " << e.what() << '\n';
        return 1;
    }
}
