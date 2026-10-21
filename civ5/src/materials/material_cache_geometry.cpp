#include "civ5/materials/material_cache_geometry.hpp"
#include "civ5/terrain/land_grid.hpp"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <stdexcept>

namespace civ5::materials {
void material_cache_execute_geometry_job(const MaterialCacheGeometryJob &job,
                                         const MaterialCacheGeometryDomain &domain,
                                         MaterialCacheStagingBackend &backend,
                                         const MaterialCacheHeightLookup &height_at) {
    const auto &p = domain.patch;
    if (!job.owner || !height_at || p.columns < 2 || p.columns > 65 || p.rows < 2 || p.rows > 65 ||
        p.geometry_stride < 1 || p.geometry_stride > 3 || domain.source_width <= 0 ||
        domain.source_height <= 0 || domain.origin[1] < 0 ||
        std::int64_t(domain.origin[1]) + p.rows > domain.source_height ||
        (domain.precision != terrain::X87Precision::single &&
         domain.precision != terrain::X87Precision::double_precision) ||
        !std::all_of(domain.scales.begin(), domain.scales.end(), [](float value) { return std::isfinite(value); }))
        throw std::invalid_argument("Invalid material cache CPU geometry domain");
    const int step = p.geometry_stride, columns = (p.columns + step - 2) / step + 1,
              rows = (p.rows + step - 2) / step + 1;
    std::array<std::int32_t, 65> global_x{}, source_x{};
    for (int x = 0; x < columns; ++x) {
        global_x[x] = terrain::wrap_add(domain.origin[0], std::min(x * step, p.columns - 1));
        source_x[x] = domain.wrap_x
                          ? terrain::wrap_add(terrain::wrap_mul(domain.source_width, 100), global_x[x]) %
                                domain.source_width
                          : global_x[x];
        if (source_x[x] < 0 || source_x[x] >= domain.source_width)
            throw std::invalid_argument("CPU geometry source X is outside its height owner");
    }
    // Original 678E80 applies the source step again to the already reduced
    // vertex grid. The worker's triangle count differs from preparation's.
    const auto indices = terrain::land_grid_indices(columns, rows, step);
    auto &state = *job.owner;
    const auto vertices = backend.staging_memory(state.staging[0]);
    const auto index_bytes = backend.staging_memory(state.staging[1]);
    if (vertices.size() < std::size_t(columns * rows) * 24 || index_bytes.size() < indices.size() * 2)
        throw std::length_error("CPU geometry exceeds its staging allocation");
    state.geometry.vertex_count = static_cast<std::uint32_t>(columns * rows);
    for (int x = 0; x < columns; ++x) {
        const int sx = std::min(x * step, p.columns - 1);
        const float u = static_cast<float>(terrain::round_x87(double(sx + 1) / double(p.columns + 1),
                                                              domain.precision));
        for (int y = 0; y < rows; ++y) {
            const int sy = std::min(y * step, p.rows - 1), gy = domain.origin[1] + sy;
            const auto height = height_at(source_x[x], gy);
            const terrain::Vector3 position{
                static_cast<float>(global_x[x]) * domain.scales[0],
                static_cast<float>(gy) * domain.scales[1],
                (static_cast<float>(height) * 0.003921568859368563f) * domain.scales[2]};
            const float v = static_cast<float>(sy + 1) / static_cast<float>(p.rows + 1);
            const std::array<std::uint16_t, 2> uv{terrain::land_unit_half(u), terrain::land_unit_half(v)};
            auto *destination = vertices.data() + std::size_t(y * columns + x) * 24;
            std::memcpy(destination, position.data(), 12);
            std::memcpy(destination + 20, uv.data(), 4);
        }
    }
    std::memcpy(index_bytes.data(), indices.data(), indices.size() * 2);
    state.geometry.triangle_count = static_cast<std::uint32_t>(indices.size() / 3);
}
} // namespace civ5::materials
