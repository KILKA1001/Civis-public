#pragma once
#include "material_geometry_pool.hpp"

namespace civ5::materials {
struct SharedLandGeometry {
    std::uint32_t columns, rows, triangle_count, vertex_buffer, index_buffer, control_buffer;
};
class SharedLandGeometryBackend {
  public:
    virtual ~SharedLandGeometryBackend() = default;
    // Creation owns one reference for the cache. Control data use a separate
    // index-format allocation; they are packed addresses, not triangle indices.
    virtual std::uint32_t create_shared(MaterialGeometryBufferKind, std::uint32_t elements,
                                        std::span<const std::uint8_t>) = 0;
    virtual void discard_shared(MaterialGeometryBufferKind, std::uint32_t handle) noexcept = 0;
};
// Original 761870: insertion-ordered cache keyed only by columns and rows.
// A hit ignores changed step/capability. The caller owns quality invalidation.
// The resource backend outlives the entries and owns their base references.
class SharedLandGeometryCache {
  public:
    const SharedLandGeometry &get(SharedLandGeometryBackend &, std::uint32_t columns, std::uint32_t rows,
                                  std::int32_t step, std::int32_t capability);
    std::span<const SharedLandGeometry> entries() const { return entries_; }
  private:
    std::vector<SharedLandGeometry> entries_;
};
} // namespace civ5::materials
