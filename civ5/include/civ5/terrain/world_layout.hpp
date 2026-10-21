#pragma once
#include "native_math.hpp"

namespace civ5::terrain {
struct WorldGrid {
    std::int32_t width, height;
    FactoryDescriptor descriptor;
    IntMatrix4 first_matrix, second_matrix;
};
struct WorldLayout {
    WorldGrid source_grid, worker_grid;
    // D80E40/E80/EC0/F00 followed by four Vec4 outputs, not GPU matrices.
    std::array<float, 80> transforms;
    std::array<float, 9> bounds;
};
struct WorldGridLayout {
    WorldGrid source_grid, worker_grid;
};
// Retail game load (45EFE6..45F010). Editor mode 2 is a separate caller.
constexpr std::int32_t game_world_grid_mode(bool wrap_x) { return wrap_x ? 1 : 0; }
// Integer grid setup is independent of the floating world/camera transforms.
WorldGridLayout build_world_grid_layout(std::int32_t width, std::int32_t height,
                                        std::int32_t mode, const FactoryDescriptor &factory);
WorldLayout build_world_layout(std::int32_t width, std::int32_t height, std::int32_t mode,
                               const FactoryDescriptor &factory,
                               const ReciprocalEstimate &reciprocal_estimate);
} // namespace civ5::terrain
