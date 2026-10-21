#include "civ5/materials/material_cache_scheduler.hpp"
#include "civ5/materials/material_cache_ordering.hpp"
#include <algorithm>
#include <bit>
#include <limits>
#include <stdexcept>

namespace civ5::materials {
void MaterialCacheCameraState::consume(std::span<const terrain::CameraMotionEvent> events) {
    for (const auto &event : events) {
        if (event.kind == terrain::CameraMotionEventKind::started)
            moving = true;
        else if (event.kind == terrain::CameraMotionEventKind::stopped)
            moving = false;
    }
}
void material_cache_camera_controls(const MaterialCacheCameraState &camera, const MaterialCacheStartup &startup,
                                    MaterialCacheSchedulerSettings &settings) {
    settings.attempt_budget = startup.moving_budget;
    settings.fast_selection_budget = startup.still_budget;
    settings.fast = !camera.moving && startup.fast_when_still;
    settings.force_fast = startup.force_fast;
}
std::vector<MaterialCacheSchedulerView> material_cache_camera_views(const terrain::CameraFrameState &camera) {
    if (camera.view_count > camera.views.size())
        throw std::invalid_argument("Material scheduler camera view count");
    std::vector<MaterialCacheSchedulerView> views(camera.view_count);
    for (unsigned v = 0; v < camera.view_count; ++v)
        views[v] = {camera.views[v], camera.frusta[v]};
    return views;
}
bool material_cache_cull(MaterialCacheSchedulerPatch &patch, const terrain::ViewFrustum &planes) {
    std::array<float, 6> distances;
    bool visible = true;
    const auto &s = patch.sphere;
    const float negative_radius = std::bit_cast<float>(std::bit_cast<std::uint32_t>(s[3]) ^ 0x80000000u);
    for (unsigned i = 0; i < 6; ++i) {
        const auto &p = planes[i];
        const float zy = s[2] * p[2] + s[1] * p[1];
        const float xyz = zy + s[0] * p[0];
        distances[i] = xyz + p[3];
        if (negative_radius > distances[i])
            visible = false;
    }
    patch.frustum_visible = visible;
    if (visible)
        patch.plane_distances = distances;
    return visible;
}
namespace {
void evict(std::span<MaterialCacheSchedulerPatch> patches, std::span<const std::uint32_t> candidates,
           const MaterialCacheSchedulerSettings &settings, MaterialCacheSchedulerBackend &backend) {
    for (auto id : candidates) {
        auto &s = *patches[id].state;
        if (s.completed_state == 3)
            backend.downgrade(id);
        if (s.completed_state == 5) {
            s.completed_state = 4;
            backend.invalidate_geometry(id);
        }
    }
    const auto release_base = [&](auto count) {
        if (count() >= 8)
            return;
        for (auto it = candidates.rbegin(); it != candidates.rend(); ++it) {
            auto &s = *patches[*it].state;
            if (s.completed_state == 1) {
                backend.release_cache(*it, true, true, true);
                s.completed_state = 0;
            }
            if (count() > 8)
                break;
        }
    };
    release_base([&] { return backend.free_count(64); });
    if (settings.compressed_base)
        release_base([&] { return backend.compressed_base_free_count(); });
    for (auto size : {128u, 256u, 512u}) {
        if (backend.free_count(size) >= 8)
            continue;
        for (auto it = candidates.rbegin(); it != candidates.rend(); ++it) {
            const auto &s = *patches[*it].state;
            if (s.textures.higher_size == size && s.completed_state != 0)
                backend.downgrade(*it);
            if (backend.free_count(size) > 8)
                break;
        }
    }
}
MaterialCachePriority priority(const MaterialCacheSchedulerPatch &p) {
    return {std::bit_cast<std::int32_t>(p.state->requested_state),
            std::bit_cast<std::int32_t>(p.state->completed_state), p.priority, p.plane_distances[1]};
}
} // namespace
bool material_cache_schedule(std::span<MaterialCacheSchedulerPatch> patches,
                             std::span<const std::uint32_t> active, std::span<const std::uint32_t> background,
                             std::span<const MaterialCacheSchedulerView> views,
                             const MaterialCacheSchedulerSettings &settings,
                             MaterialCacheSchedulerState &owner, MaterialCacheSchedulerBackend &backend) {
    if (views.size() > 4 || patches.size() > UINT32_MAX || active.size() > INT32_MAX ||
        background.size() > UINT32_MAX)
        throw std::invalid_argument("Material scheduler domain");
    for (const auto &p : patches)
        if (!p.state)
            throw std::invalid_argument("Material scheduler requires persistent patch states");
    const auto validate = [&](auto indices) {
        for (auto id : indices)
            if (id >= patches.size())
                throw std::out_of_range("Material scheduler patch index");
    };
    validate(active);
    validate(background);
    validate(owner.pending);
    for (auto &v : owner.visible)
        v.clear();
    owner.eviction_candidates.clear();
    for (auto id : active) {
        auto &p = patches[id];
        if (p.blocked == 1 || p.blocked == 2)
            continue;
        bool visible = false;
        if (p.enabled)
            for (unsigned v = 0; v < views.size(); ++v) {
                if (settings.cull && !material_cache_cull(p, views[v].frustum))
                    continue;
                owner.visible[v].push_back(id);
                visible = true;
            }
        if (!visible && p.state->completed_state)
            owner.eviction_candidates.push_back(id);
    }
    std::vector<MaterialCachePriority> priorities(patches.size());
    for (unsigned id = 0; id < patches.size(); ++id)
        priorities[id] = {0, 0, patches[id].eviction_stamp, 0};
    material_cache_order(priorities, owner.eviction_candidates);
    evict(patches, owner.eviction_candidates, settings, backend);
    std::size_t remaining_evictions = owner.eviction_candidates.size();
    for (unsigned v = 0; v < views.size(); ++v) {
        const auto matrix = terrain::matrix_multiply(views[v].matrix, settings.projection, settings.precision,
                                                     terrain::MatrixPath::dispatch_x87);
        for (auto id : owner.visible[v]) {
            auto &p = patches[id];
            auto &s = *p.state;
            s.textures.desired_higher_size =
                material_cache_projection(p.bounds, matrix, settings.viewport_width, settings.base_size,
                                          settings.precision, settings.vector_path)
                    .selected_size;
            p.submitted_view = false;
            switch (s.completed_state) {
            case 5:
                if (s.frame_stamp < p.source_revision) {
                    backend.refresh_selection(id);
                    s.requested_state = 2;
                }
                if (s.textures.desired_higher_size != s.textures.higher_size) {
                    const auto size = s.textures.desired_higher_size;
                    if ((size == 64 || size == 128 || size == 256 || size == 512) &&
                        backend.free_count(size) > 1)
                        s.requested_state = 2;
                }
                break;
            case 4:
                s.requested_state = 5;
                break;
            case 1:
                s.requested_state = 3;
                break;
            case 0:
                for (unsigned attempt = 0; attempt < 20; ++attempt) {
                    if (backend.acquire_base(id)) {
                        s.requested_state = 3;
                        break;
                    }
                    const auto end = remaining_evictions > 16 ? remaining_evictions - 16 : 0;
                    while (remaining_evictions > end) {
                        const auto old = owner.eviction_candidates[--remaining_evictions];
                        backend.release_cache(old, true, true, true);
                        patches[old].state->completed_state = 0;
                    }
                }
                break;
            default:
                break;
            }
        }
    }
    for (unsigned id = 0; id < patches.size(); ++id)
        priorities[id] = priority(patches[id]);
    for (unsigned v = 0; v < views.size(); ++v)
        material_cache_order(priorities, owner.visible[v]);
    const bool fast = settings.fast || settings.force_fast;
    const auto selection_limit = fast ? settings.fast_selection_budget : settings.attempt_budget;
    const auto attempt_limit = fast ? 8u : settings.attempt_budget;
    std::vector<std::uint32_t> selected;
    for (unsigned v = 0; v < views.size(); ++v)
        for (auto id : owner.visible[v]) {
            if (selected.size() >= selection_limit)
                break;
            const auto &s = *patches[id].state;
            if ((s.completed_state == 5 && s.requested_state == 5) ||
                (s.completed_state == 4 && s.requested_state == 5))
                break;
            if (s.completed_state != 0)
                selected.push_back(id);
        }
    std::uint32_t attempts = 0;
    bool success = false;
    const auto prepare = [&](std::uint32_t id) {
        backend.prepare(id, settings.dispatch);
        owner.pending.push_back(id);
    };
    for (auto id : selected) {
        if (attempts >= attempt_limit)
            break;
        ++attempts;
        if (backend.acquire_higher(id)) {
            patches[id].state->requested_state = 5;
            prepare(id);
            success = true;
        }
    }
    bool finished = success || owner.background_cursor >= background.size();
    if (!finished) {
        const auto id = background[owner.background_cursor];
        auto &s = *patches[id].state;
        if (s.completed_state == 0 && s.requested_state == 0 && attempts < attempt_limit) {
            ++attempts;
            if (backend.acquire_base(id)) {
                s.requested_state = 1;
                prepare(id);
            }
        }
        ++owner.background_cursor;
    }
    for (unsigned v = 0; v < views.size(); ++v)
        for (auto id : owner.visible[v]) {
            auto &p = patches[id];
            auto &s = *p.state;
            if (!((s.requested_state == 3 && s.completed_state != 3) || s.completed_state == 4))
                continue;
            if (s.requested_state == 3 && s.completed_state != 1) {
                if (attempts >= attempt_limit) {
                    if (s.completed_state == 0) {
                        backend.release_cache(id, false, true, true);
                        s.requested_state = 0;
                    }
                    continue;
                }
                ++attempts;
            }
            if (!p.submitted_view) {
                prepare(id);
                p.submitted_view = true;
            }
            finished = false;
        }
    return finished;
}
} // namespace civ5::materials
