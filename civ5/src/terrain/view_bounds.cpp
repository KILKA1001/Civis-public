#include "civ5/terrain/view_bounds.hpp"
#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>

namespace civ5::terrain {
namespace {
void require(bool ok, const char *why) {
    if (!ok)
        throw std::invalid_argument(why);
}
bool finite(const auto &values) {
    return std::all_of(values.begin(), values.end(), [](float v) { return std::isfinite(v); });
}
Vector3 cross(const Vector4 &a, const Vector4 &b) {
    return {a[1] * b[2] - b[1] * a[2], b[0] * a[2] - a[0] * b[2], a[0] * b[1] - b[0] * a[1]};
}
Vector3 intersection(const Vector4 &a, const Vector4 &b, const Vector4 &c) {
    // 8EE590 uses separately rounded scalar SSE operations. Its failed
    // intersection leaves the caller's zero-initialized point unchanged.
    const auto bc = cross(b, c), ca = cross(c, a), ab = cross(a, b);
    const float denominator = (a[0] * bc[0] + a[1] * bc[1]) + a[2] * bc[2];
    require(std::isfinite(denominator), "View plane intersection overflow");
    if (std::abs(denominator) < std::bit_cast<float>(std::uint32_t{0x38d1b717}))
        return {};
    const float inverse = 1.f / denominator;
    Vector3 result;
    for (unsigned axis = 0; axis < 3; ++axis)
        result[axis] = inverse * (((-a[3]) * bc[axis] - ca[axis] * b[3]) - ab[axis] * c[3]);
    require(finite(result), "Nonfinite view intersection");
    return result;
}
Vector3 rounded_cube(const Vector4 &v, X87Precision precision) {
    require(finite(v), "Nonfinite visible cube coordinate");
    Vector3 result;
    for (unsigned axis = 0; axis < 3; ++axis) {
        const auto sum = round_x87(double(v[axis]) + (v[axis] >= 0.f ? .5 : -.5), precision);
        result[axis] = static_cast<float>(v[axis] >= 0.f ? std::floor(sum) : std::ceil(sum));
    }
    const auto sum = result[0] + (result[1] + result[2]);
    if (sum != 0.f) {
        Vector3 delta;
        for (unsigned axis = 0; axis < 3; ++axis)
            delta[axis] = sum > 0.f ? v[axis] - result[axis] : result[axis] - v[axis];
        const auto minimum = std::min(std::min(delta[0], delta[1]), delta[2]);
        if (minimum == delta[0])
            result[0] = -(result[1] + result[2]);
        else if (minimum == delta[1])
            result[1] = -(result[0] + result[2]);
        else
            result[2] = -(result[0] + result[1]);
    }
    return result;
}
std::int32_t cvtt(float value) {
    // Preserve original CVTTSS2SI's invalid-result word without signed C++ UB.
    if (!std::isfinite(value) || double(value) < -2147483648. || double(value) >= 2147483648.)
        return std::numeric_limits<std::int32_t>::min();
    return static_cast<std::int32_t>(value);
}
TileBoundsPoint tile_point(const Vector3 &v, const IntMatrix4 &matrix, std::int32_t border) {
    const auto x = cvtt(v[0]), y = cvtt(v[1]), z = cvtt(v[2]);
    const auto component = [&](unsigned axis) {
        return wrap_add(wrap_mul(matrix[axis], x),
                        wrap_add(wrap_add(wrap_mul(matrix[4 + axis], y), matrix[12 + axis]),
                                 wrap_mul(matrix[8 + axis], z)));
    };
    const auto row = component(1);
    return {wrap_add(wrap_add(row >> 1, component(0)), border), wrap_add(row, border)};
}
} // namespace
std::array<Vector3, 4> frustum_horizontal_corners(const ViewFrustum &planes,
                                                   float height) {
    require(std::isfinite(height), "Nonfinite horizontal view plane height");
    for (unsigned side = 2; side < 6; ++side)
        require(finite(planes[side]), "Nonfinite side frustum plane");
    const Vector4 horizontal{0, 0, 1, -height};
    return {{intersection(planes[3], planes[4], horizontal),
             intersection(planes[3], planes[5], horizontal),
             intersection(planes[2], planes[4], horizontal),
             intersection(planes[2], planes[5], horizontal)}};
}
std::array<Vector3, 2> frustum_ground_bounds(const ViewFrustum &planes) {
    for (unsigned side = 2; side < 6; ++side)
        require(finite(planes[side]), "Nonfinite side frustum plane");
    std::array<Vector3, 8> points;
    unsigned cursor = 0;
    for (float height : {0.f, -127.5f})
        for (unsigned x : {2u, 3u})
            for (unsigned y : {4u, 5u})
                points[cursor++] = intersection({0, 0, 1, height}, planes[x], planes[y]);
    Vector3 low{points[0][0], points[0][1], 0}, high = low;
    for (unsigned p = 1; p < points.size(); ++p)
        for (unsigned axis = 0; axis < 2; ++axis) {
            if (high[axis] <= points[p][axis])
                high[axis] = points[p][axis];
            if (points[p][axis] <= low[axis])
                low[axis] = points[p][axis];
        }
    return {low, high};
}
ViewBounds view_bounds(const WorldLayout &world, std::span<const ViewFrustum> frusta,
                       const std::array<float, 2> &scales, const ViewBounds &prior, X87Precision precision) {
    require(frusta.size() <= 4, "At most four visibility frusta are supported");
    require(precision == X87Precision::single || precision == X87Precision::double_precision,
            "Invalid view-bounds precision");
    auto result = prior;
    if (frusta.empty())
        return result;
    require(finite(scales) && scales[0] > 0 && scales[1] > 0, "Positive finite view source scales required");
    require(finite(world.bounds) && world.source_grid.width > 0 && world.source_grid.height > 0,
            "Invalid view world bounds/grid");
    Matrix4 transform;
    std::copy_n(world.transforms.begin(), 16, transform.begin());
    require(finite(transform), "Nonfinite view world transform");
    for (std::size_t view = 0; view < frusta.size(); ++view) {
        const auto [low, high] = frustum_ground_bounds(frusta[view]);
        result.raw_low[view] = result.clipped_low[view] = low;
        result.raw_high[view] = result.clipped_high[view] = high;
        for (unsigned axis = 0; axis < 2; ++axis) {
            if (result.clipped_low[view][axis] < world.bounds[axis])
                result.clipped_low[view][axis] = world.bounds[axis];
            if (world.bounds[3 + axis] < result.clipped_high[view][axis])
                result.clipped_high[view][axis] = world.bounds[3 + axis];
        }
        const auto projected = [&](const Vector3 &bound) {
            const Vector4 point{(world.bounds[6] * .5f + bound[0]) * (1.f / scales[0]),
                                (world.bounds[7] * .5f + bound[1]) * (1.f / scales[1]), 0, 1};
            return rounded_cube(vector4_transform(point, transform, precision, Vec4Path::sse), precision);
        };
        result.raw_tile_low[view] = tile_point(projected(low), world.source_grid.first_matrix, -1);
        result.raw_tile_high[view] = tile_point(projected(high), world.source_grid.first_matrix, 1);
        const std::array limits{world.source_grid.width, world.source_grid.height};
        for (unsigned axis = 0; axis < 2; ++axis) {
            result.clipped_tile_low[view][axis] =
                std::clamp(wrap_add(result.raw_tile_low[view][axis], -1), 0, limits[axis]);
            result.clipped_tile_high[view][axis] =
                std::clamp(wrap_add(result.raw_tile_high[view][axis], 1), 0, limits[axis]);
        }
    }
    return result;
}
} // namespace civ5::terrain
