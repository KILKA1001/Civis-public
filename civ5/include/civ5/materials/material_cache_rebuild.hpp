#pragma once
#include "material_cache_lifecycle.hpp"
#include "material_cache_passes.hpp"

namespace civ5::materials {
struct LandPacketTextures {
    // Original owner +540/+550/+558/+548/+54C/+564, in packet slot order.
    std::array<std::uint32_t, 6> shared{};
    std::array<std::uint32_t, 5> style_diffuse{}, style_specular{};
};
struct MaterialCacheRebuildSettings {
    std::uint32_t columns, rows, style_mask, height_texture;
    float detail_level;
    std::int32_t capability;
    bool separate_geometry_buffers;
};
class MaterialCacheRebuildBackend {
  public:
    virtual ~MaterialCacheRebuildBackend() = default;
    // Implementations publish each resulting handle through the live output
    // reference. The paired uploader owns its capacity/reuse policy.
    virtual void upload_geometry_pair(std::uintptr_t vertices, std::uint32_t vertex_count,
                                      std::uintptr_t indices, std::uint32_t index_count,
                                      std::uint32_t &vertex_buffer, std::uint32_t &index_buffer) = 0;
    virtual void shared_geometry(std::uint32_t columns, std::uint32_t rows,
                                 std::uint32_t &vertex_buffer, std::uint32_t &primary_index_buffer,
                                 std::uint32_t &secondary_index_buffer, std::uint32_t &triangle_count) = 0;
    virtual void retain_vertex_buffer(std::uint32_t handle) = 0;
    virtual void retain_index_buffer(std::uint32_t handle) = 0;
    virtual void free_staging(std::uintptr_t token) = 0;
    // Assigns the new handle and returns storage valid until the next backend
    // call. The producer initializes every semantic field before returning.
    virtual MaterialCachePacket &allocate_land_packet(std::uint32_t &handle) = 0;
    virtual void release_packet(std::uint32_t handle) = 0;
};
// Complete original 4CE230 orchestration and DX9 785F80/4DE230 packet fields.
// Geometry upload/shared cache and resource ownership are explicit backend
// operations. This function neither completes pending jobs nor draws on a GPU.
// Calls are serialized by the owner; failed resource operations must be
// discarded/recovered by that owner before publishing a completed patch.
void material_cache_rebuild_geometry(MaterialCacheLifecycleState &state,
                                      const MaterialCacheRebuildSettings &settings,
                                      const LandPacketTextures &textures,
                                      MaterialCacheRebuildBackend &backend);
} // namespace civ5::materials
