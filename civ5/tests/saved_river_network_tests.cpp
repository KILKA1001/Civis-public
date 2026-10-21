#include "civ5/terrain/river_geometry.hpp"
#include <algorithm>
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
std::uint32_t word(std::istream &in) {
    unsigned char b[4];
    if (!in.read(reinterpret_cast<char *>(b), 4)) throw std::runtime_error("Truncated river network oracle");
    return std::uint32_t(b[0]) | (std::uint32_t(b[1]) << 8) |
           (std::uint32_t(b[2]) << 16) | (std::uint32_t(b[3]) << 24);
}
std::int32_t integer(std::istream &in) { return std::bit_cast<std::int32_t>(word(in)); }
float scalar(std::istream &in) { return std::bit_cast<float>(word(in)); }
std::vector<RiverEdge> edges(std::istream &in, std::size_t count) {
    check(count <= 1048576, "Invalid edge count");
    std::vector<RiverEdge> result(count);
    for (auto &e : result) e = {integer(in), integer(in), integer(in), integer(in)};
    return result;
}
struct Stage {
    RiverPolylines lines;
    std::uint32_t state, calls;
};
Stage stage(std::istream &in) {
    const auto count = word(in), state = word(in), calls = word(in);
    check(count <= 65536, "Invalid native line count");
    Stage result{{}, state, calls};
    for (std::uint32_t i = 0; i < count; ++i) {
        const auto points = word(in), terminal = word(in);
        check(points <= 1048576 && terminal < 2, "Invalid native river line");
        RiverPolyline line;
        line.terminal = terminal != 0;
        line.points.resize(points);
        for (auto &p : line.points) p = {scalar(in), scalar(in), scalar(in)};
        result.lines.push_back(std::move(line));
    }
    return result;
}
void compare(const RiverPolylines &actual, const Stage &expected, const TerrainRandom &random) {
    check(actual.size() == expected.lines.size(), "Original river line count mismatch");
    check(random.calls() == expected.calls && random.state() == expected.state, "Original river random cursor mismatch");
    for (std::size_t i = 0; i < actual.size(); ++i) {
        const auto &a = actual[i], &e = expected.lines[i];
        check(a.terminal == e.terminal && a.points.size() == e.points.size(), "Original river line framing mismatch");
        for (std::size_t p = 0; p < a.points.size(); ++p)
            check(std::bit_cast<std::array<std::uint32_t, 3>>(a.points[p]) ==
                      std::bit_cast<std::array<std::uint32_t, 3>>(e.points[p]), "Original river control point mismatch");
    }
}
void guards() {
    const auto boundary = RiverNetwork::build(200, 200, std::array<RiverEdge, 1>{{{100, 199, -299, 8}}});
    check(boundary.chains().size() == 1 && boundary.chains()[0].edges.size() == 1,
          "Last native direction-owner slot was rejected");
    check(boundary.direction_grid().size() == 40000 && boundary.direction_grid()[39999] == (1u << 8),
          "Native 200x200 direction-owner boundary changed");
    for (auto dimensions : {std::array<int, 2>{201, 1}, {1, 201}, {0, 200}, {200, 0}}) {
        bool rejected = false;
        try { RiverNetwork::build(dimensions[0], dimensions[1], {}); }
        catch (const std::invalid_argument &) { rejected = true; }
        check(rejected, "Out-of-capacity river map accepted");
    }
}
std::uint32_t oracle(const char *path) {
    std::ifstream in(path, std::ios::binary);
    char magic[8];
    check(bool(in.read(magic, 8)) && std::string_view(magic, 8) == "C5RNWK01", "Invalid network oracle header");
    const auto count = word(in);
    check(count > 0 && count < 100, "Invalid network oracle count");
    for (std::uint32_t c = 0; c < count; ++c) {
        const auto width = integer(in), height = integer(in), mode = integer(in);
        const auto seed = word(in), size = word(in);
        check(mode >= 0 && mode <= 2, "Invalid network oracle mode");
        const auto input = edges(in, size);
        const auto network = RiverNetwork::build(width, height, input);
        for (auto v : network.direction_grid()) check(v == word(in), "Original direction grid mismatch");
        Matrix4 matrix;
        for (auto &f : matrix) f = scalar(in);
        const auto chains = word(in);
        check(chains == network.chains().size(), "Original river topology count mismatch");
        for (const auto &chain : network.chains())
            check(chain.edges == edges(in, word(in)), "Original river traversal/order mismatch");
        const std::array<Stage, 4> expected{stage(in), stage(in), stage(in), stage(in)};
        const auto estimate_count = word(in);
        check(estimate_count <= 1048576, "Excessive reciprocal estimates");
        std::vector<std::array<std::uint32_t, 2>> estimates(estimate_count);
        for (auto &e : estimates) e = {word(in), word(in)};
        TerrainRandom random(seed);
        std::size_t stages = 0;
        const auto centers = build_river_centerlines(network, matrix, random,
            [&](RiverControlStage s, const RiverPolylines &lines, const TerrainRandom &r) {
                check(static_cast<std::size_t>(s) == stages && stages < 3, "River control stage order mismatch");
                compare(lines, expected[stages++], r);
            });
        check(stages == 3, "Missing river control stage");
        std::size_t estimate_at = 0;
        const auto extended = extend_river_centerlines(network, centers, matrix, mode, [&](float square) {
            check(estimate_at < estimates.size() && std::bit_cast<std::uint32_t>(square) == estimates[estimate_at][0],
                  "Original river extension squared length mismatch");
            return std::bit_cast<float>(estimates[estimate_at++][1]);
        });
        check(estimate_at == estimates.size(), "Unused original reciprocal estimate");
        compare(extended, expected[3], random);
        std::cout << "PASS original network " << width << 'x' << height << ": " << size
                  << " edges, " << chains << " chains, " << random.calls() << " declared-state random calls\n";
    }
    check(in.peek() == std::char_traits<char>::eof(), "Trailing river network oracle bytes");
    return count;
}
} // namespace
int main(int argc, char **argv) {
    try {
        if (argc > 2) throw std::runtime_error("Usage: civ5_saved_river_network_tests [oracle.bin]");
        guards();
        const auto cases = argc == 2 ? oracle(argv[1]) : 0;
        std::cout << "PASS saved river network: " << checks << " checks, " << cases << " original cases\n";
        return 0;
    } catch (const std::exception &e) {
        std::cerr << "FAIL saved river network: " << e.what() << '\n';
        return 1;
    }
}
