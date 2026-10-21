#pragma once

#include "civ5/ui/minimap_visible_bounds.hpp"
#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace civ5::ui {

// MNB1 is a reproducible packaging of two identical passive retail DX9
// snapshots, not a camera pose. The captured kind-8 corners are intentionally
// absent: their 1.6 lens does not match the G0 1274x925 camera.
struct MinimapNativeBoundsFixture {
    MinimapVisibleBounds visible;
    float height_source = 0, plane_height = 0;
    std::array<float, 2> delhi_world{}, reference_marker_map_local{};
    std::array<std::uint8_t, 32> capture_a_sha{}, capture_b_sha{},
                                 reference_tga_sha{}, canonical_state_sha{};

    static constexpr std::size_t binary_size = 292;

    static MinimapNativeBoundsFixture parse(
        std::span<const std::uint8_t> bytes, std::string_view expected_save_sha,
        std::string_view expected_exe_sha, std::string_view expected_tga_sha) {
        if (bytes.size() != binary_size ||
            !std::equal(bytes.begin(), bytes.begin() + 8,
                        std::array<std::uint8_t, 8>{'C','5','M','N','B','0','1',0}.begin()))
            throw std::invalid_argument("Invalid native minimap bounds fixture magic or size");
        const auto expected_bytes = [](std::string_view hex) {
            if (hex.size() != 64) throw std::invalid_argument("Invalid minimap fixture identity hash");
            std::array<std::uint8_t, 32> result{};
            const auto nibble = [](char ch) -> std::uint8_t {
                if (ch >= '0' && ch <= '9') return static_cast<std::uint8_t>(ch - '0');
                if (ch >= 'a' && ch <= 'f') return static_cast<std::uint8_t>(ch - 'a' + 10);
                throw std::invalid_argument("Noncanonical minimap fixture identity hash");
            };
            for (std::size_t i = 0; i < result.size(); ++i)
                result[i] = static_cast<std::uint8_t>((nibble(hex[i * 2]) << 4) |
                                                       nibble(hex[i * 2 + 1]));
            return result;
        };
        const auto sha_at = [&](std::size_t offset) {
            std::array<std::uint8_t, 32> value{};
            std::copy_n(bytes.begin() + offset, value.size(), value.begin());
            return value;
        };
        if (sha_at(8) != expected_bytes(expected_save_sha) ||
            sha_at(40) != expected_bytes(expected_exe_sha) ||
            sha_at(136) != expected_bytes(expected_tga_sha) ||
            sha_at(72) == sha_at(104))
            throw std::invalid_argument("Native minimap bounds provenance differs");
        const auto u32 = [&](std::size_t offset) {
            std::uint32_t value = 0;
            for (unsigned shift = 0; shift < 32; shift += 8)
                value |= std::uint32_t(bytes[offset + shift / 8]) << shift;
            return value;
        };
        const auto f32 = [&](std::size_t offset) { return std::bit_cast<float>(u32(offset)); };
        const auto width = u32(200), height = u32(204), padding = u32(208);
        const auto active_width = u32(212), count = u32(216);
        if (width != 320 || height != 190 || active_width == 0 ||
            active_width > width || padding != (width - active_width) / 2 ||
            padding + active_width > width || count == 0 || count > 2)
            throw std::invalid_argument("Invalid native minimap texture or segment count");
        MinimapNativeBoundsFixture fixture;
        fixture.capture_a_sha = sha_at(72);
        fixture.capture_b_sha = sha_at(104);
        fixture.reference_tga_sha = sha_at(136);
        fixture.canonical_state_sha = sha_at(168);
        fixture.height_source = f32(220);
        fixture.plane_height = f32(224);
        if (!std::isfinite(fixture.height_source) || fixture.height_source < 0 ||
            fixture.height_source > 1 || !std::isfinite(fixture.plane_height) ||
            std::abs(terrain::post_initialization_scalar(fixture.height_source, 0.2f) -
                     fixture.plane_height) > 1e-5f)
            throw std::invalid_argument("Native minimap height source differs from owner plane");
        fixture.visible.texture = {padding, active_width,
                                   static_cast<float>(active_width) / height};
        fixture.visible.segment_count = count;
        for (std::size_t index = 0; index < 2; ++index) {
            const auto offset = 228 + index * 16;
            const MinimapWorldBounds segment{
                f32(offset), f32(offset + 4), f32(offset + 8), f32(offset + 12)};
            if (index < count) {
                if (!(std::isfinite(segment.min_x) && std::isfinite(segment.min_y) &&
                      std::isfinite(segment.max_x) && std::isfinite(segment.max_y) &&
                      segment.max_x > segment.min_x && segment.max_y > segment.min_y))
                    throw std::invalid_argument("Invalid native minimap world bounds");
                fixture.visible.segments[index] = segment;
            } else if (segment.min_x != 0 || segment.min_y != 0 ||
                       segment.max_x != 0 || segment.max_y != 0) {
                throw std::invalid_argument("Unused native minimap segment is not empty");
            }
        }
        fixture.delhi_world = {f32(260), f32(264)};
        fixture.reference_marker_map_local = {f32(268), f32(272)};
        if (u32(276) != 40 || u32(280) != 24 || u32(284) != 25 || u32(288) != 15 ||
            std::abs(fixture.delhi_world[0] - 831.965576f) > .001f ||
            std::abs(fixture.delhi_world[1] - 166.319580f) > .001f)
            throw std::invalid_argument("Native minimap fixture is not the selected Gandhi map");
        const auto &bounds = fixture.visible.segments[0];
        const float map_x = padding + (fixture.delhi_world[0] - bounds.min_x) *
            active_width / (bounds.max_x - bounds.min_x);
        const float map_y = (bounds.max_y - fixture.delhi_world[1]) * height /
            (bounds.max_y - bounds.min_y);
        if (!std::isfinite(map_x) || !std::isfinite(map_y) ||
            !std::isfinite(fixture.reference_marker_map_local[0]) ||
            !std::isfinite(fixture.reference_marker_map_local[1]) ||
            std::abs(map_x - fixture.reference_marker_map_local[0]) >= .1f ||
            std::abs(map_y - fixture.reference_marker_map_local[1]) >= .1f)
            throw std::invalid_argument("Native minimap bounds miss the original Delhi marker");
        return fixture;
    }

    static MinimapNativeBoundsFixture read(
        const std::filesystem::path &path, std::string_view expected_save_sha,
        std::string_view expected_exe_sha, std::string_view expected_tga_sha) {
        std::ifstream input(path, std::ios::binary);
        std::vector<std::uint8_t> bytes((std::istreambuf_iterator<char>(input)),
                                         std::istreambuf_iterator<char>());
        if (!input && !input.eof())
            throw std::runtime_error("Cannot read native minimap bounds fixture: " +
                                     path.string());
        return parse(bytes, expected_save_sha, expected_exe_sha, expected_tga_sha);
    }
};

} // namespace civ5::ui
