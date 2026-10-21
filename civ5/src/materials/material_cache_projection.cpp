#include "civ5/materials/material_cache_projection.hpp"
#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace civ5::materials {
MaterialCacheProjection material_cache_projection(const std::array<float, 6> &bounds,
                                                  const terrain::Matrix4 &matrix,
                                                  std::uint32_t viewport_width, std::uint32_t base_size,
                                                  terrain::X87Precision precision, terrain::Vec4Path path,
                                                  MaterialCacheProjectionTrace *trace) {
    using namespace terrain;
    if ((precision != X87Precision::single && precision != X87Precision::double_precision) ||
        (path != Vec4Path::legacy_x87 && path != Vec4Path::sse))
        throw std::invalid_argument("Unsupported material projection arithmetic profile");
    const std::array<Vector4, 4> first{{{bounds[0], bounds[1], bounds[2], 1},
                                        {bounds[0], bounds[1], bounds[5], 1},
                                        {bounds[0], bounds[4], bounds[5], 1},
                                        {bounds[0], bounds[4], bounds[2], 1}}};
    const std::array<Vector4, 4> second{{{bounds[3], bounds[4], bounds[5], 1},
                                         {bounds[3], bounds[4], bounds[2], 1},
                                         {bounds[3], bounds[1], bounds[2], 1},
                                         {bounds[3], bounds[1], bounds[5], 1}}};
    const auto rounded = [precision](double value) { return round_x87(value, precision); };
    float maximum = 0;
    for (unsigned i = 0; i < 4; ++i) {
        const std::array transformed{vector4_transform(first[i], matrix, precision, path),
                                     vector4_transform(second[i], matrix, precision, path)};
        auto normalized = transformed;
        for (auto &corner : normalized) {
            const float reciprocal = 1.f / corner[3];
            for (auto &coordinate : corner)
                coordinate = coordinate * reciprocal;
        }
        const float dx = normalized[0][0] + (-normalized[1][0]);
        const float dy = normalized[0][1] + (-normalized[1][1]);
        const double squared = rounded(rounded(double(dy) * dy) + rounded(double(dx) * dx));
        const double half = rounded(rounded(std::sqrt(squared)) * .5);
        // The comparison uses the retained x87 value; a successful store spills
        // it to Float32 before the next corner pair is compared.
        if (half > maximum)
            maximum = static_cast<float>(half);
        if (trace) {
            trace->transformed[i] = transformed;
            trace->normalized[i] = normalized;
            trace->half_diagonals[i] = static_cast<float>(half);
        }
    }
    // FILD is signed and exact. The original unsigned correction is a separate
    // x87 addition, whose PC24 rounding must remain visible.
    double width = std::bit_cast<std::int32_t>(viewport_width);
    if (std::bit_cast<std::int32_t>(viewport_width) < 0)
        width = rounded(width + 0x1p32);
    const double metric = rounded(rounded(width * maximum) * .5);
    const auto spilled = static_cast<float>(metric);
    // Only the highest threshold sees the retained metric, before its Float32 spill.
    const auto selected = metric > 256.0 ? 512u : spilled > 128.f ? 256u : spilled > 64.f ? 128u : 64u;
    return {metric, std::max(base_size, selected)};
}
} // namespace civ5::materials
