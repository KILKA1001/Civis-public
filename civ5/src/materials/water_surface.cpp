#include "civ5/materials/water_surface.hpp"
#include "civ5/terrain/height_owner.hpp"
#include "civ5/terrain/terrain_world.hpp"
#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>

namespace civ5::materials {
namespace {
using namespace terrain;
void require(bool ok, const char *message) {
    if (!ok)
        throw std::invalid_argument(message);
}
void validate(const WaterSurfaceDomain &d, X87Precision pc) {
    require(pc == X87Precision::single || pc == X87Precision::double_precision,
            "Invalid water arithmetic precision");
    require(d.field_width > 0 && d.field_height > 0 && d.field_width <= 14336 && d.field_height <= 14336,
            "Water field exceeds the original 16 by 16 window table");
    for (auto v : {d.origin_x, d.origin_y, d.span_x, d.span_y, d.scales[0], d.scales[1], d.scales[2]})
        require(std::isfinite(v), "Nonfinite water domain");
    require(d.span_x > 0 && d.span_y > 0 && d.scales[0] > 0 && d.scales[1] > 0 && d.scales[2] > 0,
            "Invalid water domain scale");
}
int rounded_pixel(float coordinate, float origin, float scale, X87Precision pc) {
    const float relative = (coordinate - origin) / scale;
    const double shifted = round_x87(double(relative) + (relative < 0 ? -.5 : .5), pc);
    const float rounded = float(relative < 0 ? std::ceil(shifted) : std::floor(shifted));
    require(std::isfinite(rounded) && double(rounded) >= std::numeric_limits<int>::min() &&
                double(rounded) <= std::numeric_limits<int>::max(),
            "Unrepresentable water view coordinate");
    return int(rounded);
}
struct Range {
    int x0, y0, x1, y1;
};
Matrix4 multiply(const Matrix4 &a, const Matrix4 &b) {
    return matrix_multiply(a, b, X87Precision::double_precision, MatrixPath::sse);
}
template <std::size_t N>
void copy(WaterConstantBlock &block, std::size_t offset, const std::array<float, N> &v) {
    std::copy(v.begin(), v.end(), block.begin() + offset / 4);
}
float fog_coefficient(float falloff, float amount, float camera_z, X87Precision pc) {
    // Original FLDL2E/FMUL, integer split, F2XM1/FADD1/FSCALE, then -1/falloff.
    const float exponent = -(falloff * camera_z);
    const double power = round_x87(double(exponent) * 1.4426950408889634074, pc);
    const double integer = std::nearbyint(power);
    const double fraction = round_x87(power - integer, pc);
    const double mantissa = round_x87(std::expm1(fraction * 0.6931471805599453094) + 1., pc);
    require(integer >= -1022 && integer <= 1023, "Unrepresentable water fog exponent");
    const double exponential = std::ldexp(mantissa, int(integer));
    return float(round_x87(round_x87(round_x87(-1. / falloff, pc) * exponential, pc) * amount, pc));
}
} // namespace
WaterSurfaceDomain WaterSurfaceDomain::from_world(const terrain::WorldLayout &world) {
    const auto p = terrain::HeightOwnerParameters::from_world(world);
    return {p.width,
            p.height,
            world.bounds[0],
            world.bounds[1],
            world.bounds[6],
            world.bounds[7],
            {p.scale_x, p.scale_y, p.scale_z}};
}
WaterSurfaceDomain WaterSurfaceDomain::from_world(const terrain::TerrainWorld &world) {
    auto result = from_world(world.source().layout);
    require(result.field_width == world.owner().height.domain().width() &&
                result.field_height == world.owner().height.domain().height(),
            "Water domain differs from the world's retained height field");
    return result;
}
WaterSurfaceMesh build_water_surface_mesh(const WaterSurfaceDomain &d,
                                          std::span<const terrain::Vector4> views,
                                          std::uint32_t prior_capacity, terrain::X87Precision pc) {
    validate(d, pc);
    require(views.size() <= 4 && prior_capacity <= 0x3ffffff, "Invalid water view count/buffer capacity");
    WaterSurfaceMesh result{{},
                            {},
                            std::vector<std::uint16_t>(views.size() * 256),
                            std::vector<std::uint16_t>(views.size() * 256),
                            prior_capacity,
                            false};
    std::vector<Range> ranges;
    std::uint32_t quads = 0;
    for (std::size_t view = 0; view < views.size(); ++view) {
        const auto &v = views[view];
        Range r{std::max(0, rounded_pixel(v[0], d.origin_x, d.scales[0], pc) / 64),
                std::max(0, rounded_pixel(v[1], d.origin_y, d.scales[1], pc) / 64),
                std::min(d.field_width - 1, rounded_pixel(v[2], d.origin_x, d.scales[0], pc) / 64 + 1),
                std::min(d.field_height - 1, rounded_pixel(v[3], d.origin_y, d.scales[1], pc) / 64 + 1)};
        ranges.push_back(r);
        if (r.x0 >= r.x1 || r.y0 >= r.y1)
            continue;
        // The original clamp uses full field dimensions, not dimensions / 64.
        // Reject frames that would address outside its fixed table or wrap its
        // uint16 quad prefixes. Vertex indices separately wrap exactly below.
        require(r.x1 <= 224 && r.y1 <= 224, "Water view exceeds native height-window addressing");
        require(std::uint64_t(quads) + std::uint64_t(r.x1 - r.x0) * (r.y1 - r.y0) <= 65535,
                "Water view would wrap original uint16 quad prefixes");
        for (int x = r.x0; x < r.x1; ++x)
            for (int y = r.y0; y < r.y1; ++y) {
                ++result.counts[view * 256 + (y / 14) * 16 + x / 14];
                ++quads;
            }
    }
    std::uint32_t prefix = 0;
    for (std::size_t i = 0; i < result.counts.size(); ++i) {
        result.prefixes[i] = std::uint16_t(prefix);
        prefix += result.counts[i];
    }
    if (quads > prior_capacity) {
        result.capacity = std::uint32_t(round_x87(double(quads) * double(1.2f), pc));
        result.reallocate = true;
    }
    result.vertices.resize(quads * 4);
    result.indices.resize(quads * 6);
    const float ratio_x = float(round_x87(double(d.field_width) / d.span_x, pc));
    const float ratio_y = float(round_x87(double(d.field_height) / d.span_y, pc));
    const float half_x = float(round_x87(double(d.scales[0]) * 32., pc));
    const float half_y = float(round_x87(double(d.scales[1]) * 32., pc));
    std::vector<std::uint16_t> used(result.counts.size());
    for (std::size_t view = 0; view < views.size(); ++view) {
        const auto r = ranges[view];
        for (int x = r.x0; x < r.x1; ++x)
            for (int y = r.y0; y < r.y1; ++y) {
                const auto slot = view * 256 + (y / 14) * 16 + x / 14;
                const auto ordinal = result.prefixes[slot] + used[slot]++;
                const float cx = ((float(x * 64) * d.scales[0]) + 32.f * d.scales[0]) + d.origin_x;
                const float cy = ((float(y * 64) * d.scales[1]) + 32.f * d.scales[1]) + d.origin_y;
                const float x0 = cx - half_x, x1 = half_x + cx, y0 = cy - half_y, y1 = half_y + cy;
                const float ox = float(64 - (x / 14) * 896), oy = float(64 - (y / 14) * 896);
                const float u0 = (((x0 - d.origin_x) * ratio_x) + ox) * .0009765625f;
                const float u1 = (((x1 - d.origin_x) * ratio_x) + ox) * .0009765625f;
                const float v0 = (((y0 - d.origin_y) * ratio_y) + oy) * -.0009765625f;
                const float v1 = (((y1 - d.origin_y) * ratio_y) + oy) * -.0009765625f;
                const std::array<Vector4, 4> vertices{
                    {{x0, y1, u0, v1}, {x1, y1, u1, v1}, {x1, y0, u1, v0}, {x0, y0, u0, v0}}};
                std::copy(vertices.begin(), vertices.end(), result.vertices.begin() + ordinal * 4);
                constexpr std::array<unsigned, 6> offsets{0, 1, 2, 0, 2, 3};
                for (std::size_t i = 0; i < 6; ++i)
                    result.indices[ordinal * 6 + i] = std::uint16_t(ordinal * 4 + offsets[i]);
            }
    }
    return result;
}
WaterConstantBlock water_surface_constants(const WaterSurfaceDomain &d, const WaterSurfaceSettings &s,
                                           const WaterSurfaceFrame &f, const WaterSurfaceView &v,
                                           X87Precision pc) {
    validate(d, pc);
    require(std::isfinite(f.fog_parameters[0]) && f.fog_parameters[0] != 0 &&
                std::isfinite(f.fog_parameters[1]) && std::isfinite(v.camera[2]),
            "Invalid water fog input");
    WaterConstantBlock result = v.prior_constants;
    copy(result, 0, multiply(v.view, f.projection));
    copy(result, 0x40, v.overlay);
    const float phase = float(round_x87(std::sqrt(2.), pc)) * s.water_phase;
    copy(result, 0x80,
         Vector4{phase * .5555555820465088f, phase * .6666666865348816f, phase * .1388888955116272f,
                 phase * -.8333333134651184f});
    const float frequency_count = s.frequency * d.span_x;
    require(std::isfinite(frequency_count) && double(frequency_count) >= std::numeric_limits<int>::min() &&
                double(frequency_count) <= std::numeric_limits<int>::max(),
            "Invalid water wave frequency");
    const float count = float(int(frequency_count)), reciprocal = 1.f / d.span_x;
    const Vector4 frequency{reciprocal * count, (count + 1.f) * reciprocal, (count + 2.f) * reciprocal,
                            (count + 3.f) * reciprocal};
    copy(result, 0x90, frequency);
    copy(result, 0xa0, Vector4{1.7f, 1.2f, 1.7f, 1.2f});
    result[0xb0 / 4] = s.river_phase * frequency[0];
    result[0xc0 / 4] = (s.ocean_height * d.scales[2]) * 255.f;
    result[0xd0 / 4] = s.gradient_scale;
    copy(result, 0xe0, s.sun_direction);
    copy(result, 0xf0, s.sky_direction);
    copy(result, 0x100, s.specular_sun);
    copy(result, 0x110, s.specular_sky);
    copy(result, 0x120, s.fake_view_sun);
    copy(result, 0x130, s.fake_view_sky);
    copy(result, 0x140,
         Vector4{-f.fog_parameters[0],
                 fog_coefficient(f.fog_parameters[0], f.fog_parameters[1], v.camera[2], pc), 0,
                 f.fog_parameters[3]});
    copy(result, 0x150, v.camera);
    copy(result, 0x160, v.reflection);
    copy(result, 0x1a0, v.shadow);
    auto translation = identity_matrix(), scale = identity_matrix();
    translation[12] = -d.origin_x;
    translation[13] = -d.origin_y;
    scale[0] = float(round_x87(1. / d.span_x, pc));
    scale[5] = float(round_x87(-1. / d.span_y, pc));
    scale[10] = 255.f / (d.scales[2] * 65535.f);
    copy(result, 0x1e0, multiply(translation, scale));
    result[0x220 / 4] = f.shadow_fade;
    copy(result, 0x230, f.fog_color);
    result[0x240 / 4] = s.reflection_amount;
    return result;
}
WaterShaderRegisters water_shader_registers(const WaterConstantBlock &constants, const Matrix4 &world) {
    WaterShaderRegisters result;
    std::copy(world.begin(), world.end(), result.vertex.begin());
    std::copy_n(constants.begin(), 52, result.vertex.begin() + 16);
    std::copy(constants.begin() + 52, constants.end(), result.pixel.begin() + 16);
    return result;
}
std::vector<WaterSurfaceDraw> water_surface_draws(const WaterSurfaceMesh &m, const WaterSurfaceResources &r,
                                                  unsigned selector) {
    require(selector == 0 || selector == 2 || selector == 3 || selector == 5,
            "Water surface requires a surface shader selector");
    require(m.counts.size() <= 1024 && m.counts.size() % 256 == 0 && m.prefixes.size() == m.counts.size(),
            "Invalid water mesh groups");
    require(m.vertices.size() % 4 == 0 && m.vertices.size() <= 65535 * 4 &&
                m.indices.size() == m.vertices.size() / 4 * 6,
            "Invalid water mesh streams");
    std::vector<WaterSurfaceDraw> draws;
    std::uint32_t prefix = 0;
    for (std::size_t slot = 0; slot < m.counts.size(); ++slot) {
        require(m.prefixes[slot] == prefix, "Invalid water mesh prefix");
        const auto count = m.counts[slot];
        prefix += count;
        if (!count)
            continue;
        WaterSurfaceDraw draw{unsigned(slot / 256),           unsigned(slot % 256), selector,
                              unsigned(m.prefixes[slot]) * 6, unsigned(count) * 2,  {}};
        auto put = [&](unsigned at, std::uint32_t value, unsigned bytes) {
            for (unsigned i = 0; i < bytes; ++i)
                draw.packet[at + i] = std::uint8_t(value >> (i * 8));
        };
        put(4, r.index_buffer, 4);
        put(8, r.vertex_buffer, 4);
        put(12, draw.triangle_count, 4);
        put(16, draw.first_index, 4);
        put(0x60, 14, 1);
        put(0x18, r.lean_rgba, 2);
        put(0x1a, r.lean_luminance, 2);
        put(0x20, r.shared[1], 2);
        put(0x22, r.shared[0], 2);
        put(0x24, r.normal_color, 2);
        put(0x44, 3, 2);
        put(0x26, r.height_windows[draw.window], 2);
        put(0x28, r.shared[2], 2);
        put(0x48, 5, 2);
        put(0x2a, r.shared[3], 2);
        put(0x2c, r.shared[4], 2);
        put(0x2e, r.shared[5], 2);
        put(0x4e, 5, 2);
        // Original final sampler state writes 0x4e twice, leaving 0x52 at zero.
        put(0x32, r.shared[6], 2);
        draws.push_back(draw);
    }
    require(prefix * 4 == m.vertices.size(), "Water groups do not cover mesh vertices");
    return draws;
}
} // namespace civ5::materials
