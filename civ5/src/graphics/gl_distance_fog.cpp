#include "civ5/graphics/gl_distance_fog.hpp"
#include <stdexcept>
#include <utility>

namespace civ5::graphics {
GlDistanceFog::GlDistanceFog(const GlContext &context, const materials::DistanceFogAssets &assets)
    : assets_(assets), ramp_(context, assets_.ramp()) {}
materials::DistanceFogFrame GlDistanceFog::frame(float normalized_zoom) const {
    return materials::distance_fog_frame(assets_.state(), normalized_zoom, {});
}
materials::LandMaterialConstants GlDistanceFog::land_constants(materials::LandMaterialConstants constants,
                                                               float normalized_zoom,
                                                               terrain::X87Precision precision) const {
    const auto fog = materials::terrain_fog_constants(frame(normalized_zoom), constants.camera[2], precision);
    constants.fog_color = fog[0];
    constants.fog = fog[1];
    return constants;
}
void GlDistanceFog::rebuild_context(std::shared_ptr<GlContext> context) {
    if (!context)
        throw std::invalid_argument("Distance fog replacement requires an owned context");
    if (!ramp_.id())
        throw std::invalid_argument("Moved-from distance fog owner");
    if (ramp_.belongs_to(*context))
        throw std::invalid_argument("Distance fog replacement requires a different context");
    GlTexture replacement(*context, assets_.ramp());
    ramp_ = std::move(replacement);
}
} // namespace civ5::graphics
