#include "civ5/terrain/worker_geometry.hpp"
#include <algorithm>
#include <cmath>
#include <optional>
#include <stdexcept>

namespace civ5::terrain {
namespace {
void require(bool condition, const char *message) {
    if (!condition)
        throw std::invalid_argument(message);
}
Point2 bounded(float x, float y) {
    require(std::isfinite(x) && std::isfinite(y) && std::abs(x) <= 65536.f && std::abs(y) <= 65536.f,
            "Worker geometry point outside supported bounds");
    return Point2(x, y);
}
double length(float x, float y, X87Precision pc) {
    return round_x87(std::sqrt(round_x87(round_x87(double(x) * x, pc) + round_x87(double(y) * y, pc), pc)),
                     pc);
}
// Native returns x87 distance without a float spill; the caller's d1/d2 spills differ.
double distance(Point2 a, Point2 b, Point2 p, X87Precision pc) {
    const auto abx = b.x + -a.x, aby = b.y + -a.y, bpx = p.x + -b.x, bpy = p.y + -b.y, apx = p.x + -a.x,
               apy = p.y + -a.y;
    if ((a.x != b.x || a.y != b.y) && aby * bpy + bpx * abx < 0.f) {
        if (apy * -aby + apx * -abx >= 0.f)
            return length(apx, apy, pc);
        const auto numerator =
            round_x87(round_x87(double(apy) * abx, pc) - round_x87(double(aby) * apx, pc), pc);
        return std::abs(round_x87(numerator / length(abx, aby, pc), pc));
    }
    return length(bpx, bpy, pc);
}
std::optional<Point2> intersection(Point2 a, Point2 b, Point2 c, Point2 d) {
    constexpr auto epsilon = std::bit_cast<float>(0x38d1b717u);
    const auto ax = a.x - b.x, ay = b.y - a.y, ac = a.x * ay + a.y * ax;
    const auto by = d.y - c.y, bx = c.x - d.x, bc = c.x * by + c.y * bx;
    const auto determinant = bx * ay - by * ax;
    if (std::abs(determinant) < epsilon)
        return std::nullopt;
    const auto inverse = 1.f / determinant;
    const auto x = (bx * ac - bc * ax) * inverse, y = (bc * ay - by * ac) * inverse;
    if (x > std::min(std::max(a.x, b.x), std::max(c.x, d.x)) + epsilon ||
        x < std::max(std::min(a.x, b.x), std::min(c.x, d.x)) - epsilon ||
        y > std::min(std::max(a.y, b.y), std::max(c.y, d.y)) + epsilon ||
        y < std::max(std::min(a.y, b.y), std::min(c.y, d.y)) - epsilon)
        return std::nullopt;
    return bounded(x, y);
}
void repair(std::vector<Point2> &points, std::size_t first, std::size_t end, X87Precision pc) {
    for (auto i = first; i + 2 < end; ++i) {
        const auto a = points[i], b = points[i + 1], c = points[i + 2];
        const auto d1 = static_cast<float>(distance(a, b, c, pc));
        const auto d2 = distance(b, c, a, pc);
        const auto selected = d2 > double(d1) ? d1 : static_cast<float>(d2);
        if (double(selected) < .01) {
            points[i + 1] = c;
            continue;
        } // DOUBLE threshold.
        for (auto j = i + 2; j < std::min(i + 50, end - 1); ++j) {
            const auto hit = intersection(a, b, points[j], points[j + 1]);
            if (!hit)
                continue;
            for (auto k = j; k > i; --k)
                points[k] = *hit; // Preserve every slot, mutate in place.
            break;
        }
    }
}
} // namespace
WorkerGateGeometry build_worker_geometry(std::span<const CoastCurveSet> curves, X87Precision pc) {
    require(pc == X87Precision::single || pc == X87Precision::double_precision,
            "Unsupported worker geometry precision");
    require(curves.size() <= max_coast_contours, "Worker geometry curve-set capacity exceeded");
    std::uint64_t capacity = 0;
    for (const auto &set : curves) {
        require(set.owner_flag() <= 1, "Unsupported worker curve owner flag");
        capacity += set.control_count() * 3;
        require(capacity <= max_coast_worker_points, "Worker geometry shared point capacity exceeded");
        for (const auto &curve : set.curves())
            for (auto p : curve)
                bounded(p.x, p.y);
    }
    WorkerGateGeometry result;
    result.points.reserve(static_cast<std::size_t>(capacity));
    for (const auto &set : curves) {
        const auto first = static_cast<std::uint32_t>(result.points.size());
        for (auto p : set.curves()[2])
            result.points.emplace_back(p.x, p.y);
        result.boundaries.push_back(
            {first, static_cast<std::uint32_t>(result.points.size()), set.owner_flag()});
    }
    for (auto range : result.boundaries) {
        const auto count = range.end - range.first;
        for (unsigned side = 0; side < 2; ++side) {
            const auto first = static_cast<std::uint32_t>(result.points.size());
            for (unsigned i = 0; i + 1 < count; ++i) {
                const auto index = i >= count - 2 ? i - 1 : i;
                const float t = i >= count - 2 ? 1.f : 0.f;
                const auto a = result.points[range.first + index], b = result.points[range.first + index + 1],
                           c = result.points[range.first + index + 2];
                // This derivative is X=A+B+C, Y=B+A+C, distinct from curve sampling.
                const auto w0 = 1.f * t + -1.f, w1 = 1.f + -2.f * t, w2 = 1.f * t + 0.f;
                const auto dx = (a.x * w0 + w1 * b.x) + w2 * c.x, dy = (b.y * w1 + a.y * w0) + c.y * w2;
                const auto len = static_cast<float>(length(dx, dy, pc));
                require(len > 0.f && std::isfinite(len), "Degenerate native fill tangent");
                const auto inverse = 1.f / len, nx = inverse * dy, ny = -(inverse * dx);
                const auto p = result.points[range.first + i], q = result.points[range.first + i + 1];
                const auto mx = (p.x + q.x) * .5f, my = (q.y + p.y) * .5f;
                result.points.push_back(side == 0 ? bounded(mx + nx * -1.f, ny * -1.f + my)
                                                  : bounded(nx + mx, my + ny));
            }
            repair(result.points, first, result.points.size(), pc);
            result.fills.push_back(
                {first, static_cast<std::uint32_t>(result.points.size()), range.gate + side});
        }
    }
    return result;
}
} // namespace civ5::terrain
