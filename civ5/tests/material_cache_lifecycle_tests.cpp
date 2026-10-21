#include "civ5/materials/material_cache_free_lists.hpp"
#include "civ5/materials/material_cache_lifecycle.hpp"
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
struct Backend final : MaterialCacheLifecycleBackend {
    std::map<std::uintptr_t, std::unique_ptr<int>> allocations;
    std::vector<std::uintptr_t> freed;
    std::uintptr_t generated_argument = 0;
    unsigned generations = 0;
    void rebuild_geometry(MaterialCacheLifecycleState &) override {}
    void generate_cache(MaterialCacheLifecycleState &, std::uint32_t, std::uintptr_t argument) override {
        generated_argument = argument;
        ++generations;
    }
    void release_geometry_pair(std::uint32_t, std::uint32_t) override {}
    void release_vertex_buffer(std::uint32_t) override {}
    void release_index_buffer(std::uint32_t) override {}
    void release_packet(std::uint32_t) override {}
    void free_staging(std::uintptr_t token) override {
        freed.push_back(token);
        if (token)
            require(allocations.erase(token) == 1, "Native staging token truncated or freed twice");
    }
};
} // namespace
int main() {
    try {
        static_assert(sizeof(std::uintptr_t) == 8);
        MaterialCacheLifecycleState state;
        MaterialCacheFreeLists pool;
        Backend backend;
        for (unsigned i = 0; i < state.staging.size(); ++i) {
            const auto token = std::uintptr_t{0x1234567887654300ull} + i;
            state.staging[i] = token;
            backend.allocations.emplace(token, std::make_unique<int>(i));
        }
        const auto entering_staging = state.staging;
        state.completed_state = 1;
        state.requested_state = 3;
        state.pending_jobs.store(2);
        state.frame_stamp = 9;
        require(!material_cache_dispatch(state, pool, backend, false, 10, 0), "Unprepared job completed");
        require(state.completed_state == 1 && state.pending_jobs.load() == 2 && state.frame_stamp == 9,
                "Unprepared cleanup changed completion state");
        require(backend.freed ==
                        std::vector<std::uintptr_t>(entering_staging.begin(), entering_staging.end()) &&
                    backend.allocations.empty() && state.staging == std::array<std::uintptr_t, 5>{},
                "Native 64-bit staging ownership was not cleared");
        state.prepared = true;
        state.completed_state = 4;
        state.requested_state = 5;
        state.pending_jobs.store(0);
        require(material_cache_dispatch(state, pool, backend, true, 10, 0), "Prepared job did not complete");
        require(state.completed_state == 5 && state.pending_jobs.load() == UINT32_MAX &&
                    state.frame_stamp == 10 && backend.generations == 0,
                "Atomic wrapping or original 4-to-5 reuse changed");
        constexpr auto argument = std::uintptr_t{0x7654321012345678ull};
        state.requested_state = 1;
        state.pending_jobs.store(1);
        require(material_cache_dispatch(state, pool, backend, false, 11, argument),
                "State-one completion failed");
        require(backend.generations == 1 && backend.generated_argument == argument && state.pending_jobs == 0,
                "Native generation argument or atomic completion was truncated");
        std::cout << "PASS lifecycle native token width, ownership and completion boundaries\n";
        return 0;
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
