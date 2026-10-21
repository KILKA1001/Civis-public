#pragma once

#include "civ5/ui/minimap_hex_geometry.hpp"
#include "civ5/saves/map_state.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <span>
#include <stdexcept>
#include <vector>

namespace civ5::ui {

// Exact 16-byte first kind-5 record consumed by retail 0x004D7D60.
struct MinimapCityMarker {
    float world_x, world_y;
    std::int16_t cube_q, cube_r;
    std::array<std::uint8_t, 4> rgba;
};
static_assert(sizeof(MinimapCityMarker) == 16);
static_assert(offsetof(MinimapCityMarker, cube_q) == 8);
static_assert(offsetof(MinimapCityMarker, rgba) == 12);

// Retail callback payload +0x24/+0x26 contains cube q/r, not staggered map
// x/y. Another subscriber, 0x0074B440, converts q/r back to map x by adding
// r>>1. Saved cities use staggered x/y and must make this conversion first.
inline std::array<std::int16_t, 2> minimap_city_cube_from_map(
    std::uint32_t map_x, std::uint32_t map_y) {
    const auto q = static_cast<std::int64_t>(map_x) - (map_y >> 1);
    if (q < std::numeric_limits<std::int16_t>::min() ||
        q > std::numeric_limits<std::int16_t>::max() ||
        map_y > std::numeric_limits<std::int16_t>::max())
        throw std::invalid_argument("City coordinates exceed retail callback fields");
    return {static_cast<std::int16_t>(q), static_cast<std::int16_t>(map_y)};
}

// Event-fed first kind-5 list. A saved city alone does not establish that the
// original game emitted the create callback after loading that save.
class MinimapCityMarkerList {
  public:
    using ColorTable = std::span<const std::array<std::uint8_t, 4>>;

    const std::vector<MinimapCityMarker> &records() const { return records_; }

    void created(std::int16_t q, std::int16_t r, std::uint32_t color_index,
                 ColorTable colors, const MinimapWorldTransform &transform) {
        const auto color = table_color(color_index, colors);
        const auto it = find(q, r);
        if (it != records_.end()) {
            // Retail 0x006421DD recolors a duplicate without reprojecting it.
            it->rgba = color;
            return;
        }
        const auto world = minimap_project_cube(
            transform, {static_cast<float>(q), static_cast<float>(r),
                        static_cast<float>(-static_cast<std::int32_t>(q) - r), 1.0f});
        if (!(std::isfinite(world.x) && std::isfinite(world.y)))
            throw std::invalid_argument("City marker projection is invalid");
        records_.push_back({world.x, world.y, q, r, color});
    }

    void destroyed(std::int16_t q, std::int16_t r) {
        const auto it = find(q, r);
        if (it == records_.end()) return;
        // Retail 0x00520D29..0x00520D89 swaps the last record into the hole.
        *it = records_.back();
        records_.pop_back();
    }

    void recolored(std::int16_t q, std::int16_t r, std::uint32_t color_index,
                   ColorTable colors) {
        const auto color = table_color(color_index, colors);
        for (auto &record : records_)
            if (record.cube_q == q && record.cube_r == r)
                record.rgba = color;
    }

  private:
    static std::array<std::uint8_t, 4> table_color(
        std::uint32_t index, ColorTable colors) {
        if (index >= colors.size())
            throw std::invalid_argument("City marker color index is unavailable");
        return colors[index];
    }

    std::vector<MinimapCityMarker>::iterator find(std::int16_t q, std::int16_t r) {
        return std::find_if(records_.begin(), records_.end(), [&](const auto &record) {
            return record.cube_q == q && record.cube_r == r;
        });
    }

    std::vector<MinimapCityMarker> records_;
};

// A captured first-kind-5 record is accepted only when its native q/r fields
// identify a real city in the same save, the active team has revealed its plot,
// and the reconstructed owner projects to the captured Float32 position.
// Empty captured input is valid for a city-free state or an uncaptured save.
inline std::vector<MinimapCityMarker> reconcile_native_minimap_city_markers(
    const saves::SavedMapProjection &map,
    const saves::SavedMapTeamArtObservation &team_art,
    const MinimapWorldTransform &transform,
    std::span<const MinimapCityMarker> captured) {
    if (map.grid.width == 0 || map.grid.height == 0 ||
        map.grid.width > std::numeric_limits<std::size_t>::max() / map.grid.height ||
        map.cells.size() != map.grid.width * map.grid.height ||
        team_art.plots.size() != map.cells.size())
        throw std::invalid_argument("Incomplete captured city marker map");
    std::vector<MinimapCityMarker> result;
    result.reserve(captured.size());
    for (const auto &marker : captured) {
        const auto x = static_cast<std::int32_t>(marker.cube_q) +
                       (static_cast<std::int32_t>(marker.cube_r) >> 1);
        const auto y = static_cast<std::int32_t>(marker.cube_r);
        if (x < 0 || y < 0 || static_cast<std::uint64_t>(x) >= map.grid.width ||
            static_cast<std::uint64_t>(y) >= map.grid.height)
            throw std::invalid_argument("Captured city marker lies outside the map");
        const auto index = static_cast<std::size_t>(y) * map.grid.width + x;
        const auto &cell = map.cells[index];
        const auto &art = team_art.plots[index];
        if (cell.x != static_cast<std::uint64_t>(x) ||
            cell.y != static_cast<std::uint64_t>(y) ||
            !cell.plot_city || cell.plot_city->owner < 0 ||
            art.plot_index != index || art.revealed != true)
            throw std::invalid_argument("Captured city marker lacks a revealed saved city");
        const auto projected = minimap_project_cube(
            transform, {static_cast<float>(marker.cube_q),
                        static_cast<float>(marker.cube_r),
                        static_cast<float>(-static_cast<std::int32_t>(marker.cube_q) -
                                           marker.cube_r), 1.0f});
        if (!std::isfinite(marker.world_x) || !std::isfinite(marker.world_y) ||
            std::abs(projected.x - marker.world_x) > 0.001f ||
            std::abs(projected.y - marker.world_y) > 0.001f ||
            std::any_of(result.begin(), result.end(), [&](const auto &existing) {
                return existing.cube_q == marker.cube_q &&
                       existing.cube_r == marker.cube_r;
            }))
            throw std::invalid_argument("Captured city marker differs from native projection");
        result.push_back(marker);
    }
    return result;
}

} // namespace civ5::ui
