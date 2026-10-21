#include "civ5/ui/bnw_audio_source_catalog.hpp"
#include "civ5/ui/selected_bnw_preview_host.hpp"
#include "civ5/runtime/file_identity.hpp"
#include "civ5/runtime/bnw_selected_user_options_owner.hpp"
#include "civ5/platform/windows_path.hpp"
#include <fstream>
#include <locale>
#include "bnw_native_completed_resource_bank_capability.hpp"
#include "bnw_native_completed_selected_read.hpp"
#include "civ5/ui/bnw_native_package_discovery_owner.hpp"
#include "civ5/ui/bnw_native_early_input_lease.hpp"
#include <atomic>
#include <set>
#include <string>
#include <string_view>
#include <stdexcept>
#include <tuple>
#include <utility>

namespace civ5::ui {
namespace {
constexpr std::uint64_t per_file=8ULL*1024*1024,total_bytes=64ULL*1024*1024;
constexpr std::size_t max_files=512,max_depth=32;
void need(bool b,const char* m){if(!b)throw std::runtime_error(m);}
std::string root_name(BnwAudioXmlKind k){
 switch(k){case BnwAudioXmlKind::definitions:return "AudioDefinesFile";case BnwAudioXmlKind::scripts_2d:return "Script2DFile";
 case BnwAudioXmlKind::scripts_3d:return "Script3DFile";case BnwAudioXmlKind::parameters:return "AudioParams";}
 throw std::runtime_error("Audio XML unsupported document kind");
}
std::string rows_name(BnwAudioXmlKind k){
 switch(k){case BnwAudioXmlKind::definitions:return "SoundDatas";case BnwAudioXmlKind::scripts_2d:return "Script2DSounds";
 case BnwAudioXmlKind::scripts_3d:return "Script3DSounds";case BnwAudioXmlKind::parameters:return "SoundTypes";}
 throw std::runtime_error("Audio XML unsupported document kind");
}
std::string row_name(BnwAudioXmlKind k){
 switch(k){case BnwAudioXmlKind::definitions:return "SoundData";case BnwAudioXmlKind::scripts_2d:return "Script2DSound";
 case BnwAudioXmlKind::scripts_3d:return "Script3DSound";case BnwAudioXmlKind::parameters:return "SoundType";}
 throw std::runtime_error("Audio XML unsupported document kind");
}
bool xml_whitespace_only(std::string_view text){
 for(const auto c:text)if(c!=' '&&c!='\t'&&c!='\r'&&c!='\n')return false;
 return true;
}
void validate_sound_type(const content::XmlNode& row,std::size_t depth,std::size_t& count){
 need(depth<32&&count<127,"Audio SoundTypes hierarchy budget exceeded");++count;
 need(row.name=="SoundType"&&xml_whitespace_only(row.text)&&row.attributes.size()==1&&row.attributes[0].first=="name"&&
      !row.attributes[0].second.empty(),"Audio SoundType name/hierarchy subset unsupported");
 for(const auto& child:row.children)validate_sound_type(child,depth+1,count);
}
auto source_key(const UiAudioSource& s){const auto& p=*s.provenance;return std::tuple{s.mount_id,p.kind,p.source_id,p.source_relative_path,p.archive_relative_path,s.selected_name};}
struct Captured {UiAudioSource source;assets::AssetHandle handle;std::string sha;};
Captured capture(const assets::ResourceMountTable& resources,const std::string& key){
 const assets::AssetKey requested(key);need(requested.safe_relative_path(),"Audio XML include path unsafe");
 const auto selected=resources.resolve(requested);const auto* h=std::get_if<assets::AssetHandle>(&selected.asset);
 need(h,"Audio XML selected stream missing or ambiguous");
 // Reject duplicates within the actual selected source, not lower-priority
 // alternatives; authoritative payload selection remains ResourceMountTable.
 std::set<std::pair<std::string,std::optional<std::string>>> matches;
 for(const auto& hit:resources.resolve_all(requested))if(hit.mount_id==selected.mount_id)
  if(const auto* candidate=std::get_if<assets::AssetHandle>(&hit.asset)){
   const auto& p=candidate->candidate().provenance;const auto& chosen=h->candidate().provenance;
   if(p.kind==chosen.kind&&p.source_id==chosen.source_id)matches.emplace(p.source_relative_path,p.archive_relative_path);
  }
 need(matches.size()==1,"Audio XML selected source has duplicate streams");
 const auto bytes=h->read_bytes(per_file);
 return {{key,h->candidate().original_name,selected.mount_id,h->candidate().provenance},*h,runtime::identify_bytes(bytes).sha256};
}
const content::XmlNode* unique(const content::XmlNode& node,std::string_view name){
 const content::XmlNode* result=nullptr;
 for(const auto& child:node.children)if(child.name==name){need(!result,"Audio XML duplicate required element");result=&child;}
 return result;
}
}
struct BnwAudioCompletedResourceOwner::Impl {
 const std::thread::id thread=std::this_thread::get_id();
 std::atomic<bool> retired{false};bool busy=false;
 std::shared_ptr<const detail::BnwNativeCompletedResourceBankCapability> cap;
 std::shared_ptr<const BnwNativePackageDiscoveryOwner> parent;
 std::shared_ptr<const detail::BnwNativeCompletedSelectedRead> leaf;
 void check()const{need(!retired.load(),"Completed audio resource owner retired");}
 struct Call {
  std::shared_ptr<Impl> p;
  explicit Call(std::shared_ptr<Impl> source):p(std::move(source)){
   need(p->thread==std::this_thread::get_id(),"Completed audio resource owner foreign thread");
   p->check();if(p->busy){p->retired.store(true);throw std::runtime_error("Completed audio resource owner reentry");}
   p->busy=true;
  }
  ~Call(){p->busy=false;}
 };
 void validate()const{
  check();if(parent){
   const auto canonical=parent->retain_factory_owner();
   need(canonical.get()==parent.get()&&!canonical.owner_before(parent)&&!parent.owner_before(canonical),
        "Completed audio actual Parent canonical identity changed");check();
  }
  cap->require_identity_current();check();
  if(leaf){need(bool(parent),"Completed audio native read actual Parent absent");
   leaf->require_same_parent(*parent);check();}
 }
};
BnwAudioCompletedResourceOwner::BnwAudioCompletedResourceOwner(std::shared_ptr<Impl> p):impl_(std::move(p)){}
BnwAudioCompletedResourceOwner::~BnwAudioCompletedResourceOwner(){impl_->retired.store(true);}
std::shared_ptr<const BnwAudioCompletedResourceOwner> BnwAudioCompletedResourceOwner::retain_factory_owner(
 const std::shared_ptr<const BnwAudioCompletedResourceOwner>& supplied)const{
 const auto genuine=factory_owner_.lock();
 need(genuine&&genuine.get()==this&&supplied.get()==this&&!genuine.owner_before(supplied)&&!supplied.owner_before(genuine),
      "Completed audio resource owner forged factory control block");return genuine;
}
std::shared_ptr<const BnwAudioCompletedResourceOwner> BnwAudioCompletedResourceOwner::from_completed_bank(
 std::shared_ptr<const detail::BnwNativeCompletedResourceBankCapability> supplied){
 need(bool(supplied),"Completed audio resource capability absent");
 // Callback-free canonical acquisition MUST precede callbackful identity.
 // The public enable_shared_from_this base belongs to the private real factory.
 const auto genuine=supplied->shared_from_this();
 need(genuine.get()==supplied.get()&&!genuine.owner_before(supplied)&&!supplied.owner_before(genuine),
      "Completed audio resource capability forged control block");
 genuine->require_identity_current(); // Creator-thread check precedes producer callbacks.
 auto p=std::make_shared<Impl>();p->cap=genuine;
 auto result=std::shared_ptr<BnwAudioCompletedResourceOwner>(new BnwAudioCompletedResourceOwner(p));
 result->factory_owner_=result;
 result->require_current();return result;
}
std::shared_ptr<const BnwAudioCompletedResourceOwner> BnwAudioCompletedResourceOwner::from_completed_bank(
 std::shared_ptr<const BnwNativePackageDiscoveryOwner> supplied_parent,
 std::shared_ptr<const detail::BnwNativeCompletedResourceBankCapability> supplied_cap){
 need(supplied_parent&&supplied_cap,"Completed native audio requires actual Parent and capability");
 // Creator-first callback-free73 canonical Parent; incoming control first.
 const auto parent=supplied_parent->retain_factory_owner();
 need(parent.get()==supplied_parent.get()&&!parent.owner_before(supplied_parent)&&!supplied_parent.owner_before(parent),
      "Completed native audio Parent forged control block");
 const auto cap=supplied_cap->shared_from_this();
 need(cap.get()==supplied_cap.get()&&!cap.owner_before(supplied_cap)&&!supplied_cap.owner_before(cap),
      "Completed native audio capability forged control block");
 auto p=std::make_shared<Impl>();p->parent=parent;p->cap=cap;
 p->validate(); // Both actual owners strong before the first source callback.
 const auto issued=parent->capture_native_owned_completed_selected_read(cap);
 need(bool(issued),"Actual Parent returned no completed selected read leaf");
 const auto canonical=issued->retain_owner();
 need(canonical.get()==issued.get()&&!canonical.owner_before(issued)&&!issued.owner_before(canonical),
      "Completed audio selected read differs from genuine factory control block");
 p->leaf=canonical;p->validate();
 auto result=std::shared_ptr<BnwAudioCompletedResourceOwner>(new BnwAudioCompletedResourceOwner(p));
 result->factory_owner_=result;result->require_current();return result;
}
std::shared_ptr<const BnwAudioCompletedResourceOwner> BnwAudioCompletedResourceOwner::from_registered_parent(
 std::shared_ptr<const BnwNativePackageDiscoveryOwner> supplied_parent){
 need(bool(supplied_parent),"Completed audio registered Parent absent");
 // Canonical creator-thread acquisition precedes any actual source callback.
 const auto parent=supplied_parent->retain_factory_owner();
 need(parent.get()==supplied_parent.get()&&!parent.owner_before(supplied_parent)&&!supplied_parent.owner_before(parent),
      "Completed audio registered Parent forged control block");
 std::shared_ptr<const BnwAudioCompletedResourceOwner> result;
 // The existing producer retains its already-issued weak inverse edge. Its
 // closed borrower refuses absent/unreturned/retired ownership. The consumer
 // cannot manufacture completion or repeat the original capture transition.
 parent->with_native_owned_completed_resource_owner([&](const auto& cap){
  need(!result,"Completed audio registered Bank delivered twice");
  result=from_completed_bank(parent,cap);
 });
 need(bool(result),"Completed audio registered Bank did not return a source owner");
 result->require_current();return result;
}
void BnwAudioCompletedResourceOwner::with_native_parameters_bytes(
 const std::function<void(std::span<const std::uint8_t>)>& input)const{
 const auto p=impl_;Impl::Call call(p);
 if(!p->leaf){
  try{p->validate();}catch(...){p->retired.store(true);throw;}
  // Absence of this separate read grant does not retire valid identity-only ownership.
  throw std::runtime_error("Completed audio XML requires genuine native SelectedRead/FileReader70 bounded bytes loan");
 }
 try{
  // COPY while busy BEFORE any callbackful source currentness can invalidate
  // caller input storage. Immutable leaf was checked above; no copied table.
  std::optional<std::function<void(std::span<const std::uint8_t>)>> target(input);
  p->check();p->validate();need(bool(*target),"Completed native audio byte consumer absent");
  std::size_t callbacks=0;
  p->leaf->with_selected_basename_bytes("AudioParams.xml",per_file,[&](std::span<const std::uint8_t> bytes){
   p->validate();need(callbacks++==0&&target&&bytes.size()<=per_file,"Completed native AudioParams must lend one bounded source");
   (*target)(bytes);p->validate();
   target.reset(); // Destroy caller COPY inside genuine selected read lender.
   p->validate();
  });need(callbacks==1&&!target,"Completed native AudioParams byte callback missing");p->validate();
 }catch(...){p->retired.store(true);throw;}
}
BnwAudioNativeWavPayload BnwAudioCompletedResourceOwner::read_native_audio_wav(std::string_view borrowed)const{
 const auto p=impl_;Impl::Call call(p);
 try{
  const std::string query(borrowed);
  need(query.size()>4&&query.size()<=255&&query.ends_with(".wav"),"Native audio WAV basename unsupported");
  for(const char byte:query){const auto ch=static_cast<unsigned char>(byte);
   need(ch>=32&&ch<127&&ch!='/'&&ch!='\\'&&ch!=':',"Native audio WAV basename is not ordinary ASCII");}
  p->validate();need(bool(p->leaf),"Native audio WAV payload requires genuine returned SelectedRead");
  constexpr std::uint64_t maximum=16ULL*1024*1024;
  BnwAudioNativeWavPayload result;result.basename=query;std::size_t callbacks=0;
  p->leaf->with_selected_basename_bytes(query,maximum,[&](std::span<const std::uint8_t> bytes){
   p->validate();need(callbacks++==0&&!bytes.empty()&&bytes.size()<=maximum,"Native audio WAV source callback/size differs");
   result.bytes.assign(bytes.begin(),bytes.end());result.sha256=runtime::identify_bytes(bytes).sha256;
   p->validate();
  });
  need(callbacks==1&&!result.sha256.empty(),"Native audio WAV source did not return one payload");p->validate();
  return result;
 }catch(...){p->retired.store(true);throw;}
}
std::shared_ptr<const BnwNativePackageDiscoveryOwner> BnwAudioCompletedResourceOwner::retained_native_parent_for_audio_startup()const{
 const auto p=impl_;Impl::Call call(p);
 try{p->validate();need(bool(p->parent)&&bool(p->leaf),"Audio constructor supplier needs genuine registered Parent and returned SelectedRead");
  const auto parent=p->parent;p->validate();return parent;
 }catch(...){p->retired.store(true);throw;}
}
std::shared_ptr<const BnwAudioCompletedResourceOwner> BnwAudioCompletedResourceOwner::from_selected_host(
 std::shared_ptr<const SelectedBnwPreviewHost> supplied){
 need(bool(supplied)&&bool(supplied->early_package_),"Civis audio selected Host lacks authentic early Parent");
 const auto parent=supplied->early_package_;parent->require_same_selected_host(*supplied);
 const auto result=from_registered_parent(parent);parent->require_same_selected_host(*supplied);result->require_current();return result;
}
void BnwAudioCompletedResourceOwner::require_current()const{
 const auto p=impl_;Impl::Call call(p);
 try{p->validate();}catch(...){p->retired.store(true);throw;}
}
BnwAudioNativeBasenameResult BnwAudioCompletedResourceOwner::inspect_native_audio_basename(std::string_view borrowed)const{
 const auto p=impl_;Impl::Call call(p);
 try{
  // Own ordinary basename before any source callback. No stripping a path.
  const std::string query(borrowed);
  need(!query.empty()&&query.size()<=259,"Native audio inspected basename bound unsupported");
  for(const unsigned char c:query)need(c>=32&&c<127&&c!='/'&&c!='\\'&&c!=':',
       "Native audio inspection requires ordinary ASCII basename");
  need(query!="."&&query!="..","Native audio inspection traversal refused");
  p->validate();need(p->parent&&p->leaf,"Native audio inspection requires actual completed Parent/read source");
  std::size_t operations=0,results=0;
  std::optional<BnwAudioNativeBasenameResult> inspected;
  // Real new Observer/Epoch/Cold/Header/Loaded/Bank operation. Do not use
  // unavailable cap resource-query loan or nested generic table resolution.
  p->parent->with_native_owned_completed_resource_operation(p->cap,[&]{
   p->check();need(operations++==0,"Native audio inspection repeated actual Bank entry");
   p->parent->require_native_owned_completed_resource_operation_current(*p->cap);p->check();
   p->parent->with_native_owned_completed_resource_record(p->cap,query.c_str(),[&](const auto* node,const auto* prefix,const auto* first){
    // Parent checks the actual native lender/source universe before/after this
    // CLOSED internal target, including target destruction inside Bank.
    p->check();p->parent->require_native_owned_completed_resource_operation_current(*p->cap);p->check();
    need(results++==0&&((node&&prefix)||(!node&&!prefix&&!first)),"Native audio inspection omitted genuine HIT/MISS shape");
    inspected=node?BnwAudioNativeBasenameResult::inspected_hit:BnwAudioNativeBasenameResult::inspected_miss;
    p->parent->require_native_owned_completed_resource_operation_current(*p->cap);p->check();
   });
   p->parent->require_native_owned_completed_resource_operation_current(*p->cap);p->check();
  });
  p->validate();need(operations==1&&results==1&&inspected.has_value(),"Native audio inspection omitted actual result/normal return");
  return *inspected;
 }catch(...){p->retired.store(true);throw;}
}
BnwAudioNativeUserSettings BnwAudioCompletedResourceOwner::read_native_audio_user_settings()const{
 const auto p=impl_;Impl::Call call(p);
 try{
  p->validate();need(p->parent&&p->leaf,"Native audio settings require actual completed Parent/read source");
  // Private Parent method returns the SAME previously captured Boot options.
  // No lazy Directory construction, path argument, fallback or parser claim.
  const auto options=p->parent->borrow_selected_user_options_for_source();p->validate();
  need(bool(options),"Native audio actual retained UserSettings owner absent");
  const auto before=options->source_receipt();p->validate();
  need(before.kind==runtime::BnwUserOptionsInputKind::existing_file&&before.file&&
       before.file->bytes<=1024ULL*1024,"Native audio requires existing bounded Boot-selected UserSettings file");
  need(std::filesystem::weakly_canonical(before.logical_path)==before.selected_path&&
       std::filesystem::is_regular_file(before.selected_path),"Native audio Boot-selected UserSettings target changed");
  std::vector<std::uint8_t> bytes(static_cast<std::size_t>(before.file->bytes));
  std::ifstream stream;stream.imbue(std::locale::classic());
  stream.open(platform::extended_windows_path(before.selected_path),std::ios::binary);
  need(bool(stream),"Native audio Boot-selected UserSettings stream absent");
  stream.read(reinterpret_cast<char*>(bytes.data()),static_cast<std::streamsize>(bytes.size()));
  need(stream.gcount()==static_cast<std::streamsize>(bytes.size())&&
       stream.peek()==std::char_traits<char>::eof()&&!stream.bad(),"Native audio UserSettings changed during exact read");
  stream.close();const auto measured=runtime::identify_bytes(bytes);
  need(measured.bytes==before.file->bytes&&measured.sha256==before.file->sha256,
       "Native audio UserSettings bytes differ from actual retained Boot source");
  options->require_current();p->validate();
  const auto again=p->parent->borrow_selected_user_options_for_source();p->validate();
  need(again&&again.get()==options.get()&&!again.owner_before(options)&&!options.owner_before(again),
       "Native audio retained Boot UserSettings owner changed");
  const auto after=again->source_receipt();p->validate();
  need(after.kind==before.kind&&after.logical_path==before.logical_path&&after.selected_path==before.selected_path&&
       after.file&&after.file->bytes==measured.bytes&&after.file->sha256==measured.sha256,
       "Native audio Boot-selected UserSettings epoch changed");
  return {before.logical_path,before.selected_path,measured.sha256,std::string(bytes.begin(),bytes.end()),measured.bytes};
 }catch(...){p->retired.store(true);throw;}
}
BnwAudioNativeXmlDocument BnwAudioCompletedResourceOwner::read_native_audio_xml(
 std::string_view borrowed)const{
 const auto p=impl_;Impl::Call call(p);
 try{
  // Copy borrowed query BEFORE callbackful source validation. This is only
  // the genuine leaf's basename grammar; unsupported paths are not stripped.
  const std::string query(borrowed);
  need(query.size()>4&&query.size()<=259&&query.ends_with(".xml"),"Native audio dependency must be an XML basename");
  for(const unsigned char byte:query)
   need(byte>32&&byte<127&&byte!='/'&&byte!='\\'&&byte!=':',"Native audio dependency basename grammar unsupported");
  need(query!="."&&query!="..","Native audio dependency traversal refused");
  p->check();p->validate();need(bool(p->leaf),"Native audio XML requires actual selected leaf");
  std::size_t callbacks=0;std::optional<BnwAudioNativeXmlDocument> parsed;
  // Closed internal callback: no caller functor COPY/destructor is retained
  // outside the genuine lender. Only owned data survives the returned loan.
  p->leaf->with_selected_basename_bytes(query,per_file,[&](std::span<const std::uint8_t> bytes){
   p->validate();need(callbacks++==0&&bytes.size()<=per_file,"Native audio XML must lend one actual bounded source");
   const auto hash=runtime::identify_bytes(bytes).sha256;
   auto document=content::read_xml_metadata(std::string(bytes.begin(),bytes.end()));
   parsed=BnwAudioNativeXmlDocument{query,hash,bytes.size(),std::move(document)};
   p->validate();
  });need(callbacks==1&&parsed.has_value(),"Native audio XML omitted actual bytes callback");p->validate();
  return std::move(*parsed);
 }catch(...){p->retired.store(true);throw;}
}
#ifdef _WIN32
void BnwAudioCompletedResourceOwner::require_same_actual_window(
 const BnwAudioWindowActivationLoan::SourceHandle& window)const{
 const auto p=impl_;Impl::Call call(p);
 try{
  window.require_current();p->check();p->validate();
  need(p->parent&&p->leaf,"Native audio window relation requires typed actual Parent/read owner");
  const auto early=p->parent->early_input_lease();p->check();
  need(bool(early),"Native audio actual Parent window source absent");
  // This is the genuine existing Early source getter and its unchanged gate,
  // not a guessed IsIconic value or a promoted completed-device capability.
  need(early->actual_hwnd()==reinterpret_cast<std::uintptr_t>(window.window()),"Native audio XML belongs to another actual Parent window");
  p->validate();window.require_current();p->check();
 }catch(...){p->retired.store(true);throw;}
}
#endif
void BnwAudioCompletedResourceOwner::with_resource_query_loan(
 const std::function<void(const assets::ResourceMountTable&)>& input)const{
 const auto p=impl_;Impl::Call call(p); // Same storage survives facade release during COPY/invoke/destruction.
 try{
  {
   // Own caller callback BEFORE callbackful producer currentness.
   const auto consume=input;p->check();p->validate();need(bool(consume),"Completed audio query consumer absent");
   p->cap->with_actual_resource_query_loan([&](const assets::ResourceMountTable& table){
    p->validate();consume(table);p->validate();
   });p->validate();
  }p->validate();
 }catch(...){p->retired.store(true);throw;}
}
struct BnwAudioSourceCatalog::Impl {
 const std::thread::id thread=std::this_thread::get_id();std::atomic<bool> retired{false};bool busy=false;
 std::shared_ptr<const assets::ResourceMountTable> resources; // Diagnostic path only.
 std::shared_ptr<const BnwAudioCompletedResourceOwner> completed;
 void with_resources(const std::function<void(const assets::ResourceMountTable&)>& consume)const{
  if(completed){
   completed->require_current();
   // A ResourceMountTable loan is not a native selected stream/read grant.
   // Stop BEFORE resolve/capture/AssetHandle's generic std::ifstream reader.
   throw std::runtime_error("Completed audio XML requires genuine native SelectedRead/FileReader70 bounded bytes loan");
  }else consume(*resources);
 }
 struct Layer {BnwAudioXmlKind kind;UiAudioXmlLayer xml;};
 struct SourcePin {UiAudioSource source;std::string sha;};
 std::vector<Layer> layers;std::vector<SourcePin> captured; // No native stream handle escapes its loan.
 std::optional<BnwAudioNativeParameters> native; // SAME completed holder owns canonical leaf; no diagnostic provenance.
 void entry(){need(std::this_thread::get_id()==thread,"Audio catalog foreign thread");need(!retired.load(),"Audio catalog retired");if(busy){retired.store(true);throw std::runtime_error("Audio catalog reentry");}}
};
BnwAudioSourceCatalog::BnwAudioSourceCatalog(std::shared_ptr<Impl> p):impl_(std::move(p)){}
std::shared_ptr<const BnwAudioSourceCatalog> BnwAudioSourceCatalog::retain_factory_owner(
    const std::shared_ptr<const BnwAudioSourceCatalog>& supplied) const {
    const auto genuine = factory_owner_.lock();
    need(genuine && genuine.get() == this && supplied.get() == this &&
         !genuine.owner_before(supplied) && !supplied.owner_before(genuine),
         "BnwAudioSourceCatalog supplied forged factory ownership");
    return genuine;
}
std::shared_ptr<const BnwAudioSourceCatalog> BnwAudioSourceCatalog::from_resources(
 std::shared_ptr<const assets::ResourceMountTable> resources,std::span<const BnwAudioXmlRoot> roots){
 need(bool(resources)&&!roots.empty(),"Audio catalog requires actual retained resources and declared roots");
 auto p=std::make_shared<Impl>();p->resources=std::move(resources);return build(p,roots);
}
std::shared_ptr<const BnwAudioSourceCatalog> BnwAudioSourceCatalog::from_completed_resources(
 std::shared_ptr<const BnwAudioCompletedResourceOwner> owner,std::span<const BnwAudioXmlRoot> roots){
 need(owner&&!roots.empty(),"Completed audio catalog requires issued owner and declared roots");
 owner=owner->retain_factory_owner(owner);
 const std::vector<BnwAudioXmlRoot> retained_roots(roots.begin(),roots.end());
 auto p=std::make_shared<Impl>();p->completed=std::move(owner);return build(p,retained_roots);
}
std::shared_ptr<const BnwAudioSourceCatalog> BnwAudioSourceCatalog::build(
 std::shared_ptr<Impl> p,std::span<const BnwAudioXmlRoot> roots){
 if(p->completed)return build_native_parameters(p,roots);
 p->busy=true;
 auto result=std::shared_ptr<BnwAudioSourceCatalog>(new BnwAudioSourceCatalog(p));
 result->factory_owner_=result; // Before the first resource callback; weak only.
 using Identity=decltype(source_key(std::declval<UiAudioSource>()));
 std::set<std::pair<BnwAudioXmlKind,Identity>> active,completed;std::uint64_t bytes_total=0;
 const auto load=[&](const assets::ResourceMountTable& resources,const auto& self,BnwAudioXmlKind kind,const std::string& key,std::size_t depth)->void{
  need(!p->retired.load(),"Audio catalog retired during resource lookup");
  need(depth<max_depth&&p->captured.size()+active.size()<max_files,"Audio XML dependency budget exceeded");
  auto file=capture(resources,key);const auto identity=std::pair{kind,source_key(file.source)};
  need(!active.contains(identity),"Audio XML include cycle");if(completed.contains(identity))return;
  active.insert(identity);
  const auto data=file.handle.read_bytes(per_file);
  need(runtime::identify_bytes(data).sha256==file.sha,"Audio XML changed during capture");
  need(data.size()<=total_bytes-bytes_total,"Audio XML aggregate byte budget exceeded");bytes_total+=data.size();
  auto document=content::read_xml_metadata(std::string(data.begin(),data.end()));
  need(document.name==root_name(kind),"Audio XML dependency has wrong document kind");
  // Enumerate actual IncludeXmlFiles declarations; never scan a Sounds tree.
  // Winner semantics for duplicate IDs remain unsupported downstream.
  if(const auto* includes=unique(document,"IncludeXmlFiles")){
   need(kind!=BnwAudioXmlKind::parameters,"AudioParams include activation order unsupported");
   for(const auto& child:includes->children){
   need(child.name=="IncludeXmlFile"&&!child.text.empty()&&child.children.empty(),"Audio XML malformed include declaration");
   self(resources,self,kind,child.text,depth+1);
   }
  }
  const auto* declared_rows=unique(document,rows_name(kind));
  if(kind==BnwAudioXmlKind::parameters){
   need(declared_rows&&xml_whitespace_only(declared_rows->text)&&declared_rows->attributes.empty(),"AudioParams SoundTypes missing or unsupported");
   std::size_t sound_type_count=0;
   for(const auto& row:declared_rows->children)validate_sound_type(row,0,sound_type_count);
  }
  if(kind!=BnwAudioXmlKind::parameters&&declared_rows)for(const auto& row:declared_rows->children){
   need(row.name==row_name(kind),"Audio XML unexpected row kind");
   for(const auto field:kind==BnwAudioXmlKind::definitions?std::array<std::string_view,2>{"SoundID","Filename"}:std::array<std::string_view,2>{"ScriptID","SoundID"}){
    const auto* value=unique(row,field);need(value&&!value->text.empty()&&value->children.empty(),"Audio XML required row field missing or ambiguous");
   }
  }
  auto xml=UiAudioXmlLayer{std::move(document),file.source};
  const auto current=capture(resources,key);
  need(!p->retired.load(),"Audio catalog source callback invalidated loading");
  need(source_key(current.source)==source_key(file.source)&&current.sha==file.sha,"Audio XML source changed after parse");
  p->captured.push_back({std::move(file.source),std::move(file.sha)});p->layers.push_back({kind,std::move(xml)});
  active.erase(identity);completed.insert(identity);
 };
 const std::vector<BnwAudioXmlRoot> retained_roots(roots.begin(),roots.end());
 try{p->with_resources([&](const assets::ResourceMountTable& resources){
  for(const auto& root:retained_roots)load(resources,load,root.kind,root.key,0);
 });need(!p->retired.load(),"Audio catalog source loan invalidated loading");p->busy=false;result->require_current();}
 catch(...){p->retired.store(true);p->busy=false;throw;}
 return result;
}
std::shared_ptr<const BnwAudioSourceCatalog> BnwAudioSourceCatalog::build_native_parameters(
 std::shared_ptr<Impl> p,std::span<const BnwAudioXmlRoot> roots){
 need(roots.size()==1&&roots[0].kind==BnwAudioXmlKind::parameters&&roots[0].key=="Assets/Sounds/XML/AudioParams.xml",
      "Completed native audio supports only the first AudioParams parameters root; other XML orders unavailable");
 p->busy=true;auto result=std::shared_ptr<BnwAudioSourceCatalog>(new BnwAudioSourceCatalog(p));
 result->factory_owner_=result; // Real factory seeded before callback; no self cycle.
 try{
  std::size_t callbacks=0;
  p->completed->with_native_parameters_bytes([&](std::span<const std::uint8_t> bytes){
   need(callbacks++==0&&!p->retired.load(),"Native AudioParams catalog source callback repeated or retired");
   const auto hash=runtime::identify_bytes(bytes).sha256;
   auto document=content::read_xml_metadata(std::string(bytes.begin(),bytes.end()));
   need(document.name=="AudioParams","Native AudioParams has wrong XML root");
   need(!unique(document,"IncludeXmlFiles"),"Native AudioParams include order unavailable");
   const auto* types=unique(document,"SoundTypes");
   need(types&&xml_whitespace_only(types->text)&&types->attributes.empty(),"Native AudioParams SoundTypes missing or unsupported");
   std::size_t count=0;for(const auto& row:types->children)validate_sound_type(row,0,count);
   p->native=BnwAudioNativeParameters{"AudioParams.xml",hash,*types};
   need(!p->retired.load(),"Native AudioParams catalog retired during parsing");
  });need(callbacks==1&&p->native&&!p->retired.load(),"Native AudioParams catalog omitted selected bytes");
  p->busy=false;validate_current(p);return result;
 }catch(...){p->retired.store(true);p->busy=false;throw;}
}
void BnwAudioSourceCatalog::require_current()const{const auto p=impl_;validate_current(p);}
void BnwAudioSourceCatalog::validate_current(const std::shared_ptr<Impl>& p){
 p->entry();p->busy=true;
 if(p->native){
  try{
   std::size_t callbacks=0;
   p->completed->with_native_parameters_bytes([&](std::span<const std::uint8_t> bytes){
    need(callbacks++==0&&!p->retired.load(),"Native AudioParams currentness callback repeated or retired");
    need(p->native->basename=="AudioParams.xml"&&runtime::identify_bytes(bytes).sha256==p->native->sha256,
         "Native AudioParams selected bytes changed");
   });need(callbacks==1&&!p->retired.load(),"Native AudioParams currentness omitted selected bytes");
   p->busy=false;return;
  }catch(...){p->retired.store(true);p->busy=false;throw;}
 }
 try{p->with_resources([&](const assets::ResourceMountTable& resources){for(const auto& before:p->captured){
  const auto now=capture(resources,before.source.requested_key);
  need(!p->retired.load(),"Audio catalog source callback invalidated currentness");
  need(source_key(now.source)==source_key(before.source)&&now.sha==before.sha,"Audio catalog source/mount/bytes changed");
 }});need(!p->retired.load(),"Audio catalog source loan invalidated currentness");p->busy=false;}
 catch(...){p->retired.store(true);p->busy=false;throw;}
}
std::vector<UiAudioXmlLayer> BnwAudioSourceCatalog::layers(BnwAudioXmlKind kind)const{
 const auto p=impl_;p->entry();need(!p->native,"Native AudioParams has no UiAudioSource provenance; use native_parameters");require_current();std::vector<UiAudioXmlLayer> result;for(const auto& layer:p->layers)if(layer.kind==kind)result.push_back(layer.xml);return result;
}
std::vector<BnwAudioSourceRow> BnwAudioSourceCatalog::rows(BnwAudioXmlKind kind,std::string_view id)const{
 const std::string owned_id(id); // Own borrowed caller bytes before ANY source callback.
 const auto p=impl_;p->entry();need(!p->native,"Native AudioParams has no generic script rows/source provenance");require_current();
 need(kind!=BnwAudioXmlKind::parameters,"AudioParams hierarchy requires retained layers, not script rows");
 std::vector<BnwAudioSourceRow> result;
 for(const auto& layer:p->layers)if(layer.kind==kind){
  std::vector<content::XmlNode> defaults;
  for(const auto& child:layer.xml.document.children)if(child.name.starts_with("Default"))defaults.push_back(child);
  if(const auto* rows=unique(layer.xml.document,rows_name(kind)))for(const auto& row:rows->children){
   const auto* key=unique(row,kind==BnwAudioXmlKind::definitions?"SoundID":"ScriptID");
   need(key&&!key->text.empty(),"Audio XML required row identity missing");
   if(key->text==owned_id)result.push_back({kind,key->text,row,*layer.xml.source,defaults});
  }
 }
 return result;
}
BnwAudioNativeParameters BnwAudioSourceCatalog::native_parameters()const{
 const auto p=impl_;p->entry();need(bool(p->native),"Audio catalog has no genuine native AudioParams snapshot");
 validate_current(p);const auto snapshot=*p->native;validate_current(p);return snapshot;
}
void BnwAudioSourceCatalog::require_same_completed_resources(const BnwAudioCompletedResourceOwner& owner)const{
 const auto p=impl_;p->entry();need(p->completed&&p->completed.get()==&owner,"Audio catalog is diagnostic or belongs to another completed source");
 p->busy=true;try{p->completed->require_current();need(!p->retired.load(),"Audio catalog changed during completed identity");p->busy=false;}
 catch(...){p->retired.store(true);p->busy=false;throw;}
}
std::shared_ptr<const assets::ResourceMountTable> BnwAudioSourceCatalog::resources()const{
 const auto p=impl_;p->entry();need(!p->completed,"Completed audio catalog does not publish borrowed resource tables");
 require_current();return p->resources;
}
}
