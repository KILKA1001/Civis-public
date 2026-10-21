#pragma once
#include "coast_curves.hpp"
#include "coast_raster.hpp"

namespace civ5::terrain {
class TerrainRandom;
struct CoastStripChunk {
    std::uint32_t first_vertex, vertex_count, first_triangle, triangle_count;
};
class CoastStrip {
  public:
    std::span<const CoastVertex> vertices() const { return vertices_; }
    std::span<const std::uint16_t> local_indices() const { return indices_; }
    std::span<const CoastStripChunk> chunks() const { return chunks_; }
    std::size_t triangle_count() const { return indices_.size() / 3; }
    static std::array<float, 3> coefficients(std::size_t triangle);
    // Owns the expanded triangle data. Indices restart in each chunk, even when
    // the global strip contains more than 65535 vertices.
    CoastRasterJob raster_job(std::size_t chunk, std::array<std::uint8_t, 4> remap_from,
                              std::array<std::uint8_t, 4> remap_to) const;

  private:
    friend CoastStrip build_coast_strip(const CoastCurveSet &, const CoastProfiles &,
                                        std::span<const std::uint32_t>, std::uint32_t, std::uint32_t);
    std::vector<CoastVertex> vertices_;
    std::vector<std::uint16_t> indices_;
    std::vector<CoastStripChunk> chunks_;
};
// 0x00446D80. Five corresponding curves -> four bands, native local-U16 chunks.
// Profile cache/direction changes are shared across bands and rows. No fitted
// offsets, transition clamps, resampling or inferred cross-job execution order.
CoastStrip build_coast_strip(const CoastCurveSet &curves, const CoastProfiles &profiles,
                             std::span<const std::uint32_t> palette, std::uint32_t family_prefix,
                             std::uint32_t common_prefix);

struct CoastDispatchJob {
    std::size_t strip, chunk;
    // Original packet +0x48: selected job receives integrity return minus 255.
    std::uint32_t allocation_flag;
};
struct CoastDispatchPlan {
    std::uint16_t gate;
    std::array<std::vector<CoastDispatchJob>, 2> phases;
    std::array<std::uint8_t, 4> remap_from{}, remap_to{};
    unsigned direct_random_calls = 0;
};
// Original 631920 -> 66ef00, pinned unmodified retail EXE at its preferred base.
// Executing all 92 CRC regions plus the wrapper return RVA yields 255, without
// consuming RNG or writing outside the stack. Other images are not inferred.
inline constexpr std::uint32_t unmodified_coast_integrity_return = 255;
// 0x00663550 submission packets. Each phase must finish before the next starts;
// order WITHIN a phase is not an execution-order contract. Random returns and
// the image-dependent integrity result remain explicit. Empty strips draw none.
CoastDispatchPlan build_coast_dispatch(std::span<const CoastStrip> strips, std::uint16_t gate,
                                       std::span<const std::uint32_t> palette,
                                       std::span<const std::uint32_t> direct_random_values,
                                       std::uint32_t integrity_return);
CoastDispatchPlan build_coast_dispatch(std::span<const CoastStrip> strips, std::uint16_t gate,
                                       std::span<const std::uint32_t> palette, TerrainRandom &random,
                                       std::uint32_t integrity_return);
} // namespace civ5::terrain
