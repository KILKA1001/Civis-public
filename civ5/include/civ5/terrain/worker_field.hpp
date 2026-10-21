#pragma once
#include "height_addressing.hpp"
#include "native_math.hpp"
#include <memory>

namespace civ5::terrain {
struct SpatialRectangle {
    std::int32_t x0, y0, x1, y1;
};

// Decoded source grids, owned once and shared immutably by occurrences. The
// loader/PRNG/occurrence builder is a separate boundary, not inferred here.
class TerrainPieceGrids {
  public:
    TerrainPieceGrids(std::int32_t size, std::span<const std::uint16_t> heights,
                      std::span<const std::uint8_t> materials, std::span<const std::uint8_t> weights,
                      std::span<const std::uint8_t> priorities);
    std::int32_t size() const { return size_; }
    std::span<const std::uint16_t> heights() const { return heights_; }
    std::span<const std::uint8_t> materials() const { return materials_; }
    std::span<const std::uint8_t> weights() const { return weights_; }
    std::span<const std::uint8_t> priorities() const { return priorities_; }

  private:
    std::int32_t size_;
    std::vector<std::uint16_t> heights_;
    std::vector<std::uint8_t> materials_, weights_, priorities_;
};
struct TerrainDescriptor {
    SpatialRectangle rectangle;
    std::uint32_t record_state_flag;
    std::uint16_t cell_gate;
    std::shared_ptr<const TerrainPieceGrids> grids;
};
using WorkerPackets = std::vector<std::vector<std::uint32_t>>;
// Literal inclusive endpoint routing, signed truncating division, X wrap and
// clipped Y. Descriptor order AND duplicate visits are significant.
WorkerPackets route_worker_descriptors(HeightDomain domain, std::span<const SpatialRectangle> rectangles);

inline constexpr float worker_initial_denominator = 1e-6f;
inline constexpr float worker_u16_to_normalized = 1.5259021893143654e-5f;
inline constexpr float worker_weight_to_normalized = 0.003921568859368563f;
inline constexpr float worker_retail_bias = 0.23533989489078522f;
struct WorkerCell {
    float numerator = 0.f;
    float denominator = worker_initial_denominator;
    std::uint8_t material = 3;
    std::uint8_t priority = 0;
};
class WorkerAccumulation {
  public:
    std::span<const WorkerCell> cells() const { return cells_; }
    float bias() const { return bias_; }
    // Explicit original x87 precision, emulated arithmetically in the x64
    // runtime. No host-control-word mutation, clamp, fill or normalization.
    std::vector<std::uint16_t> finalize(X87Precision precision) const;

  private:
    friend WorkerAccumulation accumulate_worker64(HeightDomain, std::int32_t, std::int32_t,
                                                  std::span<const TerrainDescriptor>,
                                                  std::span<const std::uint32_t>,
                                                  std::span<const std::uint8_t>, float);
    explicit WorkerAccumulation(float bias) : bias_(bias), cells_(4096) {}
    float bias_;
    std::vector<WorkerCell> cells_;
};
WorkerAccumulation accumulate_worker64(HeightDomain domain, std::int32_t origin_x, std::int32_t origin_y,
                                       std::span<const TerrainDescriptor> descriptors,
                                       std::span<const std::uint32_t> ordered_packet,
                                       std::span<const std::uint8_t> local_gates, float bias);

class WorkerField {
  public:
    const HeightDomain &domain() const { return domain_; }
    std::span<const std::uint16_t> heights() const { return heights_; }
    std::span<const std::uint8_t> materials() const { return materials_; }
    HeightField height_field() const { return HeightField(domain_, heights_); }

  private:
    friend WorkerField compose_worker_field(HeightDomain, std::span<const TerrainDescriptor>,
                                            std::span<const std::uint8_t>, float, X87Precision);
    explicit WorkerField(HeightDomain domain);
    HeightDomain domain_;
    std::vector<std::uint16_t> heights_;
    std::vector<std::uint8_t> materials_;
};
// Complete PRE-coast field from explicit descriptors and exact gate bytes.
// Does not recover the saved game's seed entry, gates or coast-job ordering.
WorkerField compose_worker_field(HeightDomain domain, std::span<const TerrainDescriptor> descriptors,
                                 std::span<const std::uint8_t> gates, float bias, X87Precision precision);
} // namespace civ5::terrain
