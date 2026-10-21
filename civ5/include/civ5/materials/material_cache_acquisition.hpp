#pragma once
#include <array>
#include <cstdint>

namespace civ5::materials {
// Logical native resource handles, independent of GL names and original
// pointers.
struct MaterialCacheTextureState {
    std::uint32_t higher_size = 0, final_size = 0, desired_higher_size = 0;
    std::array<std::uint32_t, 2> higher{}, final{};
    bool specular = false;
};
class MaterialCacheTexturePool {
  public:
    virtual ~MaterialCacheTexturePool() = default;
    // Failure returns zero and leaves actual_size unchanged. Pool exhaustion
    // is a normal result; backend resource creation happens before acquisition.
    virtual std::uint32_t take(std::uint32_t requested_size, std::uint32_t &actual_size) = 0;
    virtual void give(std::uint32_t handle) = 0; // give(0) is allowed.
};
// Ordinary uncompressed 4F71C0. Resource-owning callers must establish that a
// specular final pair is absent or complete. Original partial-pair overwrite
// behavior is preserved for explicit diagnostic inputs, not repaired here.
bool material_cache_acquire_base(MaterialCacheTextureState &patch, MaterialCacheTexturePool &pool,
                                 std::uint32_t base_size);
// Ordinary uncompressed 54F770. Reserve replacements before returning the old
// handles. Successful replacement restores the entering final-size field;
// failure may retain a size write from a successful final reservation.
bool material_cache_acquire_higher(MaterialCacheTextureState &patch, MaterialCacheTexturePool &pool,
                                   std::uint32_t base_size);
} // namespace civ5::materials
