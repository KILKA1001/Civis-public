#include "civ5/graphics/gl_render_api.hpp"
#include "civ5/graphics/gl_texture.hpp"
using civ5::graphics::DdsSamplingColor;
using civ5::graphics::GlContext;
using civ5::graphics::GlRenderApi;
using civ5::graphics::GlTexture;
using civ5::graphics::RenderTextureFormat;
#include "gl_minimap_surface_diagnostic.hpp"
#include <array>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
std::vector<std::uint8_t> read(const std::filesystem::path &path) {
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file) throw std::runtime_error("Minimap DDS is missing");
    const auto length = file.tellg();
    if (length <= 0 || length > 1024 * 1024)
        throw std::runtime_error("Minimap DDS extent is invalid");
    std::vector<std::uint8_t> data(static_cast<std::size_t>(length));
    file.seekg(0);
    file.read(reinterpret_cast<char *>(data.data()), length);
    if (!file) throw std::runtime_error("Minimap DDS is truncated");
    return data;
}
} // namespace

int main(int argc, char **argv) {
    try {
        const bool water_only = argc == 11 && std::string(argv[10]) == "--water-only";
        const bool city_marker = argc == 14 && std::string(argv[10]) == "--city-marker";
        if (argc != 10 && !water_only && !city_marker)
            throw std::invalid_argument("Usage: civ5_gl_minimap_mountain input output.ppm minX minY maxX maxY centerX centerY radius [--water-only | --city-marker ARGB fadeMin fadeMax]");
        const std::filesystem::path input(argv[1]), output(argv[2]);
        const civ5::ui::MinimapWorldBounds bounds{
            std::stof(argv[3]), std::stof(argv[4]),
            std::stof(argv[5]), std::stof(argv[6])};
        const float center_x = std::stof(argv[7]), center_y = std::stof(argv[8]);
        const float radius = std::stof(argv[9]);
        if (!(std::isfinite(bounds.min_x) && std::isfinite(bounds.min_y) &&
              std::isfinite(bounds.max_x) && std::isfinite(bounds.max_y) &&
              std::isfinite(center_x) && std::isfinite(center_y) &&
              std::isfinite(radius) && bounds.max_x > bounds.min_x &&
              bounds.max_y > bounds.min_y && radius > 0))
            throw std::invalid_argument("Invalid mountain probe bounds or radius");
        const auto color_word = city_marker ? std::stoull(argv[11], nullptr, 0) : 0xFFFFFFFFull;
        if (color_word > 0xFFFFFFFFull)
            throw std::invalid_argument("Invalid kind-5 vertex color");
        const float fade_min = city_marker ? std::stof(argv[12]) : 0.0f;
        const float fade_max = city_marker ? std::stof(argv[13]) : 1.0f;
        if (city_marker && !(std::isfinite(fade_min) && std::isfinite(fade_max) &&
                             fade_max > fade_min))
            throw std::invalid_argument("Invalid kind-5 fade constants");
        const std::array<const char *, 6> names{
            "minimapwater.dds", "minimapshallowwater.dds", "minimapgrass.dds",
            "minimapfog.dds", "minimapmountain.dds", "minimapice.dds"};
        std::array<civ5::assets::DdsData, 6> textures{
            civ5::assets::DdsData::parse(read(input / names[0])),
            civ5::assets::DdsData::parse(read(input / names[1])),
            civ5::assets::DdsData::parse(read(input / names[2])),
            civ5::assets::DdsData::parse(read(input / names[3])),
            civ5::assets::DdsData::parse(read(input / names[4])),
            civ5::assets::DdsData::parse(read(input / names[5]))};
        auto context = std::make_shared<civ5::graphics::GlContext>();
        GlMinimapSurfaceDiagnostic surface(context, textures);
        civ5::ui::MinimapRenderMeshes meshes{};
        meshes.visible.segments[0] = bounds;
        meshes.visible.texture = {3, 313, 313.0f / 190.0f};
        civ5::ui::MinimapMountainMarkers mountains{};
        mountains.radius = radius;
        if (!water_only && !city_marker) {
            mountains.source.push_back({0, 0, {center_x, center_y}});
            mountains.visible_indices[0].push_back(0);
        }
        std::vector<GlMinimapKind5Marker> city_markers;
        if (city_marker)
            city_markers.push_back({center_x, center_y, radius,
                                    static_cast<std::uint32_t>(color_word),
                                    fade_min, fade_max, 0});
        surface.render(meshes, {}, mountains, {}, bounds.max_x - bounds.min_x,
                       nullptr, {}, {}, {}, {}, {}, {}, 0.5f, {}, {}, {}, city_markers);
        surface.save_ppm(output);
        auto report_path = output;
        report_path.replace_extension(".json");
        std::ofstream report(report_path);
        report << std::setprecision(9)
               << "{\"schema\":\"civ5-minimap-gl-"
               << (city_marker ? "kind5" : "mountain") << "-control-v1\",\"bounds\":["
               << bounds.min_x << ',' << bounds.min_y << ',' << bounds.max_x << ','
               << bounds.max_y << "],\"center\":[" << center_x << ',' << center_y
               << "],\"radius\":" << radius << ",\"waterOnly\":"
               << (water_only ? "true" : "false")
               << ",\"surfacePixelShift\":0.5,\"mountainShaderPortExecuted\":"
               << (!city_marker ? "true" : "false")
               << ",\"kind5ShaderPortExecuted\":" << (city_marker ? "true" : "false")
               << ",\"markerColorArgb\":" << color_word
               << ",\"cityFade\":[" << fade_min << ',' << fade_max << "]}\n";
        if (!report) throw std::runtime_error("GL mountain control report write failed");
        context->check("GL mountain sprite control");
        std::cout << "GL kind-" << (city_marker ? 5 : 6) << " marker control: "
                  << (water_only ? 0 : 1)
                  << " sprite, 320x190\n";
        return 0;
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
