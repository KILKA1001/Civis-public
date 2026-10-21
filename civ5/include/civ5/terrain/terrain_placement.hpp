#pragma once
#include "terrain_pieces.hpp"

namespace civ5::terrain {
// Original 4E8C90 constructor. Slot zero uses the verified global factory;
// the remaining three descriptors and reflection tables are native constants.
std::array<IntMatrix4, 4> build_terrain_orientations(const FactoryDescriptor &factory);
struct TerrainOccurrence {
    std::uint32_t cooked_index, orientation;
    LogicalHex position;
};
struct LogicalCellCoordinate {
    std::int32_t row, column;
    bool operator==(const LogicalCellCoordinate &) const = default;
};
class TerrainLogicalTransform {
  public:
    LogicalCellCoordinate cell(MarkupCoordinate markup) const;
  private:
    friend class TerrainPlacement;
    explicit TerrainLogicalTransform(IntMatrix4 matrix) : matrix_(matrix) {}
    IntMatrix4 matrix_;
};
class TerrainPlacement {
  public:
    // Explicit same-map matrices and four corners. Float grouping remains a
    // bounded evidence domain: reject a coordinate whose rounded result changes
    // between the two observed candidate sum groupings. No fitted defaults.
    TerrainPlacement(const std::array<IntMatrix4, 4> &orientations, const IntMatrix4 &map_to_logical,
                     const IntMatrix4 &logical_to_placement, const Matrix4 &occurrence_to_cache,
                     const std::array<Vector4, 4> &corners);
    SpatialRectangle rectangle(const TerrainOccurrence &occurrence, std::int32_t source_size) const;
    LogicalCellCoordinate logical_cell(const TerrainOccurrence &occurrence, MarkupCoordinate markup) const;
    TerrainLogicalTransform logical_transform(const TerrainOccurrence &occurrence) const;
    // Ordered extended lattice only; neither a shuffle nor a PRNG state default.
    std::vector<LogicalHex> candidate_positions(std::int32_t width, std::int32_t height) const;

  private:
    std::array<IntMatrix4, 4> orientations_;
    IntMatrix4 map_to_logical_, logical_to_placement_;
    Matrix4 cache_;
    std::array<std::array<std::array<std::int32_t, 4>, 2>, 4> endpoints_;
};

std::uint32_t terrain_primary_ordinal(const TerrainOccurrence &occurrence,
                                    std::span<const std::uint32_t> cooked_to_primary,
                                    const TerrainPieceLibrary &library);
// Direct typed consumer: signed endpoints and library-owned decoded sources.
// Cooked/primary order is explicit and need not be identity.
std::vector<TerrainDescriptor> build_terrain_descriptors(const TerrainPieceLibrary &library,
                                                       const TerrainPlacement &placement,
                                                       std::span<const TerrainOccurrence> occurrences,
                                                       std::span<const std::uint32_t> cooked_to_primary);
bool terrain_markup_matches(TerrainMarkup required, std::uint32_t mask, std::uint8_t category);
// Column-major logical cells (row * height + column), eight retained bytes per
// cell. Only accepted occurrence claims change byte +5. Rejection is atomic.
// This primitive does not establish the occurrence selection/shuffle order.
class TerrainLogicalClaims {
  public:
    TerrainLogicalClaims(std::int32_t width, std::int32_t height, std::span<const std::uint8_t> cells);
    bool try_claim(const TerrainPieceRecord &record, const TerrainOccurrence &occurrence,
                   const TerrainPlacement &placement);
    bool try_claim(const TerrainPieceRecord &record, const TerrainLogicalTransform &transform);
    std::span<const std::uint8_t> cells() const { return cells_; }
    std::size_t claim_count() const { return claim_count_; }

  private:
    std::int32_t width_, height_;
    std::vector<std::uint8_t> cells_;
    std::size_t claim_count_ = 0;
};
} // namespace civ5::terrain
