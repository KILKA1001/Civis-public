#pragma once
#include "civ5/assets/asset_source.hpp"
#include "worker_field.hpp"

namespace civ5::terrain {
struct MarkupCoordinate {
    std::uint8_t x, y;
    bool operator==(const MarkupCoordinate &) const = default;
};
struct TerrainMarkup {
    std::uint32_t mask;
    std::uint8_t category, claim;
};
struct TerrainRowTables {
    std::uint32_t heights, materials, weights, priorities;
    bool operator==(const TerrainRowTables &) const = default;
};
struct TerrainPieceRecord {
    std::uint32_t ordinal, serialized_ordinal, generation_pass, raw_state;
    TerrainRowTables row_tables;
    std::uint16_t result_code, selection_key_a, selection_key_b;
    std::string primary_name, secondary_name;
    std::array<std::string, 4> dds_names;
    std::array<std::uint8_t, 800> markup;
    std::vector<MarkupCoordinate> active_coordinates;
    std::int32_t size() const { return (raw_state & 1) ? 128 : 512; }
    TerrainMarkup markup_at(MarkupCoordinate coordinate) const;
};

// Direct format-0x14 FXSB storage. All pools and unknown record bytes remain
// owned losslessly. Positive RLE pairs terminate at the requested row width,
// including a clipped final pair; adjacent row pointers are not delimiters.
// Grids are decoded once on demand, shared immutably and safe for worker reads.
class TerrainPieceLibrary {
  public:
    static TerrainPieceLibrary parse(std::span<const std::uint8_t> fxsb);
    static TerrainPieceLibrary load(const assets::AssetSource &source, const assets::AssetKey &key);
    std::span<const TerrainPieceRecord> records() const;
    const TerrainPieceRecord &record(std::uint32_t ordinal) const;
    std::shared_ptr<const TerrainPieceGrids> grids(std::uint32_t ordinal) const;
    std::span<const std::uint8_t> serialized_bytes() const;
    std::span<const std::uint8_t> serialized_record(std::uint32_t ordinal) const;
    const std::optional<assets::AssetHandle> &asset() const;

  private:
    struct Storage;
    explicit TerrainPieceLibrary(std::shared_ptr<Storage> storage) : storage_(std::move(storage)) {}
    std::shared_ptr<Storage> storage_;
};
} // namespace civ5::terrain
