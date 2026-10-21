#pragma once
#include "fog_of_war.hpp"
#include "civ5/terrain/world_layout.hpp"
#include "civ5/terrain/height_owner.hpp"
#include "civ5/terrain/overlay.hpp"
#include <span>

namespace civ5::materials {
struct FogOfWarView {
    terrain::Matrix4 view, overlay;
    // x begin/end, y begin/end; the mask uses exclusive upper limits.
    std::array<std::int32_t, 4> tiles;
    // Ground bounds: minimum/maximum x, minimum/maximum y.
    std::array<float, 4> bounds;
};
struct FogOfWarCamera {
    // Original height owner +B4/+B8 and +A8/+AC. These are map-center and
    // source-scale values, independent of the current camera position.
    std::array<float, 2> height_center;
    std::array<float, 2> height_scale;
    terrain::Matrix4 projection;
};
enum class FogOfWarPassKind : std::uint32_t { mask, culling, clouds };
struct FogOfWarDraw {
    // Index in fog_of_war_draw_records(): two masks, three clouds, culling quad.
    std::uint32_t geometry, selector;
    std::vector<terrain::Vector4> constants;
};
struct FogOfWarPass {
    FogOfWarPassKind kind;
    std::uint32_t begin, end;
    terrain::Vector4 clear_color;
};
struct FogOfWarFrame {
    std::vector<FogOfWarPass> passes;
    std::vector<FogOfWarDraw> draws;
};
// Original 7A47B0, 60A830 and 5D42C0: batches of 24, original traversal,
// 2048-matrix capacity, and zero matrix tails. Camera and view state belong
// to the caller; this function does not invent a camera or visibility policy.
FogOfWarFrame fog_of_war_frame(const terrain::WorldLayout &world, const FogOfWarState &state,
                               const FogOfWarCamera &camera, std::span<const FogOfWarView> views,
                               std::uint32_t quality, bool enabled, bool clear_mask, bool override_enabled,
                               terrain::X87Precision precision);
struct FogOfWarSceneFrame {
    terrain::OverlayMatrices overlays;
    FogOfWarFrame commands;
};
// Connect the original camera/bounds and height owner to the overlay and FOW
// producers. Original FOW uses clipped X bounds but raw Y bounds, including
// tile intervals. Visibility and animation remain owned by FogOfWarState.
FogOfWarSceneFrame fog_of_war_scene_frame(const terrain::WorldLayout &world,
                                          const terrain::HeightOwnerParameters &height,
                                          const FogOfWarState &state, const terrain::CameraFrameState &camera,
                                          const terrain::ViewBounds &bounds,
                                          const terrain::OverlayMatrices &prior_overlays,
                                          std::uint32_t quality, bool enabled, bool clear_mask,
                                          bool override_enabled, terrain::X87Precision precision);
} // namespace civ5::materials
