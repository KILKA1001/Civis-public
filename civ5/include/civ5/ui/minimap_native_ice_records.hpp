#pragma once

#include <array>
#include <bit>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <optional>
#include <stdexcept>
#include <vector>

namespace civ5::ui {

// Retail 0x7240B0 stores SHORT2N XY at the start of each eight-byte vertex
// and local 16-bit triangle indices. Keep that layout through the GPU upload
// instead of expanding every index into a Float32 triangle vertex on the CPU.
struct MinimapNativeIceRegion {
    std::array<float, 4> box{};
    std::vector<std::array<std::int16_t, 4>> vertices;
    std::vector<std::uint16_t> indices;
};

struct MinimapNativeIceRecords {
    std::uint32_t seed = 0;
    // V2 retains the original ice component's +0xFF1C bounds. Retail
    // 0x878DE0 packs every vertex against this box, and 0x5CC580 copies it
    // into the kind-9 shader setup before the region draw loop. The 32-byte
    // region boxes are only used for visible-region culling.
    std::optional<std::array<float, 4>> shader_box;
    std::vector<MinimapNativeIceRegion> regions;
    std::uint32_t packed_vertices = 0, indices = 0;
};

// This is an explicit validation artifact, not a substitute for the saved
// game's unobserved runtime ice-region generator or GPU handles.
inline MinimapNativeIceRecords read_minimap_native_ice_records(
    const std::filesystem::path &path) {
    static_assert(std::endian::native == std::endian::little);
    static_assert(sizeof(std::array<std::int16_t, 4>) == 8);
    std::ifstream input(path, std::ios::binary | std::ios::ate);
    if (!input) throw std::runtime_error("Cannot read native minimap ice records");
    const auto length = input.tellg();
    if (length < 120 || length > 16 * 1024 * 1024)
        throw std::runtime_error("Invalid native minimap ice record length");
    input.seekg(0);
    std::array<std::uint8_t, 120> header{};
    input.read(reinterpret_cast<char *>(header.data()), header.size());
    const bool with_shader_box = !std::memcmp(header.data(), "C5IRTB02", 8);
    if (!input || (!with_shader_box && std::memcmp(header.data(), "C5IRTB01", 8)))
        throw std::runtime_error("Invalid native minimap ice record header");
    const auto u32 = [&](std::size_t at) {
        std::uint32_t value;
        std::memcpy(&value, header.data() + at, sizeof(value));
        return value;
    };
    const auto region_count = u32(104), vertex_count = u32(108), index_count = u32(112);
    if (!region_count || region_count > 64 || !vertex_count || vertex_count > 1'000'000 ||
        !index_count || index_count > 3'000'000 ||
        120ull + (with_shader_box ? 16ull : 0ull) + 32ull * region_count +
            8ull * vertex_count + 2ull * index_count != length)
        throw std::runtime_error("Invalid native minimap ice record extent");
    MinimapNativeIceRecords result;
    result.seed = u32(116);
    result.packed_vertices = vertex_count;
    result.indices = index_count;
    if (with_shader_box) {
        result.shader_box.emplace();
        input.read(reinterpret_cast<char *>(result.shader_box->data()), 16);
        const auto &box = *result.shader_box;
        if (!input || !(std::isfinite(box[0]) && std::isfinite(box[1]) &&
                        std::isfinite(box[2]) && std::isfinite(box[3]) &&
                        box[2] > box[0] && box[3] > box[1]))
            throw std::runtime_error("Invalid native minimap ice shader bounds");
    }
    std::vector<std::array<std::uint8_t, 32>> rows(region_count);
    input.read(reinterpret_cast<char *>(rows.data()), 32ull * region_count);
    if (!input) throw std::runtime_error("Truncated native minimap ice region table");
    const auto row_u32 = [](const auto &row, std::size_t at) {
        std::uint32_t value;
        std::memcpy(&value, row.data() + at, sizeof(value));
        return value;
    };
    const std::size_t vertex_start = 120 + (with_shader_box ? 16 : 0) + 32 * region_count;
    const std::size_t index_start = vertex_start + 8 * vertex_count;
    result.regions.reserve(region_count);
    std::uint32_t next_vertex = 0, next_index = 0;
    for (const auto &row : rows) {
        const auto first_vertex = row_u32(row, 0), first_index = row_u32(row, 4);
        const auto vertices = row_u32(row, 8), indices = row_u32(row, 12);
        if (first_vertex != next_vertex || first_index != next_index ||
            !vertices || vertices > 65536 || !indices || indices > 1'000'000 ||
            indices % 3 || first_vertex > vertex_count || first_index > index_count ||
            vertices > vertex_count - first_vertex || indices > index_count - first_index)
            throw std::runtime_error("Invalid native minimap ice region range");
        MinimapNativeIceRegion region;
        std::memcpy(region.box.data(), row.data() + 16, sizeof(region.box));
        if (!(std::isfinite(region.box[0]) && std::isfinite(region.box[1]) &&
              std::isfinite(region.box[2]) && std::isfinite(region.box[3]) &&
              region.box[2] > region.box[0] && region.box[3] > region.box[1]))
            throw std::runtime_error("Invalid native minimap ice region bounds");
        region.vertices.resize(vertices);
        region.indices.resize(indices);
        input.seekg(vertex_start + 8ull * first_vertex);
        input.read(reinterpret_cast<char *>(region.vertices.data()), 8ull * vertices);
        input.seekg(index_start + 2ull * first_index);
        input.read(reinterpret_cast<char *>(region.indices.data()), 2ull * indices);
        if (!input) throw std::runtime_error("Truncated native minimap ice region geometry");
        for (const auto index : region.indices)
            if (index >= vertices)
                throw std::runtime_error("Native minimap ice index escaped its region");
        result.regions.push_back(std::move(region));
        next_vertex += vertices;
        next_index += indices;
    }
    if (next_vertex != vertex_count || next_index != index_count)
        throw std::runtime_error("Native minimap ice region table has gaps");
    return result;
}

} // namespace civ5::ui
