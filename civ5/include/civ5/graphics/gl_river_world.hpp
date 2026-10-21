#pragma once
#include "gl_river_surface.hpp"
#include "gl_texture.hpp"
#include "civ5/terrain/terrain_world.hpp"

namespace civ5::graphics {
struct RiverWorldAssets {
    // riversmooth16 / riverrough_new16; alpha / diffuse / waterdepthcolor.
    std::array<assets::LeanData, 2> lean;
    std::array<assets::DdsData, 3> maps;
};
struct RiverWorldSettings {
    // Original D80DC0 geometry detail and D80DB8 pixel quality are distinct.
    bool high_quality_geometry = true;
    std::uint32_t pixel_quality = 2;
    terrain::X87Precision precision = terrain::X87Precision::double_precision;
};
// Builds the surface from this world's retained original graph/height results.
// Owns geometry and authored textures; reflection, overlay, FOW and ColorKey
// textures remain live caller inputs. Use the shared water frame constants.
class GlRiverWorldRenderer {
  public:
    GlRiverWorldRenderer(std::shared_ptr<GlContext>, std::shared_ptr<const terrain::TerrainWorld>,
                         const RiverWorldAssets &, RiverWorldSettings = {});
    ~GlRiverWorldRenderer();
    GlRiverWorldRenderer(const GlRiverWorldRenderer &) = delete;
    GlRiverWorldRenderer &operator=(const GlRiverWorldRenderer &) = delete;
    const terrain::RiverSurfaceMesh &mesh() const;
    const GlTexture &texture(unsigned slot) const;
    void draw(const materials::WaterConstantBlock &, GlRiverSurfaceInputs, int width, int height);
    // Recreate authored river textures and the draw pipeline on a different
    // live context. The immutable terrain graph and generated mesh are rebuilt
    // from the retained source world; caller-owned scene textures are external.
    // The old context must remain usable for resource release on this thread.
    void rebuild_context(std::shared_ptr<GlContext> context, const RiverWorldAssets &assets);

  private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
} // namespace civ5::graphics
