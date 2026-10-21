#include "civ5/terrain/camera_frame.hpp"
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
    return std::all_of(values.begin(), values.end(), [](float f) { return std::isfinite(f); });
}
Matrix4 transpose(const Matrix4 &m) {
    Matrix4 result;
    for (unsigned row = 0; row < 4; ++row)
        for (unsigned column = 0; column < 4; ++column)
            result[row * 4 + column] = m[column * 4 + row];
    return result;
}
Matrix4 perspective(const PerspectiveCamera &camera, X87Precision pc) {
    // 96BA80/96BF90 use double SSE arithmetic here, independently of x87 PC.
    constexpr auto radians = std::bit_cast<double>(std::uint64_t{0x3f91df46a2529d39});
    const float angle = static_cast<float>(static_cast<double>(camera.field_of_view) * radians);
    const auto rounded = [pc](double f) { return round_x87(f, pc); };
    // Original D3DXMatrixPerspectiveFovRH stores half-angle, cosine and sine
    // to float32 before the x87 divisions. FSINCOS is independent of PC.
    const float half = static_cast<float>(rounded(static_cast<double>(angle) * .5));
    const float cosine = static_cast<float>(std::cos(static_cast<double>(half)));
    const float sine = static_cast<float>(std::sin(static_cast<double>(half)));
    const float y = static_cast<float>(rounded(static_cast<double>(cosine) / sine));
    const float x = static_cast<float>(rounded(static_cast<double>(y) / camera.aspect_ratio));
    const auto depth = rounded(static_cast<double>(camera.near_plane) - camera.far_plane);
    const float z = static_cast<float>(rounded(static_cast<double>(camera.far_plane) / depth));
    const float translation = static_cast<float>(rounded(static_cast<double>(z) * camera.near_plane));
    const Matrix4 result{x, 0, 0, 0, 0, y, 0, 0, 0, 0, z, -1, 0, 0, translation, 0};
    require(finite(result), "Camera projection overflow");
    return result;
}
ViewFrustum frustum(const Matrix4 &projection, const Matrix4 &inverse_transpose, X87Precision pc) {
    ViewFrustum result;
    for (unsigned i = 0; i < 4; ++i) {
        const auto row = i * 4;
        result[0][i] = projection[row + 3] - projection[row + 2];
        result[1][i] = projection[row + 2];
        result[2][i] = projection[row + 3] - projection[row];
        result[3][i] = projection[row + 3] + projection[row];
        result[4][i] = projection[row + 3] + projection[row + 1];
        result[5][i] = projection[row + 3] - projection[row + 1];
    }
    for (auto &plane : result) {
        // 8EE1A0: separate scalar SSE products/additions, SQRTSS then DIVSS.
        const float squared = (plane[0] * plane[0] + plane[1] * plane[1]) + plane[2] * plane[2];
        require(std::isfinite(squared) && squared > 0, "Degenerate camera projection plane");
        const float reciprocal_length = 1.f / std::sqrt(squared);
        for (auto &value : plane)
            value = value * reciprocal_length;
        plane = vector4_transform(plane, inverse_transpose, pc, Vec4Path::sse);
        require(finite(plane), "Camera world plane overflow");
    }
    return result;
}
float dot_corner(const Vector4 &plane, float x, float y) {
    return ((plane[0] * x + plane[1] * y) + plane[2] * 0.f) + plane[3];
}
} // namespace

std::array<Matrix4, 2> camera_view_projection(const PerspectiveCamera &camera, X87Precision precision,
                                            const ReciprocalEstimate &estimate) {
    require(precision == X87Precision::single || precision == X87Precision::double_precision,
            "Invalid camera precision");
    require(finite(std::array{camera.field_of_view, camera.aspect_ratio, camera.near_plane, camera.far_plane}) &&
                camera.field_of_view > 0 && camera.field_of_view < 180 && camera.aspect_ratio > 0 &&
                camera.near_plane > 0 && camera.far_plane > camera.near_plane,
            "Invalid perspective camera lens");
    return {inverse_sse(camera.world, estimate), perspective(camera, precision)};
}

CameraFrameState camera_frame(const WorldLayout &world, std::int32_t world_mode,
                              const PerspectiveCamera &camera, const CameraFrameState &prior,
                              X87Precision precision, const ReciprocalEstimate &estimate) {
    require(precision == X87Precision::single || precision == X87Precision::double_precision,
            "Invalid camera precision");
    require(bool(estimate), "Camera requires an RCPSS estimate provider");
    require(world_mode >= 0 && world_mode <= 2, "Invalid camera world mode");
    require(
        finite(std::array{camera.field_of_view, camera.aspect_ratio, camera.near_plane, camera.far_plane}) &&
            camera.field_of_view > 0 && camera.field_of_view < 180 && camera.aspect_ratio > 0 &&
            camera.near_plane > 0 && camera.far_plane > camera.near_plane,
        "Invalid perspective camera lens");
    require(finite(camera.world), "Nonfinite camera world transform");
    require(finite(world.bounds) && world.bounds[6] > 0 && world.bounds[7] > 0,
            "Invalid camera world bounds");
    auto result = prior;
    // Preserve both original base-camera calls and their inverse operations.
    result.views[0] = inverse_sse(camera.world, estimate);
    result.projection = perspective(camera, precision);
    const auto plane_projection = perspective(camera, precision);
    const auto inverse_transpose = transpose(inverse_sse(camera.world, estimate));
    result.frusta[0] = frustum(plane_projection, inverse_transpose, precision);
    result.view_count = 1;
    const auto append_view = [&](float translation) {
        auto shift = identity_matrix();
        shift[12] = translation;
        const auto i = result.view_count;
        result.views[i] = matrix_multiply(shift, result.views[0], precision, MatrixPath::sse);
        require(finite(result.views[i]), "Wrapped camera view overflow");
        result.frusta[i] = frustum(result.projection, transpose(result.views[i]), precision);
        ++result.view_count;
    };
    if (world_mode == 1) {
        if (dot_corner(result.frusta[0][3], world.bounds[0], world.bounds[4]) > 0)
            append_view(-world.bounds[6]);
        if (dot_corner(result.frusta[0][2], world.bounds[3], world.bounds[4]) > 0)
            append_view(world.bounds[6]);
    }
    for (unsigned i = 0; i < result.view_count; ++i) {
        result.inverse_views[i] = inverse_sse(result.views[i], estimate);
        for (unsigned k = 0; k < 3; ++k)
            result.relative_offsets[i][k] = result.views[i][12 + k] - result.views[0][12 + k];
        require(finite(result.relative_offsets[i]), "Camera relative offset overflow");
    }
    return result;
}
} // namespace civ5::terrain
