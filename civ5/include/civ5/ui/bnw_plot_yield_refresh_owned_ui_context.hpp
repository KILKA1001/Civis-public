#pragma once
#include "civ5/ui/bnw_owned_camera_ui_source_input.hpp"
#include "civ5/ui/bnw_worldview_title_font_issuer.hpp"
#include "civ5/ui/bnw_plot_yield_refresh_ui_adapter.hpp"
#include "civ5/ui/bnw_client_scene_frame.hpp"
#include "civ5/ui/live_city_banner_projection.hpp"
#include "civ5/ui/economic_overview_scene.hpp"
#include "civ5/ui/screen_compositor.hpp"
#include <atomic>
namespace civ5::ui {
class BnwPlotYieldRefreshOwnedUiStorage;
class BnwTechTreeLifecycle;
class BnwPlotYieldRefreshOwnedUiContext final:
 public std::enable_shared_from_this<BnwPlotYieldRefreshOwnedUiContext> {
 public:
  static std::shared_ptr<BnwPlotYieldRefreshOwnedUiContext> create(
      std::shared_ptr<BnwOwnedCameraUiSourceInput>,
      std::shared_ptr<BnwWorldViewFontSourceScope>,
      const graphics::BnwRendererSceneReceipt&);
  ~BnwPlotYieldRefreshOwnedUiContext();
  BnwPlotYieldRefreshOwnedUiContext(const BnwPlotYieldRefreshOwnedUiContext&)=delete;
  BnwPlotYieldRefreshOwnedUiContext&operator=(const BnwPlotYieldRefreshOwnedUiContext&)=delete;
  void require_current()const;
  void retire()noexcept;
  std::shared_ptr<const BnwClientFrame> begin_client(ClientViewport,terrain::CameraControllerState);
  LiveUiFrame begin_ui(std::int32_t,std::int32_t);
  UiCommandResult dispatch(const UiCommand&,const LiveUiFrame&);
  std::shared_ptr<BnwPlotYieldRefreshUiAdapter> yield_adapter();
  // The actual renderer supplies owned value projection state before capture.
  // This does not mint a GPU/viewport receipt or execute a borrowed water callback.
  void update_city_projection(terrain::WorldLayout,terrain::HeightOwnerParameters,
      terrain::CameraFrameState,std::int32_t,std::int32_t);
  const std::shared_ptr<BnwClientSceneFrame>& scene()const;
  const std::shared_ptr<LiveUiHost>& host()const;
  UiScreenController& screens()const;
  EconomicOverviewScene* economic()const;
  UiScreenCompositor& compositor()const;
  std::shared_ptr<BnwTechTreeLifecycle> tech_tree()const;
  const std::vector<LiveCityBannerProjectionAudit>& city_projection_audit()const;
  const std::shared_ptr<BnwWorldViewFontSourceScope>& font_source()const;
  const std::shared_ptr<BnwOwnedCameraUiSourceInput>& source_input()const;
  const std::string& popup_blocker()const;
  static constexpr bool native_app_ready=false,native_vm_ready=false,full_city_refresh_ready=false;
 private:
  BnwPlotYieldRefreshOwnedUiContext()=default;
  friend class BnwPlotYieldRefreshUiAdapter;
  friend class UiScreenGlRenderer;
  friend class LiveUiHost;
  friend class BnwClientFrameOwner;
  friend class BnwClientSceneFrame;
  static std::shared_ptr<BnwClientSceneFrame> retain_owned_scene(
      const std::shared_ptr<BnwPlotYieldRefreshOwnedUiStorage>&,const BnwClientFrameOwner*);
  static std::shared_ptr<LiveUiHost> retain_owned_host(
      const std::shared_ptr<BnwPlotYieldRefreshOwnedUiStorage>&,const LiveUiHost*);
  static void require_storage_current(const std::shared_ptr<BnwPlotYieldRefreshOwnedUiStorage>&);
  static void require_storage_finality(const std::shared_ptr<BnwPlotYieldRefreshOwnedUiStorage>&);
  // Concrete private RAII scope shares the existing ContextOperation guard.
  // The pin retains source storage only; never the Context facade.
  static std::shared_ptr<void> retain_adapter_operation(const std::shared_ptr<BnwPlotYieldRefreshOwnedUiStorage>&);
  void build();
  void require_source_finality()const;
  void require_frame_finality(const LiveUiFrame&,const std::shared_ptr<const BnwClientFrame>&)const;
  std::shared_ptr<BnwPlotYieldRefreshOwnedUiStorage> storage_;
  std::shared_ptr<BnwClientSceneFrame> scene_;
  std::shared_ptr<LiveUiHost> host_;
  std::weak_ptr<BnwPlotYieldRefreshUiAdapter> adapter_;
  bool retirement_finalized_=false;
};
}
