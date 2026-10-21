#include "civ5/ui/bnw_owned_notification_minimap_receiver.hpp"
#include "civ5/ui/bnw_notification_graphics_publication.hpp"
#include "civ5/ui/bnw_owned_notification_publication_owner.hpp"
#include <atomic>
#include <stdexcept>
#include <utility>

namespace civ5::ui {
struct BnwOwnedNotificationMinimapReceiver::State final {
 const std::thread::id creator=std::this_thread::get_id();
 std::atomic<bool> alive{true},failed{false};
 std::weak_ptr<const BnwOwnedNotificationMinimapReceiver> self;
 std::weak_ptr<const BnwClientSceneFrame> scene;
 bool busy=false;
 explicit State(std::weak_ptr<const BnwClientSceneFrame> actual):scene(std::move(actual)){}
};
BnwOwnedNotificationMinimapReceiver::BnwOwnedNotificationMinimapReceiver(
 std::weak_ptr<const BnwClientSceneFrame> actual):state_(std::make_shared<State>(std::move(actual))){}
BnwOwnedNotificationMinimapReceiver::~BnwOwnedNotificationMinimapReceiver(){
 state_->alive.store(false);state_->failed.store(true);
}
void BnwOwnedNotificationMinimapReceiver::pure(const std::shared_ptr<State>& p){
 if(p->creator!=std::this_thread::get_id()){
  p->failed.store(true);throw std::runtime_error("Notification minimap receiver foreign creator");
 }
 // The retained State is not a facade/Scene pin. Check it first after every
 // callback, before weak ownership or mutable source data can be inspected.
 if(!p->alive.load()||p->failed.load())
  throw std::runtime_error("Notification minimap receiver retired or failed");
 if(p->self.expired()||p->scene.expired()){
  p->failed.store(true);throw std::runtime_error("Notification minimap receiver outside owner absent");
 }
}
std::shared_ptr<BnwOwnedNotificationMinimapReceiver>
BnwOwnedNotificationMinimapReceiver::create_for_actual_scene(const BnwNotificationPublicationLender& loan){
 if(loan.creator_!=std::this_thread::get_id())
  throw std::runtime_error("Notification minimap factory foreign lender");
 const auto source=loan.storage_; // Earn once before callback; no raw loan reads below.
 BnwNotificationPublicationLender::full(source);
 auto actual=BnwNotificationPublicationLender::scene_from_storage_without_callbacks(source);
 auto result=std::shared_ptr<BnwOwnedNotificationMinimapReceiver>(
  new BnwOwnedNotificationMinimapReceiver(std::move(actual)));
 const auto p=result->state_;p->self=result;
 try{
  pure(p);
  BnwNotificationPublicationLender::require_minimap_scene_from_storage_without_callbacks(source,p->scene);
  BnwNotificationPublicationLender::full(source);pure(p);
  BnwNotificationPublicationLender::require_minimap_scene_from_storage_without_callbacks(source,p->scene);
  pure(p);
  return result;
 }catch(...){
  // Independent retained State first; quarantine is pure and cannot revive an
  // expired lender or model. No facade/Scene access follows the callback.
  const bool alive=p->alive.load();p->failed.store(true);
  BnwNotificationPublicationLender::quarantine_from_storage_without_callbacks(source);
  if(!alive)throw std::runtime_error("Notification minimap factory retired during callback");
  throw;
 }
}
[[noreturn]] void BnwOwnedNotificationMinimapReceiver::dispatch_current_owned(
 const BnwNotificationPublicationLender& loan)const {
 if(creator_!=std::this_thread::get_id()){
  state_->failed.store(true); // Foreign violation touches an atomic only.
  throw std::runtime_error("Notification minimap dispatch foreign creator");
 }
 const auto p=state_;
 if(loan.creator_!=p->creator){
  p->failed.store(true);throw std::runtime_error("Notification minimap dispatch foreign lender");
 }
 const auto source=loan.storage_; // Retain only independent source storage.
 pure(p);
 if(p->busy){
  p->failed.store(true);
  BnwNotificationPublicationLender::quarantine_from_storage_without_callbacks(source);
  throw std::runtime_error("Notification minimap dispatch reentry");
 }
 p->busy=true;
 struct End final {std::shared_ptr<State> p;~End(){p->busy=false;}} end{p};
 try{
  BnwNotificationPublicationLender::require_minimap_scene_from_storage_without_callbacks(source,p->scene);
  BnwNotificationPublicationLender::full(source);pure(p);
  const auto effect=BnwNotificationPublicationLender::pending_effect_from_storage(source);
  pure(p);
  BnwNotificationPublicationLender::require_minimap_scene_from_storage_without_callbacks(source,p->scene);
  if(effect.kind!=BnwEngineUiNotificationEffectKind::gameplay_minimap_notification)
   throw std::runtime_error("Notification minimap dispatch has another authoritative head");
  // Engine has ALREADY added one. No second increment and no caller ID input.
  if(effect.lookup<=0)
   throw std::runtime_error("Notification minimap authoritative marker ID invalid");
  if(effect.x!=-1||effect.y!=-1)
   throw std::runtime_error("Notification minimap spatial receiver remains unsupported");
  BnwNotificationPublicationLender::full(source);pure(p);
  BnwNotificationPublicationLender::require_minimap_scene_from_storage_without_callbacks(source,p->scene);
  pure(p);
  // Native (-1,-1) enqueues type0x60/size0x80 and transforms cube(0,-1,1).
  // The current real matrix and downstream dispatch are not admitted. Do not
  // append invented packed coordinates, consume the FIFO, or issue a return.
  throw std::runtime_error("Notification minimap native matrix/type0x60 dispatch unavailable");
 }catch(...){
  const bool alive=p->alive.load();p->failed.store(true);
  BnwNotificationPublicationLender::quarantine_from_storage_without_callbacks(source);
  if(!alive)throw std::runtime_error("Notification minimap receiver retired during callback");
  throw;
 }
}
}
