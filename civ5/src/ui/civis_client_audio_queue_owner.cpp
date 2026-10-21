#include "civ5/ui/civis_client_audio_queue_owner.hpp"
#ifdef _WIN32
#include <atomic>
#include <deque>
#include <map>
#include <mutex>
#include <thread>
#include <utility>
namespace civ5::ui {
namespace {
void need(bool v,const char* why){if(!v)throw std::runtime_error(why);}
using Key=std::weak_ptr<CivisStandaloneAudioOwner>;
std::mutex registry_mutex;
std::map<Key,std::weak_ptr<CivisClientAudioQueueOwner>,std::owner_less<Key>> registry;
}
struct CivisClientAudioQueueOwner::Impl {
 const std::thread::id thread=std::this_thread::get_id();bool busy=false,closed=false;
 const std::shared_ptr<CivisStandaloneAudioOwner> backend;
 const UiAudioCatalog catalog;UiAudioQueue incoming;
 std::deque<UiAudioEvent> waiting;bool voice=false;std::uint64_t last_source=0;
 CivisClientAudioShutdown shutdown;
 explicit Impl(std::shared_ptr<CivisStandaloneAudioOwner> b):backend(std::move(b)),catalog(backend->event_catalog()),incoming(catalog){}
 struct Call{std::shared_ptr<Impl> p;explicit Call(std::shared_ptr<Impl> value):p(std::move(value)){
   need(p->thread==std::this_thread::get_id(),"Civis client audio foreign creator thread");need(!p->busy,"Civis client audio reentry");p->busy=true;
  }~Call(){p->busy=false;}};
 bool reserve(std::uint64_t sequence){
  need(!closed,"Civis client audio queue closed");backend->require_current();
  if(!sequence||sequence<=last_source)return false;
  last_source=sequence;
  need(incoming.pending()+waiting.size()+(voice?1U:0U)<64,"Civis client audio backlog full; source event consumed");return true;
 }
};
CivisClientAudioQueueOwner::CivisClientAudioQueueOwner(std::shared_ptr<Impl> p):impl_(std::move(p)){}
CivisClientAudioQueueOwner::~CivisClientAudioQueueOwner(){
 try{(void)close();}catch(...){} //Actual backend destructor still joins if source/thread teardown forbids explicit close.
}
std::shared_ptr<CivisClientAudioQueueOwner> CivisClientAudioQueueOwner::from_first_actual_activation(
 std::shared_ptr<const BnwAudioCompletedResourceOwner> resource,const BnwAudioWindowActivationLoan& borrowed,
 const graphics::BnwOwnedCameraRendererWindowLease::Loan& physical){
 const auto loan=borrowed;const auto backend=CivisStandaloneAudioOwner::from_first_actual_activation(std::move(resource),loan,physical);
 //Factory returned the canonical real object; no caller facade/control block.
 std::shared_ptr<CivisClientAudioQueueOwner> result;
 {std::lock_guard lock(registry_mutex);
  for(auto it=registry.begin();it!=registry.end();)if(it->first.expired())it=registry.erase(it);else ++it;
  const auto found=registry.find(Key(backend));if(found!=registry.end()){
   result=found->second.lock();need(bool(result),"Civis client audio queue lost/retired; replay restart refused");
  }
 }
 //Catalog reads have real callbacks: none execute under the registry mutex.
 if(!result){auto candidate=std::shared_ptr<CivisClientAudioQueueOwner>(new CivisClientAudioQueueOwner(std::make_shared<Impl>(backend)));
  std::lock_guard lock(registry_mutex);const auto [at,inserted]=registry.emplace(Key(backend),candidate);
  need(inserted,"Civis client audio concurrent/reentrant queue publication");(void)at;result=std::move(candidate);
 }
 backend->require_current();loan.require_current();physical.require_current();need(!result->impl_->closed,"Civis client audio existing queue closed");return result;
}
std::shared_ptr<CivisClientAudioQueueOwner> CivisClientAudioQueueOwner::from_first_actual_activation(
 std::shared_ptr<CivisBankAudioSourceOwner> resource,const BnwAudioWindowActivationLoan& borrowed,
 const graphics::BnwOwnedCameraRendererWindowLease::Loan& physical){
 const auto loan=borrowed;const auto backend=CivisStandaloneAudioOwner::from_first_actual_activation(std::move(resource),loan,physical);
 //Factory returned the canonical real object; no caller facade/control block.
 std::shared_ptr<CivisClientAudioQueueOwner> result;
 {std::lock_guard lock(registry_mutex);
  for(auto it=registry.begin();it!=registry.end();)if(it->first.expired())it=registry.erase(it);else ++it;
  const auto found=registry.find(Key(backend));if(found!=registry.end()){
   result=found->second.lock();need(bool(result),"Civis client audio queue lost/retired; replay restart refused");
  }
 }
 //Catalog reads have real callbacks: none execute under the registry mutex.
 if(!result){auto candidate=std::shared_ptr<CivisClientAudioQueueOwner>(new CivisClientAudioQueueOwner(std::make_shared<Impl>(backend)));
  std::lock_guard lock(registry_mutex);const auto [at,inserted]=registry.emplace(Key(backend),candidate);
  need(inserted,"Civis client audio concurrent/reentrant queue publication");(void)at;result=std::move(candidate);
 }
 backend->require_current();loan.require_current();physical.require_current();need(!result->impl_->closed,"Civis client audio existing queue closed");return result;
}
LiveUiAudioResult CivisClientAudioQueueOwner::transition(std::uint64_t sequence,UiAudioKind kind,std::string_view borrowed){
 const std::string id(borrowed);const auto p=impl_;Impl::Call call(p);
 if(id.empty()||(kind!=UiAudioKind::popup&&kind!=UiAudioKind::notification)||!sequence)return {};
 if(!p->reserve(sequence))return {LiveUiAudioDisposition::duplicate_or_stale,UiAudioEmitStatus::unknown,{}};
 //Queue chooses the native SelectedRead mapping and authored ordinal itself.
 return {LiveUiAudioDisposition::processed,p->incoming.emit(kind,id),{}};
}
LiveUiAudioResult CivisClientAudioQueueOwner::script_transition(std::uint64_t sequence,std::string_view borrowed_context,std::string_view borrowed_script){
 const std::string context(borrowed_context),script(borrowed_script);const auto p=impl_;Impl::Call call(p);
 if(!sequence||context.empty()||script.empty())return {};
 if(!p->reserve(sequence))return {LiveUiAudioDisposition::duplicate_or_stale,UiAudioEmitStatus::unknown,{}};
 return {LiveUiAudioDisposition::processed,p->incoming.emit_script(context,script),{}};
}
std::vector<CivisAudioEventResult> CivisClientAudioQueueOwner::pump(){
 const auto p=impl_;Impl::Call call(p);need(!p->closed,"Civis client audio queue closed");p->backend->require_current();
 std::vector<CivisAudioEventResult> results;
 if(p->voice){const auto completion=p->backend->poll();need(completion.has_value(),"Civis client real voice correlation disappeared");results.push_back(*completion);
  if(completion->status==CivisAudioEventStatus::pending)return results;
  need(completion->status==CivisAudioEventStatus::completed||completion->status==CivisAudioEventStatus::failed,"Civis client voice returned unexpected completion");p->voice=false;
  if(completion->status==CivisAudioEventStatus::failed){
   //Keep the real failed release receipt; dropping backlog is explicit and
   //cannot reseed/reissue a new context for the same actual Parent lifetime.
   p->shutdown.discarded_unsubmitted_events=p->incoming.drain().size()+p->waiting.size();p->waiting.clear();
   p->shutdown.backend=p->backend->close();p->closed=true;return results;
  }
 }
 for(auto& event:p->incoming.drain())p->waiting.push_back(std::move(event));
 //At most one submission per client pump; rejected authored entries remain
 //visible to the client and are consumed, never retried after partial RNG.
 if(!p->waiting.empty()){
  const auto event=p->waiting.front();p->waiting.pop_front();auto submitted=p->backend->play(event);
  p->voice=submitted.status==CivisAudioEventStatus::queued;results.push_back(std::move(submitted));
 }
 return results;
}
void CivisClientAudioQueueOwner::consume_actual_activation(const BnwAudioWindowActivationLoan& borrowed){
 const auto loan=borrowed;const auto p=impl_;Impl::Call call(p);need(!p->closed,"Civis client audio queue closed");p->backend->consume_actual_activation(loan);
}
CivisClientAudioShutdown CivisClientAudioQueueOwner::close(){
 const auto p=impl_;Impl::Call call(p);if(p->closed)return p->shutdown;
 p->shutdown.discarded_unsubmitted_events=p->incoming.drain().size()+p->waiting.size();p->waiting.clear();
 p->shutdown.backend=p->backend->close();p->voice=false;p->closed=true;return p->shutdown;
}
CivisAudioStartup CivisClientAudioQueueOwner::startup()const{const auto p=impl_;Impl::Call call(p);need(!p->closed,"Civis client audio queue closed");return p->backend->startup();}
std::uint64_t CivisClientAudioQueueOwner::last_source_sequence()const{const auto p=impl_;Impl::Call call(p);return p->last_source;}
}
#endif
