#pragma once
#include "civ5/ui/bnw_notification_graphics_publication.hpp"
#include "civ5/ui/bnw_worldview_control_effects.hpp"
#include "civ5/runtime/bnw_worldview_source_operation_loan.hpp"
#include <functional>
#include <memory>
#include <thread>
namespace civ5::ui {
// A synchronous binding/checker, never a draw or SwapBuffers completion token.
class BnwNotificationAuthoredArtPresentLeaf final {
 public:
  ~BnwNotificationAuthoredArtPresentLeaf();
  BnwNotificationAuthoredArtPresentLeaf(const BnwNotificationAuthoredArtPresentLeaf&)=delete;
  BnwNotificationAuthoredArtPresentLeaf& operator=(const BnwNotificationAuthoredArtPresentLeaf&)=delete;
  BnwNotificationAuthoredArtPresentLeaf(BnwNotificationAuthoredArtPresentLeaf&&)=delete;
  BnwNotificationAuthoredArtPresentLeaf& operator=(BnwNotificationAuthoredArtPresentLeaf&&)=delete;
 private:
  friend class BnwOwnedCameraGuiScreen;
  friend class UiScreenGlRenderer;
  struct State;
  const std::thread::id creator_=std::this_thread::get_id();
  const std::shared_ptr<State> state_;
  explicit BnwNotificationAuthoredArtPresentLeaf(const BnwNotificationPublicationLender&);
  static std::shared_ptr<State> bind(const BnwNotificationPublicationLender&);
  static void pure(const std::shared_ptr<State>&);
  static void full(const std::shared_ptr<State>&);
  static void fail(const std::shared_ptr<State>&)noexcept;
  void require_for_lender_without_callbacks(const BnwNotificationPublicationLender&)const;
  void require_current()const;
  BnwEngineUiNotificationEffect pending_native_effect()const;
  // Authored source/control selection only, never draw/present success.
  BnwWorldViewControlTree::ControlQuote select_authored_type_control_owned(
      const BnwWorldViewControlTree&,const runtime::BnwWorldViewSourceOperationLoan&,
      const runtime::LuaScriptHost&)const;
  void invoke_owned(const std::function<void(BnwNotificationAuthoredArtPresentLeaf&)>&);
  [[noreturn]] void stop_unimplemented_authored_art_owned()const;
  [[noreturn]] void stop_unimplemented_after_swap_owned()const;
};
// Opaque positive returns remain UNISSUED: neither constructors nor acceptance
// bodies exist here until real typed authored art/draw/present owners qualify.
class BnwNotificationAuthoredDrawReturn;
class BnwNotificationAfterSwapReturn;
}
