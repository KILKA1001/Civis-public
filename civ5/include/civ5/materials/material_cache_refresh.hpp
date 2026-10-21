#pragma once
#include "material_cache_scheduler.hpp"
#include "material_generation.hpp"

namespace civ5::materials {
struct MaterialCacheRefreshDomain {
    std::uint32_t columns, rows;
    float scale_x, scale_y, span_x, span_y;
};
struct MaterialCacheHeightMetrics {
    // Original +320/+324/+328. The first value compares the normalized
    // right-neighbor height and the absolute Y difference; do not relabel it
    // as a symmetric slope or substitute a conventional gradient.
    float first = 0.f, maximum = 0.f;
    std::uint32_t category = 1;
};
// Full 715AD0 scan. Dimensions include the material patch's two border
// samples, as in MaterialCachePatch::columns()/rows(). Source addressing uses
// the original wrapped U32 addition and unsigned remainder on both axes.
MaterialCacheHeightMetrics material_cache_height_metrics(int columns, int rows,
    MaterialOrigin origin, int source_width, int source_height,
    const MaterialLookup &height_at, terrain::X87Precision precision);

class MaterialCacheRefreshBackend {
  public:
    virtual ~MaterialCacheRefreshBackend() = default;
    // Read separately for each selected patch and again AFTER the barrier.
    virtual std::uint32_t source_revision() const = 0;
    // Original selected-count +1F8 only; other selection words survive until
    // the submitted producer replaces them.
    virtual void clear_selection(std::uint32_t patch) = 0;
    // 4B8040 submits the embedded owner-bound 7805F0 job. That job executes
    // 715AD0 height metrics then 6920C0 material scoring, in that order.
    virtual void submit_refresh(std::uint32_t patch) = 0;
    virtual void wait_refresh() = 0;
};
// Original 684240: inclusive end indices, X outer/Y inner, no deduplication
// across calls, and an unconditional barrier even for an empty rectangle.
// The fourth native flag is unused. Dimensions/scales remain stable while
// this serialized operation runs; backend exceptions may leave partial work.
void material_cache_refresh_region(const MaterialCacheRefreshDomain &domain,
    terrain::Point2 lower, terrain::Point2 upper, bool stamp_owner,
    terrain::X87Precision precision, std::span<MaterialCacheSchedulerPatch> patches,
    std::uint32_t &owner_revision, MaterialCacheRefreshBackend &backend);
} // namespace civ5::materials
