#include "civ5/terrain/river_geometry.hpp"
#include "river_curve.hpp"
#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace civ5::terrain {
RiverPoint detail::river_curve(std::span<const RiverPoint> p, float parameter, bool derivative,
                               const RiverCurveObserver &observer) {
    const auto segment = static_cast<std::size_t>(parameter);
    auto index = segment;
    float t = parameter - float(std::floor(double(parameter)));
    if (index >= p.size() - 2) {
        --index;
        t = 1.f;
    }
    if (index + 2 >= p.size())
        throw std::invalid_argument("River curve parameter exceeds its controls");
    float a, b, c;
    if (derivative) {
        // Original B93A6C/B93A84 coefficients, including the first segment.
        a = (segment == 0 ? 2.f : 1.f) * t + (segment == 0 ? -2.f : -1.f);
        b = (segment == 0 ? -3.f : -2.f) * t + (segment == 0 ? 2.f : 1.f);
        c = 1.f * t + 0.f;
    } else {
        // Original B8D218/B8D23C coefficients. Preserve individual SSE
        // operations, zero coefficients, signed zero and addition order.
        const float squared = t * t;
        a = ((segment == 0 ? 1.f : .5f) * squared + (segment == 0 ? -2.f : -1.f) * t) +
            (segment == 0 ? 1.f : .5f);
        b = ((segment == 0 ? -1.5f : -1.f) * squared + (segment == 0 ? 2.f : 1.f) * t) +
            (segment == 0 ? -0.f : .5f);
        c = (.5f * squared + 0.f * t) + 0.f;
    }
    const auto &p0 = p[index], &p1 = p[index + 1], &p2 = p[index + 2];
    RiverPoint out;
    out[0] = p0[0] * a + b * p1[0] + c * p2[0];
    out[1] = p1[1] * b + p0[1] * a + p2[1] * c;
    out[2] = p1[2] * b + p0[2] * a + p2[2] * c;
    if (observer)
        observer(derivative, p, parameter, out);
    return out;
}
namespace {
using Points = std::vector<RiverPoint>;
RiverPoint scale(RiverPoint p, float value) {
    for (auto &f : p)
        f *= value;
    return p;
}
RiverPoint add(const RiverPoint &a, const RiverPoint &b) { return {a[0] + b[0], a[1] + b[1], a[2] + b[2]}; }
RiverPoint negate(RiverPoint p) {
    for (auto &f : p)
        f = -f;
    return p;
}
RiverPoint normal(const RiverPoint &p, const ReciprocalSqrtEstimate &estimate) {
    return normalize3_sse({p[2] * 0.f - p[1], p[0] - p[2] * 0.f, p[1] * 0.f - p[0] * 0.f}, estimate);
}
RiverVertex vertex(const RiverPoint &p, float profile, float opacity) {
    return {p[0], p[1], p[2], profile, opacity};
}
void quad(std::vector<RiverTriangle> &out, const RiverVertex &a, const RiverVertex &b, const RiverVertex &c,
          const RiverVertex &d) {
    if (out.size() > 65534)
        throw std::invalid_argument("River stream exceeds supported triangle count");
    out.push_back({a, b, c});
    out.push_back({a, c, d});
}
void channel_section(std::vector<RiverTriangle> &out, const RiverPoint &c0, const RiverPoint &l0,
                     const RiverPoint &r0, const RiverPoint &c1, const RiverPoint &l1, const RiverPoint &r1,
                     float end_opacity) {
    quad(out, vertex(l0, 0, 1), vertex(l1, 0, end_opacity), vertex(c1, 1, end_opacity), vertex(c0, 1, 1));
    quad(out, vertex(c0, 1, 1), vertex(c1, 1, end_opacity), vertex(r1, 0, end_opacity), vertex(r0, 0, 1));
}
void bank_section(std::vector<RiverTriangle> &out, const RiverPoint &l0, const RiverPoint &r0,
                  const RiverPoint &l1, const RiverPoint &r1, const RiverPoint &outer_l0,
                  const RiverPoint &outer_r0, const RiverPoint &outer_l1, const RiverPoint &outer_r1,
                  float end_opacity) {
    quad(out, vertex(outer_l0, 0, 0), vertex(outer_l1, 0, 0), vertex(l1, 0, end_opacity), vertex(l0, 0, 1));
    quad(out, vertex(l0, 0, 1), vertex(l1, 0, end_opacity), vertex(r1, 0, end_opacity), vertex(r0, 0, 1));
    quad(out, vertex(r0, 0, 1), vertex(r1, 0, end_opacity), vertex(outer_r1, 0, 0), vertex(outer_r0, 0, 0));
}
} // namespace

