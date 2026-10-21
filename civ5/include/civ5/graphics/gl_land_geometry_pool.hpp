#pragma once
#include "civ5/materials/land_material.hpp"
#include "civ5/materials/material_geometry_pool.hpp"
#include "civ5/materials/shared_land_geometry.hpp"
#include "gl_context.hpp"

namespace civ5::graphics {
class GlLandGeometryPool;
class GlLandGeometryLease {
  public:
    std::uint32_t vertex_buffer() const { return vertex_; }
    std::uint32_t index_buffer() const { return index_; }
    std::uint32_t control_buffer() const { return control_; }
    std::uint32_t vertex_count() const { return vertices_; }
    std::uint32_t index_count() const { return indices_; }
    materials::LandVertexInput input() const { return input_; }

  private:
    friend class GlLandGeometryPool;
    std::shared_ptr<const std::uint8_t> owner_identity_;
    std::uint32_t vertex_ = 0, index_ = 0, vertices_ = 0, indices_ = 0;
    std::uint32_t control_ = 0;
    bool shared_ = false;
    std::uint64_t generation_ = 0;
    materials::LandVertexInput input_ = materials::LandVertexInput::source_positioned;
};
// Render-thread owner of the original paired geometry pool. IDs are logical,
// and returned/replaced leases cannot be used to draw another patch's data.
// A renderer retains this owner; return a lease explicitly when invalidating.
class GlLandGeometryPool {
  public:
    explicit GlLandGeometryPool(std::shared_ptr<GlContext>);
    ~GlLandGeometryPool();
    GlLandGeometryPool(const GlLandGeometryPool &) = delete;
    GlLandGeometryPool &operator=(const GlLandGeometryPool &) = delete;
    GlLandGeometryLease upload(const materials::LandMaterialMesh &);
    // Cache holds one reference; every call returns a separate draw lease.
    // Step/capability affect misses only, matching the original dimension key.
    GlLandGeometryLease shared_grid(std::uint32_t columns, std::uint32_t rows,
                                    std::int32_t step, std::int32_t capability);
    // Original rebuild callbacks publish the cache handles before retaining
    // vertex/control/primary buffers individually. This reservation cannot be
    // drawn until every nonzero buffer has been retained. release() also
    // cancels a partially retained reservation after a failed rebuild.
    GlLandGeometryLease begin_shared_grid(std::uint32_t columns, std::uint32_t rows,
                                          std::int32_t step, std::int32_t capability);
    void retain_shared_buffer(const GlLandGeometryLease &, std::uint32_t handle);
    void release_shared_buffer(const GlLandGeometryLease &, std::uint32_t handle);
    void release(const GlLandGeometryLease &);
    // Validates owner, generation, live leases and the expected GL context.
    std::array<GLuint, 2> buffers(const GlLandGeometryLease &, const GlContext &) const;
    std::vector<std::uint8_t> read_buffer(const GlLandGeometryLease &, materials::MaterialGeometryBufferKind) const;
    std::vector<std::uint8_t> read_control_buffer(const GlLandGeometryLease &) const;
    std::span<const materials::MaterialGeometryBufferEntry> entries(materials::MaterialGeometryBufferKind) const;
    std::span<const materials::SharedLandGeometry> shared_entries() const;
    std::uint32_t shared_references(std::uint32_t handle) const;

  private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
} // namespace civ5::graphics
