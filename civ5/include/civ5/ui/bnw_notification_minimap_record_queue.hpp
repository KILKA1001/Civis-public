#pragma once
#include "civ5/ui/minimap_world_bridge.hpp"
#include <cstddef>
#include <memory>
#include <thread>
#include <utility>
namespace civ5::ui {
class BnwClientSceneFrame;
class BnwOwnedNotificationMinimapReceiver;
class UiScreenGlRenderer;
struct BnwEngineUiNotificationEffect;

// Actual private record storage. Queue mutation is not drain/presentation proof.
class BnwNotificationMinimapRecordQueue final {
 public:
  ~BnwNotificationMinimapRecordQueue();
  BnwNotificationMinimapRecordQueue(const BnwNotificationMinimapRecordQueue&)=delete;
  BnwNotificationMinimapRecordQueue& operator=(const BnwNotificationMinimapRecordQueue&)=delete;
  BnwNotificationMinimapRecordQueue(BnwNotificationMinimapRecordQueue&&)=delete;
  BnwNotificationMinimapRecordQueue& operator=(BnwNotificationMinimapRecordQueue&&)=delete;
 private:
  friend class BnwClientSceneFrame;
  friend class BnwOwnedNotificationMinimapReceiver;
  friend class UiScreenGlRenderer;
  class SceneFactoryKey final {
   friend class BnwClientSceneFrame;
   friend class BnwNotificationMinimapRecordQueue;
   const std::weak_ptr<const BnwClientSceneFrame> scene_;
   const std::thread::id creator_=std::this_thread::get_id();
   explicit SceneFactoryKey(std::weak_ptr<const BnwClientSceneFrame> scene)
    :scene_(std::move(scene)){}
   SceneFactoryKey(const SceneFactoryKey&)=delete;
   SceneFactoryKey& operator=(const SceneFactoryKey&)=delete;
  };
  struct State;
  const std::thread::id creator_=std::this_thread::get_id();
  const std::shared_ptr<State> state_;
  explicit BnwNotificationMinimapRecordQueue(
      std::weak_ptr<const BnwClientSceneFrame>,MinimapWorldTransform,std::size_t);
  // Root Scene supplies its actual retained terrain/runtime binding, never
  // caller matrices. Explicit capacity is OWN storage budget, not native ACK.
  static std::shared_ptr<BnwNotificationMinimapRecordQueue> create_for_actual_scene(
      const SceneFactoryKey&,const terrain::WorldLayout&,
      const terrain::HeightOwnerParameters&,std::size_t maximum_records);
  std::shared_ptr<State> save_for_source_without_callbacks()const;
  static void pure(const std::shared_ptr<State>&);
  static void quarantine_without_callbacks(const std::shared_ptr<State>&)noexcept;
  // Existing Receiver extracts actual Effect VALUE through Source4/5 lender
  // and performs Model/GUI/window/source PRE/POST outside this pure store.
  static void append_current_owned(const std::shared_ptr<State>&,
      const std::weak_ptr<const BnwClientSceneFrame>&,
      const terrain::WorldLayout&,const terrain::HeightOwnerParameters&,
      const BnwEngineUiNotificationEffect&);
  // No drain, public event API, count receipt or positive MinimapReturn issuer.
};
}
