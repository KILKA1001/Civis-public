#pragma once
#include "civis_frontend_party_material_storage.hpp"
#include "civ5/graphics/gl_fog_of_war_map.hpp"
#include "civ5/graphics/gl_material_world.hpp"
#include "civ5/ui/bnw_client_frame.hpp"
#include "civ5/graphics/gl_water_world.hpp"
#include <functional>

namespace civ5::validation {template<class T>struct CameraPreviewLoop;}

namespace civ5::ui::detail {
// OWN saved-world LAND/basic-water/FOW/depth stage. A diagnostic draw issues
// no installed native/UI Source or normal Camera-loop completion.
class CivisFrontendAlphaLandFrame final {
    friend class CivisFrontendPartyMaterialStorage::FrameInputsLoan;
    template<class T>friend struct ::civ5::validation::CameraPreviewLoop;
    struct State;
    const std::thread::id creator_=std::this_thread::get_id();
    const std::shared_ptr<State> state_;
    explicit CivisFrontendAlphaLandFrame(std::shared_ptr<State>);
    static std::shared_ptr<State> retain_frame_owned(const CivisFrontendAlphaLandFrame&);
    terrain::CameraControllerState initial_controller_for_camera_owned()const;
    std::pair<std::shared_ptr<graphics::GlTexture>,std::shared_ptr<graphics::GlTexture>> camera_targets_owned()const;
    assets::Bytes stage_camera_asset_owned(const assets::AssetKey&)const;
    void attach_camera_producer_owned(graphics::GlWaterWorldRenderer&)const;
    void require_camera_boundary_owned()const;
    void with_current_camera_frame_owned(const graphics::GlWaterWorldRenderer&,
        const terrain::CameraControllerState&,
        const std::function<void(const CivisFrontendPartyMaterialStorage::FrameInputsLoan&)>&)const;
    void draw_basic_water_owned(const CivisFrontendPartyMaterialStorage::FrameInputsLoan&)const;
    void apply_land_layers_owned(const CivisFrontendPartyMaterialStorage::FrameInputsLoan&,
        materials::LandMaterialConstants&,graphics::GlLandMaterialInputs&)const;
public:
    static std::unique_ptr<CivisFrontendAlphaLandFrame> capture_owned(
        const CivisFrontendPartyMaterialStorage&,
        const std::shared_ptr<graphics::GlContext>& actual_context,
        std::uint32_t actual_framebuffer_width,std::uint32_t actual_framebuffer_height);
    ~CivisFrontendAlphaLandFrame();
    CivisFrontendAlphaLandFrame(const CivisFrontendAlphaLandFrame&)=delete;
    CivisFrontendAlphaLandFrame& operator=(const CivisFrontendAlphaLandFrame&)=delete;
    // Synchronous actual draw/FOW/readback. Its result cannot be mapped to
    // UiCommandStatus::performed, K003, gameplay readiness or pixel parity.
    void render_first_owned(const CivisFrontendPartyMaterialStorage&,
        const FrontendSelectedSaveInput&,const std::filesystem::path& actual_output_directory);
    // Actual installed client frame is mandatory for canonical picking. The
    // diagnostic stage never constructs a substitute BnwClientFrame/stamp.
    std::optional<std::int64_t> pick_owned(const ui::BnwClientFrame& actual_frame,
        std::int32_t framebuffer_x,std::int32_t framebuffer_y)const;
};
}
