#pragma once
#include "civ5/assets/dds.hpp"
#include <functional>

namespace civ5::materials {
// Loader route, NOT a DDS dimension: texture_data can also load cubes/volumes.
enum class LightingLoader { texture_data, cube };
struct LightingSource {
    std::string native_path;
    LightingLoader loader;
};
const std::array<LightingSource, 11> &lighting_requests();
struct LightingHandles {
    std::uint32_t spec_screen;
    std::array<std::uint32_t, 5> sunlight, shadowlight;
    std::array<std::uint32_t, 10> renderer_words() const;
};
// Full resource-handle bits (including zero), never host pointers. Resolver owns
// source precedence/loading/failure policy. Calls remain interleaved Sun/Shadow.
LightingHandles resolve_lighting_handles(const std::function<std::uint32_t(const LightingSource &)> &load);

struct LightingCubeAsset {
    LightingSource source;
    assets::AssetHandle asset;
    assets::DdsData data;
};
class LightingCubeAssets {
  public:
    // Exact ten requests; no built-in base/DLC choice or missing-file fallback.
    static LightingCubeAssets load(const std::function<assets::AssetHandle(const LightingSource &)> &resolve);
    const LightingCubeAsset &sunlight(unsigned style) const;
    const LightingCubeAsset &shadowlight(unsigned style) const;
    const std::vector<LightingCubeAsset> &cubes() const { return cubes_; }

  private:
    std::vector<LightingCubeAsset> cubes_;
};

struct MaterialLightingResources {
    std::uint32_t mask, normal, albedo, specular, shadow;
    std::array<std::uint32_t, 5> sunlight, shadowlight, color_correct, land_color_key;
    std::uint32_t global_texture9, parent_texture11, world_height;
};
struct MaterialLightingBindings {
    unsigned style;
    std::array<std::uint16_t, 12> pixel_textures, pixel_samplers;
    std::uint16_t vertex_texture, vertex_sampler;
};
// Original key4B selection and low16 stores. ColorCorrect, ColorKey, environment
// cubes and the specscreen loader are distinct; no fabricated source aliases.
unsigned material_lighting_style(std::uint32_t mask);
MaterialLightingBindings material_lighting_bindings(const MaterialLightingResources &resources);
} // namespace civ5::materials
