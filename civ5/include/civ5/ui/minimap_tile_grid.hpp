#pragma once

#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <span>
#include <stdexcept>
#include <utility>
#include <vector>

namespace civ5::ui {
// Raw four-byte cell used by the DX9 Minimap producer. The low two bits of
// byte1 identify land (0), coast (1), or ocean (2) after terrain population.
// Byte2 bit 0 is the initial fog mask. Other bits remain raw until their
// producers are established.
struct MinimapTileRecord {
    std::uint8_t byte0 = 0;
    std::uint8_t byte1 = 3;
    std::uint8_t byte2 = 0;
    std::uint8_t byte3 = 0;
};
static_assert(sizeof(MinimapTileRecord) == 4);

inline constexpr std::array<std::array<std::int32_t, 3>, 6> minimap_cube_neighbors{{
    {0, 1, -1}, {-1, 0, 1}, {1, -1, 0},
    {0, -1, 1}, {1, 0, -1}, {-1, 1, 0}}};

// The layer's two projected coordinates are integer affine transforms of
// cube coordinates (q, r, -q-r). Native 0x004BC490 then adds floor(y/2) to x.
// Coefficients are supplied by the layer; their map-specific values are not
// inferred from the saved terrain grid.
struct MinimapTileProjection {
    std::array<std::int32_t, 3> x_coeff{};
    std::array<std::int32_t, 3> y_coeff{};
    std::int32_t x_bias = 0;
    std::int32_t y_bias = 0;

    std::pair<std::int32_t, std::int32_t>
    cell(std::array<std::int32_t, 3> cube) const {
        const auto affine = [&](const std::array<std::int32_t, 3> &coeff,
                                std::int32_t bias) {
            std::uint32_t sum = static_cast<std::uint32_t>(bias);
            for (std::size_t i = 0; i < cube.size(); ++i)
                sum += static_cast<std::uint32_t>(coeff[i]) *
                       static_cast<std::uint32_t>(cube[i]);
            return std::bit_cast<std::int32_t>(sum);
        };
        const auto y = affine(y_coeff, y_bias);
        const auto raw_x = affine(x_coeff, x_bias);
        const auto x = std::bit_cast<std::int32_t>(
            static_cast<std::uint32_t>(raw_x) +
            static_cast<std::uint32_t>(y >> 1));
        return {x, y};
    }
};

// Native 0x0076A600 allocates 256x256 records and an out-of-bounds sentinel.
// Native 0x0043F270 indexes records by y*256+x, with optional X wrapping.
class MinimapTileGrid {
  public:
    static constexpr std::uint32_t stride = 256;
    explicit MinimapTileGrid(std::uint32_t width, std::uint32_t height, bool wrap_x)
        : width_(width), height_(height), wrap_x_(wrap_x),
          records_(stride * stride) {
        if (width == 0 || width > stride || height == 0 || height > stride)
            throw std::invalid_argument("Minimap tile dimensions exceed native storage");
    }

    MinimapTileRecord &at(std::int32_t x, std::int32_t y) {
        const auto index = cell_index(x, y);
        return index == records_.size() ? sentinel_ : records_[index];
    }
    const MinimapTileRecord &at(std::int32_t x, std::int32_t y) const {
        const auto index = cell_index(x, y);
        return index == records_.size() ? sentinel_ : records_[index];
    }
    std::span<const MinimapTileRecord> records() const { return records_; }
    const MinimapTileRecord &sentinel() const { return sentinel_; }
    std::uint32_t width() const { return width_; }
    std::uint32_t height() const { return height_; }
    bool wrap_x() const { return wrap_x_; }

    // Native 0x00634A60..0x00634B20 writes a fresh record for each map plot.
    // Terrain IDs 5 and 6 are coast and ocean in CIV5Terrains.xml. The
    // initial bit in byte2 selects the fog pass until later visibility events.
    bool populate_terrain_cell(std::int32_t x, std::int32_t y,
                               std::int32_t terrain_id) {
        if (x < 0 || static_cast<std::uint32_t>(x) >= width_ ||
            y < 0 || static_cast<std::uint32_t>(y) >= height_)
            return false;
        auto &record = records_[static_cast<std::size_t>(y) * stride +
                                static_cast<std::uint32_t>(x)];
        record = {0, static_cast<std::uint8_t>(terrain_id == 5 ? 1 :
                                               terrain_id == 6 ? 2 : 0), 1, 0};
        sentinel_.byte2 = 1;
        return true;
    }

