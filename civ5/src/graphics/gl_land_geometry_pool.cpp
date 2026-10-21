#include "civ5/graphics/gl_land_geometry_pool.hpp"
#include "civ5/graphics/gl_render_api.hpp"
#include <map>
#include <exception>
#include <limits>
#include <stdexcept>

namespace civ5::graphics {
namespace {
using Kind = materials::MaterialGeometryBufferKind;
void require(bool ok, const char *why) {
    if (!ok) throw std::invalid_argument(why);
}
}
struct GlLandGeometryPool::Impl final : materials::MaterialGeometryBufferBackend, materials::SharedLandGeometryBackend {
    struct Buffer { GLuint name; Kind kind; std::size_t bytes; std::uint64_t generation; bool busy; std::uint32_t references = 0; };
    std::shared_ptr<GlContext> context;
    std::shared_ptr<const std::uint8_t> identity = std::make_shared<const std::uint8_t>(0);
    GlRenderApi gl;
    materials::MaterialGeometryPool pool;
    materials::SharedLandGeometryCache shared_cache;
    struct SharedLease { std::array<std::uint32_t, 3> handles; unsigned retained = 0; };
    std::map<std::uint64_t, SharedLease> shared_leases;
    std::map<std::uint32_t, Buffer> buffers;
    std::uint32_t next_handle = 1;
    std::uint64_t next_generation = 1;
    explicit Impl(std::shared_ptr<GlContext> c) : context(std::move(c)), gl(*context) {}
    ~Impl() {
        try {
            context->api();
            for (auto &[id, buffer] : buffers) gl.DeleteBuffers(1, &buffer.name);
        } catch (...) { std::terminate(); }
    }
    std::uint32_t create(Kind kind, std::uint32_t, std::span<const std::uint8_t> bytes) override {
        auto [at, added] = buffers.emplace(next_handle, Buffer{0, kind, bytes.size(), 0, false});
        require(added, "Geometry handle collision");
        try {
            gl.CreateBuffers(1, &at->second.name);
            gl.NamedBufferData(at->second.name, bytes.size(), bytes.data(), GL_DYNAMIC_DRAW);
            context->check("LAND geometry buffer creation and upload");
        } catch (...) {
            gl.DeleteBuffers(1, &at->second.name);
            buffers.erase(at);
            throw;
        }
        return next_handle++;
    }
    void upload(Kind kind, std::uint32_t handle, std::span<const std::uint8_t> bytes) override {
        auto &buffer = buffers.at(handle);
        require(buffer.kind == kind && bytes.size() <= buffer.bytes, "Geometry reuse extent mismatch");
        gl.NamedBufferSubData(buffer.name, 0, bytes.size(), bytes.data());
        context->check("LAND geometry buffer reuse upload");
    }
    std::uint32_t create_shared(Kind kind, std::uint32_t count, std::span<const std::uint8_t> bytes) override {
        const auto handle = create(kind, count, bytes);
        buffers.at(handle).references = 1;
        return handle;
    }
    void discard_shared(Kind, std::uint32_t handle) noexcept override {
        const auto at = buffers.find(handle);
        if (at != buffers.end()) { gl.DeleteBuffers(1, &at->second.name); buffers.erase(at); }
    }
};
GlLandGeometryPool::GlLandGeometryPool(std::shared_ptr<GlContext> context) {
    require(bool(context), "Geometry pool requires an owned context");
    impl_ = std::make_unique<Impl>(std::move(context));
}
GlLandGeometryPool::~GlLandGeometryPool() = default;
GlLandGeometryLease GlLandGeometryPool::upload(const materials::LandMaterialMesh &mesh) {
    require(!mesh.vertices().empty() && !mesh.indices().empty(), "Moved-from geometry mesh");
    require(mesh.vertices().size() <= std::numeric_limits<GLsizei>::max() &&
                mesh.indices().size() <= std::numeric_limits<GLsizei>::max(), "Geometry draw count overflow");
    auto &s = *impl_;
    s.context->api();
    GlLandGeometryLease lease;
    lease.owner_identity_ = s.identity;
    lease.vertices_ = static_cast<std::uint32_t>(mesh.vertices().size());
    lease.indices_ = static_cast<std::uint32_t>(mesh.indices().size());
    lease.input_ = mesh.input();
    try {
        s.pool.upload_pair(s, {reinterpret_cast<const std::uint8_t *>(mesh.vertices().data()), mesh.vertices().size_bytes()},
                           lease.vertices_, {reinterpret_cast<const std::uint8_t *>(mesh.indices().data()), mesh.indices().size_bytes()},
                           lease.indices_, lease.vertex_, lease.index_);
    } catch (...) {
        s.pool.release_pair(lease.vertex_, lease.index_);
        throw;
    }
    lease.generation_ = s.next_generation++;
    for (auto handle : {lease.vertex_, lease.index_}) {
        auto &buffer = s.buffers.at(handle);
        buffer.generation = lease.generation_;
        buffer.busy = true;
    }
    return lease;
}
GlLandGeometryLease GlLandGeometryPool::shared_grid(std::uint32_t columns, std::uint32_t rows,
                                                   std::int32_t step, std::int32_t capability) {
    const auto lease = begin_shared_grid(columns, rows, step, capability);
    try {
        for (auto handle : {lease.vertex_, lease.control_, lease.index_}) retain_shared_buffer(lease, handle);
    } catch (...) {
        release(lease);
        throw;
    }
    return lease;
}
GlLandGeometryLease GlLandGeometryPool::begin_shared_grid(std::uint32_t columns, std::uint32_t rows,
                                                         std::int32_t step, std::int32_t capability) {
    auto &s = *impl_;
    s.context->api();
    const auto &entry = s.shared_cache.get(s, columns, rows, step, capability);
    GlLandGeometryLease lease;
    lease.owner_identity_ = s.identity;
    lease.vertex_ = entry.vertex_buffer;
    lease.index_ = entry.index_buffer;
    lease.control_ = entry.control_buffer;
    lease.vertices_ = columns * rows;
    lease.indices_ = entry.triangle_count * 3;
    lease.input_ = materials::LandVertexInput::local_grid;
    lease.shared_ = true;
    lease.generation_ = s.next_generation++;
    s.shared_leases.emplace(lease.generation_, Impl::SharedLease{std::array{lease.vertex_, lease.index_, lease.control_}});
    return lease;
}
void GlLandGeometryPool::retain_shared_buffer(const GlLandGeometryLease &lease, std::uint32_t handle) {
    if (!handle) return;
    auto &s = *impl_;
    require(lease.shared_ && lease.owner_identity_ == s.identity, "Foreign shared geometry reservation");
    s.context->api();
    const auto at = s.shared_leases.find(lease.generation_);
    require(at != s.shared_leases.end(), "Returned shared geometry reservation");
    auto &record = at->second;
    unsigned slot = 0;
    while (slot < record.handles.size() && record.handles[slot] != handle) ++slot;
    require(slot < record.handles.size() && !(record.retained & (1u << slot)), "Unknown or already retained shared buffer");
    auto &refs = s.buffers.at(handle).references;
    require(refs < UINT32_MAX, "Shared geometry reference overflow");
    ++refs;
    record.retained |= 1u << slot;
}
void GlLandGeometryPool::release_shared_buffer(const GlLandGeometryLease &lease, std::uint32_t handle) {
    if (!handle) return;
    auto &s = *impl_;
    require(lease.shared_ && lease.owner_identity_ == s.identity, "Foreign shared geometry reservation");
    s.context->api();
    const auto at = s.shared_leases.find(lease.generation_);
    require(at != s.shared_leases.end(), "Returned shared geometry reservation");
    auto &record = at->second;
    unsigned slot = 0;
    while (slot < record.handles.size() && record.handles[slot] != handle) ++slot;
    require(slot < record.handles.size() && (record.retained & (1u << slot)), "Unknown or unretained shared buffer");
    auto &refs = s.buffers.at(handle).references;
    require(refs > 1, "Shared cache base reference cannot be returned");
    --refs;
    record.retained &= ~(1u << slot);
    if (!record.retained) s.shared_leases.erase(at);
}
std::array<GLuint, 2> GlLandGeometryPool::buffers(const GlLandGeometryLease &lease, const GlContext &context) const {
    const auto &s = *impl_;
    require(lease.owner_identity_ == s.identity && s.context.get() == &context && lease.generation_ != 0,
            "Geometry lease belongs to a different pool/context");
    s.context->api();
    if (lease.shared_) {
        const auto at = s.shared_leases.find(lease.generation_);
        require(at != s.shared_leases.end() && at->second.handles == std::array{lease.vertex_, lease.index_, lease.control_} &&
                    at->second.retained == (lease.control_ ? 7u : 3u), "Returned or incompletely retained shared geometry lease");
        return {s.buffers.at(lease.vertex_).name, s.buffers.at(lease.index_).name};
    }
    std::array<GLuint, 2> result{};
    unsigned i = 0;
    for (auto handle : {lease.vertex_, lease.index_}) {
        const auto at = s.buffers.find(handle);
        require(at != s.buffers.end() && at->second.busy && at->second.generation == lease.generation_,
                "Returned or replaced geometry lease");
        result[i++] = at->second.name;
    }
    return result;
}
void GlLandGeometryPool::release(const GlLandGeometryLease &lease) {
    auto &s = *impl_;
    if (lease.shared_) {
        require(lease.owner_identity_ == s.identity, "Foreign shared geometry reservation");
        s.context->api();
        const auto at = s.shared_leases.find(lease.generation_);
        require(at != s.shared_leases.end(), "Returned shared geometry reservation");
        for (unsigned slot = 0; slot < 3; ++slot)
            if (at->second.retained & (1u << slot)) --s.buffers.at(at->second.handles[slot]).references;
        s.shared_leases.erase(at);
        return;
    }
    buffers(lease, *s.context);
    s.pool.release_pair(lease.vertex_, lease.index_);
    s.buffers.at(lease.vertex_).busy = false;
    s.buffers.at(lease.index_).busy = false;
}
std::vector<std::uint8_t> GlLandGeometryPool::read_buffer(const GlLandGeometryLease &lease, Kind kind) const {
    const auto &s = *impl_;
    const auto names = buffers(lease, *s.context);
    const auto &buffer = s.buffers.at(kind == Kind::vertex ? lease.vertex_ : lease.index_);
    std::vector<std::uint8_t> bytes(buffer.bytes);
    s.gl.GetNamedBufferSubData(names[kind == Kind::vertex ? 0 : 1], 0, bytes.size(), bytes.data());
    s.context->check("LAND geometry buffer readback");
    return bytes;
}
std::span<const materials::MaterialGeometryBufferEntry> GlLandGeometryPool::entries(Kind kind) const {
    return impl_->pool.entries(kind);
}
std::vector<std::uint8_t> GlLandGeometryPool::read_control_buffer(const GlLandGeometryLease &lease) const {
    const auto &s = *impl_;
    buffers(lease, *s.context);
    if (!lease.control_) return {};
    const auto &buffer = s.buffers.at(lease.control_);
    std::vector<std::uint8_t> bytes(buffer.bytes);
    s.gl.GetNamedBufferSubData(buffer.name, 0, bytes.size(), bytes.data());
    s.context->check("LAND shared control buffer readback");
    return bytes;
}
std::span<const materials::SharedLandGeometry> GlLandGeometryPool::shared_entries() const { return impl_->shared_cache.entries(); }
std::uint32_t GlLandGeometryPool::shared_references(std::uint32_t handle) const {
    if (!handle) return 0;
    const auto at = impl_->buffers.find(handle);
    require(at != impl_->buffers.end() && at->second.references, "Unknown shared geometry handle");
    return at->second.references;
}
} // namespace civ5::graphics
