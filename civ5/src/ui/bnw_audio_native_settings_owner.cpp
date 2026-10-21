#include "civ5/ui/bnw_audio_native_settings_owner.hpp"
#ifdef _WIN32
#include "bnw_audio_knob_state_detail.hpp"
#include "bnw_audio_user_settings_input_detail.hpp"
#include <atomic>
#include <bit>
#include <limits>
#include <stdexcept>
#include <thread>
#include <utility>
namespace civ5::ui {
namespace {
void need(bool v,const char* why){if(!v)throw std::runtime_error(why);}
const content::XmlNode& sound_types(const content::XmlNode& document){
 need(document.name=="AudioParams","Native audio settings actual AudioParams root differs");
 const content::XmlNode* found=nullptr;
 for(const auto& child:document.children)if(child.name=="SoundTypes"){
  need(!found,"Native audio settings duplicate SoundTypes");found=&child;
 }
 need(found,"Native audio settings actual SoundTypes absent");return *found;
}
}
struct BnwAudioNativeSettingsOwner::Impl {
 const std::thread::id thread=std::this_thread::get_id();
 std::atomic<bool> retired{false};bool busy=false;
 const std::shared_ptr<const BnwAudioCompletedResourceOwner> resource;
 const BnwAudioWindowActivationLoan::SourceHandle window;
 const std::shared_ptr<const graphics::BnwOwnedCameraRendererWindowLease> physical;
 BnwAudioNativeXmlDocument parameters;
 BnwAudioNativeUserSettings settings;
 std::array<std::int32_t,4> percentages{};
 std::unique_ptr<audio_settings_detail::KnobNumericGraph> graph;
 std::uint64_t processed=0;
 bool cached_minimized=false; // Fresh Civis constructor, no imported native cache claim.
 Impl(std::shared_ptr<const BnwAudioCompletedResourceOwner> r,
      BnwAudioWindowActivationLoan::SourceHandle w,
      std::shared_ptr<const graphics::BnwOwnedCameraRendererWindowLease> p)
  :resource(std::move(r)),window(std::move(w)),physical(std::move(p)){}
 void check()const{need(!retired.load(),"Native audio settings owner retired");}
 struct Call {
  std::shared_ptr<Impl> p;
  explicit Call(std::shared_ptr<Impl> v):p(std::move(v)){
   need(p->thread==std::this_thread::get_id(),"Native audio settings foreign creator thread");p->check();
   if(p->busy){p->retired.store(true);throw std::runtime_error("Native audio settings operation reentry");}p->busy=true;
  }
  ~Call(){p->busy=false;}
 };
 void guard(std::uint64_t sequence)const{
  check();physical->require_current();check();window.require_current();check();
  need(sequence>0&&window.observed_sequence()==sequence,"Native audio settings actual activation unconsumed/changed");
  resource->require_same_actual_window(window);check();physical->require_current();check();window.require_current();check();
  need(window.observed_sequence()==sequence,"Native audio settings activation changed during source callback");
 }
 void validate(std::uint64_t sequence)const{
  guard(sequence);
  const auto xml=resource->read_native_audio_xml("AudioParams.xml");guard(sequence);
  need(xml.sha256==parameters.sha256&&xml.bytes==parameters.bytes,"Native audio settings selected hierarchy changed");
  const auto actual=resource->read_native_audio_user_settings();guard(sequence);
  need(actual.logical_path==settings.logical_path&&actual.selected_path==settings.selected_path&&
       actual.sha256==settings.sha256&&actual.bytes==settings.bytes&&actual.raw==settings.raw,
       "Native audio settings Boot-selected bytes changed");
 }
 void apply_master(bool observed){
  // Native84c43d stores oldCached?1:0, then caches new observed word. Preserve order.
  if(observed!=cached_minimized)graph->direct("MASTER",cached_minimized?1.0f:0.0f);
  cached_minimized=observed;
 }
};
BnwAudioNativeSettingsOwner::BnwAudioNativeSettingsOwner(std::shared_ptr<Impl> p):impl_(std::move(p)){}
BnwAudioNativeSettingsOwner::~BnwAudioNativeSettingsOwner(){impl_->retired.store(true);}
std::shared_ptr<BnwAudioNativeSettingsOwner> BnwAudioNativeSettingsOwner::from_first_actual_activation(
 std::shared_ptr<const BnwAudioCompletedResourceOwner> supplied,const BnwAudioWindowActivationLoan& borrowed,
 const graphics::BnwOwnedCameraRendererWindowLease::Loan& actual_window){
 const auto loan=borrowed; // Immutable original before any callback.
 need(bool(supplied),"Native audio settings genuine completed resource owner absent");
 const auto resource=supplied->retain_factory_owner(supplied);
 loan.require_current();need(loan.sequence()==1,"Native audio settings first activation history missing");
 const auto physical=actual_window.retain_owner();loan.require_current();
 need(physical&&actual_window.window()==reinterpret_cast<std::uintptr_t>(loan.window())&&
      actual_window.ui_thread()==loan.ui_thread(),"Native audio settings physical/activation sources differ");
 actual_window.require_current();loan.require_current();
 auto p=std::make_shared<Impl>(resource,loan.source_handle(),physical);Impl::Call call(p);
 try{
  p->guard(1);loan.require_current();
  p->parameters=resource->read_native_audio_xml("AudioParams.xml");p->guard(1);loan.require_current();
  p->settings=resource->read_native_audio_user_settings();p->guard(1);loan.require_current();
  p->percentages=audio_settings_detail::parse_audio_settings_existing_subset(p->settings.raw);
  p->graph=std::make_unique<audio_settings_detail::KnobNumericGraph>(sound_types(p->parameters.document));
  // All four keys/category conversions are validated before any four stores.
  p->graph->load_four_saved_percentages(p->percentages);
  p->validate(1);loan.require_current();actual_window.require_current();
  p->apply_master(loan.native_minimized_word()!=0);p->processed=1;
  p->validate(1);loan.require_current();actual_window.require_current();
  return std::shared_ptr<BnwAudioNativeSettingsOwner>(new BnwAudioNativeSettingsOwner(p));
 }catch(...){p->retired.store(true);throw;}
}
void BnwAudioNativeSettingsOwner::consume_actual_activation(const BnwAudioWindowActivationLoan& borrowed){
 const auto loan=borrowed;const auto p=impl_;Impl::Call call(p);
 try{
  loan.require_current();p->check();loan.source_handle().require_same_source(p->window);p->check();
  need(p->processed<std::numeric_limits<std::uint64_t>::max()&&loan.sequence()==p->processed+1,
       "Native audio settings missed/repeated/stale activation");
  const auto sequence=loan.sequence();p->validate(sequence);loan.require_current();
  p->apply_master(loan.native_minimized_word()!=0);p->processed=sequence;
  p->validate(sequence);loan.require_current();
 }catch(...){p->retired.store(true);throw;}
}
void BnwAudioNativeSettingsOwner::require_current()const{
 const auto p=impl_;Impl::Call call(p);
 try{p->validate(p->processed);}catch(...){p->retired.store(true);throw;}
}
BnwAudioNativeCategoryFactor BnwAudioNativeSettingsOwner::category_factor(std::string_view borrowed)const{
 const auto p=impl_;Impl::Call call(p);const std::string name(borrowed);
 try{
  p->validate(p->processed);
  const auto factor=p->graph->category_effective(name);const auto master=p->graph->fields("MASTER");
  BnwAudioNativeCategoryFactor result{name,std::bit_cast<std::uint32_t>(factor),
      std::bit_cast<std::uint32_t>(master.current),p->percentages,p->processed};
  p->validate(p->processed);return result;
 }catch(...){p->retired.store(true);throw;}
}
}
#endif
