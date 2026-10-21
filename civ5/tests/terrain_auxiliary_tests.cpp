#include "civ5/terrain/terrain_auxiliary.hpp"
#include <bit>
#include <fstream>
#include <iostream>
#include <stdexcept>

using namespace civ5::terrain;
namespace {
std::size_t checks = 0, record_bytes = 0;
void check(bool ok, const char *message) {
    ++checks;
    if (!ok) throw std::runtime_error(message);
}
template <class F> void rejects(F f) {
    bool rejected = false;
    try { f(); } catch (const std::exception &) { rejected = true; }
    check(rejected, "Invalid auxiliary input accepted");
}
std::uint32_t word(std::istream &in) {
    std::array<unsigned char, 4> b{};
    if (!in.read(reinterpret_cast<char *>(b.data()), 4)) throw std::runtime_error("Truncated auxiliary oracle");
    return std::uint32_t(b[0]) | std::uint32_t(b[1]) << 8 | std::uint32_t(b[2]) << 16 | std::uint32_t(b[3]) << 24;
}
std::int32_t integer(std::istream &in) { return std::bit_cast<std::int32_t>(word(in)); }
WorldGrid grid(std::istream &in) {
    WorldGrid result;
    for (auto &v : result.first_matrix) v = integer(in);
    for (auto &v : result.second_matrix) v = integer(in);
    for (auto &v : result.descriptor) v = integer(in);
    result.width = integer(in);
    result.height = integer(in);
    return result;
}
void append(std::vector<std::uint8_t> &out, std::uint32_t value, unsigned bytes = 4) {
    for (unsigned i = 0; i < bytes; ++i) out.push_back(static_cast<std::uint8_t>(value >> (i*8)));
}
void cube(std::vector<std::uint8_t> &out, LogicalHex p) {
    append(out, p.x); append(out, p.y); append(out, p.z);
}
void stream(std::istream &in, std::size_t count, const std::vector<std::uint8_t> &actual) {
    check(word(in) == count, "Native auxiliary record count mismatch");
    const auto bytes = word(in);
    check(bytes == actual.size(), "Native auxiliary record byte count mismatch");
    std::vector<std::uint8_t> expected(bytes);
    check(bool(in.read(reinterpret_cast<char *>(expected.data()), bytes)), "Truncated auxiliary records");
    check(expected == actual, "Native auxiliary record mismatch");
    record_bytes += bytes;
}
void guards() {
    auto source = build_world_grid_layout(2, 2, 1, {}).source_grid;
    const std::vector<TerrainAuxiliaryPlot> plots{{5, 0, 128, 2}, {6, 1, 255, 0}, {0, 0, 2, 4}, {0, 1, 4, -1}};
    const auto result = build_terrain_auxiliary_sources(source, plots);
    check(result.relief.size() == 3 && result.relief[0].kind == 0 && result.relief[2].kind == 2 &&
              result.feature2.size() == 1 && result.feature2[0].continent == -128 &&
              result.feature0.size() == 1 && result.feature4.size() == 1,
          "Auxiliary source branch precedence changed");
    rejects([&] { build_terrain_auxiliary_sources(source, plots, 3); });
    rejects([&] { build_terrain_auxiliary_sources(source, std::span(plots).first(3)); });
    source.width = 0;
    rejects([&] { build_terrain_auxiliary_sources(source, plots); });
}
std::uint32_t oracle(const char *path) {
    std::ifstream in(path, std::ios::binary);
    char magic[8];
    check(bool(in.read(magic, 8)) && std::string_view(magic, 8) == "C5AUX001", "Invalid auxiliary oracle");
    const auto cases = word(in);
    check(cases > 0 && cases < 1000, "Invalid auxiliary case count");
    for (std::uint32_t n = 0; n < cases; ++n) {
        const auto source = grid(in);
        check(source.width > 0 && source.width <= 512 && source.height > 0 && source.height <= 512,
              "Invalid oracle dimensions");
        std::vector<TerrainAuxiliaryPlot> inputs(std::size_t(source.width)*source.height);
        for (auto &p : inputs) p = {integer(in), integer(in), integer(in), integer(in)};
        const auto result = build_terrain_auxiliary_sources(source, inputs);
        std::vector<std::uint8_t> relief, feature2, feature0, feature4;
        for (auto p : result.relief) {
            append(relief, 0); append(relief, static_cast<std::uint16_t>(p.x), 2);
            append(relief, static_cast<std::uint16_t>(p.y), 2); append(relief, p.kind, 1);
        }
        for (auto p : result.feature2) {
            append(feature2, 0); cube(feature2, p.position);
            append(feature2, p.terrain); append(feature2, p.continent);
        }
        for (auto p : result.feature0) cube(feature0, p);
        for (auto p : result.feature4) {
            append(feature4, 0); append(feature4, 2, 1); append(feature4, UINT32_MAX); cube(feature4, p);
        }
        stream(in, result.relief.size(), relief);
        stream(in, result.feature2.size(), feature2);
        stream(in, result.feature0.size(), feature0);
        stream(in, result.feature4.size(), feature4);
    }
    check(in.peek() == std::char_traits<char>::eof(), "Trailing auxiliary oracle bytes");
    return cases;
}
} // namespace
int main(int argc, char **argv) {
    try {
        if (argc > 2) throw std::runtime_error("Usage: civ5_terrain_auxiliary_tests [oracle.bin]");
        guards();
        const auto cases = argc == 2 ? oracle(argv[1]) : 0;
        std::cout << "PASS auxiliary: " << checks << " checks, " << cases << " native cases, "
                  << record_bytes << " defined record bytes\n";
    } catch (const std::exception &e) {
        std::cerr << "FAIL auxiliary: " << e.what() << '\n';
        return 1;
    }
}
