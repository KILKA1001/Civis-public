#pragma once
#include "civ5/assets/dds.hpp"
#include "height_addressing.hpp"
#include "terrain_auxiliary.hpp"
#include <utility>

namespace civ5::terrain {
// Original 4DC1B7..4DC1FF copies the base L8 image beginning at byte ONE
// of a 0x4008-byte allocation. Raster 5B8810 reads its first 128*128 bytes
// in x-major order. Preserve that offset and the explicit prior first byte.
class TerrainMarshStamp {
  public:
    static constexpr std::size_t storage_bytes = 0x4008;
    using Storage = std::array<std::uint8_t, storage_bytes>;
    static TerrainMarshStamp prepare(const assets::DdsData &dds, Storage prior);
    const Storage &storage() const { return storage_; }
  private:
    explicit TerrainMarshStamp(Storage bytes) : storage_(std::move(bytes)) {}
    Storage storage_;
};
struct TerrainMarshState {
    bool enabled = false;
    // Value ownership replaces native vector/pool addresses. Both lists keep
    // original insertion order; retained nodes contain all five source words.
    std::vector<TerrainFeature2Source> pending, retained;
};
struct TerrainMarshSettings {
    Matrix4 cache_matrix;
    std::int32_t map_mode, raster_width, raster_height;
    X87Precision precision;
    Vec4Path transform;
    std::span<const std::uint32_t> material_codes;
};
struct TerrainMarshApplication {
    TerrainFeature2Source source;
    std::int32_t origin_x, origin_y;
    std::uint32_t code;
    std::uint8_t material;
    // Arguments to original cache invalidation 684240, including empty or
    // reversed bounds for a wholly clipped stamp. Renderer processing is a
    // separate consumer, not simulated by this terrain operation.
    Point2 dirty_lower, dirty_upper;
};
struct TerrainMarshResult {
    std::vector<std::uint8_t> materials;
    TerrainMarshState state;
    std::vector<TerrainMarshApplication> applications;
};
using TerrainMarshWriteObserver = std::function<void(std::size_t application,
    std::int32_t destination_index, std::uint8_t material)>;
// 4ADF90 -> 7399D0 -> 5B8810. Disabled appends queue only; activate then
// executes 48A570's pending flush in source order and clears the queue.
// Heights are read-only. Every prior/input buffer remains owned by its caller
// on success or failure. The caller chooses this stage's position in loading.
TerrainMarshResult apply_terrain_marshes(HeightDomain domain,
    std::span<const std::uint16_t> heights, std::span<const std::uint8_t> materials,
    const TerrainMarshState &prior, std::span<const TerrainFeature2Source> sources,
    bool activate, const TerrainMarshStamp &stamp, const TerrainMarshSettings &settings,
    const TerrainMarshWriteObserver &write = {});
} // namespace civ5::terrain
