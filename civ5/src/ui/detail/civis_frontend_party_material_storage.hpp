#pragma once
#include "civis_frontend_party_world_source.hpp"
#include "civis_saved_world_loading_owner.hpp"
#include "civis_renderer_loading_return.hpp"
#include "frontend_party_authored_lighting.hpp"
#include "civ5/materials/material_world.hpp"
#include "civ5/terrain/camera_controller.hpp"
#include "civ5/graphics/gl_texture.hpp"
#include "civ5/graphics/gl_material_world.hpp"
#include "civ5/materials/fog_of_war.hpp"

namespace material_preview_settings {struct Profile;}

namespace civ5::ui::detail {
class CivisFrontendAlphaLandFrame;
// Own renderer generation. This is not an Original loading occurrence or a
// gameplay/Frontend completion. The selected source remains its live issuer.
class CivisFrontendPartyMaterialStorage final {
    friend class CivisFrontendAlphaLandFrame;
    struct State;
    struct FrameControl;
    const std::thread::id creator_=std::this_thread::get_id();
    const std::shared_ptr<State> state_;
    explicit CivisFrontendPartyMaterialStorage(std::shared_ptr<State>);
    std::shared_ptr<State> retain_current_owned()const;
public:
    class FrameInputsLoan final {
        friend class CivisFrontendAlphaLandFrame;
        const std::thread::id creator_=std::this_thread::get_id();
        const std::shared_ptr<State> state_;
        const std::shared_ptr<FrameControl> frame_control_;
        const std::uint64_t generation_;
        bool completed_=false;
        FrameInputsLoan(const CivisFrontendPartyMaterialStorage&,const CivisFrontendAlphaLandFrame&);
        FrameInputsLoan(std::shared_ptr<State>,std::shared_ptr<FrameControl>);
        void first()const;
        void finish_owned();
    public:
        ~FrameInputsLoan();
        FrameInputsLoan(const FrameInputsLoan&)=delete;
        FrameInputsLoan& operator=(const FrameInputsLoan&)=delete;
        FrameInputsLoan(FrameInputsLoan&&)=delete;
        // Pure authenticated reads while the actual GL target is bound.
        void require_current()const;
        // File/Bank/window callbacks: use only outside raster/target scopes.
        void require_boundary()const;
        const std::shared_ptr<const materials::MaterialWorld>& world_owned()const;
        const terrain::WorldLayout& layout_owned()const;
        const terrain::CameraFrameState& camera_owned()const;
        const terrain::CameraControllerState& controller_owned()const;
        std::weak_ptr<graphics::GlContext> context_control_owned()const;
        const graphics::GlTexture& color_owned()const;
        const graphics::GlTexture& depth_owned()const;
        const CivisSavedWorldLoadingOwner::RendererLoadingReturn& loading_owned()const;
        assets::AssetHandle stage_asset_owned(const assets::AssetKey&)const;
        std::uint32_t width_owned()const;
        std::uint32_t height_owned()const;
        void require_same_world_owned(const materials::MaterialWorld&)const;
    };
    static std::unique_ptr<CivisFrontendPartyMaterialStorage> capture_owned(
        const CivisFrontendPartyWorldSource&,
        const std::shared_ptr<graphics::GlContext>& actual_context);
    ~CivisFrontendPartyMaterialStorage();
    CivisFrontendPartyMaterialStorage(const CivisFrontendPartyMaterialStorage&)=delete;
    CivisFrontendPartyMaterialStorage& operator=(const CivisFrontendPartyMaterialStorage&)=delete;
    void require_current()const;
    std::shared_ptr<const materials::MaterialWorld> world_owned()const;
    const FrontendPartyAuthoredLighting& authored_lighting_owned()const;
    const CivisSavedWorldLoadingOwner::RendererLoadingReturn& loading_owned()const;
    const material_preview_settings::Profile& profile_owned()const;
    const materials::FogOfWarAssets& fog_assets_owned()const;
    const materials::LightingCubeAssets& cubes_owned()const;
    graphics::GlMaterialWorldLighting lighting_owned()const;
    graphics::GlLandMaterialInputs land_inputs_owned()const;
    std::shared_ptr<const graphics::GlColorKeys> color_keys_owned()const;
    const graphics::GlDistanceFog* distance_fog_owned()const;
    void require_selected_owned(const FrontendSelectedSaveInput&,const graphics::GlContext&)const;
};
}
