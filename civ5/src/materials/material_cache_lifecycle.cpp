#include "civ5/materials/material_cache_lifecycle.hpp"

namespace civ5::materials {
namespace {
void return_higher(MaterialCacheLifecycleState &state, MaterialCacheTexturePool &pool) {
    pool.give(state.textures.higher[1]);
    pool.give(state.textures.higher[0]);
    state.textures.higher[0] = 0;
    state.textures.higher[1] = 0;
}
void clear_scratch(MaterialCacheLifecycleState &state) {
    state.scratch[1] = 0;
    state.scratch[0] = 0;
    state.scratch[2] = 0;
}
} // namespace
void MaterialCacheLifecycleBackend::decrement_pending(std::atomic<std::uint32_t> &pending) {
    pending.fetch_sub(1, std::memory_order_seq_cst);
}
void material_cache_invalidate_geometry(MaterialCacheLifecycleState &state,
                                        MaterialCacheLifecycleBackend &backend,
                                        bool separate_geometry_buffers) {
    auto &geometry = state.geometry;
    if (!separate_geometry_buffers)
        backend.release_geometry_pair(geometry.vertex_buffer, geometry.primary_index_buffer);
    else {
        backend.release_vertex_buffer(geometry.vertex_buffer);
        backend.release_index_buffer(geometry.primary_index_buffer);
    }
    backend.release_index_buffer(geometry.secondary_index_buffer);
    for (unsigned i = 0; i < 4; ++i)
        backend.release_packet(geometry.packets[i]);
    geometry.primary_index_buffer = 0;
    geometry.vertex_buffer = 0;
    geometry.secondary_index_buffer = 0;
    for (unsigned i = 0; i < 4; ++i)
        geometry.packets[i] = 0;
}
void material_cache_clear_staging(MaterialCacheLifecycleState &state,
                                  MaterialCacheLifecycleBackend &backend) {
    backend.free_staging(state.staging[0]);
    backend.free_staging(state.staging[1]);
    state.staging[1] = 0;
    state.staging[0] = 0;
    backend.free_staging(state.staging[2]);
    backend.free_staging(state.staging[3]);
    backend.free_staging(state.staging[4]);
    state.staging[3] = 0;
    state.staging[4] = 0;
    state.staging[2] = 0;
}
void material_cache_downgrade(MaterialCacheLifecycleState &state, MaterialCacheTexturePool &pool,
                              MaterialCacheLifecycleBackend &backend, bool separate_geometry_buffers) {
    if (state.completed_state == 1)
        return;
    return_higher(state, pool);
    clear_scratch(state);
    material_cache_invalidate_geometry(state, backend, separate_geometry_buffers);
    state.completed_state = 1;
}
void material_cache_release(MaterialCacheLifecycleState &state, MaterialCacheTexturePool &pool,
                            MaterialCacheLifecycleBackend &backend, bool separate_geometry_buffers,
                            bool geometry, bool final, bool higher,
                            MaterialCacheTexturePool *compressed_final_pool) {
    if (higher)
        return_higher(state, pool);
    clear_scratch(state);
    if (final) {
        if (compressed_final_pool) {
            compressed_final_pool->give(state.textures.final[1]);
            compressed_final_pool->give(state.textures.final[0]);
        } else {
            pool.give(state.textures.final[0]);
            pool.give(state.textures.final[1]);
        }
        state.textures.final[0] = 0;
        state.textures.final[1] = 0;
    }
    if (geometry)
        material_cache_invalidate_geometry(state, backend, separate_geometry_buffers);
}
bool material_cache_dispatch(MaterialCacheLifecycleState &state, MaterialCacheTexturePool &pool,
                             MaterialCacheLifecycleBackend &backend, bool separate_geometry_buffers,
                             std::uint32_t frame_stamp, std::uintptr_t generation_argument) {
    if (!state.prepared) {
        material_cache_clear_staging(state, backend);
        return false;
    }
    switch (state.requested_state) {
    case 1:
        backend.rebuild_geometry(state);
        backend.generate_cache(state, state.requested_state, generation_argument);
        return_higher(state, pool);
        clear_scratch(state);
        material_cache_invalidate_geometry(state, backend, separate_geometry_buffers);
        break;
    case 3:
        if (state.completed_state != 1) {
            return_higher(state, pool);
            clear_scratch(state);
            material_cache_invalidate_geometry(state, backend, separate_geometry_buffers);
            backend.generate_cache(state, state.requested_state, generation_argument);
        }
        backend.rebuild_geometry(state);
        break;
    case 4:
        backend.rebuild_geometry(state);
        break;
    case 5:
        clear_scratch(state);
        material_cache_invalidate_geometry(state, backend, separate_geometry_buffers);
        backend.rebuild_geometry(state);
        if (state.completed_state != 4)
            backend.generate_cache(state, state.requested_state, generation_argument);
        break;
    default:
        break;
    }
    state.completed_state = state.requested_state;
    backend.decrement_pending(state.pending_jobs);
    state.frame_stamp = frame_stamp;
    return true;
}
} // namespace civ5::materials
