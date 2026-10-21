#pragma once
#include "terrain_decal_packet.hpp"

namespace civ5::materials {
inline constexpr std::uint32_t no_terrain_decal_slot = 0x0fffffffu;
struct TerrainDecalStoreDomain {
    std::array<float, 4> bounds;
    std::uint32_t columns, rows;
};
// Original 442550: independently divide physical spans by sample scales and
// by (scale*256), then truncate cell counts and add one. Do not substitute
// integer source dimensions: intermediate Float32 rounding is observable.
TerrainDecalStoreDomain terrain_decal_store_domain(float span_x, float span_y, float scale_x, float scale_y);
struct TerrainDecalLinks {
    std::uint32_t next = no_terrain_decal_slot, previous = no_terrain_decal_slot;
    bool active = false;
};
struct TerrainDecalStoredRecord : TerrainDecalLinks {
    bool uploaded = false, mode = false;
    std::uint32_t first_vertex = 0, triangles = 0;
    terrain::TerrainDecalRecord material{};
    std::array<float, 4> bounds{};
};
struct TerrainDecalBatch {
    std::uint32_t free = no_terrain_decal_slot, head = no_terrain_decal_slot, tail = no_terrain_decal_slot, live = 0;
    std::vector<TerrainDecalStoredRecord> records;
    std::vector<std::array<std::uint16_t, 4>> vertices;
    std::uint32_t buffer = 0;
    bool holes = false, upload = true;
};
struct TerrainDecalSmallRecord : TerrainDecalLinks {
    terrain::Point2 center{0, 0};
    std::uint32_t batch = 0, record = 0;
};
struct TerrainDecalLargeRecord : TerrainDecalLinks {
    std::uint32_t batch = 0, record = 0;
};
// Original 684240 boundary. stamp_owner is always false here; the original
// fourth flag is reported for fidelity, but 684240 itself does not use it.
using TerrainDecalRegionRefresh = std::function<void(std::array<float, 4> bounds, bool stamp_owner, bool specular)>;
class TerrainDecalBufferBackend {
  public:
    virtual ~TerrainDecalBufferBackend() = default;
    // Own one 65,535-element buffer of packed half XY/UV (native format 0x12)
    // and return its nonzero ID. The backend owns resource destruction.
    virtual std::uint32_t create() = 0;
    // Copy the initialized prefix synchronously; tail capacity is never drawn.
    // Original copying uses native buffer-size metadata, including tail/padding.
    virtual void upload(std::uint32_t buffer, std::span<const std::array<std::uint16_t, 4>> vertices) = 0;
};
class TerrainDecalStore {
  public:
    // Explicit provider+6C..8C domain/dimensions. 534320 computes the scales.
    // World-global construction (442550) and GPU ownership belong to caller.
    // Cell count must be 1..1,048,576; finite positive extent/scales required.
    TerrainDecalStore(std::array<float, 4> domain, std::uint32_t columns, std::uint32_t rows,
                     std::uint32_t blank_specular);
    TerrainDecalStore(const TerrainDecalStore &) = delete;
    TerrainDecalStore &operator=(const TerrainDecalStore &) = delete;
    TerrainDecalStore(TerrainDecalStore &&) noexcept = default;
    TerrainDecalStore &operator=(TerrainDecalStore &&) noexcept = default;

    // 69E140 first-fit batches; 73EC80 removal; 42E850 stable compaction.
    // Handles are reused original slot IDs, NOT generation-protected leases.
    // Empty/excessive geometry returns absent_terrain_decal without effects.
    std::uint32_t submit(const terrain::TerrainDecalRecord &, const terrain::TerrainDecalGeometry &,
                         std::uint8_t mode = 0, const TerrainDecalRegionRefresh &refresh = {});
    void remove(std::uint32_t handle, const TerrainDecalRegionRefresh &refresh = {});
    void compact(std::uint32_t batch);
    // Original 44EE70: lazy buffer creation, upload dirty batches, then set
    // each live record's uploaded bit and clear the batch upload flag.
    // Use the same backend/resource namespace for this store's lifetime.
    // A failed upload keeps its allocated ID and pending flag for retry.
    // Backend calls may inspect the store, but must not mutate/reenter it.
    void upload_pending(TerrainDecalBufferBackend &backend);
    const TerrainDecalStoredRecord *find(std::uint32_t handle) const;
    std::uint32_t spatial_cell(terrain::Point2) const;

    // Read-only views survive until the next mutation. Dead slot payloads and
    // un-compacted vertex holes are retained as in the original.
    std::span<const TerrainDecalBatch> batches() const { return batches_; }
    std::span<const TerrainDecalSmallRecord> small_records() const { return small_; }
    std::span<const TerrainDecalLargeRecord> large_records() const { return large_; }
    std::span<const std::uint32_t> cell_heads() const { return heads_; }
    std::array<float, 4> domain() const { return domain_; }
    terrain::Point2 scales() const { return scales_; }
    std::uint32_t columns() const { return columns_; }
    std::uint32_t rows() const { return rows_; }
    std::uint32_t small_free() const { return small_free_; }
    std::uint32_t large_free() const { return large_free_; }
    std::uint32_t large_head() const { return large_head_; }
    std::uint32_t large_tail() const { return large_tail_; }
    std::uint32_t small_live() const { return small_live_; }
    std::uint32_t large_live() const { return large_live_; }
    // Refresh callbacks may inspect this store, but must not mutate it or
    // submit recursively. On callback failure, submit retains any prior
    // compaction/new empty batch; remove restores its handle/list state.
    // External callback side effects are the caller's responsibility.
  private:
    std::array<float, 4> domain_;
    terrain::Point2 scales_{0, 0};
    std::uint32_t columns_, rows_, blank_;
    std::vector<std::uint32_t> heads_;
    std::vector<TerrainDecalBatch> batches_;
    std::vector<TerrainDecalSmallRecord> small_;
    std::vector<TerrainDecalLargeRecord> large_;
    std::uint32_t small_free_ = no_terrain_decal_slot, large_free_ = no_terrain_decal_slot;
    std::uint32_t small_live_ = 0, large_live_ = 0;
    std::uint32_t large_head_ = no_terrain_decal_slot, large_tail_ = no_terrain_decal_slot;
};
} // namespace civ5::materials
