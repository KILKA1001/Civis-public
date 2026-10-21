#include "civ5/materials/water_reflection.hpp"
#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace civ5::materials {
namespace {
using namespace terrain;
void require(bool ok, const char *why) {
    if (!ok)
        throw std::invalid_argument(why);
}
bool finite(const auto &values) {
    return std::all_of(values.begin(), values.end(), [](float f) { return std::isfinite(f); });
}
struct Math {
    X87Precision precision;
    double rounded(double f) const { return round_x87(f, precision); }
    double mul(double a, double b) const { return rounded(a * b); }
    double add(double a, double b) const { return rounded(a + b); }
    double sub(double a, double b) const { return rounded(a - b); }
    Vector3 cross(const Vector3 &a, const Vector3 &b) const {
        Vector3 result;
        for (unsigned i = 0; i < 3; ++i) {
            const auto j = (i + 1) % 3, k = (i + 2) % 3;
            result[i] = static_cast<float>(sub(mul(a[j], b[k]), mul(a[k], b[j])));
        }
        return result;
    }
    float negative_dot(const Vector3 &a, const Vector3 &b) const {
        return -static_cast<float>(add(add(mul(a[0], b[0]), mul(a[1], b[1])), mul(a[2], b[2])));
    }
    Matrix4 look_at(const Vector3 &eye, const Vector3 &center, const ReciprocalSqrtEstimate &estimate) const {
        Vector3 delta;
        for (unsigned i = 0; i < 3; ++i)
            delta[i] = static_cast<float>(sub(eye[i], center[i]));
        const auto z = normalize3_sse(delta, estimate);
        const auto x = normalize3_sse(cross({0, 1, 0}, z), estimate);
        const auto y = cross(z, x);
        return {x[0],
                y[0],
                z[0],
                0,
                x[1],
                y[1],
                z[1],
                0,
                x[2],
                y[2],
                z[2],
                0,
                negative_dot(x, eye),
                negative_dot(y, eye),
                negative_dot(z, eye),
                1};
    }
    Matrix4 projection(float half_x, float half_y) const {
        const float left = half_x, right = -half_x, bottom = half_y, top = -half_y;
        const auto inv_x = static_cast<float>(rounded(1. / sub(right, left)));
        const auto inv_y = static_cast<float>(rounded(1. / sub(top, bottom)));
        const auto inv_z = static_cast<float>(rounded(1. / sub(30000., -30000.)));
        return {static_cast<float>(mul(inv_x, 2)),
                0,
                0,
                0,
                0,
                static_cast<float>(mul(inv_y, 2)),
                0,
                0,
                0,
                0,
                inv_z,
                0,
                static_cast<float>(-mul(add(left, right), inv_x)),
                static_cast<float>(-mul(add(bottom, top), inv_y)),
                static_cast<float>(mul(inv_z, 30000)),
                1};
    }
    Matrix4 multiply(const Matrix4 &a, const Matrix4 &b) const {
        const auto result = matrix_multiply(a, b, precision, MatrixPath::sse);
        require(finite(result), "Reflection matrix product overflow");
        return result;
    }
};
} // namespace
WaterReflectionResource water_reflection_resource(std::uint32_t quality, std::uint32_t reflections) {
    const unsigned mode = quality == 1 ? 2 : quality == 2 ? 3 : 0;
    const bool target = mode != 0 && reflections == 1;
    const unsigned extent = target ? 1024 : 32;
    return {mode, extent, extent, target};
}
assets::Rgba8Level water_reflection_fallback() { return {32, 32, assets::Bytes(4096, 0)}; }
WaterReflectionDirections water_reflection_directions(const WaterReflectionAngles &angles,
                                                      X87Precision precision) {
    require(precision == X87Precision::single || precision == X87Precision::double_precision,
            "Invalid reflection direction precision");
    const std::array values{angles.light_azimuth, angles.light_polar, angles.reflection_azimuth,
                            angles.reflection_polar};
    require(finite(values), "Nonfinite reflection direction angle");
    const Math math{precision};
    // Original double constant at 9DBD30; never round the degree-to-radian
    // factor to float before the x87-precision multiplication.
    constexpr auto radians = std::bit_cast<double>(std::uint64_t{0x3f91df46a2529d39});
    const auto direction = [&](float azimuth, float polar) -> Vector3 {
        const auto a = math.mul(azimuth, radians), p = math.mul(polar, radians);
        // FSIN/FCOS retain extended results independently of the x87 precision
        // field. Apply the selected precision to FMUL, then store float32.
        return {static_cast<float>(-math.mul(std::sin(p), std::cos(a))),
                static_cast<float>(-math.mul(std::sin(p), std::sin(a))), static_cast<float>(std::cos(p))};
    };
    return {direction(angles.light_azimuth, angles.light_polar),
            direction(angles.reflection_azimuth, angles.reflection_polar)};
}
WaterReflectionMatrices water_reflection_matrices(const WaterReflectionFrame &frame,
                                                  const WaterReflectionMatrices &prior,
                                                  X87Precision precision,
                                                  const ReciprocalSqrtEstimate &estimate) {
    require(precision == X87Precision::single || precision == X87Precision::double_precision,
            "Invalid reflection matrix precision");
    require(bool(estimate), "Reflection matrix requires an RSQRTSS estimate provider");
    require(frame.view_count <= 4, "At most four reflection views are supported");
    for (const auto &v :
         {frame.bounds_low, frame.bounds_high, frame.shadow_direction, frame.reflection_direction})
        require(finite(v), "Nonfinite reflection frame vector");
    require(frame.bounds_low[0] < frame.bounds_high[0] && frame.bounds_low[1] < frame.bounds_high[1],
            "Reflection XY bounds must have positive extent");
    if (frame.view_count > 1)
        require(finite(frame.base_view_inverse), "Nonfinite base view inverse");
    for (unsigned i = 1; i < frame.view_count; ++i)
        require(finite(frame.extra_views[i - 1]), "Nonfinite extra reflection view");
    Vector3 center, shadow_eye, reflection_eye;
    for (unsigned i = 0; i < 3; ++i) {
        center[i] = (frame.bounds_high[i] + frame.bounds_low[i]) * .5f;
        shadow_eye[i] = frame.shadow_direction[i] * 5000.f + center[i];
        reflection_eye[i] = -(frame.reflection_direction[i] * 5000.f) + center[i];
    }
    require(finite(center) && finite(shadow_eye) && finite(reflection_eye), "Reflection eye overflow");
    const auto half_x = (-frame.bounds_high[0] + frame.bounds_low[0]) * .5f;
    const auto half_y = (-frame.bounds_high[1] + frame.bounds_low[1]) * .5f;
    require(std::isfinite(half_x) && std::isfinite(half_y), "Reflection projection extent overflow");
    const Math math{precision};
    auto result = prior;
    const auto shadow_view = math.look_at(shadow_eye, center, estimate);
    result.reflection_projection = math.projection(half_x, half_y);
    result.shadow[0] = math.multiply(shadow_view, result.reflection_projection);
    for (unsigned i = 1; i < frame.view_count; ++i) {
        const auto relative = math.multiply(frame.extra_views[i - 1], frame.base_view_inverse);
        result.shadow[i] = math.multiply(relative, result.shadow[0]);
    }
    result.reflection_view[0] = math.look_at(reflection_eye, center, estimate);
    result.reflection[0] = math.multiply(result.reflection_view[0], result.reflection_projection);
    for (unsigned i = 1; i < frame.view_count; ++i) {
        const auto relative = math.multiply(frame.extra_views[i - 1], frame.base_view_inverse);
        result.reflection[i] = math.multiply(relative, result.reflection[0]);
        result.reflection_view[i] = math.multiply(relative, result.reflection_view[0]);
    }
    return result;
}
WaterReflectionFrameState
water_reflection_frame(const terrain::WorldLayout &world, const WaterReflectionCamera &camera,
                       const WaterReflectionFrameSettings &settings, const WaterReflectionFrameState &prior,
                       X87Precision precision, const ReciprocalSqrtEstimate &estimate) {
    require(camera.view_count <= camera.frusta.size(), "At most four reflection camera views are supported");
    WaterReflectionFrameState result;
    result.bounds = terrain::view_bounds(world, std::span(camera.frusta).first(camera.view_count),
                                         settings.source_scales, prior.bounds, precision);
    result.directions = water_reflection_directions(settings.angles, precision);
    const WaterReflectionFrame frame{result.bounds.raw_low[0],  result.bounds.raw_high[0],
                                     settings.shadow_direction, result.directions.reflection,
                                     camera.view_count,         camera.base_view_inverse,
                                     camera.extra_views};
    result.matrices = water_reflection_matrices(frame, prior.matrices, precision, estimate);
    return result;
}
WaterCameraFrameState water_camera_frame(const terrain::WorldLayout &world, std::int32_t world_mode,
                                         const terrain::PerspectiveCamera &camera,
                                         const WaterReflectionFrameSettings &settings,
                                         const WaterCameraFrameState &prior, X87Precision precision,
                                         const ReciprocalEstimate &reciprocal,
                                         const ReciprocalSqrtEstimate &reciprocal_sqrt) {
    WaterCameraFrameState result;
    result.camera = camera_frame(world, world_mode, camera, prior.camera, precision, reciprocal);
    const auto &c = result.camera;
    const WaterReflectionCamera reflection_camera{
        c.frusta, c.view_count, c.inverse_views[0], {c.views[1], c.views[2], c.views[3]}};
    result.water =
        water_reflection_frame(world, reflection_camera, settings, prior.water, precision, reciprocal_sqrt);
    return result;
}
} // namespace civ5::materials
