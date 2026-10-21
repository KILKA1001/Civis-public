#include "civ5/ui/civis_bank_audio_source_owner.hpp"
#ifdef _WIN32
#include "civ5/ui/selected_bnw_preview_host.hpp"
#include "civ5/ui/bnw_audio_native_rng_numeric.hpp"
#include "bnw_audio_knob_state_detail.hpp"
#include "bnw_audio_user_settings_input_detail.hpp"
#include "bnw_audio_native_filename_probes_detail.hpp"
#include <shlobj.h>
#include <atomic>
#include <bit>
#include <charconv>
#include <limits>
#include <utility>
#include <vector>
#include <map>
#include <mutex>
#include <set>
#include <thread>
namespace civ5::ui {
namespace {
void need(bool v,const char* why){if(!v)throw std::runtime_error(why);}
std::string basename(std::string_view text){
 std::string name(text);need(!name.empty()&&name.size()<=259,"Own bank audio basename exceeds native bounded subset");
 for(const char byte:name){const auto c=static_cast<unsigned char>(byte);
  need(c>=32&&c<127&&c!='/'&&c!='\\'&&c!=':',"Own bank audio requires ordinary ASCII basename");}
 need(name!="."&&name!="..","Own bank audio basename traversal refused");return name;
}
std::string fold(std::string_view text){std::string v(text);for(auto &c:v)if(c>='A'&&c<='Z')c=static_cast<char>(c-'A'+'a');return v;}
const content::XmlNode* field(const content::XmlNode& n,std::string_view name){
 const content::XmlNode* value=nullptr;for(const auto &child:n.children)if(child.name==name){need(!value,"Own bank audio duplicate XML field");value=&child;}return value;
}
std::string scalar(const content::XmlNode& n,std::string_view key){
 const auto* p=field(n,key);
 if(!(p&&!p->text.empty()&&p->attributes.empty()&&p->children.empty()))
  throw std::runtime_error("Own bank audio required scalar missing/malformed: key="+std::string(key));
 return p->text;
}
std::string sound_load_type(const content::XmlNode& n){
 // Original858eab: only the genuinely absent child defaults to DYNAMIC_RES1.
 // field() keeps duplicate refusal; a present value keeps all scalar checks.
 if(!field(n,"LoadType"))return "DYNAMIC_RES";
 return scalar(n,"LoadType");
}
bool boolean(const content::XmlNode& n,std::string_view key){
 const auto* p=field(n,key);if(!p)return false;
 need(p->attributes.empty()&&p->children.empty(),"Own bank audio Boolean malformed");if(p->text.empty())return false;
 const auto v=fold(p->text);if(v=="true"||v=="1")return true;if(v=="false"||v=="0")return false;
 throw std::runtime_error("Own bank audio Boolean input unsupported");
}
std::filesystem::path settings_path(){
 wchar_t personal[MAX_PATH]{};
 need(SHGetSpecialFolderPathW(nullptr,personal,CSIDL_PERSONAL,FALSE)!=FALSE,
      "Own bank audio actual Documents selection failed");
 need(std::filesystem::is_directory(personal),"Own bank audio actual Documents missing");
 return std::filesystem::path(personal)/L"My Games"/L"Sid Meier's Civilization 5"/L"UserSettings.ini";
}
using Key=std::weak_ptr<assets::CivisResourceBankOwner>;
std::mutex registry_mutex;
std::map<Key,std::weak_ptr<CivisBankAudioSourceOwner>,std::owner_less<Key>> registry;
}
struct CivisBankAudioSourceOwner::Impl {
 const std::thread::id thread=std::this_thread::get_id();std::atomic<bool> retired{false};bool busy=false;
 const std::shared_ptr<assets::CivisResourceBankOwner> bank;
 const std::shared_ptr<const SelectedBnwPreviewHost> host;
 const assets::CivisResourceBankCapability capability;
 const std::string recipe_identity;
 std::map<std::string,BnwAudioNativeXmlDocument,std::less<>> xml;
 std::map<std::string,std::vector<CivisBankSoundData>,std::less<>> definitions;
 mutable std::map<std::string,BnwAudioInspectedVariationCount,std::less<>> counts;
 BnwAudioNativeUserSettings settings;std::array<std::int32_t,4> percentages{};
 std::unique_ptr<audio_settings_detail::KnobNumericGraph> knobs;
 std::int32_t cap=0;
 mutable audio_rng_detail::NativeAudioRngNumericFields rng;
 std::optional<BnwAudioWindowActivationLoan::SourceHandle> window;
 std::shared_ptr<const graphics::BnwOwnedCameraRendererWindowLease> physical;
 bool bound=false,cached_minimized=false,prepared=false;std::uint64_t processed=0;
 explicit Impl(std::shared_ptr<assets::CivisResourceBankOwner> b,std::shared_ptr<const SelectedBnwPreviewHost> selected)
  :bank(std::move(b)),host(std::move(selected)),capability(bank->capability()),recipe_identity(bank->source_recipe().recipe_identity){}
 struct Call{std::shared_ptr<Impl> p;explicit Call(std::shared_ptr<Impl> v):p(std::move(v)){
  need(p->thread==std::this_thread::get_id(),"Own bank audio foreign creator thread");need(!p->retired.load(),"Own bank audio retired");
  if(p->busy){p->retired.store(true);throw std::runtime_error("Own bank audio reentry");}p->busy=true;
 }~Call(){p->busy=false;}};
 void guard()const {
  need(!retired.load(),"Own bank audio retired");bank->require_current();capability.require_current();
  const auto canonical=bank->retain_owner();
  need(canonical.get()==bank.get()&&!canonical.owner_before(bank)&&!bank.owner_before(canonical),"Own bank audio Bank control changed");
  if(bound){physical->require_current();window->require_current();
   need(bank->actual_hwnd()==reinterpret_cast<std::uintptr_t>(window->window()),"Own bank audio Bank/actual HWND differ");
   need(window->observed_sequence()==processed,"Own bank audio actual activation unconsumed");}
  host->require_same_civis_resources(*bank);
  bank->require_boundary();
  need(bank->source_recipe().recipe_identity==recipe_identity,"Own bank audio recipe changed");
  if(prepared){need(settings_path()==settings.logical_path&&std::filesystem::weakly_canonical(settings.logical_path)==settings.selected_path,
      "Own bank audio selected settings path changed");
   const auto bytes=audio_settings_detail::read_settings_raw(settings.selected_path);
   const auto id=runtime::identify_bytes(bytes);
   need(id.bytes==settings.bytes&&id.sha256==settings.sha256,"Own bank audio selected settings changed");}
  if(bound){physical->require_current();window->require_current();
   need(window->observed_sequence()==processed,"Own bank audio activation changed during Bank boundary");}
  need(!retired.load(),"Own bank audio retired during source operation");
 }
 BnwAudioNativeXmlDocument capture_xml(const std::string& name){
  const auto key=assets::AssetKey(name);need(key.safe_relative_path(),"Own bank audio XML name unsafe");
  guard();bank->load(key,assets::CivisResourceParse::xml);guard();const auto loaded=capability.loaded(key);
  need(loaded.bytes&&loaded.xml&&loaded.payload_identity.bytes==loaded.bytes->size()&&loaded.bytes->size()<=8ULL*1024*1024,
       "Own bank audio actual XML cache missing/bounded incorrectly");
  return {name,loaded.payload_identity.sha256,loaded.payload_identity.bytes,*loaded.xml};
 }
 BnwAudioNativeBasenameResult inspect(const std::string& name)const {
  (void)basename(name);guard();const auto key=assets::AssetKey(name);
  if(!bank->has_indexed(key)){guard();return BnwAudioNativeBasenameResult::inspected_miss;}
  // A present ambiguous winner or failed source read throws. Neither can
  // become MISS. Explicit IO/cache publication remains on this UI creator.
  bank->load(key,assets::CivisResourceParse::bytes);guard();
  const auto actual=capability.loaded(key);need(actual.bytes&&actual.payload_identity.bytes==actual.bytes->size(),
    "Own bank audio indexed payload did not become actual cached bytes");guard();
  return BnwAudioNativeBasenameResult::inspected_hit;
 }
 BnwAudioInspectedVariationCount variation(std::string_view text)const {
  auto stem=audio_probe_detail::require_stem(text);guard();const auto found=counts.find(stem);
  const auto sequence=bound?processed:0;
  if(found!=counts.end()){auto result=found->second;
   for(const auto &probe:result.inspected)need(inspect(probe.basename)==probe.result,"Own bank audio cached count namespace changed");
   result.actual_activation_sequence=sequence;guard();return result;}
  auto result=audio_probe_detail::collect(std::move(stem),cap,sequence,[&](const std::string& name){return inspect(name);});
  counts.emplace(result.queried_stem,result);guard();return result;
 }
 void prepare(){
  std::set<std::string> active;std::uint64_t total=0;
  const auto load=[&](const auto& self,const std::string& name,std::size_t depth)->void {
   need(depth<32&&xml.size()<512,"Own bank audio XML closure budget exceeded");const auto key=fold(name);
   need(!active.contains(key),"Own bank audio XML cycle");if(xml.contains(key))return;active.insert(key);
   auto file=capture_xml(name);need(file.bytes<=64ULL*1024*1024-total,"Own bank audio XML closure byte budget exceeded");total+=file.bytes;
   if(const auto* includes=field(file.document,"IncludeXmlFiles"))for(const auto &child:includes->children){
    need(child.name=="IncludeXmlFile"&&!child.text.empty()&&child.attributes.empty()&&child.children.empty(),"Own bank audio include malformed");
    self(self,child.text,depth+1);
   }
   if(file.document.name=="AudioDefinesFile")if(const auto* sound=field(file.document,"SoundDatas"))for(const auto &row:sound->children){
    need(row.name=="SoundData","Own bank audio SoundData row malformed");
    try {
    CivisBankSoundData value{scalar(row,"SoundID"),scalar(row,"Filename"),sound_load_type(row),
      boolean(row,"bDontCache"),boolean(row,"bOnlyLoadOneVariationEachTime")};
    const auto id=fold(value.sound_id);definitions[id].push_back(std::move(value));
    } catch(const std::exception& error) {
     // Error-only metadata: no new resource lookup or default SoundData values.
     const content::XmlNode* sound_id=nullptr;bool unique_id=true;
     for(const auto& child:row.children)if(child.name=="SoundID"){
      if(sound_id)unique_id=false;sound_id=&child;
     }
     std::string diagnostic_id="<malformed/unavailable>";
     if(unique_id&&sound_id&&!sound_id->text.empty()&&sound_id->text.size()<=259&&
        sound_id->attributes.empty()&&sound_id->children.empty()){
      bool ordinary=true;for(const char byte:sound_id->text){
       const auto c=static_cast<unsigned char>(byte);if(c<32||c>=127)ordinary=false;
      }
      if(ordinary)diagnostic_id=sound_id->text;
     }
     throw std::runtime_error("Own bank audio SoundData preparation: xml="+file.basename+
      " row="+std::to_string(static_cast<std::size_t>(&row-sound->children.data()))+
      " SoundID="+diagnostic_id+" sha256="+file.sha256+" bytes="+std::to_string(file.bytes)+": "+error.what());
    }
   }
   xml.emplace(key,std::move(file));active.erase(key);
  };
  for(const auto* name:{"AudioParams.xml","Audio2DScripts.xml","Audio3DScripts.xml","AudioDefines.xml","ButtonPopupSounds.xml","NotificationSounds.xml"})load(load,name,0);
  const auto &parameters=xml.at("audioparams.xml").document;
  need(parameters.name=="AudioParams","Own bank audio parameters root differs");
  const auto* options=field(parameters,"GameOptions");const auto* cap_field=options?field(*options,"iMaxVariationCap"):nullptr;
  need(cap_field&&cap_field->attributes.empty()&&cap_field->children.empty()&&!cap_field->text.empty(),"Own bank audio variation cap missing/malformed");
  const auto parsed=std::from_chars(cap_field->text.data(),cap_field->text.data()+cap_field->text.size(),cap);
  need(parsed.ec==std::errc{}&&parsed.ptr==cap_field->text.data()+cap_field->text.size()&&cap>=0,"Own bank audio variation cap unsupported");
  const auto* types=field(parameters,"SoundTypes");need(types,"Own bank audio SoundTypes missing");
  settings.logical_path=settings_path();settings.selected_path=std::filesystem::weakly_canonical(settings.logical_path);
  const auto bytes=audio_settings_detail::read_settings_raw(settings.selected_path);const auto id=runtime::identify_bytes(bytes);
  settings.sha256=id.sha256;settings.bytes=id.bytes;settings.raw=std::string(bytes.begin(),bytes.end());
  percentages=audio_settings_detail::parse_audio_settings_existing_subset(settings.raw);
  knobs=std::make_unique<audio_settings_detail::KnobNumericGraph>(*types);knobs->load_four_saved_percentages(percentages);
  const auto def=definitions.find("snd_interface_new_era");need(def!=definitions.end(),"Own bank audio actual admission SoundData missing");
  need(def->second.size()==1,"Own bank audio admission SoundData selected replacement ambiguous");
  const auto admission=variation(def->second.front().filename);
  need(admission.count>0&&admission.selected_extension==".wav","Own bank audio admission WAV absent/unsupported codec");
  prepared=true;guard();
 }
 void apply_master(std::uint16_t minimized){
  const bool now=minimized!=0;if(now!=cached_minimized)knobs->direct("MASTER",cached_minimized?1.0f:0.0f);cached_minimized=now;
 }
};
CivisBankAudioSourceOwner::CivisBankAudioSourceOwner(std::shared_ptr<Impl> p):impl_(std::move(p)){}
CivisBankAudioSourceOwner::~CivisBankAudioSourceOwner(){impl_->retired.store(true);}
std::shared_ptr<CivisBankAudioSourceOwner> CivisBankAudioSourceOwner::prepare_from_selected_host(
 const std::shared_ptr<const SelectedBnwPreviewHost>& supplied){
 need(bool(supplied),"Own bank audio selected Host absent");
 const auto host=supplied->retain_civis_resource_host_owned();
 need(host&&host.get()==supplied.get()&&!host.owner_before(supplied)&&!supplied.owner_before(host),"Own bank audio selected Host control forged");
 need(host->uses_civis_resource_bank(),"Own bank audio requires actual selected own-Bank Host");
 const auto borrowed=host->civis_resource_bank_;need(bool(borrowed),"Own bank audio Host Bank absent");
 const auto bank=borrowed->retain_owner();
 need(bank.get()==borrowed.get()&&!bank.owner_before(borrowed)&&!borrowed.owner_before(bank),"Own bank audio Host Bank control forged");
 host->require_same_civis_resources(*bank);
 std::shared_ptr<CivisBankAudioSourceOwner> result;bool initialize=false;
 {std::lock_guard lock(registry_mutex);
  for(auto it=registry.begin();it!=registry.end();)if(it->first.expired())it=registry.erase(it);else ++it;
  const auto at=registry.find(Key(bank));
  if(at!=registry.end()){result=at->second.lock();need(bool(result),"Own Bank audio context lost/retired; constructor reseed refused");}
 }
 // Impl's capability/recipe reads are actual Bank operations and therefore
 // MUST run outside the global context registry lock.
 if(!result){auto candidate=std::shared_ptr<CivisBankAudioSourceOwner>(new CivisBankAudioSourceOwner(std::make_shared<Impl>(bank,host)));
  candidate->factory_owner_=candidate;
  std::lock_guard lock(registry_mutex);const auto [at,inserted]=registry.emplace(Key(bank),candidate);
  need(inserted,"Own bank audio concurrent/reentrant context publication");(void)at;result=std::move(candidate);initialize=true;
 }
 const auto p=result->impl_;Impl::Call call(p);
 try{p->guard();if(initialize)p->prepare();need(p->prepared,"Own bank audio preparation incomplete");
  host->require_same_civis_resources(*bank);p->guard();return result;
 }catch(...){p->retired.store(true);throw;}
}
std::shared_ptr<CivisBankAudioSourceOwner> CivisBankAudioSourceOwner::retain_factory_owner(const std::shared_ptr<CivisBankAudioSourceOwner>& supplied)const {
 const auto actual=factory_owner_.lock();need(actual&&actual.get()==this&&supplied.get()==this&&!actual.owner_before(supplied)&&!supplied.owner_before(actual),"Own bank audio source control forged");return actual;
}
std::shared_ptr<assets::CivisResourceBankOwner> CivisBankAudioSourceOwner::retained_bank()const {require_current();return impl_->bank;}
void CivisBankAudioSourceOwner::require_current()const {const auto p=impl_;Impl::Call call(p);try{p->guard();}catch(...){p->retired.store(true);throw;}}
void CivisBankAudioSourceOwner::bind_first_actual_activation(const BnwAudioWindowActivationLoan& borrowed,
 const graphics::BnwOwnedCameraRendererWindowLease::Loan& physical){
 const auto loan=borrowed;const auto p=impl_;Impl::Call call(p);
 try{loan.require_current();need(loan.sequence()==1,"Own bank audio first activation required");
  const auto actual=physical.retain_owner();loan.require_current();physical.require_current();
  need(actual&&physical.window()==reinterpret_cast<std::uintptr_t>(loan.window())&&physical.ui_thread()==loan.ui_thread()&&
       p->bank->actual_hwnd()==physical.window(),"Own bank audio Bank/WM_ACTIVATE/physical sources differ");
  if(p->bound){p->window->require_same_source(loan.source_handle());p->guard();}
  else{p->guard();p->physical=actual;p->window.emplace(loan.source_handle());p->processed=1;p->bound=true;
   p->apply_master(loan.native_minimized_word());p->guard();}
  loan.require_current();physical.require_current();
 }catch(...){p->retired.store(true);throw;}
}
void CivisBankAudioSourceOwner::require_same_actual_window(const BnwAudioWindowActivationLoan::SourceHandle& window)const {
 const auto p=impl_;Impl::Call call(p);try{need(p->bound,"Own bank audio not activated");p->guard();p->window->require_same_source(window);p->guard();}catch(...){p->retired.store(true);throw;}
}
void CivisBankAudioSourceOwner::consume_actual_activation(const BnwAudioWindowActivationLoan& borrowed){
 const auto loan=borrowed;const auto p=impl_;Impl::Call call(p);
 try{need(p->bound,"Own bank audio not activated");loan.require_current();p->window->require_same_source(loan.source_handle());
  need(p->processed<std::numeric_limits<std::uint64_t>::max()&&loan.sequence()==p->processed+1,"Own bank audio missed/replayed activation");
  // The authentic window source has advanced. Admit exactly this loan before
  // source validation; failure retires the owner, never rolls back history.
  p->processed=loan.sequence();p->guard();p->apply_master(loan.native_minimized_word());p->guard();loan.require_current();
 }catch(...){p->retired.store(true);throw;}
}
BnwAudioNativeXmlDocument CivisBankAudioSourceOwner::read_audio_xml(std::string_view borrowed)const {
 const std::string name(borrowed);const auto p=impl_;Impl::Call call(p);p->guard();const auto at=p->xml.find(fold(name));
 need(at!=p->xml.end(),"Own bank audio XML outside actual prepared closure");const auto actual=p->capability.loaded(assets::AssetKey(at->second.basename));
 need(actual.xml&&actual.payload_identity.sha256==at->second.sha256&&actual.payload_identity.bytes==at->second.bytes,"Own bank audio cached XML identity changed");
 p->guard();auto result=at->second;result.basename=name;return result;
}
BnwAudioNativeWavPayload CivisBankAudioSourceOwner::read_audio_wav(std::string_view borrowed)const {
 const auto name=basename(borrowed);const auto p=impl_;Impl::Call call(p);p->guard();
 need(p->inspect(name)==BnwAudioNativeBasenameResult::inspected_hit,"Own bank audio WAV missing");
 const auto actual=p->capability.loaded(assets::AssetKey(name));need(bool(actual.bytes),"Own bank audio WAV bytes absent");
 need(actual.bytes->size()<=64ULL*1024*1024,"Own bank audio WAV payload budget exceeded");
 BnwAudioNativeWavPayload result{name,actual.payload_identity.sha256,*actual.bytes};p->guard();return result;
}
BnwAudioNativeBasenameResult CivisBankAudioSourceOwner::inspect_audio_basename(std::string_view borrowed)const {
 const auto name=basename(borrowed);const auto p=impl_;Impl::Call call(p);return p->inspect(name);
}
BnwAudioNativeUserSettings CivisBankAudioSourceOwner::own_selected_user_settings()const {const auto p=impl_;Impl::Call call(p);p->guard();return p->settings;}
CivisBankSoundData CivisBankAudioSourceOwner::sound_definition(std::string_view borrowed)const {
 const std::string id(borrowed);const auto p=impl_;Impl::Call call(p);p->guard();const auto at=p->definitions.find(fold(id));
 need(at!=p->definitions.end()&&at->second.size()==1,"Own bank audio cached SoundData absent/selected replacement ambiguous");return at->second.front();
}
BnwAudioInspectedVariationCount CivisBankAudioSourceOwner::inspect_variation_count(std::string_view borrowed)const {
 const std::string stem(borrowed);const auto p=impl_;Impl::Call call(p);need(p->bound,"Own bank audio count requires activation");return p->variation(stem);
}
BnwAudioNativeCategoryFactor CivisBankAudioSourceOwner::category_factor(std::string_view borrowed)const {
 const std::string name(borrowed);const auto p=impl_;Impl::Call call(p);need(p->bound,"Own bank audio gain requires activation");p->guard();
 const auto factor=p->knobs->category_effective(name);const auto master=p->knobs->fields("MASTER");
 const auto result=BnwAudioNativeCategoryFactor{name,std::bit_cast<std::uint32_t>(factor),std::bit_cast<std::uint32_t>(master.current),p->percentages,p->processed};p->guard();return result;
}
std::uint32_t CivisBankAudioSourceOwner::draw_numeric_component(std::uint32_t bound)const {
 const auto p=impl_;Impl::Call call(p);need(p->bound,"Own bank audio RNG requires activation");p->guard();
 const auto result=audio_rng_detail::step_native_audio_rng_numeric(p->rng,bound);p->guard();return result;
}
BnwAudioConstructorState CivisBankAudioSourceOwner::constructor_state()const {
 const auto p=impl_;Impl::Call call(p);need(p->bound,"Own bank audio RNG requires activation");p->guard();
 return {{},p->rng.state,p->rng.draws,false}; // Real own constructor stream; Original startup still unverified.
}
}
#endif