RiverTriangleStreams build_river_triangles(const RiverPolylines &centers,
                                           const std::array<RiverPolylines, 2> &banks,
                                           const RiverHeights &heights,
                                           const ReciprocalSqrtEstimate &estimate,
                                           const RiverCurveObserver &observer) {
    if (centers.size() != heights.size() || banks[0].size() != centers.size() ||
        banks[1].size() != centers.size())
        throw std::invalid_argument("River triangle family count mismatch");
    RiverTriangleStreams out;
    for (std::size_t line = 0; line < centers.size(); ++line) {
        const auto size = centers[line].points.size();
        if (size > 1048576 || heights[line].size() != size || banks[0][line].points.size() != size ||
            banks[1][line].points.size() != size)
            throw std::invalid_argument(
                "River triangles require complete corresponding controls and heights");
        std::array<Points, 3> p{centers[line].points, banks[0][line].points, banks[1][line].points};
        for (auto &family : p)
            for (std::size_t i = 0; i < size; ++i) {
                family[i][2] = heights[line][i];
                for (auto f : family[i])
                    if (!std::isfinite(f))
                        throw std::invalid_argument("Nonfinite river triangle control");
            }
        if (size < 4)
            continue;
        const auto sample = [&](unsigned family, float t, bool derivative = false) {
            return detail::river_curve(p[family], t, derivative, observer);
        };
        const float maximum = float(size - 2), limit = maximum + 9.999999747378752e-05f;
        float last = 0;
        for (float t = .1f; t <= limit; t += .1f) {
            if (t > maximum)
                t = maximum;
            last = t;
            const float previous = t - .1f;
            const auto c1 = sample(0, t), l1 = sample(1, t), r1 = sample(2, t);
            const auto c0 = sample(0, previous), l0 = sample(1, previous), r0 = sample(2, previous);
            const auto d1 = sample(0, t, true), d0 = sample(0, previous, true);
            const auto tangent1 = normalize3_sse(d1, estimate), tangent0 = normalize3_sse(d0, estimate);
            const auto n1 = scale(normal(tangent1, estimate), 15.f),
                       n0 = scale(normal(tangent0, estimate), 15.f);
            channel_section(out.channels, c0, l0, r0, c1, l1, r1, 1);
            bank_section(out.banks, l0, r0, l1, r1, add(l0, n0), add(negate(n0), r0), add(n1, l1),
                         add(negate(n1), r1), 1);
        }
        const auto ce = sample(0, maximum), le = sample(1, maximum), re = sample(2, maximum);
        auto d = sample(0, maximum, true);
        d[2] = 0;
        auto tangent = normalize3_sse(d, estimate);
        // The original also normalizes the perpendicular here, even though
        // channel end vertices only use the tangent extrusion.
        (void)normal(tangent, estimate);
        tangent = scale(tangent, 15.f);
        channel_section(out.channels, ce, le, re, add(tangent, ce), add(le, tangent), add(tangent, re), 0);

        const auto l0 = sample(1, 0), r0 = sample(2, 0), l1 = sample(1, last), r1 = sample(2, last);
        auto d0 = sample(0, 0, true), d1 = sample(0, last, true);
        d0[2] = d1[2] = 0;
        auto t0 = normalize3_sse(d0, estimate), t1 = normalize3_sse(d1, estimate);
        const auto n0 = scale(normal(t0, estimate), 15.f), n1 = scale(normal(t1, estimate), 15.f);
        t0 = negate(scale(t0, 30.f));
        t1 = scale(t1, 30.f);
        const auto ol0 = add(l0, n0), or0 = add(r0, negate(n0));
        const auto back_l = add(l0, t0), back_r = add(r0, t0);
        // 7482C1..748309 first forms R-N, then adds the backward tangent.
        const auto back_or = add(t0, or0);
        bank_section(out.banks, l0, r0, back_l, back_r, ol0, or0, add(t0, ol0), back_or, 0);
        const auto ol1 = add(l1, n1), or1 = add(negate(n1), r1);
        const auto forward_l = add(t1, l1), forward_r = add(t1, r1);
        const auto forward_or = add(t1, or1);
        bank_section(out.banks, l1, r1, forward_l, forward_r, ol1, or1, add(t1, ol1), forward_or, 0);
    }
    return out;
}
} // namespace civ5::terrain
