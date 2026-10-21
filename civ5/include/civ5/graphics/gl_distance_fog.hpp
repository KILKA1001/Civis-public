#pragma once
#include "civ5/materials/terrain_frame.hpp"
#include "gl_land_material.hpp"
#include "gl_texture.hpp"

namespace civ5::graphics {
// Shared authored DistanceFog resource for LAND pixel slot2 and water slot8.
// The immutable owner retains the original asset provenance and complete mip
// chain. Frame updates change constants without recreating the texture.
class GlDistanceFog {
  public:
    GlDistanceFog(const GlContext &context, const materials::DistanceFogAssets &assets);
    const materials::DistanceFogAssets &assets() const { return assets_; }
    const GlTexture &ramp() const { return ramp_; }
    materials::DistanceFogFrame frame(float normalized_zoom) const;
    materials::LandMaterialConstants land_constants(materials::LandMaterialConstants constants,
                                                    float normalized_zoom,
                                                    terrain::X87Precision precision) const;
    // Reupload the retained authored ramp on a different live context. The
    // numerical fog state remains unchanged and can be reused by the caller.
    void rebuild_context(std::shared_ptr<GlContext> context);

  private:
    materials::DistanceFogAssets assets_;
    GlTexture ramp_;
};
} // namespace civ5::graphics
