#pragma once
#include "civ5/materials/material_cache_free_lists.hpp"
#include "civ5/materials/material_cache_lifecycle.hpp"
#include "material_cache_acquisition_validation.hpp"
#include <functional>
#include <memory>

using namespace civ5::materials;
using namespace cache_acquisition_validation;
namespace cache_lifecycle_validation {
constexpr std::array<unsigned, 5> staging_offsets{0x318, 0x31c, 0x30c, 0x304, 0x308};
struct Event {
    unsigned kind;
    std::vector<std::uint64_t> arguments;
    Bytes patch;
};
struct Case {
    Bytes entering;
    MaterialCacheLifecycleState state;
    std::vector<Event> events;
    bool observe_geometry_job = false;
    std::function<std::uint32_t(std::uintptr_t)> staging_label = [](std::uintptr_t token) {
        require(token <= UINT32_MAX, "Original fixture transport requires 32-bit staging labels");
        return static_cast<std::uint32_t>(token);
    };
    explicit Case(Bytes bytes) : entering(std::move(bytes)) {
        auto &s = state;
        s.prepared = entering.at(0x136) != 0;
        s.requested_state = get(entering, 0x314);
        s.completed_state = get(entering, 0x310);
        s.frame_stamp = get(entering, 0x1d0);
        s.pending_jobs.store(get(entering, 0x1e0));
        s.textures = {get(entering, 0x178),
                      get(entering, 0x17c),
                      get(entering, 0x180),
                      {get(entering, 0x190), get(entering, 0x194)},
                      {get(entering, 0x198), get(entering, 0x19c)},
                      get(entering, 0x2fc) != 0};
        s.geometry.vertex_buffer = get(entering, 0x168);
        s.geometry.primary_index_buffer = get(entering, 0xf4);
        s.geometry.secondary_index_buffer = get(entering, 0xf8);
        s.geometry.vertex_count = get(entering, 0x16c);
        s.geometry.triangle_count = get(entering, 0x170);
        for (unsigned i = 0; i < 3; ++i)
            s.material_stream_offsets[i] = get(entering, 0xc8 + 4 * i);
        for (unsigned i = 0; i < 5; ++i) {
            s.geometry.packets[i] = get(entering, 0x154 + 4 * i);
            s.staging[i] = get(entering, staging_offsets[i]);
        }
        for (unsigned i = 0; i < 3; ++i)
            s.scratch[i] = get(entering, 0x184 + 4 * i);
    }
    Bytes snapshot() const {
        auto out = entering;
        const auto &s = state;
        out[0x136] = s.prepared;
        if (observe_geometry_job) {
            require(!s.geometry_job.owner || s.geometry_job.owner == &s, "Geometry job owner mismatch");
            put(out, 0x150, s.geometry_job.owner ? 0x31000000 : 0);
        }
        put(out, 0x314, s.requested_state);
        put(out, 0x310, s.completed_state);
        put(out, 0x1d0, s.frame_stamp);
        put(out, 0x1e0, s.pending_jobs.load());
        put(out, 0x178, s.textures.higher_size);
        put(out, 0x17c, s.textures.final_size);
        put(out, 0x180, s.textures.desired_higher_size);
        put(out, 0x190, s.textures.higher[0]);
        put(out, 0x194, s.textures.higher[1]);
        put(out, 0x198, s.textures.final[0]);
        put(out, 0x19c, s.textures.final[1]);
        put(out, 0x168, s.geometry.vertex_buffer);
        put(out, 0xf4, s.geometry.primary_index_buffer);
        put(out, 0xf8, s.geometry.secondary_index_buffer);
        put(out, 0x16c, s.geometry.vertex_count);
        put(out, 0x170, s.geometry.triangle_count);
        for (unsigned i = 0; i < 3; ++i)
            put(out, 0xc8 + 4 * i, s.material_stream_offsets[i]);
        for (unsigned i = 0; i < 5; ++i) {
            put(out, 0x154 + 4 * i, s.geometry.packets[i]);
            put(out, staging_offsets[i], staging_label(s.staging[i]));
        }
        for (unsigned i = 0; i < 3; ++i)
            put(out, 0x184 + 4 * i, s.scratch[i]);
        return out;
    }
    void record(unsigned kind, std::vector<std::uint64_t> arguments = {}) {
        events.push_back({kind, std::move(arguments), snapshot()});
    }
};
struct Pool final : MaterialCacheTexturePool {
    Case &test;
    MaterialCacheFreeLists storage;
    explicit Pool(Case &c) : test(c) {}
    std::uint32_t take(std::uint32_t, std::uint32_t &) override {
        throw std::runtime_error("Completion dispatcher must not reserve textures");
    }
    void give(std::uint32_t handle) override {
        test.record(2, {handle});
        storage.give(handle);
    }
};
struct Backend final : MaterialCacheLifecycleBackend {
    Case &test;
    std::map<std::uintptr_t, std::unique_ptr<Bytes>> staging;
    explicit Backend(Case &c) : test(c) {
        for (auto token : test.state.staging)
            if (token)
                require(staging.emplace(token, std::make_unique<Bytes>(64, 0xa5)).second,
                        "Duplicate staging fixture token");
    }
    void rebuild_geometry(MaterialCacheLifecycleState &) override { test.record(0); }
    void generate_cache(MaterialCacheLifecycleState &, std::uint32_t state,
                        std::uintptr_t argument) override {
        test.record(1, {state, argument});
    }
    void release_geometry_pair(std::uint32_t vertex, std::uint32_t index) override {
        test.record(3, {vertex, index});
    }
    void release_vertex_buffer(std::uint32_t handle) override { test.record(4, {handle}); }
    void release_index_buffer(std::uint32_t handle) override { test.record(5, {handle}); }
    void release_packet(std::uint32_t handle) override { test.record(6, {handle}); }
    void free_staging(std::uintptr_t token) override {
        test.record(7, {token});
        if (token)
            require(staging.erase(token) == 1, "Unknown or repeatedly freed native staging allocation");
    }
    void decrement_pending(std::atomic<std::uint32_t> &pending) override {
        test.record(8, {pending.load()});
        MaterialCacheLifecycleBackend::decrement_pending(pending);
    }
    void validate() const {
        std::size_t retained = 0;
        for (const auto token : test.state.staging)
            if (token) {
                ++retained;
                require(staging.contains(token), "Live staging token lost its native allocation");
            }
        require(retained == staging.size(), "Native staging allocation has no live token");
    }
};
} // namespace cache_lifecycle_validation
