#include "civ5/terrain/terrain_decal_geometry.hpp"
#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace civ5::terrain {
namespace {
constexpr float y_scale = std::bit_cast<float>(0x3f93ce56u);
constexpr float v_step = std::bit_cast<float>(0x3fa66666u);
void require(bool ok, const char *message) {
    if (!ok) throw std::invalid_argument(message);
}
void validate(TerrainDecalMapView map) {
    require(map.width >= 0 && map.height >= 0 &&
        std::uint64_t(map.width)*std::uint64_t(map.height) == map.flags.size() &&
        std::isfinite(map.offset_x) && std::isfinite(map.offset_y), "Invalid terrain decal mask");
}
// CVTTSS2SI followed by arithmetic shift, including negative fractions that
// truncate to zero and the native indefinite result for conversion overflow.
std::int32_t coordinate(float value) {
    if (!(value >= -0x1p31f && value < 0x1p31f)) return -0x40000000;
    return static_cast<std::int32_t>(value) >> 1;
}
bool enabled(Point2 point, TerrainDecalMapView map) {
    const auto x = coordinate(map.offset_x + point.x);
    const auto y = coordinate((map.offset_y + point.y)*y_scale);
    return x >= 0 && y >= 0 && x < map.width && y < map.height &&
        (map.flags[std::size_t(y)*std::size_t(map.width)+std::size_t(x)] & 0x10) != 0;
}
Point2 normal(Point2 a, Point2 b) {
    const float x = a.y + -b.y, y = -(a.x + -b.x);
    const float squared = y*y + x*x;
    require(std::isfinite(squared) && squared > 0, "Degenerate terrain decal edge");
    // Original spills SSE sum to float, FSQRT, spills to float, then DIVSS.
    const float inverse = 1.f / std::sqrt(squared);
    return {inverse*x, inverse*y};
}
} // namespace
TerrainDecalGeometry build_terrain_decal_geometry(
    std::span<const TerrainDecalPolyline> polylines, TerrainDecalMapView map) {
    std::vector<TerrainDecalPolylineView> views;
    views.reserve(polylines.size());
    for (const auto &line : polylines) views.emplace_back(line);
    return build_terrain_decal_geometry_views(views, map);
}
TerrainDecalGeometry build_terrain_decal_geometry_views(
    std::span<const TerrainDecalPolylineView> polylines, TerrainDecalMapView map) {
    validate(map);
    std::vector<TerrainDecalPolylineView> accepted;
    std::size_t edges = 0, vertices = 0;
    for (const auto &line : polylines) {
        require(!line.empty(), "Empty terrain decal polyline");
        for (const auto p : line)
            require(std::isfinite(p.x) && std::isfinite(p.y), "Nonfinite terrain decal point");
        if (line.size() == 1 ||
            !std::all_of(line.begin(), line.end(), [&](auto p) { return enabled(p, map); })) continue;
        require(line.size()-1 <= 10922-edges, "Terrain decal exceeds native index capacity");
        edges += line.size()-1;
        vertices += line.size()*2;
        accepted.push_back(line);
    }
    TerrainDecalGeometry result;
    result.vertices.reserve(vertices);
    result.indices.reserve(edges*6);
    for (const auto points : accepted) {
        const auto base = result.vertices.size();
        auto previous = normal(points[0], points[1]);
        const auto emit = [&](Point2 p, float nx, float ny, float v) {
            const float dx = nx*.25f, dy = ny*.25f;
            result.vertices.push_back({(p.x+dx)+map.offset_x, ((p.y+dy)+map.offset_y)*y_scale, 0, v});
            result.vertices.push_back({(p.x+-dx)+map.offset_x, ((p.y+-dy)+map.offset_y)*y_scale, 1, v});
        };
        emit(points[0], previous.x, previous.y, 0);
        float v = v_step;
        for (std::size_t i = 1; i+1 < points.size(); ++i) {
            const auto next = normal(points[i], points[i+1]);
            const float x = previous.x+next.x, y = previous.y+next.y;
            const float squared = y*y+x*x;
            require(std::isfinite(squared) && squared > 0, "Degenerate terrain decal join");
            const float inverse = 1.f/std::sqrt(squared);
            emit(points[i], x*inverse, inverse*y, v);
            v += v_step;
            previous = next;
        }
        emit(points.back(), previous.x, previous.y, v);
        for (std::size_t i = 0; i+1 < points.size(); ++i) {
            const auto b = base+i*2;
            for (auto index : {b, b+1, b+2, b+2, b+1, b+3})
                result.indices.push_back(static_cast<std::uint16_t>(index));
        }
    }
    return result;
}
void refresh_terrain_decal_geometry(std::int32_t width, std::int32_t height,
    std::span<TerrainDecalCell> cells, std::span<const TerrainDecalCellGeometry> geometry,
    TerrainDecalMapView map, const TerrainDecalPalette &palette, TerrainRandom &random,
    const TerrainDecalRemove &remove, const TerrainDecalSubmit &submit) {
    validate(map);
    const auto range = palette.ranges[0];
    require(geometry.size() == cells.size() && submit && range.first < palette.records.size() &&
        range.end >= range.first && range.end <= palette.records.size(), "Invalid terrain decal geometry refresh");
    refresh_terrain_decals(width, height, cells, remove, [&](std::size_t index, const TerrainDecalCell &) {
        auto mesh = build_terrain_decal_geometry(geometry[index], map);
        if (mesh.indices.empty()) return absent_terrain_decal;
        const auto selected = select_terrain_decal(range, random);
        return submit(index, selected, palette.records[selected], mesh);
    });
}
} // namespace civ5::terrain
