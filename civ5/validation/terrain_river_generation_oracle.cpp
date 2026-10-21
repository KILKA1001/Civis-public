// Original assets -> owned source/coast world -> lattice river graph -> final
// height/material owner. Input contains no river controls, triangles or fields.
#include "terrain_river_output.hpp"
#include <numeric>
#include <type_traits>
namespace {
Bytes load_input(const fs::path &path) {
    const auto size = fs::file_size(path);
    check(size >= 12 && size <= 16 * 1024 * 1024, "Invalid generated world river input size");
    Bytes bytes(static_cast<std::size_t>(size));
    std::ifstream in(path, std::ios::binary);
    in.read(reinterpret_cast<char *>(bytes.data()), bytes.size());
    check(bool(in), "Generated world river input read failed");
    return bytes;
}
struct RiverRequest {
    int map_width, map_height, mode, width, height;
    std::uint32_t seed, code;
    float level;
    X87Precision precision;
    std::array<std::uint8_t, 4> from, to;
    std::vector<std::uint32_t> codes;
    std::vector<RiverEdge> edges;
    std::vector<std::array<std::uint32_t, 2>> estimates;
};
RiverRequest river_request(Reader &r) {
    RiverRequest q;
    q.map_width = r.integer();
    q.map_height = r.integer();
    q.mode = r.integer();
    q.seed = r.word();
    q.width = r.integer();
    q.height = r.integer();
    q.precision = precision(r);
    q.level = r.scalar();
    q.code = r.word();
    const auto remap = r.take(8);
    std::copy_n(remap.begin(), 4, q.from.begin());
    std::copy_n(remap.begin() + 4, 4, q.to.begin());
    q.codes = palette(r);
    check(q.codes.size() <= 256, "Excessive generated river code table");
    const auto count = r.word();
    check(count <= 1048576 && std::size_t(count) * 16 <= r.bytes.size() - r.at,
          "Invalid generated river edge count");
    for (unsigned i = 0; i < count; ++i) {
        const auto x = r.integer(), y = r.integer(), z = r.integer(), d = r.integer();
        q.edges.push_back({x, y, z, d});
    }
    for (unsigned phase = 0; phase < 3; ++phase) {
        const auto n = r.word();
        check(n <= 1048576 && std::size_t(n) * 8 <= r.bytes.size() - r.at,
              "Invalid generated river RSQRT count");
        for (unsigned i = 0; i < n; ++i) {
            const auto squared = r.word(), estimate = r.word();
            q.estimates.push_back({squared, estimate});
        }
    }
    r.end();
    return q;
}
} // namespace
#ifdef _WIN32
int wmain(int argc, wchar_t **argv) {
#else
int main(int argc, char **argv) {
#endif
    try {
        static_assert(std::is_nothrow_move_constructible_v<TerrainWorldRiverGeometry>);
        check(argc == 6, "Usage: terrain_river_generation_oracle asset-root "
                         "coast-archive world-input river-input fresh-output");
        const fs::path root = argv[1], output = argv[5];
        check(!fs::exists(output), "Generated river output must be fresh");
        auto world_bytes = load_input(argv[3]), river_bytes = load_input(argv[4]);
        Reader wr{world_bytes}, rr{river_bytes};
        const auto wm = wr.take(8), rm = rr.take(8);
        check(std::string(wm.begin(), wm.end()) == "C5TWLD01" &&
                  std::string(rm.begin(), rm.end()) == "C5RGWL01",
              "Wrong generated world/river magic");
        const auto count = wr.word();
        check(count >= 1 && count <= 16 && rr.word() == count, "Invalid generated river world count");
        const civ5::assets::AssetKey piece_key(key(wr)), coast_key(key(wr));
        std::vector<WorldRequest> worlds_input;
        std::vector<RiverRequest> rivers_input;
        for (unsigned i = 0; i < count; ++i) {
            Reader w{wr.blob()}, r{rr.blob()};
            worlds_input.push_back(request(w));
            rivers_input.push_back(river_request(r));
        }
        wr.end();
        rr.end();
        check(fs::create_directories(output), "Cannot create generated river world output");
        std::vector<TerrainWorld> worlds;
        {
            const civ5::assets::LooseSource loose(root);
            const auto pieces = TerrainPieceLibrary::load(loose, piece_key);
            const auto source = coast_source(root, civ5::assets::path_utf8(fs::path(argv[2])));
            const auto coasts = CoastLibrary::load(*source, coast_key, X87Precision::double_precision);
            Writer assets;
            write_asset(assets, *pieces.asset(), pieces.serialized_bytes());
            save(output / "source-pieces.bin", assets.bytes);
            write_sources(coasts, output / "source-coasts.bin");
            for (unsigned i = 0; i < count; ++i) {
                auto &q = worlds_input[i];
                auto &v = q.input;
                auto &r = rivers_input[i];
                v.cooked_to_primary = q.mapping;
                v.coast_palette = q.keys;
                v.expanded_logical_cells = q.cells;
                check(v.map_width == r.map_width && v.map_height == r.map_height && v.mode == r.mode,
                      "World/river map identity differs");
                const LandStyleGrid prior(q.prior);
                std::size_t reciprocal_calls = 0;
                auto world = TerrainWorld::build(
                    pieces, coasts, v,
                    {prior, q.masks, nullptr, nullptr, [&](int n) { return Bytes(n, std::uint8_t(q.fill)); }},
                    [&](float determinant) {
                        check(reciprocal_calls < 2 && std::bit_cast<std::uint32_t>(determinant) ==
                                                          q.reciprocal[reciprocal_calls * 2],
                              "World reciprocal input differs");
                        return std::bit_cast<float>(q.reciprocal[reciprocal_calls++ * 2 + 1]);
                    },
                    [&](unsigned, std::span<const CoastDispatchJob> phase) {
                        std::vector<std::size_t> order(phase.size());
                        std::iota(order.begin(), order.end(), 0);
                        if (q.order)
                            std::reverse(order.begin(), order.end());
                        return order;
                    });
                check(reciprocal_calls == 2 && world.land().domain().width() == r.width &&
                          world.land().domain().height() == r.height,
                      "Generated world/river field domain differs");
                const auto path = output / std::to_string(i);
                check(fs::create_directory(path), "Cannot create generated river case");
                write_field(world, path, "pre-");
                RiverRasterSettings settings{r.level, r.precision, r.from, r.to, r.codes, r.code};
                const auto original = world.owner().height.value_at(0, 0);
                bool rejected = false;
                if (!r.estimates.empty()) {
                    try {
                        auto unused = std::move(world).with_rivers(
                            r.edges, r.seed,
                            [](float) -> float { throw std::invalid_argument("Injected RSQRT failure"); },
                            settings);
                    } catch (const std::invalid_argument &) {
                        rejected = true;
                    }
                    check(rejected && !world.rivers() && world.owner().height.value_at(0, 0) == original,
                          "Failed generated river transition consumed world");
                }
                Writer stores, coverage, curves;
                std::size_t estimate_at = 0;
                const auto height_headers = world.owner().height.header_bytes(),
                           material_headers = world.owner().material.header_bytes();
                TerrainRiverObservers observers;
                observers.controls = [&](RiverControlStage stage, const RiverPolylines &p,
                                         const TerrainRandom &random) {
                    Writer out;
                    lines(out, p);
                    out.word(random.state());
                    out.word(static_cast<std::uint32_t>(random.calls()));
                    save(path / ("stage-" + std::to_string(static_cast<unsigned>(stage)) + ".bin"),
                         out.bytes);
                };
                observers.curves = [&](bool derivative, std::span<const RiverPoint> controls, float t,
                                       const RiverPoint &point) {
                    curves.word(derivative);
                    curves.word(static_cast<std::uint32_t>(controls.size()));
                    curves.scalar(t);
                    for (auto f : point)
                        curves.scalar(f);
                };
                observers.write = [&](RiverPass pass, RiverWriteChannel channel, int index,
                                      std::uint32_t value) {
                    stores.bytes.push_back(std::uint8_t(pass));
                    stores.bytes.push_back(std::uint8_t(channel));
                    stores.word(index);
                    stores.word(value);
                };
                observers.sample = [&](RiverPass pass, std::size_t ti, int x, int y,
                                       const std::array<float, 3> &w) {
                    coverage.bytes.push_back(std::uint8_t(pass));
                    coverage.word(static_cast<std::uint32_t>(ti));
                    coverage.word(x);
                    coverage.word(y);
                    for (auto f : w)
                        coverage.scalar(f);
                };
                observers.pass_complete = [&](RiverPass pass, const RiverRasterField &field) {
                    if (pass == RiverPass::banks) {
                        Writer h;
                        h.u16(field.heights);
                        save(path / "banksHeight.bin", h.bytes);
                        save(path / "banksMaterial.bin", field.materials);
                    }
                };
                auto completed = std::move(world).with_rivers(
                    r.edges, r.seed,
                    [&](float squared) {
                        check(estimate_at < r.estimates.size() &&
                                  std::bit_cast<std::uint32_t>(squared) == r.estimates[estimate_at][0],
                              "Generated world river RSQRT input differs");
                        return std::bit_cast<float>(r.estimates[estimate_at++][1]);
                    },
                    settings, observers);
                check(estimate_at == r.estimates.size(), "Generated world river RSQRT count differs");
                check(completed.owner().height.header_bytes() == height_headers &&
                          completed.owner().material.header_bytes() == material_headers,
                      "Generated river transition changed page allocation");
                rejected = false;
                try {
                    auto unused = std::move(completed).with_rivers({}, r.seed, {}, settings);
                } catch (const std::invalid_argument &) {
                    rejected = true;
                }
                check(rejected && completed.rivers() && completed.rivers()->generated,
                      "Duplicate generated river transition consumed world");
                save(path / "stores.bin", stores.bytes);
                save(path / "coverage.bin", coverage.bytes);
                save(path / "curves.bin", curves.bytes);
                worlds.push_back(std::move(completed));
                std::cout << "PASS graph-to-owner world " << i << '\n' << std::flush;
            }
        }
        worlds_input.clear();
        rivers_input.clear();
        world_bytes.clear();
        river_bytes.clear();
        for (unsigned i = 0; i < count; ++i) {
            auto moved = std::move(worlds[i]);
            const auto path = output / std::to_string(i);
            write_field(moved, path, "");
            write_retained(moved, path);
        }
        std::cout << "PASS " << count
                  << " generated river worlds after input/library destruction and "
                     "moves\n";
        return 0;
    } catch (const std::exception &e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
