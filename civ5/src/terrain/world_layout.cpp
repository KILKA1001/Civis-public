#include "civ5/terrain/world_layout.hpp"
#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>

namespace civ5::terrain {
namespace {
Matrix4 multiply(const Matrix4 &a, const Matrix4 &b) {
    return matrix_multiply(a, b, X87Precision::double_precision, MatrixPath::sse);
}
Matrix4 scale(float x, float y, float z) {
    auto m = identity_matrix();
    m[0] = x;
    m[5] = y;
    m[10] = z;
    return m;
}
Matrix4 translation(float x, float y, float z) {
    auto m = identity_matrix();
    m[12] = x;
    m[13] = y;
    m[14] = z;
    return m;
}
} // namespace
WorldGridLayout build_world_grid_layout(std::int32_t width, std::int32_t height, std::int32_t mode,
                                        const FactoryDescriptor &factory) {
    if (width < 1 || height < 1 || width > INT32_MAX - 4 || height > INT32_MAX - 4 || mode < 0 || mode > 2)
        throw std::invalid_argument("Unsupported native world dimensions/mode");
    const auto base_grid = unshifted_global_map_matrix(factory);
    const auto worker_width = width + (mode == 0 ? 4 : mode == 1 ? 0 : 2);
    const auto worker_height = height + (mode == 2 ? 2 : 4);
    const std::array<std::int32_t, 3> offset = mode == 0   ? std::array<std::int32_t, 3>{-1, -2, 3}
                                               : mode == 1 ? std::array<std::int32_t, 3>{1, -2, 1}
                                                           : std::array<std::int32_t, 3>{0, -1, 1};
    auto grid = [&](int w, int h, std::array<std::int32_t, 3> t) {
        auto first = base_grid, second = base_grid;
        for (int i = 0; i < 3; ++i) {
            first[12 + i] = wrap_neg(t[i]);
            second[12 + i] = t[i];
        }
        return WorldGrid{w, h, {t[0], t[1], t[2], 0, 0}, first, second};
    };
    return {grid(width, height, {0, 0, 0}), grid(worker_width, worker_height, offset)};
}
WorldLayout build_world_layout(std::int32_t width, std::int32_t height, std::int32_t mode,
                               const FactoryDescriptor &factory, const ReciprocalEstimate &estimate) {
    const auto grids = build_world_grid_layout(width, height, mode, factory);
    const auto worker_width = grids.worker_grid.width, worker_height = grids.worker_grid.height;
    // The native extent expression is signed 32-bit. A 128-cell restriction
    // was only a former fixture limit; prevent actual arithmetic overflow.
    if (worker_width > std::numeric_limits<std::int32_t>::max() / 64 ||
        worker_height > std::numeric_limits<std::int32_t>::max() / 64)
        throw std::invalid_argument("Native world extent overflow");
    // Original float/double literals; no pi-derived replacements or fitted matrices.
    const float angle = 0.5235987901687622f;
    const float cosine = static_cast<float>(std::cos(static_cast<double>(angle)));
    const float sine = static_cast<float>(std::sin(static_cast<double>(angle)));
    auto rotation = identity_matrix();
    rotation[0] = cosine;
    rotation[1] = sine;
    rotation[4] = -sine;
    rotation[5] = cosine;
    auto shear = identity_matrix();
    shear[1] = -static_cast<float>(std::cos(1.0471975511965976));
    const auto base = multiply(multiply(multiply(multiply(identity_matrix(), scale(1, 0.866f, 1)), rotation),
                                        scale(0.018042724579572678f, 0.015625f, 1)),
                               shear);
    const auto shift = mode == 0   ? translation(-1, -2, 3)
                       : mode == 1 ? translation(.5f, -2, 1.5f)
                                   : translation(.5f, 0, -.5f);
    const auto world = multiply(base, shift), canonical = multiply(base, translation(.5f, 0, -.5f));
    const auto flatten = scale(1, 1, 0);
    auto plane = flatten;
    plane[2] = -1;
    plane[6] = -1;
    const auto world_inverse = multiply(inverse_sse(world, estimate), flatten);
    const auto canonical_inverse = multiply(inverse_sse(canonical, estimate), flatten);
    const auto world_projection = multiply(world, plane), canonical_projection = multiply(canonical, plane);
    std::array<float, 80> transforms;
    const std::array<Matrix4, 4> matrices{world_projection, world_inverse, canonical_projection,
                                          canonical_inverse};
    for (int i = 0; i < 4; ++i)
        std::copy(matrices[i].begin(), matrices[i].end(), transforms.begin() + 16 * i);
    const Vector4 inputs{0, 128, 0, 512};
    for (int i = 0; i < 4; ++i) {
        const auto v = vector4_transform({inputs[i], inputs[i], 0, 1}, canonical_projection,
                                         X87Precision::double_precision, Vec4Path::sse);
        std::copy(v.begin(), v.end(), transforms.begin() + 64 + i * 4);
    }
    const float span_x = static_cast<float>(worker_width * 64 - (mode == 2 ? 65 : 1));
    const float span_y = static_cast<float>(worker_height * 64 - 1) * 0.866f;
    return {grids.source_grid,
            grids.worker_grid,
            transforms,
            {span_x * -.5f, span_y * -.5f, 0, span_x * .5f, span_y * .5f, 255, span_x, span_y, 0}};
}
} // namespace civ5::terrain
