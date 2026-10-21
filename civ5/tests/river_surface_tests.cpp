#include "civ5/terrain/river_surface.hpp"
#include <iostream>
#include <limits>
#include <stdexcept>
using namespace civ5::terrain;
namespace {
void check(bool ok, const char *message) {
    if (!ok)
        throw std::runtime_error(message);
}
template <class F> void rejects(F action) {
    try {
        action();
    } catch (const std::invalid_argument &) {
        return;
    }
    throw std::runtime_error("Invalid river surface input accepted");
}
} // namespace
int main() {
    try {
        RiverPolylines centers{{{{0, 0, 3}, {4, 0, 3}, {8, 0, 3}, {12, 0, 3}}, true}};
        std::array<RiverPolylines, 2> banks{centers, centers};
        for (auto &p : banks[0][0].points)
            p[1] = -3;
        for (auto &p : banks[1][0].points)
            p[1] = 3;
        RiverHeights heights{{14000, 14000, 14000, 14000}};
        RiverSurfaceSettings settings;
        const auto mesh = build_river_surface(centers, banks, heights, settings);
        check(!mesh.vertices.empty() && !mesh.indices.empty(), "Surface unexpectedly empty");
        for (auto i : mesh.indices)
            check(i < mesh.vertices.size(), "River index outside owned vertices");
        for (const auto &v : mesh.vertices)
            check(v[7] == 0, "Unspecified COLOR0 not initialized");
        check(build_river_surface({}, {}, {}, settings).vertices.empty(), "Empty river produced a mesh");
        const auto original = banks;
        // Native input Z is replaced by independently sampled river heights.
        banks[0][0].points[0][2] = std::numeric_limits<float>::quiet_NaN();
        check(build_river_surface(centers, banks, heights, settings).vertices == mesh.vertices,
              "Unused control Z leaked into the surface");
        banks = original;
        rejects([&] { build_river_surface({}, banks, heights, settings); });
        banks[0][0].points.pop_back();
        rejects([&] { build_river_surface(centers, banks, heights, settings); });
        banks = original;
        heights[0].pop_back();
        rejects([&] { build_river_surface(centers, banks, heights, settings); });
        heights[0].push_back(14000);
        banks[0][0].points[0][0] = std::numeric_limits<float>::infinity();
        rejects([&] { build_river_surface(centers, banks, heights, settings); });
        banks = original;
        heights[0][0] = std::numeric_limits<float>::quiet_NaN();
        rejects([&] { build_river_surface(centers, banks, heights, settings); });
        heights[0][0] = 14000;
        settings.offset[1] = std::numeric_limits<float>::infinity();
        rejects([&] { build_river_surface(centers, banks, heights, settings); });
        settings.offset[1] = 0;
        for (float scale :
             {0.f, -1.f, std::numeric_limits<float>::infinity(), std::numeric_limits<float>::max()}) {
            settings.height_scale = scale;
            rejects([&] { build_river_surface(centers, banks, heights, settings); });
        }
        settings.height_scale = .2f;
        settings.precision = static_cast<X87Precision>(17);
        rejects([&] { build_river_surface(centers, banks, heights, settings); });
        std::cout << "PASS river surface ownership, unused inputs, finite arithmetic and extent guards\n";
        return 0;
    } catch (const std::exception &e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
