#include "civ5/terrain/height_addressing.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>

namespace civ5::terrain {
namespace {
static_assert(sizeof(float) == 4 && std::numeric_limits<float>::is_iec559);
void require(bool condition, const char* message) {
    if (!condition) throw std::invalid_argument(message);
}
std::int32_t floor_mod(std::int32_t value, std::int32_t extent) {
    const auto remainder = value % extent;
    return remainder < 0 ? remainder + extent : remainder;
}
float wrap(float value, std::int32_t extent) {
    const float remainder = std::fmod(value, static_cast<float>(extent));
    return remainder < 0 ? remainder + static_cast<float>(extent) : remainder;
}
} // namespace

HeightDomain::HeightDomain(std::int32_t width, std::int32_t height) : width_(width), height_(height) {
    require(width > 0 && height > 0 && width % 16 == 0 && height % 16 == 0 &&
        static_cast<std::int64_t>(width) * height <= std::numeric_limits<std::int32_t>::max(),
        "Invalid native U16 field dimensions");
}
HeightWindow HeightDomain::texture_window(std::int32_t x, std::int32_t y) const {
    require(x >= 0 && x < texture_columns() && y >= 0 && y < texture_rows(),
        "Height texture outside native layout");
    return HeightWindow(x, y);
}
std::optional<std::int32_t> HeightDomain::descriptor_destination_index(std::int32_t x, std::int32_t y) const {
    if (y < 0 || y >= height_) return std::nullopt;
    return y * width_ + floor_mod(x, width_);
}
std::int32_t HeightDomain::texture_source_index(std::int32_t x, std::int32_t y) const {
    return floor_mod(y, height_) * width_ + floor_mod(x, width_);
}
std::int32_t HeightWindow::source_origin_x() const { return tile_x_ * HeightDomain::texture_core_stride - HeightDomain::texture_halo; }
std::int32_t HeightWindow::source_origin_y() const { return tile_y_ * HeightDomain::texture_core_stride - HeightDomain::texture_halo; }
HeightUv HeightWindow::source_to_uv(float x, float y) const {
    require(std::isfinite(x) && std::isfinite(y), "Non-finite height texture coordinate");
    constexpr float size = static_cast<float>(HeightDomain::texture_size);
    return {(x - static_cast<float>(source_origin_x()) + 0.5f) / size,
            (size - 0.5f - (y - static_cast<float>(source_origin_y()))) / size};
}

HeightField::HeightField(HeightDomain domain, std::span<const std::uint16_t> source) : domain_(domain) {
    require(source.size() == static_cast<std::size_t>(domain.width()) * domain.height(),
        "U16 values do not match native domain");
    values_.assign(source.begin(), source.end());
}
std::uint16_t HeightField::u16_at(std::int32_t x, std::int32_t y) const { return values_[domain_.texture_source_index(x, y)]; }
std::uint8_t HeightField::a8_at(std::int32_t x, std::int32_t y) const { return static_cast<std::uint8_t>(u16_at(x, y) >> 8); }
float HeightField::sample_a8_linear(float source_x, float source_y) const {
    require(std::isfinite(source_x) && std::isfinite(source_y), "Non-finite height source coordinate");
    const float x = wrap(source_x, domain_.width()), y = wrap(source_y, domain_.height());
    const auto x0 = static_cast<std::int32_t>(std::floor(x)), y0 = static_cast<std::int32_t>(std::floor(y));
    const float fx = x - static_cast<float>(x0), fy = y - static_cast<float>(y0);
    const float a = a8_at(x0, y0), b = a8_at(x0 + 1, y0);
    const float c = a8_at(x0, y0 + 1), d = a8_at(x0 + 1, y0 + 1);
    const float upper = a + (b - a) * fx;
    return upper + (c + (d - c) * fx - upper) * fy;
}
std::vector<std::uint8_t> HeightField::extract_a8_texture(std::int32_t tile_x, std::int32_t tile_y) const {
    const auto window = domain_.texture_window(tile_x, tile_y);
    constexpr auto size = HeightDomain::texture_size;
    std::vector<std::uint8_t> texture(size * size);
    for (std::int32_t local_y = 0; local_y < size; ++local_y) {
        const auto target_row = (size - 1 - local_y) * size;
        const auto y = floor_mod(window.source_origin_y() + local_y, domain_.height());
        for (std::int32_t local_x = 0; local_x < size; ++local_x) {
            const auto x = floor_mod(window.source_origin_x() + local_x, domain_.width());
            texture[target_row + local_x] = static_cast<std::uint8_t>(values_[y * domain_.width() + x] >> 8);
        }
    }
    return texture;
}

LandLayout::LandLayout(HeightDomain domain) : domain_(domain) {
    require(domain.texture_columns() <= 16 && domain.texture_rows() <= 16, "Native LAND height-handle table is 16 by 16");
}
LandPatch LandLayout::patch(std::int32_t x, std::int32_t y) const {
    require(x >= 0 && x < columns() && y >= 0 && y < rows(), "LAND patch outside source domain");
    return LandPatch(*this, x, y);
}
LandPatch::LandPatch(const LandLayout& layout, std::int32_t column, std::int32_t row)
    : column_(column), row_(row), source_x_(column * 64), source_y_(row * 64),
      samples_x_(std::min(64, layout.domain().width() - 1 - source_x_) + 1),
      samples_y_(std::min(64, layout.domain().height() - 1 - source_y_) + 1),
      layout_rows_(layout.rows()), center_x_(layout.center_x()), center_y_(layout.center_y()),
      window_(layout.domain().texture_window(source_x_ / 896, source_y_ / 896)) {}
std::array<float, 4> LandPatch::offset_constants() const {
    return {static_cast<float>(source_x_), static_cast<float>(source_y_),
            static_cast<float>(texture_offset_x()), static_cast<float>(texture_offset_y())};
}
std::array<float, 3> LandPatch::world_translation() const { return {-center_x_, -center_y_, -0.0f}; }
std::array<float, 6> LandPatch::culling_bounds() const {
    const float x = static_cast<float>(source_x_) * LandLayout::scale_x - center_x_;
    const float y = static_cast<float>(source_y_) * LandLayout::scale_y - center_y_;
    const float z = -0.0f;
    return {x, y, z, static_cast<float>(samples_x_) * LandLayout::scale_x + x,
        static_cast<float>(samples_y_) * LandLayout::scale_y + y, 255.0f * LandLayout::scale_z + z};
}
std::array<float, 4> LandPatch::culling_sphere(X87Precision precision) const {
    require(precision == X87Precision::single || precision == X87Precision::double_precision,
            "Invalid LAND sphere precision");
    const auto bounds = culling_bounds();
    std::array<float, 4> result{};
    std::array<float, 3> extents{};
    for (unsigned i = 0; i < 3; ++i) {
        result[i] = (bounds[i] + bounds[i + 3]) * 0.5f;
        const auto negative = std::bit_cast<float>(std::bit_cast<std::uint32_t>(bounds[i]) ^ 0x80000000u);
        extents[i] = bounds[i + 3] + negative;
    }
    const auto square = [&](unsigned i) { return round_x87(double(extents[i]) * extents[i], precision); };
    const auto zy = round_x87(square(2) + square(1), precision);
    const auto length = round_x87(std::sqrt(round_x87(zy + square(0), precision)), precision);
    result[3] = static_cast<float>(round_x87(length * 0.5, precision));
    return result;
}
HeightUv LandPatch::displacement_uv(float local_x, float local_y) const {
    require(std::isfinite(local_x) && std::isfinite(local_y), "Non-finite LAND local coordinate");
    const float x = local_x + static_cast<float>(source_x_), y = local_y + static_cast<float>(source_y_);
    return {(x + static_cast<float>(texture_offset_x()) + 0.5f) * (1.0f / 1024.0f),
            (y + static_cast<float>(texture_offset_y()) + 0.5f) * (-1.0f / 1024.0f)};
}

} // namespace civ5::terrain
