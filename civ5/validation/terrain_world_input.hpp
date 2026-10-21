#pragma once
#include "civ5/terrain/terrain_world.hpp"
#include "coast_map_io.hpp"
namespace {
std::string key(Reader &r) {
    const auto b = r.blob(); check(!b.empty() && b.size() <= 1024, "Invalid world source key");
    return {b.begin(), b.end()};
}
struct WorldRequest {
    TerrainWorldInput input;
    std::vector<std::uint32_t> mapping, keys, masks;
    Bytes cells, prior;
    std::array<std::uint32_t, 4> reciprocal;
    std::uint32_t fill, order;
};
WorldRequest request(Reader &r) {
    WorldRequest q;
    auto &v = q.input;
    v.map_width = r.integer(); v.map_height = r.integer(); v.mode = r.integer();
    for (auto &x : v.factory) x = r.integer();
    for (auto &m : v.orientations) for (auto &x : m) x = r.integer();
    v.maximum_pass = r.integer();
    v.random_states = {r.word(), r.word(), r.word()};
    const auto coast = precision(r), gates = precision(r), worker = precision(r), membership = precision(r);
    const auto ct = r.word(), gt = r.word(), mt = r.word(), power = r.word(), bounds = r.word();
    check(ct <= 1 && gt <= 1 && mt <= 2 && power <= 1 && bounds <= 1, "Unsupported world arithmetic path");
    v.precision = {coast, gates, worker, membership, ct ? Vec4Path::sse : Vec4Path::legacy_x87,
        gt ? Vec4Path::sse : Vec4Path::legacy_x87, static_cast<MatrixPath>(mt),
        power ? CoastPowPath::sse2 : CoastPowPath::legacy_x87,
        bounds ? CoastBoundsPrecision::double_or_extended : CoastBoundsPrecision::single};
    v.worker_bias = r.scalar(); v.wrap_x = flag(r);
    const auto outside = r.word(); check(outside <= 255, "Invalid outside world gate");
    v.outside_gate_category = static_cast<std::uint8_t>(outside);
    q.keys = palette(r);
    v.coast_family_prefix = r.word(); v.coast_common_prefix = r.word();
    const auto gate = r.word(); check(gate <= 65535, "Invalid world coast gate");
    v.coast_dispatch_gate = static_cast<std::uint16_t>(gate); v.coast_helper_return = r.word();
    q.mapping = palette(r);
    const auto cells = r.blob(), prior = r.blob();
    q.cells.assign(cells.begin(), cells.end()); q.prior.assign(prior.begin(), prior.end());
    q.masks = palette(r); q.fill = r.word();
    for (auto &x : q.reciprocal) x = r.word();
    q.order = r.word(); r.end();
    check(q.fill <= 255 && q.order <= 1, "Invalid world allocator/order state");
    return q; // Views are bound after the owning request reaches its final address.
}
} // namespace
