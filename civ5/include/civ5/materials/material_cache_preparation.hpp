#pragma once
#include "material_cache_lifecycle.hpp"
#include <cstddef>
#include <span>

namespace civ5::materials {
struct MaterialCachePreparationDomain {
    std::int32_t columns, rows, geometry_stride;
};
class MaterialCachePreparationBackend : public MaterialCacheLifecycleBackend {
  public:
    // Allocation refusal returns zero. Successful results are native pointer-
    // width storage tokens understood by free_staging and the producer jobs.
    virtual std::uintptr_t allocate_staging(std::size_t bytes) = 0;
    virtual void submit_geometry(MaterialCacheGeometryJob &job, std::uintptr_t dispatch) = 0;
    virtual void prepare_material(MaterialCacheLifecycleState &state, std::uintptr_t dispatch) = 0;
};
// Jobs resolve the owner's live pointer-width staging tokens through the same
// resource owner that allocated them. Storage remains stable during a worker.
class MaterialCacheStagingBackend : public MaterialCachePreparationBackend {
  public:
    virtual std::span<std::uint8_t> staging_memory(std::uintptr_t token) = 0;
};
// Original 6BAB00. Allocates both geometry buffers before checking either
// result, submits the embedded geometry job, then selects material preparation
// from the current requested/completed states. Does not increment pending_jobs.
// The owner serializes this call and keeps domain values fixed throughout it.
// Incoming geometry staging slots must be available for replacement. Submitted
// work and the state/record must outlive the call; this function is not a queue.
// CPU geometry requires dimensions >= 2, positive stride and nonoverflowing
// original arithmetic. Invalid native domains throw before state or callbacks
// change. The separate-buffer mode does not inspect the geometry domain.
void material_cache_prepare(MaterialCacheLifecycleState &state, const MaterialCachePreparationDomain &domain,
                            MaterialCachePreparationBackend &backend, bool separate_geometry_buffers,
                            std::uintptr_t dispatch);
} // namespace civ5::materials
