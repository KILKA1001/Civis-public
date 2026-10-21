#pragma once
#include "civ5/ui/bnw_worldview_renderer_projection_loan.hpp"
#include "civ5/ui/bnw_notification_graphics_publication.hpp"
#include "civ5/terrain/world_layout.hpp"
#include "civ5/terrain/height_owner.hpp"
#include <array>
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <thread>
namespace civ5::ui {
class BnwNotificationMinimapRecordQueue;
class BnwOwnedNotificationPlotFxReceiver;
// Synchronous actual renderer/Scene terrain borrow. No public matrix/event
// constructor and no physical return, drain or presentation acceptance.
class BnwNotificationTerrainSceneLoan final {
 public:
  ~BnwNotificationTerrainSceneLoan();
  BnwNotificationTerrainSceneLoan(const BnwNotificationTerrainSceneLoan&)=delete;
  BnwNotificationTerrainSceneLoan& operator=(const BnwNotificationTerrainSceneLoan&)=delete;
  BnwNotificationTerrainSceneLoan(BnwNotificationTerrainSceneLoan&&)=delete;
  BnwNotificationTerrainSceneLoan& operator=(BnwNotificationTerrainSceneLoan&&)=delete;
 private:
  friend class UiScreenGlRenderer;
  friend class BnwClientSceneFrame;
  friend class BnwNotificationMinimapRecordQueue;
  friend class BnwOwnedNotificationPlotFxReceiver;
  struct State;
  struct Geometry final {
   terrain::WorldLayout layout;
   terrain::HeightOwnerParameters height;
   bool wrap_x;
  };
  struct PlotRead final {
   std::optional<std::int64_t> plot;
   std::optional<bool> revealed;
   std::int64_t published_team;
  };
  const std::thread::id creator_=std::this_thread::get_id();
  const std::shared_ptr<State> state_;
  explicit BnwNotificationTerrainSceneLoan(const BnwWorldViewRendererProjectionLoan&,
      const BnwNotificationPublicationLender&);
  static std::shared_ptr<State> bind(const BnwWorldViewRendererProjectionLoan&,
      const BnwNotificationPublicationLender&);
  static void pure(const std::shared_ptr<State>&);
  static void full(const std::shared_ptr<State>&);
  static void fail(const std::shared_ptr<State>&)noexcept;
  std::shared_ptr<State> saved_state_without_callbacks()const;
  void require_current()const;
  Geometry read_geometry_without_callbacks()const;
  std::array<std::int32_t,3> transformed_effect_coordinates_without_callbacks()const;
  PlotRead read_effect_plot_without_callbacks()const;
  void invoke_owned(const std::function<void(BnwNotificationTerrainSceneLoan&)>&);
};
}
