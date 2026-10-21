#include "civ5/materials/fog_of_war_frame.hpp"
#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace civ5::materials {
namespace {
using namespace terrain;
Matrix4 multiply(const Matrix4 &a, const Matrix4 &b) {
    return matrix_multiply(a, b, X87Precision::double_precision, MatrixPath::sse);
}
Matrix4 scale(float x, float y, float z) {
    auto result = identity_matrix();
    result[0] = x;
    result[5] = y;
    result[10] = z;
    return result;
}
Matrix4 translate(float x, float y, float z) {
    auto result = identity_matrix();
    result[12] = x;
    result[13] = y;
    result[14] = z;
    return result;
}
std::array<std::int32_t, 2> cell(std::int32_t x, std::int32_t y, const IntMatrix4 &m) {
    const auto a = wrap_add(x, wrap_neg(y >> 1)), c = wrap_neg(wrap_add(a, y));
    return {wrap_add(wrap_add(wrap_add(wrap_mul(a, m[0]), m[12]), wrap_mul(c, m[8])), wrap_mul(y, m[4])),
            wrap_add(wrap_add(wrap_add(wrap_mul(y, m[5]), m[13]), wrap_mul(c, m[9])), wrap_mul(a, m[1]))};
}
void store(std::vector<Vector4> &out, std::size_t index, const Matrix4 &m) {
    if (!std::all_of(m.begin(), m.end(), [](float value) { return std::isfinite(value); }))
        throw std::invalid_argument("FOW computed matrix overflow");
    for (std::size_t row = 0; row < 4; ++row)
        std::copy_n(m.begin() + row * 4, 4, out[index + row].begin());
}
void batches(FogOfWarFrame &frame, const std::vector<Matrix4> &matrices, std::uint32_t geometry,
             std::uint32_t selector) {
    for (std::size_t start = 0; start < matrices.size(); start += 24) {
        FogOfWarDraw draw{geometry, selector, std::vector<Vector4>(96)};
        for (std::size_t i = 0; i < 24 && start + i < matrices.size(); ++i)
            store(draw.constants, 4 * i, matrices[start + i]);
        frame.draws.push_back(std::move(draw));
    }
}
template <class Range> bool finite(const Range &range) {
    return std::all_of(range.begin(), range.end(), [](float x) { return std::isfinite(x); });
}
} // namespace
FogOfWarFrame fog_of_war_frame(const WorldLayout &world, const FogOfWarState &state,
                               const FogOfWarCamera &camera, std::span<const FogOfWarView> views,
                               std::uint32_t quality, bool enabled, bool clear_mask, bool override_enabled,
                               X87Precision precision) {
    if (views.size() > 4 || world.source_grid.width != state.width() ||
        world.source_grid.height != state.height() ||
        (precision != X87Precision::single && precision != X87Precision::double_precision) ||
        !finite(camera.height_center) || !finite(camera.height_scale) || !finite(camera.projection) ||
        !finite(world.transforms) || !finite(world.bounds))
        throw std::invalid_argument("FOW frame input domain");
    for (const auto &view : views) {
        if (!finite(view.view) || !finite(view.overlay) || !finite(view.bounds))
            throw std::invalid_argument("FOW finite view required");
        for (auto value : view.tiles)
            if (value < -32768 || value > 32767)
                throw std::invalid_argument("FOW tile interval domain");
        if (std::int64_t(std::max(0, view.tiles[1] - view.tiles[0] + 1)) *
                std::max(0, view.tiles[3] - view.tiles[2] + 2) >
            1048576)
            throw std::invalid_argument("FOW bounded tile traversal required");
    }
    Matrix4 world_matrix;
    std::copy_n(world.transforms.begin() + 16, 16, world_matrix.begin());
    const auto camera_matrix = multiply(multiply(identity_matrix(), scale(1, .866f, 1)),
                                        translate(-camera.height_center[0], -camera.height_center[1], 0));
    FogOfWarFrame result;
    result.passes.push_back(
        {FogOfWarPassKind::mask, 0, 0, clear_mask ? Vector4{1, 1, 1, 1} : Vector4{0, 0, 0, 0}});
    const auto inside = [&](int x, int y) {
        return x >= 0 && x < state.width() && y >= 0 && y < state.height();
    };
    const auto index = [&](int x, int y) { return std::size_t(x) * state.height() + y; };
    if (!clear_mask)
        for (unsigned phase = 0; phase < 3; ++phase) {
            std::vector<Matrix4> matrices;
            for (const auto &view : views)
                for (int x = view.tiles[0]; x < view.tiles[1]; ++x)
                    for (int y = view.tiles[2]; y < view.tiles[3]; ++y) {
                        auto visibility = inside(x, y) ? state.visibility()[index(x, y)] : 2u;
                        if (override_enabled && visibility != 2 && inside(x, y) &&
                            state.visibility_override()[index(x, y)] == 0)
                            visibility = 1;
                        if (visibility != std::array<unsigned, 3>{1, 0, 2}[phase] || matrices.size() >= 2048)
                            continue;
                        const auto p = cell(x, y, world.source_grid.second_matrix);
                        const float a = static_cast<float>(p[0]), b = static_cast<float>(p[1]), c = -a - b;
                        matrices.push_back(
                            multiply(multiply(multiply(translate(a, b, c), world_matrix), camera_matrix),
                                     view.overlay));
                    }
            batches(result, matrices, phase == 2 ? 1 : 0, std::array<unsigned, 3>{2, 1, 0}[phase]);
        }
    result.passes[0].end = static_cast<std::uint32_t>(result.draws.size());
    if (!enabled || fog_of_war_startup(quality).quality == 0)
        return result;
    result.passes.push_back(
        {FogOfWarPassKind::culling, static_cast<std::uint32_t>(result.draws.size()), 0, {0, 0, 0, 0}});
    const auto quad_scale = scale(32, static_cast<float>(round_x87(double(.866f) * 32, precision)), 1);
    const float shift_x = 32, shift_y = .866f * 32;
    for (const auto &view : views) {
        std::vector<Matrix4> matrices;
        for (int x = view.tiles[0]; x < view.tiles[1] + 1; ++x)
            for (int y = view.tiles[2]; y < view.tiles[3] + 2; ++y) {
                if (!inside(x, y) || state.culling()[index(x, y)] == 0 || matrices.size() >= 2048)
                    continue;
                const auto p = cell(x, y, world.source_grid.second_matrix);
                const float a = static_cast<float>(p[0]), b = static_cast<float>(p[1]),
                            c = static_cast<float>(wrap_neg(wrap_add(p[0], p[1])));
                auto project = [&](unsigned axis) {
                    float v = a * world_matrix[axis];
                    v = v + c * world_matrix[8 + axis];
                    v = v + b * world_matrix[4 + axis];
                    v = v + world_matrix[12 + axis];
                    v = camera.height_scale[axis] * v;
                    return v - camera.height_center[axis];
                };
                const auto location =
                    translate(shift_x + project(0), (shift_y + project(1)) - 128, .2f * 510);
                matrices.push_back(
                    multiply(multiply(multiply(quad_scale, location), view.view), camera.projection));
            }
        batches(result, matrices, 5, 3);
    }
    result.passes[1].end = static_cast<std::uint32_t>(result.draws.size());
    result.passes.push_back(
        {FogOfWarPassKind::clouds, static_cast<std::uint32_t>(result.draws.size()), 0, {0, 0, 0, 0}});
    const std::array<float, 3> heights{static_cast<float>(round_x87(double(.2f) * 56, precision)),
                                       .2f * 191.25f,
                                       static_cast<float>(round_x87(double(.2f) * 255, precision))};
    for (unsigned layer = 0; layer < 3; ++layer)
        for (const auto &view : views) {
            auto bound = [&](unsigned at) {
                const float difference = view.bounds[at] - world.bounds[at < 2 ? 0 : 1];
                const float value = difference / (at < 2 ? 1.f : .866f);
                const double sum = round_x87(double(value) + (value < 0 ? -.5 : .5), precision);
                const double rounded = value < 0 ? std::ceil(sum) : std::floor(sum);
                if (!std::isfinite(rounded) || rounded < -1048576 || rounded > 1048576)
                    throw std::invalid_argument("FOW cloud bound domain");
                return static_cast<int>(rounded) / 64;
            };
            const auto min_x = bound(0), max_x = bound(1) + 1, min_y = bound(2), max_y = bound(3) + 2;
            if (std::int64_t(std::max(0, max_x - min_x)) * std::max(0, max_y - min_y) > 1048576)
                throw std::invalid_argument("FOW bounded cloud traversal required");
            std::vector<Matrix4> worlds, matrices;
            for (int x = min_x; x < max_x; ++x)
                for (int y = min_y; y < max_y; ++y) {
                    if (matrices.size() >= 2048)
                        continue;
                    float tx = static_cast<float>(x * 64);
                    tx = tx + world.bounds[0];
                    tx = tx + shift_x;
                    float ty = static_cast<float>(y * 64) * .866f;
                    ty = ty + shift_y;
                    ty = ty + world.bounds[1];
                    const auto m = multiply(quad_scale, translate(tx, ty, heights[layer]));
                    worlds.push_back(m);
                    matrices.push_back(multiply(multiply(m, view.view), camera.projection));
                }
            for (std::size_t start = 0; start < matrices.size(); start += 24) {
                FogOfWarDraw draw{layer + 2, 0, std::vector<Vector4>(197)};
                for (std::size_t i = 0; i < 24 && start + i < matrices.size(); ++i) {
                    store(draw.constants, i * 4, matrices[start + i]);
                    store(draw.constants, 96 + i * 4, worlds[start + i]);
                }
                store(draw.constants, 192, view.overlay);
                draw.constants[196] = state.frame_scale();
                draw.constants[196][2] =
                    draw.constants[196][2] * fog_of_war_startup(quality).layer_strength[layer];
                draw.constants[196][3] =
                    draw.constants[196][3] * fog_of_war_startup(quality).layer_strength[layer];
                result.draws.push_back(std::move(draw));
            }
        }
    result.passes[2].end = static_cast<std::uint32_t>(result.draws.size());
    std::size_t constant_bytes = 0;
    for (const auto &draw : result.draws)
        constant_bytes += draw.constants.size() * sizeof(Vector4);
    if (constant_bytes > 1048576 || result.draws.size() > 8192)
        throw std::invalid_argument("FOW original command arena capacity exceeded");
    return result;
}
FogOfWarSceneFrame fog_of_war_scene_frame(const WorldLayout &world, const HeightOwnerParameters &height,
                                          const FogOfWarState &state, const CameraFrameState &camera,
                                          const ViewBounds &bounds, const OverlayMatrices &prior_overlays,
                                          std::uint32_t quality, bool enabled, bool clear_mask,
                                          bool override_enabled, X87Precision precision) {
    if (camera.view_count > 4)
        throw std::invalid_argument("FOW scene camera view capacity");
    auto overlays = overlay_matrices(camera, prior_overlays, true, precision);
    std::array<FogOfWarView, 4> views;
    for (unsigned i = 0; i < camera.view_count; ++i)
        views[i] = {camera.views[i],
                    overlays[i],
                    {bounds.clipped_tile_low[i][0], bounds.clipped_tile_high[i][0], bounds.raw_tile_low[i][1],
                     bounds.raw_tile_high[i][1]},
                    {bounds.clipped_low[i][0], bounds.clipped_high[i][0], bounds.raw_low[i][1],
                     bounds.raw_high[i][1]}};
    const FogOfWarCamera input{
        {height.center_x, height.center_y}, {height.scale_x, height.scale_y}, camera.projection};
    auto commands = fog_of_war_frame(world, state, input, std::span(views).first(camera.view_count), quality,
                                     enabled, clear_mask, override_enabled, precision);
    return {std::move(overlays), std::move(commands)};
}
} // namespace civ5::materials
