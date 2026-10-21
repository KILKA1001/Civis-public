#include "civ5/graphics/gl_river_world.hpp"
#include <map>
#include <stdexcept>

namespace civ5::graphics {
struct GlRiverWorldRenderer::Impl {
    std::shared_ptr<GlContext> context;
    std::shared_ptr<const terrain::TerrainWorld> world;
    RiverWorldSettings settings;
    terrain::RiverSurfaceMesh mesh;
    std::map<unsigned, GlTexture> textures;
    std::unique_ptr<GlRiverSurfaceRenderer> renderer;
    Impl(std::shared_ptr<GlContext> c, std::shared_ptr<const terrain::TerrainWorld> source,
         const RiverWorldAssets &assets, const RiverWorldSettings &settings)
        : context(std::move(c)), world(std::move(source)), settings(settings) {
        for (const auto &map : assets.maps)
            if (map.face_count() != 1)
                throw std::invalid_argument("River authored maps must be two-dimensional");
        const auto &p = world->owner().parameters;
        const terrain::RiverSurfaceSettings surface{{p.center_x, p.center_y, p.center_z},
                                                    p.scale_z,
                                                    settings.high_quality_geometry,
                                                    settings.precision};
        const auto &rivers = world->rivers();
        if (rivers) {
            if (!rivers->generated)
                throw std::invalid_argument("River surface requires source graph and sampled heights");
            const auto &g = *rivers->generated;
            mesh = terrain::build_river_surface(g.centerlines, g.bank_lines, g.sampled_heights, surface);
        } else {
            // Validate settings even for a world without rivers.
            mesh = terrain::build_river_surface({}, {}, {}, surface);
        }
        for (unsigned slot = 0; slot < 4; ++slot)
            textures.emplace(
                slot, GlTexture::lean(*context, assets.lean[slot / 2],
                                      slot % 2 ? assets::LeanMap::luminance16 : assets::LeanMap::rgba16));
        for (unsigned i = 0; i < 3; ++i)
            textures.emplace(i == 2 ? 8 : 5 + i,
                             GlTexture(*context, assets.maps[i],
                                       i == 1 ? DdsSamplingColor::srgb : DdsSamplingColor::linear));
        const auto selector = settings.pixel_quality == 1 || settings.pixel_quality == 2 ? 4u : 1u;
        renderer = std::make_unique<GlRiverSurfaceRenderer>(context, mesh, selector);
    }
};
GlRiverWorldRenderer::GlRiverWorldRenderer(std::shared_ptr<GlContext> context,
                                           std::shared_ptr<const terrain::TerrainWorld> world,
                                           const RiverWorldAssets &assets, RiverWorldSettings settings) {
    if (!context || !world)
        throw std::invalid_argument("River renderer requires an owned world and context");
    impl_ = std::make_unique<Impl>(std::move(context), std::move(world), assets, settings);
}
GlRiverWorldRenderer::~GlRiverWorldRenderer() = default;
const terrain::RiverSurfaceMesh &GlRiverWorldRenderer::mesh() const { return impl_->mesh; }
const GlTexture &GlRiverWorldRenderer::texture(unsigned slot) const {
    const auto found = impl_->textures.find(slot);
    if (found == impl_->textures.end())
        throw std::invalid_argument("River texture is supplied by the scene");
    return found->second;
}
void GlRiverWorldRenderer::draw(const materials::WaterConstantBlock &constants, GlRiverSurfaceInputs inputs,
                                int width, int height) {
    for (const auto &[slot, texture] : impl_->textures)
        inputs.pixel[slot] = texture.id();
    impl_->renderer->draw(constants, inputs, width, height);
}
void GlRiverWorldRenderer::rebuild_context(std::shared_ptr<GlContext> context, const RiverWorldAssets &assets) {
    if (!context)
        throw std::invalid_argument("River context recovery needs a live replacement context");
    if (context == impl_->context)
        throw std::invalid_argument("River context recovery requires a different OpenGL context");
    auto replacement = std::make_unique<Impl>(context, impl_->world, assets, impl_->settings);
    impl_.swap(replacement);
}
} // namespace civ5::graphics
