#include "civ5/terrain/terrain_source_grid.hpp"
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
    bool rejected = false;
    try { action(); } catch (const std::exception &) { rejected = true; }
    check(rejected, "Expected invalid source grid to be rejected");
}
std::uint32_t word(std::istream &in) {
    unsigned char bytes[4];
    if (!in.read(reinterpret_cast<char *>(bytes), 4)) throw std::runtime_error("Truncated grid oracle");
    return std::uint32_t(bytes[0]) | (std::uint32_t(bytes[1]) << 8) |
           (std::uint32_t(bytes[2]) << 16) | (std::uint32_t(bytes[3]) << 24);
}
template <std::size_t N> std::array<std::int32_t, N> ints(std::istream &in) {
    std::array<std::int32_t, N> result;
    for (auto &v : result) v = std::bit_cast<std::int32_t>(word(in));
    return result;
}
WorldGrid grid(std::istream &in) {
    auto first = ints<16>(in), second = ints<16>(in);
    auto descriptor = ints<5>(in);
    auto size = ints<2>(in);
    return {size[0], size[1], descriptor, first, second};
}
void compare(const WorldGrid &actual, const WorldGrid &expected) {
    check(actual.width == expected.width && actual.height == expected.height &&
              actual.descriptor == expected.descriptor && actual.first_matrix == expected.first_matrix &&
              actual.second_matrix == expected.second_matrix, "Native world grid header mismatch");
}
void guards() {
    const auto layout = build_world_grid_layout(3, 2, 0, {});
    std::vector<TerrainSourceCell> source(6, {1, 7});
    const auto result = build_terrain_source_grid(layout, source, 0xa55a);
    check(result.width == 7 && result.height == 6 && result.cells.size() == 336, "Expanded grid dimensions");
    check(result.cells[0] == 0 && result.cells[1] == 0x20 && result.cells[4] == 255 &&
              result.cells[5] == 0 && result.cells[6] == 0x5a && result.cells[7] == 0xa5,
          "Coast padding/category/claim/reserved storage changed");
    check(result.cells[(2 * 6 + 2) * 8] == 1 && result.cells[(2 * 6 + 2) * 8 + 4] == 7,
          "Source corner did not land inside its border");
    rejects([&] { build_terrain_source_grid(layout, source, 0, 41); });
    source.pop_back();
    rejects([&] { build_terrain_source_grid(layout, source, 0); });
    source.push_back({1, 7});
    auto bad = layout;
    bad.worker_grid.first_matrix[12] = 1000;
    rejects([&] { build_terrain_source_grid(bad, source, 0); });
    bad = layout;
    bad.source_grid.height = 0;
    rejects([&] { build_terrain_source_grid(bad, source, 0); });
    rejects([] { build_world_grid_layout(INT32_MAX, 1, 0, {}); });
    rejects([] { build_world_grid_layout(1, 0, 0, {}); });
    const auto wide = build_world_grid_layout(180, 94, 1, {});
    check(wide.worker_grid.width == 180 && wide.worker_grid.height == 98, "Real VP dimensions rejected");
}
std::uint32_t oracle(const char *path) {
    std::ifstream in(path, std::ios::binary);
    char magic[8];
    check(bool(in.read(magic, 8)) && std::string_view(magic, 8) == "C5SGRD01", "Invalid grid oracle header");
    const auto count = word(in);
    check(count > 0 && count < 1000, "Invalid grid oracle count");
    for (std::uint32_t n = 0; n < count; ++n) {
        const auto dimensions = ints<4>(in), reciprocal = ints<4>(in);
        const auto source_grid = grid(in), worker_grid = grid(in);
        const auto transforms = ints<80>(in);
        const auto bounds = ints<9>(in);
        std::size_t calls = 0;
        const auto layout = build_world_layout(dimensions[0], dimensions[1], dimensions[2], {}, [&](float det) {
            check(calls < 2 && std::bit_cast<std::int32_t>(det) == reciprocal[calls * 2], "Native determinant mismatch");
            return std::bit_cast<float>(reciprocal[calls++ * 2 + 1]);
        });
        check(calls == 2, "World inverse callback count");
        compare(layout.source_grid, source_grid);
        compare(layout.worker_grid, worker_grid);
        check(std::bit_cast<std::array<std::int32_t, 80>>(layout.transforms) == transforms &&
                  std::bit_cast<std::array<std::int32_t, 9>>(layout.bounds) == bounds, "Native world float state mismatch");
        const auto integer_layout = build_world_grid_layout(dimensions[0], dimensions[1], dimensions[2], {});
        compare(integer_layout.source_grid, source_grid);
        compare(integer_layout.worker_grid, worker_grid);
        check(dimensions[0] > 0 && dimensions[0] <= 512 && dimensions[1] > 0 && dimensions[1] <= 512 &&
                  dimensions[3] >= 0 && dimensions[3] <= 65535, "Unsupported oracle dimensions/reserved value");
        std::vector<TerrainSourceCell> source(std::size_t(dimensions[0]) * dimensions[1]);
        // Oracle source records are in save order; API consumes x-major order.
        for (int y = 0; y < dimensions[1]; ++y)
            for (int x = 0; x < dimensions[0]; ++x) {
                const auto mask = word(in);
                const auto category = in.get();
                check(category >= 0, "Truncated source classification");
                source[std::size_t(x) * dimensions[1] + y] = {mask, static_cast<std::uint8_t>(category)};
            }
        const auto actual = build_terrain_source_grid(integer_layout, source, static_cast<std::uint16_t>(dimensions[3]));
        std::vector<std::uint8_t> expected(actual.cells.size());
        check(bool(in.read(reinterpret_cast<char *>(expected.data()), expected.size())), "Truncated native grid");
        if (expected != actual.cells) throw std::runtime_error("Native logical grid mismatch at case " + std::to_string(n));
        ++checks;
    }
    check(in.peek() == std::char_traits<char>::eof(), "Trailing grid oracle bytes");
    return count;
}
} // namespace
int main(int argc, char **argv) {
    try {
        if (argc > 2) throw std::runtime_error("Usage: civ5_terrain_source_grid_tests [oracle.bin]");
        guards();
        const auto cases = argc == 2 ? oracle(argv[1]) : 0;
        std::cout << "PASS source grid: " << checks << " checks, " << cases << " native cases\n";
        return 0;
    } catch (const std::exception &e) {
        std::cerr << "FAIL source grid: " << e.what() << '\n';
        return 1;
    }
}
