#pragma once
#include "scene_direction.hpp"
#include "scene_mask.hpp"

namespace civ5::terrain {
struct ScenePolygonDecalPalette {
    // Original owner +24..+40 stores eight packed U16 start/count pairs.
    // These slots are not the ten TerrainDecalAssets XML families.
    std::array<TerrainDecalRange,8> ranges{};
    std::vector<TerrainDecalRecord> records;
};
struct ScenePolygonDecalState {
    std::uint32_t flags;              // original plot record +0C
    std::uint8_t type, enabled;        // original plot record +04/+07
    std::array<std::uint32_t,2> handles; // original polygon record +24/+28
};
using ScenePolygonDecalSubmit=std::function<std::uint32_t(unsigned,std::uint32_t,
    const TerrainDecalRecord &,const TerrainDecalGeometry &)>;
struct ScenePolygonDecalUpdate {
    std::vector<SceneMaskUpdate> mask_updates;
    std::optional<float> texture_angle;
    unsigned submitted=0;
};
// Complete original 68B640 production, through its two possible 69E140 calls.
// Sets admission before building the mesh. A successful triangulation always
// consumes the orientation draw, even if state/range gates suppress submission.
// Each selected material consumes another rand, including singleton ranges.
// Submit's first argument is output slot 0/1; both use native store mode zero.
// Handles change only after successful synchronous submission. Suppressed draws
// retain prior handles; removal and source polygon/event ownership are upstream.
// Callbacks must not mutate/reenter the mask, source state, points, palette or
// random stream. Failure can retain mask writes, RNG progress and prior submits.
ScenePolygonDecalUpdate update_scene_polygon_decals(std::span<const Point2> points,
    ScenePolygonDecalState &state,SceneMask &mask,Point2 world_offset,
    const SceneDirectionField &direction,const ScenePolygonDecalPalette &palette,
    TerrainRandom &random,CoastBoundsPrecision precision,Vec4Path transform,
    const ReciprocalEstimate &reciprocal,const ScenePolygonDecalSubmit &submit,
    const SceneMaskWriteObserver &mask_observer={});
} // namespace civ5::terrain
