#include "civ5/terrain/coast_map.hpp"
#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace civ5::terrain {
namespace {
void require(bool ok, const char *message) {
    if (!ok)
        throw std::invalid_argument(message);
}
using Cube = std::array<std::int32_t, 3>;
constexpr std::array<Cube, 6> directions{
    {{0, 1, -1}, {-1, 0, 1}, {1, -1, 0}, {0, -1, 1}, {1, 0, -1}, {-1, 1, 0}}};
constexpr std::array<std::array<unsigned, 2>, 3> endpoints{{{4, 5}, {5, 3}, {3, 4}}};
int floor_div(int a, int b) { return a / b - (a % b < 0 ? 1 : 0); }
int normalized(int a, int width) {
    const int r = a % width;
    return r < 0 ? r + width : r;
}
Cube transform(Cube v, const IntMatrix4 &m) {
    Cube result{};
    for (unsigned c = 0; c < 3; ++c)
        result[c] = wrap_add(
            wrap_add(wrap_add(wrap_mul(v[2], m[8 + c]), wrap_mul(v[1], m[4 + c])), wrap_mul(v[0], m[c])),
            m[12 + c]);
    return result;
}
Cube base_position(int x, int y, const IntMatrix4 &m) {
    const int a = wrap_add(x, wrap_neg(y >> 1));
    auto p = transform({a, y, wrap_neg(wrap_add(a, y))}, m);
    p[2] = wrap_neg(wrap_add(p[0], p[1]));
    return p;
}
std::array<int, 2> logical(Cube p, const IntMatrix4 &m) {
    p = transform(p, m);
    return {wrap_add(p[0], p[1] >> 1), p[1]};
}
Cube moved(Cube p, unsigned a, std::optional<unsigned> b = {}, bool through_float = false) {
    for (unsigned c = 0; c < 3; ++c) {
        p[c] = wrap_add(p[c], directions[a][c]);
        if (b)
            p[c] = wrap_add(p[c], directions[*b][c]);
        if (through_float) {
            const float f = static_cast<float>(p[c]);
            require(double(f) >= -2147483648.0 && double(f) < 2147483648.0,
                    "Coast neighbor exceeds native integer conversion range");
            p[c] = static_cast<std::int32_t>(f);
        }
    }
    return p;
}
Point2 world(Cube p, const CoastMapSource &source) {
    const auto v = vector4_transform({float(p[0]), float(p[1]), float(p[2]), 1.f}, source.world,
                                     source.precision, source.transform_path);
    return {v[0], v[1]};
}
std::optional<std::uint8_t> tag(unsigned a, unsigned b, unsigned d) {
    if (a == 0 && b == 1)
        return d;
    if (a == 1 && b == 0)
        return d + 6;
    if (a == 1 && b == 2)
        return d + 12;
    if (a == 2 && b == 1)
        return d + 18;
    return {};
}
Point2 centroid(Point2 a, Point2 b) { return {(a.x + b.x) * .33333334f, (a.y + b.y) * .33333334f}; }
Point2 weighted(Point2 a, Point2 b, float w) {
    return {a.x * w + b.x * (1.f - w), a.y * w + b.y * (1.f - w)};
}
bool family(unsigned a, unsigned b) { return (a < 12) == (b < 12); }
int index(int x, int y, const CoastSegmentGrid &g, bool wrap) {
    if (wrap)
        x = normalized(x, g.width());
    require(x >= 0 && x < g.width() && y >= 0 && y < g.height(),
            "Native coast requires unresolved fallback-cell data");
    return y * g.width() + x;
}
CoastCellClass classify_mask(std::uint32_t mask, std::optional<std::uint8_t> prior_style) {
    const bool land = (mask & 0xf3ef9fffu) != 0;
    require(land || (mask & 0x106000u) != 0 || prior_style.has_value(),
            "Coast mask does not independently initialize native style");
    constexpr std::array<std::array<unsigned, 2>, 11> styles{{{0x400, 11},
                                                              {1, 1},
                                                              {0x20, 2},
                                                              {2, 3},
                                                              {0x40, 4},
                                                              {0x10, 5},
                                                              {0x200, 6},
                                                              {4, 7},
                                                              {0x80, 8},
                                                              {8, 9},
                                                              {0x100, 10}}};
    std::uint8_t style = land ? 0 : (mask & 0x106000u) ? 12 : prior_style.value_or(0);
    for (auto pair : styles)
        if (mask & pair[0]) {
            style = pair[1];
            break;
        }
    return {std::uint8_t(land                   ? 0
                         : (mask & 0x0c102000u) ? 1
                                                : 2),
            style, std::uint8_t(mask & 0x400u ? 2 : 0)};
}
} // namespace
CoastCellClass classify_coast_mask(std::uint32_t mask) { return classify_mask(mask, {}); }
CoastMapGraph CoastMapGraph::build(const CoastMapSource &s) {
    require(s.precision == X87Precision::single || s.precision == X87Precision::double_precision,
            "Unsupported coast map precision");
    require(s.transform_path == Vec4Path::legacy_x87 || s.transform_path == Vec4Path::sse,
            "Unsupported coast map transform path");
    // Native graph cells have a 255-column stride and a 255x255 owner;
    // the old 128 bound described the initial differential fixtures only.
    require(s.width >= 1 && s.width <= 255 && s.height >= 1 && s.height <= 255,
            "Coast map dimensions must be 1..255");
    require(s.masks.size() == std::size_t(s.width * s.height), "Coast source mask size mismatch");
    for (float f : s.world)
        require(std::isfinite(f), "Nonfinite coast world matrix");
    CoastMapGraph graph;
    graph.wrap_x_ = s.wrap_x;
    auto &g = graph.initial_;
    g.width_ = s.width;
    g.height_ = s.height;
    for (int y = 0; y < s.height; ++y) {
        // 0x63E9C0 retains its style local across X within one row job. A
        // feature-only water mask can inherit a preceding cell's style. The
        // first unresolved style remains an error, not fabricated stack data.
        std::optional<std::uint8_t> prior_style;
        for (int x = 0; x < s.width; ++x) {
            const auto classification = classify_mask(s.masks[x * s.height + y], prior_style);
            prior_style = classification.style;
            g.cells_.push_back({classification, {}, {}});
        }
    }
    const auto start = base_position(0, 0, s.logical_to_map);
    g.origin_ = world(start, s);
    for (unsigned d = 0; d < 6; ++d) {
        const auto p = world(moved(start, d), s);
        g.corners_.push_back({-g.origin_.x + p.x, -g.origin_.y + p.y});
    }
    const auto end = world(base_position(s.width, 0, s.logical_to_map), s);
    g.wrap_ = {-g.origin_.x + end.x, -g.origin_.y + end.y};
    for (unsigned d = 0; d < 3; ++d) {
        const auto a = centroid(g.corners_[d], g.corners_[endpoints[d][0]]);
        const auto b = centroid(g.corners_[d], g.corners_[endpoints[d][1]]);
        g.offsets_.push_back({weighted(b, a, .25f), weighted(b, a, .75f)});
    }
    for (int y = 0; y < s.height; ++y)
        for (int x = 0; x < s.width; ++x) {
            auto &own = g.cells_[y * s.width + x];
            const auto p = base_position(x, y, s.logical_to_map);
            const auto center = world(p, s);
            for (unsigned d = 0; d < 6; ++d) {
                auto [nx, ny] = logical(moved(p, d), s.map_to_logical);
                if (ny < 0 || ny >= s.height || (!s.wrap_x && (nx < 0 || nx >= s.width)))
                    continue;
                if (s.wrap_x)
                    nx = normalized(nx, s.width);
                const auto &other = g.cells_[ny * s.width + nx].classification;
                const auto code = tag(own.classification.category, other.category, d);
                if (!code)
                    continue;
                if (d >= 3) {
                    own.opposite_directions.push_back(*code);
                    continue;
                }
                const auto style = own.classification.category == 0 ? own.classification.style
                                   : other.category == 0            ? other.style
                                                                    : 12;
                const auto &offset = g.offsets_[d];
                own.segments.push_back({*code,
                                        std::uint8_t(style),
                                        {offset[0].x + center.x, offset[0].y + center.y},
                                        {offset[1].x + center.x, offset[1].y + center.y}});
            }
        }
    const auto cell = [&](Cube p) -> const CoastMapCell & {
        const auto [x, y] = logical(p, s.map_to_logical);
        return g.cells_[index(x, y, g, s.wrap_x)];
    };
    const auto neighbor = [&](Cube p, const CoastMapCell &own, unsigned edge, bool mode) {
        const auto t = own.segments[edge].direction;
        const auto f = t > 11 ? 12 : 0;
        const auto [a, b] = endpoints[t % 3];
        const auto first = mode ? b : a, second = mode ? a : b;
        const auto result = [&](Cube q, int k) {
            const auto [x, y] = logical(q, s.map_to_logical);
            return CoastEdgeLink{x, y, k};
        };
        const auto search = [&](Cube q, unsigned d) {
            const auto &segments = cell(q).segments;
            for (std::size_t k = 0; k < segments.size(); ++k)
                if (family(segments[k].direction, d + f) && segments[k].direction % 6 == (d % 6 + 3) % 6)
                    return int(k);
            return -1;
        };
        if (std::any_of(own.opposite_directions.begin(), own.opposite_directions.end(),
                        [&](auto d) { return family(d, first + f) && d % 6 == first; })) {
            const auto q = moved(p, first, {}, true);
            const auto k = search(q, first);
            if (k >= 0)
                return result(q, k);
        }
        const auto q = moved(p, a, b, true);
        const auto k = search(q, second);
        return k >= 0 ? result(q, k) : result(p, -1);
    };
    for (int i = 0; i < s.width * s.height; ++i) {
        const auto &own = g.cells_[i];
        const auto p = base_position(i % s.width, i / s.width, s.logical_to_map);
        std::vector<CoastEdgeLinks> links;
        for (unsigned k = 0; k < own.segments.size(); ++k) {
            const bool mode = own.segments[k].direction % 12 < 6;
            links.push_back({neighbor(p, own, k, mode), neighbor(p, own, k, !mode)});
        }
        graph.links_.push_back(std::move(links));
    }
    return graph;
}
CoastSegmentGrid CoastMapGraph::smooth(X87Precision precision, const StageObserver &observer) const {
    require(precision == X87Precision::single || precision == X87Precision::double_precision,
            "Unsupported coast smoothing precision");
    auto current = initial_, scratch = initial_;
    const auto controls = [&](std::size_t i, std::size_t k) {
        const auto &seg = current.cells_[i].segments[k];
        const auto a = seg.first, b = seg.second;
        const float dx = a.x + -b.x, dy = a.y + -b.y, hx = dx * .5f, hy = dy * .5f;
        std::array<Point2, 6> p{
            {{a.x + dx, a.y + dy}, {a.x + hx, hy + a.y}, a, b, {b.x - hx, b.y - hy}, {b.x - dx, b.y - dy}}};
        const auto &links = links_[i][k];
        const bool reversed = seg.direction % 12 >= 6;
        const auto replace = [&](CoastEdgeLink link, unsigned start) {
            if (link.edge == -1)
                return;
            const int x = wrap_x_ ? normalized(link.x, current.width_) : link.x;
            const auto &other = current.cells_[index(x, link.y, current, wrap_x_)].segments.at(link.edge);
            // Native neighbor controls shift X only, even for nonzero wrap Y.
            const float shift = float((std::int64_t(link.x) - x) / current.width_) * current.wrap_.x;
            p[start] = {other.second.x + shift, other.second.y};
            p[start + 1] = {other.first.x + shift, other.first.y};
        };
        replace(reversed ? links.reverse : links.forward, 4);
        replace(reversed ? links.forward : links.reverse, 0);
        if (reversed)
            std::reverse(p.begin(), p.end());
        return p;
    };
    const auto weight = [](Point2 a, Point2 b, Point2 c) {
        return Point2{(a.x * .25f + b.x * .5f) + c.x * .25f, (a.y * .25f + b.y * .5f) + c.y * .25f};
    };
    const auto normal = [&](const std::array<Point2, 6> &p, unsigned i) {
        const auto a = p[i], b = p[i + 1], c = p[i + 2];
        const float dx = (a.x * -.5f + b.x * 0.f) + c.x * .5f, dy = (b.y * 0.f + a.y * -.5f) + c.y * .5f,
                    ny = -dx;
        const double square = round_x87(double(dy) * dy, precision);
        const float length =
            float(round_x87(std::sqrt(round_x87(square + double(ny * ny), precision)), precision));
        require(std::isfinite(length) && length > 0.f, "Degenerate coast displacement derivative");
        const float inv = 1.f / length;
        return Point2{inv * dy, inv * ny};
    };
    const auto offset = [](Point2 a, Point2 n) { return Point2{a.x + n.x * 4.f, n.y * 4.f + a.y}; };
    constexpr std::array<unsigned, 10> modes{1, 2, 3, 2, 1, 2, 3, 2, 1, 2};
    for (unsigned stage = 0; stage < modes.size(); ++stage) {
        const auto mode = modes[stage];
        if (mode == 2)
            current = scratch;
        else {
            scratch = current;
            for (std::size_t i = 0; i < current.cells_.size(); ++i)
                for (std::size_t k = 0; k < current.cells_[i].segments.size(); ++k) {
                    const auto p = controls(i, k);
                    const auto &seg = current.cells_[i].segments[k];
                    auto &out = scratch.cells_[i].segments[k];
                    const bool reversed = seg.direction % 12 >= 6;
                    if (mode == 1) {
                        const auto a = weight(p[1], p[2], p[3]), b = weight(p[2], p[3], p[4]);
                        out.first = reversed ? b : a;
                        out.second = reversed ? a : b;
                    } else if (current.cells_[i].classification.displacement_flag != 0) {
                        const auto a = normal(p, 1), b = normal(p, 2);
                        out.first = offset(seg.first, reversed ? a : b);
                        out.second = offset(seg.second, reversed ? b : a);
                    }
                }
        }
        if (observer)
            observer(stage, mode, current);
    }
    return current;
}
std::vector<CoastContour> CoastMapGraph::scan(const CoastSegmentGrid &g) const {
    require(g.width_ == initial_.width_ && g.height_ == initial_.height_ &&
                g.cells_.size() == initial_.cells_.size() && g.wrap_ == initial_.wrap_,
            "Coast scan geometry topology mismatch");
    for (std::size_t i = 0; i < g.cells_.size(); ++i) {
        const auto &a = g.cells_[i], &b = initial_.cells_[i];
        require(a.classification == b.classification && a.opposite_directions == b.opposite_directions &&
                    a.segments.size() == b.segments.size(),
                "Coast scan cell topology mismatch");
        for (std::size_t k = 0; k < a.segments.size(); ++k)
            require(a.segments[k].direction == b.segments[k].direction &&
                        a.segments[k].style == b.segments[k].style,
                    "Coast scan segment topology mismatch");
    }
    std::vector<bool> visited(g.cells_.size() * 3);
    // Original 0x5DE5B0 grows the initial 2048-point allocation. A traversal
    // emits two points per edge plus its closing edge; bound work by topology.
    std::size_t maximum_points = 2;
    for (const auto &cell : g.cells_)
        maximum_points += cell.segments.size() * 2;
    std::vector<CoastContour> result;
    const auto walk = [&](std::size_t start, int start_edge) {
        const auto &first = g.cells_[start];
        const bool reversed = first.segments[start_edge].direction % 12 >= 6;
        const auto cat = first.classification.category;
        const unsigned owner = (!reversed && cat == 0) || (reversed && cat == 1)   ? 0
                               : (!reversed && cat == 1) || (reversed && cat == 2) ? 1
                                                                                   : 2;
        require(owner <= 1, "Coast start does not initialize native owner flag");
        CoastContour out{int(start) % g.width_, int(start) / g.width_, start_edge, false, {}, owner, 1, {}};
        int last_style = -1;
        std::uint8_t previous_style = 0;
        float transition = 0.f;
        const auto emit = [&](const CoastSegment &segment, std::optional<float> shift) {
            const auto pair = segment.direction % 12 < 6 ? std::array{segment.first, segment.second}
                                                         : std::array{segment.second, segment.first};
            for (auto p : pair) {
                require(out.points.size() < maximum_points, "Coast contour exceeds graph edge capacity");
                if (segment.style != last_style) {
                    transition = transition == 1.f ? 0.f : 1.f;
                    last_style = segment.style;
                }
                const Point2 finite{shift ? p.x + *shift : p.x, p.y};
                out.points.push_back({finite.x, finite.y, transition, segment.style, previous_style});
                previous_style = segment.style;
            }
        };
        emit(first.segments[start_edge], {});
        auto current = start;
        int edge = start_edge, raw_x = out.start_x, raw_y = out.start_y;
        float shift = 0.f;
        while (!visited[current * 3 + edge]) {
            const auto next = links_[current][edge].forward;
            raw_x = next.x;
            raw_y = next.y;
            const auto target = index(raw_x, raw_y, g, wrap_x_);
            // Native MOVZX 255 / CMP -1 does not terminate an open walk.
            require(next.edge >= 0 && std::size_t(next.edge) < g.cells_[target].segments.size(),
                    "Native coast walk reaches unresolved edge byte 255");
            shift = float(floor_div(raw_x, g.width_)) * g.wrap_.x + shift;
            emit(g.cells_[target].segments[next.edge], shift);
            visited[current * 3 + edge] = true;
            current = target;
            edge = next.edge;
        }
        out.closed = (wrap_x_ ? normalized(raw_x, g.width_) : raw_x) == out.start_x && raw_y == out.start_y &&
                     edge == start_edge;
        if (out.closed)
            out.wrap_offset =
                Point2{-out.points[1].x + out.points.back().x, -out.points[1].y + out.points.back().y};
        return out;
    };
    for (bool open_only : {true, false})
        for (std::size_t i = 0; i < links_.size(); ++i)
            for (std::size_t k = 0; k < links_[i].size(); ++k) {
                const auto &link = links_[i][k];
                if (link.forward.edge == -1 || visited[i * 3 + k] || (open_only && link.reverse.edge != -1))
                    continue;
                auto contour = walk(i, int(k));
                if (contour.points.size() >= 4)
                    result.push_back(std::move(contour));
            }
    return result;
}
} // namespace civ5::terrain
