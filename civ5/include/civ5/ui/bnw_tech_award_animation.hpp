#pragma once
#include "civ5/ui/bnw_client_frame.hpp"
#include <memory>
#include <thread>
#include <cstdint>
#include <string>
#include <string_view>
#include <utility>
#include <vector>
namespace civ5::graphics {class BnwOwnedCameraRendererWindowLease;}
namespace civ5::ui {
class UiScreenGlRenderer;class LiveUiHost;class BnwClientSceneFrame;
class BnwTechAwardContextOwner;struct BnwTechAwardContextLayout;
// Only the actual renderer loop may mint a sample. No float/time/caller flag
// constructor is public. It is valid for one synchronous update of its bound
// Scene/UI/Window sources; the next loop sample invalidates its predecessor.
class BnwTechAwardAnimationSample final {
 friend class UiScreenGlRenderer;friend class BnwTechAwardContextOwner;
 friend class BnwTechAwardAnimationWitness;
 friend class BnwScrollAnimationLoopTick;friend class BnwScrollAnimationUpdateLoan;
 struct Clock {
  const std::thread::id creator=std::this_thread::get_id();
  bool alive=true,active=false,failed=false;
  bool tick_initialized=false;std::uint32_t previous_tick=0;
  std::uint64_t sequence=0;
  std::weak_ptr<const BnwTechAwardAnimationSample> sample;
  std::weak_ptr<BnwClientSceneFrame> scene;
  std::weak_ptr<LiveUiHost> ui;
  std::weak_ptr<const graphics::BnwOwnedCameraRendererWindowLease> window;
 };
 std::weak_ptr<Clock> clock_;
 std::weak_ptr<const BnwTechAwardAnimationSample> self_;
 const std::thread::id creator_=std::this_thread::get_id();
 std::uint64_t sequence_=0;std::uint32_t tick_=0;float seconds_=0;
 mutable bool consumed_=false;
 BnwTechAwardAnimationSample(const std::shared_ptr<Clock>&,std::uint64_t,std::uint32_t,float);
 void require_current()const;
 public:
 ~BnwTechAwardAnimationSample()=default;
 BnwTechAwardAnimationSample(const BnwTechAwardAnimationSample&)=delete;
 BnwTechAwardAnimationSample& operator=(const BnwTechAwardAnimationSample&)=delete;
};
// Actual private LiveUiHost publication seals canonical Context control,
// selected layout control and publication. Copying the observation DTO cannot
// mint this witness or change its sealed authored profile membership.
class BnwTechAwardAnimationWitness final {
 friend class LiveUiHost;friend class UiScreenGlRenderer;friend class BnwTechAwardContextOwner;
 class Pin;
 std::weak_ptr<const BnwTechAwardContextOwner> owner_;
 std::weak_ptr<LiveUiHost> ui_;
 std::weak_ptr<const BnwTechAwardContextLayout> layout_;
 const std::thread::id creator_=std::this_thread::get_id();
 BnwClientFrameStamp publication_;
 std::uint64_t occurrence_=0,generation_=0,setters_=0;
 std::string xml_,styles_;
 BnwTechAwardAnimationWitness(const std::shared_ptr<const BnwTechAwardContextOwner>&,
  const std::shared_ptr<LiveUiHost>&,const BnwClientFrameStamp&,std::uint64_t,std::uint64_t,
  const std::shared_ptr<const BnwTechAwardContextLayout>&);
 void require_current()const;
 void require_current_without_callbacks()const;
 void quarantine()const noexcept;
 void require_publication(const BnwClientFrameStamp&,std::uint64_t,std::uint64_t,
  const std::shared_ptr<const BnwTechAwardContextLayout>&)const;
 void pointer_transition(ClientPoint,const BnwTechAwardContextLayout&)const;
 bool mouse_visible(std::string_view,const BnwTechAwardContextLayout&)const;
 std::vector<std::pair<std::string,float>> advance(const BnwTechAwardAnimationSample&)const;
 public:
 ~BnwTechAwardAnimationWitness()=default;
 BnwTechAwardAnimationWitness(const BnwTechAwardAnimationWitness&)=delete;
 BnwTechAwardAnimationWitness& operator=(const BnwTechAwardAnimationWitness&)=delete;
};
}
