#include "civ5/terrain/coast_perturbation.hpp"
#include "civ5/terrain/terrain_occurrences.hpp"
#include "curve_wavelet.hpp"
#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace civ5::terrain {
namespace {
void require(bool ok, const char *message) {
    if (!ok)
        throw std::invalid_argument(message);
}
using curve_wavelet::Points;
using curve_wavelet::reduce;
using curve_wavelet::detail;
using curve_wavelet::reconstruct;
Point2 derivative(const Points &p, float argument) {
    auto i = std::size_t(argument);
    float t = argument - std::floor(argument);
    if (i >= p.size() - 2) {
        --i;
        t = 1.f;
    }
    const auto a = p[i], b = p[i + 1], c = p[i + 2];
    const float w0 = 1.f * t + -1.f, w1 = 1.f + -2.f * t, w2 = 1.f * t + 0.f;
    return {(a.x * w0 + w1 * b.x) + w2 * c.x, (b.y * w1 + a.y * w0) + c.y * w2};
}
template <class Next>
CoastPerturbation perturb(std::span<const CoastControlPoint> points, bool closed, Point2 wrap,
                          Next &&random, X87Precision precision, CoastPowPath pow_path) {
    require(precision == X87Precision::single || precision == X87Precision::double_precision,
            "Unsupported coast perturbation precision");
    require(pow_path == CoastPowPath::legacy_x87 || pow_path == CoastPowPath::sse2,
            "Unsupported coast perturbation CRT power path");
    require(points.size() >= 4 && points.size() <= max_coast_contour_controls,
            "Coast perturbation exceeds graph contour capacity");
    require(std::isfinite(wrap.x) && std::isfinite(wrap.y), "Nonfinite coast wrap offset");
    for (auto p : points)
        require(std::isfinite(p.x) && std::isfinite(p.y) && std::isfinite(p.transition),
                "Nonfinite coast perturbation input");
    unsigned coarse = static_cast<unsigned>(points.size()), levels = 0;
    while (coarse >= 6 && levels < 4) {
        if (coarse % 2)
            ++coarse;
        coarse = coarse / 2 + 1;
        ++levels;
    }
    const unsigned padded = levels == 0 ? coarse : (1u << levels) * (coarse - 2) + 2;
    Points current;
    for (unsigned i = 0; i < padded; ++i) {
        const auto p = points[std::min(std::size_t(i), points.size() - 1)];
        current.push_back({p.x, p.y});
    }
    std::vector<Points> details;
    for (unsigned i = 0; i < levels; ++i) {
        details.push_back(detail(current));
        current = reduce(current);
    }
    // Frozen original MSVCR90 _CIpow finite domain, including its PC24 legacy
    // exponent-2 one-ULP difference. Generic host pow/powf changes the coast.
    constexpr std::array<std::uint32_t, 5> bits{0x3e93d2c4, 0x3ebd845e, 0x3ef2f879, 0x3f1bc019, 0x3f47ae14};
    const float initial = std::bit_cast<float>(levels == 3 && precision == X87Precision::single &&
                                                       pow_path == CoastPowPath::legacy_x87
                                                   ? 0x3f1bc01au
                                                   : bits[levels]);
    float amplitude = initial;
    unsigned calls = 0;
    const auto next = [&]() {
        const auto v = random();
        ++calls;
        require(v <= 32767, "Native coast rand value exceeds 32767");
        return v;
    };
    for (unsigned level = 0; level < levels; ++level) {
        auto curve = reconstruct(current, details[levels - 1 - level]);
        const auto displace = [&](std::size_t i, float argument, float amount) {
            const auto d = derivative(curve, argument);
            const float nx = -d.x, xx = nx * nx;
            const double yy = round_x87(double(d.y) * d.y, precision);
            const float length = float(round_x87(std::sqrt(round_x87(yy + xx, precision)), precision));
            require(std::isfinite(length) && length > 0.f, "Degenerate coast perturbation tangent");
            const float inverse = 1.f / length, move_x = (d.y * inverse) * amount,
                        move_y = (inverse * nx) * amount;
            const auto p = curve[i];
            curve[i] = {i == 0 ? move_x + p.x : p.x + move_x, move_y + p.y};
        };
        const auto random_amount = [&]() {
            return ((float(next() % 1000) * .001f - .5f) * amplitude) * 35.f;
        };
        displace(0, 0.f, random_amount());
        for (std::size_t i = 1; i + 1 < curve.size(); ++i)
            displace(i, float(i) - .5f, random_amount());
        next(); // Deliberately consumed and ignored by the original.
        displace(curve.size() - 1, float(curve.size() - 2), amplitude);
        amplitude *= amplitude;
        current = std::move(curve);
    }
    CoastPerturbation out{{points.begin(), points.end()}, levels, padded, calls, initial};
    for (std::size_t i = 0; i < points.size(); ++i) {
        out.points[i].x = current[i].x;
        out.points[i].y = current[i].y;
    }
    if (closed)
        for (unsigned i = 0; i < 2; ++i) {
            const auto p = out.points[out.points.size() - 2 + i];
            const Point2 xy{p.x - wrap.x, p.y - wrap.y};
            out.points[i].x = xy.x;
            out.points[i].y = xy.y;
        }
    return out;
}
} // namespace
CoastPerturbation perturb_coast(std::span<const CoastControlPoint> points, bool closed, Point2 wrap,
                                std::span<const std::uint32_t> randoms, X87Precision precision,
                                CoastPowPath pow_path) {
    std::size_t cursor = 0;
    return perturb(points, closed, wrap, [&]() {
        require(cursor < randoms.size(), "Insufficient explicit native coast rand stream");
        return randoms[cursor++];
    }, precision, pow_path);
}
CoastPerturbation perturb_coast(std::span<const CoastControlPoint> points, bool closed, Point2 wrap,
                                TerrainRandom &random, X87Precision precision, CoastPowPath pow_path) {
    return perturb(points, closed, wrap, [&]() { return random.next(); }, precision, pow_path);
}
} // namespace civ5::terrain
