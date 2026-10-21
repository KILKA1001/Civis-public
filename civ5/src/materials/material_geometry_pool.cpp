#include "civ5/materials/material_geometry_pool.hpp"
#include <limits>
#include <stdexcept>

namespace civ5::materials {
void MaterialGeometryPool::take(MaterialGeometryBufferBackend &backend, MaterialGeometryBufferKind kind,
                                std::span<const std::uint8_t> bytes, std::uint32_t count, std::uint32_t &out) {
    auto &list = kind == MaterialGeometryBufferKind::vertex ? vertices_ : indices_;
    for (auto &entry : list)
        if (!entry.busy && entry.capacity >= count) {
            out = entry.handle;
            entry.busy = true;
            // Original wrappers copy the entire device allocation here, which
            // can read beyond staging. Only these bytes are used by the draw.
            backend.upload(kind, out, bytes);
            return;
        }
    if (list.size() == 1024)
        throw std::length_error("Original geometry pool capacity exhausted");
    list.reserve(list.size() + 1);
    const auto handle = backend.create(kind, count, bytes);
    if (!handle)
        throw std::runtime_error("Geometry backend returned a null buffer");
    list.push_back({handle, count, true});
    out = handle;
}
void MaterialGeometryPool::upload_pair(MaterialGeometryBufferBackend &backend,
                                       std::span<const std::uint8_t> vertices, std::uint32_t vertex_count,
                                       std::span<const std::uint8_t> indices, std::uint32_t index_count,
                                       std::uint32_t &vertex_buffer, std::uint32_t &index_buffer) {
    if (!vertex_count || !index_count || vertex_count > std::numeric_limits<std::int32_t>::max() ||
        index_count > std::numeric_limits<std::int32_t>::max() ||
        std::uint64_t(vertex_count) * 24 > vertices.size() || std::uint64_t(index_count) * 2 > indices.size())
        throw std::invalid_argument("Invalid geometry upload extent");
    take(backend, MaterialGeometryBufferKind::vertex, vertices.first(std::size_t(vertex_count) * 24),
         vertex_count, vertex_buffer);
    take(backend, MaterialGeometryBufferKind::index, indices.first(std::size_t(index_count) * 2),
         index_count, index_buffer);
}
void MaterialGeometryPool::release_pair(std::uint32_t vertex, std::uint32_t index) {
    for (auto &entry : vertices_)
        if (entry.handle == vertex) { entry.busy = false; break; }
    for (auto &entry : indices_)
        if (entry.handle == index) { entry.busy = false; break; }
}
std::span<const MaterialGeometryBufferEntry> MaterialGeometryPool::entries(MaterialGeometryBufferKind kind) const {
    return kind == MaterialGeometryBufferKind::vertex ? vertices_ : indices_;
}
} // namespace civ5::materials
