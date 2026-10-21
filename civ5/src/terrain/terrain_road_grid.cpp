#include "civ5/terrain/terrain_road_grid.hpp"
#include <algorithm>
#include <bit>
#include <cmath>
#include <limits>
#include <stdexcept>

namespace civ5::terrain {
namespace {
void require(bool ok, const char *message) {
    if (!ok) throw std::invalid_argument(message);
}
void shape(const TerrainRoadGrid &g) {
    require(g.width > 0 && g.height > 0 && std::uint64_t(g.width)*std::uint64_t(g.height) <= 65535 &&
        g.cells.size() == std::size_t(g.width)*std::size_t(g.height) && g.members.size() == g.cells.size() &&
        std::all_of(g.bounds.begin(), g.bounds.end(), [](float v) { return std::isfinite(v); }) &&
        std::isfinite(g.span_x) && std::isfinite(g.span_y) && g.span_x > 0 && g.span_y > 0 &&
        g.span_x == g.bounds[2]+-g.bounds[0] && g.span_y == g.bounds[3]+-g.bounds[1],
        "Invalid terrain road grid");
}
TerrainRoadSegment &segment(TerrainRoadGrid &g, std::uint32_t index) {
    require(index < g.segments.size(), "Missing terrain road segment");
    auto &s = g.segments[index];
    require(!s.points.empty(), "Empty terrain road segment");
    for (auto p : s.points)
        require(std::isfinite(p.x) && std::isfinite(p.y), "Nonfinite terrain road point");
    return s;
}
std::int32_t truncate(float value) {
    require(value >= -0x1p31f && value < 0x1p31f, "Unrepresentable terrain road cell");
    return static_cast<std::int32_t>(value);
}
void membership_words(TerrainRoadGrid &g, std::size_t i) {
    const auto &m = g.members[i];
    g.cells[i].geometry = {0, static_cast<std::uint32_t>(m.size()),
        m.empty() ? absent_terrain_road_segment : m.front(),
        m.empty() ? absent_terrain_road_segment : m.back()};
}
} // namespace
TerrainRoadGrid make_terrain_road_grid(std::int32_t width, std::int32_t height,
                                       std::array<float, 4> bounds) {
    require(width > 0 && height > 0 && std::uint64_t(width)*std::uint64_t(height) <= 65535 &&
        std::all_of(bounds.begin(), bounds.end(), [](float v) { return std::isfinite(v); }),
        "Invalid terrain road dimensions/bounds");
    TerrainRoadGrid result{width, height, bounds, bounds[2]+-bounds[0], bounds[3]+-bounds[1], {}, {}, {}, {}};
    require(std::isfinite(result.span_x) && std::isfinite(result.span_y) && result.span_x > 0 && result.span_y > 0,
            "Invalid terrain road spans");
    result.cells.resize(std::size_t(width)*std::size_t(height),
        {{0, 0, absent_terrain_road_segment, absent_terrain_road_segment}, absent_terrain_decal, 0, {0, 0, 0}});
    result.members.resize(result.cells.size());
    return result;
}
TerrainRoadGrid make_terrain_road_grid(const WorldLayout &world) {
    require(world.source_grid.width > 0 && world.source_grid.width <= 32767 &&
        world.source_grid.height > 0 && world.source_grid.height <= 32767, "Invalid terrain road source grid");
    return make_terrain_road_grid(world.source_grid.width*2, world.source_grid.height*2,
        {world.bounds[0], world.bounds[1], world.bounds[3], world.bounds[4]});
}
void shift_terrain_road_segment(TerrainRoadGrid &g, std::uint32_t index, std::int32_t wrap_count) {
    shape(g);
    auto &s = segment(g, index);
    const float shift = static_cast<float>(wrap_count)*g.span_x;
    require(std::isfinite(shift), "Terrain road shift overflow");
    for (auto &p : s.points) p.x = std::max(std::min(shift+p.x, g.bounds[2]), g.bounds[0]);
}
void publish_terrain_road_segment(TerrainRoadGrid &g, std::uint32_t index,
                                   std::uint32_t group, std::uint8_t type, std::uint8_t flags) {
    shape(g);
    auto &s = segment(g, index);
    for (auto node : {s.first_junction, s.last_junction}) {
        require(node < g.junctions.size() && g.junctions[node].size() <= 4, "Invalid terrain road junction");
        for (auto link : g.junctions[node]) {
            require(link.boundary <= 3, "Invalid terrain road boundary flag");
            if (!link.boundary) segment(g, link.segment);
        }
    }
    if (s.cell != absent_terrain_road_cell) {
        require(s.cell < g.members.size() && std::count(g.members[s.cell].begin(), g.members[s.cell].end(), index) == 1,
                "Invalid prior terrain road membership");
    }
    const auto p = s.points[s.points.size()/2];
    auto x = truncate((static_cast<float>(g.width)/g.span_x)*(-g.bounds[0]+p.x));
    const auto y = truncate((static_cast<float>(g.height)/g.span_y)*(p.y+-g.bounds[1]));
    if (x == g.width) --x;
    std::uint16_t cell = absent_terrain_road_cell;
    if (y < g.height) {
        require(x >= 0 && x < g.width && y >= 0, "Terrain road midpoint outside cell storage");
        cell = static_cast<std::uint16_t>(x*g.height+y);
        // Reserve before touching published state so allocation failure cannot
        // strand a segment between lists. Admitted native allocation succeeds.
        g.members[cell].reserve(g.members[cell].size()+1);
    }
    if (s.cell != absent_terrain_road_cell) {
        auto &old = g.members[s.cell];
        old.erase(std::find(old.begin(), old.end(), index));
        membership_words(g, s.cell);
    }
    s.group = group; s.type = type; s.flags = flags; s.cell = cell;
    if (cell != absent_terrain_road_cell) {
        g.members[cell].push_back(index); membership_words(g, cell); g.cells[cell].dirty = 1;
    }
    constexpr auto maximum = std::numeric_limits<float>::max();
    s.bounds = {maximum, maximum, -maximum, -maximum};
    for (auto q : s.points) {
        s.bounds[0] = std::min(q.x, s.bounds[0]); s.bounds[1] = std::min(q.y, s.bounds[1]);
        s.bounds[2] = std::max(q.x, s.bounds[2]); s.bounds[3] = std::max(q.y, s.bounds[3]);
    }
    // Snapshotting is implicit: links themselves are not changed by snapping.
    for (auto node : {s.first_junction, s.last_junction}) {
        Point2 anchor{0, 0}; bool first = true;
        for (auto link : g.junctions[node]) {
            if (link.boundary) continue;
            auto &points = g.segments[link.segment].points;
            auto &endpoint = link.first ? points.front() : points.back();
            if (first) { anchor = endpoint; first = false; } else endpoint = anchor;
        }
    }
}
void refresh_terrain_road_decals(TerrainRoadGrid &g, TerrainDecalMapView map,
    const TerrainDecalPalette &palette, TerrainRandom &random,
    const TerrainDecalRemove &remove, const TerrainDecalSubmit &submit) {
    shape(g);
    const auto range = palette.ranges[0];
    require(submit && range.first < palette.records.size() && range.end >= range.first &&
        range.end <= palette.records.size(), "Invalid terrain road decal palette/submit");
    require(map.width >= 0 && map.height >= 0 && std::uint64_t(map.width)*std::uint64_t(map.height) == map.flags.size() &&
        std::isfinite(map.offset_x) && std::isfinite(map.offset_y), "Invalid terrain road decal mask");
    refresh_terrain_decals(g.width, g.height, g.cells, remove, [&](std::size_t cell, const TerrainDecalCell &) {
        std::vector<TerrainDecalPolylineView> lines;
        lines.reserve(g.members[cell].size());
        for (auto index : g.members[cell]) lines.emplace_back(segment(g, index).points);
        auto mesh = build_terrain_decal_geometry_views(lines, map);
        if (mesh.indices.empty()) return absent_terrain_decal;
        const auto selected = select_terrain_decal(range, random);
        return submit(cell, selected, palette.records[selected], mesh);
    });
}
void dirty_terrain_road_region(TerrainRoadGrid &g,std::array<float,4> bounds,Point2 offset,
    const TerrainRoadDirtyObserver &observer) {
    shape(g);
    require(std::isfinite(offset.x) && std::isfinite(offset.y) &&
        std::all_of(bounds.begin(),bounds.end(),[](float v){return std::isfinite(v);}),
        "Invalid terrain road region");
    // Original reciprocal and multiplication order differs from width/span.
    const float sx=(1.f/g.span_x)*static_cast<float>(g.width);
    const float sy=(1.f/g.span_y)*static_cast<float>(g.height);
    const auto x0=truncate(sx*(offset.x+bounds[0])),y0=truncate(sy*(offset.y+bounds[1]));
    const auto last_x=truncate(sx*(offset.x+bounds[2])),last_y=truncate(sy*(offset.y+bounds[3]));
    require(last_x<std::numeric_limits<std::int32_t>::max() && last_y<std::numeric_limits<std::int32_t>::max(),
        "Terrain road region upper bound overflow");
    const auto x1=last_x+1,y1=last_y+1;
    if (x0>=x1 || y0>=y1) return;
    const auto nx=std::uint64_t(std::int64_t(x1)-x0),ny=std::uint64_t(std::int64_t(y1)-y0);
    require(nx<=16777216 && ny<=16777216 && nx*ny<=16777216,"Terrain road region exceeds bounded work");
    const auto cell=[&](std::int32_t x,std::int32_t y){return std::int64_t(x%g.width)*g.height+y%g.height;};
    // Signed remainder can address before the field for negative inputs. Keep
    // valid native addresses, but never turn this into a positive-wrap repair.
    for (auto y=y0;y<y1;++y) for (auto x=x0;x<x1;++x) {
        const auto at=cell(x,y);
        require(at>=0 && std::uint64_t(at)<g.cells.size(),"Terrain road region outside cell storage");
    }
    for (auto y=y0;y<y1;++y) for (auto x=x0;x<x1;++x) {
        const auto at=static_cast<std::size_t>(cell(x,y));
        g.cells[at].dirty=1;
        if (observer) observer(at);
    }
}
void refresh_terrain_road_region(TerrainRoadGrid &g,std::array<float,4> bounds,TerrainDecalMapView map,
    const TerrainDecalPalette &palette,TerrainRandom &random,
    const TerrainDecalRemove &remove,const TerrainDecalSubmit &submit,const TerrainRoadDirtyObserver &observer) {
    dirty_terrain_road_region(g,bounds,{map.offset_x,map.offset_y},observer);
    refresh_terrain_road_decals(g,map,palette,random,remove,submit);
}
TerrainRoadRegionCommand make_terrain_road_region_command(std::array<float,4> bounds) {
    TerrainRoadRegionCommand packet{};
    const auto put=[&](std::size_t offset,std::uint32_t value){
        for(unsigned byte=0;byte<4;++byte)
            packet[offset+byte]=static_cast<std::uint8_t>(value>>(8*byte));
    };
    put(0,0x80);put(4,0x21);
    for(unsigned lane=0;lane<4;++lane)
        put(16+4*lane,std::bit_cast<std::uint32_t>(bounds[lane]));
    return packet;
}
void dispatch_terrain_road_region_command(const TerrainRoadRegionCommand &packet,
    TerrainRoadGrid &grid,TerrainDecalMapView map,
    const TerrainDecalPalette &palette,TerrainRandom &random,
    const TerrainDecalRemove &remove,const TerrainDecalSubmit &submit,
    const TerrainRoadDirtyObserver &observer) {
    const std::span bytes(packet);
    const auto word=[&](std::size_t offset){
        std::uint32_t value=0;
        for(unsigned byte=0;byte<4;++byte)
            value|=std::uint32_t(bytes[offset+byte])<<(8*byte);
        return value;
    };
    require(word(0)==0x80 && word(4)==0x21 && word(8)==0 && word(12)==0,
        "Invalid terrain road region command");
    dispatch_terrain_road_region_payload(bytes.subspan(16),grid,map,palette,random,
        remove,submit,observer);
}
void dispatch_terrain_road_region_payload(std::span<const std::uint8_t> payload,
    TerrainRoadGrid &grid,TerrainDecalMapView map,
    const TerrainDecalPalette &palette,TerrainRandom &random,
    const TerrainDecalRemove &remove,const TerrainDecalSubmit &submit,
    const TerrainRoadDirtyObserver &observer) {
    require(payload.size()>=16,"Truncated terrain road region payload");
    std::array<float,4> bounds{};
    for(unsigned lane=0;lane<4;++lane){
        std::uint32_t value=0;
        for(unsigned byte=0;byte<4;++byte)
            value|=std::uint32_t(payload[4*lane+byte])<<(8*byte);
        bounds[lane]=std::bit_cast<float>(value);
    }
    refresh_terrain_road_region(grid,bounds,map,palette,random,remove,submit,observer);
}
} // namespace civ5::terrain
