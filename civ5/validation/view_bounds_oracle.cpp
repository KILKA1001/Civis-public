#include "civ5/materials/water_reflection.hpp"
#include "civ5/terrain/view_bounds.hpp"
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <vector>
using namespace civ5::terrain;
using namespace civ5::materials;
namespace {
void require(bool ok, const char *why) {
    if (!ok)
        throw std::invalid_argument(why);
}
struct Reader {
    std::vector<std::uint8_t> bytes;
    std::size_t at = 8;
    std::uint32_t word() {
        require(at <= bytes.size() && bytes.size() - at >= 4, "Truncated view-bounds primitive");
        std::uint32_t result = 0;
        for (unsigned i = 0; i < 4; ++i)
            result |= std::uint32_t(bytes[at++]) << (8 * i);
        return result;
    }
    std::int32_t integer() { return std::bit_cast<std::int32_t>(word()); }
    void floats(auto &values) {
        for (auto &v : values)
            v = std::bit_cast<float>(word());
    }
    ViewBounds previous() {
        ViewBounds result;
        for (auto *group : {&result.clipped_low, &result.clipped_high, &result.raw_low, &result.raw_high})
            for (auto &v : *group)
                floats(v);
        for (auto *group : {&result.clipped_tile_low, &result.clipped_tile_high, &result.raw_tile_low,
                            &result.raw_tile_high})
            for (auto &point : *group)
                for (auto &value : point)
                    value = integer();
        return result;
    }
};
struct Writer {
    std::vector<std::uint8_t> bytes;
    void value(const auto &v) {
        const auto *p = reinterpret_cast<const std::uint8_t *>(&v);
        bytes.insert(bytes.end(), p, p + sizeof(v));
    }
    void grid(const WorldGrid &g) {
        value(g.first_matrix);
        value(g.second_matrix);
        value(g.descriptor);
        value(g.width);
        value(g.height);
    }
};
} // namespace
int main(int argc, char **argv) {
    try {
        require(argc == 3, "Usage: view_bounds_oracle primitive-input fresh-output");
        std::ifstream input(argv[1], std::ios::binary | std::ios::ate);
        require(bool(input), "Open view-bounds input");
        const auto size = input.tellg();
        require(size >= 12 && size <= 16 * 1024 * 1024, "Bounded view-bounds input");
        Reader r{std::vector<std::uint8_t>(static_cast<std::size_t>(size))};
        input.seekg(0);
        input.read(reinterpret_cast<char *>(r.bytes.data()), size);
        const bool reflection = std::memcmp(r.bytes.data(), "C5RFRM01", 8) == 0;
        require(bool(input) && (reflection || std::memcmp(r.bytes.data(), "C5VBND01", 8) == 0),
                "View-bounds input schema");
        const auto count = r.word();
        require(count > 0 && count <= 1024, "View-bounds case count");
        Writer output;
        output.value(count);
        for (unsigned i = 0; i < count; ++i) {
            const auto width = r.integer(), height = r.integer(), mode = r.integer();
            const auto pc = r.word(), views = r.word();
            require((pc == 24 || pc == 53) && views <= 4, "View-bounds precision/count");
            std::array<float, 2> scales;
            r.floats(scales);
            std::array<std::uint32_t, 4> estimates;
            for (auto &value : estimates)
                value = r.word();
            std::array<ViewFrustum, 4> frusta;
            for (auto &f : frusta)
                for (auto &p : f)
                    r.floats(p);
            const auto prior = r.previous();
            WaterReflectionFrameSettings settings{};
            WaterReflectionCamera camera{frusta, views, {}, {}};
            WaterReflectionFrameState previous{prior, {}, {}};
            std::array<std::uint32_t, 8> normalizations{};
            if (reflection) {
                settings.angles = {std::bit_cast<float>(r.word()), std::bit_cast<float>(r.word()),
                                   std::bit_cast<float>(r.word()), std::bit_cast<float>(r.word())};
                r.floats(settings.shadow_direction);
                settings.source_scales = scales;
                r.floats(camera.base_view_inverse);
                for (auto &m : camera.extra_views)
                    r.floats(m);
                for (auto *group : {&previous.matrices.shadow, &previous.matrices.reflection,
                                    &previous.matrices.reflection_view})
                    for (auto &m : *group)
                        r.floats(m);
                r.floats(previous.matrices.reflection_projection);
                for (auto &v : normalizations)
                    v = r.word();
            }
            unsigned cursor = 0;
            const auto world = build_world_layout(width, height, mode, {}, [&](float det) {
                require(cursor < 2 && std::bit_cast<std::uint32_t>(det) == estimates[2 * cursor],
                        "Original view world determinant mismatch");
                return std::bit_cast<float>(estimates[2 * cursor++ + 1]);
            });
            require(cursor == 2, "Complete view world inverse chain");
            output.value(world.transforms);
            output.value(world.bounds);
            output.grid(world.source_grid);
            output.grid(world.worker_grid);
            const auto precision = pc == 24 ? X87Precision::single : X87Precision::double_precision;
            if (reflection) {
                cursor = 0;
                const auto result =
                    water_reflection_frame(world, camera, settings, previous, precision, [&](float squared) {
                        require(cursor < 4 &&
                                    std::bit_cast<std::uint32_t>(squared) == normalizations[2 * cursor],
                                "Original connected reflection RSQRT input mismatch");
                        return std::bit_cast<float>(normalizations[2 * cursor++ + 1]);
                    });
                require(cursor == 4, "Complete connected reflection normalization chain");
                static_assert(sizeof(result) == 1176);
                output.value(result);
            } else {
                const auto result =
                    view_bounds(world, std::span(frusta).first(views), scales, prior, precision);
                static_assert(sizeof(result) == 320);
                output.value(result);
            }
        }
        require(r.at == r.bytes.size(), "Trailing view-bounds input");
        require(!std::filesystem::exists(argv[2]), "Fresh view-bounds output required");
        std::ofstream out(argv[2], std::ios::binary);
        out.write(reinterpret_cast<const char *>(output.bytes.data()), output.bytes.size());
        out.close();
        require(bool(out), "Complete view-bounds output");
        std::cout << "PASS " << count << " original world/view-bounds cases\n";
        return 0;
    } catch (const std::exception &e) {
        std::cerr << "FAIL " << e.what() << '\n';
        return 1;
    }
}
