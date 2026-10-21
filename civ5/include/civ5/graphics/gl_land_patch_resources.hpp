#pragma once
#include "civ5/materials/material_cache_preparation.hpp"
#include "civ5/materials/material_cache_rebuild.hpp"
#include "civ5/materials/material_packet_pool.hpp"
#include "gl_land_geometry_pool.hpp"
#include <functional>

namespace civ5::graphics {
// One patch owns its geometry reference and original LAND packets. Shared
// buffers and packet storage outlive the patch through their retained owners.
// CPU rebuilds borrow the actual staging owner only for the synchronous upload
// and original free callbacks. Source staging is never replaced by expected
// data.
class GlLandPatchResources {
public:
  GlLandPatchResources(std::shared_ptr<GlLandGeometryPool>,
                       std::shared_ptr<materials::MaterialPacketPool>);
  ~GlLandPatchResources();
  GlLandPatchResources(const GlLandPatchResources &) = delete;
  GlLandPatchResources &operator=(const GlLandPatchResources &) = delete;
  materials::MaterialCacheLifecycleState &state();
  const materials::MaterialCacheLifecycleState &state() const;
  // Requires the preceding geometry to have been invalidated. The ordinary
  // invalidator retains the optional overlay packet, as in the original.
  void rebuild(const materials::MaterialCacheRebuildSettings &,
               const materials::LandPacketTextures &,
               std::int32_t geometry_step,
               materials::MaterialCacheStagingBackend *staging = nullptr);
  void invalidate();
  // Complete a prepared job on this stable owner. The generation callback is
  // borrowed only for this call; geometry and packet callbacks use the same
  // owned resources as rebuild/invalidate.
  bool
  complete(materials::MaterialCacheTexturePool &pool,
           const materials::MaterialCacheRebuildSettings &settings,
           const materials::LandPacketTextures &textures,
           std::int32_t geometry_step, std::uint32_t frame_stamp,
           std::uintptr_t generation_argument,
           const std::function<void(materials::MaterialCacheLifecycleState &,
                                    std::uint32_t, std::uintptr_t)> &generate,
           materials::MaterialCacheStagingBackend *staging = nullptr);
  void downgrade_cache(materials::MaterialCacheTexturePool &pool);
  void release_cache(materials::MaterialCacheTexturePool &pool, bool geometry,
                     bool final, bool higher);
  const GlLandGeometryLease &geometry() const;
  const materials::MaterialCachePacket &packet(unsigned slot) const;

private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};
} // namespace civ5::graphics
