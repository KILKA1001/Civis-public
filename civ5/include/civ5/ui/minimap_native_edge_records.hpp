#pragma once

#include "civ5/ui/minimap_coast_edge_candidates.hpp"
#include <bit>
#include <cstdint>
#include <map>
#include <span>
#include <stdexcept>
#include <string_view>

namespace civ5::ui {

// Portable slot/point dump from one verified retail DX9 save load. It contains
// both coast and river records; no controlled CRT cursor is used for rendering.
class MinimapNativeEdgeRecordTable {
  public:
    static MinimapNativeEdgeRecordTable parse(
        std::span<const std::uint8_t> bytes, std::string_view expected_save_sha256,
        const terrain::WorldGrid &source) {
        std::size_t at = 0;
        const auto word = [&]() {
            if (bytes.size() - at < 4)
                throw std::invalid_argument("Truncated native minimap edge records");
            std::uint32_t result = 0;
            for (unsigned shift = 0; shift < 32; shift += 8)
                result |= std::uint32_t(bytes[at++]) << shift;
            return result;
        };
        if (bytes.size() < 4 || bytes[0] != 'M' || bytes[1] != 'N' ||
            bytes[2] != 'E' || bytes[3] != 'R')
            throw std::invalid_argument("Invalid native minimap edge magic");
        at = 4;
        if (word() != 1 || expected_save_sha256.size() != 64 || bytes.size() - at < 64)
            throw std::invalid_argument("Invalid native minimap edge identity");
        const auto save_sha = std::string_view(
            reinterpret_cast<const char *>(bytes.data() + at), 64);
        if (save_sha != expected_save_sha256)
            throw std::invalid_argument("Native minimap edges belong to another save");
        at += 64;
        const auto source_width = word(), source_height = word();
        const auto table_width = word(), table_height = word();
        if (source_width != static_cast<std::uint32_t>(source.width) ||
            source_height != static_cast<std::uint32_t>(source.height) ||
            table_width != 200 || table_height != 200)
            throw std::invalid_argument("Native minimap edge grid dimensions differ");
        MinimapNativeEdgeRecordTable result(source);
        terrain::IntMatrix4 source_matrix{};
        for (auto &value : source_matrix) value = std::bit_cast<std::int32_t>(word());
        if (source_matrix != source.first_matrix)
            throw std::invalid_argument("Native minimap edge source matrix differs");
        for (auto &value : result.table_matrix_) value = std::bit_cast<std::int32_t>(word());
        const auto count = word();
        if (count != 1294 || bytes.size() - at != std::size_t(count) * 20)
            throw std::invalid_argument("Native minimap edge record count differs");
        std::uint32_t previous_slot = 0;
        for (std::uint32_t index = 0; index < count; ++index) {
            const auto slot = word();
            if (slot >= table_width * table_height * 12 ||
                (index && slot <= previous_slot))
                throw std::invalid_argument("Native minimap edge slots are unordered");
            previous_slot = slot;
            MinimapTerrainEdgeRecord record{{}};
            for (unsigned point = 0; point < 2; ++point) {
                const auto x = std::bit_cast<float>(word());
                const auto y = std::bit_cast<float>(word());
                if (!std::isfinite(x) || !std::isfinite(y))
                    throw std::invalid_argument("Native minimap edge point is nonfinite");
                record.relative_world_points.push_back({x, y});
            }
            result.records_.emplace(slot, std::move(record));
        }
        return result;
    }

    std::optional<MinimapTerrainEdgeRecord> lookup(const MinimapEdgeQuery &query) const {
        if (query.direction >= 12)
            throw std::invalid_argument("Invalid native minimap edge direction");
        const auto cube = detail::coast_normalize_source_cube(query.cube, source_);
        const auto offset = detail::coast_transform_cube(cube, table_matrix_);
        const auto x = std::bit_cast<std::int32_t>(
            static_cast<std::uint32_t>(offset[0]) +
            static_cast<std::uint32_t>(offset[1] >> 1));
        const auto y = offset[1];
        if (x < 0 || x >= 200 || y < 0 || y >= 200)
            return std::nullopt;
        const auto slot = std::uint32_t((200 * y + x) * 12 + query.direction);
        const auto found = records_.find(slot);
        if (found == records_.end()) return std::nullopt;
        return found->second;
    }

    std::size_t size() const { return records_.size(); }

  private:
    explicit MinimapNativeEdgeRecordTable(const terrain::WorldGrid &source) : source_(source) {}
    terrain::WorldGrid source_;
    terrain::IntMatrix4 table_matrix_{};
    std::map<std::uint32_t, MinimapTerrainEdgeRecord> records_;
};

} // namespace civ5::ui
