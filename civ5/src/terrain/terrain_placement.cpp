#include "civ5/terrain/terrain_placement.hpp"
#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>

namespace civ5::terrain {
namespace {
void require(bool ok, const char *message) {
    if (!ok)
        throw std::invalid_argument(message);
}
using IntVector4 = std::array<std::int32_t, 4>;
IntMatrix4 multiply(const IntMatrix4 &a, const IntMatrix4 &b) {
    IntMatrix4 out{};
    for (unsigned row = 0; row < 4; ++row)
        for (unsigned col = 0; col < 4; ++col)
            for (unsigned k = 0; k < 4; ++k)
                out[row * 4 + col] = wrap_add(out[row * 4 + col], wrap_mul(a[row * 4 + k], b[k * 4 + col]));
    return out;
}
IntVector4 transform(const IntVector4 &v, const IntMatrix4 &m) {
    IntVector4 out{};
    for (unsigned col = 0; col < 4; ++col)
        for (unsigned row = 0; row < 4; ++row)
            out[col] = wrap_add(out[col], wrap_mul(v[row], m[row * 4 + col]));
    return out;
}
Vector4 transform(const Vector4 &v, const Matrix4 &m, bool paired = false) {
    Vector4 out;
    for (unsigned col = 0; col < 4; ++col) {
        const float a = v[0] * m[col], b = v[1] * m[4 + col], c = v[2] * m[8 + col], d = v[3] * m[12 + col];
        out[col] = paired ? (a + b) + (c + d) : ((a + b) + c) + d;
    }
    return out;
}
std::int32_t round_coordinate(float value) {
    require(std::isfinite(value), "Non-finite terrain placement coordinate");
    const double rounded = value >= 0.f ? std::floor(static_cast<double>(value + .5f))
                                       : std::ceil(static_cast<double>(value - .5f));
    require(rounded >= std::numeric_limits<std::int32_t>::min() &&
                rounded <= std::numeric_limits<std::int32_t>::max(),
            "Terrain placement coordinate overflow");
    return static_cast<std::int32_t>(rounded);
}
std::int32_t unambiguous(float value, float alternative) {
    const auto rounded = round_coordinate(value);
    require(rounded == round_coordinate(alternative), "Unverified terrain placement float sum grouping");
    return rounded;
}
IntVector4 cube(std::int32_t row, std::int32_t col) {
    const auto x = wrap_add(row, wrap_neg(col >> 1));
    return {x, col, wrap_neg(wrap_add(x, col)), 1};
}
} // namespace
std::array<IntMatrix4, 4> build_terrain_orientations(const FactoryDescriptor &factory) {
    const auto rotation = unshifted_global_map_matrix(factory);
    std::array<IntMatrix4, 4> result{rotation};
    // 4E8C90 literal translation/selector pairs: (1,-3,2)/2, (-4,0,4)/6,
    // (-2,-3,5)/9. All use rotation zero. C80150 contains the static tables.
    constexpr std::array<std::array<std::int32_t, 3>, 3> translations{{{1,-3,2}, {-4,0,4}, {-2,-3,5}}};
    constexpr std::array<IntMatrix4, 3> tables{{
        {0,0,-1,0, 0,-1,0,0, -1,0,0,0, 0,0,0,1},
        {0,0,1,0, 0,1,0,0, 1,0,0,0, 0,0,0,1},
        {-1,0,0,0, 0,-1,0,0, 0,0,-1,0, 0,0,0,1}}};
    for (unsigned i = 0; i < 3; ++i) {
        IntMatrix4 translation{1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1};
        std::copy(translations[i].begin(), translations[i].end(), translation.begin() + 12);
        result[i + 1] = multiply(multiply(translation, rotation), tables[i]);
    }
    return result;
}
TerrainPlacement::TerrainPlacement(const std::array<IntMatrix4, 4> &orientations,
                                   const IntMatrix4 &map_to_logical, const IntMatrix4 &logical_to_placement,
                                   const Matrix4 &occurrence_to_cache, const std::array<Vector4, 4> &corners)
    : orientations_(orientations), map_to_logical_(map_to_logical),
      logical_to_placement_(logical_to_placement), cache_(occurrence_to_cache) {
    require(std::all_of(cache_.begin(), cache_.end(), [](auto f) { return std::isfinite(f); }),
            "Non-finite terrain placement matrix");
    for (auto corner : corners)
        require(std::all_of(corner.begin(), corner.end(), [](auto f) { return std::isfinite(f); }),
                "Non-finite terrain corner");
    const auto origin = transform(Vector4{0, 0, -0.f, 1}, cache_);
    for (unsigned orientation = 0; orientation < 4; ++orientation) {
        Matrix4 matrix;
        for (unsigned i = 0; i < 16; ++i)
            matrix[i] = static_cast<float>(orientations_[orientation][i]);
        for (unsigned size = 0; size < 2; ++size) {
            auto &rect = endpoints_[orientation][size];
            for (unsigned endpoint = 0; endpoint < 2; ++endpoint) {
                const auto corner = corners[size * 2 + endpoint];
                const auto sequential = transform(transform(corner, matrix), cache_),
                           paired = transform(transform(corner, matrix, true), cache_, true);
                for (unsigned axis = 0; axis < 2; ++axis)
                    rect[endpoint * 2 + axis] =
                        unambiguous(sequential[axis] - origin[axis], paired[axis] - origin[axis]);
            }
            const auto extent = size == 0 ? 128 : 512;
            require(std::abs(std::int64_t(rect[2]) - rect[0]) == extent &&
                        std::abs(std::int64_t(rect[3]) - rect[1]) == extent,
                    "Terrain placement endpoints disagree with source size");
        }
    }
}
SpatialRectangle TerrainPlacement::rectangle(const TerrainOccurrence &o, std::int32_t size) const {
    require(o.orientation < orientations_.size(), "Invalid terrain orientation");
    require(size == 128 || size == 512, "Unsupported terrain placement source size");
    const Vector4 position{static_cast<float>(o.position.x), static_cast<float>(o.position.y),
                           static_cast<float>(o.position.z), 1};
    const auto sequential = transform(position, cache_), paired = transform(position, cache_, true);
    const std::array anchor{unambiguous(sequential[0], paired[0]), unambiguous(sequential[1], paired[1])};
    const auto &offset = endpoints_[o.orientation][size == 128 ? 0 : 1];
    std::array<std::int32_t, 4> rect;
    for (unsigned i = 0; i < 4; ++i) {
        const auto value = std::int64_t(offset[i]) + anchor[i % 2];
        require(value >= std::numeric_limits<std::int32_t>::min() &&
                    value <= std::numeric_limits<std::int32_t>::max(),
                "Terrain descriptor endpoint overflow");
        rect[i] = static_cast<std::int32_t>(value);
    }
    return {rect[0], rect[1], rect[2], rect[3]};
}
LogicalCellCoordinate TerrainPlacement::logical_cell(const TerrainOccurrence &o, MarkupCoordinate markup) const {
    return logical_transform(o).cell(markup);
}
TerrainLogicalTransform TerrainPlacement::logical_transform(const TerrainOccurrence &o) const {
    require(o.orientation < orientations_.size(), "Invalid terrain orientation");
    const IntMatrix4 translation{1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, o.position.x, o.position.y, o.position.z, 1};
    return TerrainLogicalTransform(multiply(multiply(orientations_[o.orientation], translation), map_to_logical_));
}
LogicalCellCoordinate TerrainLogicalTransform::cell(MarkupCoordinate markup) const {
    require(markup.x < 10 && markup.y < 10, "Terrain markup is outside 10x10 storage");
    const auto v = transform(cube(markup.x, markup.y), matrix_);
    return {wrap_add(v[0], v[1] >> 1), v[1]};
}
std::vector<LogicalHex> TerrainPlacement::candidate_positions(std::int32_t width, std::int32_t height) const {
    require(width > 0 && height > 0 && (std::uint64_t(width) + 4) * (std::uint64_t(height) + 4) <= 1100000,
            "Unsafe terrain candidate lattice dimensions");
    std::vector<LogicalHex> result;
    result.reserve(std::size_t(width + 4) * (height + 4));
    for (int row = -2; row <= width + 1; ++row)
        for (int col = -2; col <= height + 1; ++col) {
            const auto v = transform(cube(row, col), logical_to_placement_);
            result.push_back({v[0], v[1], wrap_neg(wrap_add(v[0], v[1]))});
        }
    return result;
}
std::uint32_t terrain_primary_ordinal(const TerrainOccurrence &o, std::span<const std::uint32_t> mapping,
                                    const TerrainPieceLibrary &library) {
    require(o.cooked_index < mapping.size(), "Terrain occurrence cooked index exceeds mapping");
    const auto ordinal = mapping[o.cooked_index];
    library.record(ordinal);
    return ordinal;
}
std::vector<TerrainDescriptor> build_terrain_descriptors(const TerrainPieceLibrary &library,
                                                       const TerrainPlacement &placement,
                                                       std::span<const TerrainOccurrence> occurrences,
                                                       std::span<const std::uint32_t> mapping) {
    std::vector<TerrainDescriptor> descriptors;
    descriptors.reserve(occurrences.size());
    for (const auto &o : occurrences) {
        const auto ordinal = terrain_primary_ordinal(o, mapping, library);
        const auto &record = library.record(ordinal);
        descriptors.push_back({placement.rectangle(o, record.size()), record.raw_state & 1,
                               record.result_code, library.grids(ordinal)});
    }
    return descriptors;
}
bool terrain_markup_matches(TerrainMarkup required, std::uint32_t mask, std::uint8_t category) {
    // Retail 0x5f571a..0x5f5764: either zero mask bypasses BOTH gates.
    return required.mask == 0 || mask == 0 ||
           ((required.mask & mask) != 0 &&
            (required.category == 0 || category == 0 || required.category == category));
}
TerrainLogicalClaims::TerrainLogicalClaims(std::int32_t width, std::int32_t height,
                                         std::span<const std::uint8_t> cells)
    : width_(width), height_(height) {
    require(width > 0 && height > 0 && std::uint64_t(width) * height <= 1000000 &&
                std::uint64_t(width) * height * 8 == cells.size(),
            "Invalid terrain logical cell dimensions");
    cells_.assign(cells.begin(), cells.end());
}
bool TerrainLogicalClaims::try_claim(const TerrainPieceRecord &record, const TerrainOccurrence &occurrence,
                                   const TerrainPlacement &placement) {
    return try_claim(record, placement.logical_transform(occurrence));
}
bool TerrainLogicalClaims::try_claim(const TerrainPieceRecord &record, const TerrainLogicalTransform &transform) {
    std::array<std::size_t, 100> pending;
    std::size_t count = 0;
    require(record.active_coordinates.size() <= pending.size(), "Too many terrain active markup coordinates");
    for (auto coordinate : record.active_coordinates) {
        const auto cell = transform.cell(coordinate);
        if (cell.row < 0 || cell.row >= width_ || cell.column < 0 || cell.column >= height_)
            continue;
        const auto at = (std::size_t(cell.row) * height_ + cell.column) * 8;
        const auto required = record.markup_at(coordinate);
        const auto mask = std::uint32_t(cells_[at]) | std::uint32_t(cells_[at + 1]) << 8 |
                          std::uint32_t(cells_[at + 2]) << 16 | std::uint32_t(cells_[at + 3]) << 24;
        if (!terrain_markup_matches(required, mask, cells_[at + 4]))
            return false;
        if (required.claim) {
            if (cells_[at + 5])
                return false;
            pending[count++] = at + 5;
        }
    }
    if (count == 0)
        return false;
    for (std::size_t i = 0; i < count; ++i)
        cells_[pending[i]] = 1;
    claim_count_ += count;
    return true;
}
} // namespace civ5::terrain
