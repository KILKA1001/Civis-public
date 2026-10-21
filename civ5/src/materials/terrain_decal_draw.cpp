#include "civ5/materials/terrain_decal_draw.hpp"
#include <algorithm>
#include <bit>
#include <cmath>
#include <limits>
#include <stdexcept>

namespace civ5::materials {
namespace {
struct Selected { std::uint32_t batch, record; float priority; };
// 5812A0 and its original MSVC helpers. Equal priorities must retain the
// original partition exchanges; neither std::sort nor stable_sort specifies
// this order. Selected nonfinite priorities are rejected before sorting.
void sort_selected(std::span<Selected> v, std::size_t ideal) {
    const auto less = [](const auto &a, const auto &b) { return a.priority < b.priority; };
    const auto median = [&](std::size_t a, std::size_t b, std::size_t c) {
        if (less(v[b],v[a])) std::swap(v[a],v[b]);
        if (less(v[c],v[b])) std::swap(v[b],v[c]);
        if (less(v[b],v[a])) std::swap(v[a],v[b]);
    };
    while (v.size() > 32 && ideal) {
        const auto mid = v.size()/2, last = v.size()-1;
        if (last < 41) median(0,mid,last);
        else {
            const auto step = v.size()/8;
            median(0,step,2*step); median(mid-step,mid,mid+step);
            median(last-2*step,last-step,last); median(step,mid,last-step);
        }
        auto first_equal = mid, last_equal = mid+1;
        const auto equal = [&](std::size_t a, std::size_t b) { return !less(v[a],v[b]) && !less(v[b],v[a]); };
        while (first_equal && equal(first_equal-1,first_equal)) --first_equal;
        while (last_equal < v.size() && equal(last_equal,first_equal)) ++last_equal;
        auto left = first_equal, right = last_equal;
        for (;;) {
            for (; right < v.size(); ++right) {
                if (less(v[first_equal],v[right])) continue;
                if (less(v[right],v[first_equal])) break;
                std::swap(v[last_equal++],v[right]);
            }
            for (; left > 0; --left) {
                if (less(v[left-1],v[first_equal])) continue;
                if (less(v[first_equal],v[left-1])) break;
                std::swap(v[--first_equal],v[left-1]);
            }
            if (!left && right == v.size()) break;
            if (!left) {
                if (last_equal != right) std::swap(v[first_equal],v[last_equal]);
                ++last_equal; std::swap(v[first_equal++],v[right++]);
            } else if (right == v.size()) {
                if (--left != --first_equal) std::swap(v[left],v[first_equal]);
                std::swap(v[first_equal],v[--last_equal]);
            } else std::swap(v[--left],v[right++]);
        }
        ideal = ideal/2+(ideal/2)/2;
        if (first_equal < v.size()-last_equal) {
            sort_selected(v.first(first_equal),ideal); v = v.subspan(last_equal);
        } else { sort_selected(v.subspan(last_equal),ideal); v = v.first(first_equal); }
    }
    if (v.size() > 32) {
        const auto adjust = [&](std::size_t hole, std::size_t length, Selected value) {
            const auto top = hole;
            auto child = hole*2+2;
            while (child < length) {
                if (less(v[child],v[child-1])) --child;
                v[hole] = v[child]; hole = child; child = hole*2+2;
            }
            if (child == length) { v[hole] = v[length-1]; hole = length-1; }
            while (hole > top) {
                const auto parent = (hole-1)/2;
                if (!less(v[parent],value)) break;
                v[hole] = v[parent]; hole = parent;
            }
            v[hole] = value;
        };
        for (auto parent = v.size()/2; parent; ) { --parent; adjust(parent,v.size(),v[parent]); }
        for (auto length = v.size(); length > 1; ) {
            const auto value = v[--length]; v[length] = v[0]; adjust(0,length,value);
        }
    } else {
        for (std::size_t i = 1; i < v.size(); ++i) {
            const auto value = v[i]; auto j = i;
            while (j && less(value,v[j-1])) { v[j] = v[j-1]; --j; }
            v[j] = value;
        }
    }
}
std::int32_t cvtt(float v) {
    if (!(v >= -2147483648.f && v < 2147483648.f)) return std::numeric_limits<std::int32_t>::min();
    return static_cast<std::int32_t>(v);
}
} // namespace
TerrainDecalDrawList prepare_terrain_decal_draws(
    const TerrainDecalStore &store, std::array<float,4> bounds, std::uint32_t mode,
    std::uint32_t available, const std::array<float,16> &matrix, std::uint32_t index_buffer) {
    for (float v : bounds) if (!std::isfinite(v)) throw std::invalid_argument("Terrain decal query bounds");
    if (bounds[2] < bounds[0] || bounds[3] < bounds[1]) throw std::invalid_argument("Terrain decal inverted query");
    const std::array<float,4> expanded{bounds[0]-256.f,bounds[1]-256.f,bounds[2]+256.f,bounds[3]+256.f};
    const auto domain = store.domain(); const auto scale = store.scales();
    const auto x0 = std::max(0,cvtt((-domain[0]+expanded[0])*scale.x));
    const auto y0 = std::max(0,cvtt((-domain[1]+expanded[1])*scale.y));
    const auto upper = [](float v, std::uint32_t size) {
        // CVTTSS2SI followed by wrapping ADD 1, then signed max(0)/min(size).
        const auto bits = static_cast<std::uint32_t>(cvtt(v))+1u;
        return std::min(std::max(0,std::bit_cast<std::int32_t>(bits)),static_cast<int>(size));
    };
    const auto x1 = upper((-domain[0]+expanded[2])*scale.x,store.columns());
    const auto y1 = upper((-domain[1]+expanded[3])*scale.y,store.rows());
    if (x0 >= static_cast<int>(store.columns()) || y0 >= static_cast<int>(store.rows()))
        throw std::invalid_argument("Terrain decal query starts beyond spatial grid");
    std::vector<Selected> selected;
    const auto append = [&](std::uint32_t batch, std::uint32_t record, bool small) {
        const auto &b = store.batches()[batch]; const auto &r = b.records[record];
        if (!r.uploaded || std::uint32_t(r.mode) != mode || (small && b.upload)) return;
        if (r.bounds[0] <= bounds[2] && bounds[0] <= r.bounds[2] &&
            r.bounds[1] <= bounds[3] && bounds[1] <= r.bounds[3]) {
            if (!std::isfinite(r.material.priority)) throw std::invalid_argument("Terrain decal draw priority");
            selected.push_back({batch,record,r.material.priority});
        }
    };
    // 4A1AD0/476320 visits the initial cell even for an empty integer range.
    // Subsequent cells increment X, then Y. Centers use half-open bounds.
    auto x = x0, y = y0;
    for (;;) {
        auto id = store.cell_heads()[std::size_t(y)*store.columns()+std::size_t(x)];
        while (id != no_terrain_decal_slot) {
            const auto &s = store.small_records()[id];
            if (expanded[0] <= s.center.x && s.center.x < expanded[2] &&
                expanded[1] <= s.center.y && s.center.y < expanded[3]) append(s.batch,s.record,true);
            id = s.next;
        }
        if (++x >= x1) { x = x0; if (++y >= y1) break; }
    }
    for (auto id = store.large_head(); id != no_terrain_decal_slot; id = store.large_records()[id].next) {
        const auto &r = store.large_records()[id]; append(r.batch,r.record,false);
    }
    TerrainDecalDrawList result;
    result.selected = static_cast<std::uint32_t>(selected.size());
    result.capacity_exhausted = result.selected >= available-1000u;
    if (result.capacity_exhausted) return result;
    sort_selected(selected,selected.size());
    result.draws.reserve(selected.size());
    for (const auto &s : selected) {
        const auto &b = store.batches()[s.batch]; const auto &r = b.records[s.record];
        TerrainDecalDraw draw;
        draw.batch = s.batch; draw.record = s.record;
        draw.index_buffer = index_buffer; draw.vertex_buffer = b.buffer;
        draw.first_vertex = r.first_vertex; draw.triangles = r.triangles;
        draw.albedo = static_cast<std::uint16_t>(r.material.albedo);
        draw.heightmap = static_cast<std::uint16_t>(r.material.heightmap);
        std::copy(matrix.begin(),matrix.end(),draw.constants.begin());
        draw.constants[16] = draw.constants[17] = 1.f;
        std::copy(r.bounds.begin(),r.bounds.end(),draw.constants.begin()+20);
        draw.constants[24] = r.material.priority;
        result.draws.push_back(draw);
    }
    return result;
}
} // namespace civ5::materials
