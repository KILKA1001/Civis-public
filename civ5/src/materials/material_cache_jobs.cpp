#include "civ5/materials/material_cache_jobs.hpp"
#include "civ5/terrain/land_grid.hpp"
#include <algorithm>
#include <cstring>
#include <memory>
#include <new>
#include <stdexcept>

namespace civ5::materials {
namespace {
std::size_t count(const MaterialCachePreparationDomain &domain) {
    if (domain.columns < 0 || domain.columns > 65 || domain.rows < 0 || domain.rows > 65)
        throw std::invalid_argument("Invalid material job domain");
    return std::size_t(domain.columns + 2) * (domain.rows + 2);
}
std::span<std::uint8_t> extent(std::span<std::uint8_t> bytes, std::size_t size) {
    if (bytes.size() < size)
        throw std::invalid_argument("Material job storage extent is too small");
    return bytes.first(size);
}
class OwnedMaterialJobs final : public MaterialCacheMaterialBackend {
    struct Buffer {
        std::unique_ptr<std::uint8_t[]> data;
        std::size_t size = 0;
    };
    std::unique_ptr<MaterialCacheMaterialJob[]> records_;
    std::array<Buffer, 2> buffers_;
    std::size_t allocation_ = 0;

    [[noreturn]] static void unexpected() {
        throw std::logic_error("CPU material runner received an unrelated lifecycle operation");
    }

  public:
    MaterialPatch patch;
    // At most ceil(67/12)^2 submitted material tiles. Record storage still
    // contains one initialized native record per vertex, as preparation requires.
    std::array<MaterialCacheMaterialJob *, 36> queue{};
    std::size_t queued = 0;

    OwnedMaterialJobs(int columns, int rows) : patch{columns, rows, {}, {}} {}
    void discard() noexcept {
        records_.reset();
        for (auto &buffer : buffers_) {
            buffer.data.reset();
            buffer.size = 0;
        }
        queue.fill(nullptr);
        queued = 0;
    }
    MaterialCacheMaterialJobStorage allocate_material_records(std::size_t n) override {
        records_.reset(new (std::nothrow) MaterialCacheMaterialJob[n]);
        return {reinterpret_cast<std::uintptr_t>(records_.get()),
                records_ ? std::span<MaterialCacheMaterialJob>(records_.get(), n)
                         : std::span<MaterialCacheMaterialJob>{}};
    }
    std::uintptr_t allocate_staging(std::size_t size) override {
        auto &buffer = buffers_.at(allocation_++);
        // The public patch API defines untouched vertex lanes as zero, matching
        // the previous owned producer; no uninitialized bytes are published.
        buffer.data.reset(new (std::nothrow) std::uint8_t[size]{});
        buffer.size = size;
        return reinterpret_cast<std::uintptr_t>(buffer.data.get());
    }
    std::span<std::uint8_t> staging_memory(std::uintptr_t token) override {
        for (auto &buffer : buffers_)
            if (token && token == reinterpret_cast<std::uintptr_t>(buffer.data.get()))
                return {buffer.data.get(), buffer.size};
        throw std::logic_error("CPU material staging token is not owned");
    }
    void free_staging(std::uintptr_t token) override {
        if (!token)
            return;
        if (token == reinterpret_cast<std::uintptr_t>(records_.get())) {
            records_.reset();
            queue.fill(nullptr);
            queued = 0;
            return;
        }
        for (auto &buffer : buffers_)
            if (token == reinterpret_cast<std::uintptr_t>(buffer.data.get())) {
                buffer.data.reset();
                buffer.size = 0;
                return;
            }
        throw std::logic_error("CPU material release token is not owned");
    }
    void submit_material(MaterialCacheMaterialJob &job, std::uintptr_t) override {
        queue.at(queued++) = &job;
    }
    MaterialCacheAppendSpan append_material_stream(MaterialCacheStream stream, std::size_t n) override {
        if (stream == MaterialCacheStream::indices16) {
            patch.indices.resize(n);
            return {0, {reinterpret_cast<std::uint8_t *>(patch.indices.data()), n * sizeof(std::uint16_t)}};
        }
        auto &bytes =
            stream == MaterialCacheStream::material48 ? patch.streams.material48 : patch.streams.normal24;
        bytes.resize(n * (stream == MaterialCacheStream::material48 ? 48 : 24));
        return {0, bytes};
    }
    void rebuild_geometry(MaterialCacheLifecycleState &) override { unexpected(); }
    void generate_cache(MaterialCacheLifecycleState &, std::uint32_t, std::uintptr_t) override {
        unexpected();
    }
    void release_geometry_pair(std::uint32_t, std::uint32_t) override { unexpected(); }
    void release_vertex_buffer(std::uint32_t) override { unexpected(); }
    void release_index_buffer(std::uint32_t) override { unexpected(); }
    void release_packet(std::uint32_t) override { unexpected(); }
    void submit_geometry(MaterialCacheGeometryJob &, std::uintptr_t) override { unexpected(); }
    void prepare_material(MaterialCacheLifecycleState &, std::uintptr_t) override { unexpected(); }
};
} // namespace
struct MaterialCacheJobBatch::Impl {
    MaterialCacheLifecycleState &state;
    MaterialCacheLifecycleState isolated_state;
    MaterialCacheLifecycleState *storage;
    MaterialCachePreparationDomain domain;
    OwnedMaterialJobs jobs;
    bool executed = false, retained = false;

