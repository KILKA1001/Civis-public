#include "civ5/materials/terrain_frame.hpp"
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
void precision_check(X87Precision precision) {
    require(precision == X87Precision::single || precision == X87Precision::double_precision,
            "Invalid terrain frame precision");
}
bool finite(const auto &values) {
    return std::all_of(values.begin(), values.end(), [](float f) { return std::isfinite(f); });
}
Vector3 startup_direction(double azimuth, double polar, X87Precision precision) {
    // Force the runtime transcendental implementation used by the validated
    // original x87 profile; constant folding uses different argument reduction.
    volatile double a = azimuth, p = polar;
    return {static_cast<float>(-round_x87(std::sin(p) * std::cos(a), precision)),
            static_cast<float>(-round_x87(std::sin(p) * std::sin(a), precision)),
            static_cast<float>(std::cos(p))};
}
Vector3 half_vector(const Vector3 &sun, const Vector3 &forward, const ReciprocalSqrtEstimate &estimate) {
    Vector3 sum;
    for (unsigned i = 0; i < 3; ++i)
        sum[i] = sun[i] + forward[i];
    return normalize3_sse(sum, estimate);
}
} // namespace

TerrainLighting terrain_lighting_defaults(X87Precision precision) {
    precision_check(precision);
    const auto value = [](std::uint64_t bits) { return std::bit_cast<double>(bits); };
    return {{90, 25, 80, 135},
            .5f,
            startup_direction(value(0x4004f1a6c638d03f), value(0x3fe0c152382d7365), precision),
            startup_direction(value(0x3ff921fb54442d18), value(0x3fdbecde5da115a9), precision),
            startup_direction(value(0x3ff657184ae74487), value(0x4002d97c7f3321d2), precision)};
}
TerrainLighting terrain_lighting_update(const TerrainLighting &state, X87Precision precision) {
    precision_check(precision);
    require(finite(state.shadow) && std::isfinite(state.scalar), "Nonfinite terrain lighting state");
    auto result = state;
    const auto directions = water_reflection_directions(state.angles, precision);
    result.sun = directions.light;
    result.reflection = directions.reflection;
    return result;
}
TerrainCacheLighting terrain_cache_lighting(const TerrainLighting &lighting,
                                            const ReciprocalSqrtEstimate &estimate) {
    require(finite(lighting.shadow), "Nonfinite terrain shadow direction");
    return {lighting.shadow, half_vector(lighting.sun, {0, 0, 1}, estimate)};
}
std::array<Vector4, 2> terrain_fog_constants(const DistanceFogFrame &fog, float camera_height,
                                             X87Precision precision) {
    precision_check(precision);
    require(finite(fog.color) && finite(fog.parameters) && std::isfinite(camera_height) &&
                fog.parameters[0] != 0,
            "Invalid terrain fog frame");
    const auto q = [&](double value) { return round_x87(value, precision); };
    // Original inline x87 exp: FLDL2E, FRNDINT, F2XM1, FADD, FSCALE.
    const double product = q(static_cast<double>(fog.parameters[0]) * camera_height);
    const double exponent = q(-product * std::log2(std::exp(1.)));
    const double integral = std::nearbyint(exponent);
    const double fraction = q(exponent - integral);
    const double reduced = q(std::expm1(fraction * std::log(2.)) + 1.);
    require(integral >= -16384 && integral <= 16384, "Terrain frame fog exponent overflow");
    const double exponential = std::ldexp(reduced, static_cast<int>(integral));
    const double reciprocal = q(-1. / fog.parameters[0]);
    const float density = static_cast<float>(q(q(exponential * reciprocal) * fog.parameters[1]));
    require(std::isfinite(density), "Terrain frame fog result overflow");
    return {fog.color, Vector4{-fog.parameters[0], density, 0, fog.parameters[3]}};
}
TerrainFrameState terrain_frame(const TerrainFrameInput &in, const TerrainFrameState &prior,
                                X87Precision precision, const ReciprocalSqrtEstimate &estimate) {
    precision_check(precision);
    require(bool(estimate), "Terrain frame requires an RSQRTSS estimate provider");
    for (const auto *matrix :
         {&in.view, &in.inverse_view, &in.projection, &in.shadow_projection, &in.overlay_projection})
        require(finite(*matrix), "Nonfinite terrain frame matrix");
    for (const auto *vector : {&in.world_scales, &in.shadow_direction, &in.sun_direction})
        require(finite(*vector), "Nonfinite terrain frame vector");
    require(finite(in.overlay) && finite(in.fog_color), "Nonfinite terrain frame overlay/fog");
    require(
        finite(std::array{in.fog_density, in.fog_strength, in.fog_distance, in.texture_extent, in.control}) &&
            in.fog_density != 0,
        "Invalid terrain frame fog or control");
    auto result = prior;
    auto &c = result.constants;
    const auto matrix = [&](unsigned start, const Matrix4 &values) {
        for (unsigned i = 0; i < 16; ++i)
            c[start + i / 4][i % 4] = values[i];
    };
    const auto vector = [&](unsigned start, const Vector3 &values) {
        std::copy(values.begin(), values.end(), c[start].begin());
    };
    matrix(0, matrix_multiply(in.view, in.projection, precision, MatrixPath::sse));
    matrix(4, in.overlay_projection);
    c[9] = in.overlay;
    c[10] = {-.125f, .875f, -.875f, .125f};
    c[11] = {.375f, -1.5f, 1.125f, 0};
    c[12] = {-.375f, 0, .375f, 0};
    c[13] = {.125f, .75f, .125f, 0};
    result.minimum_level = std::min(prior.minimum_level, in.level);
    result.maximum_level = std::max(prior.maximum_level, in.level);
    c[14][0] = static_cast<float>(in.level);
    c[15][0] = 1;
    vector(16, in.world_scales);
    vector(17, in.shadow_direction);
    vector(18, in.sun_direction);
    vector(19, {in.inverse_view[12], in.inverse_view[13], in.inverse_view[14]});
    const Vector3 forward{in.inverse_view[2], in.inverse_view[6], in.inverse_view[10]};
    vector(20, forward);
    vector(21, half_vector(in.sun_direction, forward, estimate));
    c[22][0] = 1.f / 1024;
    c[22][1] = -1.f / 1024;
    c[23][0] = c[23][1] = 1.f / 512;
    c[24][0] = static_cast<float>(in.dimensions[0]);
    c[24][1] = static_cast<float>(in.dimensions[1]);
    c[25][0] = in.control;
    c[26][0] = in.options[0] == 2 ? 1.f : 0.f;
    c[27][0] = in.options[3] != 0 ? 1.f : 0.f;
    c[28][0] = in.options[2] != 0 ? 1.f : 0.f;
    c[29][0] = 1;
    c[30][0] = in.options[1] != 0 ? 1.f : 0.f;
    matrix(31, in.shadow_projection);
    c[35][0] = static_cast<float>(in.screen[0]);
    c[35][1] = static_cast<float>(in.screen[1]);
    c[36][0] = in.texture_extent;
    result.fog = terrain_fog_constants({in.fog_color, {in.fog_density, in.fog_strength, 0, in.fog_distance}},
                                       in.inverse_view[14], precision);
    return result;
}
LandMaterialConstants land_material_frame(const TerrainFrameState &frame) {
    LandMaterialConstants result;
    const auto &c = frame.constants;
    std::copy_n(c.begin(), 4, result.view_projection.begin());
    std::copy_n(c.begin() + 4, 4, result.composite_projection.begin());
    std::copy_n(c.begin() + 31, 4, result.shadow_projection.begin());
    result.fow_scale = c[9];
    result.world_scales = c[16];
    result.height_texture_scale = c[22];
    result.camera = c[19];
    result.shadow_fade = c[25][0];
    result.apply_specular = c[27][0];
    result.apply_diffuse = c[28][0];
    result.fog_color = frame.fog[0];
    result.fog = frame.fog[1];
    result.validate();
    return result;
}
} // namespace civ5::materials
