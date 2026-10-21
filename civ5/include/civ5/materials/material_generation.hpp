#pragma once
#include "civ5/terrain/native_math.hpp"
#include "material_mesh.hpp"
#include <functional>

namespace civ5::materials {
using MaterialVector3 = std::array<float, 3>;
using MaterialOrigin = std::array<std::int32_t, 2>;
using MaterialLookup = std::function<std::int32_t(std::int32_t, std::int32_t)>;
using MaterialNormalLookup = std::function<MaterialVector3(std::int32_t, std::int32_t)>;
using MaterialFactorLookup = std::function<std::vector<float>(std::int32_t, std::int32_t, int)>;
using MaterialNormalSeed = std::function<float(int, int, float)>;

// Original conditional UInt32 sort and complete 64-word owner array. Only the
// active prefix (<=8) is used for vertices; the sorted suffix is retained.
class MaterialSelection {
  public:
    explicit MaterialSelection(std::span<const std::uint32_t> scores, int cache_mode = 0,
                               int parent_flag = 0);
    MaterialSelection(std::span<const std::uint32_t> scores, int cache_mode, int parent_flag,
                      std::span<const std::int32_t> material_slots,
                      std::span<const std::uint32_t> specular_handles);
    std::span<const std::int32_t> sources() const { return {words_.data(), count_}; }
    const std::array<std::int32_t, 64> &owner_words() const { return words_; }
    int specular() const { return specular_; }
    // 684240 clears only +1F8 before submitting a fresh selection job.
    void clear_sources() { count_ = 0; }

  private:
    std::array<std::int32_t, 64> words_{};
    std::size_t count_ = 0;
    int specular_ = 0;
};
struct MaterialSample {
    std::int32_t x, y;
    float weight;
};
// Wrapped ADD precedes unsigned remainder. Zero totals retain IEEE NaNs.
std::vector<float> material_factors(int count, std::span<const MaterialSample> samples, int width, int height,
                                    MaterialOrigin origin, int axis, std::span<const std::int32_t> slots,
                                    std::span<const std::int32_t> modes, const MaterialLookup &lookup);
std::array<std::uint8_t, 48> write_material_lanes(std::span<const std::uint8_t> prior,
                                                  const MaterialSelection &selection,
                                                  std::span<const float> normalized,
                                                  terrain::X87Precision precision, int alternate = 0,
                                                  int axis = 0);

struct MaterialSourceDomain {
    int width, height;
    MaterialVector3 scales;
    int normal_x_remap;
};
std::array<float, 25> material_normal_kernel(int axis);
MaterialVector3 material_normal_input(MaterialOrigin position, const MaterialSourceDomain &source,
                                      const MaterialLookup &height_at);
// RSQRTSS approximation is explicit state, not an invented 1/sqrt replacement.
MaterialVector3 normalize_material_normal(const MaterialVector3 &input,
                                          const terrain::ReciprocalEstimate &seed);
int material_projection_axis(const MaterialVector3 &normal);
std::uint16_t material_half(float value); // original scalar D3DX, including non-IEEE 7fff

struct MaterialTile {
    int x0, x1, y0, y1;
    int vertex_count() const;
    bool operator==(const MaterialTile &) const = default;
};
struct MaterialGeometryDomain {
    int columns, rows;
    MaterialOrigin origin;
    MaterialSourceDomain source;
    int cache_size;
    terrain::X87Precision precision;
};
struct MaterialStreams {
    assets::Bytes material48, normal24;
};
struct MaterialStreamViews {
    std::span<std::uint8_t> material48, normal24;
};
// Worker form writes into caller-owned staging. Bounds and extents are checked
// before writes. Producer exceptions can leave a partially written tile;
// the owner must discard that failed job's output before publication.
void write_material_tile_into(const MaterialGeometryDomain &domain, MaterialTile tile,
                              MaterialStreamViews destination, const MaterialSelection &selection,
                              int alternate, const MaterialLookup &height_at,
                              const MaterialNormalLookup &normal_at, const MaterialFactorLookup &factors_at);
// Copy-on-result preserves unvisited bytes and leaves caller buffers untouched
// on failure. Alternate writes skip factors and preserve all but material+40.
MaterialStreams write_material_tile(const MaterialGeometryDomain &domain, MaterialTile tile,
                                    const MaterialStreams &prior, const MaterialSelection &selection,
                                    int alternate, const MaterialLookup &height_at,
                                    const MaterialNormalLookup &normal_at,
                                    const MaterialFactorLookup &factors_at);
std::vector<MaterialTile> material_tiles(int columns, int rows);
struct MaterialPatch {
    int columns, rows;
    MaterialStreams streams;
    std::vector<std::uint16_t> indices;
    MaterialPatchMesh mesh() const { return {columns, rows, streams.material48, indices}; }
};
// Serial disjoint 12x12 callbacks in original order, not a port of the OS scheduler.
MaterialPatch build_material_patch(const MaterialGeometryDomain &domain, const MaterialSelection &selection,
                                   const MaterialLookup &height_at, const MaterialNormalLookup &normal_at,
                                   const MaterialFactorLookup &factors_at);

inline constexpr int material_normal_side = 69;
using MaterialScoreWords = std::array<std::uint32_t, 128>;
std::vector<MaterialVector3> material_normal_field(MaterialOrigin origin, const MaterialSourceDomain &source,
                                                   const MaterialLookup &height_at,
                                                   const MaterialNormalSeed &seed);
MaterialScoreWords material_scores(int columns, int rows, MaterialOrigin origin, int width, int height,
                                   std::span<const std::int32_t, 64> slots,
                                   std::span<const std::int32_t, 64> modes,
                                   const MaterialNormalLookup &normal_at, const MaterialLookup &material_at);
struct MaterialScoreResult {
    std::vector<MaterialVector3> normals;
    MaterialScoreWords scores;
    MaterialSelection selected;
};
MaterialScoreResult
produce_material_scores(int columns, int rows, MaterialOrigin origin, const MaterialSourceDomain &source,
                        int source_count, std::span<const std::int32_t, 64> slots,
                        std::span<const std::int32_t, 64> modes, int cache_mode, int parent_flag,
                        std::span<const std::int32_t> material_slots,
                        std::span<const std::uint32_t> specular_handles, const MaterialLookup &height_at,
                        const MaterialLookup &material_at, const MaterialNormalSeed &seed);
} // namespace civ5::materials
