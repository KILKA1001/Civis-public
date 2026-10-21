#include "civ5/materials/material_sources.hpp"
#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace civ5::materials {
MaterialExpansion::MaterialExpansion(std::span<const std::int32_t> projections) {
    if (projections.empty() || projections.size() > 64)
        throw std::invalid_argument("Material row count must be 1..64");
    bases_.assign(projections.size(), -1);
    for (std::size_t row = 0; row < projections.size(); ++row) {
        bases_[row] = static_cast<std::int32_t>(rows_.size());
        for (std::int32_t mode = 0; mode < (projections[row] == 0 ? 1 : 3); ++mode) {
            if (rows_.size() == 64)
                return;
            rows_.push_back(static_cast<std::int32_t>(row));
            modes_.push_back(mode);
        }
    }
}
std::array<std::uint16_t, 16> MaterialExpansion::ordinary_slots(std::span<const std::int32_t> selected,
                                                                std::span<const std::uint32_t> diffuse,
                                                                std::span<const std::uint32_t> height) const {
    if (selected.size() > 8 || diffuse.size() != bases_.size() || height.size() != bases_.size())
        throw std::invalid_argument("Invalid material slot input extent");
    std::array<std::uint16_t, 16> result{};
    for (std::size_t lane = 0; lane < selected.size(); ++lane) {
        const auto source = selected[lane];
        if (source < 0 || static_cast<std::size_t>(source) >= rows_.size())
            throw std::invalid_argument("Selected material source is unavailable");
        const auto row = rows_[source];
        result[lane] = static_cast<std::uint16_t>(diffuse[row]);
        result[lane + 8] = static_cast<std::uint16_t>(height[row]);
    }
    return result;
}
float material_texture_multiplier(float scale, std::int32_t projection, float global_span) {
    if (!std::isfinite(scale) || !std::isfinite(global_span) || global_span == 0)
        throw std::invalid_argument("Material multiplier requires finite scale/nonzero span");
    const float factor = projection == 0 ? 0.33000001311302185f : 0.6600000262260437f;
    const float reciprocal = projection == 0 ? 3.0303030014038086f : 1.5151515007019043f;
    const float quantized_input = (scale * factor) * global_span;
    if (!std::isfinite(quantized_input) || quantized_input < -2147483648.f || quantized_input >= 2147483648.f)
        throw std::invalid_argument("Material quantization outside finite Int32 domain");
    const auto quantized = static_cast<std::int32_t>(quantized_input);
    const float inverse_span = 1.f / global_span;
    const float multiplier = (static_cast<float>(quantized) * inverse_span) * reciprocal;
    if (!std::isfinite(multiplier))
        throw std::invalid_argument("Nonfinite material multiplier");
    return multiplier;
}
void write_material_constants(std::span<const std::int32_t> source_rows,
                              std::span<const std::int32_t> source_modes, std::span<const float> row_scales,
                              float global_span, std::span<float, 384> destination) {
    if (source_rows.size() > 64 || source_modes.size() != source_rows.size() || row_scales.empty() ||
        row_scales.size() > 64 || !std::isfinite(global_span) || global_span == 0 ||
        !std::all_of(row_scales.begin(), row_scales.end(), [](float x) { return std::isfinite(x); }))
        throw std::invalid_argument("Invalid material constant producer input");
    std::array<float, 256> active{};
    for (std::size_t i = 0; i < source_rows.size(); ++i) {
        const auto row = source_rows[i];
        if (row < 0 || static_cast<std::size_t>(row) >= row_scales.size())
            throw std::invalid_argument("Material constant source row is unavailable");
        active[i * 4] = material_texture_multiplier(row_scales[row], source_modes[i], global_span);
        active[i * 4 + 1] = static_cast<float>(source_modes[i]);
    }
    std::copy_n(active.begin(), source_rows.size() * 4, destination.begin());
}
std::array<assets::Rgba8Level, 7> material_resource_zero() {
    constexpr std::array<int, 16> dither{31, 15, 27, 11, 7, 23, 3, 19, 25, 9, 29, 13, 1, 17, 5, 21};
    std::array<assets::Rgba8Level, 7> levels{};
    for (std::size_t level = 0; level < levels.size(); ++level) {
        const std::uint32_t size = 256u >> level;
        auto &image = levels[level];
        image.width = image.height = size;
        image.rgba.resize(size * size * 4);
        const float inverse = 1.f / static_cast<float>(size);
        for (std::uint32_t y = 0; y < size; ++y)
            for (std::uint32_t x = 0; x < size; ++x) {
                const float u = (static_cast<float>(x) + .5f) * inverse;
                const float v = (static_cast<float>(y) + .5f) * inverse;
                const bool red = ((static_cast<int>(u * 255.f) ^ static_cast<int>(v * 255.f)) & 16) != 0;
                const auto ordinal = (y & 1) == 0 ? x : size - 1 - x;
                const float threshold = static_cast<float>(dither[(y & 3) * 4 + (ordinal & 3)]) / 32.f;
                const auto offset = (y * size + x) * 4;
                image.rgba[offset] = 255;
                image.rgba[offset + 1] = image.rgba[offset + 2] = red ? 0 : 255;
                image.rgba[offset + 3] =
                    static_cast<std::uint8_t>(static_cast<int>((red ? 255.f : 127.5f) + threshold));
            }
    }
    return levels;
}
} // namespace civ5::materials