    Impl(MaterialCacheLifecycleState &s, int columns, int rows, bool independent_staging)
        : state(s), storage(independent_staging ? &isolated_state : &s),
          domain{columns - 2, rows - 2, 0}, jobs(columns, rows) {
        if (independent_staging) {
            isolated_state.textures = s.textures;
            isolated_state.prepared = s.prepared;
        }
    }
    MaterialCacheLifecycleState &batch_state() { return *storage; }
    void discard() noexcept {
        if (!retained)
            return;
        // No other material batch may replace these slots while this owner is
        // retained. Geometry tokens and lifecycle metadata belong to the caller.
        storage->staging[2] = storage->staging[3] = storage->staging[4] = 0;
        storage->prepared = false;
        jobs.discard();
        retained = false;
    }
    ~Impl() { discard(); }
};
MaterialCacheJobBatch::MaterialCacheJobBatch(MaterialCacheLifecycleState &state, int columns, int rows,
                                             std::uintptr_t dispatch)
    : MaterialCacheJobBatch(state, columns, rows, dispatch, false) {}
MaterialCacheJobBatch::MaterialCacheJobBatch(MaterialCacheLifecycleState &state, int columns, int rows,
                                             std::uintptr_t dispatch, bool independent_staging) {
    if (columns < 2 || columns > 67 || rows < 2 || rows > 67 || !state.prepared ||
        !state.textures.final_size || state.textures.final_size > INT32_MAX ||
        (!independent_staging && (state.staging[2] || state.staging[3] || state.staging[4])))
        throw std::invalid_argument("Invalid retained material batch owner or extent");
    auto owned = std::make_unique<Impl>(state, columns, rows, independent_staging);
    owned->retained = true;
    material_cache_prepare_material_jobs(owned->batch_state(), owned->domain, owned->jobs, dispatch);
    if (!owned->batch_state().prepared)
        throw std::bad_alloc();
    impl_ = std::move(owned);
}
MaterialCacheJobBatch::~MaterialCacheJobBatch() = default;
void MaterialCacheJobBatch::execute(const MaterialGeometryDomain &domain, const MaterialSelection &selection,
                                    const MaterialLookup &height_at, const MaterialNormalLookup &normal_at,
                                    const MaterialFactorLookup &factors_at) {
    auto &s = *impl_;
    if (!s.retained || s.executed || domain.columns != s.domain.columns + 2 ||
        domain.rows != s.domain.rows + 2)
        throw std::invalid_argument("Material batch is not awaiting workers for this domain");
    try {
        for (std::size_t i = 0; i < s.jobs.queued; ++i)
            material_cache_execute_material_job(*s.jobs.queue[i], domain, selection, s.jobs, height_at,
                                                normal_at, factors_at);
        s.executed = true;
    } catch (...) {
        discard();
        throw;
    }
}
MaterialPatch MaterialCacheJobBatch::append() {
    auto &s = *impl_;
    if (!s.retained || !s.executed)
        throw std::logic_error("Material append requires the completed worker barrier");
    try {
        material_cache_append_material_jobs(s.batch_state(), s.domain, s.jobs);
        s.retained = false;
        return std::move(s.jobs.patch);
    } catch (...) {
        discard();
        throw;
    }
}
void MaterialCacheJobBatch::discard() noexcept {
    if (impl_)
        impl_->discard();
}
const MaterialCacheLifecycleState &MaterialCacheJobBatch::state() const { return impl_->state; }
std::size_t MaterialCacheJobBatch::job_count() const { return impl_->jobs.queued; }
bool MaterialCacheJobBatch::executed() const { return impl_->executed; }
void material_cache_prepare_material_jobs(MaterialCacheLifecycleState &state,
                                          const MaterialCachePreparationDomain &domain,
                                          MaterialCacheMaterialBackend &backend, std::uintptr_t dispatch) {
    const auto vertices = count(domain);
    auto jobs = backend.allocate_material_records(vertices);
    state.staging[2] = jobs.token;
    state.staging[3] = backend.allocate_staging(vertices * 48);
    state.staging[4] = backend.allocate_staging(vertices * 24);
    if (!state.staging[3] || !state.staging[4] || !state.staging[2]) {
        backend.free_staging(state.staging[2]);
        backend.free_staging(state.staging[3]);
        backend.free_staging(state.staging[4]);
        state.staging[3] = 0;
        state.staging[4] = 0;
        state.staging[2] = 0;
        state.prepared = false;
        return;
    }
    if (jobs.records.size() != vertices)
        throw std::invalid_argument("Material job record count differs from its allocation");
    std::fill(jobs.records.begin(), jobs.records.end(), MaterialCacheMaterialJob{});
    std::size_t next = 0;
    for (int x = -1; x < domain.columns + 1; x += 12)
        for (int y = -1; y < domain.rows + 1; y += 12) {
            auto &job = jobs.records[next++];
            job = {&state,
                   state.staging[4],
                   {x, std::min(x + 12, domain.columns + 1), y, std::min(y + 12, domain.rows + 1)}};
            backend.submit_material(job, dispatch);
        }
}
void material_cache_execute_material_job(const MaterialCacheMaterialJob &job, MaterialGeometryDomain domain,
                                         const MaterialSelection &selection,
                                         MaterialCacheMaterialBackend &backend,
                                         const MaterialLookup &height_at,
                                         const MaterialNormalLookup &normal_at,
                                         const MaterialFactorLookup &factors_at) {
    if (!job.owner)
        throw std::invalid_argument("Material job has no owner");
    if (domain.columns < 2 || domain.columns > 67 || domain.rows < 2 || domain.rows > 67 ||
        !job.owner->textures.final_size || job.owner->textures.final_size > INT32_MAX)
        throw std::invalid_argument("Invalid material job geometry or cache size");
    const auto vertices = count({domain.columns - 2, domain.rows - 2, 0});
    domain.cache_size = static_cast<int>(job.owner->textures.final_size);
    write_material_tile_into(domain, job.tile,
                             {extent(backend.staging_memory(job.owner->staging[3]), vertices * 48),
                              extent(backend.staging_memory(job.normal_staging), vertices * 24)},
                             selection, 0, height_at, normal_at, factors_at);
}
void material_cache_append_material_jobs(MaterialCacheLifecycleState &state,
                                         const MaterialCachePreparationDomain &domain,
                                         MaterialCacheMaterialBackend &backend) {
    const auto vertices = count(domain);
    const auto indices = std::size_t(domain.columns + 1) * (domain.rows + 1) * 6;
    const auto material = backend.append_material_stream(MaterialCacheStream::material48, vertices);
    state.material_stream_offsets[0] = material.offset;
    const auto index = backend.append_material_stream(MaterialCacheStream::indices16, indices);
    state.material_stream_offsets[2] = index.offset;
    backend.free_staging(state.staging[2]);
    state.staging[2] = 0;
    const auto generated = terrain::land_grid_indices(domain.columns + 2, domain.rows + 2, 1);
    const auto index_destination = extent(index.bytes, indices * 2);
    std::memcpy(index_destination.data(), generated.data(), index_destination.size());
    const auto material_source = extent(backend.staging_memory(state.staging[3]), vertices * 48);
    const auto material_destination = extent(material.bytes, material_source.size());
    std::memcpy(material_destination.data(), material_source.data(), material_source.size());
    backend.free_staging(state.staging[3]);
    state.staging[3] = 0;
    const auto normal = backend.append_material_stream(MaterialCacheStream::normal24, vertices);
    state.material_stream_offsets[1] = normal.offset;
    const auto normal_source = extent(backend.staging_memory(state.staging[4]), vertices * 24);
    const auto normal_destination = extent(normal.bytes, normal_source.size());
    std::memcpy(normal_destination.data(), normal_source.data(), normal_source.size());
    backend.free_staging(state.staging[4]);
    state.staging[4] = 0;
}
MaterialPatch build_material_patch_jobs(const MaterialGeometryDomain &domain,
                                        const MaterialSelection &selection, const MaterialLookup &height_at,
                                        const MaterialNormalLookup &normal_at,
                                        const MaterialFactorLookup &factors_at) {
    if (domain.columns < 2 || domain.columns > 67 || domain.rows < 2 || domain.rows > 67 ||
        domain.cache_size <= 0)
        throw std::invalid_argument("Invalid owned material job domain");
    MaterialCacheLifecycleState state;
    state.prepared = true;
    state.textures.final_size = static_cast<std::uint32_t>(domain.cache_size);
    return build_material_patch_jobs(state, domain, selection, height_at, normal_at, factors_at);
}
MaterialPatch build_material_patch_jobs(MaterialCacheLifecycleState &state,
                                        const MaterialGeometryDomain &domain,
                                        const MaterialSelection &selection, const MaterialLookup &height_at,
                                        const MaterialNormalLookup &normal_at,
                                        const MaterialFactorLookup &factors_at) {
    if (domain.columns < 2 || domain.columns > 67 || domain.rows < 2 || domain.rows > 67 ||
        domain.cache_size <= 0 ||
        state.textures.final_size != static_cast<std::uint32_t>(domain.cache_size) || !state.prepared ||
        state.staging[2] || state.staging[3] || state.staging[4])
        throw std::invalid_argument("Invalid persistent material job owner or domain");
    MaterialCacheJobBatch batch(state, domain.columns, domain.rows);
    batch.execute(domain, selection, height_at, normal_at, factors_at);
    return batch.append();
}
} // namespace civ5::materials
