#pragma once
#include "civ5/terrain/land_grid.hpp"
#include <optional>

namespace civ5::materials {
enum class LandVertexInput { local_grid, source_positioned };
enum class LandPass { main, overlay };
enum class LandPixelProgram { cached_specular, cached_diffuse, overlay };
struct LandTechnique {
    unsigned selector;
    LandVertexInput input;
    LandPixelProgram pixel;
};
struct LandSelection {
    LandTechnique technique;
    unsigned packet_offset;
};
LandTechnique land_technique(unsigned selector);
// Original conditional selection, not inferred current DX9 quality/state.
std::optional<LandSelection> select_land_technique(LandPass pass, int displacement_mode, int extra_mode,
                                                   int patch_state, int specular_override);
class LandMaterialMesh {
  public:
    LandMaterialMesh(LandVertexInput input, std::span<const terrain::LandGridVertex> vertices,
                     std::span<const std::uint16_t> indices);
    LandVertexInput input() const { return input_; }
    std::span<const terrain::LandGridVertex> vertices() const { return vertices_; }
    std::span<const std::uint16_t> indices() const { return indices_; }

  private:
    LandVertexInput input_;
    std::vector<terrain::LandGridVertex> vertices_;
    std::vector<std::uint16_t> indices_;
};
using LandVector = std::array<float, 4>;
// Four literal shader constant vectors in original register order. No pointer ABI.
using LandMatrix = std::array<LandVector, 4>;
struct LandMaterialConstants {
    LandMatrix view_projection{}, composite_projection{}, world{}, shadow_projection{};
    LandVector fow_scale{}, world_scales{}, height_texture_scale{}, offset{}, camera{}, fog_color{}, fog{};
    // Original PS CMP uses sign versus zero, not C++ bool/nonzero conversion.
    float shadow_fade = 0, apply_specular = 0, apply_diffuse = 0;
    void validate() const;
};
} // namespace civ5::materials
