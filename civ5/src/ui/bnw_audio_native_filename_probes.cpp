#include "civ5/ui/bnw_audio_native_filename_probes.hpp"
#ifdef _WIN32
#include "bnw_audio_native_filename_probes_detail.hpp"
#include <atomic>
#include <charconv>
#include <stdexcept>
#include <thread>
#include <utility>
namespace civ5::ui {
namespace {
using audio_probe_detail::need;
const content::XmlNode& one(const content::XmlNode& node,std::string_view name){
 const content::XmlNode* found=nullptr;
 for(const auto& c:node.children)if(c.name==name){need(!found,"Native audio probe duplicate cap field");found=&c;}
 need(found,"Native audio probe explicit cap field absent");return *found;
}
std::int32_t explicit_cap(const content::XmlNode& document){
 need(document.name=="AudioParams","Native audio probe AudioParams root differs");
 const auto& field=one(one(document,"GameOptions"),"iMaxVariationCap");
 need(field.attributes.empty()&&field.children.empty()&&!field.text.empty(),"Native audio probe explicit scalar cap required");
 std::int32_t cap=0;const auto parsed=std::from_chars(field.text.data(),field.text.data()+field.text.size(),cap);
 need(parsed.ec==std::errc{}&&parsed.ptr==field.text.data()+field.text.size()&&cap>=0,
      "Native audio probe authored cap numeric branch unsupported");return cap;
}
}
struct BnwAudioNativeFilenameProbes::Impl {
 const std::thread::id thread=std::this_thread::get_id();
 std::atomic<bool> retired{false};bool busy=false;
 const std::shared_ptr<const BnwAudioCompletedResourceOwner> resource;
 const BnwAudioWindowActivationLoan::SourceHandle window;
 const std::shared_ptr<const graphics::BnwOwnedCameraRendererWindowLease> physical;
 BnwAudioNativeXmlDocument parameters;std::int32_t cap=0;
 Impl(std::shared_ptr<const BnwAudioCompletedResourceOwner> r,BnwAudioWindowActivationLoan::SourceHandle w,
      std::shared_ptr<const graphics::BnwOwnedCameraRendererWindowLease> p):resource(std::move(r)),window(std::move(w)),physical(std::move(p)){}
 void check()const{need(!retired.load(),"Native audio filename probes retired");}
 struct Call{
  std::shared_ptr<Impl> p;
  explicit Call(std::shared_ptr<Impl> v):p(std::move(v)){
   need(p->thread==std::this_thread::get_id(),"Native audio filename probes foreign creator thread");p->check();
   if(p->busy){p->retired.store(true);throw std::runtime_error("Native audio filename probes reentry");}p->busy=true;
  }
  ~Call(){p->busy=false;}
 };
 void guard(std::uint64_t sequence)const{
  check();physical->require_current();check();window.require_current();check();
  need(sequence>0&&window.observed_sequence()==sequence,"Native audio filename probes actual activation changed");
  resource->require_same_actual_window(window);check();physical->require_current();check();window.require_current();check();
  need(window.observed_sequence()==sequence,"Native audio filename probes activation changed during source callback");
 }
 void validate(std::uint64_t sequence)const{
  guard(sequence);const auto now=resource->read_native_audio_xml("AudioParams.xml");guard(sequence);
  need(now.sha256==parameters.sha256&&now.bytes==parameters.bytes&&explicit_cap(now.document)==cap,
       "Native audio filename probes authored source/cap changed");
 }
};
BnwAudioNativeFilenameProbes::BnwAudioNativeFilenameProbes(std::shared_ptr<Impl> p):impl_(std::move(p)){}
BnwAudioNativeFilenameProbes::~BnwAudioNativeFilenameProbes(){impl_->retired.store(true);}
std::shared_ptr<const BnwAudioNativeFilenameProbes> BnwAudioNativeFilenameProbes::from_actual_activation(
 std::shared_ptr<const BnwAudioCompletedResourceOwner> supplied,const BnwAudioWindowActivationLoan& borrowed,
 const graphics::BnwOwnedCameraRendererWindowLease::Loan& actual_window){
 const auto loan=borrowed;need(bool(supplied),"Native audio filename probes actual resource owner absent");
 const auto resource=supplied->retain_factory_owner(supplied);
 loan.require_current();const auto sequence=loan.sequence();
 const auto physical=actual_window.retain_owner();loan.require_current();
 need(physical&&actual_window.window()==reinterpret_cast<std::uintptr_t>(loan.window())&&
      actual_window.ui_thread()==loan.ui_thread(),"Native audio filename probes physical/activation sources differ");
 actual_window.require_current();loan.require_current();
 auto p=std::make_shared<Impl>(resource,loan.source_handle(),physical);Impl::Call call(p);
 try{
  p->guard(sequence);loan.require_current();
  p->parameters=resource->read_native_audio_xml("AudioParams.xml");p->guard(sequence);loan.require_current();
  p->cap=explicit_cap(p->parameters.document);p->validate(sequence);loan.require_current();actual_window.require_current();
  return std::shared_ptr<const BnwAudioNativeFilenameProbes>(new BnwAudioNativeFilenameProbes(p));
 }catch(...){p->retired.store(true);throw;}
}
void BnwAudioNativeFilenameProbes::require_current()const{
 const auto p=impl_;Impl::Call call(p);
 try{p->window.require_current();const auto sequence=p->window.observed_sequence();p->validate(sequence);}
 catch(...){p->retired.store(true);throw;}
}
BnwAudioInspectedVariationCount BnwAudioNativeFilenameProbes::inspect_uncached_variation_count(std::string_view borrowed)const{
 const auto p=impl_;Impl::Call call(p);
 try{
  // Owned query before callback; this remains a query, not SoundData identity.
  auto stem=audio_probe_detail::require_stem(borrowed);
  p->window.require_current();const auto sequence=p->window.observed_sequence();p->validate(sequence);
  auto result=audio_probe_detail::collect(std::move(stem),p->cap,sequence,[&](const std::string& basename){
   p->guard(sequence);const auto actual=p->resource->inspect_native_audio_basename(basename);p->guard(sequence);return actual;
  });
  p->validate(sequence);return result;
 }catch(...){p->retired.store(true);throw;}
}
}
#endif
