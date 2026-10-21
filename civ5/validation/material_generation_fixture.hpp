#pragma once
// Input-only offline transport. Full patches receive source fields/RSQRT seeds,
// not expected scores/normals/vertex buffers. Smaller controls deliberately
// supply scores/factors/prior sentinels at their documented function boundary.
#include "civ5/materials/material_generation.hpp"
#include <cstring>
#include <fstream>
#include <stdexcept>
#include <string_view>
namespace generation_validation {
using namespace civ5::materials;
using Bytes = civ5::assets::Bytes;
inline void require(bool ok, const char *why) {
    if (!ok)
        throw std::runtime_error(why);
}
inline void word(Bytes &b, std::uint32_t v) {
    for (unsigned i = 0; i < 4; ++i)
        b.push_back(static_cast<std::uint8_t>(v >> (i * 8)));
}
inline void blob(Bytes &b, std::span<const std::uint8_t> v) {
    word(b, static_cast<std::uint32_t>(v.size()));
    b.insert(b.end(), v.begin(), v.end());
}
template <class T> void vector(Bytes &b, std::span<const T> v) {
    static_assert(sizeof(T) == 4);
    word(b, static_cast<std::uint32_t>(v.size()));
    for (auto value : v)
        word(b, std::bit_cast<std::uint32_t>(value));
}
template <class T> void raw(Bytes &b, std::span<const T> v) {
    static_assert(std::endian::native == std::endian::little);
    if (v.empty())
        return;
    const auto *p = reinterpret_cast<const std::uint8_t *>(v.data());
    b.insert(b.end(), p, p + v.size_bytes());
}
struct Reader {
    std::span<const std::uint8_t> data;
    std::size_t at = 0;
    auto take(std::size_t n) {
        require(n <= data.size() - at, "Truncated generation input");
        auto b = data.subspan(at, n);
        at += n;
        return b;
    }
    std::uint32_t word() {
        auto b = take(4);
        return b[0] | (std::uint32_t(b[1]) << 8) | (std::uint32_t(b[2]) << 16) | (std::uint32_t(b[3]) << 24);
    }
    std::int32_t integer() { return std::bit_cast<std::int32_t>(word()); }
    float floating() { return std::bit_cast<float>(word()); }
    auto blob() { return take(word()); }
    template <class T> std::vector<T> values(std::uint32_t maximum = 65536) {
        const auto n = word();
        require(n <= maximum, "Oversized generation vector");
        std::vector<T> result;
        result.reserve(n);
        for (std::uint32_t i = 0; i < n; ++i)
            result.push_back(std::bit_cast<T>(word()));
        return result;
    }
    template <class T, std::size_t N> std::array<T, N> fixed() {
        auto v = values<T>(N);
        require(v.size() == N, "Wrong fixed generation extent");
        std::array<T, N> result{};
        std::copy(v.begin(), v.end(), result.begin());
        return result;
    }
    void end() { require(at == data.size(), "Trailing generation input"); }
};
inline Bytes read(const std::filesystem::path &path) {
    const auto n = std::filesystem::file_size(path);
    require(n >= 12 && n <= 16 * 1024 * 1024, "Invalid generation input size");
    Bytes bytes(static_cast<std::size_t>(n));
    std::ifstream in(path, std::ios::binary);
    in.read(reinterpret_cast<char *>(bytes.data()), bytes.size());
    require(bool(in), "Generation read failed");
    return bytes;
}
inline void write(const std::filesystem::path &path, std::span<const std::uint8_t> data) {
    require(!std::filesystem::exists(path), "Refusing generation output overwrite");
    std::ofstream out(path, std::ios::binary);
    out.write(reinterpret_cast<const char *>(data.data()), data.size());
    out.close();
    require(bool(out), "Generation write failed");
}
inline MaterialSelection selection(Reader &r) {
    const auto scores = r.values<std::uint32_t>(64);
    const int mode = r.integer(), flag = r.integer();
    const auto slots = r.values<std::int32_t>(64);
    const auto handles = r.values<std::uint32_t>(64);
    return {scores, mode, flag, slots, handles};
}
inline void selection(Bytes &b, const MaterialSelection &s) {
    word(b, static_cast<std::uint32_t>(s.sources().size()));
    word(b, s.specular());
    raw(b, std::span<const std::int32_t>(s.owner_words()));
}
inline civ5::terrain::X87Precision pc(Reader &r) {
    const int v = r.integer();
    require(v == 24 || v == 53, "Unverified generation precision");
    return v == 24 ? civ5::terrain::X87Precision::single : civ5::terrain::X87Precision::double_precision;
}
struct Source {
    int width, height, count;
    std::vector<std::int32_t> heights;
    Bytes field;
    std::array<std::int32_t, 64> slots, modes;
    std::vector<MaterialSample> samples;
    explicit Source(Reader &r) : width(r.integer()), height(r.integer()), count(r.integer()) {
        require(width > 0 && height > 0 && width <= 1024 && height <= 1024 && count >= 0 && count <= 64,
                "Invalid source fixture dimensions");
        heights = r.values<std::int32_t>(1024 * 1024);
        auto b = r.blob();
        field.assign(b.begin(), b.end());
        require((heights.empty() || heights.size() == std::size_t(width * height)) &&
                    field.size() == std::size_t(width * height),
                "Source field extent mismatch");
        slots = r.fixed<std::int32_t, 64>();
        modes = r.fixed<std::int32_t, 64>();
        const auto n = r.word();
        require(n <= 81, "Too many factor samples");
        for (unsigned i = 0; i < n; ++i) {
            const auto x = r.integer(), y = r.integer();
            samples.push_back({x, y, r.floating()});
        }
    }
    int height_at(int x, int y) const {
        require(x >= 0 && x < width && y >= 0 && y < height && heights.size() == std::size_t(width * height),
                "Source height lookup out of range");
        return heights[y * width + x];
    }
    int material_at(int x, int y) const {
        require(x >= 0 && x < width && y >= 0 && y < height, "Source material lookup out of range");
        return field[y * width + x];
    }
    std::vector<float> factors(int x, int y, int axis) const {
        return material_factors(count, samples, width, height, {x, y}, axis, slots, modes,
                                [&](int xx, int yy) { return material_at(xx, yy); });
    }
};
inline MaterialGeometryDomain domain(Reader &r, const Source &source) {
    const int columns = r.integer(), rows = r.integer();
    const MaterialOrigin origin{r.integer(), r.integer()};
    const MaterialVector3 scales{r.floating(), r.floating(), r.floating()};
    const int cache = r.integer();
    const auto precision = pc(r);
    const int remap = r.integer();
    return {columns, rows, origin, {source.width, source.height, scales, remap}, cache, precision};
}
struct NormalTrace {
    const Source &source;
    const MaterialSourceDomain &domain;
    std::vector<float> seeds;
    Bytes trace;
    std::size_t at = 0;
    MaterialVector3 normal(int x, int y) {
        require(at < seeds.size(), "Missing observed RSQRTSS seed");
        const auto input =
            material_normal_input({x, y}, domain, [&](int xx, int yy) { return source.height_at(xx, yy); });
        const float seed = seeds[at++];
        const auto out = normalize_material_normal(input, [&](float) { return seed; });
        raw(trace, std::span<const float>(input));
        raw(trace, std::span<const float>(out));
        word(trace, material_projection_axis(out));
        return out;
    }
    void end() { require(at == seeds.size(), "Unused observed RSQRTSS seeds"); }
};
inline MaterialStreams tile(Reader &r, const Source &source, const MaterialSelection &selected, Bytes &out) {
    const auto d = domain(r, source);
    const int alternate = r.integer();
    const MaterialTile bounds{r.integer(), r.integer(), r.integer(), r.integer()};
    auto a = r.blob(), b = r.blob();
    MaterialStreams prior{Bytes(a.begin(), a.end()), Bytes(b.begin(), b.end())};
    NormalTrace trace{source, d.source, r.values<float>(4356), {}, 0};
    auto streams = write_material_tile(
        d, bounds, prior, selected, alternate, [&](int x, int y) { return source.height_at(x, y); },
        [&](int x, int y) { return trace.normal(x, y); },
        [&](int x, int y, int axis) { return source.factors(x, y, axis); });
    trace.end();
    blob(out, trace.trace);
    blob(out, streams.material48);
    blob(out, streams.normal24);
    return streams;
}
inline MaterialScoreResult scores(Reader &r, const Source &source, MaterialGeometryDomain &d, Bytes &out) {
    d = domain(r, source);
    const int mode = r.integer(), flag = r.integer();
    const auto material_slots = r.values<std::int32_t>(64);
    const auto handles = r.values<std::uint32_t>(64);
    const auto seeds = r.values<float>(69 * 69);
    require(seeds.size() == 69 * 69, "Incomplete broad normal seed field");
    Bytes inputs;
    auto result = produce_material_scores(
        d.columns, d.rows, d.origin, d.source, source.count, source.slots, source.modes, mode, flag,
        material_slots, handles, [&](int x, int y) { return source.height_at(x, y); },
        [&](int x, int y) { return source.material_at(x, y); },
        [&](int x, int y, float) {
            // Recompute only for diagnostic trace; production producer computed its own input.
            const auto v = material_normal_input(
                {civ5::terrain::wrap_add(d.origin[0], x), civ5::terrain::wrap_add(d.origin[1], y)}, d.source,
                [&](int xx, int yy) { return source.height_at(xx, yy); });
            raw(inputs, std::span<const float>(v));
            return seeds[x * 69 + y];
        });
    require(inputs.size() == 69 * 69 * 12, "Unexpected tiny-vector broad input domain");
    blob(out, inputs);
    Bytes normals;
    for (const auto &n : result.normals)
        raw(normals, std::span<const float>(n));
    blob(out, normals);
    raw(out, std::span<const std::uint32_t>(result.scores));
    selection(out, result.selected);
    return result;
}
struct ComputedPatch {
    MaterialPatch patch;
    MaterialSelection selected;
};
inline ComputedPatch full_patch(Reader &r, Bytes &out) {
    const Source source(r);
    MaterialGeometryDomain d{};
    auto produced = scores(r, source, d, out);
    NormalTrace trace{source, d.source, r.values<float>(4356), {}, 0};
    auto patch = build_material_patch(
        d, produced.selected, [&](int x, int y) { return source.height_at(x, y); },
        [&](int x, int y) { return trace.normal(x, y); },
        [&](int x, int y, int axis) { return source.factors(x, y, axis); });
    trace.end();
    blob(out, trace.trace);
    const auto tiles = material_tiles(d.columns, d.rows);
    word(out, static_cast<std::uint32_t>(tiles.size()));
    for (auto t : tiles) {
        word(out, t.x0);
        word(out, t.x1);
        word(out, t.y0);
        word(out, t.y1);
        word(out, t.vertex_count());
    }
    blob(out, patch.streams.material48);
    blob(out, patch.streams.normal24);
    blob(out, {reinterpret_cast<const std::uint8_t *>(patch.indices.data()), patch.indices.size() * 2});
    return {std::move(patch), std::move(produced.selected)};
}
inline Bytes run(unsigned op, Reader r) {
    Bytes out;
    if (op == 0) {
        raw(out, std::span<const float>(material_normal_kernel(0)));
        raw(out, std::span<const float>(material_normal_kernel(1)));
    } else if (op == 1) {
        selection(out, selection(r));
    } else if (op == 2) {
        const auto selected = selection(r);
        const auto precision = pc(r);
        const int alternate = r.integer(), axis = r.integer();
        const auto prior = r.blob();
        const auto kind = r.word();
        require(kind <= 1, "Unknown factor input kind");
        std::vector<float> factors;
        if (kind == 0)
            factors = r.values<float>(64);
        else {
            const Source source(r);
            const MaterialOrigin origin{r.integer(), r.integer()};
            factors = source.factors(origin[0], origin[1], axis);
        }
        const auto lanes = write_material_lanes(prior, selected, factors, precision, alternate, axis);
        vector(out, std::span<const float>(factors));
        raw(out, std::span<const std::uint8_t>(lanes));
    } else if (op == 3) {
        const Source source(r);
        const auto selected = selection(r);
        tile(r, source, selected, out);
    } else if (op == 4) {
        const Source source(r);
        MaterialGeometryDomain d{};
        auto produced = scores(r, source, d, out);
        const auto connected = r.word();
        require(connected <= 1, "Invalid score connection flag");
        word(out, connected);
        if (connected)
            tile(r, source, produced.selected, out);
    } else if (op == 5) {
        full_patch(r, out);
    } else
        throw std::runtime_error("Unknown generation opcode");
    r.end();
    return out;
}
struct Record {
    unsigned op;
    std::span<const std::uint8_t> label, input;
};
inline std::vector<Record> records(std::span<const std::uint8_t> data) {
    Reader r{data};
    auto magic = r.take(8);
    require(std::string_view(reinterpret_cast<const char *>(magic.data()), 8) == "C5MATG01",
            "Wrong generation schema");
    const auto count = r.word();
    require(count > 0 && count <= 1000, "Invalid generation record count");
    std::vector<Record> records;
    for (unsigned i = 0; i < count; ++i) {
        const auto op = r.word();
        const auto label = r.blob(), input = r.blob();
        require(op <= 5 && !label.empty() && label.size() <= 128, "Invalid generation record identity");
        records.push_back({op, label, input});
    }
    r.end();
    return records;
}
inline std::vector<ComputedPatch> gpu_patches(std::span<const std::uint8_t> data) {
    std::vector<ComputedPatch> result;
    for (const auto &record : records(data))
        if (record.op == 5) {
            Bytes output;
            Reader input{record.input};
            result.push_back(full_patch(input, output));
            input.end();
        }
    require(result.size() == 3, "Expected three generated material patches");
    return result;
}
} // namespace generation_validation
