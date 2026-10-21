#include "civ5/materials/material_cache_preparation.hpp"
#include <stdexcept>

namespace civ5::materials {
namespace {
struct GeometryAllocation {
    std::uint32_t vertices, triangles;
    std::size_t vertex_bytes, index_bytes;
};
GeometryAllocation allocation(const MaterialCachePreparationDomain &domain) {
    if (domain.columns < 2 || domain.rows < 2 || domain.geometry_stride <= 0)
        throw std::invalid_argument("Invalid material cache preparation domain");
    const auto x = std::uint64_t(domain.columns) + domain.geometry_stride - 2;
    const auto y = std::uint64_t(domain.rows) + domain.geometry_stride - 2;
    if (x > INT32_MAX || y > INT32_MAX)
        throw std::overflow_error("Material geometry signed division input overflows");
    const auto cx = x / domain.geometry_stride + 1;
    const auto cy = y / domain.geometry_stride + 1;
    const auto vertices = cx * cy, triangles = 2 * (cx - 1) * (cy - 1);
    const auto source_cells = std::uint64_t(domain.columns - 1) * (domain.rows - 1);
    if (vertices > UINT32_MAX / 24 || source_cells > UINT32_MAX / 6 || triangles > UINT32_MAX)
        throw std::overflow_error("Material geometry staging extent overflows");
    return {static_cast<std::uint32_t>(vertices), static_cast<std::uint32_t>(triangles),
            static_cast<std::size_t>(vertices * 24), static_cast<std::size_t>(source_cells * 6)};
}
} // namespace
void material_cache_prepare(MaterialCacheLifecycleState &state, const MaterialCachePreparationDomain &domain,
                            MaterialCachePreparationBackend &backend, bool separate_geometry_buffers,
                            std::uintptr_t dispatch) {
    const auto sizes = separate_geometry_buffers ? GeometryAllocation{} : allocation(domain);
    state.prepared = true;
    if (!separate_geometry_buffers) {
        state.geometry.vertex_count = sizes.vertices;
        state.geometry.triangle_count = sizes.triangles;
        state.staging[0] = backend.allocate_staging(sizes.vertex_bytes);
        state.staging[1] = backend.allocate_staging(sizes.index_bytes);
        if (!state.staging[0] || !state.staging[1]) {
            backend.free_staging(state.staging[0]);
            backend.free_staging(state.staging[1]);
            state.staging[0] = 0;
            state.staging[1] = 0;
            state.prepared = false;
            return;
        }
        state.geometry_job.owner = &state;
        backend.submit_geometry(state.geometry_job, dispatch);
    }
    const auto requested = state.requested_state;
    if (requested == 1 || (requested == 3 && state.completed_state != 1) ||
        (requested == 5 && state.completed_state != 4))
        backend.prepare_material(state, dispatch);
}
} // namespace civ5::materials
