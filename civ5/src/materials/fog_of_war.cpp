#include "civ5/materials/fog_of_war.hpp"
#include <algorithm>
#include <bit>
#include <cmath>
#include <stdexcept>

namespace civ5::materials {
FogOfWarStartup fog_of_war_startup(std::uint32_t requested_quality) {
    const auto quality = std::min(requested_quality, 2u);
    const float scale = std::bit_cast<float>(0x3b03126fu);
    return {quality, quality == 2 ? 1024u : 512u, {scale, scale, scale, scale}, {1, 1.5f, 2}};
}
FogOfWarGeometry fog_of_war_geometry() {
    FogOfWarGeometry result;
    // Original B8C118 table contains stored float thirds, not double fractions.
    constexpr float one = std::bit_cast<float>(0x3eaaaaabu);
    constexpr float two = std::bit_cast<float>(0x3f2aaaabu);
    constexpr std::array<std::array<float, 3>, 6> hex{{{two, -one, -one},
                                                       {one, one, -two},
                                                       {-one, two, -one},
                                                       {-two, one, one},
                                                       {-one, -one, two},
                                                       {one, -two, one}}};
    constexpr std::array<std::uint32_t, 48> indices{5, 1, 0,  5, 4,  1,  1, 4, 2,  4, 3, 2,  1, 7, 0,  7,
                                                    6, 0, 8,  7, 1,  2,  8, 1, 9,  8, 2, 3,  9, 2, 10, 9,
                                                    3, 4, 10, 3, 11, 10, 4, 5, 11, 4, 6, 11, 5, 0, 6,  5};
    for (unsigned group = 0; group < 2; ++group) {
        const float radius = std::bit_cast<float>(group == 0 ? 0x3faa3d71u : 0x3fb33333u);
        for (unsigned cell = 0; cell < 24; ++cell) {
            const auto batch = group * 24 + cell;
            for (unsigned outer = 0; outer < 2; ++outer) {
                for (unsigned point = 0; point < 6; ++point) {
                    auto &v = result.mask_vertices[batch * 12 + outer * 6 + point];
                    for (unsigned axis = 0; axis < 3; ++axis) {
                        v[axis] = outer ? hex[point][axis] * radius : hex[point][axis];
                        v[axis + 3] = static_cast<float>(cell);
                    }
                    v[8] = v[9] = outer ? 0.f : 1.f;
                }
            }
            for (unsigned i = 0; i < indices.size(); ++i)
                result.mask_indices[batch * 48 + i] = batch * 12 + indices[i];
        }
    }
    constexpr std::array<std::array<float, 2>, 4> corners{{{1, 1}, {-1, 1}, {1, -1}, {-1, -1}}};
    constexpr std::array<std::uint32_t, 6> quad_indices{2, 1, 0, 3, 1, 2};
    for (unsigned cell = 0; cell < 24; ++cell) {
        for (unsigned corner = 0; corner < 4; ++corner) {
            const auto [x, y] = corners[corner];
            result.layer_vertices[cell * 4 + corner] = {
                x, y, 0, x, y, static_cast<float>(cell), corner % 2 == 0 ? 1.f : 0.f, corner < 2 ? 0.f : 1.f};
        }
        for (unsigned i = 0; i < quad_indices.size(); ++i)
            result.layer_indices[cell * 6 + i] = cell * 4 + quad_indices[i];
    }
    return result;
}
std::vector<std::array<std::uint8_t, 100>> fog_of_war_draw_records(std::uint32_t requested_quality,
                                                                   const FogOfWarHandles &h) {
    std::vector<std::array<std::uint8_t, 100>> result;
    const auto add = [&](std::uint32_t shader, bool mask, std::uint32_t first) -> auto & {
        auto &record = result.emplace_back();
        const std::array<std::uint32_t, 5> fields{shader, mask ? h.mask_indices : h.layer_indices,
                                                  mask ? h.mask_vertices : h.layer_vertices,
                                                  mask ? 384u : 48u, first};
        for (unsigned i = 0; i < fields.size(); ++i)
            for (unsigned byte = 0; byte < 4; ++byte)
                record[i * 4 + byte] = static_cast<std::uint8_t>(fields[i] >> (byte * 8));
        return record;
    };
    add(52, true, 0);
    add(52, true, 1152);
    if (fog_of_war_startup(requested_quality).quality != 0) {
        for (auto texture : h.cloud_textures) {
            auto &record = add(53, false, 0);
            record[0x18] = static_cast<std::uint8_t>(texture);
            record[0x19] = static_cast<std::uint8_t>(texture >> 8);
            record[0x1a] = static_cast<std::uint8_t>(h.mask_texture);
            record[0x1b] = static_cast<std::uint8_t>(h.mask_texture >> 8);
            record[0x60] = 2;
        }
        add(52, false, 0);
    }
    return result;
}
FogOfWarAssets FogOfWarAssets::load(std::uint32_t requested_quality,
                                    const std::function<assets::AssetHandle(const char *)> &resolve) {
    FogOfWarAssets result(fog_of_war_startup(requested_quality));
    if (result.state_.quality == 0)
        return result;
    if (!resolve)
        throw std::invalid_argument("Fog of war cloud asset resolver is required");
    for (const char *path : cloud_paths) {
        auto asset = resolve(path);
        auto data = assets::DdsData::parse(asset.read_bytes());
        if (data.face_count() != 1)
            throw std::invalid_argument("Fog of war clouds must be two-dimensional textures");
        result.clouds_.push_back(std::move(data));
        result.sources_.push_back(std::move(asset));
    }
    return result;
}
namespace {
std::uint32_t visibility_value(std::uint8_t type) { return type == 0 ? 2u : type == 1 ? 1u : 0u; }
} // namespace
FogOfWarState::FogOfWarState(std::int32_t width, std::int32_t height, std::array<std::int32_t, 3> grid_offset,
                             float minimum_x, float maximum_x) {
    restart(width, height, grid_offset, minimum_x, maximum_x);
}
void FogOfWarState::restart(std::int32_t width, std::int32_t height, std::array<std::int32_t, 3> grid_offset,
                            float minimum_x, float maximum_x) {
    const auto count = static_cast<std::int64_t>(width) * height;
    if (width < 1 || height < 1 || width > 32768 || height > 32768 || count > 1048576 ||
        static_cast<std::int64_t>(grid_offset[0]) + grid_offset[1] + grid_offset[2] != 0)
        throw std::invalid_argument("Fog of war requires a bounded cube grid");
    const float span = maximum_x - minimum_x;
    const float repeated = span * std::bit_cast<float>(0x3b03126fu);
    if (!std::isfinite(minimum_x) || !std::isfinite(maximum_x) || !std::isfinite(span) || span <= 0 ||
        !std::isfinite(repeated) || repeated >= 2147483648.f)
        throw std::invalid_argument("Fog of war requires finite positive world width");
    const float scale = static_cast<float>(static_cast<std::int32_t>(repeated)) / span;
    std::vector<std::uint32_t> visible(static_cast<std::size_t>(count), 0);
    auto override_values = visible;
    std::vector<std::uint32_t> cull(visible.size(), 1);
    width_ = width;
    height_ = height;
    grid_offset_ = grid_offset;
    normalized_scale_ = scale;
    visibility_ = std::move(visible);
    visibility_override_ = std::move(override_values);
    culling_ = std::move(cull);
    dirty_ = true;
}
void FogOfWarState::visibility_event(std::array<std::int32_t, 3> world_cube, std::uint8_t native_type) {
    if (static_cast<std::int64_t>(world_cube[0]) + world_cube[1] + world_cube[2] != 0 ||
        world_cube[0] < -32768 || world_cube[0] > 32767 || world_cube[1] < -32768 || world_cube[1] > 32767)
        throw std::invalid_argument("Fog of war event requires signed 16-bit cube coordinates");
    const auto row = static_cast<std::int64_t>(world_cube[1]) - grid_offset_[1];
    const auto column = static_cast<std::int64_t>(world_cube[0]) - grid_offset_[0] + (row >> 1);
    if (row < 0 || row >= height_ || column < 0 || column >= width_)
        throw std::out_of_range("Fog of war visibility event is outside the grid");
    visibility_[static_cast<std::size_t>(column * height_ + row)] = visibility_value(native_type);
    dirty_ = true;
}
void FogOfWarState::visibility_event_all(std::uint8_t native_type) {
    std::fill(visibility_.begin(), visibility_.end(), visibility_value(native_type));
    dirty_ = true;
}
void FogOfWarState::tick(float delta_seconds) {
    const float elapsed = delta_seconds + elapsed_;
    const float translation = elapsed * -2.f;
    if (!std::isfinite(delta_seconds) || !std::isfinite(elapsed) || !std::isfinite(translation))
        throw std::invalid_argument("Fog of war animation requires finite elapsed time");
    elapsed_ = elapsed;
    frame_scale_ = {normalized_scale_, normalized_scale_, translation, 0};
    if (!dirty_)
        return;
    // Original stack table: center, eight neighbors and twelve cells at
    // distance two. Both axes wrap before the explicit border-row exclusion.
    constexpr std::array<std::array<int, 2>, 21> neighbors{
        {{0, 0},  {-1, 0}, {1, 0},   {0, -1}, {0, 1},   {-1, 1}, {-1, -1}, {1, -1}, {1, 1},  {0, -2}, {0, 2},
         {-2, 0}, {2, 0},  {-1, -2}, {-1, 2}, {-2, -1}, {2, -1}, {1, -2},  {1, 2},  {-2, 1}, {2, 1}}};
    for (int x = 0; x < width_; ++x) {
        for (int y = 0; y < height_; ++y) {
            std::uint32_t cull = 1;
            for (auto [dx, dy] : neighbors) {
                const int nx = (x + dx + width_) % width_;
                const int ny = (y + dy + height_) % height_;
                cull &= visibility_[static_cast<std::size_t>(nx * height_ + ny)] != 2;
            }
            if (y < 2 || y > height_ - 2)
                cull = 0;
            culling_[static_cast<std::size_t>(x * height_ + y)] = cull;
        }
    }
    dirty_ = false;
}
} // namespace civ5::materials
