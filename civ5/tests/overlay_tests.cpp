#include "civ5/terrain/overlay.hpp"
#include <cstring>
#include <iostream>
#include <limits>
#include <stdexcept>
using namespace civ5::terrain;
namespace {
unsigned checks = 0;
void check(bool ok, const char *why) {
    ++checks;
    if (!ok)
        throw std::runtime_error(why);
}
void rejects(auto action) {
    bool rejected = false;
    try {
        action();
    } catch (const std::invalid_argument &) {
        rejected = true;
    }
    check(rejected, "Invalid overlay was accepted");
}
void run() {
    CameraFrameState camera{};
    camera.view_count = 3;
    camera.frusta[0] = {Vector4{},
                        Vector4{},
                        Vector4{-1, 0, 0, 64},
                        Vector4{1, 0, 0, 64},
                        Vector4{0, 1, 0, 64},
                        Vector4{0, -1, 0, 64}};
    camera.inverse_views[0] = camera.views[1] = camera.views[2] = identity_matrix();
    camera.views[1][12] = -512;
    camera.views[2][12] = 512;
    OverlayMatrices prior;
    for (auto &matrix : prior)
        matrix.fill(std::bit_cast<float>(0x7fc01234u));
    for (auto pc : {X87Precision::single, X87Precision::double_precision}) {
        const auto result = overlay_matrices(camera, prior, true, pc);
        check(result[0][0] == 1.f / 128 && result[0][5] == 1.f / 128 && result[0][10] == -.5f &&
                  result[0][14] == .5f,
              "Overlay 64-unit border or depth projection");
        check(result[1][12] == -4 && result[2][12] == 4, "Wrapped view displacement lost");
        check(std::memcmp(result[3].data(), prior[3].data(), 64) == 0, "Inactive overlay slot changed");
        auto perturbed = camera;
        // Keep both bounds within their current integer grid interval.
        perturbed.frusta[0][2][3] += 31;
        check(overlay_matrices(perturbed, prior, true, pc)[0] == result[0],
              "Overlay moved inside grid interval");
        perturbed.frusta[0][2][3] += 34;
        check(overlay_matrices(perturbed, prior, true, pc)[0] != result[0],
              "Overlay did not cross grid interval");
        perturbed = camera;
        perturbed.view_count = 0;
        const auto empty = overlay_matrices(perturbed, prior, true, pc);
        check(empty[0] == result[0] && std::memcmp(empty[1].data(), prior[1].data(), 192) == 0,
              "Zero-view overlay write scope");
        perturbed.view_count = 999;
        perturbed.frusta[0][2][0] = std::numeric_limits<float>::infinity();
        const auto disabled = overlay_matrices(perturbed, prior, false, pc);
        check(std::memcmp(disabled.data(), prior.data(), 256) == 0,
              "Disabled overlay touched entering storage");
        rejects([&] { overlay_matrices(perturbed, prior, true, pc); });
        perturbed = camera;
        perturbed.inverse_views[0][0] = std::numeric_limits<float>::quiet_NaN();
        rejects([&] { overlay_matrices(perturbed, prior, true, pc); });
        perturbed = camera;
        perturbed.views[2][12] = std::numeric_limits<float>::infinity();
        rejects([&] { overlay_matrices(perturbed, prior, true, pc); });
        perturbed = camera;
        perturbed.frusta[0][2][3] = 2147483648.f;
        rejects([&] { overlay_matrices(perturbed, prior, true, pc); });
        perturbed = camera;
        perturbed.frusta[0][2][0] = std::numeric_limits<float>::quiet_NaN();
        rejects([&] { overlay_matrices(perturbed, prior, true, pc); });
        perturbed = camera;
        perturbed.views[1][0] = perturbed.inverse_views[0][0] = 1e30f;
        rejects([&] { overlay_matrices(perturbed, prior, true, pc); });
    }
    rejects([&] { overlay_matrices(camera, prior, true, static_cast<X87Precision>(99)); });
}
} // namespace
int main() {
    try {
        run();
        std::cout << "PASS overlay " << checks << " checks\n";
    } catch (const std::exception &e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
