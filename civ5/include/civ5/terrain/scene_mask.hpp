#pragma once
#include "coast_raster.hpp"
#include "coast_curves.hpp"
#include "terrain_decal_geometry.hpp"
#include <optional>

namespace civ5::terrain {
struct SceneMaskUpdate {
    // Original command 0x20: bounds of all supplied, unscaled points, including
    // vertices unused by the index buffer. Empty input retains +/-FLT_MAX.
    std::array<float,4> bounds;
    std::uint8_t bits;
};
using SceneMaskWriteObserver = std::function<void(std::uint32_t,std::uint8_t)>;
struct ScenePolygonShape {
    Point2 centroid;
    std::vector<std::uint16_t> indices;
};
// Shared original 895770/8954B0 geometry used by admission and decal production.
std::optional<ScenePolygonShape> triangulate_scene_polygon(std::span<const Point2> points,
    CoastBoundsPrecision precision);
struct SceneMaskPolygon {
    Point2 centroid;
    std::vector<Point2> raster_points,expanded_raster_points;
    std::vector<std::uint16_t> indices;
};
// Original 475020/895770/8954B0: triangulate in source order, find the area
// centroid and expand each radial direction by 5 scene units. Both coordinate
// wrappers require the caller's explicit RCPSS estimate for 0.866f. A failed
// original triangulation returns nullopt, including some reversed/degenerate
// polygons; it must not be replaced by a different triangulation algorithm.
// Polygons are supplied by the scene owner, not inferred from coast contours.
std::optional<SceneMaskPolygon> build_scene_mask_polygon(std::span<const Point2> points,
    Point2 offset,CoastBoundsPrecision precision,const ReciprocalEstimate &reciprocal);
class SceneMask {
  public:
    // Original 4A5050: halve source raster dimensions, then round up to 16.
    // A fresh field is zero-filled. This is not a populated saved-scene mask.
    SceneMask(std::int32_t raster_width,std::int32_t raster_height);
    std::int32_t width() const { return width_; }
    std::int32_t height() const { return height_; }
    std::span<const std::uint8_t> flags() const { return flags_; }
    TerrainDecalMapView decal_view(Point2 offset) const { return {width_,height_,flags_,offset.x,offset.y}; }
    // Original 767050/69A8A0 and 7331A0/544640. Coordinates are the caller's
    // uncentered raster XY, scaled by 0.5 before rasterization. X always wraps;
    // Y clips. Each covered sample sets/clears bits while preserving the rest.
    // Returns the invalidation command even for empty/degenerate geometry.
    // Observer includes repeated/unchanged writes and must not mutate this field.
    SceneMaskUpdate indexed(std::span<const Point2> points,std::span<const std::uint16_t> indices,
        std::uint8_t bits,bool setting,CoastBoundsPrecision precision,const SceneMaskWriteObserver &observer={});
    SceneMaskUpdate triangles(std::span<const Point2> points,std::uint8_t bits,bool setting,
        CoastBoundsPrecision precision,const SceneMaskWriteObserver &observer={});
    // Original 475020 always SETS bit 0 on the original polygon, then sets or
    // clears bit 4 on the expanded polygon. Failed triangulation emits nothing.
    // Validates both packets before writing. The two returned commands preserve
    // submission order; the later command consumer remains a separate stage.
    std::vector<SceneMaskUpdate> polygon(std::span<const Point2> points,Point2 offset,bool setting,
        CoastBoundsPrecision precision,const ReciprocalEstimate &reciprocal,
        const SceneMaskWriteObserver &observer={});
  private:
    void raster(Point2 a,Point2 b,Point2 c,std::uint8_t bits,bool setting,
        CoastBoundsPrecision precision,const SceneMaskWriteObserver &observer);
    std::int32_t width_,height_;
    std::vector<std::uint8_t> flags_;
};
struct CoastMaskChunk {
    std::uint32_t first_vertex,vertex_count,first_triangle,triangle_count;
    std::uint8_t bits=9;
};
struct CoastMaskMesh {
    std::vector<Point2> points;
    std::vector<std::uint16_t> indices;
    std::vector<CoastMaskChunk> chunks;
};
// Original 488C50: the band between sampled curves 1 and 4, with local U16
// chunk indices. 5C2A50 selects only owner_flag==0 before calling this producer.
CoastMaskMesh build_coast_mask_mesh(const CoastCurveSet &curves);
// Original post-coast initialization phase of 4A54A0. Later scene writers are
// separate; this sets bits 0 and 3 and does not infer decal-admission bit 4.
SceneMask build_coast_scene_mask(std::span<const CoastCurveSet> curves,
    std::int32_t raster_width,std::int32_t raster_height,CoastBoundsPrecision precision);
} // namespace civ5::terrain