    // Native 0x00634B60..0x00634D46 compares each populated cell with its
    // six neighbors. The normal map layer uses the identity cube transform,
    // with storage x = q + floor(r/2), y = r. This is a one-shot startup pass:
    // native code ORs edge bits into freshly populated records.
    void build_initial_terrain_edges() {
        const auto &read_only = std::as_const(*this);
        for (std::int32_t y = 0; y < static_cast<std::int32_t>(height_); ++y) {
            for (std::int32_t x = 0; x < static_cast<std::int32_t>(width_); ++x) {
                auto &current = at(x, y);
                const auto kind = current.byte1 & 3u;
                const auto q = x - (y >> 1);
                for (std::size_t i = 0; i < minimap_cube_neighbors.size(); ++i) {
                    const auto nr = y + minimap_cube_neighbors[i][1];
                    const auto nq = q + minimap_cube_neighbors[i][0];
                    const auto nx = nq + (nr >> 1);
                    if (kind != (read_only.at(nx, nr).byte1 & 3u))
                        current.byte1 |= static_cast<std::uint8_t>(1u << (i + 2));
                }
            }
        }
    }

    // The callback at 0x004BC490 stores payload byte + 1 and reconciles six
    // paired boundary bits. This models only that callback, not the other
    // native callbacks that fill the remaining record fields.
    bool apply_cell_payload(std::int32_t q, std::int32_t r,
                            std::uint8_t payload_byte,
                            const MinimapTileProjection &projection) {
        const auto s = std::bit_cast<std::int32_t>(
            0u - static_cast<std::uint32_t>(q) - static_cast<std::uint32_t>(r));
        const std::array<std::int32_t, 3> cube{q, r, s};
        const auto [x, y] = projection.cell(cube);
        auto &current = at(x, y);
        if ((current.byte1 & 3u) != 0) return false;
        current.byte0 = static_cast<std::uint8_t>(payload_byte + 1u);

        // Exact six triples at retail DX9 0x00C7FB08, in callback order.
        for (std::size_t i = 0; i < minimap_cube_neighbors.size(); ++i) {
            std::array<std::int32_t, 3> adjacent{};
            for (std::size_t axis = 0; axis < adjacent.size(); ++axis)
                adjacent[axis] = std::bit_cast<std::int32_t>(
                    static_cast<std::uint32_t>(cube[axis]) +
                    static_cast<std::uint32_t>(minimap_cube_neighbors[i][axis]));
            const auto [nx, ny] = projection.cell(adjacent);
            auto &other = at(nx, ny);
            const auto current_bit = static_cast<std::uint8_t>(1u << (i + 2));
            const auto other_bit = static_cast<std::uint8_t>(1u << ((i + 3) % 6 + 2));
            if (current.byte0 != other.byte0) {
                current.byte3 |= current_bit;
                other.byte3 |= other_bit;
            } else {
                current.byte3 &= static_cast<std::uint8_t>(~current_bit);
                other.byte3 &= static_cast<std::uint8_t>(~other_bit);
            }
        }
        return true;
    }

    // The single-cell branch of callback 0x004FD0A0 clears fog bit 0 and
    // reconciles its six edges against the same bit in adjacent records.
    // Event eligibility and bulk-clear behavior are handled by its caller.
    void clear_cell_fog(std::int32_t q, std::int32_t r,
                        const MinimapTileProjection &projection) {
        const auto s = std::bit_cast<std::int32_t>(
            0u - static_cast<std::uint32_t>(q) - static_cast<std::uint32_t>(r));
        const std::array<std::int32_t, 3> cube{q, r, s};
        const auto [x, y] = projection.cell(cube);
        auto &current = at(x, y);
        current.byte2 &= 0xfeu;
        for (std::size_t i = 0; i < minimap_cube_neighbors.size(); ++i) {
            std::array<std::int32_t, 3> adjacent{};
            for (std::size_t axis = 0; axis < adjacent.size(); ++axis)
                adjacent[axis] = std::bit_cast<std::int32_t>(
                    static_cast<std::uint32_t>(cube[axis]) +
                    static_cast<std::uint32_t>(minimap_cube_neighbors[i][axis]));
            const auto [nx, ny] = projection.cell(adjacent);
            auto &other = at(nx, ny);
            const auto current_bit = static_cast<std::uint8_t>(1u << (i + 2));
            const auto other_bit = static_cast<std::uint8_t>(1u << ((i + 3) % 6 + 2));
            if (((current.byte2 ^ other.byte2) & 1u) != 0) {
                current.byte2 |= current_bit;
                other.byte2 |= other_bit;
            } else {
                current.byte2 &= static_cast<std::uint8_t>(~current_bit);
                other.byte2 &= static_cast<std::uint8_t>(~other_bit);
            }
        }
    }

  private:
    std::size_t cell_index(std::int32_t x, std::int32_t y) const {
        if (y < 0 || static_cast<std::uint32_t>(y) >= height_)
            return records_.size();
        if (wrap_x_) {
            x %= static_cast<std::int32_t>(width_);
            if (x < 0) x += static_cast<std::int32_t>(width_);
        } else if (x < 0 || static_cast<std::uint32_t>(x) >= width_) {
            return records_.size();
        }
        return static_cast<std::size_t>(y) * stride +
               static_cast<std::uint32_t>(x);
    }
    std::uint32_t width_, height_;
    bool wrap_x_;
    std::vector<MinimapTileRecord> records_;
    MinimapTileRecord sentinel_{};
};
} // namespace civ5::ui
