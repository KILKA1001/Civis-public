#include "civ5/ui/bnw_audio_constructor_rng_owner.hpp"
#ifdef _WIN32
#include "civ5/ui/bnw_audio_native_rng_numeric.hpp"
#include <atomic>
#include <map>
#include <mutex>
#include <stdexcept>
#include <thread>
#include <utility>
namespace civ5::ui {
namespace {
void need(bool value,const char* why){if(!value)throw std::runtime_error(why);}
using Parent= BnwNativePackageDiscoveryOwner;
using Key=std::weak_ptr<const Parent>;
std::mutex registry_mutex;
//Weak entries are deliberately tombstones while the real Parent lives:
// dropping a facade cannot restart state7 inside an existing audio session.
std::map<Key,std::weak_ptr<const BnwAudioConstructorRngOwner>,std::owner_less<Key>> registry;
}
struct BnwAudioConstructorRngOwner::Impl {
 const std::thread::id thread=std::this_thread::get_id();std::atomic<bool> retired{false};bool busy=false;
 const std::shared_ptr<const BnwAudioCompletedResourceOwner> resource;
 const std::shared_ptr<const Parent> parent;
 const BnwAudioWindowActivationLoan::SourceHandle window;
 const std::shared_ptr<const graphics::BnwOwnedCameraRendererWindowLease> physical;
 //Exact ca7c40 data initializer; private instance is never caller copied in.
 audio_rng_detail::NativeAudioRngNumericFields fields;
 const std::string constructor_prefix;
 Impl(std::shared_ptr<const BnwAudioCompletedResourceOwner> r,std::shared_ptr<const Parent> p,
      BnwAudioWindowActivationLoan::SourceHandle w,std::shared_ptr<const graphics::BnwOwnedCameraRendererWindowLease> actual)
  :resource(std::move(r)),parent(std::move(p)),window(std::move(w)),physical(std::move(actual)){}
 struct Call {
  std::shared_ptr<Impl> p;explicit Call(std::shared_ptr<Impl> value):p(std::move(value)){
   need(p->thread==std::this_thread::get_id(),"Audio constructor RNG foreign creator thread");
   need(!p->retired.load(),"Audio constructor RNG retired");
   if(p->busy){p->retired.store(true);throw std::runtime_error("Audio constructor RNG reentry");}p->busy=true;
  }
  ~Call(){p->busy=false;}
 };
 void validate()const{
  need(!retired.load(),"Audio constructor RNG retired");physical->require_current();window.require_current();
  resource->require_same_actual_window(window);window.require_current();physical->require_current();
  need(!retired.load(),"Audio constructor RNG retired during actual source callback");
 }
};
BnwAudioConstructorRngOwner::BnwAudioConstructorRngOwner(std::shared_ptr<Impl> p):impl_(std::move(p)){}
BnwAudioConstructorRngOwner::~BnwAudioConstructorRngOwner(){impl_->retired.store(true);}
std::shared_ptr<const BnwAudioConstructorRngOwner> BnwAudioConstructorRngOwner::from_actual_activation(
 std::shared_ptr<const BnwAudioCompletedResourceOwner> supplied,const BnwAudioWindowActivationLoan& borrowed,
 const graphics::BnwOwnedCameraRendererWindowLease::Loan& actual){
 const auto loan=borrowed;need(bool(supplied),"Audio constructor RNG actual resource owner absent");
 //Callback-free canonical factory retain, creator thread, actual physical
 // owner and registered Parent precede access to the single-context registry.
 const auto resource=supplied->retain_factory_owner(supplied);loan.require_current();
 const auto physical=actual.retain_owner();loan.require_current();
 need(physical&&actual.window()==reinterpret_cast<std::uintptr_t>(loan.window())&&actual.ui_thread()==loan.ui_thread(),
      "Audio constructor RNG physical and activation owner differ");
 const auto window=loan.source_handle();resource->require_same_actual_window(window);loan.require_current();actual.require_current();
 const auto parent=resource->retained_native_parent_for_audio_startup();loan.require_current();actual.require_current();
 std::shared_ptr<const BnwAudioConstructorRngOwner> result;
 {
  std::lock_guard lock(registry_mutex);
  for(auto it=registry.begin();it!=registry.end();)if(it->first.expired())it=registry.erase(it);else ++it;
  const auto found=registry.find(Key(parent));
  if(found!=registry.end()){
   result=found->second.lock();need(bool(result),"Existing Parent audio constructor RNG was retired/lost; reseed refused");
  }else{
   auto p=std::make_shared<Impl>(resource,parent,window,physical);
   result=std::shared_ptr<const BnwAudioConstructorRngOwner>(new BnwAudioConstructorRngOwner(std::move(p)));
   registry.emplace(Key(parent),result);
  }
 }
 //No producer callbacks under the registry mutex. Failure after publication
 // retires the actual context rather than permitting seed7 replacement.
 try{result->impl_->window.require_same_source(window);result->require_current();loan.require_current();actual.require_current();return result;}
 catch(...){result->impl_->retired.store(true);throw;}
}
void BnwAudioConstructorRngOwner::require_current()const{
 const auto p=impl_;Impl::Call call(p);try{p->validate();}catch(...){p->retired.store(true);throw;}
}
BnwAudioConstructorState BnwAudioConstructorRngOwner::constructor_state()const{
 const auto p=impl_;Impl::Call call(p);
 try{p->validate();const auto result=BnwAudioConstructorState{p->constructor_prefix,p->fields.state,p->fields.draws,false};p->validate();return result;}
 catch(...){p->retired.store(true);throw;}
}
std::uint32_t BnwAudioConstructorRngOwner::draw_numeric_component(std::uint32_t bound)const{
 const auto p=impl_;Impl::Call call(p);
 try{p->validate();const auto result=audio_rng_detail::step_native_audio_rng_numeric(p->fields,bound);p->validate();return result;}
 catch(...){p->retired.store(true);throw;}
}
}
#endif
