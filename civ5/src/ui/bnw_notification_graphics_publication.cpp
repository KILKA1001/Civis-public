#include "civ5/ui/bnw_notification_graphics_publication.hpp"
#include "civ5/runtime/bnw_owned_gameplay_event_pump_loan.hpp"
#include "civ5/ui/bnw_client_scene_frame.hpp"
#include <stdexcept>
#include <utility>
namespace civ5::ui {
struct BnwNotificationPublicationLender::Storage final {
 const std::thread::id creator=std::this_thread::get_id();
 std::atomic<bool> alive{true},failed{false};
 bool checking=false,invoking=false;
 std::uint64_t source_serial=0;
 std::unique_ptr<BnwNotificationOwnerModelLoan> model;
 const BnwOwnedCameraGuiScreen* gui;
 const BnwOwnedCameraGuiScreen::Loan* gui_loan;
 const graphics::BnwOwnedCameraRendererWindowLease::Loan* window_loan;
 const BnwClientSceneFrame* scene;
 const BnwClientFrame* frame;
 const LiveUiFrame* ui_frame;
 const ComposedUiFrame* composed;
 const runtime::BnwOwnedGameplayEventPumpLoan* pump;
 const runtime::BnwStockLuaAttemptLease* source;
 const std::weak_ptr<const BnwOwnedCameraGuiScreen> gui_origin;
 const std::weak_ptr<const BnwClientSceneFrame> scene_origin;
 const std::weak_ptr<const graphics::BnwOwnedCameraRendererWindowLease> window_origin;
 Storage(std::unique_ptr<BnwNotificationOwnerModelLoan>& m,
   const BnwOwnedCameraGuiScreen& g,const BnwOwnedCameraGuiScreen::Loan& gl,
   const graphics::BnwOwnedCameraRendererWindowLease::Loan& wl,
   const BnwClientSceneFrame& sc,const BnwClientFrame& f,const LiveUiFrame& ui,
   const ComposedUiFrame& co,const runtime::BnwOwnedGameplayEventPumpLoan& p,
   const runtime::BnwStockLuaAttemptLease& s,
   std::weak_ptr<const BnwOwnedCameraGuiScreen> go,
   std::weak_ptr<const BnwClientSceneFrame> so,
   std::weak_ptr<const graphics::BnwOwnedCameraRendererWindowLease> wo)noexcept
  :model(std::move(m)),gui(&g),gui_loan(&gl),window_loan(&wl),scene(&sc),frame(&f),
   ui_frame(&ui),composed(&co),pump(&p),source(&s),gui_origin(std::move(go)),
   scene_origin(std::move(so)),window_origin(std::move(wo)){}
};
void BnwNotificationPublicationLender::pure(const std::shared_ptr<Storage>& s){
 // Independent saved lifetime FIRST: no borrowed GUI/Scene/source/model use
 // after a provider destroyed this lender or retired its actual stack scope.
 if(!s||s->creator!=std::this_thread::get_id()||!s->alive.load()||s->failed.load())
  throw std::runtime_error("Notification graphics lender retired, failed or foreign");
 try{
 if(!s->model)throw std::runtime_error("Notification genuine model loan absent");
 if(s->gui_origin.expired()||s->scene_origin.expired()||s->window_origin.expired())
  throw std::runtime_error("Notification actual GUI/Scene/window origin expired");
 s->model->require_origin_without_callbacks();
 s->model->require_source_origin_without_callbacks(*s->pump,*s->source);
 s->gui->require_notification_storage_origin_without_callbacks(*s->gui_loan,
  *s->window_loan,*s->scene,*s->frame,*s->ui_frame,*s->composed,
  s->gui_origin,s->scene_origin,s->window_origin,
  s->model->host_,s->model->world_,s->model->generation_);
 s->model->require_same_frame_payload_without_callbacks(*s->ui_frame);
 }catch(...){quarantine_from_storage_without_callbacks(s);throw;}
}
std::unique_ptr<BnwNotificationOwnerModelLoan>
BnwNotificationPublicationLender::capture_model_owned(
 const BnwOwnedNotificationPublicationOwner& owner,const LiveUiFrame& frame,
 const runtime::BnwOwnedGameplayEventPumpLoan& pump,
 const runtime::BnwStockLuaAttemptLease& source){
 // Actual Policy6 capture holds/counts the canonical Owner BEFORE any target
 // copy. Both real model and source admission methods are callback-free.
 auto model=owner.capture_notification_model_for_graphics_without_callbacks(frame);
 try{
  model->require_origin_without_callbacks();
  model->require_source_origin_without_callbacks(pump,source);
  model->require_same_frame_payload_without_callbacks(frame);
  return model;
 }catch(...){quarantine_captured_model_without_callbacks(model);throw;}
}
void BnwNotificationPublicationLender::quarantine_captured_model_without_callbacks(
 const std::unique_ptr<BnwNotificationOwnerModelLoan>& model)noexcept {
 // Actual counted model custody only; Owner7 performs its creator-first pure
 // quarantine. Null means no model was captured, not a successful admission.
 if(model)model->quarantine_without_callbacks();
}
BnwNotificationPublicationLender::BnwNotificationPublicationLender(
 std::unique_ptr<BnwNotificationOwnerModelLoan>& model,
 const BnwOwnedCameraGuiScreen& gui,const BnwOwnedCameraGuiScreen::Loan& gl,
 const graphics::BnwOwnedCameraRendererWindowLease::Loan& wl,
 const BnwClientSceneFrame& scene,const BnwClientFrame& frame,const LiveUiFrame& ui,
 const ComposedUiFrame& co,const runtime::BnwOwnedGameplayEventPumpLoan& pump,
 const runtime::BnwStockLuaAttemptLease& source,
 std::weak_ptr<const BnwOwnedCameraGuiScreen> go,
 std::weak_ptr<const BnwClientSceneFrame> so,
 std::weak_ptr<const graphics::BnwOwnedCameraRendererWindowLease> wo)
 :storage_(std::make_shared<Storage>(model,gui,gl,wl,scene,frame,ui,co,pump,source,
    std::move(go),std::move(so),std::move(wo))){
 try{
  const auto s=storage_;pure(s);
  s->source_serial=source.operation_serial();pure(s);
  source.require_operation(s->source_serial);pure(s);
  full(s);
 }
 catch(...){quarantine_from_storage_without_callbacks(storage_);storage_->alive.store(false);
  throw;}
}
BnwNotificationPublicationLender::~BnwNotificationPublicationLender(){
 // The GUI's enclosing genuine loan owns normal unwind order; no borrowed
 // pointer or callback is consulted by destruction, even on foreign teardown.
 storage_->alive.store(false);
}
void BnwNotificationPublicationLender::require_origin_without_callbacks()const {
 if(creator_!=std::this_thread::get_id())throw std::runtime_error("Notification graphics foreign creator");
 const auto s=storage_;pure(s);
}
void BnwNotificationPublicationLender::require_current()const {
 if(creator_!=std::this_thread::get_id())throw std::runtime_error("Notification graphics foreign creator");
 const auto s=storage_;full(s);
}
void BnwNotificationPublicationLender::full(const std::shared_ptr<Storage>& s){
 pure(s);
 if(s->checking){quarantine_from_storage_without_callbacks(s);
  throw std::runtime_error("Notification graphics lender reentered");}
 s->checking=true;
 struct Checking {Storage& s;~Checking(){s.checking=false;}} checking{*s};
 try{
  s->window_loan->require_current();pure(s);
  s->gui_loan->require_current();pure(s);
  s->pump->require_for_source(*s->source);pure(s);
  s->source->require_operation(s->source_serial);pure(s);
 }catch(...){quarantine_from_storage_without_callbacks(s);throw;}
}
void BnwNotificationPublicationLender::invoke_owned(
 const std::function<void(BnwNotificationPublicationLender&)>& target){
 if(creator_!=std::this_thread::get_id())throw std::runtime_error("Notification graphics foreign creator");
 const auto s=storage_;pure(s);
 if(s->invoking){quarantine_from_storage_without_callbacks(s);
  throw std::runtime_error("Notification graphics target reentered");}
 s->invoking=true;
 struct Invoking {Storage& s;~Invoking(){s.invoking=false;}} invoking{*s};
 std::unique_ptr<std::function<void(BnwNotificationPublicationLender&)>> owned;
 try{
  full(s);pure(s);
  // Actual Owner6 ModelLoan owns/counts canonical Owner before target COPY.
  owned=std::make_unique<std::function<void(BnwNotificationPublicationLender&)>>(target);
  pure(s);full(s);pure(s);
  if(!*owned)throw std::invalid_argument("Notification graphics target absent");
  (*owned)(*this);pure(s);full(s);pure(s);
  owned.reset();pure(s);full(s);pure(s);
 }catch(...){quarantine_from_storage_without_callbacks(s);
  owned.reset();throw;}
}
BnwEngineUiNotificationEffect BnwNotificationPublicationLender::pending_native_effect()const {
 if(creator_!=std::this_thread::get_id())throw std::runtime_error("Notification graphics foreign creator");
 const auto s=storage_;return pending_effect_from_storage(s);
}
BnwEngineUiNotificationEffect BnwNotificationPublicationLender::pending_effect_from_storage(
 const std::shared_ptr<Storage>& s){
 pure(s);
 try{auto effect=s->model->pending_effect();pure(s);return effect;}
 catch(...){quarantine_from_storage_without_callbacks(s);throw;}
}
void BnwNotificationPublicationLender::fail_from_storage_without_callbacks(
 const std::shared_ptr<Storage>& s)noexcept {
 if(s)s->failed.store(true);
}
void BnwNotificationPublicationLender::quarantine_from_storage_without_callbacks(
 const std::shared_ptr<Storage>& s)noexcept {
 fail_from_storage_without_callbacks(s);
 // Actual Policy Owner7 pure delegate: no callback/weak resolve/effect and
 // no Scope-current claim, including after Source retirement.
 if(s&&s->model)s->model->quarantine_without_callbacks();
}
std::weak_ptr<const BnwClientSceneFrame> BnwNotificationPublicationLender::scene_from_storage_without_callbacks(
 const std::shared_ptr<Storage>& s){
 pure(s);return s->scene_origin;
}
void BnwNotificationPublicationLender::require_minimap_scene_from_storage_without_callbacks(
 const std::shared_ptr<Storage>& s,const std::weak_ptr<const BnwClientSceneFrame>& expected){
 pure(s);
 {
  const auto actual=s->scene_origin.lock();const auto other=expected.lock();
  if(!actual||!other||actual.get()!=s->scene||other.get()!=actual.get()||
   actual.owner_before(other)||other.owner_before(actual))
   throw std::runtime_error("Notification minimap Scene get/control differs");
 } // No checker Scene pin crosses callbacks or the final pure origin fence.
 pure(s);
}
std::weak_ptr<const BnwClientSceneFrame> BnwNotificationPublicationLender::actual_scene_without_callbacks()const {
 if(creator_!=std::this_thread::get_id())throw std::runtime_error("Notification graphics foreign creator");
 const auto s=storage_;return scene_from_storage_without_callbacks(s);
}
void BnwNotificationPublicationLender::require_same_minimap_scene_without_callbacks(
 const std::weak_ptr<const BnwClientSceneFrame>& expected)const {
 if(creator_!=std::this_thread::get_id())throw std::runtime_error("Notification graphics foreign creator");
 const auto s=storage_;require_minimap_scene_from_storage_without_callbacks(s,expected);
}
[[noreturn]] void BnwNotificationPublicationLender::stop_unimplemented_native_effect_owned()const {
 if(creator_!=std::this_thread::get_id())throw std::runtime_error("Notification graphics foreign creator");
 const auto s=storage_;pure(s);
 // Quarantine the independent lender and actual retained canonical Model.
 // No physical call/store occurred: no prefix, receipt, ACK or history of a
 // successful effect may be manufactured by this mandatory unsupported stop.
 quarantine_from_storage_without_callbacks(s);
 throw std::runtime_error("Notification mandatory nullable-plot FX/minimap queue/type-art/afterSwap receiver is unimplemented");
}
}
