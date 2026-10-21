#pragma once
#include "material_cache_acquisition.hpp"
#include <map>
#include <span>
#include <vector>

namespace civ5::materials {
// Ordinary 710F40/6C1460 semantics over registered resources. The four lists
// are LIFO, and an exhausted request falls back 512 -> 256 -> 128 -> 64.
// This owner tracks handles and leases; the backend owns the actual textures.
// It never creates a texture to satisfy take(). Calls are serialized by the
// resource owner, on its render thread when backed by GL.
class MaterialCacheFreeLists final : public MaterialCacheTexturePool {
  public:
    MaterialCacheFreeLists() = default;
    MaterialCacheFreeLists(const MaterialCacheFreeLists &) = delete;
    MaterialCacheFreeLists &operator=(const MaterialCacheFreeLists &) = delete;
    // Register an existing resource. Available entries append in registration
    // order; reserved entries may be returned later. Zero/duplicate handles
    // and unsupported metadata are rejected before changing logical state.
    void add_texture(std::uint32_t handle, std::uint32_t width, bool available);
    std::uint32_t take(std::uint32_t requested_size, std::uint32_t &actual_size) override;
    // Unknown handles and double returns are ownership errors. Capacity is
    // reserved during registration, so a valid return cannot allocate memory.
    void give(std::uint32_t handle) override;
    bool contains(std::uint32_t handle) const;
    bool available(std::uint32_t handle) const;
    std::uint32_t texture_width(std::uint32_t handle) const;
    std::size_t texture_count() const { return entries_.size(); }
    std::span<const std::uint32_t> free_handles(std::uint32_t width) const;

  private:
    struct Entry {
        unsigned slot;
        bool available;
    };
    std::map<std::uint32_t, Entry> entries_;
    std::array<std::vector<std::uint32_t>, 4> free_;
    std::array<std::size_t, 4> counts_{};
};
} // namespace civ5::materials
