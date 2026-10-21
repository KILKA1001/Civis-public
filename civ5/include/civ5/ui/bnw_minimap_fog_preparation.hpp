#pragma once

#include "civ5/graphics/bnw_scene_restore_receipt.hpp"
#include "civ5/ui/bnw_client_frame.hpp"
#include "civ5/ui/minimap_render_meshes.hpp"
#include <memory>

namespace civ5::ui {
struct BnwMinimapFogGeometry {
    MinimapCellWindow window{};
    MinimapWorldTransform transform{};
    std::uint32_t edge_subdivisions = 2;
};
struct BnwMinimapFogIdentity {
    BnwClientFrameIdentity client;
    std::uint64_t state_generation = 0, preparation = 0;
    std::string source_grid_sha256, context_sha256, fog_payload_sha256;
};

// CPU preparation only. A consumer must upload/render this exact packet and
// independently retain an applied GPU receipt before accepting presentation.
class BnwMinimapFogPacket {
  public:
    const BnwMinimapFogIdentity &identity() const noexcept { return identity_; }
    const MinimapTileGrid &grid() const noexcept { return grid_; }
    const MinimapRenderLayer &fog_mesh() const noexcept { return fog_mesh_; }
    const BnwMinimapFogGeometry &geometry() const noexcept { return geometry_; }
    std::size_t opened_cells() const noexcept { return opened_; }
    BnwMinimapFogPacket(const BnwMinimapFogPacket &) = delete;
    BnwMinimapFogPacket &operator=(const BnwMinimapFogPacket &) = delete;
  private:
    friend class BnwMinimapFogPreparationOwner;
    BnwMinimapFogPacket(BnwMinimapFogIdentity identity, MinimapTileGrid grid,
        MinimapRenderLayer mesh, BnwMinimapFogGeometry geometry, std::size_t opened);
    const BnwMinimapFogIdentity identity_;
    const MinimapTileGrid grid_;
    const MinimapRenderLayer fog_mesh_;
    const BnwMinimapFogGeometry geometry_;
    const std::size_t opened_;
};

// Single-thread borrowed owner. Session and client must outlive it. State
// replacement, including identical restore, requires a new preparation owner.
// initial_grid must be the unmodified project_saved_minimap_terrain result;
// preserve its existing callback/sentinel semantics on each private replay.
class BnwMinimapFogPreparationOwner {
  public:
    BnwMinimapFogPreparationOwner(const runtime::LiveSession &session,
        const BnwClientFrameOwner &client, graphics::BnwRendererSceneReceipt scene,
        MinimapTileGrid initial_grid, BnwMinimapFogGeometry geometry);
    ~BnwMinimapFogPreparationOwner();
    BnwMinimapFogPreparationOwner(const BnwMinimapFogPreparationOwner &) = delete;
    BnwMinimapFogPreparationOwner &operator=(const BnwMinimapFogPreparationOwner &) = delete;
    std::shared_ptr<const BnwMinimapFogPacket> prepare(
        const std::shared_ptr<const BnwClientFrame> &frame);
    bool current(const std::shared_ptr<const BnwMinimapFogPacket> &packet) const noexcept;
    // Declared source compatibility, not an independent native/GPU base proof.
    bool source_context_matches(const graphics::BnwRendererSceneReceipt &scene,
        const BnwMinimapFogGeometry &geometry) const noexcept;
  private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
} // namespace civ5::ui
