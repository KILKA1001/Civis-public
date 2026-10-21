#pragma once
#include "civ5/materials/material_cache_geometry.hpp"
#include "material_cache_lifecycle_validation.hpp"

namespace cache_geometry_validation {
using cache_lifecycle_validation::Case;
using cache_lifecycle_validation::Event;
using cache_acquisition_validation::Bytes;
using cache_acquisition_validation::get;
using cache_acquisition_validation::put;
using cache_acquisition_validation::require;
struct Backend final : MaterialCacheStagingBackend {
    Case &test;
    unsigned failure_mask = 0, allocation_count = 0;
    MaterialCacheGeometryJob *queued = nullptr;
    std::map<std::uintptr_t, std::unique_ptr<Bytes>> allocations;
    std::map<std::uintptr_t, std::uint32_t> labels;
    std::vector<Event> events;
    explicit Backend(Case &value) : test(value) {
        for (auto token : test.state.staging)
            require(!token, "Geometry fixture requires empty entering staging");
    }
    std::uint32_t label(std::uintptr_t token) const { return token ? labels.at(token) : 0; }
    Bytes snapshot() const {
        auto out = test.snapshot();
        require(!test.state.geometry_job.owner || test.state.geometry_job.owner == &test.state,
                "Geometry job has a different native owner");
        put(out, 0x150, test.state.geometry_job.owner ? 0x20000000 : 0);
        return out;
    }
    void record(unsigned kind, std::vector<std::uint64_t> arguments = {}) {
        events.push_back({kind, std::move(arguments), snapshot()});
    }
    std::uintptr_t allocate_staging(std::size_t size) override {
        const auto slot = allocation_count++;
        require(slot < 2 && size <= 0x20000, "Geometry fixture allocation extent");
        std::uintptr_t token = 0;
        if (!(failure_mask & (1u << slot))) {
            auto bytes = std::make_unique<Bytes>(size + 32, 0xda);
            std::fill(bytes->begin() + 16, bytes->end() - 16, 0xa5);
            token = reinterpret_cast<std::uintptr_t>(bytes->data() + 16);
            require(token > UINT32_MAX && labels.emplace(token, 0x28000000u + slot * 0x1000000u).second,
                    "Geometry allocation did not retain a distinct native pointer");
            allocations.emplace(token, std::move(bytes));
        }
        record(9, {size, label(token)});
        return token;
    }
    std::span<std::uint8_t> staging_memory(std::uintptr_t token) override {
        const auto &bytes = allocations.at(token);
        return std::span(*bytes).subspan(16, bytes->size() - 32);
    }
    void validate_guards() const {
        for (const auto &[token, bytes] : allocations) {
            require(std::all_of(bytes->begin(), bytes->begin() + 16, [](auto b) { return b == 0xda; }) &&
                        std::all_of(bytes->end() - 16, bytes->end(), [](auto b) { return b == 0xda; }),
                    "Geometry worker exceeded a native allocation");
            require(token == reinterpret_cast<std::uintptr_t>(bytes->data() + 16), "Staging address moved");
        }
    }
    void free_staging(std::uintptr_t token) override {
        record(7, {label(token)});
        validate_guards();
        if (token)
            require(allocations.erase(token) == 1, "Geometry staging freed twice");
    }
    void submit_geometry(MaterialCacheGeometryJob &job, std::uintptr_t dispatch) override {
        require(!queued && &job == &test.state.geometry_job && job.owner == &test.state,
                "Geometry submitted record or owner mismatch");
        queued = &job;
        record(10, {dispatch});
    }
    void prepare_material(MaterialCacheLifecycleState &, std::uintptr_t) override {
        throw std::runtime_error("Geometry-only fixture submitted a material job");
    }
    void rebuild_geometry(MaterialCacheLifecycleState &state) override {
        require(&state == &test.state, "Geometry completion lost owner");
        record(0);
    }
    void generate_cache(MaterialCacheLifecycleState &, std::uint32_t, std::uintptr_t) override {
        throw std::runtime_error("Geometry-only fixture generated material caches");
    }
    void release_geometry_pair(std::uint32_t, std::uint32_t) override {
        throw std::runtime_error("Unexpected geometry pair release");
    }
    void release_vertex_buffer(std::uint32_t) override { throw std::runtime_error("Unexpected vertex release"); }
    void release_index_buffer(std::uint32_t) override { throw std::runtime_error("Unexpected index release"); }
    void release_packet(std::uint32_t) override { throw std::runtime_error("Unexpected packet release"); }
};
inline void observe_staging(Backend &backend) {
    backend.test.staging_label = [&backend](std::uintptr_t token) { return backend.label(token); };
}
inline Bytes entering(int columns, int rows) {
    Bytes bytes(1024, 0);
    put(bytes, 0xd4, columns);
    put(bytes, 0xd8, rows);
    put(bytes, 0x310, 1);
    put(bytes, 0x314, 3);
    return bytes;
}
} // namespace cache_geometry_validation
