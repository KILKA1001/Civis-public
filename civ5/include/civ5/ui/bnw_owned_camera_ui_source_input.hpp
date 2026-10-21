#pragma once
#include "civ5/ui/selected_bnw_preview_host.hpp"
#include <thread>
namespace civ5::graphics { class BnwOwnedCameraRendererWindowLease; }
namespace civ5::ui {
class BnwLivePreviewLaunchEngine;
class BnwPlotYieldRefreshOwnedUiContext;
// Actual private selected-preview producer owns both input callable storage and
// its authentic parent. It issues no native VM/App/UIManager/renderer authority.
class BnwOwnedCameraUiSourceInput final {
 public:
  ~BnwOwnedCameraUiSourceInput()=default;
  BnwOwnedCameraUiSourceInput(const BnwOwnedCameraUiSourceInput&)=delete;
  BnwOwnedCameraUiSourceInput&operator=(const BnwOwnedCameraUiSourceInput&)=delete;
  const LiveBnwPreviewInput& input()const;
  const std::shared_ptr<const SelectedBnwPreviewHost>& parent()const;
  void require_current()const;
 private:
  friend class SelectedBnwReadOnlyPreview;
  friend class BnwLivePreviewLaunchEngine;
  friend class civ5::graphics::BnwOwnedCameraRendererWindowLease;
  friend class BnwPlotYieldRefreshOwnedUiContext;
  BnwOwnedCameraUiSourceInput(std::shared_ptr<const SelectedBnwPreviewHost>,LiveBnwPreviewInput&&);
  void install_checked_launch_input(LiveBnwPreviewInput&&);
  void require_finality()const;
  std::shared_ptr<BnwOwnedCameraUiSourceInput> retain_owned_ui_owner(const std::shared_ptr<BnwOwnedCameraUiSourceInput>&)const;
  const LiveBnwPreviewInput& final_input()const;
  struct Impl;std::shared_ptr<Impl> impl_;

};
}
