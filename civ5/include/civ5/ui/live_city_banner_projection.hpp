#pragma once

#include "civ5/ui/city_screens.hpp"
#include "civ5/ui/live_session_frame.hpp"
#include "civ5/terrain/camera_frame.hpp"
#include "civ5/terrain/height_owner.hpp"
#include "civ5/terrain/world_layout.hpp"
#include <cmath>
#include <cstdint>
#include <limits>
#include <memory>

namespace civ5::ui {

struct LiveCityBannerProjectionAudit {
    std::int64_t city_id = -1;
    float world_x = 0.f, world_y = 0.f;
    double clip_w = 0.0, ndc_x = 0.0, ndc_y = 0.0, ndc_z = 0.0;
    std::uint32_t view = 0;
};

// Add only cities whose identity, name and population were resolved from the
// owned live session and its matching saved record. The caller supplies the
// same camera and terrain layout used for the current world frame.
inline UiFrameSnapshot project_live_city_banners(const UiFrameSnapshot &frame,
    const terrain::WorldLayout &layout, const terrain::HeightOwnerParameters &height,
    const terrain::CameraFrameState &camera, std::int32_t width,
    std::int32_t viewport_height,
    std::vector<LiveCityBannerProjectionAudit> *audit = nullptr) {
    if (audit) audit->clear();
    const auto *world = frame.find<WorldViewState>();
    const auto *cities = frame.find<CityScreenFrame>();
    if (!world || !cities || !frame.active_player_id() ||
        world->player_id != *frame.active_player_id() ||
        cities->player_id != world->player_id || width <= 0 || viewport_height <= 0 ||
        camera.view_count == 0 || camera.view_count > camera.views.size()) return frame;

    auto projected = *world;
    projected.city_banners.clear();
    const auto &m = layout.source_grid.second_matrix;
    const auto &t = layout.transforms;
    for (const auto &city : cities->cities) {
        if (city.owner != world->player_id || city.id < 0 || !city.name ||
            city.name->empty() || city.population <= 0 ||
            city.population > std::numeric_limits<std::int32_t>::max() ||
            city.x < 0 || city.y < 0 || city.x >= layout.source_grid.width ||
            city.y >= layout.source_grid.height) continue;
        if (world->map_visibility) {
            const auto &visibility = *world->map_visibility;
            if (visibility.width != static_cast<std::uint64_t>(layout.source_grid.width) ||
                visibility.height != static_cast<std::uint64_t>(layout.source_grid.height) ||
                visibility.plots.size() != visibility.width * visibility.height) continue;
            const auto plot = static_cast<std::size_t>(city.y) * visibility.width + city.x;
            if (visibility.plots[plot] == WorldViewPlotFog::hidden) continue;
        }
        // Same saved-plot-to-ground-plane transform used to focus the preview
        // camera. The city billboard's native elevation/offset is not known.
        const auto a = terrain::wrap_add(static_cast<std::int32_t>(city.x),
                                         terrain::wrap_neg(static_cast<std::int32_t>(city.y) >> 1));
        const auto c = terrain::wrap_neg(terrain::wrap_add(a, static_cast<std::int32_t>(city.y)));
        const auto grid_x = terrain::wrap_add(terrain::wrap_add(
            terrain::wrap_add(terrain::wrap_mul(a, m[0]), m[12]),
            terrain::wrap_mul(c, m[8])),
            terrain::wrap_mul(static_cast<std::int32_t>(city.y), m[4]));
        const auto grid_y = terrain::wrap_add(terrain::wrap_add(
            terrain::wrap_add(terrain::wrap_mul(static_cast<std::int32_t>(city.y), m[5]), m[13]),
            terrain::wrap_mul(c, m[9])), terrain::wrap_mul(a, m[1]));
        const float x = static_cast<float>(grid_x), y = static_cast<float>(grid_y);
        const float z = -x - y;
        const float ground_x = height.scale_x *
            (x * t[16] + z * t[24] + y * t[20] + t[28]) - height.center_x;
        const float ground_y = height.scale_y *
            (x * t[17] + z * t[25] + y * t[21] + t[29]) - height.center_y;
        if (!std::isfinite(ground_x) || !std::isfinite(ground_y)) continue;

        for (std::uint32_t view = 0; view < camera.view_count; ++view) {
            const auto vp = terrain::matrix_multiply(camera.views[view], camera.projection,
                terrain::X87Precision::double_precision, terrain::MatrixPath::sse);
            const double point[4]{ground_x, ground_y, 0.0, 1.0};
            double clip[4]{};
            for (std::size_t column = 0; column < 4; ++column)
                for (std::size_t row = 0; row < 4; ++row)
                    clip[column] += point[row] * vp[row * 4 + column];
            const double w = clip[3];
            if (!std::isfinite(w) || w <= 0.0 || clip[0] < -w || clip[0] > w ||
                clip[1] < -w || clip[1] > w || clip[2] < 0.0 || clip[2] > w) continue;
            const double sx = (clip[0] / w + 1.0) * width * 0.5;
            const double sy = (1.0 - clip[1] / w) * viewport_height * 0.5;
            if (!std::isfinite(sx) || !std::isfinite(sy)) continue;
            projected.city_banners.push_back({
                {city.id, {static_cast<std::int32_t>(std::lround(sx)),
                           static_cast<std::int32_t>(std::lround(sy))},
                 *city.name, {}, {}, true},
                static_cast<std::int32_t>(city.population), true});
            if (const auto *appearances=frame.find<LiveCityBannerAppearances>()) {
                const auto found=appearances->cities.find({city.owner,city.id});
                if (found!=appearances->cities.end() &&
                    found->second.active_player_id==world->player_id &&
                    found->second.player_id==city.owner && found->second.city_id==city.id &&
                    found->second.frame_number==frame.frame() &&
                    frame.turn() && found->second.turn==*frame.turn())
                    projected.city_banners.back().appearance=found->second;
            }
            if (audit) audit->push_back({city.id, ground_x, ground_y, w,
                clip[0] / w, clip[1] / w, clip[2] / w, view});
            break;
        }
    }
    return frame.with(std::make_shared<const WorldViewState>(std::move(projected)));
}

} // namespace civ5::ui
