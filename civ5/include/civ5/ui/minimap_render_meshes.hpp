#pragma once

#include "civ5/ui/minimap_draw_batches.hpp"
#include "civ5/ui/minimap_masked_hex_mesh.hpp"
#include "civ5/ui/minimap_plain_hex_mesh.hpp"
#include "civ5/ui/minimap_visible_bounds.hpp"
#include <cstddef>
#include <cstdint>
#include <span>
#include <stdexcept>
#include <vector>

namespace civ5::ui {

// DrawArrays-ready world positions, three vertices per triangle. The native
// minimap uses separate ordered draw groups before its final fog pass.
struct MinimapRenderLayer {
    std::vector<MinimapPoint2> triangles;
    std::size_t triangle_count() const { return triangles.size() / 3; }
};

struct MinimapRenderCultureLayer {
    std::uint8_t payload;
    MinimapRenderLayer mesh;
};

struct MinimapRenderMeshes {
    MinimapVisibleBounds visible;
    MinimapRenderLayer coast, land, fog;
    std::vector<MinimapRenderCultureLayer> culture;
};

// The same draw-cell traversal and native plain/masked tessellation used by
// the saved-map export now produce reusable render geometry. The lookup is
// where actual terrain-edge records enter; a missing record invokes the
// original synthetic fallback rather than inventing an edge shape here.
template <class TerrainEdgeLookup>
MinimapRenderMeshes build_minimap_render_meshes(
    const MinimapTileGrid &grid, MinimapCellWindow window,
    const MinimapWorldTransform &transform, MinimapVisibleBounds visible,
    TerrainEdgeLookup &&terrain_edge_lookup) {
    if (visible.edge_subdivisions < 2 || visible.segment_count == 0 ||
        visible.segment_count > visible.segments.size())
        throw std::invalid_argument("Invalid minimap render bounds");
    const auto batches = build_minimap_draw_batches(grid, window);
    MinimapRenderMeshes result{};
    result.visible = visible;
    const auto append = [&](MinimapRenderLayer &layer, const auto &mesh) {
        if (mesh.indices.size() % 3 != 0)
            throw std::invalid_argument("Minimap tessellation is not triangles");
        for (const auto index : mesh.indices)
            layer.triangles.push_back(mesh.vertices.at(index));
    };
    const auto emit = [&](MinimapRenderLayer &layer,
                          const std::vector<MinimapDrawCell> &cells, bool fog) {
        for (const auto &cell : cells) {
            if (cell.edge_mask == 0) {
                append(layer, build_minimap_plain_hex_mesh(
                                  std::span<const MinimapDrawCell>(&cell, 1), transform));
            } else if (fog) {
                append(layer, build_minimap_synthetic_masked_hex_mesh(
                                  cell, transform, visible.edge_subdivisions));
            } else {
                append(layer, build_minimap_terrain_masked_hex_mesh(
                                  cell, transform, visible.edge_subdivisions,
                                  terrain_edge_lookup));
            }
        }
    };
    emit(result.coast, batches.coast, false);
    emit(result.land, batches.land, false);
    for (const auto &batch : batches.culture) {
        result.culture.push_back({batch.payload, {}});
        emit(result.culture.back().mesh, batch.cells, false);
    }
    emit(result.fog, batches.fog, true);
    return result;
}

} // namespace civ5::ui
