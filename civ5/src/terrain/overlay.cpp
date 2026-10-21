#include "civ5/terrain/overlay.hpp"
#include <algorithm>
#include <cmath>
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
float snap(float value) {
    require(std::isfinite(value) && double(value) >= -2147483648. && double(value) < 2147483648.,
            "Overlay bound exceeds original integer domain");
    const auto bits = std::bit_cast<std::uint32_t>(static_cast<std::int32_t>(value)) & 0xffffffc0u;
    return static_cast<float>(std::bit_cast<std::int32_t>(bits));
}
} // namespace
OverlayMatrices overlay_matrices(const CameraFrameState &camera, const OverlayMatrices &prior, bool enabled,
                                 X87Precision precision) {
    require(precision == X87Precision::single || precision == X87Precision::double_precision,
            "Invalid overlay precision");
    if (!enabled)
        return prior;
    require(camera.view_count <= 4, "At most four overlay views are supported");
    if (camera.view_count > 1)
        require(finite(camera.inverse_views[0]), "Nonfinite overlay base inverse");
    for (unsigned i = 1; i < camera.view_count; ++i)
        require(finite(camera.views[i]), "Nonfinite wrapped overlay view");
    const auto [low, high] = frustum_ground_bounds(camera.frusta[0]);
    const float left = snap(low[0] - 64.f), right = snap(high[0] + 64.f);
    const float bottom = snap(low[1] - 64.f), top = snap(high[1] + 64.f);
    require(left < right && bottom < top, "Degenerate overlay projection extent");
    const auto q = [precision](double v) { return round_x87(v, precision); };
    // Original D3DX stores each reciprocal as float before reloading it for
    // the remaining x87 products. Near=-1 and far=1 are original constants.
    const float rx = static_cast<float>(q(1. / q(double(right) - left)));
    const float ry = static_cast<float>(q(1. / q(double(top) - bottom)));
    auto result = prior;
    result[0] = {static_cast<float>(q(double(rx) * 2)),
                 0,
                 0,
                 0,
                 0,
                 static_cast<float>(q(double(ry) * 2)),
                 0,
                 0,
                 0,
                 0,
                 -.5f,
                 0,
                 static_cast<float>(-q(q(double(left) + right) * rx)),
                 static_cast<float>(-q(q(double(bottom) + top) * ry)),
                 .5f,
                 1};
    for (unsigned i = 1; i < camera.view_count; ++i) {
        const auto relative =
            matrix_multiply(camera.views[i], camera.inverse_views[0], precision, MatrixPath::sse);
        result[i] = matrix_multiply(relative, result[0], precision, MatrixPath::sse);
        require(finite(result[i]), "Wrapped overlay matrix overflow");
    }
    require(finite(result[0]), "Overlay projection overflow");
    return result;
}
} // namespace civ5::terrain
