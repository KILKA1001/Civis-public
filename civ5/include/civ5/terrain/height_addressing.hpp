#pragma once

#include "native_math.hpp"

#include <array>
#include <cstdint>
#include <optional>
#include <span>
#include <vector>

namespace civ5::terrain {

static_assert(sizeof(void*) == 8, "Civ V runtime must be 64-bit");

// Source-space contracts, NOT a source-height generator, LOD policy or a fix
// for the legacy preview's unknown world/cache ownership and height steps.
struct HeightUv { float u, v; };

class HeightDomain;
class HeightWindow {
public:
    std::int32_t tile_x() const { return tile_x_; }
    std::int32_t tile_y() const { return tile_y_; }
    std::int32_t source_origin_x() const;
    std::int32_t source_origin_y() const;
    HeightUv source_to_uv(float x, float y) const;
private:
    friend class HeightDomain;
    HeightWindow(std::int32_t x, std::int32_t y) : tile_x_(x), tile_y_(y) {}
    std::int32_t tile_x_, tile_y_;
};

class HeightDomain {
public:
    static constexpr std::int32_t texture_size = 1024;
    static constexpr std::int32_t texture_core_stride = 896;
    static constexpr std::int32_t texture_halo = 64;

    HeightDomain(std::int32_t width, std::int32_t height);
    std::int32_t width() const { return width_; }
    std::int32_t height() const { return height_; }
    // Floor + 1, including a terminal texture for an exactly divisible extent.
    std::int32_t texture_columns() const { return width_ / texture_core_stride + 1; }
    std::int32_t texture_rows() const { return height_ / texture_core_stride + 1; }
    HeightWindow texture_window(std::int32_t tile_x, std::int32_t tile_y) const;
    // Descriptor composition wraps X only; texture extraction wraps BOTH axes.
    std::optional<std::int32_t> descriptor_destination_index(std::int32_t x, std::int32_t y) const;
    std::int32_t texture_source_index(std::int32_t x, std::int32_t y) const;
private:
    std::int32_t width_, height_;
};

class HeightField {
public:
    HeightField(HeightDomain domain, std::span<const std::uint16_t> source);
    const HeightDomain& domain() const { return domain_; }
    std::uint16_t u16_at(std::int32_t x, std::int32_t y) const;
    std::uint8_t a8_at(std::int32_t x, std::int32_t y) const;
    float sample_a8_linear(float x, float y) const;
    // Base level only. No mip invention, hole filling, smoothing or rescaling.
    std::vector<std::uint8_t> extract_a8_texture(std::int32_t tile_x, std::int32_t tile_y) const;
private:
    HeightDomain domain_;
    std::vector<std::uint16_t> values_;
};

class LandLayout;
class LandPatch {
public:
    std::int32_t column() const { return column_; }
    std::int32_t row() const { return row_; }
    std::int32_t source_x() const { return source_x_; }
    std::int32_t source_y() const { return source_y_; }
    std::int32_t source_samples_x() const { return samples_x_; }
    std::int32_t source_samples_y() const { return samples_y_; }
    std::int32_t patch_index() const { return column_ * layout_rows_ + row_; }
    std::int32_t texture_table_slot() const { return window_.tile_x() + 16 * window_.tile_y(); }
    std::int32_t texture_offset_x() const { return -window_.source_origin_x(); }
    std::int32_t texture_offset_y() const { return -window_.source_origin_y(); }
    const HeightWindow& height_window() const { return window_; }
    std::array<float, 4> offset_constants() const;
    std::array<float, 3> world_translation() const;
    std::array<float, 6> culling_bounds() const;
    // Original initializer +124..130. Center/extents use SSE float operations;
    // radius uses the requested original x87 precision at every operation.
    std::array<float, 4> culling_sphere(X87Precision precision) const;
    HeightUv displacement_uv(float local_x, float local_y) const;
private:
    friend class LandLayout;
    LandPatch(const LandLayout& layout, std::int32_t column, std::int32_t row);
    // Own the small addressing state: a returned patch never dangles after a
    // temporary layout is destroyed. This changes ownership, not native math.
    std::int32_t column_, row_, source_x_, source_y_, samples_x_, samples_y_, layout_rows_;
    float center_x_, center_y_;
    HeightWindow window_;
};

class LandLayout {
public:
    static constexpr float scale_x = 1.0f, scale_y = 0.866f, scale_z = 0.2f;
    explicit LandLayout(HeightDomain domain);
    const HeightDomain& domain() const { return domain_; }
    std::int32_t columns() const { return (domain_.width() + 62) / 64; }
    std::int32_t rows() const { return (domain_.height() + 62) / 64; }
    float center_x() const { return static_cast<float>(domain_.width() / 2) * scale_x; }
    float center_y() const { return static_cast<float>(domain_.height() / 2) * scale_y; }
    LandPatch patch(std::int32_t x, std::int32_t y) const;
private:
    HeightDomain domain_;
};

} // namespace civ5::terrain
