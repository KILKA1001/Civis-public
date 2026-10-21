#include "civ5/ui/bnw_notification_authored_art_present_leaf.hpp"
#include "civ5/ui/bnw_engine_ui_turn_owner.hpp"
#include <atomic>
#include <stdexcept>
namespace civ5::ui {
struct BnwNotificationAuthoredArtPresentLeaf::State final {
 const std::thread::id creator=std::this_thread::get_id();
 std::atomic<bool> alive{true},failed{false};
 bool checking=false,invoking=false;
 const std::weak_ptr<BnwNotificationPublicationLender::Storage> lender;
 const BnwEngineUiNotificationEffect pending;
 State(std::weak_ptr<BnwNotificationPublicationLender::Storage> l,
       BnwEngineUiNotificationEffect e):lender(std::move(l)),pending(std::move(e)){}
};
std::shared_ptr<BnwNotificationAuthoredArtPresentLeaf::State>
BnwNotificationAuthoredArtPresentLeaf::bind(const BnwNotificationPublicationLender& lender){
 // No callback occurs before taking the exact private lender storage binding.
 lender.require_origin_without_callbacks();
 const auto actual=lender.storage_;
 BnwNotificationPublicationLender::pure(actual);
 auto pending=BnwNotificationPublicationLender::pending_effect_from_storage(actual);
 BnwNotificationPublicationLender::pure(actual);
 return std::make_shared<State>(actual,std::move(pending));
}
void BnwNotificationAuthoredArtPresentLeaf::fail(const std::shared_ptr<State>& s)noexcept{
 s->failed.store(true);
 if(const auto actual=s->lender.lock())
  BnwNotificationPublicationLender::quarantine_from_storage_without_callbacks(actual);
}
void BnwNotificationAuthoredArtPresentLeaf::pure(const std::shared_ptr<State>& s){
 // Saved independent life FIRST, before any borrowed lender/scope inspection.
 if(s->creator!=std::this_thread::get_id()||!s->alive.load()||s->failed.load())
  throw std::runtime_error("Notification art/present leaf retired, failed or foreign");
 const auto actual=s->lender.lock();
 if(!actual)throw std::runtime_error("Notification art/present lender storage retired");
 BnwNotificationPublicationLender::pure(actual);
 const auto pending=BnwNotificationPublicationLender::pending_effect_from_storage(actual);
 if(pending!=s->pending){fail(s);throw std::runtime_error("Notification art/present exact pending model effect changed");}
}
void BnwNotificationAuthoredArtPresentLeaf::full(const std::shared_ptr<State>& s){
 pure(s);
 if(s->checking){fail(s);throw std::runtime_error("Notification art/present checker reentered");}
 s->checking=true;
 struct Checking {State& s;~Checking(){s.checking=false;}} checking{*s};
 try{
  const auto actual=s->lender.lock();
  if(!actual)throw std::runtime_error("Notification art/present lender storage retired");
  BnwNotificationPublicationLender::full(actual);
  pure(s); // FIRST leaf lifetime POST before any facade/raw borrowed reuse.
 }catch(...){fail(s);throw;}
}
BnwNotificationAuthoredArtPresentLeaf::BnwNotificationAuthoredArtPresentLeaf(
 const BnwNotificationPublicationLender& lender):state_(bind(lender)){
 const auto saved=state_;
 try{full(saved);}catch(...){fail(saved);saved->alive.store(false);throw;}
}
BnwNotificationAuthoredArtPresentLeaf::~BnwNotificationAuthoredArtPresentLeaf(){
 state_->alive.store(false); // No callback or borrowed scope used on retirement.
}
void BnwNotificationAuthoredArtPresentLeaf::require_for_lender_without_callbacks(
 const BnwNotificationPublicationLender& lender)const{
 if(creator_!=std::this_thread::get_id())throw std::runtime_error("Notification art/present foreign creator");
 const auto saved=state_;pure(saved);
 lender.require_origin_without_callbacks();
 const auto actual=saved->lender.lock();const auto other=lender.storage_;
 if(!actual||actual.get()!=other.get()||actual.owner_before(other)||other.owner_before(actual)){
  fail(saved);throw std::runtime_error("Notification art/present canonical lender substituted");
 }
 pure(saved);
}
void BnwNotificationAuthoredArtPresentLeaf::require_current()const{
 if(creator_!=std::this_thread::get_id())throw std::runtime_error("Notification art/present foreign creator");
 const auto saved=state_;full(saved);
}
BnwEngineUiNotificationEffect BnwNotificationAuthoredArtPresentLeaf::pending_native_effect()const{
 if(creator_!=std::this_thread::get_id())throw std::runtime_error("Notification art/present foreign creator");
 const auto saved=state_;full(saved);pure(saved);
 return saved->pending; // Value only: no native art, draw or presentation proof.
}
BnwWorldViewControlTree::ControlQuote
BnwNotificationAuthoredArtPresentLeaf::select_authored_type_control_owned(
 const BnwWorldViewControlTree& tree,const runtime::BnwWorldViewSourceOperationLoan& operation,
 const runtime::LuaScriptHost& vm)const {
 if(creator_!=std::this_thread::get_id())throw std::runtime_error("Notification authored selector foreign creator");
 const auto saved=state_;pure(saved);
 try {
  full(saved);pure(saved);
  const auto serial=operation.operation_serial();pure(saved);
  operation.require_operation(serial);pure(saved);
  operation.require_same_vm(vm);pure(saved);
  const auto source=tree.actual_source();pure(saved);
  if(!source)throw std::runtime_error("Notification actual authored descriptor missing");
  operation.require_same_source(*source);pure(saved);
  if(tree.context()!="NotificationPanel"){pure(saved);throw std::runtime_error("Notification authored selector has another canonical tree");}
  pure(saved);
  // Genuine pending effect from the SAME immutable actual Owner7 model.
  // These hashes are the current Engine's CvEnums FREE_POLICY/FREE_TECH rows;
  // BNW Lua's g_NameTable selects these authored static BigStack buttons.
  const char* id=nullptr;
  if(saved->pending.type==0x1e4417c5U)id="FreePolicyButton";
  else if(saved->pending.type==0x489b80b6U)id="FreeTechButton";
  else throw std::runtime_error("Notification authored selector requires additional genuine stock type binding");
  const auto control=tree.static_control(id);pure(saved);
  auto quote=tree.inspect_control(control);pure(saved);
  if(!quote.alive||quote.dynamic||quote.authored.name!="Button"||
     quote.authored.attribute("ID")!=id||
     quote.authored.attribute("Texture")!="assets\\UI\\Art\\Notification\\NotificationIconsFrame.dds")
   throw std::runtime_error("Notification authored static type button/source differs");
  // This value deliberately does NOT prove OnNotificationAdded invocation,
  // Void1/id binding, shown state, actual GL writes or a returned present.
  // The genuine native graph/event and renderer must qualify those separately.
  operation.require_operation(serial);pure(saved);full(saved);pure(saved);
  return quote;
 }catch(...){fail(saved);throw;}
}
void BnwNotificationAuthoredArtPresentLeaf::invoke_owned(
 const std::function<void(BnwNotificationAuthoredArtPresentLeaf&)>& target){
 if(creator_!=std::this_thread::get_id())throw std::runtime_error("Notification art/present foreign creator");
 const auto saved=state_;pure(saved);
 if(saved->invoking){fail(saved);throw std::runtime_error("Notification art/present target reentered");}
 saved->invoking=true;
 struct Invoking {State& s;~Invoking(){s.invoking=false;}} invoking{*saved};
 std::unique_ptr<std::function<void(BnwNotificationAuthoredArtPresentLeaf&)>> owned;
 try{
  full(saved);pure(saved);
  owned=std::make_unique<std::function<void(BnwNotificationAuthoredArtPresentLeaf&)>>(target);
  pure(saved);full(saved);pure(saved);
  if(!*owned)throw std::invalid_argument("Notification art/present target absent");
  (*owned)(*this);pure(saved);full(saved);pure(saved);
  owned.reset();pure(saved);full(saved);pure(saved);
 }catch(...){fail(saved);owned.reset();throw;}
}
[[noreturn]] void BnwNotificationAuthoredArtPresentLeaf::stop_unimplemented_authored_art_owned()const{
 if(creator_!=std::this_thread::get_id())throw std::runtime_error("Notification art/present foreign creator");
 const auto saved=state_;pure(saved);fail(saved);
 throw std::runtime_error("Notification exact native type authored control/art and physical draw issuer is unimplemented");
}
[[noreturn]] void BnwNotificationAuthoredArtPresentLeaf::stop_unimplemented_after_swap_owned()const{
 if(creator_!=std::this_thread::get_id())throw std::runtime_error("Notification art/present foreign creator");
 const auto saved=state_;pure(saved);fail(saved);
 throw std::runtime_error("Notification SAME authored draw/frame/HWND returned present and source POST issuer is unimplemented");
}
}
