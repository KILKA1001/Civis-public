#pragma once
#include "material_cache_acquisition.hpp"
#include <atomic>

namespace civ5::materials {
struct MaterialCacheLifecycleState;
struct MaterialCacheGeometryJob {
    // Native counterpart of the embedded geometry record's owner at +150.
    // The owner and this record must remain alive until submitted work finishes.
    MaterialCacheLifecycleState *owner = nullptr;
};
struct MaterialCacheGeometryState {
    std::uint32_t vertex_buffer = 0, primary_index_buffer = 0, secondary_index_buffer = 0;
    // Final, higher, displaced-final, displaced-higher, overlay. Original
    // ordinary invalidation releases the first four and retains the overlay.
    std::array<std::uint32_t, 5> packets{};
    std::uint32_t vertex_count = 0, triangle_count = 0;
    // +1C4/+1C8, written only by the capability>=10 detail-packet path.
    std::array<std::uint32_t, 2> detail_levels{};
};
struct MaterialCacheLifecycleState {
    MaterialCacheTextureState textures;
    MaterialCacheGeometryState geometry;
    MaterialCacheGeometryJob geometry_job;
    // Appended material48, normal24 and index16 stream offsets: +C8/+CC/+D0.
    std::array<std::uint32_t, 3> material_stream_offsets{};
    std::array<std::uint32_t, 3> scratch{};
    // Pointer-width native staging tokens. Original transport order is
    // +318, +31C, +30C, +304, +308; these are not original process pointers.
    std::array<std::uintptr_t, 5> staging{};
    std::uint32_t requested_state = 0, completed_state = 0, frame_stamp = 0;
    // Raw original +1E0 completion counter; preparation leaves it untouched.
    // This is not a worker fence or native storage lifetime reference.
    std::atomic<std::uint32_t> pending_jobs{0};
    bool prepared = false;
};
// The resource owner supplies real geometry, material generation and storage
// operations. Zero release/free tokens are valid. Rebuild/generate callbacks
// receive live state; subsequent decisions read their resulting fields.
class MaterialCacheLifecycleBackend {
  public:
    virtual ~MaterialCacheLifecycleBackend() = default;
    virtual void rebuild_geometry(MaterialCacheLifecycleState &state) = 0;
    virtual void generate_cache(MaterialCacheLifecycleState &state, std::uint32_t requested_state,
                                std::uintptr_t generation_argument) = 0;
    virtual void release_geometry_pair(std::uint32_t vertex_buffer, std::uint32_t index_buffer) = 0;
    virtual void release_vertex_buffer(std::uint32_t handle) = 0;
    virtual void release_index_buffer(std::uint32_t handle) = 0;
    virtual void release_packet(std::uint32_t handle) = 0;
    virtual void free_staging(std::uintptr_t token) = 0;
    // Default implementation is an actual sequentially consistent atomic
    // decrement. An observing override must preserve that operation.
    virtual void decrement_pending(std::atomic<std::uint32_t> &pending);
};
// Original 609980. Does not clear scratch fields or return cache textures.
void material_cache_invalidate_geometry(MaterialCacheLifecycleState &state,
                                        MaterialCacheLifecycleBackend &backend,
                                        bool separate_geometry_buffers);
// Original 68ADB0. A completed base-only patch is unchanged; otherwise return
// higher textures, clear scratch, invalidate geometry, then publish state 1.
void material_cache_downgrade(MaterialCacheLifecycleState &state, MaterialCacheTexturePool &pool,
                              MaterialCacheLifecycleBackend &backend, bool separate_geometry_buffers);
// Original 455A60. The compressed final pool, when supplied, receives final
// handles in reverse order. Requested/completed states are not changed here.
void material_cache_release(MaterialCacheLifecycleState &state, MaterialCacheTexturePool &pool,
                            MaterialCacheLifecycleBackend &backend, bool separate_geometry_buffers,
                            bool geometry, bool final, bool higher,
                            MaterialCacheTexturePool *compressed_final_pool = nullptr);
// Original 72C1F0: release all five staging slots, preserving the original
// point at which each group of tokens is cleared.
void material_cache_clear_staging(MaterialCacheLifecycleState &state, MaterialCacheLifecycleBackend &backend);
// Original 71F260. Returns whether the completion tail executed. An unprepared
// job clears staging without changing completed state, pending count or stamp.
// Calls are serialized by the owner; this is not a worker queue or scheduler.
bool material_cache_dispatch(MaterialCacheLifecycleState &state, MaterialCacheTexturePool &pool,
                             MaterialCacheLifecycleBackend &backend, bool separate_geometry_buffers,
                             std::uint32_t frame_stamp, std::uintptr_t generation_argument);
} // namespace civ5::materials
