#include "civ5/terrain/river_geometry.hpp"
#include "civ5/terrain/world_layout.hpp"
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
using namespace civ5::terrain;
namespace fs = std::filesystem;
using Bytes = std::vector<std::uint8_t>;
namespace {
void check(bool ok, const char *message) {
    if (!ok)
        throw std::invalid_argument(message);
}
struct Reader {
    std::span<const std::uint8_t> bytes;
    std::size_t at = 0;
    auto take(std::size_t size) {
        check(size <= bytes.size() - at, "Truncated river generation input");
        const auto result = bytes.subspan(at, size);
        at += size;
        return result;
    }
    std::uint32_t word() {
        const auto b = take(4);
        return std::uint32_t(b[0]) | std::uint32_t(b[1]) << 8 | std::uint32_t(b[2]) << 16 | std::uint32_t(b[3]) << 24;
    }
    auto integer() { return std::bit_cast<std::int32_t>(word()); }
    void end() { check(at == bytes.size(), "Trailing river generation input"); }
};
struct Writer {
    Bytes bytes;
    void word(std::uint32_t v) {
        for (int i = 0; i < 4; ++i)
            bytes.push_back(static_cast<std::uint8_t>(v >> (i * 8)));
    }
    void scalar(float v) { word(std::bit_cast<std::uint32_t>(v)); }
    void lines(const RiverPolylines &lines) {
        word(static_cast<std::uint32_t>(lines.size()));
        for (const auto &line : lines) {
            word(static_cast<std::uint32_t>(line.points.size()));
            word(line.terminal);
            for (const auto &p : line.points)
                for (auto f : p)
                    scalar(f);
        }
    }
};
void save(const fs::path &file, const Bytes &bytes) {
    check(!fs::exists(file), "Refusing river generation output overwrite");
    std::ofstream out(file, std::ios::binary);
    out.write(reinterpret_cast<const char *>(bytes.data()), bytes.size());
    out.close();
    check(bool(out), "River generation output failed");
}
void run(Reader &r, const fs::path &folder) {
    const auto width = r.integer(), height = r.integer(), mode = r.integer();
    const auto pc = r.word(), seed = r.word();
    check(pc == 24 || pc == 53, "Invalid river generation precision");
    std::array<std::uint32_t, 4> inverses;
    for (auto &v : inverses)
        v = r.word();
    const auto count = r.word();
    check(count <= 1048576 && std::size_t(count) * 16 <= r.bytes.size() - r.at, "Invalid river edge count");
    std::vector<RiverEdge> edges;
    for (std::uint32_t i = 0; i < count; ++i) {
        const auto x = r.integer(), y = r.integer(), z = r.integer(), direction = r.integer();
        edges.push_back({x, y, z, direction});
    }
    std::array<std::vector<std::array<std::uint32_t, 2>>, 3> estimates;
    for (auto &phase : estimates) {
        const auto estimate_count = r.word();
        check(estimate_count <= 1048576 && std::size_t(estimate_count) * 8 <= r.bytes.size() - r.at,
              "Invalid river estimate count");
        for (std::uint32_t i = 0; i < estimate_count; ++i) {
            const auto squared = r.word(), seed = r.word();
            phase.push_back({squared, seed});
        }
    }
    const auto height_count = r.word();
    check(height_count <= 16777216, "Excessive river input height count");
    const auto raw_heights = r.take(std::size_t(height_count) * 2);
    std::vector<std::uint16_t> heights;
    for (std::size_t i = 0; i < raw_heights.size(); i += 2)
        heights.push_back(std::uint16_t(raw_heights[i]) | std::uint16_t(raw_heights[i + 1]) << 8);
    r.end();
    unsigned inverse_count = 0;
    const auto world = build_world_layout(width, height, mode, {}, [&](float det) {
        check(inverse_count < 2 && std::bit_cast<std::uint32_t>(det) == inverses[inverse_count * 2],
              "River world determinant mismatch");
        return std::bit_cast<float>(inverses[inverse_count++ * 2 + 1]);
    });
    check(inverse_count == 2, "River world inverse count");
    Writer transforms;
    for (const auto v : world.transforms)
        transforms.scalar(v);
    save(folder / "transforms.bin", transforms.bytes);
    const auto network = RiverNetwork::build(width, height, edges);
    Writer grid, topology;
    for (const auto v : network.direction_grid())
        grid.word(v);
    topology.word(static_cast<std::uint32_t>(network.chains().size()));
    for (const auto &chain : network.chains()) {
        topology.word(static_cast<std::uint32_t>(chain.edges.size()));
        topology.word(chain.terminal);
        for (auto e : chain.edges) {
            topology.word(e.x);
            topology.word(e.y);
            topology.word(e.z);
            topology.word(e.direction);
        }
    }
    save(folder / "direction-grid.bin", grid.bytes);
    save(folder / "topology.bin", topology.bytes);
    Matrix4 matrix;
    std::copy_n(world.transforms.begin() + 16, 16, matrix.begin());
    TerrainRandom random(seed);
    const auto centers = build_river_centerlines(
        network, matrix, random, [&](RiverControlStage stage, const RiverPolylines &lines, const TerrainRandom &state) {
            Writer out;
            out.lines(lines);
            out.word(state.state());
            out.word(static_cast<std::uint32_t>(state.calls()));
            save(folder / ("stage-" + std::to_string(static_cast<unsigned>(stage)) + ".bin"), out.bytes);
        });
    std::size_t estimate_at = 0;
    const auto extended = extend_river_centerlines(network, centers, matrix, mode, [&](float squared) {
        check(estimate_at < estimates[0].size(), "Insufficient river extension RSQRTSS estimates");
        check(std::bit_cast<std::uint32_t>(squared) == estimates[0][estimate_at][0],
              "River extension squared length mismatch");
        return std::bit_cast<float>(estimates[0][estimate_at++][1]);
    });
    check(estimate_at == estimates[0].size(), "Unused river extension RSQRTSS estimates");
    Writer extension;
    extension.lines(extended);
    save(folder / "extended.bin", extension.bytes);
    const HeightDomain domain(world.worker_grid.width * 64, world.worker_grid.height * 64);
    const auto samples = sample_river_heights(domain, heights, extended);
    Writer sampled;
    sampled.word(static_cast<std::uint32_t>(samples.size()));
    for (const auto &line : samples) {
        sampled.word(static_cast<std::uint32_t>(line.size()));
        for (auto value : line)
            sampled.scalar(value);
    }
    save(folder / "samples.bin", sampled.bytes);
    estimate_at = 0;
    const auto banks = widen_river_centerlines(extended, random, [&](float squared) {
        check(estimate_at < estimates[1].size(), "Insufficient river width RSQRTSS estimates");
        check(std::bit_cast<std::uint32_t>(squared) == estimates[1][estimate_at][0],
              "River width squared length mismatch");
        return std::bit_cast<float>(estimates[1][estimate_at++][1]);
    });
    check(estimate_at == estimates[1].size(), "Unused river width RSQRTSS estimates");
    for (unsigned i = 0; i < 2; ++i) {
        Writer out;
        out.lines(banks[i]);
        save(folder / (i == 0 ? "left.bin" : "right.bin"), out.bytes);
    }
    Writer state;
    state.word(random.state());
    state.word(static_cast<std::uint32_t>(random.calls()));
    save(folder / "widths-state.bin", state.bytes);
    estimate_at = 0;
    Writer curves;
    const auto triangles = build_river_triangles(
        extended, banks, samples,
        [&](float squared) {
            check(estimate_at < estimates[2].size(), "Insufficient river triangles RSQRTSS estimates");
            if (std::bit_cast<std::uint32_t>(squared) != estimates[2][estimate_at][0])
                throw std::invalid_argument("River triangle squared length mismatch at " + std::to_string(estimate_at) +
                                            ": " + std::to_string(std::bit_cast<std::uint32_t>(squared)) +
                                            " expected " + std::to_string(estimates[2][estimate_at][0]));
            return std::bit_cast<float>(estimates[2][estimate_at++][1]);
        },
        [&](bool derivative, std::span<const RiverPoint> controls, float parameter, const RiverPoint &point) {
            curves.word(derivative);
            curves.word(static_cast<std::uint32_t>(controls.size()));
            curves.scalar(parameter);
            for (auto f : point)
                curves.scalar(f);
        });
    check(estimate_at == estimates[2].size(), "Unused river triangles RSQRTSS estimates");
    save(folder / "curves.bin", curves.bytes);
    for (unsigned i = 0; i < 2; ++i) {
        Writer out;
        for (const auto &t : i == 0 ? triangles.banks : triangles.channels)
            for (const auto &v : {t.a, t.b, t.c})
                for (auto f : {v.x, v.y, v.height, v.profile, v.opacity})
                    out.scalar(f);
        save(folder / (i == 0 ? "banks.bin" : "channels.bin"), out.bytes);
    }
}
} // namespace
#ifdef _WIN32
int wmain(int argc, wchar_t **argv) {
#else
int main(int argc, char **argv) {
#endif
    try {
        check(argc == 3, "Usage: river_generation_oracle inputs.bin fresh-output");
        const fs::path input = argv[1], output = argv[2];
        check(!fs::exists(output), "River generation output must be fresh");
        const auto size = fs::file_size(input);
        check(size >= 48 && size <= 256 * 1024 * 1024, "Unsupported river generation input size");
        Bytes bytes(static_cast<std::size_t>(size));
        std::ifstream in(input, std::ios::binary);
        in.read(reinterpret_cast<char *>(bytes.data()), bytes.size());
        check(bool(in), "River generation input read failed");
        Reader r{bytes};
        const auto magic = r.take(8);
        check(std::string(magic.begin(), magic.end()) == "C5RGEN01", "Wrong river generation magic");
        check(fs::create_directories(output), "Cannot create river generation output");
        run(r, output);
        std::cout << "PASS river generation\n";
        return 0;
    } catch (const std::exception &e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
