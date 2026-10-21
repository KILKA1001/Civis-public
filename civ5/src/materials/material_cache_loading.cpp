#include "civ5/materials/material_cache_loading.hpp"
#include <stdexcept>

namespace civ5::materials {
void material_cache_begin_load(std::span<MaterialCacheSchedulerPatch> patches,
                                std::span<const std::uint32_t> active,
                                std::span<const MaterialCacheSchedulerView> views,
                                bool block_on_load, bool cull, MaterialCacheLoadState &owner) {
    if (active.size() > UINT32_MAX || views.size() > 4)
        throw std::invalid_argument("Material loading domain");
    for (auto id : active)
        if (id >= patches.size())
            throw std::out_of_range("Material loading patch index");
    owner.foreground.clear();
    owner.background.clear();
    for (auto id : active) {
        auto &patch = patches[id];
        if (patch.blocked == 1)
            continue;
        bool immediate = block_on_load;
        if (!immediate && patch.enabled) {
            immediate = !cull;
            if (cull)
                for (auto view = views.rbegin(); view != views.rend(); ++view)
                    if (material_cache_cull(patch, view->frustum)) {
                        immediate = true;
                        break;
                    }
        }
        (immediate ? owner.foreground : owner.background).push_back(id);
    }
    owner.cursor = owner.background_cursor = 0;
}
void material_cache_load_step(std::span<MaterialCacheSchedulerPatch> patches,
                               MaterialCacheLoadState &owner, terrain::X87Precision precision,
                               std::uintptr_t dispatch, std::uint32_t frame_stamp,
                               std::uintptr_t argument, MaterialCacheLoadBackend &backend) {
    if (precision != terrain::X87Precision::single && precision != terrain::X87Precision::double_precision)
        throw std::invalid_argument("Material loading precision");
    if (owner.foreground.size() > UINT32_MAX || owner.cursor > owner.foreground.size())
        throw std::invalid_argument("Material loading cursor");
    for (auto id : owner.foreground)
        if (id >= patches.size() || !patches[id].state)
            throw std::invalid_argument("Material loading requires persistent patch states");
    if (owner.foreground.empty()) {
        owner.progress = 1.f;
        owner.finished = 1;
        return;
    }
    if (owner.cursor == owner.foreground.size()) {
        owner.finished = 1;
        return;
    }
    backend.prepare_frame();
    owner.prepared_in_step = 0;
    backend.begin_step();
    while (owner.cursor < owner.foreground.size() && owner.prepared_in_step < 4) {
        const auto id = owner.foreground[owner.cursor];
        if (!backend.acquire_base(id)) {
            owner.cursor = static_cast<std::uint32_t>(owner.foreground.size());
            break;
        }
        patches[id].state->requested_state = 1;
        backend.prepare(id, dispatch);
        backend.wait();
        backend.complete(id, frame_stamp, argument);
        ++owner.cursor;
        ++owner.prepared_in_step;
    }
    backend.end_generation();
    const auto numerator = terrain::round_x87(double(owner.cursor), precision);
    const auto denominator = terrain::round_x87(double(owner.foreground.size()), precision);
    owner.progress = static_cast<float>(terrain::round_x87(numerator / denominator, precision));
    backend.end_step();
}
} // namespace civ5::materials
