#pragma once
#include "material_cache_preparation.hpp"
#include "material_generation.hpp"
#include <memory>

namespace civ5::materials {
struct MaterialCacheMaterialJob {
    MaterialCacheLifecycleState *owner = nullptr;
    std::uintptr_t normal_staging = 0;
    MaterialTile tile{};
};
struct MaterialCacheMaterialJobStorage {
    std::uintptr_t token;
    std::span<MaterialCacheMaterialJob> records;
};
enum class MaterialCacheStream { material48, normal24, indices16 };
struct MaterialCacheAppendSpan {
    std::uint32_t offset;
    std::span<std::uint8_t> bytes;
};
class MaterialCacheMaterialBackend : public MaterialCacheStagingBackend {
  public:
    // Native typed records replace the original 40-byte x86 callback ABI.
    // A refusal returns {0,{}}. free_staging also owns these record tokens.
    virtual MaterialCacheMaterialJobStorage allocate_material_records(std::size_t count) = 0;
    virtual void submit_material(MaterialCacheMaterialJob &job, std::uintptr_t dispatch) = 0;
    // The owner supplies retained append storage and its original element
    // offset, with enough capacity for the complete requested element count.
    virtual MaterialCacheAppendSpan append_material_stream(MaterialCacheStream stream,
                                                           std::size_t elements) = 0;
};
// Original 701EF0. Domain columns/rows exclude the two material border samples.
// Valid base dimensions are 0..65; geometry_stride is unused here. Calls all
// three allocations before checking failure, initializes the full native
// record array, then submits the 12x12 tiles in original X/Y order. It leaves
// entering readiness unchanged on success and clears it on allocation failure.
// Owner, records, source domain and staging must survive submitted execution.
void material_cache_prepare_material_jobs(MaterialCacheLifecycleState &state,
                                          const MaterialCachePreparationDomain &domain,
                                          MaterialCacheMaterialBackend &backend, std::uintptr_t dispatch);
// Original callback wrapper and material writer. The domain includes the two
// border samples. Final-cache size is read from the live owner; normal24 uses
// the token captured in the job, while material48 uses the owner's current
// slot.
void material_cache_execute_material_job(const MaterialCacheMaterialJob &job, MaterialGeometryDomain domain,
                                         const MaterialSelection &selection,
                                         MaterialCacheMaterialBackend &backend,
                                         const MaterialLookup &height_at,
                                         const MaterialNormalLookup &normal_at,
                                         const MaterialFactorLookup &factors_at);
// Original 7A79A0 append/copy/release order. Call only after all material jobs
// completed. Appends index16 and both vertex streams, stores their offsets and
// frees the three material staging slots. Does not complete the outer job.
void material_cache_append_material_jobs(MaterialCacheLifecycleState &state,
                                         const MaterialCachePreparationDomain &domain,
                                         MaterialCacheMaterialBackend &backend);
// Retained native staging for one submitted patch. The logical state must
// outlive this owner. Independent batches may share that logical state; each
// then owns private material staging while preserving original prepare/worker/
// append order. The three phases match preparation, the worker barrier, then
// generation-time append; none completes the outer lifecycle or changes its
// counter/stamp.
class MaterialCacheJobBatch {
  public:
    // independent_staging gives this submitted batch a private copy of the
    // material-job staging owner. The logical lifecycle owner remains state;
    // this is required when the original scheduler submits the same patch
    // more than once before the worker barrier.
    MaterialCacheJobBatch(MaterialCacheLifecycleState &state, int columns, int rows,
                          std::uintptr_t dispatch = 0);
    MaterialCacheJobBatch(MaterialCacheLifecycleState &state, int columns, int rows,
                          std::uintptr_t dispatch, bool independent_staging);
    ~MaterialCacheJobBatch();
    MaterialCacheJobBatch(const MaterialCacheJobBatch &) = delete;
    MaterialCacheJobBatch &operator=(const MaterialCacheJobBatch &) = delete;
    void execute(const MaterialGeometryDomain &domain, const MaterialSelection &selection,
                 const MaterialLookup &height_at, const MaterialNormalLookup &normal_at,
                 const MaterialFactorLookup &factors_at);
    MaterialPatch append();
    // Cancellation releases only this batch's material allocations. A batch
    // that has already appended leaves readiness unchanged when destroyed.
    void discard() noexcept;
    const MaterialCacheLifecycleState &state() const;
    std::size_t job_count() const;
    bool executed() const;

  private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
// Owned CPU runner: allocate and queue native records, execute their tiles in
// submission order, then append and release staging. A failed producer throws
// without publishing any patch. This does not complete GPU cache generation or
// advance the world's requested/completed state.
MaterialPatch build_material_patch_jobs(const MaterialGeometryDomain &domain,
                                        const MaterialSelection &selection, const MaterialLookup &height_at,
                                        const MaterialNormalLookup &normal_at,
                                        const MaterialFactorLookup &factors_at);
// Execute on the caller's persistent patch state. Requires prepared=true, a
// matching final size and empty material staging slots. Every queued record
// references this exact state. A producer exception clears owned staging and
// readiness before rethrowing; geometry staging and lifecycle counters survive.
MaterialPatch build_material_patch_jobs(MaterialCacheLifecycleState &state,
                                        const MaterialGeometryDomain &domain,
                                        const MaterialSelection &selection, const MaterialLookup &height_at,
                                        const MaterialNormalLookup &normal_at,
                                        const MaterialFactorLookup &factors_at);
} // namespace civ5::materials
