#pragma once

#include "civ5/terrain/terrain_world.hpp"
#include "civ5/ui/minimap_edge_control_points.hpp"
#include <array>
#include <bit>
#include <cmath>
#include <cstdint>
#include <map>
#include <optional>
#include <span>
#include <stdexcept>
#include <tuple>
#include <vector>

namespace civ5::ui {

// The pairs passed by retail 0x460870 to 0x7A3370, before that routine's
// source-map bounds guards and record-table writes. Point coordinates depend
// on the caller-supplied coast CRT cursor; they are not recovered save data.
struct MinimapCoastEdgeCandidate {
    std::array<std::int32_t, 3> worker_cube;
    std::array<std::int32_t, 3> table_cube;
    std::uint8_t direction;
    std::array<MinimapPoint2, 2> world_points;
    std::uint32_t contour_index, pair_index;
};

namespace detail {
inline std::array<std::int32_t, 3> coast_transform_cube(
    std::array<std::int32_t, 3> cube, const terrain::IntMatrix4 &matrix) {
    std::array<std::int32_t, 3> result{};
    for (std::size_t lane = 0; lane < 3; ++lane) {
        std::uint32_t sum = static_cast<std::uint32_t>(matrix[12 + lane]);
        for (std::size_t column = 0; column < 3; ++column)
            sum += static_cast<std::uint32_t>(cube[column]) *
                   static_cast<std::uint32_t>(matrix[4 * column + lane]);
        result[lane] = std::bit_cast<std::int32_t>(sum);
    }
    return result;
}

inline std::array<std::int32_t, 3> coast_offset_cube(
    std::int32_t x, std::int32_t y, const terrain::IntMatrix4 &matrix) {
    const auto q = std::bit_cast<std::int32_t>(
        static_cast<std::uint32_t>(x) - static_cast<std::uint32_t>(y >> 1));
    auto cube = coast_transform_cube(
        {q, y, std::bit_cast<std::int32_t>(
                   0u - static_cast<std::uint32_t>(q) - static_cast<std::uint32_t>(y))},
        matrix);
    cube[2] = std::bit_cast<std::int32_t>(
        0u - static_cast<std::uint32_t>(cube[0]) - static_cast<std::uint32_t>(cube[1]));
    return cube;
}

inline std::array<std::int32_t, 3> coast_normalize_source_cube(
    std::array<std::int32_t, 3> cube, const terrain::WorldGrid &source) {
    if (source.width <= 0 || source.height <= 0)
        throw std::invalid_argument("Invalid minimap coast source grid");
    const auto offset = coast_transform_cube(cube, source.first_matrix);
    // 0x77D500 adds one period before signed IDIV, including non-wrapped maps.
    const auto x = std::bit_cast<std::int32_t>(
        static_cast<std::uint32_t>(offset[0]) +
        static_cast<std::uint32_t>(offset[1] >> 1) +
        static_cast<std::uint32_t>(source.width)) % source.width;
    const auto y = std::bit_cast<std::int32_t>(
        static_cast<std::uint32_t>(offset[1]) +
        static_cast<std::uint32_t>(source.height)) % source.height;
    return coast_offset_cube(x, y, source.second_matrix);
}

inline std::int32_t coast_wrap_period(std::int32_t x, std::int32_t width) {
    return x / width - (x % width < 0 ? 1 : 0);
}
} // namespace detail

inline std::vector<MinimapCoastEdgeCandidate> build_minimap_coast_edge_candidates(
    const terrain::WorldLayout &layout, const terrain::TerrainWorldCoast &coast) {
    const auto &worker = layout.worker_grid;
    const auto &contours = coast.contours;
    const auto &perturbations = coast.perturbations;
    const auto &links = coast.graph.links();
    if (worker.width <= 0 || worker.height <= 0 ||
        contours.size() != perturbations.size() ||
        links.size() != std::size_t(worker.width) * worker.height ||
        coast.smoothed.width() != worker.width ||
        coast.smoothed.height() != worker.height)
        throw std::invalid_argument("Minimap coast topology differs from worker grid");
    std::vector<MinimapCoastEdgeCandidate> result;
    for (std::size_t contour_index = 0; contour_index < contours.size(); ++contour_index) {
        const auto &contour = contours[contour_index];
        if (contour.owner_flag != 0)
            continue; // 0x460878 skips other contour owners.
        const auto &points = perturbations[contour_index].points;
        if (points.size() != contour.points.size() || points.size() < 4 ||
            points.size() % 2 != 0 ||
            contour.start_x < 0 || contour.start_x >= worker.width ||
            contour.start_y < 0 || contour.start_y >= worker.height)
            throw std::invalid_argument("Invalid minimap coast contour");
        auto x = contour.start_x, y = contour.start_y, edge = contour.start_edge;
        float wrap_shift = 0.0f;
        for (std::size_t pair = 0; pair < points.size() / 2; ++pair) {
            const auto cell = std::size_t(y) * worker.width + x;
            if (edge < 0 || std::size_t(edge) >= links.at(cell).size())
                throw std::invalid_argument("Minimap coast edge left graph");
            const auto direction = coast.smoothed.cells()[cell].segments.at(edge).direction;
            if (direction >= 12)
                throw std::invalid_argument("Minimap coast owner direction exceeds record slots");
            const auto cube = detail::coast_offset_cube(x, y, worker.second_matrix);
            const std::array<MinimapPoint2, 2> xy{{
                {points[2 * pair].x - wrap_shift, points[2 * pair].y},
                {points[2 * pair + 1].x - wrap_shift, points[2 * pair + 1].y}}};
            result.push_back({cube, detail::coast_normalize_source_cube(cube, layout.source_grid),
                              direction, xy, static_cast<std::uint32_t>(contour_index),
                              static_cast<std::uint32_t>(pair)});
            if (pair + 1 == points.size() / 2)
                break;
            const auto next = links[cell][edge].forward;
            if (next.edge < 0 || next.y < 0 || next.y >= worker.height)
                throw std::invalid_argument("Minimap coast traversal has no next edge");
            wrap_shift += float(detail::coast_wrap_period(next.x, worker.width)) *
                          coast.smoothed.wrap_delta().x;
            x = next.x % worker.width;
            if (x < 0) x += worker.width;
            y = next.y;
            edge = next.edge;
        }
    }
    return result;
}

// 0x7A3370 asks 0x52C250 whether the neighboring source row is outside the
// map. On a non-wrapping map it also asks 0x46FC50 about the source column.
// Both guards normalize the input cube first and then step in direction % 6.
inline bool minimap_coast_edge_registration_allowed(
    const MinimapCoastEdgeCandidate &candidate,
    const terrain::WorldGrid &source, bool wrap_x) {
    if (source.width <= 0 || source.height <= 0 || candidate.direction >= 12)
        throw std::invalid_argument("Invalid minimap coast registration input");
    auto neighbor = candidate.table_cube;
    const auto &step = minimap_neighbor_cube_steps[candidate.direction % 6];
    for (std::size_t lane = 0; lane < 3; ++lane)
        neighbor[lane] = std::bit_cast<std::int32_t>(
            static_cast<std::uint32_t>(neighbor[lane]) +
            static_cast<std::uint32_t>(step[lane]));
    const auto logical = detail::coast_transform_cube(neighbor, source.first_matrix);
    if (logical[1] < 0 || logical[1] >= source.height)
        return false;
    const auto x = std::bit_cast<std::int32_t>(
        static_cast<std::uint32_t>(logical[0]) +
        static_cast<std::uint32_t>(logical[1] >> 1));
    return wrap_x || (x >= 0 && x < source.width);
}

// Diagnostic form of 0x7A3370 -> 0x424BD0: accepted coast pairs register in
// forward and reverse slots, and a later write replaces an earlier slot.
// The record is relative to the incoming unwrapped worker cube. Only its
// address is normalized for table storage/lookup.
class MinimapCoastEdgeRecordTable {
  public:
    static MinimapCoastEdgeRecordTable build(
        std::span<const MinimapCoastEdgeCandidate> candidates,
        const terrain::WorldGrid &source, bool wrap_x,
        const MinimapWorldTransform &transform) {
        MinimapCoastEdgeRecordTable result(source);
        for (const auto &candidate : candidates) {
            if (candidate.direction >= 12 ||
                detail::coast_normalize_source_cube(candidate.worker_cube, source) !=
                    candidate.table_cube)
                throw std::invalid_argument("Minimap coast candidate table key differs");
            for (const auto &point : candidate.world_points)
                if (!std::isfinite(point.x) || !std::isfinite(point.y))
                    throw std::invalid_argument("Nonfinite minimap coast point");
            if (!minimap_coast_edge_registration_allowed(candidate, source, wrap_x))
                continue;
            ++result.accepted_pairs_;
            const auto slot = candidate.direction;
            const auto reverse = std::uint8_t(slot < 6 ? slot + 6 : slot - 6);
            const auto key = [&](std::uint8_t direction) {
                const auto &cube = candidate.table_cube;
                return std::tuple{cube[0], cube[1], cube[2], int(direction)};
            };
            result.records_.insert_or_assign(
                key(slot), make_minimap_terrain_edge_record(
                    candidate.worker_cube, candidate.world_points, transform, false));
            result.records_.insert_or_assign(
                key(reverse), make_minimap_terrain_edge_record(
                    candidate.worker_cube, candidate.world_points, transform, true));
        }
        return result;
    }

    std::optional<MinimapTerrainEdgeRecord> lookup(const MinimapEdgeQuery &query) const {
        if (query.direction >= 12)
            throw std::invalid_argument("Invalid minimap coast record slot");
        const auto cube = detail::coast_normalize_source_cube(query.cube, source_);
        const auto found = records_.find({cube[0], cube[1], cube[2], int(query.direction)});
        if (found == records_.end())
            return std::nullopt;
        return found->second;
    }

    std::size_t accepted_pairs() const { return accepted_pairs_; }
    std::size_t distinct_slots() const { return records_.size(); }

  private:
    explicit MinimapCoastEdgeRecordTable(const terrain::WorldGrid &source) : source_(source) {}
    terrain::WorldGrid source_;
    std::map<std::tuple<std::int32_t, std::int32_t, std::int32_t, int>,
             MinimapTerrainEdgeRecord> records_;
    std::size_t accepted_pairs_ = 0;
};

} // namespace civ5::ui
