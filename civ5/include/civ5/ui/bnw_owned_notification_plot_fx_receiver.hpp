#pragma once
#include "civ5/terrain/world_layout.hpp"
#include <cstddef>
#include <memory>
#include <thread>
#include <utility>
namespace civ5::ui {
class BnwClientSceneFrame;
class BnwNotificationPublicationLender;
class UiScreenGlRenderer;
// Scene-owned command storage, not an effect draw/drain/publication receipt.
class BnwOwnedNotificationPlotFxReceiver final {
 public:
  ~BnwOwnedNotificationPlotFxReceiver();
  BnwOwnedNotificationPlotFxReceiver(const BnwOwnedNotificationPlotFxReceiver&)=delete;
  BnwOwnedNotificationPlotFxReceiver& operator=(const BnwOwnedNotificationPlotFxReceiver&)=delete;
 private:
  friend class BnwClientSceneFrame;
  friend class BnwNotificationPublicationLender;
  friend class UiScreenGlRenderer;
  class SceneFactoryKey final {
   friend class BnwClientSceneFrame;
   friend class BnwOwnedNotificationPlotFxReceiver;
   const std::thread::id creator_=std::this_thread::get_id();
   const std::weak_ptr<const BnwClientSceneFrame> scene_;
   explicit SceneFactoryKey(std::weak_ptr<const BnwClientSceneFrame> scene):scene_(std::move(scene)){}
   SceneFactoryKey(const SceneFactoryKey&)=delete;
  };
  struct State;
  const std::thread::id creator_=std::this_thread::get_id();
  const std::shared_ptr<State> state_;
  explicit BnwOwnedNotificationPlotFxReceiver(std::weak_ptr<const BnwClientSceneFrame>,terrain::IntMatrix4,std::size_t);
  static std::shared_ptr<BnwOwnedNotificationPlotFxReceiver> create_for_actual_scene(
      const SceneFactoryKey&,const terrain::WorldLayout&,std::size_t maximum_operations);
  static void pure(const std::shared_ptr<State>&);
  static void fail(const std::shared_ptr<State>&)noexcept;
  // Root Scene/renderer must lend SAME retained TerrainWorld layout under its
  // real Run/frame/window/source scope. No public matrix or caller grant.
  void dispatch_current_owned(const BnwNotificationPublicationLender&,
                              const terrain::WorldLayout&)const;
  // No nullable-FX return constructor, drain, visual completion or ACK issuer.
};
}
