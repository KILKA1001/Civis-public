#include "civ5/terrain/river_geometry.hpp"
#include "curve_wavelet.hpp"
#include <algorithm>
#include <cmath>
#include <map>
#include <stdexcept>

namespace civ5::terrain {
namespace {
constexpr std::array<int, 12> corner_map{2, 4, 0, 5, 1, 3, 1, 3, 5, 4, 0, 2};
constexpr std::array<int, 12> forward_turn{4, 5, 3, 1, 2, 0, 11, 9, 10, 8, 6, 7};
constexpr float third = 0.3333333432674408f, two_thirds = 0.6666666865348816f;
constexpr std::array<Vector4, 6> corners{{{two_thirds, -third, -third, 1},
                                          {third, third, -two_thirds, 1},
                                          {-third, two_thirds, -third, 1},
                                          {-two_thirds, third, third, 1},
                                          {-third, -third, two_thirds, 1},
                                          {third, -two_thirds, third, 1}}};
Vector4 cube_corner(RiverEdge e) {
    auto translation = identity_matrix();
    translation[12] = float(e.x);
    translation[13] = float(e.y);
    translation[14] = float(e.z);
    return vector4_transform(corners[corner_map[e.direction]], translation, X87Precision::double_precision,
                             Vec4Path::sse);
}
std::array<RiverPoint, 2> edge_controls(const RiverNetwork &network, RiverEdge e, const Matrix4 &m) {
    if (e.direction >= 6)
        e = network.opposite(e);
    const auto a = cube_corner(e);
    e.direction = forward_turn[e.direction];
    const auto b = cube_corner(e);
    std::array<float, 2> p{}, q{};
    for (unsigned c = 0; c < 2; ++c) {
        p[c] = ((a[1] * m[4 + c] + a[2] * m[8 + c]) + m[c] * a[0]) + m[12 + c];
        q[c] = ((b[2] * m[8 + c] + b[1] * m[4 + c]) + m[c] * b[0]) + m[12 + c];
    }
    return {{{p[0] * .75f + q[0] * .25f, p[1] * .75f + q[1] * .25f, 0},
             {p[0] * .25f + q[0] * .75f, p[1] * .25f + q[1] * .75f, 0}}};
}
curve_wavelet::Points xy(const RiverPolyline &line) {
    curve_wavelet::Points out;
    for (const auto &p : line.points)
        out.push_back({p[0], p[1]});
    return out;
}
void assign_xy(RiverPolyline &line, const curve_wavelet::Points &p) {
    if (line.points.size() != p.size())
        throw std::invalid_argument("River wavelet output size mismatch");
    for (std::size_t i = 0; i < p.size(); ++i) {
        line.points[i][0] = p[i].x;
        line.points[i][1] = p[i].y;
    }
}
Point2 cache_origin(RiverEdge e, const Matrix4 &m) {
    const auto x = float(e.x), y = float(e.y), z = float(e.z);
    return {((z * m[8] + y * m[4]) + m[0] * x) + m[12], ((z * m[9] + y * m[5]) + m[1] * x) + m[13]};
}
} // namespace
RiverPolylines build_river_centerlines(const RiverNetwork &network, const Matrix4 &m, TerrainRandom &random,
                                       const RiverControlObserver &observer) {
    for (const auto v : m)
        if (!std::isfinite(v))
            throw std::invalid_argument("Nonfinite river world matrix");
    RiverPolylines lines;
    for (const auto &chain : network.chains()) {
        RiverPolyline line;
        for (auto e : chain.edges) {
            const auto points = edge_controls(network, e, m);
            line.points.insert(line.points.end(), points.begin(), points.end());
        }
        lines.push_back(std::move(line));
    }
    if (observer)
        observer(RiverControlStage::controls, lines, random);
    for (auto &line : lines)
        if (line.points.size() > 6) {
            const auto p = xy(line);
            const auto details = curve_wavelet::detail(p), original = curve_wavelet::reduce(p);
            auto coarse = original;
            for (std::size_t i = 1; i + 1 < original.size(); ++i) {
                const auto a = original[i - 1], b = original[i], c = original[i + 1];
                coarse[i] = {(a.x * .125f + b.x * .75f) + c.x * .125f, (a.y * .125f + b.y * .75f) + c.y * .125f};
            }
            assign_xy(line, curve_wavelet::reconstruct(coarse, details));
        }
    if (observer)
        observer(RiverControlStage::smoothed, lines, random);
    for (auto &line : lines)
        if (line.points.size() > 6) {
            const auto p = xy(line), details = curve_wavelet::detail(p);
            auto coarse = curve_wavelet::reduce(p);
            for (std::size_t i = 1; i + 1 < coarse.size(); ++i)
                if (random.next() % 2 == 0) {
                    const auto x = float(random.next() % 10) - 5.f;
                    const auto y = float(random.next() % 10) - 5.f;
                    coarse[i] = {x + coarse[i].x, y + coarse[i].y};
                }
            assign_xy(line, curve_wavelet::reconstruct(coarse, details));
            for (auto &point : line.points)
                if (random.next() % 2 == 0) {
                    const auto x = float(random.next() % 15) - 7.5f;
                    const auto y = float(random.next() % 15) - 7.5f;
                    point[0] = x + point[0];
                    point[1] = y + point[1];
                }
        }
    if (observer)
        observer(RiverControlStage::jittered, lines, random);
    return lines;
}
RiverPolylines extend_river_centerlines(const RiverNetwork &network, const RiverPolylines &lines, const Matrix4 &m,
                                        int mode, const ReciprocalSqrtEstimate &estimate) {
    if (mode < 0 || mode > 2 || lines.size() != network.chains().size())
        throw std::invalid_argument("River coordinate cache input mismatch");
    for (auto f : m)
        if (!std::isfinite(f))
            throw std::invalid_argument("Nonfinite river cache matrix");
    std::map<RiverEdge, std::array<RiverPoint, 2>> cache;
    for (std::size_t i = 0; i < lines.size(); ++i) {
        const auto &points = lines[i].points;
        const auto &edges = network.chains()[i].edges;
        if (points.size() != edges.size() * 2)
            throw std::invalid_argument("River edge/control correspondence mismatch");
        for (const auto &p : points)
            for (auto f : p)
                if (!std::isfinite(f))
                    throw std::invalid_argument("Nonfinite river control");
        for (std::size_t j = 0; j < edges.size(); ++j) {
            auto e = edges[j];
            const auto neighbor = network.opposite(network.normalized(e));
            const auto x = wrap_add(neighbor.x, neighbor.y >> 1);
            if (neighbor.y < 0 || neighbor.y >= network.height() || (mode == 0 && (x < 0 || x >= network.width())))
                continue;
            e = network.canonical(e);
            const auto origin = cache_origin(e, m);
            std::array<RiverPoint, 2> p;
            for (unsigned k = 0; k < 2; ++k)
                p[k] = {points[j * 2 + k][0] - origin.x, points[j * 2 + k][1] - origin.y, 1.f};
            cache[network.normalized(e)] = p;
            e.direction += e.direction < 6 ? 6 : -6;
            std::swap(p[0], p[1]);
            cache[network.normalized(e)] = p;
        }
    }
    auto out = lines;
    for (std::size_t i = 0; i < out.size(); ++i) {
        auto &line = out[i];
        const auto last = network.chains()[i].edges.back();
        const auto next = network.next(last);
        if (!next) {
            const auto p = edge_controls(network, last, m);
            const auto n = normalize2_sse({p[1][0] - p[0][0], p[1][1] - p[0][1]}, estimate);
            const Point2 step{n.x * 16.f, n.y * 16.f};
            auto point = line.points.back();
            point[2] = 0;
            for (unsigned k = 0; k < 4; ++k) {
                point = {step.x + point[0], step.y + point[1], 0};
                line.points.push_back(point);
            }
        } else {
            const auto edge = network.canonical(*next);
            const auto found = cache.find(network.normalized(edge));
            if (found == cache.end()) {
                const auto p = edge_controls(network, *next, m);
                line.points.insert(line.points.end(), p.begin(), p.end());
            } else {
                const auto origin = cache_origin(edge, m);
                for (auto p : found->second)
                    line.points.push_back({p[0] + origin.x, p[1] + origin.y, p[2]});
            }
        }
        line.terminal = !next.has_value();
    }
    return out;
}
std::array<RiverPolylines, 2> widen_river_centerlines(const RiverPolylines &lines, TerrainRandom &random,
                                                      const ReciprocalSqrtEstimate &estimate) {
    std::array<RiverPolylines, 2> out{lines, lines};
    for (auto &family : out)
        for (auto &line : family)
            line.terminal = false;
    for (std::size_t line_index = 0; line_index < lines.size(); ++line_index) {
        const auto &line = lines[line_index];
        const auto &p = line.points;
        if (p.size() > 1048576)
            throw std::invalid_argument("Excessive river control count");
        for (const auto &point : p)
            for (auto f : point)
                if (!std::isfinite(f))
                    throw std::invalid_argument("Nonfinite river width input");
        if (p.size() < 4)
            continue;
        float amplitude = 0;
        for (std::size_t i = 0; i < p.size(); ++i) {
            Vector3 direction;
            if (i == 0)
                direction = {-p[0][0] + p[1][0], -p[0][1] + p[1][1], -0.f};
            else if (i == p.size() - 1)
                direction = {-p[i - 1][0] + p[i][0], -p[i - 1][1] + p[i][1], -0.f};
            else {
                const auto a = normalize3_sse({-p[i][0] + p[i - 1][0], -p[i][1] + p[i - 1][1], -0.f}, estimate);
                const auto b = normalize3_sse({-p[i][0] + p[i + 1][0], -p[i][1] + p[i + 1][1], -0.f}, estimate);
                direction = {-(a[0] + p[i][0]) + (b[0] + p[i][0]), -(a[1] + p[i][1]) + (b[1] + p[i][1]), -a[2] + b[2]};
            }
            direction = normalize3_sse(direction, estimate);
            const auto n = normalize3_sse({direction[1] - direction[2] * 0.f, direction[2] * 0.f - direction[0],
                                           direction[0] * 0.f - direction[1] * 0.f},
                                          estimate);
            amplitude = std::min(amplitude + .75f, 1.f);
            float left, right;
            const auto tail = p.size() - 4;
            if (i < 2 || (i >= tail && line.terminal))
                left = right = amplitude + 0.4050000011920929f;
            else if (i < 4)
                left = right = amplitude + 0.7050000429153442f;
            else if (i < 6)
                left = right = amplitude + 1.6050000190734863f;
            else {
                left = (float(random.next() % 5) + amplitude) + 0.4050000011920929f;
                right = (float(random.next() % 5) + amplitude) + 0.4050000011920929f;
            }
            if (!line.terminal && i >= p.size() - 6 && i != 0) {
                left = (float(random.next() % 3) + amplitude) + 0.4050000011920929f;
                right = (float(random.next() % 3) + amplitude) + 0.4050000011920929f;
            }
            if (line.terminal && i >= tail) {
                const auto k = i - tail + 4;
                const float flare = (float(k * k) * 0.3499999940395355f) * 0.8333333134651184f;
                left = flare + left;
                right = flare + right;
            }
            out[0][line_index].points[i][0] = p[i][0] + -(n[0] * left);
            out[0][line_index].points[i][1] = -(n[1] * left) + p[i][1];
            out[1][line_index].points[i][0] = p[i][0] + n[0] * right;
            out[1][line_index].points[i][1] = n[1] * right + p[i][1];
        }
    }
    return out;
}
RiverHeights sample_river_heights(const HeightDomain &domain, std::span<const std::uint16_t> heights,
                                  const RiverPolylines &lines) {
    if (heights.size() != std::uint64_t(domain.width()) * domain.height() || heights.size() > 16777216)
        throw std::invalid_argument("Incomplete or excessive river height field");
    RiverHeights out;
    const auto integer = [](float v) {
        if (!std::isfinite(v) || double(v) < -2147483648.0 || double(v) >= 2147483648.0)
            throw std::invalid_argument("Unrepresentable river sample coordinate");
        return static_cast<std::int32_t>(v);
    };
    for (const auto &line : lines) {
        std::vector<float> sampled;
        float minimum = -1;
        for (auto p : line.points) {
            const auto y = integer(p[1]);
            if (y < 0 || y >= domain.height())
                continue;
            const auto x = wrap_add(wrap_mul(domain.width(), 100), integer(p[0])) % domain.width();
            if (x < 0)
                throw std::invalid_argument("River sample exceeds native X wrapping envelope");
            auto value = float(heights[std::size_t(y) * domain.width() + x]);
            if (minimum < 0 || value < minimum)
                minimum = value;
            if (minimum < value)
                value = minimum;
            sampled.push_back(value);
        }
        out.push_back(std::move(sampled));
    }
    return out;
}
} // namespace civ5::terrain
