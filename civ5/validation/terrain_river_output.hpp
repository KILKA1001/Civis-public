#pragma once
#include "terrain_world_input.hpp"
namespace {
void lines(Writer &out, const RiverPolylines &lines) {
    out.word(static_cast<std::uint32_t>(lines.size()));
    for (const auto &line : lines) {
        out.word(static_cast<std::uint32_t>(line.points.size()));
        out.word(line.terminal);
        for (const auto &p : line.points)
            for (auto f : p)
                out.scalar(f);
    }
}
void stream(Writer &out, const std::vector<RiverTriangle> &triangles) {
    for (const auto &t : triangles)
        for (const auto &v : {t.a, t.b, t.c})
            for (auto f : {v.x, v.y, v.height, v.profile, v.opacity})
                out.scalar(f);
}
void write_field(const TerrainWorld &world, const fs::path &path, const std::string &prefix) {
    Writer h;
    Bytes m;
    const auto &d = world.land().domain();
    for (int y = 0; y < d.height(); ++y)
        for (int x = 0; x < d.width(); ++x) {
            const auto value = world.owner().height.value_at(x, y);
            h.u16(std::span(&value, 1));
            m.push_back(std::uint8_t(world.owner().material.value_at(x, y)));
        }
    save(path / (prefix + "height.bin"), h.bytes);
    save(path / (prefix + "material.bin"), m);
}
void write_retained(const TerrainWorld &world, const fs::path &path) {
    check(world.rivers() && world.rivers()->generated, "World lost generated river state");
    const auto &r = *world.rivers();
    const auto &g = *r.generated;
    const auto &s = world.source();
    Writer primitive;
    primitive.word(g.network.width());
    primitive.word(g.network.height());
    primitive.word(s.map_mode);
    primitive.word(g.random.entering);
    primitive.word(static_cast<std::uint32_t>(g.input_edges.size()));
    for (auto e : g.input_edges) {
        primitive.word(e.x);
        primitive.word(e.y);
        primitive.word(e.z);
        primitive.word(e.direction);
    }
    save(path / "retained-edges.bin", primitive.bytes);
    Writer settings;
    settings.word(world.land().domain().width());
    settings.word(world.land().domain().height());
    settings.word(r.precision == X87Precision::single ? 24 : 53);
    settings.scalar(r.level);
    settings.word(r.river_code);
    settings.raw(r.remap_from);
    settings.raw(r.remap_to);
    settings.word(static_cast<std::uint32_t>(r.material_codes.size()));
    for (auto v : r.material_codes)
        settings.word(v);
    save(path / "retained-settings.bin", settings.bytes);
    Writer transforms, grid, topology, center, left, right, samples, state, banks, channels;
    for (auto f : s.layout.transforms)
        transforms.scalar(f);
    for (auto bits : g.network.direction_grid())
        grid.word(bits);
    topology.word(static_cast<std::uint32_t>(g.network.chains().size()));
    for (const auto &chain : g.network.chains()) {
        topology.word(static_cast<std::uint32_t>(chain.edges.size()));
        topology.word(chain.terminal);
        for (auto e : chain.edges) {
            topology.word(e.x);
            topology.word(e.y);
            topology.word(e.z);
            topology.word(e.direction);
        }
    }
    lines(center, g.centerlines);
    lines(left, g.bank_lines[0]);
    lines(right, g.bank_lines[1]);
    samples.word(static_cast<std::uint32_t>(g.sampled_heights.size()));
    for (const auto &line : g.sampled_heights) {
        samples.word(static_cast<std::uint32_t>(line.size()));
        for (auto f : line)
            samples.scalar(f);
    }
    state.word(g.random.final);
    state.word(static_cast<std::uint32_t>(g.random.calls));
    stream(banks, r.banks);
    stream(channels, r.channels);
    for (const auto &[name, out] :
         std::initializer_list<std::pair<const char *, const Writer *>>{{"transforms.bin", &transforms},
                                                                        {"direction-grid.bin", &grid},
                                                                        {"topology.bin", &topology},
                                                                        {"extended.bin", &center},
                                                                        {"left.bin", &left},
                                                                        {"right.bin", &right},
                                                                        {"samples.bin", &samples},
                                                                        {"widths-state.bin", &state},
                                                                        {"banks.bin", &banks},
                                                                        {"channels.bin", &channels}})
        save(path / name, out->bytes);
    save(path / "visited.bin", r.bank_visited);
    save(path / "owner-height-headers.bin", world.owner().height.header_bytes());
    save(path / "owner-material-headers.bin", world.owner().material.header_bytes());
    save(path / "owner-height-pool.bin", world.owner().height.pool_bytes());
    save(path / "owner-material-pool.bin", world.owner().material.pool_bytes());
    const auto hf = world.height_field();
    Writer retained;
    for (int y = 0; y < hf.domain().height(); ++y)
        for (int x = 0; x < hf.domain().width(); ++x) {
            const auto value = hf.u16_at(x, y);
            retained.u16(std::span(&value, 1));
        }
    save(path / "lifetime.u16", retained.bytes);
}
} // namespace
