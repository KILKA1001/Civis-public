#include "civ5/materials/terrain_decal_store.hpp"
#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>

namespace civ5::materials {
TerrainDecalStoreDomain terrain_decal_store_domain(float span_x, float span_y, float scale_x, float scale_y) {
    for (float f : {span_x, span_y, scale_x, scale_y})
        if (!std::isfinite(f) || f <= 0) throw std::invalid_argument("Invalid world decal domain input");
    TerrainDecalStoreDomain result{{0, 0, span_x / scale_x, span_y / scale_y}, 0, 0};
    const float x = span_x / (scale_x * 256.f), y = span_y / (scale_y * 256.f);
    if (!std::isfinite(result.bounds[2]) || !std::isfinite(result.bounds[3]) ||
        result.bounds[2] <= 0 || result.bounds[3] <= 0 ||
        !(x >= 0 && x < 1048576 && y >= 0 && y < 1048576))
        throw std::invalid_argument("Unbounded world decal grid");
    result.columns = static_cast<std::uint32_t>(x) + 1;
    result.rows = static_cast<std::uint32_t>(y) + 1;
    if (std::uint64_t(result.columns) * result.rows > 1048576)
        throw std::invalid_argument("Excessive world decal cells");
    return result;
}
namespace {
constexpr auto none = no_terrain_decal_slot;
template<class T> void prepare_slot(std::vector<T> &slots, std::uint32_t free) {
    if (free != none) return;
    if (slots.size() >= none) throw std::length_error("Terrain decal slot limit");
    if (slots.size() == slots.capacity()) slots.reserve(std::max<std::size_t>(1, slots.size()*2));
}
template<class T> std::uint32_t take(std::vector<T> &slots, std::uint32_t &free, T value) {
    const auto id = free == none ? static_cast<std::uint32_t>(slots.size()) : free;
    value.active = true;
    if (free == none) slots.push_back(value);
    else { free = slots[id].next; slots[id] = value; }
    return id;
}
template<class T> void release(std::vector<T> &slots, std::uint32_t &free, std::uint32_t id) {
    auto &r = slots[id]; r.active = false;
    if (id < free) { r.next = free; free = id; }
    else { r.next = slots[free].next; slots[free].next = id; }
}
template<class T> void append(std::vector<T> &slots, std::uint32_t &head, std::uint32_t &tail, std::uint32_t id) {
    slots[id].previous = tail; slots[id].next = none;
    if (tail != none) slots[tail].next = id;
    else head = id;
    tail = id;
}
template<class T> void unlink(std::vector<T> &slots, std::uint32_t &head, std::uint32_t &tail, std::uint32_t id) {
    const auto &r = slots[id];
    if (r.next != none) slots[r.next].previous = r.previous;
    else tail = r.previous;
    if (r.previous != none) slots[r.previous].next = r.next;
    else head = r.next;
}
// At most four records change during unlink + free-list insertion. Retain
// these before mutation so a throwing external refresh cannot orphan a slot.
template<class T> struct Undo {
    std::array<std::uint32_t, 4> ids;
    std::array<T, 4> values;
    Undo(const std::vector<T> &slots, std::uint32_t id, std::uint32_t free)
        : ids{id, slots[id].next, slots[id].previous, free} {
        for (std::size_t i = 0; i < ids.size(); ++i) if (ids[i] != none) values[i] = slots[ids[i]];
    }
    void restore(std::vector<T> &slots) const noexcept {
        for (std::size_t i = 0; i < ids.size(); ++i) if (ids[i] != none) slots[ids[i]] = values[i];
    }
};
std::int32_t cvtt(float v) {
    if (!(v >= -2147483648.f && v < 2147483648.f)) return std::numeric_limits<std::int32_t>::min();
    return static_cast<std::int32_t>(v);
}
} // namespace

TerrainDecalStore::TerrainDecalStore(std::array<float, 4> domain, std::uint32_t columns,
                                   std::uint32_t rows, std::uint32_t blank_specular)
    : domain_(domain), columns_(columns), rows_(rows), blank_(blank_specular) {
    if (!columns || !rows || std::uint64_t(columns)*rows > 1048576)
        throw std::invalid_argument("Terrain decal spatial dimensions");
    for (float v : domain) if (!std::isfinite(v)) throw std::invalid_argument("Terrain decal spatial domain");
    const float x = domain[2] + -domain[0], y = domain[3] + -domain[1];
    if (!(x > 0 && y > 0) || !std::isfinite(x) || !std::isfinite(y))
        throw std::invalid_argument("Terrain decal spatial extent");
    scales_ = {float(columns)/x, float(rows)/y};
    if (!std::isfinite(scales_.x) || !std::isfinite(scales_.y) || !(scales_.x > 0 && scales_.y > 0))
        throw std::invalid_argument("Terrain decal spatial scale");
    heads_.assign(std::size_t(columns)*rows, none);
}
std::uint32_t TerrainDecalStore::spatial_cell(terrain::Point2 p) const {
    const auto x = std::clamp(cvtt((-domain_[0] + p.x)*scales_.x), 0, int(columns_)-1);
    const auto y = std::clamp(cvtt((-domain_[1] + p.y)*scales_.y), 0, int(rows_)-1);
    return columns_*std::uint32_t(y)+std::uint32_t(x);
}
const TerrainDecalStoredRecord *TerrainDecalStore::find(std::uint32_t handle) const {
    if (handle == terrain::absent_terrain_decal) return nullptr;
    const auto id = handle & 0x7fffffffu;
    if (handle & 0x80000000u) {
        if (id >= large_.size() || !large_[id].active) return nullptr;
        const auto &r = large_[id]; return &batches_[r.batch].records[r.record];
    }
    if (id >= small_.size() || !small_[id].active) return nullptr;
    const auto &r = small_[id]; return &batches_[r.batch].records[r.record];
}
std::uint32_t TerrainDecalStore::submit(const terrain::TerrainDecalRecord &material,
                                      const terrain::TerrainDecalGeometry &geometry,
                                      std::uint8_t mode, const TerrainDecalRegionRefresh &refresh) {
    auto packed = pack_terrain_decal_geometry(geometry, domain_);
    if (!packed) return terrain::absent_terrain_decal;
    const auto retained_material = material;
    const auto &bounds = packed->bounds;
    const terrain::Point2 center{(bounds[0]+bounds[2])*.5f, (bounds[1]+bounds[3])*.5f};
    if (!std::isfinite(center.x) || !std::isfinite(center.y))
        throw std::invalid_argument("Terrain decal center overflow");
    const auto count = packed->vertices.size();
    std::uint32_t batch = 0;
    for (; batch < batches_.size(); ++batch) {
        if (batches_[batch].vertices.size()+count < 65535) break;
        if (batches_[batch].holes) {
            compact(batch);
            if (batches_[batch].vertices.size()+count < 65535) break;
        }
    }
    if (batch == batches_.size()) {
        if (batch >= none) throw std::length_error("Terrain decal batch limit");
        TerrainDecalBatch fresh;
        fresh.vertices.reserve(65535); fresh.records.reserve(256);
        batches_.push_back(std::move(fresh));
    }
    auto &b = batches_[batch];
    prepare_slot(b.records, b.free);
    if (packed->large) prepare_slot(large_, large_free_);
    else prepare_slot(small_, small_free_);
    if (refresh) refresh(bounds, false, retained_material.specular != 0 && retained_material.specular != blank_);
    TerrainDecalStoredRecord record;
    record.mode = (mode & 1) != 0;
    record.first_vertex = static_cast<std::uint32_t>(b.vertices.size());
    record.triangles = static_cast<std::uint32_t>(count/3);
    record.material = retained_material; record.bounds = bounds;
    const auto local = take(b.records, b.free, record);
    append(b.records, b.head, b.tail, local); ++b.live;
    b.vertices.insert(b.vertices.end(), packed->vertices.begin(), packed->vertices.end()); b.upload = true;
    if (packed->large) {
        TerrainDecalLargeRecord r; r.batch = batch; r.record = local;
        const auto id = take(large_, large_free_, r);
        append(large_, large_head_, large_tail_, id); ++large_live_;
        return id | 0x80000000u;
    }
    TerrainDecalSmallRecord r; r.batch = batch; r.record = local; r.center = center;
    const auto cell = spatial_cell(center); r.next = heads_[cell];
    const auto id = take(small_, small_free_, r);
    if (r.next != none) small_[r.next].previous = id;
    heads_[cell] = id; ++small_live_;
    return id;
}
void TerrainDecalStore::remove(std::uint32_t handle, const TerrainDecalRegionRefresh &refresh) {
    const auto *found = find(handle);
    if (!found) throw std::invalid_argument("Unknown terrain decal handle");
    const auto bounds = found->bounds;
    const auto id = handle & 0x7fffffffu;
    std::uint32_t batch, local;
    if (handle & 0x80000000u) {
        batch = large_[id].batch; local = large_[id].record;
        const Undo saved(large_, id, large_free_);
        const auto free = large_free_, head = large_head_, tail = large_tail_;
        unlink(large_, large_head_, large_tail_, id); release(large_, large_free_, id); --large_live_;
        try { if (refresh) refresh(bounds, false, false); }
        catch (...) {
            saved.restore(large_); large_free_ = free; large_head_ = head; large_tail_ = tail; ++large_live_;
            throw;
        }
    } else {
        const auto r = small_[id]; batch = r.batch; local = r.record;
        const auto cell = spatial_cell(r.center), head = heads_[cell], free = small_free_;
        const Undo saved(small_, id, small_free_);
        if (r.next != none) small_[r.next].previous = r.previous;
        if (r.previous != none) small_[r.previous].next = r.next;
        else heads_[cell] = r.next;
        release(small_, small_free_, id); --small_live_;
        try { if (refresh) refresh(bounds, false, false); }
        catch (...) { saved.restore(small_); heads_[cell] = head; small_free_ = free; ++small_live_; throw; }
    }
    auto &b = batches_[batch];
    unlink(b.records, b.head, b.tail, local); release(b.records, b.free, local); --b.live; b.holes = true;
    // Removal alone does not set the upload flag or shorten the vertex buffer.
}
void TerrainDecalStore::compact(std::uint32_t batch) {
    if (batch >= batches_.size()) throw std::invalid_argument("Unknown terrain decal batch");
    auto &b = batches_[batch]; std::uint32_t at = 0;
    for (auto id = b.head; id != none; id = b.records[id].next) {
        auto &r = b.records[id]; const auto count = r.triangles*3;
        if (at != r.first_vertex) {
            for (std::uint32_t i = 0; i < count; ++i) b.vertices[at+i] = b.vertices[r.first_vertex+i];
            r.first_vertex = at;
        }
        at += count;
    }
    b.vertices.resize(at); b.holes = false; b.upload = true;
}
void TerrainDecalStore::upload_pending(TerrainDecalBufferBackend &backend) {
    for (auto &b : batches_) {
        if (!b.upload) continue;
        if (!b.buffer) {
            b.buffer = backend.create();
            if (!b.buffer) throw std::runtime_error("Terrain decal backend returned a null buffer");
        }
        backend.upload(b.buffer, b.vertices);
        for (auto id = b.head; id != none; id = b.records[id].next) b.records[id].uploaded = true;
        b.upload = false;
    }
}
} // namespace civ5::materials
