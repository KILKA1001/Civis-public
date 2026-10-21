#include "civ5/materials/shared_land_geometry.hpp"
#include "civ5/terrain/land_grid.hpp"
#include <stdexcept>

namespace civ5::materials {
namespace {
template<class T> std::span<const std::uint8_t> bytes(std::span<const T> data) {
    return {reinterpret_cast<const std::uint8_t *>(data.data()), data.size_bytes()};
}
}
const SharedLandGeometry &SharedLandGeometryCache::get(SharedLandGeometryBackend &backend,
                                                      std::uint32_t columns, std::uint32_t rows,
                                                      std::int32_t step, std::int32_t capability) {
    for (const auto &entry : entries_)
        if (entry.columns == columns && entry.rows == rows) return entry;
    if (columns < 2 || columns > 65 || rows < 2 || rows > 65 || step < 1 || step > 3)
        throw std::invalid_argument("Invalid shared LAND geometry extent/step");
    const terrain::LandGrid grid(columns, rows, step);
    const auto controls = capability >= 10 ? terrain::land_control_stream(columns, rows) : terrain::LandControlStream{};
    entries_.reserve(entries_.size() + 1);
    SharedLandGeometry entry{columns, rows, static_cast<std::uint32_t>(grid.indices().size() / 3), 0, 0, 0};
    const auto create = [&](MaterialGeometryBufferKind kind, std::uint32_t count, std::span<const std::uint8_t> data) {
        const auto handle = backend.create_shared(kind, count, data);
        if (!handle) throw std::runtime_error("Shared geometry backend returned a null handle");
        return handle;
    };
    try {
        entry.index_buffer = create(MaterialGeometryBufferKind::index, static_cast<std::uint32_t>(grid.indices().size()), bytes(grid.indices()));
        entry.vertex_buffer = create(MaterialGeometryBufferKind::vertex, static_cast<std::uint32_t>(grid.vertices().size()), bytes(grid.vertices()));
        if (!controls.addresses.empty())
            entry.control_buffer = create(MaterialGeometryBufferKind::index, static_cast<std::uint32_t>(controls.addresses.size()),
                                           bytes(std::span<const std::uint16_t>(controls.addresses)));
    } catch (...) {
        if (entry.vertex_buffer) backend.discard_shared(MaterialGeometryBufferKind::vertex, entry.vertex_buffer);
        if (entry.index_buffer) backend.discard_shared(MaterialGeometryBufferKind::index, entry.index_buffer);
        if (entry.control_buffer) backend.discard_shared(MaterialGeometryBufferKind::index, entry.control_buffer);
        throw;
    }
    entries_.push_back(entry);
    return entries_.back();
}
} // namespace civ5::materials
