// Actual source libraries -> generated world -> explicitly supplied river
// packets. No height/material field or other world result is loaded as input.
#include "terrain_world_input.hpp"
#include <numeric>
namespace {
Bytes load_input(const fs::path &path) {
    const auto size = fs::file_size(path);
    check(size >= 12 && size <= 16 * 1024 * 1024, "Invalid world river input size");
    Bytes bytes(static_cast<std::size_t>(size));
    std::ifstream in(path, std::ios::binary);
    in.read(reinterpret_cast<char *>(bytes.data()), bytes.size());
    check(bool(in), "World river input read failed");
    return bytes;
}
struct RiverRequest {
    int width, height;
    float level;
    X87Precision precision;
    std::uint32_t code;
    std::array<std::uint8_t, 4> from, to;
    std::vector<std::uint32_t> codes;
    std::vector<RiverTriangle> banks, channels;
};
RiverVertex river_vertex(Reader &r) {
    const auto x = r.scalar(), y = r.scalar(), h = r.scalar(), p = r.scalar(), o = r.scalar();
    return {x, y, h, p, o};
}
RiverRequest river_request(Reader &r) {
    RiverRequest q;
    q.width = r.integer(); q.height = r.integer(); q.precision = precision(r);
    q.level = r.scalar(); q.code = r.word();
    const auto remap = r.take(8);
    std::copy_n(remap.begin(), 4, q.from.begin()); std::copy_n(remap.begin() + 4, 4, q.to.begin());
    q.codes = palette(r); check(q.codes.size() <= 256, "Excessive river code table");
    for (auto *out : {&q.banks, &q.channels}) {
        const auto count = r.word();
        check(count <= 65536 && std::size_t(count) * 60 <= r.bytes.size() - r.at, "Invalid river packet count");
        for (unsigned i = 0; i < count; ++i) {
            const auto a = river_vertex(r), b = river_vertex(r), c = river_vertex(r);
            out->push_back({a, b, c});
        }
    }
    r.end();
    return q;
}
void write_field(const TerrainWorld &world, const fs::path &path, const std::string &prefix) {
    Writer h; Bytes m;
    const auto &d = world.land().domain();
    for (int y = 0; y < d.height(); ++y)
        for (int x = 0; x < d.width(); ++x) {
            const auto value = world.owner().height.value_at(x, y);
            h.u16(std::span(&value, 1)); m.push_back(std::uint8_t(world.owner().material.value_at(x, y)));
        }
    save(path / (prefix + "height.bin"), h.bytes); save(path / (prefix + "material.bin"), m);
}
void write_retained(const TerrainWorld &world, const fs::path &path) {
    check(bool(world.rivers()), "World lost river phase state");
    const auto &r = *world.rivers(); Writer out;
    out.word(world.land().domain().width()); out.word(world.land().domain().height());
    out.word(r.precision == X87Precision::single ? 24 : 53); out.scalar(r.level); out.word(r.river_code);
    out.raw(r.remap_from); out.raw(r.remap_to);
    out.word(static_cast<std::uint32_t>(r.material_codes.size()));
    for (auto v : r.material_codes) out.word(v);
    for (const auto *triangles : {&r.banks, &r.channels}) {
        out.word(static_cast<std::uint32_t>(triangles->size()));
        for (const auto &t : *triangles)
            for (const auto &v : {t.a, t.b, t.c})
                for (auto f : {v.x, v.y, v.height, v.profile, v.opacity}) out.scalar(f);
    }
    save(path / "retained-rivers.bin", out.bytes);
    save(path / "visited.bin", r.bank_visited);
    save(path / "owner-height-headers.bin", world.owner().height.header_bytes());
    save(path / "owner-material-headers.bin", world.owner().material.header_bytes());
    save(path / "owner-height-pool.bin", world.owner().height.pool_bytes());
    save(path / "owner-material-pool.bin", world.owner().material.pool_bytes());
    const auto hf = world.height_field(); Writer h;
    for (int y = 0; y < hf.domain().height(); ++y)
        for (int x = 0; x < hf.domain().width(); ++x) {
            const auto value = hf.u16_at(x, y); h.u16(std::span(&value, 1));
        }
    save(path / "lifetime.u16", h.bytes);
}
} // namespace
#ifdef _WIN32
int wmain(int argc, wchar_t **argv) {
#else
int main(int argc, char **argv) {
#endif
    try {
        check(argc == 6, "Usage: terrain_river_world_oracle asset-root coast-archive world-input river-input fresh-output");
        const fs::path root = argv[1], output = argv[5];
        check(!fs::exists(output), "World river output must be fresh");
        auto world_bytes = load_input(argv[3]), river_bytes = load_input(argv[4]);
        Reader wr{world_bytes}, rr{river_bytes};
        const auto wm = wr.take(8), rm = rr.take(8);
        check(std::string(wm.begin(), wm.end()) == "C5TWLD01" &&
                  std::string(rm.begin(), rm.end()) == "C5RWRL01", "Wrong world/river input magic");
        const auto count = wr.word();
        check(count >= 1 && count <= 16 && rr.word() == count, "Invalid world/river case count");
        const civ5::assets::AssetKey piece_key(key(wr)), coast_key(key(wr));
        std::vector<WorldRequest> worlds_input; std::vector<RiverRequest> rivers_input;
        for (unsigned i = 0; i < count; ++i) {
            Reader w{wr.blob()}, r{rr.blob()};
            worlds_input.push_back(request(w)); rivers_input.push_back(river_request(r));
        }
        wr.end(); rr.end(); check(fs::create_directories(output), "Cannot create world river output");
        std::vector<TerrainWorld> worlds;
        {
            const civ5::assets::LooseSource loose(root);
            const auto pieces = TerrainPieceLibrary::load(loose, piece_key);
            const auto source = coast_source(root, civ5::assets::path_utf8(fs::path(argv[2])));
            const auto coasts = CoastLibrary::load(*source, coast_key, X87Precision::double_precision);
            Writer assets; write_asset(assets, *pieces.asset(), pieces.serialized_bytes());
            save(output / "source-pieces.bin", assets.bytes); write_sources(coasts, output / "source-coasts.bin");
            for (unsigned i = 0; i < count; ++i) {
                auto &q = worlds_input[i]; auto &v = q.input; auto &r = rivers_input[i];
                v.cooked_to_primary = q.mapping; v.coast_palette = q.keys; v.expanded_logical_cells = q.cells;
                const LandStyleGrid prior(q.prior); std::size_t reciprocal_calls = 0;
                auto world = TerrainWorld::build(pieces, coasts, v,
                    {prior, q.masks, nullptr, nullptr, [&](int n) { return Bytes(n, std::uint8_t(q.fill)); }},
                    [&](float determinant) {
                        check(reciprocal_calls < 2 && std::bit_cast<std::uint32_t>(determinant) == q.reciprocal[reciprocal_calls * 2],
                              "World reciprocal input differs");
                        return std::bit_cast<float>(q.reciprocal[reciprocal_calls++ * 2 + 1]);
                    }, [&](unsigned, std::span<const CoastDispatchJob> phase) {
                        std::vector<std::size_t> order(phase.size()); std::iota(order.begin(), order.end(), 0);
                        if (q.order) std::reverse(order.begin(), order.end()); return order;
                    });
                check(reciprocal_calls == 2 && world.land().domain().width() == r.width &&
                          world.land().domain().height() == r.height, "Computed world/river domain differs");
                const auto path = output / std::to_string(i); check(fs::create_directory(path), "Cannot create world river case");
                write_field(world, path, "pre-");
                const auto height_headers = world.owner().height.header_bytes(), material_headers = world.owner().material.header_bytes();
                const auto original_sample = world.owner().height.value_at(0, 0);
                RiverRasterSettings settings{r.level, r.precision, r.from, r.to, r.codes, r.code};
                // A failed immutable transition must leave the input world usable.
                auto invalid = settings; invalid.precision = static_cast<X87Precision>(99);
                bool rejected = false;
                try { auto unused = std::move(world).with_river_triangles(r.banks, r.channels, invalid); }
                catch (const std::invalid_argument &) { rejected = true; }
                check(rejected && !world.rivers() && world.owner().height.value_at(0, 0) == original_sample,
                      "Failed river transition consumed or changed world");
                Writer stores, coverage;
                auto completed = std::move(world).with_river_triangles(r.banks, r.channels, settings,
                    [&](RiverPass pass, RiverWriteChannel channel, int index, std::uint32_t value) {
                        stores.bytes.push_back(std::uint8_t(pass)); stores.bytes.push_back(std::uint8_t(channel));
                        stores.word(index); stores.word(value);
                    }, [&](RiverPass pass, std::size_t ti, int x, int y, const std::array<float, 3> &w) {
                        coverage.bytes.push_back(std::uint8_t(pass)); coverage.word(static_cast<std::uint32_t>(ti));
                        coverage.word(x); coverage.word(y); for (auto f : w) coverage.scalar(f);
                    }, [&](RiverPass pass, const RiverRasterField &field) {
                        if (pass == RiverPass::banks) {
                            Writer h; h.u16(field.heights); save(path / "banksHeight.bin", h.bytes);
                            save(path / "banksMaterial.bin", field.materials);
                        }
                    });
                check(completed.owner().height.header_bytes() == height_headers &&
                          completed.owner().material.header_bytes() == material_headers, "River transition changed page allocation");
                rejected = false;
                try { auto unused = std::move(completed).with_river_triangles(r.banks, r.channels, settings); }
                catch (const std::invalid_argument &) { rejected = true; }
                check(rejected && completed.rivers(), "Duplicate river transition was accepted or consumed world");
                save(path / "stores.bin", stores.bytes); save(path / "coverage.bin", coverage.bytes);
                worlds.push_back(std::move(completed));
                std::cout << "PASS connected river world " << i << '\n' << std::flush;
            }
        }
        worlds_input.clear(); rivers_input.clear(); world_bytes.clear(); river_bytes.clear();
        for (unsigned i = 0; i < count; ++i) {
            auto moved = std::move(worlds[i]); const auto path = output / std::to_string(i);
            write_field(moved, path, ""); write_retained(moved, path);
        }
        std::cout << "PASS " << count << " worlds after river input/library destruction and moves\n";
        return 0;
    } catch (const std::exception &e) { std::cerr << e.what() << '\n'; return 1; }
}
