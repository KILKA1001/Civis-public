#pragma once
#include "civ5/materials/material_cache_free_lists.hpp"
#include "civ5/materials/material_cache_jobs.hpp"
#include "material_cache_lifecycle_validation.hpp"

namespace material_jobs_validation {
using namespace civ5::materials;
using Bytes = civ5::assets::Bytes;
using cache_acquisition_validation::require;
struct Event {
    unsigned kind;
    std::vector<std::uint64_t> arguments;
    Bytes patch;
};
struct Backend final : MaterialCacheMaterialBackend {
    MaterialCacheLifecycleState &state;
    MaterialCachePreparationDomain domain;
    cache_lifecycle_validation::Case *observed = nullptr;
    unsigned failure_mask = 0, allocation_count = 0;
    std::vector<MaterialCacheMaterialJob> records;
    std::vector<MaterialCacheMaterialJob *> queued;
    std::map<std::uintptr_t, std::unique_ptr<Bytes>> buffers;
    std::map<std::uintptr_t, std::uint32_t> labels;
    std::vector<Event> events;
    std::array<Bytes, 3> appended;
    std::array<std::uint32_t, 3> offsets{}, cursors{};
    unsigned executed_jobs = 0;
    static constexpr std::array<std::uint32_t, 3> original_labels{0x23000000, 0x23040000, 0x23080000};
    Backend(MaterialCacheLifecycleState &s, MaterialCachePreparationDomain d) : state(s), domain(d) {}
    void observe(cache_lifecycle_validation::Case &test) {
        observed = &test;
        // These entering geometry slots are outside the material producer's
        // ownership. Preserve their original opaque labels in full snapshots.
        for (unsigned i = 0; i < 2; ++i) {
            const auto token = test.state.staging[i];
            require(token <= UINT32_MAX, "Original geometry sentinel width");
            if (token)
                labels.emplace(token, static_cast<std::uint32_t>(token));
        }
        test.staging_label = [this](std::uintptr_t token) { return label(token); };
    }
    std::uint32_t label(std::uintptr_t token) const {
        if (!token)
            return 0;
        const auto found = labels.find(token);
        require(found != labels.end(), "Unknown native material storage token");
        return found->second;
    }
    void record(unsigned kind, std::vector<std::uint64_t> arguments) {
        events.push_back({kind, std::move(arguments), observed ? observed->snapshot() : Bytes{}});
    }
    MaterialCacheMaterialJobStorage allocate_material_records(std::size_t count) override {
        require(allocation_count++ == 0, "Material record allocation order");
        if (failure_mask & 1) {
            record(0, {count, 0});
            return {0, {}};
        }
        records.resize(count);
        for (auto &job : records)
            job = {&state, 999, {1, 2, 3, 4}};
        const auto token = reinterpret_cast<std::uintptr_t>(records.data());
        require(labels.emplace(token, original_labels[0]).second, "Reused record allocation address");
        record(0, {count, label(token)});
        return {token, records};
    }
    std::uintptr_t allocate_staging(std::size_t bytes) override {
        const auto slot = allocation_count++;
        require(slot == 1 || slot == 2, "Material stream allocation order");
        std::uintptr_t token = 0;
        if (!(failure_mask & (1u << slot))) {
            auto buffer = std::make_unique<Bytes>(bytes + 32, 0xda);
            std::fill(buffer->begin() + 16, buffer->end() - 16, slot == 1 ? 0xa5 : 0x93);
            token = reinterpret_cast<std::uintptr_t>(buffer->data() + 16);
            require(labels.emplace(token, original_labels[slot]).second,
                    "Reused material allocation address");
            buffers.emplace(token, std::move(buffer));
        }
        record(1, {bytes, label(token)});
        return token;
    }
    void check_guards() const {
        for (const auto &[token, pointer] : buffers) {
            const auto &bytes = *pointer;
            require(std::all_of(bytes.begin(), bytes.begin() + 16, [](auto b) { return b == 0xda; }) &&
                        std::all_of(bytes.end() - 16, bytes.end(), [](auto b) { return b == 0xda; }),
                    "Material worker overran a staging allocation");
        }
    }
    void free_staging(std::uintptr_t token) override {
        record(2, {label(token)});
        if (!token)
            return;
        check_guards();
        if (!records.empty() && token == reinterpret_cast<std::uintptr_t>(records.data())) {
            std::vector<MaterialCacheMaterialJob>().swap(records);
            queued.clear();
        } else
            require(buffers.erase(token) == 1, "Material staging freed twice");
    }
    std::span<std::uint8_t> staging_memory(std::uintptr_t token) override {
        const auto found = buffers.find(token);
        require(found != buffers.end(), "Material worker accessed released storage");
        return std::span(*found->second).subspan(16, found->second->size() - 32);
    }
    void submit_geometry(MaterialCacheGeometryJob &, std::uintptr_t) override {
        throw std::runtime_error("Material fixture does not submit CPU geometry");
    }
    void prepare_material(MaterialCacheLifecycleState &owner, std::uintptr_t dispatch) override {
        require(&owner == &state, "Material preparation owner mismatch");
        material_cache_prepare_material_jobs(owner, domain, *this, dispatch);
    }
    void submit_material(MaterialCacheMaterialJob &job, std::uintptr_t dispatch) override {
        const auto index = static_cast<std::size_t>(&job - records.data());
        require(index == queued.size() && index < records.size() && job.owner == &state &&
                    job.normal_staging == state.staging[4],
                "Material submitted record identity");
        queued.push_back(&job);
        record(3, {index, label(job.normal_staging), dispatch, static_cast<std::uint32_t>(job.tile.x0),
                   static_cast<std::uint32_t>(job.tile.x1), static_cast<std::uint32_t>(job.tile.y0),
                   static_cast<std::uint32_t>(job.tile.y1)});
    }
    MaterialCacheAppendSpan append_material_stream(MaterialCacheStream stream,
                                                   std::size_t elements) override {
        const auto role = static_cast<std::size_t>(stream);
        const auto stride = std::array<std::size_t, 3>{48, 24, 2}[role];
        require(appended[role].empty(), "Material fixture appended a stream twice");
        appended[role].resize(elements * stride);
        cursors[role] = offsets[role] + static_cast<std::uint32_t>(elements);
        record(4, {role, elements, offsets[role]});
        return {offsets[role], appended[role]};
    }
    void rebuild_geometry(MaterialCacheLifecycleState &) override {}
    void generate_cache(MaterialCacheLifecycleState &owner, std::uint32_t, std::uintptr_t) override {
        require(&owner == &state, "Material append owner mismatch");
        // The append at the beginning of original 64A7B0 executes here. GPU
        // queue generation remains outside this CPU fixture.
        material_cache_append_material_jobs(owner, domain, *this);
    }
    void release_geometry_pair(std::uint32_t, std::uint32_t) override {}
    void release_vertex_buffer(std::uint32_t) override {}
    void release_index_buffer(std::uint32_t) override {}
    void release_packet(std::uint32_t) override {}
    void validate_prepared() const {
        check_guards();
        if (failure_mask) {
            require(records.empty() && buffers.empty() && queued.empty(),
                    "Failed preparation retained storage");
            return;
        }
        require(records.size() == std::size_t(domain.columns + 2) * (domain.rows + 2),
                "Incomplete record array");
        for (std::size_t i = queued.size(); i < records.size(); ++i)
            require(!records[i].owner && !records[i].normal_staging && records[i].tile == MaterialTile{},
                    "Unused native record was not initialized");
        require(buffers.size() == 2, "Prepared streams are not owned");
    }
    void execute(MaterialGeometryDomain geometry, const MaterialSelection &selection,
                 const MaterialLookup &height, const MaterialNormalLookup &normal,
                 const MaterialFactorLookup &factors) {
        for (const auto job : queued) {
            material_cache_execute_material_job(*job, geometry, selection, *this, height, normal, factors);
            ++executed_jobs;
        }
        check_guards();
    }
    void validate_released() const {
        require(records.empty() && buffers.empty() && state.staging[2] == 0 && state.staging[3] == 0 &&
                    state.staging[4] == 0,
                "Completed material jobs retained staging storage");
    }
};
} // namespace material_jobs_validation
