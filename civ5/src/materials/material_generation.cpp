#include "civ5/materials/material_generation.hpp"
#include "civ5/terrain/land_grid.hpp"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <numeric>
#include <stdexcept>

namespace civ5::materials {
namespace {
using terrain::wrap_add;
void require(bool ok, const char *why) {
    if (!ok)
        throw std::invalid_argument(why);
}
void dimensions(int columns, int rows) {
    require(columns >= 2 && columns <= 67 && rows >= 2 && rows <= 67, "Material dimensions outside 2..67");
}
void precision(terrain::X87Precision pc) {
    require(pc == terrain::X87Precision::single || pc == terrain::X87Precision::double_precision,
            "Unverified material x87 precision");
}
void source_domain(const MaterialSourceDomain &s) {
    require(s.width > 0 && s.height > 0 &&
                std::all_of(s.scales.begin(), s.scales.end(), [](float v) { return std::isfinite(v); }),
            "Invalid material source domain");
}
int unsigned_remainder(std::int32_t value, int extent) {
    return static_cast<int>(std::bit_cast<std::uint32_t>(value) % static_cast<std::uint32_t>(extent));
}
std::array<std::int32_t, 64> identity_slots() {
    std::array<std::int32_t, 64> result{};
    std::iota(result.begin(), result.end(), 0);
    return result;
}
constexpr float inv255 = std::bit_cast<float>(0x3b808081u);
template <class T> void store(std::span<std::uint8_t> bytes, std::size_t offset, T value) {
    static_assert(std::endian::native == std::endian::little);
    std::memcpy(bytes.data() + offset, &value, sizeof(value));
}
} // namespace
MaterialSelection::MaterialSelection(std::span<const std::uint32_t> scores, int mode, int flag)
    : MaterialSelection(scores, mode, flag, identity_slots(), std::array<std::uint32_t, 64>{}) {}
MaterialSelection::MaterialSelection(std::span<const std::uint32_t> scores, int cache_mode, int parent_flag,
                                     std::span<const std::int32_t> material_slots,
                                     std::span<const std::uint32_t> specular_handles) {
    require(scores.size() <= 64, "Too many material scores");
    std::vector<std::int32_t> indices;
    for (std::size_t i = 0; i < scores.size(); ++i)
        if (scores[i])
            indices.push_back(static_cast<std::int32_t>(i));
    if (indices.size() > 8)
        std::stable_sort(indices.begin(), indices.end(),
                         [&](auto a, auto b) { return scores[a] > scores[b]; });
    std::copy(indices.begin(), indices.end(), words_.begin());
    count_ = std::min<std::size_t>(indices.size(), 8);
    if (cache_mode != 0) {
        specular_ = parent_flag != 0;
        for (auto index : sources()) {
            require(static_cast<std::size_t>(index) < material_slots.size(),
                    "Missing selected material slot");
            const auto slot = material_slots[index];
            require(slot >= 0 && static_cast<std::size_t>(slot) < specular_handles.size(),
                    "Missing specular handle");
            if (specular_handles[slot])
                specular_ = 1;
        }
    }
}
std::vector<float> material_factors(int count, std::span<const MaterialSample> samples, int width, int height,
                                    MaterialOrigin origin, int axis, std::span<const std::int32_t> slots,
                                    std::span<const std::int32_t> modes, const MaterialLookup &lookup) {
    require(count >= 0 && count <= 64 && samples.size() <= 81 && width > 0 && height > 0,
            "Invalid material factor domain");
    std::vector<float> result(static_cast<std::size_t>(count));
    for (const auto &sample : samples) {
        const int x = unsigned_remainder(wrap_add(sample.x, origin[0]), width);
        const int y = unsigned_remainder(wrap_add(sample.y, origin[1]), height);
        const auto code = lookup(x, y);
        require(code >= 0 && code < 64 && static_cast<std::size_t>(code) < slots.size() &&
                    static_cast<std::size_t>(code) < modes.size(),
                "Invalid material source code");
        auto slot = slots[code];
        if ((axis == 1 || axis == 2) && modes[code] == 1)
            slot = wrap_add(slot, axis);
        require(slot >= 0 && slot < count, "Material factor slot out of range");
        result[slot] = result[slot] + sample.weight;
    }
    float even = 0, odd = 0;
    int i = 0;
    for (; i + 1 < count; i += 2) {
        even += result[i];
        odd += result[i + 1];
    }
    const float tail = i < count ? result[i] : 0.f;
    const float inverse = 1.f / ((even + odd) + tail);
    for (auto &v : result)
        v *= inverse;
    return result;
}
std::array<std::uint8_t, 48> write_material_lanes(std::span<const std::uint8_t> prior,
                                                  const MaterialSelection &selection,
                                                  std::span<const float> normalized, terrain::X87Precision pc,
                                                  int alternate, int axis) {
    require(prior.size() == 48, "Wrong material record extent");
    precision(pc);
    std::array<std::uint8_t, 48> out{};
    std::copy(prior.begin(), prior.end(), out.begin());
    if (alternate != 0) {
        out[40] = static_cast<std::uint8_t>(axis);
        return out;
    }
    std::array<float, 8> weights{};
    const auto indices = selection.sources();
    for (std::size_t i = 0; i < indices.size(); ++i) {
        require(static_cast<std::size_t>(indices[i]) < normalized.size(), "Missing selected material factor");
        weights[i] = normalized[indices[i]];
    }
    float even = 0, odd = 0;
    std::size_t i = 0;
    for (; i + 1 < indices.size(); i += 2) {
        even += weights[i];
        odd += weights[i + 1];
    }
    const float tail = i < indices.size() ? weights[i] : 0.f;
    const double sum =
        terrain::round_x87(terrain::round_x87(double(even) + double(odd), pc) + double(tail), pc);
    if (sum < .001)
        weights[0] = 1.f; // unordered skips native fallback
    for (std::size_t lane = 0; lane < 8; ++lane) {
        const float packed = weights[lane] * 255.f;
        out[32 + lane] = !std::isfinite(packed) || packed < -2147483648.f || packed >= 2147483648.f
                             ? 0
                             : static_cast<std::uint8_t>(static_cast<std::int32_t>(packed));
        out[40 + lane] = lane < indices.size() ? static_cast<std::uint8_t>(indices[lane]) : 0;
    }
    return out;
}
std::array<float, 25> material_normal_kernel(int axis) {
    require(axis == 0 || axis == 1, "Invalid derivative axis");
    constexpr std::array<float, 5> derivative{.0702f, .3526f, 0.f, -.3526f, -.0702f};
    constexpr std::array<float, 5> gaussian{.0228f, .2293f, .4948f, .2293f, .0228f};
    std::array<float, 25> result{};
    for (int i = 0; i < 25; ++i)
        result[i] = axis == 0 ? derivative[i / 5] * gaussian[i % 5] : gaussian[i / 5] * derivative[i % 5];
    return result;
}
MaterialVector3 material_normal_input(MaterialOrigin position, const MaterialSourceDomain &s,
                                      const MaterialLookup &height_at) {
    source_domain(s);
    const auto x = position[0], y = position[1];
    auto raw = [&](std::int32_t dx, std::int32_t dy) {
        const auto sx = s.normal_x_remap == 1 ? wrap_add(terrain::wrap_mul(s.width, 100), dx) % s.width : dx;
        const auto value = height_at(std::clamp(sx, 0, s.width - 1), std::clamp(dy, 0, s.height - 1));
        require(value >= 0 && value <= 65535, "Invalid U16 normal source");
        return value;
    };
    const float center_height = static_cast<float>(raw(x, y)) * inv255;
    const std::array<std::int32_t, 2> gradients{(raw(wrap_add(x, 1), y) - raw(wrap_add(x, -1), y)) >> 1,
                                                (raw(x, wrap_add(y, 1)) - raw(x, wrap_add(y, -1))) >> 1};
    std::array<float, 2> filtered{};
    for (int axis = 0; axis < 2; ++axis) {
        const auto weights = material_normal_kernel(axis);
        float center = (static_cast<float>(gradients[axis]) * inv255) * -2.f + center_height;
        float numerator = 0, denominator = 0;
        for (int d = -2; d <= 2; ++d) {
            for (int other = -2; other <= 2; ++other) {
                if (d == 0 || (std::abs(d) == 2 && std::abs(other) == 2))
                    continue;
                const int dx = axis == 0 ? d : other, dy = axis == 0 ? other : d;
                const float h = static_cast<float>(raw(wrap_add(x, dx), wrap_add(y, dy))) * inv255;
                const float difference = center - h;
                const float q = ((difference * difference) * std::bit_cast<float>(0xc0555555u)) *
                                std::bit_cast<float>(0x38800000u);
                const float polynomial = ((q * q) * .5f + q) + 1.f;
                const float weight = (polynomial * 1.03f) * weights[(dx + 2) * 5 + dy + 2];
                numerator += weight * h;
                denominator += std::abs(weight);
            }
            center += 1.f;
        }
        filtered[axis] = numerator / denominator;
    }
    const float dx = -(filtered[0] * s.scales[2]), dy = -(filtered[1] * s.scales[2]);
    return {dy * 0.f - s.scales[1] * dx, dx * 0.f - dy * s.scales[0], s.scales[1] * s.scales[0]};
}
MaterialVector3 normalize_material_normal(const MaterialVector3 &input,
                                          const terrain::ReciprocalEstimate &seed_at) {
    require(std::all_of(input.begin(), input.end(), [](float v) { return std::isfinite(v); }),
            "Nonfinite normal input");
    const float length2 = (input[0] * input[0] + input[1] * input[1]) + input[2] * input[2];
    if (length2 < std::bit_cast<float>(0x28800000u))
        return {};
    const float seed = seed_at(length2);
    require(std::isfinite(seed) && seed > 0.f, "Invalid RSQRTSS seed");
    const float product = (seed * length2) * seed;
    const float inverse = (3.f - product) * (seed * .5f);
    return {input[0] * inverse, input[1] * inverse, input[2] * inverse};
}
int material_projection_axis(const MaterialVector3 &normal) {
    if (normal[2] > .9f)
        return 0;
    return std::abs(normal[1]) > std::abs(normal[0]) ? 1 : 2;
}
std::uint16_t material_half(float value) {
    const auto bits = std::bit_cast<std::uint32_t>(value), sign = (bits >> 16) & 0x8000u;
    const auto magnitude = bits & 0x7fffffffu;
    if (magnitude > 0x47ffefffu)
        return static_cast<std::uint16_t>(sign | 0x7fffu);
    std::uint32_t rounded;
    if (magnitude < 0x38800000u) {
        const auto shift = 113u - (magnitude >> 23);
        const auto significand = shift > 31 ? 0u : ((magnitude & 0x7fffffu) | 0x800000u) >> shift;
        rounded = significand + 0xfffu + ((significand >> 13) & 1u);
    } else
        rounded = magnitude - 0x38000000u + 0xfffu + ((magnitude >> 13) & 1u);
    return static_cast<std::uint16_t>((rounded >> 13) | sign);
}
namespace {
void validate_geometry(const MaterialGeometryDomain &d) {
    dimensions(d.columns, d.rows);
    source_domain(d.source);
    precision(d.precision);
    require(d.cache_size > 0, "Invalid material cache size");
}
void write_tile(const MaterialGeometryDomain &d, MaterialTile tile, MaterialStreamViews out,
                const MaterialSelection &selected, int alternate, const MaterialLookup &height_at,
                const MaterialNormalLookup &normal_at, const MaterialFactorLookup &factors_at) {
    const auto pc = d.precision;
    const float bias = static_cast<float>(terrain::round_x87(1.0 / d.cache_size, pc));
    for (int x = tile.x0; x < tile.x1; ++x) {
        const double qx = terrain::round_x87(double(x + 1) / (d.columns - 1), pc);
        const float u = static_cast<float>(qx);
        const float clip_x =
            static_cast<float>(terrain::round_x87(terrain::round_x87(qx + qx, pc) - 1.0, pc));
        for (int y = tile.y0; y < tile.y1; ++y) {
            const auto sx = wrap_add(d.origin[0], x), sy = wrap_add(d.origin[1], y);
            const auto normal = normal_at(sx, sy);
            const int axis = material_projection_axis(normal);
            const auto h = height_at(unsigned_remainder(wrap_add(d.source.width, sx), d.source.width),
                                     unsigned_remainder(wrap_add(d.source.height, sy), d.source.height));
            require(h >= 0 && h <= 65535, "Invalid U16 material height");
            const float v = static_cast<float>(y + 1) / static_cast<float>(d.rows - 1), doubled_v = v * 2.f;
            const std::size_t index = (y + 1) * d.columns + x + 1;
            const std::array<float, 8> prefix{clip_x,
                                              1.f - doubled_v,
                                              .5f,
                                              static_cast<float>(sx),
                                              static_cast<float>(sy),
                                              d.source.scales[0] * static_cast<float>(sx),
                                              d.source.scales[1] * static_cast<float>(sy),
                                              (static_cast<float>(h) * inv255) * d.source.scales[2]};
            for (std::size_t i = 0; i < prefix.size(); ++i)
                store(out.material48, index * 48 + i * 4, prefix[i]);
            const auto factors = alternate ? std::vector<float>{} : factors_at(sx, sy, axis);
            const auto lanes = write_material_lanes(std::span(out.material48).subspan(index * 48, 48),
                                                    selected, factors, pc, alternate, axis);
            std::copy(lanes.begin(), lanes.end(), out.material48.begin() + index * 48);
            store(out.normal24, index * 24, clip_x - bias);
            store(out.normal24, index * 24 + 4, (bias - doubled_v) + 1.f);
            store(out.normal24, index * 24 + 8, .5f);
            const std::array<float, 6> half_values{normal[0], normal[1], normal[2], 1.f, u, v};
            for (std::size_t i = 0; i < half_values.size(); ++i)
                store(out.normal24, index * 24 + 12 + i * 2, material_half(half_values[i]));
        }
    }
}
} // namespace
void write_material_tile_into(const MaterialGeometryDomain &d, MaterialTile t,
                              MaterialStreamViews destination, const MaterialSelection &selected,
                              int alternate, const MaterialLookup &height_at,
                              const MaterialNormalLookup &normal_at, const MaterialFactorLookup &factors_at) {
    validate_geometry(d);
    require(t.x0 >= -1 && t.x0 < t.x1 && t.x1 <= d.columns - 1 && t.y0 >= -1 && t.y0 < t.y1 &&
                t.y1 <= d.rows - 1 && t.x1 - t.x0 <= 12 && t.y1 - t.y0 <= 12,
            "Invalid material callback bounds");
    const auto n = static_cast<std::size_t>(d.columns * d.rows);
    require(destination.material48.size() == n * 48 && destination.normal24.size() == n * 24,
            "Wrong paired stream extent");
    write_tile(d, t, destination, selected, alternate, height_at, normal_at, factors_at);
}
MaterialStreams write_material_tile(const MaterialGeometryDomain &d, MaterialTile t,
                                    const MaterialStreams &prior, const MaterialSelection &selected,
                                    int alternate, const MaterialLookup &height_at,
                                    const MaterialNormalLookup &normal_at,
                                    const MaterialFactorLookup &factors_at) {
    auto out = prior;
    write_material_tile_into(d, t, {out.material48, out.normal24}, selected, alternate, height_at, normal_at,
                             factors_at);
    return out;
}
std::vector<MaterialTile> material_tiles(int columns, int rows) {
    dimensions(columns, rows);
    std::vector<MaterialTile> tiles;
    for (int x = -1; x < columns - 1; x += 12)
        for (int y = -1; y < rows - 1; y += 12)
            tiles.push_back({x, std::min(x + 12, columns - 1), y, std::min(y + 12, rows - 1)});
    return tiles;
}
int MaterialTile::vertex_count() const {
    require(x0 >= -1 && x1 <= 66 && x0 < x1 && y0 >= -1 && y1 <= 66 && y0 < y1 && x1 - x0 <= 12 &&
                y1 - y0 <= 12,
            "Invalid material callback bounds");
    return (x1 - x0) * (y1 - y0);
}
MaterialPatch build_material_patch(const MaterialGeometryDomain &d, const MaterialSelection &selected,
                                   const MaterialLookup &height_at, const MaterialNormalLookup &normal_at,
                                   const MaterialFactorLookup &factors_at) {
    validate_geometry(d);
    const auto count = static_cast<std::size_t>(d.columns * d.rows);
    MaterialStreams streams{assets::Bytes(count * 48), assets::Bytes(count * 24)};
    for (auto tile : material_tiles(d.columns, d.rows))
        write_tile(d, tile, {streams.material48, streams.normal24}, selected, 0, height_at, normal_at,
                   factors_at);
    return {d.columns, d.rows, std::move(streams), terrain::land_grid_indices(d.columns, d.rows, 1)};
}
std::vector<MaterialVector3> material_normal_field(MaterialOrigin origin, const MaterialSourceDomain &source,
                                                   const MaterialLookup &height_at,
                                                   const MaterialNormalSeed &seed) {
    source_domain(source);
    std::vector<MaterialVector3> normals(material_normal_side * material_normal_side);
    for (int x = 0; x < material_normal_side; ++x)
        for (int y = 0; y < material_normal_side; ++y) {
            const auto input =
                material_normal_input({wrap_add(origin[0], x), wrap_add(origin[1], y)}, source, height_at);
            normals[y * material_normal_side + x] =
                normalize_material_normal(input, [&](float len) { return seed(x, y, len); });
        }
    return normals;
}
MaterialScoreWords material_scores(int columns, int rows, MaterialOrigin origin, int width, int height,
                                   std::span<const std::int32_t, 64> slots,
                                   std::span<const std::int32_t, 64> modes,
                                   const MaterialNormalLookup &normal_at, const MaterialLookup &material_at) {
    dimensions(columns, rows);
    require(width > 0 && height > 0, "Invalid material score extent");
    MaterialScoreWords scores{};
    for (int y = -2; y < rows - 1; ++y)
        for (int x = -2; x < columns - 1; ++x) {
            const auto sx = unsigned_remainder(wrap_add(wrap_add(width, origin[0]), x), width);
            const auto sy = unsigned_remainder(wrap_add(wrap_add(height, origin[1]), y), height);
            const auto code = material_at(sx, sy);
            require(code >= 0 && code < 64, "Invalid scored material code");
            auto slot = slots[code];
            if (modes[code] == 1) {
                slot = wrap_add(slot, material_projection_axis(normal_at(x + 2, y + 2)));
                require(slot >= 0 && slot <= 125, "Projected score scratch overflow");
                ++scores[slot];
                ++scores[slot + 1];
                ++scores[slot + 2];
            }
            require(slot >= 0 && slot < 128, "Material score slot out of range");
            ++scores[slot];
        }
    return scores;
}
MaterialScoreResult
produce_material_scores(int columns, int rows, MaterialOrigin origin, const MaterialSourceDomain &source,
                        int source_count, std::span<const std::int32_t, 64> slots,
                        std::span<const std::int32_t, 64> modes, int cache_mode, int parent_flag,
                        std::span<const std::int32_t> material_slots,
                        std::span<const std::uint32_t> specular_handles, const MaterialLookup &height_at,
                        const MaterialLookup &material_at, const MaterialNormalSeed &seed) {
    dimensions(columns, rows);
    require(source_count >= 0 && source_count <= 64, "Invalid selected source count");
    auto normals = material_normal_field(origin, source, height_at, seed);
    const auto scores = material_scores(
        columns, rows, origin, source.width, source.height, slots, modes,
        [&](int x, int y) { return normals[y * material_normal_side + x]; }, material_at);
    MaterialSelection selection(std::span(scores).first(source_count), cache_mode, parent_flag,
                                material_slots, specular_handles);
    return {std::move(normals), scores, std::move(selection)};
}
} // namespace civ5::materials
