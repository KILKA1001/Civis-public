#include "civ5/graphics/gl_lighting_cubes.hpp"
#include <stdexcept>

namespace civ5::graphics {
GlLightingCubes::GlLightingCubes(const GlContext &context, const materials::LightingCubeAssets &assets)
    : assets_(std::make_shared<const materials::LightingCubeAssets>(assets)) {
    if (assets.cubes().size() != 10)
        throw std::invalid_argument("Ten lighting cube sources required");
    for (const auto &cube : assets.cubes())
        textures_.emplace_back(context, cube.data, DdsSamplingColor::srgb);
}
void GlLightingCubes::rebuild_context(std::shared_ptr<GlContext> context) {
    if (!context)
        throw std::invalid_argument("Lighting cube replacement requires an owned context");
    if (!assets_ || textures_.empty())
        throw std::invalid_argument("Moved-from lighting cube owner");
    if (textures_.front().belongs_to(*context))
        throw std::invalid_argument("Lighting cube replacement requires a different context");
    GlLightingCubes replacement(*context, *assets_);
    textures_.swap(replacement.textures_);
}
const GlTexture &GlLightingCubes::sunlight(unsigned style) const {
    if (style >= 5)
        throw std::out_of_range("Lighting style must be 0..4");
    return textures_.at(style * 2);
}
const GlTexture &GlLightingCubes::shadowlight(unsigned style) const {
    if (style >= 5)
        throw std::out_of_range("Lighting style must be 0..4");
    return textures_.at(style * 2 + 1);
}
} // namespace civ5::graphics
