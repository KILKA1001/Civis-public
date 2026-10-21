// Only primitive world/frame/settings/resource inputs cross this offline bridge.
#include "civ5/materials/water_surface.hpp"
#include <bit>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <xmmintrin.h>
namespace {
namespace fs = std::filesystem;
using namespace civ5::materials;
using namespace civ5::terrain;
using Bytes = std::vector<std::uint8_t>;
void check(bool ok, const char *message) {
    if (!ok)
        throw std::invalid_argument(message);
}
struct Reader {
    std::span<const std::uint8_t> bytes;
    std::size_t at = 0;
    auto take(std::size_t n) {
        check(n <= bytes.size() - at, "Truncated water surface input");
        const auto result = bytes.subspan(at, n);
        at += n;
        return result;
    }
    std::uint32_t word() {
        const auto b = take(4);
        return std::uint32_t(b[0]) | std::uint32_t(b[1]) << 8 | std::uint32_t(b[2]) << 16 |
               std::uint32_t(b[3]) << 24;
    }
    float scalar() { return std::bit_cast<float>(word()); }
    template <std::size_t N> auto floats() {
        std::array<float, N> out;
        for (auto &v : out)
            v = scalar();
        return out;
    }
    auto blob() { return take(word()); }
};
struct Writer {
    Bytes bytes;
    void word(std::uint32_t value) {
        for (unsigned i = 0; i < 4; ++i)
            bytes.push_back(std::uint8_t(value >> (i * 8)));
    }
    void raw(std::span<const std::uint8_t> values) {
        bytes.insert(bytes.end(), values.begin(), values.end());
    }
    void blob(std::span<const std::uint8_t> values) {
        word(unsigned(values.size()));
        raw(values);
    }
    void floats(std::span<const float> values) {
        for (auto v : values)
            word(std::bit_cast<std::uint32_t>(v));
    }
    void u16(std::span<const std::uint16_t> values) {
        for (auto v : values) {
            bytes.push_back(std::uint8_t(v));
            bytes.push_back(std::uint8_t(v >> 8));
        }
    }
};
} // namespace
#ifdef _WIN32
int wmain(int argc, wchar_t **argv) {
#else
int main(int argc, char **argv) {
#endif
    try {
        check(argc == 3, "Usage: water_surface_oracle input fresh-output");
        check(!fs::exists(argv[2]), "Fresh water surface output required");
        std::ifstream source(fs::path(argv[1]), std::ios::binary);
        check(bool(source), "Water input read failed");
        const Bytes input{std::istreambuf_iterator<char>(source), {}};
        Reader r{input};
        const auto magic = r.take(8);
        check(std::string(magic.begin(), magic.end()) == "C5WATR01", "Water input schema");
        const auto count = r.word();
        check(count > 0 && count <= 4096, "Water case count");
        Writer output;
        output.word(count);
        for (unsigned i = 0; i < count; ++i) {
            Reader q{r.blob()};
            const auto width = q.word(), height = q.word(), mode = q.word(), pc = q.word();
            check(pc == 24 || pc == 53, "Water precision");
            const auto precision = pc == 24 ? X87Precision::single : X87Precision::double_precision;
            const auto prior_capacity = q.word(), selector = q.word();
            const auto world = build_world_layout(int(width), int(height), int(mode), {}, [](float v) {
                return _mm_cvtss_f32(_mm_rcp_ss(_mm_set_ss(v)));
            });
            const auto domain = WaterSurfaceDomain::from_world(world);
            WaterSurfaceSettings settings;
            settings.reflection_amount = q.scalar();
            settings.specular_sun = q.floats<4>();
            settings.specular_sky = q.floats<4>();
            settings.sun_direction = q.floats<3>();
            settings.sky_direction = q.floats<3>();
            settings.fake_view_sun = q.floats<4>();
            settings.fake_view_sky = q.floats<4>();
            settings.ocean_height = q.scalar();
            settings.gradient_scale = q.scalar();
            settings.frequency = q.scalar();
            settings.river_phase = q.scalar();
            settings.water_phase = q.scalar();
            const WaterSurfaceFrame frame{q.floats<16>(), q.floats<4>(), q.floats<4>(), q.scalar()};
            WaterSurfaceResources resources;
            resources.vertex_buffer = q.word();
            resources.index_buffer = q.word();
            const auto grown_vb = q.word(), grown_ib = q.word();
            resources.lean_rgba = q.word();
            resources.lean_luminance = q.word();
            resources.normal_color = q.word();
            for (auto &v : resources.shared)
                v = q.word();
            for (auto &v : resources.height_windows)
                v = q.word();
            const auto view_count = q.word();
            check(view_count <= 4, "Water view count");
            std::vector<Vector4> bounds;
            std::vector<WaterSurfaceView> views;
            for (unsigned j = 0; j < view_count; ++j) {
                bounds.push_back(q.floats<4>());
                views.push_back({q.floats<16>(), q.floats<16>(), q.floats<16>(), q.floats<16>(),
                                 q.floats<3>(), q.floats<145>()});
            }
            check(q.at == q.bytes.size(), "Trailing water case input");
            const auto mesh = build_water_surface_mesh(domain, bounds, prior_capacity, precision);
            if (mesh.reallocate) {
                resources.vertex_buffer = grown_vb;
                resources.index_buffer = grown_ib;
            }
            const auto draws = water_surface_draws(mesh, resources, selector);
            Writer out, vertices, indices, counts, prefixes;
            out.word(mesh.capacity);
            out.word(mesh.reallocate);
            out.floats(world.bounds);
            for (const auto &v : mesh.vertices)
                vertices.floats(v);
            indices.u16(mesh.indices);
            counts.u16(mesh.counts);
            prefixes.u16(mesh.prefixes);
            out.blob(vertices.bytes);
            out.blob(indices.bytes);
            out.blob(counts.bytes);
            out.blob(prefixes.bytes);
            out.word(view_count);
            for (const auto &v : views)
                out.floats(water_surface_constants(domain, settings, frame, v, precision));
            out.word(unsigned(draws.size()));
            for (const auto &draw : draws) {
                out.word(draw.view);
                out.word(draw.window);
                out.word(draw.selector);
                out.word(draw.first_index);
                out.word(draw.triangle_count);
                out.raw(draw.packet);
            }
            output.blob(out.bytes);
        }
        check(r.at == r.bytes.size(), "Trailing water input");
        std::ofstream file(fs::path(argv[2]), std::ios::binary);
        file.write(reinterpret_cast<const char *>(output.bytes.data()), output.bytes.size());
        file.close();
        check(bool(file), "Water output write failed");
        std::cout << "Water surface cases: " << count << '\n';
    } catch (const std::exception &e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
