#pragma once
#include "civ5/terrain/native_math.hpp"

namespace civ5::materials {
struct MaterialCacheProjection {
    double metric;
    std::uint32_t selected_size;
};
// Optional observations of the actual calculation, for original-code validation.
struct MaterialCacheProjectionTrace {
    std::array<std::array<terrain::Vector4, 2>, 4> transformed, normalized;
    std::array<float, 4> half_diagonals;
};
// Original 6826D0 projected metric followed by 413030 size selection.
// Bounds are {minX,minY,minZ,maxX,maxY,maxZ}; matrix and viewport width come
// from the caller's view. Preserve the unsigned viewport and base-size inputs.
// The verified domain has finite intermediate values and nonzero projected W;
// negative W is allowed. This function does not perform visibility/culling.
MaterialCacheProjection material_cache_projection(const std::array<float, 6> &bounds,
                                                  const terrain::Matrix4 &matrix,
                                                  std::uint32_t viewport_width, std::uint32_t base_size,
                                                  terrain::X87Precision precision, terrain::Vec4Path path,
                                                  MaterialCacheProjectionTrace *trace = nullptr);
} // namespace civ5::materials
