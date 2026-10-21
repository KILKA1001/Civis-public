#pragma once
#include "civ5/ui/bnw_owned_camera_gui_screen.hpp"
#include "civ5/ui/bnw_owned_notification_publication_owner.hpp"
#include <atomic>
#include <functional>
#include <memory>
#include <thread>
namespace civ5::runtime {class BnwOwnedGameplayEventPumpLoan;class BnwStockLuaAttemptLease;}
namespace civ5::ui {
struct BnwEngineUiNotificationEffect;
// Synchronous genuine GUI/Scene/model borrow only. No physical receipt issuer.
class BnwNotificationPublicationLender final {
 public:
  ~BnwNotificationPublicationLender();
  BnwNotificationPublicationLender(const BnwNotificationPublicationLender&)=delete;
  BnwNotificationPublicationLender& operator=(const BnwNotificationPublicationLender&)=delete;
  BnwNotificationPublicationLender(BnwNotificationPublicationLender&&)=delete;
  BnwNotificationPublicationLender& operator=(BnwNotificationPublicationLender&&)=delete;
 private:
  friend class BnwOwnedCameraGuiScreen;
  friend class BnwClientSceneFrame;
  friend class UiScreenGlRenderer;
  friend class BnwNotificationTerrainSceneLoan;
  friend class BnwNotificationAuthoredArtPresentLeaf;
  friend class BnwOwnedNotificationMinimapReceiver;
  friend class BnwOwnedNotificationPlotFxReceiver;
  static std::unique_ptr<BnwNotificationOwnerModelLoan> capture_model_owned(
      const BnwOwnedNotificationPublicationOwner&,const LiveUiFrame&,
      const runtime::BnwOwnedGameplayEventPumpLoan&,
      const runtime::BnwStockLuaAttemptLease&);
  static void quarantine_captured_model_without_callbacks(
      const std::unique_ptr<BnwNotificationOwnerModelLoan>&)noexcept;
  struct Storage;
  const std::shared_ptr<Storage> storage_;
  const std::thread::id creator_=std::this_thread::get_id();
  explicit BnwNotificationPublicationLender(
      std::unique_ptr<BnwNotificationOwnerModelLoan>&,
      const BnwOwnedCameraGuiScreen&,const BnwOwnedCameraGuiScreen::Loan&,
      const graphics::BnwOwnedCameraRendererWindowLease::Loan&,
      const BnwClientSceneFrame&,const BnwClientFrame&,
      const LiveUiFrame&,const ComposedUiFrame&,
      const runtime::BnwOwnedGameplayEventPumpLoan&,
      const runtime::BnwStockLuaAttemptLease&,
      std::weak_ptr<const BnwOwnedCameraGuiScreen>,
      std::weak_ptr<const BnwClientSceneFrame>,
      std::weak_ptr<const graphics::BnwOwnedCameraRendererWindowLease>);
  static void pure(const std::shared_ptr<Storage>&);
  static void full(const std::shared_ptr<Storage>&);
  static BnwEngineUiNotificationEffect pending_effect_from_storage(const std::shared_ptr<Storage>&);
  static void fail_from_storage_without_callbacks(const std::shared_ptr<Storage>&)noexcept;
  static void quarantine_from_storage_without_callbacks(const std::shared_ptr<Storage>&)noexcept;
  static std::weak_ptr<const BnwClientSceneFrame> scene_from_storage_without_callbacks(const std::shared_ptr<Storage>&);
  static void require_minimap_scene_from_storage_without_callbacks(
      const std::shared_ptr<Storage>&,const std::weak_ptr<const BnwClientSceneFrame>&);
  void invoke_owned(const std::function<void(BnwNotificationPublicationLender&)>&);
  void require_origin_without_callbacks()const;
  void require_current()const;
  BnwEngineUiNotificationEffect pending_native_effect()const;
  std::weak_ptr<const BnwClientSceneFrame> actual_scene_without_callbacks()const;
  void require_same_minimap_scene_without_callbacks(const std::weak_ptr<const BnwClientSceneFrame>&)const;
  [[noreturn]] void stop_unimplemented_native_effect_owned()const;
};
// No return constructors or accepting/checking return methods are supplied:
// actual NULLFX/minimap physical store, authored typed draw and afterSwap
// dependencies have not earned any immutable return storage yet.
class BnwNotificationNullablePlotFxReturn;
class BnwNotificationMinimapReturn;
class BnwNotificationAuthoredDrawReturn;
class BnwNotificationAfterSwapReturn;
}
