#include "civ5/terrain/river_raster.hpp"
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
std::uint16_t narrow_height(float value) {
    const auto integer = std::isfinite(value) && double(value) >= -2147483648.0 &&
                                 double(value) < 2147483648.0
                             ? static_cast<std::int32_t>(value)
                             : std::numeric_limits<std::int32_t>::min();
    return static_cast<std::uint16_t>(integer);
}
float interpolate(float a, float b, float c, const std::array<float, 3> &w) {
    // Both original writers multiply and add A, C, B, in that order.
    return a * w[0] + c * w[2] + b * w[1];
}
float clamp_unit(float x) { return x < 0.f ? 0.f : x > 1.f ? 1.f : x; }

void raster(HeightDomain domain, RiverRasterField &field, std::span<const RiverTriangle> triangles,
            RiverPass pass, const RiverRasterSettings &settings, std::uint8_t river_material,
            const RiverWriteObserver &write_observer, const RiverSampleObserver &sample_observer) {
    const auto q = [&](double value) { return round_x87(value, settings.precision); };
    const auto store = [&](RiverWriteChannel channel, std::int32_t index, std::uint32_t value) {
        if (write_observer)
            write_observer(pass, channel, index, value);
    };
    for (std::size_t ti = 0; ti < triangles.size(); ++ti) {
        const auto &[a, b, c] = triangles[ti];
        auto lower = [&](float value) { return float(std::floor(q(double(value) + 0.5))); };
        const float start_xf = lower(std::min({a.x, b.x, c.x})), start_yf = lower(std::min({a.y, b.y, c.y}));
        const auto start_x = static_cast<std::int32_t>(start_xf), start_y = static_cast<std::int32_t>(start_yf);
        const auto end_x = static_cast<std::int32_t>(std::max({a.x, b.x, c.x})) + 1,
                   end_y = static_cast<std::int32_t>(std::max({a.y, b.y, c.y})) + 1;
        require(std::int64_t(std::max(0, end_x - start_x)) * std::max(0, end_y - start_y) <= 16777216,
                "River triangle exceeds bounded raster work");
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
                const std::array<float, 3> w{edge_ax * step_x + row_a,
                                              edge_bx * step_x + row_b + base_b,
                                              edge_cx * step_x + base_c + row_c};
                if (w[0] >= 0.f && w[1] >= 0.f && w[2] >= 0.f) {
                    if (sample_observer)
                        sample_observer(pass, ti, x, y, w);
                    if (y >= 0 && y < domain.height()) {
                        const auto dx = (x + 100 * domain.width()) % domain.width();
                        const auto index = y * domain.width() + dx;
                        const float old_height = float(field.heights[index]) * worker_u16_to_normalized;
                        const auto old_material = field.materials[index];
                        const float blend = interpolate(a.opacity, b.opacity, c.opacity, w);
                        const float target = interpolate(a.height, b.height, c.height, w) * worker_u16_to_normalized;
                        if (pass == RiverPass::banks) {
                            if (!field.bank_visited[index]) {
                                field.bank_visited[index] = 1;
                                store(RiverWriteChannel::visited, index, 1);
                                const float opacity = clamp_unit(blend);
                                const auto height = narrow_height((old_height * (1.f - opacity) + target * opacity) * 65535.f);
                                if (height <= 0x88b8) {
                                    for (std::size_t k = 0; k < 4; ++k)
                                        if (old_material == settings.remap_from[k]) {
                                            field.materials[index] = settings.remap_to[k];
                                            store(RiverWriteChannel::material, index, settings.remap_to[k]);
                                        }
                                }
                                field.heights[index] = height;
                                store(RiverWriteChannel::height, index, height);
                            }
                        } else {
                            const float profile = clamp_unit(interpolate(a.profile, b.profile, c.profile, w) * 3.f);
                            const double angle = q(q(double(profile) * 0.5) * double(3.1414999961853027f));
                            // FSIN does not round to the x87 precision-control
                            // field. Each following arithmetic instruction does.
                            const double depth = q(q(std::sin(angle) * double(0.05000000074505806f)) *
                                                   double(0.3333333432674408f));
                            const double carved = q(double(target) - depth);
                            const double weighted = q(carved * double(blend));
                            const double remaining = q(1.0 - double(blend));
                            const float blended = float(q(weighted + q(remaining * double(old_height))));
                            float lowered = old_height > blended ? blended : old_height;
                            const float minimum = settings.level - 0.05999999865889549f;
                            if (minimum > lowered)
                                lowered = minimum > old_height ? old_height : minimum;
                            const auto height = narrow_height(lowered * 65535.f);
                            field.heights[index] = height;
                            store(RiverWriteChannel::height, index, height);
                            field.materials[index] = river_material;
                            store(RiverWriteChannel::material, index, river_material);
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

RiverRasterField apply_river_triangles(HeightDomain domain, std::span<const std::uint16_t> heights,
                                       std::span<const std::uint8_t> materials,
                                       std::span<const RiverTriangle> banks,
                                       std::span<const RiverTriangle> channels,
                                       const RiverRasterSettings &settings,
                                       const RiverWriteObserver &write_observer,
                                       const RiverSampleObserver &sample_observer,
                                       const RiverPassComplete &pass_complete) {
    require(domain.width() <= 65536 && domain.height() <= 65536 &&
                std::uint64_t(domain.width()) * domain.height() <= 16777216,
            "River field exceeds supported domain");
    require(heights.size() == std::size_t(domain.width()) * domain.height() && materials.size() == heights.size(),
            "Incomplete river fields");
    require(std::isfinite(settings.level), "Nonfinite river level");
    require(settings.precision == X87Precision::single || settings.precision == X87Precision::double_precision,
            "Unsupported river precision");
    require(settings.material_codes.size() <= 256, "River material table exceeds byte ordinals");
    for (auto stream : {banks, channels}) {
        require(stream.size() <= 65536, "River stream exceeds supported triangle count");
        for (const auto &t : stream)
            for (const auto &v : {t.a, t.b, t.c})
                require(std::isfinite(v.x) && std::isfinite(v.y) && std::isfinite(v.height) &&
                            std::isfinite(v.profile) && std::isfinite(v.opacity) &&
                            std::abs(v.x) <= 65536.f && std::abs(v.y) <= 65536.f &&
                            v.x >= -float(domain.width()) && v.x <= 2.f * float(domain.width()),
                        "Unsupported river vertex");
    }
    const auto code = settings.river_code | 0x30u;
    const auto found = std::find(settings.material_codes.begin(), settings.material_codes.end(), code);
    const auto material = found == settings.material_codes.end() ? std::uint8_t(255)
                            : static_cast<std::uint8_t>(found - settings.material_codes.begin());
    RiverRasterField result{{heights.begin(), heights.end()}, {materials.begin(), materials.end()},
                             std::vector<std::uint8_t>(heights.size())};
    raster(domain, result, banks, RiverPass::banks, settings, material, write_observer, sample_observer);
    if (pass_complete)
        pass_complete(RiverPass::banks, result);
    raster(domain, result, channels, RiverPass::channels, settings, material, write_observer, sample_observer);
    if (pass_complete)
        pass_complete(RiverPass::channels, result);
    return result;
}
} // namespace civ5::terrain
