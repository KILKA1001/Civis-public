#include "civ5/ui/civis_standalone_audio_owner.hpp"
#ifdef _WIN32
#include "civ5/ui/civis_bank_audio_source_owner.hpp"
#include "civ5/ui/bnw_audio_constructor_rng_owner.hpp"
#include "civ5/ui/bnw_audio_native_settings_owner.hpp"
#include "civ5/ui/bnw_audio_native_filename_probes.hpp"
#include "civ5/ui/ui_winmm_audio.hpp"
#include "civis_standalone_audio_wave_detail.hpp"
#include <atomic>
#include <bit>
#include <charconv>
#include <map>
#include <mutex>
#include <set>
#include <thread>
#include <utility>
#include <mmsystem.h>
namespace civ5::ui {
namespace {
void need(bool v,const char* why){if(!v)throw std::runtime_error(why);}
std::string folded(std::string_view value){std::string s(value);for(auto& c:s)if(c>='A'&&c<='Z')c=static_cast<char>(c-'A'+'a');return s;}
const content::XmlNode* unique(const content::XmlNode& n,std::string_view key){
 const content::XmlNode* found=nullptr;for(const auto& child:n.children)if(child.name==key){need(!found,"Civis audio duplicate scalar/container");found=&child;}return found;
}
std::string scalar(const content::XmlNode& n,std::string_view key){
 const auto* f=unique(n,key);need(f&&!f->text.empty()&&f->attributes.empty()&&f->children.empty(),"Civis audio required authored scalar missing/malformed");return f->text;
}
int integer(const content::XmlNode& n,std::string_view key,int fallback){
 const auto* f=unique(n,key);if(!f)return fallback;
 need(f->attributes.empty()&&f->children.empty()&&!f->text.empty(),"Civis audio integer malformed");
 int v=0;const auto [end,error]=std::from_chars(f->text.data(),f->text.data()+f->text.size(),v);
 need(error==std::errc{}&&end==f->text.data()+f->text.size(),"Civis audio integer conversion unsupported");return v;
}
bool boolean(const content::XmlNode& n,std::string_view key){
 const auto* f=unique(n,key);if(!f||f->text.empty())return false;
 need(f->attributes.empty()&&f->children.empty(),"Civis audio Boolean malformed");
 const auto value=folded(f->text);if(value=="true"||value=="1")return true;if(value=="false"||value=="0")return false;
 throw std::runtime_error("Civis audio Boolean conversion unsupported");
}
const char* root_name(BnwAudioXmlKind kind){switch(kind){
 case BnwAudioXmlKind::parameters:return "AudioParams";
 case BnwAudioXmlKind::scripts_2d:return "Script2DFile";
 case BnwAudioXmlKind::scripts_3d:return "Script3DFile";
 case BnwAudioXmlKind::definitions:return "AudioDefinesFile";}throw std::runtime_error("Civis audio registry kind invalid");}
using Parent=BnwNativePackageDiscoveryOwner;using ParentKey=std::weak_ptr<const Parent>;
using BankKey=std::weak_ptr<assets::CivisResourceBankOwner>;
std::map<BankKey,std::weak_ptr<CivisStandaloneAudioOwner>,std::owner_less<BankKey>> bank_registry;
std::mutex registry_mutex;
std::map<ParentKey,std::weak_ptr<CivisStandaloneAudioOwner>,std::owner_less<ParentKey>> registry;
}
struct CivisStandaloneAudioOwner::Impl {
 const std::thread::id thread=std::this_thread::get_id();std::atomic<bool> retired{false};bool busy=false;
 const std::shared_ptr<const BnwAudioCompletedResourceOwner> resource;
 const std::shared_ptr<CivisBankAudioSourceOwner> own_bank;
 const std::shared_ptr<const Parent> parent;const BnwAudioWindowActivationLoan::SourceHandle window;
 const std::shared_ptr<const graphics::BnwOwnedCameraRendererWindowLease> physical;
 std::shared_ptr<BnwAudioNativeSettingsOwner> settings;
 std::shared_ptr<const BnwAudioNativeFilenameProbes> probes;
 std::shared_ptr<const BnwAudioConstructorRngOwner> rng;
 std::unique_ptr<WinMmPcmSink> sink;
 struct Row {content::XmlNode node;std::string file;};
 std::vector<BnwAudioNativeXmlDocument> files;
 std::map<std::pair<BnwAudioXmlKind,std::string>,std::vector<Row>> rows;
 std::map<std::string,BnwAudioInspectedVariationCount,std::less<>> counts;
 UiAudioCatalog events;CivisAudioStartup ready;
 CivisAudioShutdown closed;
 std::uint64_t last_event=0;std::optional<CivisAudioEventResult> pending;WinMmAudioSnapshot before_voice;
 Impl(std::shared_ptr<const BnwAudioCompletedResourceOwner> r,std::shared_ptr<const Parent> p,
      BnwAudioWindowActivationLoan::SourceHandle w,std::shared_ptr<const graphics::BnwOwnedCameraRendererWindowLease> actual)
  :resource(std::move(r)),parent(std::move(p)),window(std::move(w)),physical(std::move(actual)){}
 Impl(std::shared_ptr<CivisBankAudioSourceOwner> own,
      BnwAudioWindowActivationLoan::SourceHandle w,std::shared_ptr<const graphics::BnwOwnedCameraRendererWindowLease> actual)
  :own_bank(std::move(own)),window(std::move(w)),physical(std::move(actual)){ready.own_resource_bank=true;}
 BnwAudioNativeXmlDocument read_xml(std::string_view name)const {
  return own_bank?own_bank->read_audio_xml(name):resource->read_native_audio_xml(name);
 }
 BnwAudioNativeWavPayload read_wav(std::string_view name)const {
  return own_bank?own_bank->read_audio_wav(name):resource->read_native_audio_wav(name);
 }
 BnwAudioNativeBasenameResult inspect(std::string_view name)const {
  return own_bank?own_bank->inspect_audio_basename(name):resource->inspect_native_audio_basename(name);
 }
 BnwAudioNativeCategoryFactor factor(std::string_view name)const {
  return own_bank?own_bank->category_factor(name):settings->category_factor(name);
 }
 std::uint32_t draw(std::uint32_t bound)const {
  return own_bank?own_bank->draw_numeric_component(bound):rng->draw_numeric_component(bound);
 }
 BnwAudioConstructorState numeric()const {
  return own_bank?own_bank->constructor_state():rng->constructor_state();
 }
 struct Call {std::shared_ptr<Impl> p;explicit Call(std::shared_ptr<Impl> value,bool closing=false):p(std::move(value)){
   need(p->thread==std::this_thread::get_id(),"Civis audio foreign creator thread");need(closing||!p->retired.load(),"Civis audio retired");
   if(p->busy){p->retired.store(true);throw std::runtime_error("Civis audio reentry");}p->busy=true;
  }~Call(){p->busy=false;}};
 void guard()const{
  need(!retired.load(),"Civis audio retired");physical->require_current();window.require_current();
  if(own_bank)own_bank->require_same_actual_window(window);else resource->require_same_actual_window(window);
  window.require_current();physical->require_current();
  need(!retired.load(),"Civis audio retired during supplier callback");
 }
 void validate_files()const{
  guard();for(const auto& old:files){const auto current=read_xml(old.basename);guard();
   need(current.basename==old.basename&&current.sha256==old.sha256&&current.bytes==old.bytes,"Civis selected audio XML changed");}
 }
 void load_registry(){
  std::set<std::pair<BnwAudioXmlKind,std::string>> active,visited;std::uint64_t bytes=0;std::size_t declared=0;
  const auto load=[&](const auto& self,BnwAudioXmlKind kind,const std::string& name,std::size_t depth)->void{
   need(depth<32&&declared<512,"Civis audio XML registry dependency budget exceeded");const auto key=std::pair{kind,folded(name)};
   need(!active.contains(key),"Civis audio XML include cycle");if(visited.contains(key))return;active.insert(key);++declared;
   guard();auto file=read_xml(name);guard();
   need(file.basename==name&&file.document.name==root_name(kind),"Civis native audio registry root/query differs");
   need(file.bytes<=64ULL*1024*1024-bytes,"Civis audio XML registry byte budget exceeded");bytes+=file.bytes;
   if(const auto* includes=unique(file.document,"IncludeXmlFiles"))for(const auto& child:includes->children){
    need(child.name=="IncludeXmlFile"&&!child.text.empty()&&child.attributes.empty()&&child.children.empty(),"Civis audio malformed include");
    self(self,kind,child.text,depth+1); //Previous genuine SelectedRead loan has returned.
   }
   const bool defs=kind==BnwAudioXmlKind::definitions;
   if(kind!=BnwAudioXmlKind::parameters){
    const auto* container=unique(file.document,defs?"SoundDatas":kind==BnwAudioXmlKind::scripts_2d?"Script2DSounds":"Script3DSounds");
    if(container)for(const auto& row:container->children){
     need(row.name==(defs?"SoundData":kind==BnwAudioXmlKind::scripts_2d?"Script2DSound":"Script3DSound"),"Civis audio row kind differs");
     rows[{kind,folded(scalar(row,defs?"SoundID":"ScriptID"))}].push_back({row,file.basename});
     if(defs)++ready.sound_definitions;else if(kind==BnwAudioXmlKind::scripts_2d)++ready.scripts_2d;else ++ready.scripts_3d;
    }
   }
   files.push_back(std::move(file));active.erase(key);visited.insert(key);
  };
  load(load,BnwAudioXmlKind::parameters,"AudioParams.xml",0);
  load(load,BnwAudioXmlKind::scripts_2d,"Audio2DScripts.xml",0);
  load(load,BnwAudioXmlKind::scripts_3d,"Audio3DScripts.xml",0);
  load(load,BnwAudioXmlKind::definitions,"AudioDefines.xml",0);
  guard();auto popup=read_xml("ButtonPopupSounds.xml");guard();
  auto notification=read_xml("NotificationSounds.xml");guard();
  events=UiAudioCatalog::from_documents(popup.document,notification.document);
  files.push_back(std::move(popup));files.push_back(std::move(notification));
  ready.selected_xml_files=files.size();ready.own_registry_loaded=true;
 }
 const Row& row(BnwAudioXmlKind kind,std::string_view id)const{
  const auto found=rows.find({kind,folded(id)});need(found!=rows.end(),"Civis audio authored ID missing");
  need(found->second.size()==1,"Civis audio duplicate ID winning rule not qualified");return found->second.front();
 }
 BnwAudioInspectedVariationCount count(std::string_view stem){
  guard();const auto existing=counts.find(stem);
  if(existing!=counts.end()){
   auto result=existing->second;for(const auto& old:result.inspected){const auto actual=inspect(old.basename);guard();
    need(actual==old.result,"Civis audio source count/cache changed");}
   result.actual_activation_sequence=window.observed_sequence();return result;
  }
  auto result=own_bank?own_bank->inspect_variation_count(stem):probes->inspect_uncached_variation_count(stem);guard();counts.emplace(std::string(stem),result);return result;
 }
 std::vector<std::string> wav_variations(const BnwAudioInspectedVariationCount& c)const{
  need(c.count>0&&c.selected_extension==".wav","Civis audio missing asset or codec unsupported (WAV only)");
  std::vector<std::string> result;for(const auto& inspected:c.inspected)
   if(inspected.result==BnwAudioNativeBasenameResult::inspected_hit&&inspected.basename.ends_with(c.selected_extension))result.push_back(inspected.basename);
  need(result.size()==c.count,"Civis source variation transcript/count differs");return result;
 }
 civis_audio_wave_detail::Decoded decoded(const std::string& basename,std::string& hash){
  guard();auto payload=read_wav(basename);guard();need(payload.basename==basename&&!payload.sha256.empty(),"Civis actual WAVE source identity differs");
  hash=payload.sha256;return civis_audio_wave_detail::decode(payload.bytes);
 }
 void admit_device(){
  const auto& script=row(BnwAudioXmlKind::scripts_2d,"AS2D_INTERFACE_NEW_ERA");
  const auto& definition=row(BnwAudioXmlKind::definitions,scalar(script.node,"SoundID"));
  const auto variations=wav_variations(count(scalar(definition.node,"Filename")));std::string hash;
  const auto audio=decoded(variations.front(),hash);WAVEFORMATEX format{};format.wFormatTag=WAVE_FORMAT_PCM;
  format.nChannels=audio.pcm.channels;format.wBitsPerSample=audio.pcm.bits_per_sample;format.nSamplesPerSec=audio.pcm.sample_rate;
  format.nBlockAlign=static_cast<WORD>(format.nChannels*(format.wBitsPerSample/8));
  need(format.nSamplesPerSec<=std::numeric_limits<DWORD>::max()/format.nBlockAlign,"Civis device byte-rate overflow");format.nAvgBytesPerSec=format.nSamplesPerSec*format.nBlockAlign;
  HWAVEOUT device=nullptr;const auto opened=waveOutOpen(&device,WAVE_MAPPER,&format,0,0,CALLBACK_NULL);
  need(opened==MMSYSERR_NOERROR&&device,"Civis actual Windows PCM device admission failed");
  const auto closed=waveOutClose(device);need(closed==MMSYSERR_NOERROR,"Civis actual Windows PCM device admission close failed");
  guard();sink=std::make_unique<WinMmPcmSink>(1);ready.own_device_format_admitted=true;
 }
};
CivisStandaloneAudioOwner::CivisStandaloneAudioOwner(std::shared_ptr<Impl> p):impl_(std::move(p)){}
CivisStandaloneAudioOwner::~CivisStandaloneAudioOwner(){
 //Explicit close reports device state. Destructor still joins/cancels the
 //closed sink on exceptional unwind without inventing a released receipt.
 impl_->retired.store(true);
}
std::shared_ptr<CivisStandaloneAudioOwner> CivisStandaloneAudioOwner::from_first_actual_activation(
 std::shared_ptr<const BnwAudioCompletedResourceOwner> supplied,const BnwAudioWindowActivationLoan& borrowed,
 const graphics::BnwOwnedCameraRendererWindowLease::Loan& actual){
 const auto loan=borrowed;need(bool(supplied),"Civis audio needs actual completed resource owner");
 const auto resource=supplied->retain_factory_owner(supplied);loan.require_current();need(loan.sequence()==1,"Civis audio first actual activation required");
 const auto physical=actual.retain_owner();loan.require_current();
 need(physical&&actual.window()==reinterpret_cast<std::uintptr_t>(loan.window())&&actual.ui_thread()==loan.ui_thread(),"Civis audio physical/window source differs");
 const auto window=loan.source_handle();resource->require_same_actual_window(window);loan.require_current();actual.require_current();
 const auto parent=resource->retained_native_parent_for_audio_startup();loan.require_current();actual.require_current();
 std::shared_ptr<CivisStandaloneAudioOwner> result;bool initialize=false;
 {std::lock_guard lock(registry_mutex);
  for(auto it=registry.begin();it!=registry.end();)if(it->first.expired())it=registry.erase(it);else ++it;
  const auto found=registry.find(ParentKey(parent));if(found!=registry.end()){
   result=found->second.lock();need(bool(result),"Civis audio existing Parent context lost/retired; restart refused");
  }else{result=std::shared_ptr<CivisStandaloneAudioOwner>(new CivisStandaloneAudioOwner(std::make_shared<Impl>(resource,parent,window,physical)));registry.emplace(ParentKey(parent),result);initialize=true;}
 }
 const auto p=result->impl_;Impl::Call call(p);
 try{p->window.require_same_source(window);p->guard();
  if(initialize){
   p->settings=BnwAudioNativeSettingsOwner::from_first_actual_activation(resource,loan,actual);
   p->probes=BnwAudioNativeFilenameProbes::from_actual_activation(resource,loan,actual);
   p->rng=BnwAudioConstructorRngOwner::from_actual_activation(resource,loan,actual);
   p->load_registry();p->validate_files();p->admit_device();
  }
  need(p->ready.own_registry_loaded&&p->ready.own_device_format_admitted&&p->sink,"Civis own audio startup incomplete");
  p->guard();loan.require_current();actual.require_current();return result;
 }catch(...){p->retired.store(true);throw;}
}
std::shared_ptr<CivisStandaloneAudioOwner> CivisStandaloneAudioOwner::from_first_actual_activation(
 std::shared_ptr<CivisBankAudioSourceOwner> supplied,const BnwAudioWindowActivationLoan& borrowed,
 const graphics::BnwOwnedCameraRendererWindowLease::Loan& actual){
 const auto loan=borrowed;need(bool(supplied),"Own Bank standalone actual source absent");
 const auto resource=supplied->retain_factory_owner(supplied);loan.require_current();
 need(loan.sequence()==1,"Own Bank standalone first actual activation required");
 const auto physical=actual.retain_owner();loan.require_current();actual.require_current();
 need(physical&&actual.window()==reinterpret_cast<std::uintptr_t>(loan.window())&&actual.ui_thread()==loan.ui_thread(),
      "Own Bank standalone actual physical/activation source differs");
 resource->bind_first_actual_activation(loan,actual);const auto window=loan.source_handle();
 resource->require_same_actual_window(window);const auto bank=resource->retained_bank();loan.require_current();actual.require_current();
 std::shared_ptr<CivisStandaloneAudioOwner> result;bool initialize=false;
 {std::lock_guard lock(registry_mutex);
  for(auto it=bank_registry.begin();it!=bank_registry.end();)if(it->first.expired())it=bank_registry.erase(it);else ++it;
  const auto at=bank_registry.find(BankKey(bank));
  if(at!=bank_registry.end()){result=at->second.lock();need(bool(result),"Own Bank standalone context lost/retired; reseed refused");}
  else{result=std::shared_ptr<CivisStandaloneAudioOwner>(new CivisStandaloneAudioOwner(std::make_shared<Impl>(resource,window,physical)));
   bank_registry.emplace(BankKey(bank),result);initialize=true;}
 }
 const auto p=result->impl_;Impl::Call call(p);
 try{p->window.require_same_source(window);p->guard();if(initialize){p->load_registry();p->validate_files();p->admit_device();}
  need(p->ready.own_registry_loaded&&p->ready.own_device_format_admitted&&p->sink,"Own Bank standalone startup incomplete");
  p->guard();loan.require_current();actual.require_current();return result;
 }catch(...){p->retired.store(true);throw;}
}
CivisAudioShutdown CivisStandaloneAudioOwner::close(){
 const auto p=impl_;need(p->thread==std::this_thread::get_id(),"Civis audio close foreign creator thread");
 if(p->closed.closed)return p->closed;Impl::Call call(p,true);
 //No source validation here: a destroyed/retired HWND cannot excuse leaving
 //a genuine waveOut buffer submitted. The closed worker owns cancellation.
 if(p->sink)p->closed.device=p->sink->close();
 const auto& s=p->closed.device;
 p->closed.device_released=!s.active&&s.queued==0&&s.last_error.empty()&&s.failed==0&&
    s.accepted==s.completed+s.canceled;
 p->pending.reset();p->closed.closed=true;p->retired.store(true);return p->closed;
}
void CivisStandaloneAudioOwner::require_current()const{const auto p=impl_;Impl::Call call(p);try{p->guard();}catch(...){p->retired.store(true);throw;}}
CivisAudioStartup CivisStandaloneAudioOwner::startup()const{const auto p=impl_;Impl::Call call(p);p->guard();return p->ready;}
UiAudioCatalog CivisStandaloneAudioOwner::event_catalog()const{const auto p=impl_;Impl::Call call(p);p->validate_files();return p->events;}
void CivisStandaloneAudioOwner::consume_actual_activation(const BnwAudioWindowActivationLoan& borrowed){
 const auto loan=borrowed;const auto p=impl_;Impl::Call call(p);
 try{if(p->own_bank){p->window.require_same_source(loan.source_handle());p->own_bank->consume_actual_activation(loan);}
  else{p->guard();p->window.require_same_source(loan.source_handle());p->settings->consume_actual_activation(loan);}
  p->guard();loan.require_current();}
 catch(...){p->retired.store(true);throw;}
}
CivisAudioEventResult CivisStandaloneAudioOwner::play(const UiAudioEvent& borrowed){
 const auto event=borrowed;const auto p=impl_;Impl::Call call(p);CivisAudioEventResult result;result.event=event;
 try{p->guard();need(event.sequence>p->last_event,"Civis audio zero/replayed/out-of-order event sequence");p->last_event=event.sequence;
  if(p->pending){result.status=CivisAudioEventStatus::busy;result.error="Civis audio single voice still pending; event consumed";return result;}
  p->validate_files();
  if(event.kind==UiAudioKind::explicit_script)need(!event.id.empty()&&!event.script_id.empty()&&!event.authored_ordinal,"Civis explicit audio event malformed");
  else{need(event.kind==UiAudioKind::popup||event.kind==UiAudioKind::notification,"Civis audio event kind invalid");
   const auto* mapping=p->events.find(event.kind,event.id);need(mapping&&mapping->script_id==event.script_id&&event.authored_ordinal==mapping->ordinal,"Civis actual UI audio table mapping differs");}
  const auto& script=p->row(BnwAudioXmlKind::scripts_2d,event.script_id);
  //This finite own profile plays one-shot2D. Unsupported authored effects
  //are explicit errors before RNG/voice, not successful omissions.
  for(const auto& f:script.node.children)need(f.name=="ScriptID"||f.name=="SoundID"||f.name=="SoundType"||f.name=="iMinVolume"||f.name=="iMaxVolume"||f.name=="bDontTriggerDuplicates"||f.name=="iDontPlayMoreThan","Civis audio authored 2D effect outside supported one-shot profile");
  (void)boolean(script.node,"bDontTriggerDuplicates");need(integer(script.node,"iDontPlayMoreThan",1)>0,"Civis audio authored voice limit unsupported");
  const auto minimum=integer(script.node,"iMinVolume",100),maximum=integer(script.node,"iMaxVolume",100);
  need(minimum>=0&&maximum>=minimum&&maximum<=100,"Civis audio authored volume unsupported");
  const auto& definition=p->row(BnwAudioXmlKind::definitions,scalar(script.node,"SoundID"));
  for(const auto& f:definition.node.children)need(f.name=="SoundID"||f.name=="Filename"||f.name=="LoadType"||f.name=="bDontCache"||f.name=="bOnlyLoadOneVariationEachTime","Civis authored SoundData effect outside supported PCM profile");
  if(p->own_bank){const auto cached=p->own_bank->sound_definition(scalar(script.node,"SoundID"));
   need(cached.filename==scalar(definition.node,"Filename")&&cached.load_type==scalar(definition.node,"LoadType")&&
        cached.dont_cache==boolean(definition.node,"bDontCache")&&
        cached.only_load_one_variation==boolean(definition.node,"bOnlyLoadOneVariationEachTime"),
        "Own Bank actual cached SoundData differs from selected registry");}
  const auto load=scalar(definition.node,"LoadType");need(load=="RESIDENT"||load=="DYNAMIC_RES","Civis streamed audio resource not implemented");
  //The own PCM buffer remains alive in the actual device until release.
  //No original resident/cache/Miles load receipt is fabricated.
  (void)boolean(definition.node,"bDontCache");need(!boolean(definition.node,"bOnlyLoadOneVariationEachTime"),"Civis one-variation native cache policy not implemented");
  const auto variations=p->wav_variations(p->count(scalar(definition.node,"Filename")));
  const auto factor=p->factor(scalar(script.node,"SoundType"));
  const auto selected=p->draw(static_cast<std::uint32_t>(variations.size()));
  need(selected<variations.size(),"Civis audio numeric variation outside source count");
  const auto volume=minimum+static_cast<int>(p->draw(static_cast<std::uint32_t>(maximum-minimum)));
  result.native_basename=variations[selected];auto audio=p->decoded(result.native_basename,result.native_sha256);
  //Category effective factor already includes every parent and MASTER.
  const float gain=std::bit_cast<float>(factor.factor_bits)*(static_cast<float>(volume)*0.01f);
  civis_audio_wave_detail::scale(audio,gain);const auto state=p->numeric();result.numeric_state=state.numeric_state;result.numeric_draws=state.numeric_draws;
  p->guard();p->before_voice=p->sink->snapshot();need(!p->before_voice.active&&p->before_voice.queued==0,"Civis own PCM sink unexpectedly busy");
  //SelectedRead identity is retained above; the unused internal VFS field
  //is empty. It is not an invented native AssetHandle/provenance grant.
  const UiPcmBuffer buffer{audio.pcm,std::move(audio.samples),{}};
  need(p->sink->enqueue(buffer)==WinMmEnqueueStatus::enqueued,"Civis actual PCM sink refused voice");
  result.status=CivisAudioEventStatus::queued;p->pending=result;p->guard();return result;
 }catch(const std::exception& error){result.status=CivisAudioEventStatus::failed;result.error=error.what();return result;}
}
std::optional<CivisAudioEventResult> CivisStandaloneAudioOwner::poll(){
 const auto p=impl_;Impl::Call call(p);p->guard();if(!p->pending)return std::nullopt;
 auto result=*p->pending;const auto now=p->sink->snapshot();
 if(now.failed!=p->before_voice.failed||now.canceled!=p->before_voice.canceled||!now.last_error.empty()){
  result.status=CivisAudioEventStatus::failed;result.error=now.last_error.empty()?"Civis actual voice failed/canceled":now.last_error;
  //An asynchronous OS failure terminates this exclusive voice context.
  //Reusing its sticky last_error would falsely complete/fail an unrelated
  //subsequent submitted buffer and lose its actual completion correlation.
  p->closed.device=p->sink->close();
  const auto& released=p->closed.device;
  p->closed.device_released=!released.active&&released.queued==0&&released.last_error.empty()&&released.failed==0&&
      released.accepted==released.completed+released.canceled;
  p->closed.closed=true;p->pending.reset();p->retired.store(true);
  return result; //Retired source cannot pass a new currentness/play grant.
 }else if(now.completed==p->before_voice.completed+1&&!now.active&&now.queued==0){
  need(now.accepted==p->before_voice.accepted+1,"Civis own completion accepted-count mismatch");result.status=CivisAudioEventStatus::completed;p->pending.reset();
 }else{need(now.completed==p->before_voice.completed,"Civis own completion correlation mismatch");result.status=CivisAudioEventStatus::pending;}
 p->guard();return result;
}
}
#endif
