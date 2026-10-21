#include "civ5/terrain/river_surface.hpp"
#include "river_curve.hpp"
#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace civ5::terrain {
RiverSurfaceMesh build_river_surface(const RiverPolylines &centers,
                                     const std::array<RiverPolylines, 2> &banks, const RiverHeights &heights,
                                     const RiverSurfaceSettings &settings,
                                     const RiverCurveObserver &observer) {
    if (centers.size() != heights.size() || banks[0].size() != centers.size() ||
        banks[1].size() != centers.size())
        throw std::invalid_argument("River surface family count mismatch");
    if ((settings.precision != X87Precision::single &&
         settings.precision != X87Precision::double_precision) ||
        !std::isfinite(settings.height_scale) || settings.height_scale <= 0 ||
        !std::all_of(settings.offset.begin(), settings.offset.end(),
                     [](float f) { return std::isfinite(f); }))
        throw std::invalid_argument("Invalid river surface settings");
    const auto rounded = [&](double v) { return round_x87(v, settings.precision); };
    RiverSurfaceMesh out;
    for (std::size_t line = 0; line < centers.size(); ++line) {
        const auto size = centers[line].points.size();
        if (size > 1048576 || heights[line].size() != size || banks[0][line].points.size() != size ||
            banks[1][line].points.size() != size)
            throw std::invalid_argument("River surface requires complete corresponding controls and heights");
        std::array<std::vector<RiverPoint>, 2> p{banks[0][line].points, banks[1][line].points};
        for (auto &family : p)
            for (std::size_t i = 0; i < size; ++i) {
                family[i][2] = heights[line][i];
                for (float f : family[i])
                    if (!std::isfinite(f))
                        throw std::invalid_argument("Nonfinite river surface control");
            }
        // The native center family is copied and measured, but the maximum
        // center-to-bank width is dead after 659908. Retain input validation.
        for (const auto &point : centers[line].points)
            if (!std::isfinite(point[0]) || !std::isfinite(point[1]))
                throw std::invalid_argument("Nonfinite river surface center");
        if (size < 4)
            continue;
        const auto sample = [&](unsigned bank, float t) {
            return detail::river_curve(p[bank], t, false, observer);
        };
        const float maximum = float(size - 2), limit = maximum + 9.999999747378752e-05f;
        const auto end0 = sample(0, maximum), end1 = sample(1, maximum);
        float previous_x = (end0[0] + end1[0]) * .5f;
        float previous_y = (end0[1] + end1[1]) * .5f;
        float distance = centers[line].terminal ? 0.f : 640.f;
        for (float t = 0; t <= limit; t += .1f) {
            // Native reverse parameter is computed before clamping t.
            const float reverse = std::clamp(maximum - t, 0.f, maximum);
            t = std::clamp(t, 0.f, maximum);
            const auto a = sample(0, reverse), b = sample(1, reverse);
            const auto position = [&](const RiverPoint &v) {
                RiverPoint r{v[0] - settings.offset[0], v[1] * .866f - settings.offset[1],
                             ((v[2] - settings.offset[2]) * .003921568859368563f) * settings.height_scale};
                if (!settings.high_quality)
                    r[2] += .25f;
                if (r[2] < 10.899999618530273f)
                    r[2] += .25f;
                return r;
            };
            auto left = position(a), right = position(b);
            const float center_x = (a[0] + b[0]) * .5f, center_y = (a[1] + b[1]) * .5f;
            const float dx = -center_x + previous_x, dy = -center_y + previous_y;
            const float squared_x = dx * dx, squared_y = dy * dy;
            const double length = rounded(std::sqrt(rounded(double(squared_y) + squared_x)));
            distance = float(rounded(length + distance));
            const float u = distance * .01f;
            const float wx = left[0] + -right[0], wy = -right[1] + left[1], wz = -right[2] + left[2];
            const auto square = [&](float f) { return rounded(double(f) * f); };
            double width = rounded(square(wz) + square(wy));
            width = rounded(width + square(wx));
            width = rounded(std::sqrt(width));
            const float half_width = float(rounded(width * .05000000074505806f)) * .5f;
            const float v_left = (.5f - half_width) * .8f;
            const float v_right = ((half_width + .5f) * 4.f + 1.f) * .2f;
            const float remaining = limit - t;
            const float fade = remaining < 1.f ? 1.f - remaining * .5f
                               : remaining > 1.f && distance * .008196720853447914f > .5f
                                   ? .5f
                                   : distance * .008196720853447914f;
            if (!centers[line].terminal && t < 1.100000023841858f) {
                left[2] += .5f;
                right[2] += .5f;
            }
            if (out.vertices.size() > 1048574 || out.indices.size() > 1048570)
                throw std::invalid_argument("River surface exceeds supported mesh extent");
            for (const auto &v : {RiverSurfaceVertex{left[0], left[1], left[2], u, v_left, fade, 0, 0},
                                  RiverSurfaceVertex{right[0], right[1], right[2], u, v_right, fade, 1, 0}}) {
                if (!std::all_of(v.begin(), v.end(), [](float f) { return std::isfinite(f); }))
                    throw std::invalid_argument("River surface arithmetic overflow");
                out.vertices.push_back(v);
            }
            const auto n = static_cast<std::uint32_t>(out.vertices.size());
            if (t >= .1f)
                out.indices.insert(out.indices.end(), {n - 4, n - 2, n - 1, n - 4, n - 1, n - 3});
            previous_x = center_x;
            previous_y = center_y;
        }
    }
    return out;
}
} // namespace civ5::terrain
