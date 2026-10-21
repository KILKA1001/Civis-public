#include "civ5/terrain/coast_road_network.hpp"
#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace civ5::terrain {
namespace {
constexpr float equal_distance = std::bit_cast<float>(0x2edbe6feu);
constexpr float intersection_tolerance = std::bit_cast<float>(0x38d1b717u);
constexpr float simplify_tolerance = std::bit_cast<float>(0x3b83126fu);
constexpr auto no_node = absent_terrain_road_cell;
constexpr auto no_segment = absent_terrain_decal;
void require(bool ok, const char *message) { if (!ok) throw std::invalid_argument(message); }
float squared(Point2 a, Point2 b) {
    const float x = a.x-b.x, y = a.y-b.y;
    return y*y+x*x;
}
struct Arithmetic {
    X87Precision precision;
    double r(double v) const { return round_x87(v, precision); }
    double length_squared(float x, float y) const { return r(r(double(y)*y)+r(double(x)*x)); }
    double distance(Point2 a, Point2 b, Point2 p) const {
        const float dx = b.x+-a.x, dy = b.y+-a.y;
        const float ex = p.x+-b.x, ey = p.y+-b.y, sx = p.x+-a.x, sy = p.y+-a.y;
        if (a != b && dy*ey+ex*dx < 0) {
            if (sy*-dy+sx*-dx >= 0) return length_squared(sx, sy);
            const auto cross = r(r(double(sy)*dx)-r(double(sx)*dy));
            return std::abs(r(r(cross*cross)/length_squared(dx, dy)));
        }
        return length_squared(ex, ey);
    }
    void append(TerrainDecalPolyline &line, Point2 p) const {
        if (line.size() == 1) { if (line.back() != p) line.push_back(p); return; }
        if (line.size() > 1) {
            const auto previous = line[line.size()-2];
            if (line.back() != p && previous == p) line.push_back(p);
            if (distance(p, previous, line.back()) < simplify_tolerance) { line.back() = p; return; }
        }
        line.push_back(p);
    }
    void prepend(TerrainDecalPolyline &line, Point2 p) const {
        if (line.size() == 1) { if (line.front() != p) line.insert(line.begin(), p); return; }
        if (line.size() > 1) {
            const auto next = line[1];
            // Original 4C90B0 compares against the LAST endpoint here and can
            // append before replacing/prepending the first endpoint.
            if (line.back() != p && next == p) line.push_back(p);
            if (distance(p, next, line.front()) < simplify_tolerance) { line.front() = p; return; }
        }
        line.insert(line.begin(), p);
    }
};
bool intersection(Point2 a, Point2 b, Point2 c, Point2 d, float tolerance, Point2 &out) {
    const float ax = a.x-b.x, ay = b.y-a.y, av = a.x*ay+a.y*ax;
    const float cy = d.y-c.y, cx = c.x-d.x, cv = c.x*cy+c.y*cx;
    const float determinant = cx*ay-cy*ax;
    if (std::abs(determinant) < tolerance) return false;
    const float inverse = 1.f/determinant;
    out = {(cx*av-cv*ax)*inverse, (cv*ay-cy*av)*inverse};
    return out.x <= std::min(std::max(a.x,b.x),std::max(c.x,d.x))+tolerance &&
        std::max(std::min(a.x,b.x),std::min(c.x,d.x))-tolerance <= out.x &&
        out.y <= std::min(std::max(a.y,b.y),std::max(c.y,d.y))+tolerance &&
        std::max(std::min(a.y,b.y),std::min(c.y,d.y))-tolerance <= out.y;
}
Point2 closest(Point2 a, Point2 b, Point2 p, Arithmetic arithmetic) {
    const float dx = a.x-b.x, dy = b.y-a.y;
    const float length = static_cast<float>(arithmetic.r(std::sqrt(arithmetic.length_squared(dx, dy))));
    const float inverse = 1.f/length;
    const Point2 normal{(inverse*dy)*1000.f, (inverse*dx)*1000.f};
    Point2 result{0,0};
    if (intersection(a, b, {p.x+normal.x,p.y+normal.y}, {p.x-normal.x,p.y-normal.y},
                     intersection_tolerance, result)) return result;
    const auto first = arithmetic.r(std::sqrt(arithmetic.length_squared(a.x-p.x, a.y-p.y)));
    const auto last = arithmetic.r(std::sqrt(arithmetic.length_squared(b.x-p.x, b.y-p.y)));
    return first < last ? a : b;
}
struct Crossing {
    std::uint8_t kind;
    std::uint32_t segment, edge, group;
    float distance;
    Point2 point;
};
// Original MSVC partition/sort order, including equal-key exchanges.
void sort_crossings(std::span<Crossing> v, std::size_t ideal) {
    const auto less = [](const auto &a, const auto &b) { return a.distance < b.distance; };
    const auto median = [&](std::size_t a, std::size_t b, std::size_t c) {
        if (less(v[b],v[a])) std::swap(v[a],v[b]);
        if (less(v[c],v[b])) std::swap(v[b],v[c]);
        if (less(v[b],v[a])) std::swap(v[a],v[b]);
    };
    while (v.size() > 32 && ideal) {
        const auto mid = v.size()/2, last = v.size()-1;
        if (last < 41) median(0, mid, last);
        else {
            const auto step = v.size()/8;
            median(0, step, 2*step); median(mid-step,mid,mid+step);
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
            sort_crossings(v.first(first_equal),ideal); v = v.subspan(last_equal);
        } else { sort_crossings(v.subspan(last_equal),ideal); v = v.first(first_equal); }
    }
    if (v.size() > 32) {
        // The original heap's equal-child choice is the right child.
        const auto adjust = [&](std::size_t hole, std::size_t length, Crossing value) {
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
class Builder {
    TerrainRoadGrid &g;
    Arithmetic arithmetic;
    std::uint32_t group;
    std::int32_t wrap = 0;
    std::uint16_t node() {
        require(g.junctions.size() < no_node, "Coast road junction capacity");
        const auto index = static_cast<std::uint16_t>(g.junctions.size());
        g.junctions.emplace_back(); return index;
    }
    std::uint32_t create(std::span<const Point2> points, std::uint16_t first, std::uint16_t last,
                         std::uint8_t first_boundary, std::uint8_t last_boundary) {
        if (first == no_node) first = node();
        if (last == no_node) last = node();
        if (g.junctions[first].size() >= 4 || g.junctions[last].size() >= 4) return no_segment;
        require(g.segments.size() < absent_terrain_road_segment, "Coast road segment capacity");
        const auto index = static_cast<std::uint32_t>(g.segments.size());
        TerrainRoadSegment segment{{},first,last};
        for (auto p : points) arithmetic.append(segment.points,p);
        g.segments.push_back(std::move(segment));
        g.junctions[first].push_back({index,true,first_boundary});
        g.junctions[last].push_back({index,false,last_boundary});
        return index;
    }
    std::uint16_t split(const Crossing &hit) {
        const auto index = hit.segment;
        auto &original = g.segments[index];
        if (squared(original.points.back(),hit.point) < equal_distance) return original.last_junction;
        if (squared(original.points.front(),hit.point) < equal_distance) return original.first_junction;
        const auto last = original.last_junction, added_node = node();
        const auto added = static_cast<std::uint32_t>(g.segments.size());
        TerrainRoadSegment segment{{original.points.begin()+hit.edge+1,original.points.end()},added_node,last};
        segment.group = original.group;
        original.points.erase(original.points.begin()+hit.edge+1,original.points.end());
        arithmetic.prepend(segment.points,hit.point); arithmetic.append(original.points,hit.point);
        auto &old_node = g.junctions[last];
        const auto link = std::find_if(old_node.begin(),old_node.end(),[&](auto l) { return l.segment == index; });
        require(link != old_node.end(), "Missing coast road endpoint link");
        const auto boundary = link->boundary;
        old_node.erase(link);
        original.last_junction = added_node;
        g.junctions[added_node] = {{index,false,0},{added,true,0}};
        old_node.push_back({added,false,boundary});
        const auto old_group = original.group;
        g.segments.push_back(std::move(segment));
        publish_terrain_road_segment(g,index,old_group,0,0);
        publish_terrain_road_segment(g,added,old_group,0,0);
        return added_node;
    }
    std::vector<Crossing> crossings(Point2 a, Point2 b) const {
        std::vector<Crossing> hits;
        const std::array<float,4> bounds{std::min(a.x,b.x)-20.f,std::min(a.y,b.y)-20.f,
                                       std::max(a.x,b.x)+20.f,std::max(a.y,b.y)+20.f};
        for (std::size_t i = 0; i < g.segments.size(); ++i) {
            const auto &s = g.segments[i];
            if (bounds[0] > s.bounds[2] || s.bounds[0] > bounds[2] ||
                bounds[1] > s.bounds[3] || s.bounds[1] > bounds[3]) continue;
            for (std::size_t edge = 0; edge+1 < s.points.size(); ++edge) {
                const auto c = s.points[edge], d = s.points[edge+1];
                if (squared(c,a) < equal_distance || squared(d,a) < equal_distance) continue;
                Point2 p{0,0}; float distance;
                if (squared(c,b) < equal_distance || squared(d,b) < equal_distance) { p=b; distance=1; }
                else if (intersection(a,b,c,d,intersection_tolerance,p)) {
                    if (squared(a,p) < equal_distance) continue;
                    distance = squared(p,a);
                } else if (arithmetic.distance(c,d,b) < intersection_tolerance) { p=b; distance=1; }
                else {
                    p = closest(a,b,d,arithmetic);
                    if (squared(p,d) >= intersection_tolerance) continue;
                    p=d; distance=squared(a,d);
                }
                p.x -= static_cast<float>(wrap)*g.span_x;
                hits.push_back({1,static_cast<std::uint32_t>(i),static_cast<std::uint32_t>(edge),s.group,distance,p});
            }
        }
        std::uint8_t kind = 0; float x = 0;
        if (b.x < g.bounds[0]) { kind=2; x=g.bounds[0]; }
        if (b.x > g.bounds[2]) { kind=4; x=g.bounds[2]; }
        if (kind) {
            require(std::abs(0.f*(b.y-a.y)-(g.bounds[3]-g.bounds[1])*(a.x-b.x)) >= .001f,
                    "Degenerate coast road boundary crossing");
            Point2 p{0,0};
            intersection(a,b,{x,g.bounds[1]},{x,g.bounds[3]},.001f,p);
            require(std::isfinite(p.y), "Degenerate coast road boundary crossing");
            const float dy = -p.y+g.bounds[1], dx = -x+x;
            hits.push_back({kind,absent_terrain_road_segment,0,0,dy*dy+dx*dx,
                            {x-static_cast<float>(wrap)*g.span_x,p.y}});
        }
        return hits;
    }
    std::array<std::uint16_t,2> chunk(std::span<const Point2> points, std::uint16_t first_node,
                                     std::uint16_t last_node) {
        std::vector<std::uint32_t> output;
        const auto ends = [&]() -> std::array<std::uint16_t,2> {
            if (output.empty()) return {no_node,no_node};
            return {g.segments[output.front()].first_junction,g.segments[output.back()].last_junction};
        };
        auto start_x = points.front().x; wrap=0;
        while (start_x >= g.bounds[2]) { start_x -= g.span_x; --wrap; }
        while (start_x < g.bounds[0]) { start_x += g.span_x; ++wrap; }
        if (points.size() < 2) return ends();
        std::size_t start = 0, current = 1;
        bool first = true;
        std::uint8_t first_boundary = 0;
        Crossing previous{0,0,0,0xffff,0,{0,0}};
        Point2 a{static_cast<float>(wrap)*g.span_x+points[0].x,points[0].y};
        Point2 b{static_cast<float>(wrap)*g.span_x+points[1].x,points[1].y};
        for (;;) {
            auto hits = crossings(a,b); sort_crossings(hits,hits.size());
            for (std::size_t i = 0; i < hits.size();) {
                if (!(hits[i].kind & 6) && (hits[i].group == previous.group || hits[i].group == group ||
                    (i && hits[i-1].group == hits[i].group))) { hits.erase(hits.begin()+i); i=0; }
                else ++i;
            }
            const auto hit_count = hits.size();
            if (start != current && !hits.empty()) {
                const auto hit = hits.front();
                std::uint16_t next; std::uint8_t boundary = 0; bool stop = false;
                if (hit.kind == 1) {
                    next=split(hit);
                    // This entry point is the original type-zero coast caller:
                    // every link has the same type. Two matches terminate here.
                    const auto count = g.junctions[next].size();
                    if (count == 2) return ends();
                    stop = count == 1 || count == 3;
                } else { next=node(); boundary = hit.kind == 2 ? 2 : 1; }
                const auto index = create(points.subspan(start,current-start),first_node,next,first_boundary,boundary);
                if (index == no_segment) return ends();
                output.push_back(index);
                arithmetic.append(g.segments[index].points,hit.point);
                if (!first) arithmetic.prepend(g.segments[index].points,previous.point);
                first=false;
                shift_terrain_road_segment(g,index,wrap); publish_terrain_road_segment(g,index,group,0,0);
                if (stop) return ends();
                const auto old_wrap = wrap;
                if (hit.kind & 2) ++wrap; else if (hit.kind & 4) --wrap;
                b.x=static_cast<float>(wrap)*g.span_x+(b.x-static_cast<float>(old_wrap)*g.span_x);
                first_node=next; first_boundary=boundary; previous=hit; start=current;
            }
            if (start == current && hit_count >= 2) {
                a=previous.point;
                // Original rescans this shortened edge once, then advances
                // without creating another segment when start==current.
                crossings(a,b);
            }
            if (current == points.size()-1) break;
            a=b; ++current; b={static_cast<float>(wrap)*g.span_x+points[current].x,points[current].y};
        }
        if (start != current) {
            if (first) ++start;
            if (start != current) {
                const auto index = create(points.subspan(start,current-start+1),first_node,last_node,first_boundary,0);
                if (index != no_segment) {
                    if (!first) arithmetic.prepend(g.segments[index].points,previous.point);
                    require(g.segments[index].points.size() > 1, "Degenerate coast road path");
                    output.push_back(index); shift_terrain_road_segment(g,index,wrap);
                    publish_terrain_road_segment(g,index,group,0,0);
                }
            }
        }
        return ends();
    }
public:
    Builder(TerrainRoadGrid &grid, X87Precision precision, std::uint32_t group)
        : g(grid), arithmetic{precision}, group(group) {}
    void path(std::span<const Point2> points, bool closed) {
        std::size_t chunks=points.size()>>4;
        if (closed) chunks=std::max<std::size_t>(2,chunks);
        auto per_chunk=points.size()/chunks, offset=std::size_t{0};
        std::uint16_t first=no_node, last=no_node;
        bool tail=false;
        for (std::size_t i=0; i<chunks; ++i) {
            const auto part = i ? points.subspan(offset-1,per_chunk+1) : points.subspan(offset,per_chunk);
            const auto result=chunk(part,last,closed && i==chunks-1 ? first : no_node);
            last=result[1]; if (!i) first=result[0];
            offset+=per_chunk;
            if (tail || offset==points.size()) return;
            const auto remaining=points.size()-offset;
            if (remaining<24) { tail=true; i=chunks-2; per_chunk=remaining-1; }
            else if (remaining<=per_chunk) per_chunk=remaining;
        }
    }
};
} // namespace
void insert_coast_road_path(TerrainRoadGrid &grid, std::span<const Point2> points,
                            bool closed, std::uint32_t group, X87Precision precision) {
    require(precision == X87Precision::single || precision == X87Precision::double_precision,
            "Unsupported coast road precision");
    require(points.size() >= (closed ? 4u : 16u) && points.size() <= max_coast_graph_edges*10,
            "Invalid coast road path length");
    require(grid.width > 0 && grid.height > 0 && std::uint64_t(grid.width)*std::uint64_t(grid.height) <= 65535 &&
        grid.cells.size() == std::size_t(grid.width)*std::size_t(grid.height) && grid.members.size() == grid.cells.size() &&
        std::all_of(grid.bounds.begin(),grid.bounds.end(),[](float v) { return std::isfinite(v); }) &&
        std::isfinite(grid.span_x) && std::isfinite(grid.span_y) && grid.span_x > 0 && grid.span_y > 0 &&
        grid.span_x == grid.bounds[2]+-grid.bounds[0] && grid.span_y == grid.bounds[3]+-grid.bounds[1],
        "Invalid coast road grid");
    for (auto p : points) require(std::isfinite(p.x) && std::isfinite(p.y) &&
        std::abs(double(p.x)/grid.span_x) < 1000000, "Invalid coast road point/wrap extent");
    for (const auto &segment : grid.segments) {
        require(segment.type == 0 && segment.points.size() >= 2 &&
            segment.first_junction < grid.junctions.size() && segment.last_junction < grid.junctions.size(),
            "Invalid/mixed segment in coast network");
    }
    for (const auto &node : grid.junctions) {
        require(node.size() <= 4, "Invalid coast road junction");
        for (auto link : node) require(link.segment < grid.segments.size() && link.boundary <= 3,
                                      "Invalid coast road connection");
    }
    Builder(grid,precision,group).path(points,closed);
}
TerrainRoadGrid build_coast_road_network(const CoastDecalPaths &paths, const WorldLayout &world,
                                        X87Precision precision) {
    require(precision == X87Precision::single || precision == X87Precision::double_precision,
            "Unsupported coast road precision");
    auto result=make_terrain_road_grid(world);
    std::uint32_t group=0;
    for (const auto &path : paths.paths) {
        require(path.first <= paths.points.size() && path.count <= paths.points.size()-path.first,
                "Invalid coast road path range");
        insert_coast_road_path(result,std::span(paths.points).subspan(path.first,path.count),path.closed,++group,precision);
    }
    return result;
}
} // namespace civ5::terrain
