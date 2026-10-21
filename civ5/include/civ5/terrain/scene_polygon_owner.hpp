#pragma once
#include "scene_polygon_chunks.hpp"
#include "scene_polygon_plot_store.hpp"
#include "native_math.hpp"
#include <array>
#include <cstdint>
#include <functional>
#include <list>
#include <span>
#include <unordered_map>
#include <vector>

namespace civ5::terrain {
struct ScenePolygonEvent {
    std::array<std::int32_t,3> position{};
    std::uint32_t polygon_id=0;
    std::uint32_t opaque_word=0; // original event +0x10, retained by deferred copies
    ScenePolygonPointChunk chunk;
};
struct ScenePolygonOwnerGrid {
    // Original 423350 integer matrices, indexed as the original 4x4 rows.
    // +5B0 gates 5750F0 polygon events; +700 addresses tiles for polygon
    // events, plot insertion/removal, neighbor reapplication and dirty bounds.
    std::array<std::int32_t,16> bounds_matrix{},tile_matrix{};
    std::int32_t width=0,height=0;
    // 7A0C10 uses a fixed 256-tile row stride after matrix conversion. The
    // caller keeps this source array alive for the lifetime of the owner.
    std::span<const std::uint16_t> tile_ids;
    // Optional original plot-slot owner. The caller keeps it alive and uses
    // insert_cell/remove_cell_using_plot for scene event lifecycle updates.
    ScenePolygonPlotStore *plot_store=nullptr;
};
enum class ScenePolygonEventResult { rejected,deferred,pending,completed };
struct ScenePolygonOwnerRecord {
    std::uint32_t polygon_id;
    std::uint16_t tile_id;
    ScenePolygonPointAssembly assembly;
    std::array<std::uint32_t,2> handles{0xffffffffu,0xffffffffu};
    bool active=true;
};
struct ScenePolygonNeighborReapply {
    std::uint16_t tile_id;
    std::uint16_t record_index;
    std::uint16_t slot=0;
    bool operator==(const ScenePolygonNeighborReapply &) const = default;
};
using ScenePolygonOwnerSubmit=std::function<void(std::uint16_t,std::uint32_t,
    std::uint32_t,std::span<const Point2>)>;
using ScenePolygonHandleRemove=std::function<void(std::uint32_t)>;
using ScenePolygonMaskApply=std::function<void(std::uint16_t,std::uint32_t,
    std::uint32_t,bool,std::span<const Point2>)>;
// Source plot +10/+12 and global D80E80/D80E1C/D80E20 projection inputs.
// They are supplied by the scene owner; polygon event coordinates need not be
// the same as the retained plot coordinates.
struct ScenePolygonDirtyProjection {
    std::array<std::int16_t,2> plot_coordinates{};
    Matrix4 world_matrix{};
    std::array<float,2> world_offset{};
};
using ScenePolygonDirty=std::function<void(std::array<float,4>)>;
using ScenePolygonCellNotify=std::function<void(std::uint16_t)>;
struct ScenePolygonRemovalResult {
    bool found=false;
    std::uint16_t tile_id=0xffffu;
    std::size_t removed=0,reapplied=0;
};
enum class ScenePolygonPlotEventAction { rejected, removed, inserted, replaced };
struct ScenePolygonPlotEventResult {
    ScenePolygonPlotEventAction action=ScenePolygonPlotEventAction::rejected;
    std::uint16_t tile_id=0xffffu;
};

// Original 5750F0 event owner. The source grid and events are supplied by the
// caller; this class does not infer saved-scene polygons from rendered terrain.
// The submission callback can enter 68B640's port and the world GPU owner.
// Callbacks must not reenter or mutate this owner and must complete normally.
class ScenePolygonOwner {
  public:
    explicit ScenePolygonOwner(ScenePolygonOwnerGrid grid);
    ScenePolygonEventResult accept(const ScenePolygonEvent &event,
        const ScenePolygonOwnerSubmit &submit={},bool defer=false);
    // Replay the owner's queued polygon commands in arrival order. The source
    // dispatcher clears its defer flag before invoking stored callbacks.
    std::size_t drain_deferred(const ScenePolygonOwnerSubmit &submit={});
    std::span<const ScenePolygonOwnerRecord> records() const { return records_; }
    void set_record_handles(std::uint32_t record_index,
        std::array<std::uint32_t,2> handles);
    ScenePolygonRemovalResult remove_cell(const std::array<std::int32_t,3> &position,
        const ScenePolygonDirtyProjection &projection,
        const ScenePolygonHandleRemove &remove_handle,
        const ScenePolygonMaskApply &apply_mask,
        const ScenePolygonDirty &dirty);
    // Use the retained plot's +10/+12 coordinates, rather than event cube
    // coordinates, when the scene supplies original plot records.
    ScenePolygonRemovalResult remove_cell_using_plot(
        const std::array<std::int32_t,3> &position,const Matrix4 &world_matrix,
        std::array<float,2> world_offset,const ScenePolygonHandleRemove &remove_handle,
        const ScenePolygonMaskApply &apply_mask,const ScenePolygonDirty &dirty);
    // Original 5415F0 -> 681700: allocate/reuse a 144-byte plot slot, then
    // publish its U16 index into the fixed-stride tile grid.
    std::uint16_t insert_cell(const std::array<std::int32_t,3> &position,
        const ScenePolygonPlotBytes &source);
    // Synchronous 5415F0 lifecycle: reject an empty cell with the wrong gate;
    // when occupied, remove its old plot before checking the new event's gate.
    // Material selection and 5ACEC0 cell notification are caller boundaries.
    ScenePolygonPlotEventResult apply_plot_source_event(
        const ScenePolygonPlotSourceEvent &event,std::uint32_t scene_identity,
        std::uint32_t scene_gate,std::uint32_t variation_count,
        const ScenePolygonRandomDraw &random,
        const ScenePolygonMaterialResolve &resolve_material,
        const Matrix4 &world_matrix,std::array<float,2> world_offset,
        const ScenePolygonHandleRemove &remove_handle,
        const ScenePolygonMaskApply &apply_mask,const ScenePolygonDirty &dirty,
        const ScenePolygonCellNotify &notify);
    std::size_t active_record_count() const { return records_.size()-free_indices_.size(); }
    std::span<const std::uint16_t> plot(std::uint16_t tile_id) const;
    // 4A9750 walks six adjacent cube cells in source order after removing a
    // plot. Preserve duplicate tile IDs and each plot's polygon order. The
    // supplied grid dimensions must match the source world's removal bounds.
    std::vector<ScenePolygonNeighborReapply> neighbor_reapplications(
        const std::array<std::int32_t,3> &position) const;
    std::span<const ScenePolygonEvent> deferred_events() const { return deferred_; }
    std::size_t pending_count() const { return pending_.size(); }
    std::uint32_t completed_count() const { return completed_count_; }
  private:
    ScenePolygonOwnerGrid grid_;
    std::vector<ScenePolygonOwnerRecord> records_;
    std::vector<ScenePolygonEvent> deferred_;
    std::vector<std::uint32_t> free_indices_;
    std::list<std::uint32_t> pending_;
    std::unordered_map<std::uint16_t,std::vector<std::uint16_t>> plots_;
    std::unordered_map<std::size_t,std::uint16_t> tile_overrides_;
    std::uint32_t completed_count_=0;
    std::uint16_t tile_at(std::size_t offset) const;
    std::array<float,4> dirty_region(const std::array<std::int32_t,3> &position,
        const ScenePolygonDirtyProjection &projection) const;
    void finish(std::uint16_t tile_id,std::uint32_t record_index,
        const ScenePolygonOwnerSubmit &submit);
};
} // namespace civ5::terrain
