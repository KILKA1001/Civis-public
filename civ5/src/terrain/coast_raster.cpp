#include "civ5/terrain/coast_raster.hpp"
#include "civ5/terrain/worker_field.hpp"
#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>

namespace civ5::terrain {
namespace {
void require(bool ok, const char *message) {
    if (!ok)
        throw std::invalid_argument(message);
}
void raster(HeightDomain domain, std::span<std::uint16_t> heights, std::span<std::uint8_t> materials,
            const CoastRasterJob &job, bool wrap_x, CoastBoundsPrecision precision,
            const CoastWriteObserver &observer) {
    for (const auto &triangle : job.triangles()) {
        const auto &a = triangle.a;
        const auto &b = triangle.b;
        const auto &c = triangle.c;
        for (const auto &v : {a, b, c}) {
            require(std::abs(v.x) <= 65536.f && std::abs(v.y) <= 65536.f && v.x >= -float(domain.width()) &&
                        v.x <= 2.f * float(domain.width()),
                    "Unsupported coast coordinates");
            if (!wrap_x)
                require(v.x >= 0.f && v.x < float(domain.width()), "Nonwrapping coast outside X domain");
        }
        auto lower = [&](float value) {
            const double sum = double(value) + 0.5;
            return float(std::floor(precision == CoastBoundsPrecision::single ? double(float(sum)) : sum));
        };
        const float start_xf = lower(std::min({a.x, b.x, c.x})), start_yf = lower(std::min({a.y, b.y, c.y}));
        const auto start_x = static_cast<std::int32_t>(start_xf),
                   start_y = static_cast<std::int32_t>(start_yf);
        const auto end_x = static_cast<std::int32_t>(std::max({a.x, b.x, c.x})) + 1;
        const auto end_y = static_cast<std::int32_t>(std::max({a.y, b.y, c.y})) + 1;
        require(std::int64_t(std::max(0, end_x - start_x)) * std::max(0, end_y - start_y) <= 16777216,
                "Coast triangle exceeds bounded raster work");
        const float ax = a.x - start_xf, ay = a.y - start_yf;
        const float bx = b.x - start_xf, by = b.y - start_yf;
        const float cx = c.x - start_xf, cy = c.y - start_yf;
        const float edge_ax = by - cy, edge_ay = cx - bx, edge_ac = cy * bx - cx * by;
        const float edge_bx = cy - ay, edge_by = ax - cx, edge_bc = cx * ay - cy * ax;
        const float edge_cx = ay - by, edge_cy = bx - ax, edge_cc = by * ax - bx * ay;
        const float determinant = edge_ay * ay + edge_ax * ax + edge_ac;
        if (std::abs(double(determinant)) < 1e-5)
            continue;
        const float inverse = 1.f / determinant;
        float step_y = 0.f;
        for (auto y = start_y; y < end_y; ++y) {
            float step_x = 0.f;
            const float row_a = edge_ac * inverse + edge_ay * step_y;
            const float row_b = edge_by * step_y, row_c = edge_cy * step_y;
            const float base_b = edge_bc * inverse, base_c = edge_cc * inverse;
            for (auto x = start_x; x < end_x; ++x) {
                const float wa = edge_ax * step_x + row_a;
                const float wb = edge_bx * step_x + base_b + row_b;
                const float wc = edge_cx * step_x + base_c + row_c;
                if (wa >= 0.f && wb >= 0.f && wc >= 0.f && y >= 0 && y < domain.height()) {
                    const auto dx = wrap_x ? (x % domain.width() + domain.width()) % domain.width() : x;
                    require(dx >= 0 && dx < domain.width(), "Coast destination outside X domain");
                    const auto index = y * domain.width() + dx;
                    const auto old_material = materials[index];
                    if (a.material != 255) {
                        materials[index] = a.material;
                        if (observer)
                            observer(CoastWriteChannel::material, index, a.material);
                    }
                    const float quadratic = a.opacity * wa * wa + b.opacity * wb * wb + c.opacity * wc * wc +
                                            triangle.edge_ab * wb * wa * 2.f +
                                            triangle.edge_bc * wc * wb * 2.f +
                                            triangle.edge_ca * wc * wa * 2.f;
                    // Preserve NaN through CVTTSS2SI; max(0,NaN) in std::max
                    // would silently turn an invalid arithmetic result into 0.
                    // MAXSS +0, quadratic also returns its second operand for
                    // signed-zero ties (as well as unordered comparisons).
                    const float blend = quadratic < 0.f ? 0.f : quadratic;
                    const float old_height = float(heights[index]) * worker_u16_to_normalized;
                    const float target = c.height * wc + b.height * wb + a.height * wa;
                    const float scaled = (target * blend + old_height * (1.f - blend)) * 65535.f;
                    const auto integer = std::isfinite(scaled) && double(scaled) >= -2147483648.0 &&
                                                 double(scaled) < 2147483648.0
                                             ? static_cast<std::int32_t>(scaled)
                                             : std::numeric_limits<std::int32_t>::min();
                    const auto height = static_cast<std::uint16_t>(integer);
                    heights[index] = height;
                    if (observer)
                        observer(CoastWriteChannel::height, index, height);
                    if (height <= 0x88b8) {
                        // All four tests read ORIGINAL material, not the override
                        // or preceding remap. Duplicate keys produce real stores.
                        for (std::size_t i = 0; i < 4; ++i)
                            if (old_material == job.remap_from()[i]) {
                                materials[index] = job.remap_to()[i];
                                if (observer)
                                    observer(CoastWriteChannel::material, index, job.remap_to()[i]);
                            }
                    }
                }
                step_x += inverse;
            }
            step_y += inverse;
        }
    }
}
} // namespace
CoastRasterJob::CoastRasterJob(std::span<const CoastTriangle> triangles,
                               std::array<std::uint8_t, 4> remap_from, std::array<std::uint8_t, 4> remap_to)
    : from_(remap_from), to_(remap_to) {
    for (const auto &t : triangles) {
        for (const auto &v : {t.a, t.b, t.c})
            require(std::isfinite(v.x) && std::isfinite(v.y) && std::isfinite(v.height) &&
                        std::isfinite(v.opacity),
                    "Nonfinite coast vertex");
        require(std::isfinite(t.edge_ab) && std::isfinite(t.edge_bc) && std::isfinite(t.edge_ca),
                "Nonfinite coast edge coefficient");
    }
    triangles_.assign(triangles.begin(), triangles.end());
}
CoastRasterField::CoastRasterField(HeightDomain domain, std::span<const std::uint16_t> heights,
                                   std::span<const std::uint8_t> materials)
    : domain_(domain), heights_(heights.begin(), heights.end()),
      materials_(materials.begin(), materials.end()) {}
CoastRasterField apply_coast_jobs(HeightDomain domain, std::span<const std::uint16_t> heights,
                                  std::span<const std::uint8_t> materials,
                                  std::span<const CoastRasterJob> ordered_jobs, bool wrap_x,
                                  CoastBoundsPrecision precision, const CoastWriteObserver &observer,
                                  const CoastJobComplete &job_complete) {
    require(domain.width() <= 65536 && domain.height() <= 65536, "Coast domain exceeds supported addressing");
    require(heights.size() == std::size_t(domain.width()) * domain.height() &&
                materials.size() == heights.size(),
            "Incomplete coast source fields");
    require(precision == CoastBoundsPrecision::single ||
                precision == CoastBoundsPrecision::double_or_extended,
            "Unsupported coast bounds precision");
    CoastRasterField result(domain, heights, materials);
    for (std::size_t i = 0; i < ordered_jobs.size(); ++i) {
        raster(domain, result.heights_, result.materials_, ordered_jobs[i], wrap_x, precision, observer);
        if (job_complete)
            job_complete(i);
    }
    return result;
}
} // namespace civ5::terrain
