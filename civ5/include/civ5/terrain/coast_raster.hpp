#pragma once
#include "height_addressing.hpp"
#include <functional>

namespace civ5::terrain {
struct CoastVertex {
    float x, y;
    std::uint8_t material;
    float height, opacity;
};
struct CoastTriangle {
    CoastVertex a, b, c;
    float edge_ab, edge_bc, edge_ca;
};
class CoastRasterJob {
  public:
    CoastRasterJob(std::span<const CoastTriangle> triangles, std::array<std::uint8_t, 4> remap_from,
                   std::array<std::uint8_t, 4> remap_to);
    std::span<const CoastTriangle> triangles() const { return triangles_; }
    const auto &remap_from() const { return from_; }
    const auto &remap_to() const { return to_; }

  private:
    std::vector<CoastTriangle> triangles_;
    std::array<std::uint8_t, 4> from_, to_;
};
// Only floor(min + 0.5) uses this precision. On bounded float32 coordinates
// PC53/64 produce the same bounds; raster arithmetic remains separate float32.
enum class CoastBoundsPrecision { single, double_or_extended };
enum class CoastWriteChannel { height, material };
using CoastWriteObserver = std::function<void(CoastWriteChannel, std::int32_t, std::uint32_t)>;
using CoastJobComplete = std::function<void(std::size_t)>;

class CoastRasterField {
  public:
    const HeightDomain &domain() const { return domain_; }
    std::span<const std::uint16_t> heights() const { return heights_; }
    std::span<const std::uint8_t> materials() const { return materials_; }
    HeightField height_field() const { return HeightField(domain_, heights_); }

  private:
    friend CoastRasterField apply_coast_jobs(HeightDomain, std::span<const std::uint16_t>,
                                             std::span<const std::uint8_t>, std::span<const CoastRasterJob>,
                                             bool, CoastBoundsPrecision, const CoastWriteObserver &,
                                             const CoastJobComplete &);
    CoastRasterField(HeightDomain domain, std::span<const std::uint16_t> heights,
                     std::span<const std::uint8_t> materials);
    HeightDomain domain_;
    std::vector<std::uint16_t> heights_;
    std::vector<std::uint8_t> materials_;
};
// Serial order is caller-owned, NEVER inferred from native dispatch submission.
// Copies input fields once; observers see all stores, even unchanged/repeated.
// Does not generate coast strips, select a scheduler, allocate native pages or
// infer an actual saved-game state. No upper blend/height clamp or smoothing.
CoastRasterField apply_coast_jobs(HeightDomain domain, std::span<const std::uint16_t> heights,
                                  std::span<const std::uint8_t> materials,
                                  std::span<const CoastRasterJob> ordered_jobs, bool wrap_x,
                                  CoastBoundsPrecision precision, const CoastWriteObserver &observer = {},
                                  const CoastJobComplete &job_complete = {});
} // namespace civ5::terrain
