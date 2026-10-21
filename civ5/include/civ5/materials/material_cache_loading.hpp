#pragma once
#include "material_cache_scheduler.hpp"

namespace civ5::materials {
struct MaterialCacheLoadState {
    std::vector<std::uint32_t> foreground, background;
    std::uint32_t cursor = 0, background_cursor = 0, prepared_in_step = 0, finished = 0;
    float progress = 0;
};
// Original 73E230/872D60. Preserve input order and repeated indices, exclude
// only blocked==1, and test camera frusta in reverse order when requested.
// Replaces both queues and resets their cursors; progress, finished and the
// previous step count remain owned by the enclosing loading lifecycle.
void material_cache_begin_load(std::span<MaterialCacheSchedulerPatch> patches,
                                std::span<const std::uint32_t> active,
                                std::span<const MaterialCacheSchedulerView> views,
                                bool block_on_load, bool cull, MaterialCacheLoadState &);

class MaterialCacheLoadBackend {
  public:
    virtual ~MaterialCacheLoadBackend() = default;
    // Prepare the cache lighting frame with identity camera matrices.
    virtual void prepare_frame() = 0;
    // Own the local dispatch context and begin GPU cache generation.
    virtual void begin_step() = 0;
    virtual bool acquire_base(std::uint32_t patch) = 0;
    virtual void prepare(std::uint32_t patch, std::uintptr_t dispatch) = 0;
    virtual void wait() = 0;
    virtual void complete(std::uint32_t patch, std::uint32_t frame_stamp,
                          std::uintptr_t generation_argument) = 0;
    virtual void end_generation() = 0;
    virtual void end_step() = 0;
};
// Original 475B90. Up to four acquire -> request1 -> prepare -> wait ->
// complete operations, individually serialized. A failed acquisition advances
// to the queue end. Completion is published on the following call; an empty
// queue publishes progress1 immediately. No normal-frame budget is consumed.
// Queue membership and owner addresses must stay stable across callbacks.
void material_cache_load_step(std::span<MaterialCacheSchedulerPatch> patches,
                               MaterialCacheLoadState &, terrain::X87Precision precision,
                               std::uintptr_t dispatch, std::uint32_t frame_stamp,
                               std::uintptr_t generation_argument, MaterialCacheLoadBackend &);
} // namespace civ5::materials
