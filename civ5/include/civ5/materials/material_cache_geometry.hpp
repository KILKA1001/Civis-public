#pragma once
#include "material_cache_preparation.hpp"
#include "civ5/terrain/native_math.hpp"

namespace civ5::materials {
struct MaterialCacheGeometryDomain {
    // Source LAND patch dimensions, without material border samples. The
    // original quality initializer supplies geometry_stride 1, 2 or 3.
    MaterialCachePreparationDomain patch;
    std::array<std::int32_t, 2> origin;
    std::int32_t source_width, source_height;
    terrain::Vector3 scales;
    terrain::X87Precision precision;
    bool wrap_x = true;
};
using MaterialCacheHeightLookup = std::function<std::uint16_t(std::int32_t, std::int32_t)>;
// Original 4FCAD0 -> 678E80. Resolve the owner's current geometry allocations,
// write position/half-UV lanes in the original X/Y order, preserve normal bytes
// +12..+19 and unused allocation tails, then store the actual triangle count.
// Domains, storage extents and source addresses are checked before writes.
// Owner/domain/storage must remain stable throughout execution. A height
// callback exception can leave partial vertices; discard the failed job before
// publishing it. This neither uploads geometry nor completes the outer job.
void material_cache_execute_geometry_job(const MaterialCacheGeometryJob &job,
                                         const MaterialCacheGeometryDomain &domain,
                                         MaterialCacheStagingBackend &backend,
                                         const MaterialCacheHeightLookup &height_at);
} // namespace civ5::materials
