#pragma once

#include "civ5/assets/resource_mounts.hpp"
#include "civ5/content/rules_database.hpp"
#include "civ5/graphics/gl_bnw_world_anchor_scene.hpp"
#include "civ5/graphics/gl_texture.hpp"
#include "civ5/terrain/camera_frame.hpp"
#include "civ5/terrain/height_owner.hpp"
#include "civ5/terrain/world_layout.hpp"
#include "civ5/ui/screen_compositor.hpp"
#include <map>
#include <functional>
#include <optional>
#include <span>

namespace civ5::graphics {

struct GlBnwNaturalWonderFrame {
    ui::ComposedUiFrame overlay;
    std::vector<std::uint64_t> projected_plots;
    std::uint64_t revision=0;
};

// The selected BNW GenericWorldAnchor.lua NaturalWonder instance is a 64x64
// image nested in a WorldAnchor. This projects its hex position using the
// current live camera; native UI world-anchor elevation remains a separate
// pixel-parity question. Offscreen instances stay retained, not fabricated.
std::optional<ui::WorldViewPoint> project_bnw_natural_wonder_anchor(
    const terrain::WorldLayout &layout,
    const terrain::HeightOwnerParameters &height,
    const terrain::CameraFrameState &camera,
    std::int32_t plot_x,std::int32_t plot_y,
    std::int32_t viewport_width,std::int32_t viewport_height);

// Concrete type-0 GameplayWorldAnchor backend. It resolves the saved feature
// ID through active BNW rules, the 64px IconTextureAtlas through active mounts,
// and retains each source instance with an uploaded GL texture until a
// post-commit removal. The live GL UI painter consumes compose_frame()
// separately before normal screen paint. Visibility and painting are not
// prerequisites for removing an offscreen retained source object.
class GlBnwNaturalWonderAnchorBackend final:public GlBnwWorldAnchorDrawBackend {
  public:
    GlBnwNaturalWonderAnchorBackend(
        const runtime::LiveSessionState &state,
        std::shared_ptr<GlContext> context,
        const content::RulesDatabase &rules,
        const content::InfoTypeCatalog &types,
        const assets::ResourceMountTable &resources);
    const GlContext *context() const noexcept override {return context_.get();}
    runtime::BnwWorldAnchorGraphicsIdentity world_identity() const override {
        return identity_;
    }
    bool has_drawable(std::uint64_t plot,std::int32_t anchor,
                      std::int32_t x,std::int32_t y) const override;
    void remove_drawable(std::uint64_t plot,std::int32_t anchor,
                         std::int32_t x,std::int32_t y,
                         std::int32_t event_data) override;
    GlBnwNaturalWonderFrame compose_frame(
        const terrain::WorldLayout &layout,
        const terrain::HeightOwnerParameters &height,
        const terrain::CameraFrameState &camera,
        std::int32_t viewport_width,std::int32_t viewport_height) const;
    using ProjectPlot=std::function<std::optional<ui::WorldViewPoint>(
        std::int32_t,std::int32_t)>;
    GlBnwNaturalWonderFrame compose_frame(
        std::int32_t viewport_width,std::int32_t viewport_height,
        const ProjectPlot &project) const;
    void confirm_paint(const GlBnwNaturalWonderFrame &frame,
                       std::size_t painted_textures,
                       std::span<const std::string> unresolved);
    std::size_t retained_count() const noexcept {return instances_.size();}

  private:
    struct Instance {
        std::int32_t x=0,y=0,feature=-1;
        std::string texture;
        ui::WorldViewRect crop;
        std::shared_ptr<GlTexture> resource;
    };
    runtime::BnwWorldAnchorGraphicsIdentity identity_;
    std::shared_ptr<GlContext> context_;
    std::map<std::uint64_t,Instance> instances_;
    std::uint64_t revision_=0;
};

} // namespace civ5::graphics
