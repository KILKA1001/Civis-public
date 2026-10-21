#include "civ5/materials/material_cache_free_lists.hpp"
#include "civ5/materials/material_cache_preparation.hpp"
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <stdexcept>
#include <vector>

using namespace civ5::materials;
namespace {
void require(bool value, const char *message) {
    if (!value)
        throw std::runtime_error(message);
}
struct Backend final : MaterialCachePreparationBackend {
    std::map<std::uintptr_t, std::unique_ptr<std::uint8_t[]>> allocations;
    std::vector<std::size_t> sizes;
    std::vector<std::uintptr_t> freed;
    MaterialCacheGeometryJob *queued = nullptr;
    std::uintptr_t dispatch = 0;
    unsigned failure_mask = 0, materials = 0;
    std::function<void(MaterialCacheLifecycleState &)> on_submit, on_material;
    std::uintptr_t allocate_staging(std::size_t bytes) override {
        const auto slot = sizes.size();
        sizes.push_back(bytes);
        if (failure_mask & (1u << slot))
            return 0;
        const auto token = std::uintptr_t{0x1234567887654300ull} + slot;
        allocations.emplace(token, std::make_unique<std::uint8_t[]>(bytes));
        return token;
    }
    void free_staging(std::uintptr_t token) override {
        freed.push_back(token);
        if (token)
            require(allocations.erase(token) == 1, "Staging allocation lost or freed twice");
    }
    void submit_geometry(MaterialCacheGeometryJob &job, std::uintptr_t argument) override {
        queued = &job;
        dispatch = argument;
        require(job.owner != nullptr, "Geometry job lost its owner");
        if (on_submit)
            on_submit(*job.owner);
    }
    void prepare_material(MaterialCacheLifecycleState &state, std::uintptr_t argument) override {
        ++materials;
        dispatch = argument;
        if (on_material)
            on_material(state);
    }
    void rebuild_geometry(MaterialCacheLifecycleState &) override {}
    void generate_cache(MaterialCacheLifecycleState &, std::uint32_t, std::uintptr_t) override {}
    void release_geometry_pair(std::uint32_t, std::uint32_t) override {}
    void release_vertex_buffer(std::uint32_t) override {}
    void release_index_buffer(std::uint32_t) override {}
    void release_packet(std::uint32_t) override {}
};
} // namespace
int main() {
    try {
        static_assert(sizeof(std::uintptr_t) == 8);
        constexpr std::uintptr_t dispatch = 0x7654321012345678ull;
        for (unsigned failure = 0; failure < 4; ++failure) {
            MaterialCacheLifecycleState state;
            MaterialCacheFreeLists pool;
            Backend backend;
            backend.failure_mask = failure;
            state.requested_state = 3;
            state.completed_state = 1;
            state.pending_jobs.store(2);
            state.frame_stamp = 7;
            material_cache_prepare(state, {65, 65, 3}, backend, false, dispatch);
            require(backend.sizes == std::vector<std::size_t>{529 * 24, 24576},
                    "Original staging sizes changed");
            require(state.prepared == (failure == 0) && backend.materials == 0,
                    "Allocation failure or 1-to-3 preparation changed");
            require(state.geometry.vertex_count == 529 && state.geometry.triangle_count == 968,
                    "Coarse geometry dimensions changed");
            require(state.pending_jobs == 2 && state.frame_stamp == 7 && state.completed_state == 1,
                    "Preparation changed completion counters");
            if (failure) {
                require(backend.allocations.empty() && backend.freed.size() == 2 &&
                            backend.queued == nullptr && state.staging[0] == 0 && state.staging[1] == 0,
                        "Failed allocation retained storage or submitted a job");
                require(!material_cache_dispatch(state, pool, backend, false, 8, 0) &&
                            state.pending_jobs == 2 && state.completed_state == 1,
                        "Failed preparation completed a job");
            } else {
                require(backend.queued == &state.geometry_job && backend.queued->owner == &state &&
                            backend.dispatch == dispatch && state.staging[0] > UINT32_MAX,
                        "Queued owner or 64-bit staging/dispatch argument changed");
                material_cache_clear_staging(state, backend);
                require(backend.allocations.empty(), "Successful staging cleanup lost ownership");
            }
        }
        MaterialCacheLifecycleState state;
        Backend backend;
        state.requested_state = 3;
        state.completed_state = 1;
        backend.on_submit = [](auto &owner) { owner.requested_state = 1; };
        backend.on_material = [](auto &owner) { owner.prepared = false; };
        material_cache_prepare(state, {3, 5, 1}, backend, false, dispatch);
        require(backend.materials == 1 && !state.prepared, "Preparation ignored live callback state");
        material_cache_clear_staging(state, backend);
        Backend separate;
        state.requested_state = 4;
        state.geometry.vertex_count = 42;
        material_cache_prepare(state, {0, 0, 0}, separate, true, dispatch);
        require(state.prepared && state.geometry.vertex_count == 42 && separate.sizes.empty(),
                "Separate-buffer preparation consulted unused CPU dimensions");
        for (const auto domain : {MaterialCachePreparationDomain{1, 5, 1},
                                  {3, 5, 0},
                                  {INT32_MAX, 3, INT32_MAX},
                                  {INT32_MAX, INT32_MAX, 1}}) {
            state.prepared = false;
            try {
                material_cache_prepare(state, domain, separate, false, dispatch);
                throw std::runtime_error("Invalid preparation domain accepted");
            } catch (const std::invalid_argument &) {
            } catch (const std::overflow_error &) {
            }
            require(!state.prepared && state.geometry.vertex_count == 42 && separate.sizes.empty(),
                    "Invalid domain changed state or allocated storage");
        }
        std::cout << "PASS preparation ownership, failures, native widths and callback state\n";
        return 0;
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
