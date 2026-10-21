#pragma once
// Offline input-only bridge: parsed authored fields and already-perturbed base
// contours -> C++ profiles/Character/five curves -> worker geometry/gates/height.
#include "civ5/terrain/worker_geometry.hpp"
#include "worker_source_input.hpp"
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <stdexcept>
using namespace civ5::terrain;
using Bytes = std::vector<std::uint8_t>;
namespace fs = std::filesystem;
namespace {
void check(bool condition, const char *message) {
    if (!condition)
        throw std::invalid_argument(message);
}
struct Reader {
    std::span<const std::uint8_t> bytes;
    std::size_t at = 0;
    auto take(std::size_t n) {
        check(n <= bytes.size() - at, "Truncated coast geometry input");
        const auto result = bytes.subspan(at, n);
        at += n;
        return result;
    }
    std::uint32_t word() {
        const auto b = take(4);
        return std::uint32_t(b[0]) | std::uint32_t(b[1]) << 8 | std::uint32_t(b[2]) << 16 |
               std::uint32_t(b[3]) << 24;
    }
    std::int32_t integer() { return std::bit_cast<std::int32_t>(word()); }
    float scalar() { return std::bit_cast<float>(word()); }
    auto blob() { return take(word()); }
    SpatialRectangle rectangle() {
        const auto x = integer(), y = integer(), ex = integer(), ey = integer();
        return {x, y, ex, ey};
    }
    void end() const { check(at == bytes.size(), "Trailing coast geometry input"); }
};
struct Writer {
    Bytes bytes;
    void word(std::uint32_t v) {
        for (int i = 0; i < 4; ++i)
            bytes.push_back(static_cast<std::uint8_t>(v >> (8 * i)));
    }
    void scalar(float v) { word(std::bit_cast<std::uint32_t>(v)); }
    void raw(std::span<const std::uint8_t> b) { bytes.insert(bytes.end(), b.begin(), b.end()); }
    void blob(std::span<const std::uint8_t> b) {
        check(b.size() <= 0xffffffffu, "Coast geometry output too large");
        word(static_cast<std::uint32_t>(b.size()));
        raw(b);
    }
    void u16(std::span<const std::uint16_t> values) {
        for (auto v : values) {
            bytes.push_back(static_cast<std::uint8_t>(v));
            bytes.push_back(static_cast<std::uint8_t>(v >> 8));
        }
    }
};
X87Precision precision(Reader &r) {
    const auto pc = r.word();
    check(pc == 24 || pc == 53, "Unsupported coast geometry precision");
    return pc == 24 ? X87Precision::single : X87Precision::double_precision;
}
void save(const fs::path &path, std::span<const std::uint8_t> data) {
    check(!fs::exists(path), "Refusing coast geometry output overwrite");
    std::ofstream file(path, std::ios::binary);
    file.write(reinterpret_cast<const char *>(data.data()), data.size());
    file.close();
    check(bool(file), "Coast geometry output write failed");
}
CoastBeachSource beach(Reader &r) {
    const auto height = r.scalar(), offset = r.scalar();
    const auto character = r.integer(), terrain = r.integer();
    return {height, offset, character, terrain};
}
struct Library {
    CoastProfiles profiles;
    std::array<CoastCharacterTables, 2> characters;
};
Library library(Reader &r) {
    const auto count = r.word();
    check(count <= 4096, "Coast profile source count exceeded");
    std::vector<CoastProfileRecord> records;
    for (std::uint32_t i = 0; i < count; ++i) {
        const auto type = r.blob();
        check(type.size() <= 1024, "Coast style name too long");
        const auto high = r.scalar(), low = r.scalar();
        const auto high_beach = beach(r), low_beach = beach(r);
        const auto water_height = r.scalar(), water_offset = r.scalar();
        const auto character = r.integer();
        records.push_back(compile_coast_profile({std::string(type.begin(), type.end()),
                                                 high,
                                                 low,
                                                 high_beach,
                                                 low_beach,
                                                 {water_height, water_offset, character}}));
    }
    Library result{CoastProfiles(records), {}};
    const auto characters = r.word();
    check(characters <= 4096, "Coast Character source count exceeded");
    std::uint64_t total = 0;
    for (std::uint32_t i = 0; i < characters; ++i) {
        const auto size = r.word();
        total += size;
        check(size >= 98 && size <= 65536 && total <= 1048576, "Coast Character source size exceeded");
        check(std::size_t(size) * 12 <= r.bytes.size() - r.at, "Truncated Character source");
        std::vector<CoastCharacterSourcePoint> source;
        for (std::uint32_t j = 0; j < size; ++j) {
            const auto x = r.scalar(), y = r.scalar(), z = r.scalar();
            source.push_back({x, y, z});
        }
        result.characters[0].push_back(compile_coast_character(source, X87Precision::single));
        result.characters[1].push_back(compile_coast_character(source, X87Precision::double_precision));
    }
    r.end();
    return result;
}
void write_library(const Library &lib, const fs::path &output) {
    Writer styles;
    for (const auto &record : lib.profiles.records()) {
        for (auto n : record.nodes) {
            styles.scalar(n.offset);
            styles.scalar(n.opacity);
            styles.scalar(n.height);
            styles.word(n.character);
        }
        for (auto m : record.materials)
            styles.word(m);
        styles.word(record.key);
    }
    save(output / "styles.bin", styles.bytes);
    for (int pc = 0; pc < 2; ++pc)
        for (std::size_t i = 0; i < lib.characters[pc].size(); ++i) {
            Writer pairs;
            for (auto p : lib.characters[pc][i]) {
                pairs.scalar(p.tangent);
                pairs.scalar(p.normal);
            }
            save(output / ("character-" + std::to_string(pc ? 53 : 24) + "-" + std::to_string(i) + ".bin"),
                 pairs.bytes);
        }
}
CoastCharacterTables character_tables(Reader &r) {
    const auto count = r.word();
    check(count <= 4096, "Coast explicit Character table count exceeded");
    CoastCharacterTables result;
    std::uint64_t total = 0;
    for (std::uint32_t i = 0; i < count; ++i) {
        const auto size = r.word();
        total += size;
        check(size <= 65536 && total <= 1048576 && std::size_t(size) * 8 <= r.bytes.size() - r.at,
              "Coast explicit table capacity");
        std::vector<CoastCharacterPoint> values;
        for (std::uint32_t j = 0; j < size; ++j) {
            const auto x = r.scalar(), y = r.scalar();
            values.push_back({x, y});
        }
        result.push_back(std::move(values));
    }
    return result;
}
CoastCurveSet read_curve_request(Reader &r, bool explicit_characters, const Library &lib) {
    const auto pc = precision(r);
    const auto owner = r.word(), closed = r.word(), family = r.word(), common = r.word();
    check(closed <= 1, "Invalid coast closed flag");
    const auto wx = r.scalar(), wy = r.scalar();
    const auto palette_count = r.word();
    check(palette_count <= 65536, "Coast palette count exceeded");
    std::vector<std::uint32_t> palette;
    for (std::uint32_t i = 0; i < palette_count; ++i)
        palette.push_back(r.word());
    const auto raw = r.blob();
    check(raw.size() % 14 == 0 && raw.size() / 14 >= 4 && raw.size() / 14 <= max_coast_contour_controls,
          "Invalid base coast controls");
    Reader controls{raw};
    std::vector<CoastControlPoint> points;
    while (controls.at < controls.bytes.size()) {
        const auto x = controls.scalar(), y = controls.scalar(), t = controls.scalar();
        const auto styles = controls.take(2);
        points.push_back({x, y, t, styles[0], styles[1]});
    }
    CoastCharacterTables direct;
    const CoastCharacterTables *characters;
    if (explicit_characters) {
        direct = character_tables(r);
        characters = &direct;
    } else {
        const auto loader_pc = precision(r);
        characters = &lib.characters[loader_pc == X87Precision::single ? 0 : 1];
    }
    r.end();
    const CoastCurveState state{closed == 1, Point2(wx, wy), owner, palette, family, common, pc};
    return build_coast_curves(points, lib.profiles, *characters, state);
}
void write_curves(Writer &out, const CoastCurveSet &set) {
    Writer controls;
    for (const auto &curve : set.curves())
        for (auto p : curve) {
            controls.scalar(p.x);
            controls.scalar(p.y);
            controls.scalar(p.transition);
            controls.bytes.push_back(p.style_a);
            controls.bytes.push_back(p.style_b);
        }
    out.blob(controls.bytes);
    const auto count = static_cast<std::uint32_t>(set.control_count());
    for (unsigned band = 0; band < 5; ++band) {
        out.word(band * count);
        out.word((band + 1) * count);
    }
    // Oracle-only native count/capacity metadata; no pointer or padding bytes.
    out.word(count * 5 + 1);
    out.word(count * 5 + 1);
    out.word(set.owner_flag());
}
void write_geometry(Writer &out, const WorkerGateGeometry &geometry) {
    Writer points;
    for (auto p : geometry.points) {
        points.scalar(p.x);
        points.scalar(p.y);
    }
    out.blob(points.bytes);
    for (const auto *list : {&geometry.boundaries, &geometry.fills}) {
        Writer ranges;
        for (auto r : *list) {
            ranges.word(r.first);
            ranges.word(r.end);
            ranges.word(r.gate);
        }
        out.blob(ranges.bytes);
    }
}
using ComputedFieldConsumer = std::function<void(const WorkerField &, bool)>;
Bytes compose(const WorkerGateGeometry &geometry, std::span<const std::uint8_t> source_prefix,
              std::span<const std::uint8_t> worker_source, const fs::path &prefix,
              const ComputedFieldConsumer &consume = {},
              const civ5::validation::WorkerCompositionSource *computed_source = nullptr) {
    Reader r{source_prefix};
    const auto width = r.integer(), height = r.integer(), mw = r.integer(), mh = r.integer();
    check(mw >= 1 && mh >= 1 && mw <= 255 && mh <= 255, "Invalid geometry logical source dimensions");
    const HeightDomain domain(width, height);
    std::vector<std::uint32_t> masks;
    for (int i = 0; i < mw * mh; ++i)
        masks.push_back(r.word());
    IntMatrix4 m1, m2;
    Matrix4 world;
    for (auto &v : m1)
        v = r.integer();
    for (auto &v : m2)
        v = r.integer();
    for (auto &v : world)
        v = r.scalar();
    const auto wrap = r.word(), outside = r.word();
    const auto pc = precision(r);
    const auto path = r.word(), field = r.word();
    r.end();
    check(wrap <= 1 && outside <= 255 && path <= 1 && field == 1, "Invalid geometry routing state");
    const WorkerGateSource grid{mw,        mh,
                                masks,     m1,
                                m2,        world,
                                wrap == 1, static_cast<std::uint8_t>(outside),
                                pc,        path ? Vec4Path::sse : Vec4Path::legacy_x87};
    const auto routing =
        route_worker_gates(domain, grid, geometry.points, geometry.boundaries, geometry.fills);
    const auto gates = routing.build_gate_field64();
    civ5::validation::WorkerCompositionSource legacy;
    if (computed_source) {
        check(worker_source.empty(), "Computed terrain sources cannot be mixed with serialized worker grids");
    } else {
        Reader s{worker_source};
        legacy.width = s.integer();
        legacy.height = s.integer();
        legacy.bias = s.scalar();
        legacy.precision = precision(s);
        legacy.descriptors = civ5::validation::read_worker_sources(s, check);
        s.end();
        computed_source = &legacy;
    }
    const auto &source = *computed_source;
    check(source.width == width && source.height == height, "Geometry/worker domain mismatch");
    const auto heights = compose_worker_field(domain, source.descriptors, gates, source.bias, source.precision);
    Writer out;
    out.blob(routing.logical_categories());
    out.word(static_cast<std::uint32_t>(routing.workers().size()));
    for (const auto &worker : routing.workers()) {
        out.word(worker.origin_x);
        out.word(worker.origin_y);
        for (const auto *list : {&worker.boundaries, &worker.fills}) {
            out.word(static_cast<std::uint32_t>(list->size()));
            for (auto r : *list) {
                out.word(r.first);
                out.word(r.end);
                out.word(r.gate);
            }
        }
        out.word(static_cast<std::uint32_t>(worker.seeds.size()));
        for (auto seed : worker.seeds) {
            out.word(seed.x);
            out.word(seed.y);
            out.word(seed.gate);
        }
    }
    out.blob(gates);
    Writer raw_heights;
    raw_heights.u16(heights.heights());
    out.blob(raw_heights.bytes);
    out.blob(heights.materials());
    auto height_path = prefix, material_path = prefix;
    height_path += ".u16";
    material_path += ".u8";
    save(height_path, raw_heights.bytes);
    save(material_path, heights.materials());
    if (consume)
        consume(heights, wrap == 1);
    return out.bytes;
}
Bytes run(std::uint32_t kind, Reader &r, const Library &lib, std::span<const std::uint8_t> worker,
          std::span<const std::uint8_t> gate_source, const fs::path &prefix) {
    Writer out;
    if (kind <= 1) {
        write_curves(out, read_curve_request(r, kind == 0, lib));
        return out.bytes;
    }
    check(kind == 2, "Unknown coast geometry operation");
    const auto pc = precision(r);
    const auto route = r.word(), count = r.word();
    check(route <= 1 && count >= 1 && count <= max_coast_contours, "Invalid geometry curve-set count/route flag");
    std::vector<CoastCurveSet> curves;
    for (std::uint32_t i = 0; i < count; ++i) {
        Reader item{r.blob()};
        curves.push_back(read_curve_request(item, false, lib));
    }
    r.end();
    out.word(count);
    for (const auto &curve : curves)
        write_curves(out, curve);
    const auto geometry = build_worker_geometry(curves, pc);
    write_geometry(out, geometry);
    if (route)
        out.blob(compose(geometry, gate_source, worker, prefix));
    return out.bytes;
}
} // namespace
