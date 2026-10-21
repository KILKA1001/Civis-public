#include "civ5/terrain/coast_decal_paths.hpp"
#include <fstream>
#include <iostream>
#include <limits>
#include <stdexcept>

using namespace civ5::terrain;
namespace {
using Bytes = std::vector<std::uint8_t>;
std::size_t checks = 0, point_bytes = 0, path_count = 0;
void check(bool ok, const char *message) {
    ++checks;
    if (!ok) throw std::runtime_error(message);
}
template<class F> void rejects(F f) {
    bool caught = false;
    try { f(); } catch (const std::invalid_argument &) { caught = true; }
    check(caught, "Invalid coast decal input accepted");
}
struct Reader {
    std::ifstream in;
    explicit Reader(const char *path) : in(path, std::ios::binary) {}
    std::uint32_t word(unsigned size = 4) {
        std::uint32_t v = 0;
        for (unsigned i = 0; i < size; ++i) {
            const auto c = in.get();
            check(c != std::char_traits<char>::eof(), "Truncated coast decal oracle");
            v |= std::uint32_t(static_cast<unsigned char>(c)) << (i*8);
        }
        return v;
    }
    float real() { return std::bit_cast<float>(word()); }
    Bytes blob() {
        const auto count = word(); check(count <= 16*1024*1024, "Oversized coast decal input");
        Bytes b(count);
        if (count) check(bool(in.read(reinterpret_cast<char *>(b.data()), count)), "Truncated coast decal data");
        return b;
    }
};
void guards() {
    const std::array<CoastControlPoint, 4> controls{{{0, 0, 0, 0, 0}, {1, 1, 0, 0, 0},
                                                   {2, 2, 0, 0, 0}, {3, 3, 0, 0, 0}}};
    const std::array<CoastDecalSource, 1> sources{{{controls, 0, true}}};
    auto result = build_coast_decal_paths(sources, {0, 0}, X87Precision::double_precision);
    check(result.paths.size() == 1 && result.points.size() == 10 && result.paths[0].closed,
          "Coast decal point count/closure");
    check(build_coast_decal_paths({}, {0, 0}, X87Precision::single).points.empty(), "Empty source produced paths");
    const std::array<CoastDecalSource, 1> excluded{{{{}, 7, false}}};
    check(build_coast_decal_paths(excluded, {0, 0}, X87Precision::single).points.empty(), "Nonzero owner was processed");
    rejects([&] { build_coast_decal_paths(sources, {0, 0}, static_cast<X87Precision>(9)); });
    auto bad_center = Point2{0, 0}; bad_center.x = std::numeric_limits<float>::infinity();
    rejects([&] { build_coast_decal_paths(sources, bad_center, X87Precision::single); });
    const std::array<CoastDecalSource, 1> short_source{{{std::span(controls).first(3), 0, false}}};
    rejects([&] { build_coast_decal_paths(short_source, {0, 0}, X87Precision::single); });
    auto points = controls;
    points.fill({1, 1, 0, 0, 0});
    const std::array<CoastDecalSource, 1> degenerate{{{points, 0, false}}};
    rejects([&] { build_coast_decal_paths(degenerate, {0, 0}, X87Precision::single); });
    points = controls; points[2].x = std::numeric_limits<float>::quiet_NaN();
    rejects([&] { build_coast_decal_paths(degenerate, {0, 0}, X87Precision::single); });
    points = controls;
    for (auto &p : points) { p.transition = std::numeric_limits<float>::quiet_NaN(); p.style_a = 255; }
    const auto metadata = build_coast_decal_paths(degenerate, {0, 0}, X87Precision::double_precision);
    check(metadata.points == result.points, "Unused style/transition metadata affected XY path");
}
void oracle(const char *path) {
    Reader r(path);
    check(bool(r.in), "Cannot open coast decal oracle");
    std::string magic;
    for (unsigned i = 0; i < 8; ++i) magic += static_cast<char>(r.word(1));
    check(magic == "C5CDP001", "Wrong coast decal oracle");
    const auto cases = r.word(); check(cases <= 1000, "Excessive cases");
    for (unsigned c = 0; c < cases; ++c) {
        try {
            const auto pc = r.word(); check(pc == 24 || pc == 53, "Invalid precision");
            const auto x = r.real(), y = r.real();
            const auto count = r.word(); check(count <= max_coast_contours, "Excessive contours");
            CoastDecalPaths actual;
            {
                std::vector<std::vector<CoastControlPoint>> controls(count);
                std::vector<CoastDecalSource> sources;
                for (unsigned i = 0; i < count; ++i) {
                    const auto flag = r.word(), closed = r.word(), size = r.word();
                    check(closed <= 1 && size%16 == 0 && size/16 <= max_coast_contour_controls, "Invalid original contour");
                    for (unsigned j = 0; j < size/16; ++j) {
                        const auto px = r.real(), py = r.real(), transition = r.real();
                        const auto a = r.word(1), b = r.word(1); r.word(2);
                        controls[i].push_back({px, py, transition, std::uint8_t(a), std::uint8_t(b)});
                    }
                    sources.push_back({controls[i], flag, closed != 0});
                }
                actual = build_coast_decal_paths(sources, {x, y},
                    pc == 24 ? X87Precision::single : X87Precision::double_precision);
            }
            check(r.word() == actual.points.size()*8, "Original path point count");
            for (auto point : actual.points) {
                check(std::bit_cast<std::uint32_t>(point.x) == r.word(), "Original path X bits");
                check(std::bit_cast<std::uint32_t>(point.y) == r.word(), "Original path Y bits");
            }
            check(r.word() == actual.paths.size(), "Original path count");
            for (auto p : actual.paths) {
                check(p.first == r.word() && p.count == r.word() && p.source == r.word() && p.closed == bool(r.word()),
                      "Original path range/order/closure");
            }
            point_bytes += actual.points.size()*8; path_count += actual.paths.size();
        } catch (const std::exception &e) {
            throw std::runtime_error("Case "+std::to_string(c)+": "+e.what());
        }
    }
    check(r.in.peek() == std::char_traits<char>::eof(), "Trailing coast decal data");
    std::cout << cases << " original cases, ";
}
} // namespace
int main(int argc, char **argv) {
    try {
        check(argc <= 2, "Usage: civ5_coast_decal_paths_tests [oracle.bin]");
        guards(); if (argc == 2) oracle(argv[1]);
        std::cout << "PASS coast decal paths: " << checks << " checks, " << point_bytes << " point bytes, "
                  << path_count << " paths\n";
    } catch (const std::exception &e) {
        std::cerr << "FAIL coast decal paths: " << e.what() << '\n'; return 1;
    }
}
