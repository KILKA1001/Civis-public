#pragma once
#include <cstdint>
#include <span>
#include <vector>

namespace civ5::materials {
enum class MaterialGeometryBufferKind { vertex, index };
struct MaterialGeometryBufferEntry {
    std::uint32_t handle, capacity;
    bool busy;
};
class MaterialGeometryBufferBackend {
  public:
    virtual ~MaterialGeometryBufferBackend() = default;
    // Own the new buffer and its initial bytes before returning a nonzero ID.
    virtual std::uint32_t create(MaterialGeometryBufferKind kind, std::uint32_t elements,
                                 std::span<const std::uint8_t> bytes) = 0;
    virtual void upload(MaterialGeometryBufferKind kind, std::uint32_t handle,
                         std::span<const std::uint8_t> bytes) = 0;
};
// Original 872F60 first available fit, with independent vertex/index lists.
// Capacity is the original element count at creation, not driver padding.
// Original 873080 returns leases without deleting the physical resources.
class MaterialGeometryPool {
  public:
    void upload_pair(MaterialGeometryBufferBackend &backend, std::span<const std::uint8_t> vertices,
                      std::uint32_t vertex_count, std::span<const std::uint8_t> indices,
                      std::uint32_t index_count, std::uint32_t &vertex_buffer, std::uint32_t &index_buffer);
    void release_pair(std::uint32_t vertex_buffer, std::uint32_t index_buffer);
    std::span<const MaterialGeometryBufferEntry> entries(MaterialGeometryBufferKind kind) const;

  private:
    std::vector<MaterialGeometryBufferEntry> vertices_, indices_;
    void take(MaterialGeometryBufferBackend &, MaterialGeometryBufferKind,
              std::span<const std::uint8_t>, std::uint32_t, std::uint32_t &);
};
} // namespace civ5::materials
