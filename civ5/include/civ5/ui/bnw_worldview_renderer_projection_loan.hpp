#pragma once
#include "civ5/ui/detail/bnw_label_system_font_request.hpp"
#include "civ5/graphics/detail/bnw_label_system_font_atlas_metadata.hpp"
#include "civ5/graphics/detail/bnw_font_texture_observation.hpp"
#include "civ5/ui/detail/bnw_font_texture_request.hpp"
#include "civ5/runtime/detail/bnw_font_resource_loan.hpp"
#include "civ5/ui/bnw_owned_camera_gui_screen.hpp"
#include "civ5/terrain/camera_frame.hpp"
#include "civ5/terrain/camera_controller.hpp"
#include <memory>
#include <atomic>
#include <optional>
#include <thread>
#include <functional>
namespace civ5::graphics {class GlWaterWorldRenderer;class BnwClearHexHighlightOwner;}
namespace civ5::terrain {class TerrainWorld;}
namespace civ5::runtime {class BnwWorldViewSourceOperationLoan;class BnwWorldViewNativeContextIssuer;class BnwSelectedStockLuaBootstrap;}
namespace civ5::validation {template<class LandRenderer>struct CameraPreviewLoop;}
namespace civ5::ui::detail {class BnwLabelProcessModuleOwner;class BnwOriginalNonFontDeviceInitializer;}
namespace civ5::ui {
class UiScreenGlRenderer;class BnwClientSceneFrame;struct BnwClientFrame;class BnwWorldViewControlTree;
class BnwWorldViewRendererProjectionLoan;
class BnwWorldViewGridToWorldLoan;
// Declared AFTER the real water/controller locals by CameraPreviewLoop, hence
// retired BEFORE either borrowed producer is destroyed. No public constructor.
class BnwWorldViewRendererRunLoan final {
 private:
    friend class ::civ5::graphics::BnwClearHexHighlightOwner;
    friend class UiScreenGlRenderer;
    friend class BnwWorldViewRendererProjectionLoan;
    friend class BnwWorldViewGridToWorldLoan;
    friend class BnwNotificationTerrainSceneLoan;
    template<class LandRenderer>friend struct ::civ5::validation::CameraPreviewLoop;
    struct RendererLifetime {
        const std::thread::id creator=std::this_thread::get_id();
        const UiScreenGlRenderer* renderer=nullptr;
        std::atomic<bool> alive{true},failed{false},active{false};
        const void* active_scope=nullptr;
    };
    struct State {
        const std::thread::id creator=std::this_thread::get_id();
        const UiScreenGlRenderer* const renderer;
        const graphics::GlWaterWorldRenderer* const water;
        const terrain::CameraControllerState* const controller;
        const std::weak_ptr<RendererLifetime> renderer_life;
        const std::weak_ptr<const terrain::TerrainWorld> terrain_owner;
        const std::weak_ptr<BnwClientSceneFrame> scene_owner;
        const std::array<std::uint8_t,16> map_guid;
        const std::uint64_t map_width,map_height;
        std::atomic<bool> alive{true},failed{false};
        State(const UiScreenGlRenderer*,const graphics::GlWaterWorldRenderer*,
            const terrain::CameraControllerState*,const std::shared_ptr<RendererLifetime>&,
            const std::shared_ptr<const terrain::TerrainWorld>&,
            const std::shared_ptr<BnwClientSceneFrame>&);
        void require_current(const UiScreenGlRenderer*)const;
    };
 public:
    class Binding final {
        friend class BnwWorldViewRendererRunLoan;
        friend class BnwWorldViewRendererProjectionLoan;
        const std::weak_ptr<State> state_;
        explicit Binding(const std::shared_ptr<State>& state):state_(state){}
        std::shared_ptr<State> current_for(const UiScreenGlRenderer*)const;
     public:
        Binding()=delete;
        Binding(const Binding&)=default;
        Binding& operator=(const Binding&)=delete;
    };
    BnwWorldViewRendererRunLoan(const BnwWorldViewRendererRunLoan&)=delete;
    BnwWorldViewRendererRunLoan& operator=(const BnwWorldViewRendererRunLoan&)=delete;
    ~BnwWorldViewRendererRunLoan();
 private:
    const std::shared_ptr<State> state_;
    BnwWorldViewRendererRunLoan(UiScreenGlRenderer&,const graphics::GlWaterWorldRenderer&,
        const terrain::CameraControllerState&,
        const std::shared_ptr<const terrain::TerrainWorld>&,
        const std::shared_ptr<BnwClientSceneFrame>&);
    Binding bind_owned()const;
};
// A copied matrix/screen point cannot mint this one-paint creator-stack loan.
class BnwWorldViewRendererProjectionLoan final {
 public:
    BnwWorldViewRendererProjectionLoan(const BnwWorldViewRendererProjectionLoan&)=delete;
    BnwWorldViewRendererProjectionLoan& operator=(const BnwWorldViewRendererProjectionLoan&)=delete;
    ~BnwWorldViewRendererProjectionLoan();
 private:
    friend class UiScreenGlRenderer;
    friend class detail::BnwOriginalNonFontDeviceInitializer;
    friend class BnwWorldViewGridToWorldLoan;
    friend class BnwNotificationTerrainSceneLoan;
    template<class LandRenderer>friend struct ::civ5::validation::CameraPreviewLoop;
    using Lifetime=BnwWorldViewRendererRunLoan::RendererLifetime;
    const std::shared_ptr<BnwWorldViewRendererRunLoan::State> run_;
    const UiScreenGlRenderer* const renderer_;
    const std::shared_ptr<Lifetime> lifetime_;
    const std::weak_ptr<BnwClientSceneFrame> scene_;
    const std::shared_ptr<const BnwClientFrame> frame_;
    const terrain::CameraFrameState camera_;
    const std::int32_t width_,height_;
    const int unwind_;
    BnwWorldViewRendererProjectionLoan(UiScreenGlRenderer&,
        const BnwWorldViewRendererRunLoan::Binding&,
        const std::shared_ptr<BnwClientSceneFrame>&,std::int32_t framebuffer_width,
        std::int32_t framebuffer_height);
    static terrain::CameraFrameState capture_current_camera(
        const std::shared_ptr<BnwWorldViewRendererRunLoan::State>&,const UiScreenGlRenderer*);
    void require_current_without_callbacks()const;
    // Behind-camera/off-frustum points remain absent, never default zero.
    std::optional<std::array<double,2>> project(const std::array<double,3>&)const;
};
// Only the actual renderer can lend this provider across its synchronous fresh
// Native scope. A source bank or copied vector cannot manufacture grid authority.
class BnwWorldViewGridToWorldLoan {
 public:
    BnwWorldViewGridToWorldLoan(const BnwWorldViewGridToWorldLoan&)=delete;
    BnwWorldViewGridToWorldLoan& operator=(const BnwWorldViewGridToWorldLoan&)=delete;
    virtual ~BnwWorldViewGridToWorldLoan()=default;
    virtual void require_for_source(const runtime::BnwWorldViewSourceOperationLoan&)const=0;
    virtual std::array<double,3> world_xyz(const runtime::BnwWorldViewSourceOperationLoan&,
        std::int32_t map_x,std::int32_t map_y)const=0;
 protected:
    // Defined by the runtime NativeLoan owner, never a validation free symbol.
    static void require_scene_for_source(const runtime::BnwWorldViewSourceOperationLoan&,
        const std::weak_ptr<BnwClientSceneFrame>&);
 private:
    friend class UiScreenGlRenderer;
    friend class ::civ5::runtime::BnwWorldViewNativeContextIssuer;
    friend class ::civ5::runtime::BnwSelectedStockLuaBootstrap;
    friend class ::civ5::runtime::BnwWorldViewSourceOperationLoan;
    virtual std::shared_ptr<detail::BnwLabelProcessModuleOwner> label_module_for_source_owned(
        const runtime::BnwWorldViewSourceOperationLoan&)const=0;
    virtual std::unique_ptr<graphics::detail::BnwFontTextureObservation> query_font_texture_owned(
        const runtime::BnwWorldViewSourceOperationLoan&,
        const BnwOwnedCameraGuiScreen::Loan&,
        const graphics::BnwOwnedCameraRendererWindowLease::Loan&,
        const runtime::detail::BnwFontResourceLoan&,
        const detail::BnwFontTextureRequest&)const=0;
    virtual std::shared_ptr<const graphics::detail::BnwLabelSystemFontAtlasMetadata> query_system_font_atlas_owned(
        const runtime::BnwWorldViewSourceOperationLoan&,
        const BnwOwnedCameraGuiScreen::Loan&,
        const graphics::BnwOwnedCameraRendererWindowLease::Loan&,
        const detail::BnwLabelSystemFontRequest::Guard&)const=0;
    //Closed PRE operation after Source publication, before any Context Tree/Lua.
    virtual void prepare_clear_hex_highlights_owned(
        const runtime::BnwWorldViewSourceOperationLoan&,const BnwOwnedCameraGuiScreen::Loan&,
        const graphics::BnwOwnedCameraRendererWindowLease::Loan&)const=0;
    virtual std::shared_ptr<graphics::BnwClearHexHighlightOwner> clear_hex_highlight_owner_owned(
        const runtime::BnwWorldViewSourceOperationLoan&)const=0;
    // Genuine renderer lender is already inside Source/GUI/window/Scene scope.
    virtual void prepare_notification_cpu_geometry_owned(
        const runtime::BnwWorldViewSourceOperationLoan&,
        const BnwOwnedCameraGuiScreen::Loan&,
        const graphics::BnwOwnedCameraRendererWindowLease::Loan&,
        const std::array<std::shared_ptr<BnwWorldViewControlTree>,5>&)const=0;
    // Only the actual initial Bootstrap entry may enclose its protected Lua
    // callback in a fresh SAME renderer resource scope before immutable paint.
    virtual void with_notification_initial_lua_resources_owned(
        const runtime::BnwWorldViewSourceOperationLoan&,
        const BnwOwnedCameraGuiScreen::Loan&,
        const graphics::BnwOwnedCameraRendererWindowLease::Loan&,
        const std::array<std::shared_ptr<BnwWorldViewControlTree>,5>&,
        const std::function<void()>&)const=0;
    BnwWorldViewGridToWorldLoan()=default;
};
}
