#include "civ5/graphics/gl_land_patch_resources.hpp"
#include <algorithm>
#include <cstring>
#include <exception>
#include <optional>
#include <stdexcept>

namespace civ5::graphics {
namespace {
using namespace materials;
void require(bool ok, const char *why) {
  if (!ok)
    throw std::invalid_argument(why);
}
} // namespace
struct GlLandPatchResources::Impl final : MaterialCacheRebuildBackend,
                                          MaterialCacheLifecycleBackend {
  std::shared_ptr<GlLandGeometryPool> geometry_pool;
  std::shared_ptr<MaterialPacketPool> packet_pool;
  MaterialCacheLifecycleState state;
  MaterialCacheRebuildSettings settings{};
  LandPacketTextures textures{};
  std::int32_t step = 1;
  MaterialCacheStagingBackend *staging = nullptr;
  const std::function<void(MaterialCacheLifecycleState &, std::uint32_t,
                           std::uintptr_t)> *generate = nullptr;
  std::optional<GlLandGeometryLease> lease;
  std::array<std::uint32_t, 5> owned_packets{};
  unsigned retained = 0;
  bool ready = false;
  Impl(std::shared_ptr<GlLandGeometryPool> geometry,
       std::shared_ptr<MaterialPacketPool> packets)
      : geometry_pool(std::move(geometry)), packet_pool(std::move(packets)) {}
  void discard() {
    ready = false;
    if (lease) {
      geometry_pool->release(*lease);
      lease.reset();
    }
    retained = 0;
    for (auto &handle : owned_packets) {
      packet_pool->release(handle);
      handle = 0;
    }
    auto &g = state.geometry;
    g.vertex_buffer = g.primary_index_buffer = g.secondary_index_buffer = 0;
    g.packets = {};
  }
  ~Impl() {
    try {
      if (ready)
        invalidate();
      discard();
    } catch (...) {
      std::terminate();
    }
  }
  void invalidate() {
    ready = false;
    material_cache_invalidate_geometry(state, *this,
                                       settings.separate_geometry_buffers);
  }
  void rebuild_geometry(MaterialCacheLifecycleState &s) override {
    require(&s == &state, "Foreign patch rebuild state");
    material_cache_rebuild_geometry(state, settings, textures, *this);
    ready = true;
  }
  void generate_cache(MaterialCacheLifecycleState &s, std::uint32_t requested,
                      std::uintptr_t argument) override {
    require(generate && &s == &state,
            "Material generation requires its world job owner");
    (*generate)(s, requested, argument);
  }
  void upload_geometry_pair(std::uintptr_t vertices, std::uint32_t nv,
                            std::uintptr_t indices, std::uint32_t ni,
                            std::uint32_t &vb, std::uint32_t &ib) override {
    require(staging && !lease,
            "Paired rebuild needs a staging owner and an empty geometry slot");
    require(nv && ni, "Paired rebuild requires nonempty geometry streams");
    const auto source_vertices = staging->staging_memory(vertices),
               source_indices = staging->staging_memory(indices);
    require(source_vertices.size() >= std::size_t(nv) * 24 &&
                source_indices.size() >= std::size_t(ni) * 2,
            "Owned geometry staging extent");
    std::vector<terrain::LandGridVertex> v(nv);
    std::vector<std::uint16_t> i(ni);
    std::memcpy(v.data(), source_vertices.data(), v.size() * sizeof(v[0]));
    std::memcpy(i.data(), source_indices.data(), i.size() * sizeof(i[0]));
    const LandMaterialMesh mesh(LandVertexInput::source_positioned, v, i);
    lease = geometry_pool->upload(mesh);
    vb = lease->vertex_buffer();
    ib = lease->index_buffer();
  }
  void shared_geometry(std::uint32_t columns, std::uint32_t rows,
                       std::uint32_t &vb, std::uint32_t &ib,
                       std::uint32_t &control,
                       std::uint32_t &triangles) override {
    require(!lease, "Shared rebuild requires an empty geometry slot");
    lease = geometry_pool->begin_shared_grid(columns, rows, step,
                                             settings.capability);
    vb = lease->vertex_buffer();
    ib = lease->index_buffer();
    control = lease->control_buffer();
    triangles = lease->index_count() / 3;
  }
  void retain_vertex_buffer(std::uint32_t handle) override {
    require(lease && handle == lease->vertex_buffer(),
            "Retained vertex is not owned by this patch");
    geometry_pool->retain_shared_buffer(*lease, handle);
    retained |= 1;
  }
  void retain_index_buffer(std::uint32_t handle) override {
    if (!handle)
      return;
    require(lease && (handle == lease->index_buffer() ||
                      handle == lease->control_buffer()),
            "Retained index is not owned by this patch");
    geometry_pool->retain_shared_buffer(*lease, handle);
    retained |= handle == lease->index_buffer() ? 2u : 4u;
  }
  void release_geometry_pair(std::uint32_t vb, std::uint32_t ib) override {
    if (!vb && !ib)
      return;
    require(lease && !settings.separate_geometry_buffers &&
                vb == lease->vertex_buffer() && ib == lease->index_buffer(),
            "Returned geometry pair is not owned by this patch");
    geometry_pool->release(*lease);
    ready = false;
    lease.reset();
  }
  void release_vertex_buffer(std::uint32_t handle) override {
    if (!handle)
      return;
    require(lease && settings.separate_geometry_buffers &&
                handle == lease->vertex_buffer(),
            "Foreign vertex return");
    geometry_pool->release_shared_buffer(*lease, handle);
    ready = false;
    retained &= ~1u;
    if (!retained)
      lease.reset();
  }
  void release_index_buffer(std::uint32_t handle) override {
    if (!handle)
      return;
    require(lease && settings.separate_geometry_buffers &&
                (handle == lease->index_buffer() ||
                 handle == lease->control_buffer()),
            "Foreign index return");
    const auto mask = handle == lease->index_buffer() ? 2u : 4u;
    geometry_pool->release_shared_buffer(*lease, handle);
    ready = false;
    retained &= ~mask;
    if (!retained)
      lease.reset();
  }
  MaterialCachePacket &allocate_land_packet(std::uint32_t &handle) override {
    unsigned slot = 0;
    while (slot < 5 && &handle != &state.geometry.packets[slot])
      ++slot;
    require(slot < 5 && !owned_packets[slot] && !handle,
            "Packet slot must be empty and belong to this patch");
    auto &packet = packet_pool->allocate(0x49, handle);
    owned_packets[slot] = handle;
    return packet;
  }
  void release_packet(std::uint32_t handle) override {
    if (!handle)
      return;
    const auto at =
        std::find(owned_packets.begin(), owned_packets.end(), handle);
    require(at != owned_packets.end(), "Packet does not belong to this patch");
    packet_pool->release(handle);
    *at = 0;
  }
  void free_staging(std::uintptr_t token) override {
    if (!token)
      return;
    require(staging, "Geometry staging release requires its owner");
    staging->free_staging(token);
  }
};
GlLandPatchResources::GlLandPatchResources(
    std::shared_ptr<GlLandGeometryPool> geometry,
    std::shared_ptr<MaterialPacketPool> packets) {
  require(geometry && packets,
          "Patch resources require geometry and packet owners");
  impl_ = std::make_unique<Impl>(std::move(geometry), std::move(packets));
}
GlLandPatchResources::~GlLandPatchResources() = default;
MaterialCacheLifecycleState &GlLandPatchResources::state() {
  return impl_->state;
}
const MaterialCacheLifecycleState &GlLandPatchResources::state() const {
  return impl_->state;
}
void GlLandPatchResources::rebuild(const MaterialCacheRebuildSettings &settings,
                                   const LandPacketTextures &textures,
                                   std::int32_t step,
                                   MaterialCacheStagingBackend *staging) {
  auto &s = *impl_;
  require(!s.lease && !s.ready &&
              std::all_of(s.owned_packets.begin(), s.owned_packets.begin() + 4,
                          [](auto h) { return h == 0; }),
          "Invalidate patch geometry before rebuilding");
  require(settings.separate_geometry_buffers || staging,
          "CPU geometry requires its actual staging owner");
  s.settings = settings;
  s.textures = textures;
  s.step = step;
  s.staging = staging;
  try {
    s.rebuild_geometry(s.state);
    s.ready = true;
    s.staging = nullptr;
  } catch (...) {
    s.staging = nullptr;
    s.discard();
    throw;
  }
}
void GlLandPatchResources::invalidate() { impl_->invalidate(); }
bool GlLandPatchResources::complete(
    MaterialCacheTexturePool &pool,
    const MaterialCacheRebuildSettings &settings,
    const LandPacketTextures &textures, std::int32_t step, std::uint32_t stamp,
    std::uintptr_t argument,
    const std::function<void(MaterialCacheLifecycleState &, std::uint32_t,
                             std::uintptr_t)> &generate,
    MaterialCacheStagingBackend *staging) {
  auto &s = *impl_;
  require(!s.generate && (settings.separate_geometry_buffers || staging),
          "Patch completion requires exclusive generation and a geometry "
          "staging owner");
  s.settings = settings;
  s.textures = textures;
  s.step = step;
  s.staging = staging;
  s.generate = &generate;
  try {
    const auto result = material_cache_dispatch(
        s.state, pool, s, settings.separate_geometry_buffers, stamp, argument);
    s.generate = nullptr;
    s.staging = nullptr;
    return result;
  } catch (...) {
    s.generate = nullptr;
    s.staging = nullptr;
    s.discard();
    throw;
  }
}
void GlLandPatchResources::downgrade_cache(MaterialCacheTexturePool &pool) {
  auto &s = *impl_;
  if (s.state.completed_state != 1)
    s.ready = false;
  material_cache_downgrade(s.state, pool, s,
                           s.settings.separate_geometry_buffers);
}
void GlLandPatchResources::release_cache(MaterialCacheTexturePool &pool,
                                         bool geometry, bool final,
                                         bool higher) {
  auto &s = *impl_;
  if (geometry)
    s.ready = false;
  material_cache_release(s.state, pool, s, s.settings.separate_geometry_buffers,
                         geometry, final, higher);
}
const GlLandGeometryLease &GlLandPatchResources::geometry() const {
  require(impl_->ready && impl_->lease.has_value(),
          "Patch geometry has not been rebuilt");
  return *impl_->lease;
}
const MaterialCachePacket &GlLandPatchResources::packet(unsigned slot) const {
  require(slot < 5 &&
              impl_->owned_packets[slot] == impl_->state.geometry.packets[slot],
          "Invalid patch packet slot");
  return impl_->packet_pool->packet(impl_->owned_packets[slot]);
}
} // namespace civ5::graphics
