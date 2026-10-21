#include "civ5/ui/bnw_new_era_native_xml_owner.hpp"
#ifdef _WIN32
#include "civ5/runtime/file_identity.hpp"
#include <atomic>
#include <functional>
#include <map>
#include <optional>
#include <set>
#include <stdexcept>
#include <string_view>
#include <thread>
#include <type_traits>
#include <utility>

namespace civ5::ui {
static_assert(std::is_copy_constructible_v<BnwAudioWindowActivationLoan>);
namespace {
constexpr std::size_t maximum_files=512,maximum_depth=32;
constexpr std::uint64_t maximum_bytes=64ULL*1024*1024;
void need(bool value,const char* why){if(!value)throw std::runtime_error(why);}
const content::XmlNode* unique(const content::XmlNode& node,std::string_view key){
 const content::XmlNode* found=nullptr;
 for(const auto& child:node.children)if(child.name==key){need(!found,"Native NewEra XML duplicate element");found=&child;}
 return found;
}
std::string leaf_text(const content::XmlNode& row,std::string_view key){
 const auto* field=unique(row,key);
 need(field&&!field->text.empty()&&field->children.empty()&&field->attributes.empty(),"Native NewEra XML required identity absent or malformed");
 return field->text;
}
std::string identity(std::string_view value){
 std::string result(value);
 for(auto& c:result)if(c>='A'&&c<='Z')c=static_cast<char>(c-'A'+'a');
 return result;
}
}
struct BnwNewEraNativeXmlOwner::Impl {
 const std::thread::id thread=std::this_thread::get_id();
 std::atomic<bool> retired{false};bool busy=false;
 const std::shared_ptr<const BnwAudioCompletedResourceOwner> resource;
 const BnwAudioWindowActivationLoan::SourceHandle window;
 const std::shared_ptr<const graphics::BnwOwnedCameraRendererWindowLease> physical;
 struct File {BnwNewEraNativeXmlFile identity;content::XmlNode document;};
 std::vector<File> files;
 std::optional<BnwNewEraNativeXmlRow> script,definition;
 Impl(std::shared_ptr<const BnwAudioCompletedResourceOwner> source,
      BnwAudioWindowActivationLoan::SourceHandle actual,
      std::shared_ptr<const graphics::BnwOwnedCameraRendererWindowLease> owned)
  :resource(std::move(source)),window(std::move(actual)),physical(std::move(owned)){}
 void check()const{need(!retired.load(),"Native NewEra XML owner retired");}
 struct Call {
  std::shared_ptr<Impl> p;
  explicit Call(std::shared_ptr<Impl> source):p(std::move(source)){
   need(p->thread==std::this_thread::get_id(),"Native NewEra XML owner foreign thread");p->check();
   if(p->busy){p->retired.store(true);throw std::runtime_error("Native NewEra XML owner reentry");}
   p->busy=true;
  }
  ~Call(){p->busy=false;}
 };
 void guard(std::uint64_t sequence)const{
  check();physical->require_current();check();window.require_current();check();
  need(sequence>0&&window.observed_sequence()==sequence,"Native NewEra XML activation changed");
  resource->require_same_actual_window(window);check();window.require_current();check();
  need(window.observed_sequence()==sequence,"Native NewEra XML activation changed during source callback");
  physical->require_current();check();window.require_current();check();
  need(window.observed_sequence()==sequence,"Native NewEra activation changed during physical owner check");
 }
 File read(BnwAudioXmlKind kind,const std::string& basename,std::size_t ordinal,
           std::uint64_t sequence)const{
  guard(sequence);
  // Hash/count/parse happened inside the genuine closed callback. This return
  // owns XML data only; no span/handle/caller callback escapes that read loan.
  auto actual=resource->read_native_audio_xml(basename);guard(sequence);
  need(actual.basename==basename&&actual.document.name==(kind==BnwAudioXmlKind::scripts_2d?"Script2DFile":"AudioDefinesFile"),"Native NewEra XML root/query differs");
  return File{{kind,basename,actual.sha256,ordinal,actual.bytes},std::move(actual.document)};
 }
 void validate(std::uint64_t sequence)const{
  guard(sequence);
  for(const auto& file:files){const auto actual=read(file.identity.kind,file.identity.basename,file.identity.declaration_ordinal,sequence);
   need(actual.identity.sha256==file.identity.sha256&&actual.identity.bytes==file.identity.bytes,"Native NewEra selected XML bytes changed");}
  guard(sequence);
 }
 void collect(const File& file){
  const bool scripts=file.identity.kind==BnwAudioXmlKind::scripts_2d;
  const auto* rows=unique(file.document,scripts?"Script2DSounds":"SoundDatas");
  std::vector<content::XmlNode> defaults;
  for(const auto& child:file.document.children)if(child.name.starts_with("Default"))defaults.push_back(child);
  if(!rows)return;
  std::size_t ordinal=0;
  for(const auto& row:rows->children){
   need(row.name==(scripts?"Script2DSound":"SoundData"),"Native NewEra XML row kind differs");
   const auto id=leaf_text(row,scripts?"ScriptID":"SoundID");
   const std::string expected=scripts?"AS2D_INTERFACE_NEW_ERA":"SND_INTERFACE_NEW_ERA";
   if(identity(id)==identity(expected)){
    need(id==expected,"Native NewEra noncanonical ID interpretation unavailable");
    auto& target=scripts?script:definition;
    need(!target,"Native NewEra duplicate ID winner semantics unavailable");
    if(scripts){need(leaf_text(row,"SoundID")=="SND_INTERFACE_NEW_ERA","Native NewEra actual SoundID changed");}
    else need(!leaf_text(row,"Filename").empty(),"Native NewEra actual filename absent");
    target=BnwNewEraNativeXmlRow{row,defaults,file.identity,ordinal};
   }
   ++ordinal;
  }
 }
};
BnwNewEraNativeXmlOwner::BnwNewEraNativeXmlOwner(std::shared_ptr<Impl> p):impl_(std::move(p)){}
BnwNewEraNativeXmlOwner::~BnwNewEraNativeXmlOwner(){impl_->retired.store(true);}
std::shared_ptr<const BnwNewEraNativeXmlOwner> BnwNewEraNativeXmlOwner::from_actual_activation(
 std::shared_ptr<const BnwAudioCompletedResourceOwner> supplied,const BnwAudioWindowActivationLoan& borrowed_loan,
 const graphics::BnwOwnedCameraRendererWindowLease::Loan& actual_window){
 // Own the original issued observation BEFORE callbacks can reset/move the
 // caller's loan holder. This copies only immutable genuine State/sequence.
 const auto loan=borrowed_loan;
 need(bool(supplied),"Native NewEra XML requires genuine completed resource owner");
 // Private canonical acquisition before callbackful validation; no no-op alias.
 const auto actual=supplied->retain_factory_owner(supplied);
 loan.require_current();const auto sequence=loan.sequence();
 // Only the genuine active physical Loan issues its canonical strong owner.
 // No caller shared_ptr alias, copied HWND or callback grant is accepted.
 const auto physical=actual_window.retain_owner();loan.require_current();
 need(physical&&actual_window.window()==reinterpret_cast<std::uintptr_t>(loan.window())&&
      actual_window.ui_thread()==loan.ui_thread(),"Native NewEra physical/activation window owners differ");
 loan.require_current();actual_window.require_current();
 auto p=std::make_shared<Impl>(actual,loan.source_handle(),physical);Impl::Call call(p);
 try{
  p->guard(sequence);loan.require_current();
  std::set<std::pair<BnwAudioXmlKind,std::string>> active,visited;
  std::uint64_t total=0;std::size_t declaration=0;
  const auto load=[&](const auto& self,BnwAudioXmlKind kind,const std::string& basename,std::size_t depth)->void{
   need(depth<maximum_depth&&declaration<maximum_files,"Native NewEra XML dependency budget exceeded");
   const auto key=std::pair{kind,identity(basename)};
   need(!active.contains(key),"Native NewEra XML dependency cycle");
   if(visited.contains(key))return;
   active.insert(key);const auto ordinal=declaration++;
   auto file=p->read(kind,basename,ordinal,sequence);loan.require_current();
   // Exact byte count captured inside the genuine read; no second read solely
   // to count bytes and no parsed-capacity estimate.
   need(file.identity.bytes<=maximum_bytes-total,"Native NewEra aggregate XML byte budget exceeded");total+=file.identity.bytes;
   if(const auto* includes=unique(file.document,"IncludeXmlFiles")){
    for(const auto& child:includes->children){
     need(child.name=="IncludeXmlFile"&&!child.text.empty()&&child.children.empty()&&child.attributes.empty(),"Native NewEra malformed XML include declaration");
     self(self,kind,child.text,depth+1); // Previous native read has RETURNED.
    }
   }
   p->collect(file);p->files.push_back(std::move(file));active.erase(key);visited.insert(key);
  };
  load(load,BnwAudioXmlKind::scripts_2d,"Audio2DScripts.xml",0);
  load(load,BnwAudioXmlKind::definitions,"AudioDefines.xml",0);
  need(p->script&&p->definition,"Native NewEra actual script/definition absent");
  p->validate(sequence);loan.require_current();actual_window.require_current();
  return std::shared_ptr<const BnwNewEraNativeXmlOwner>(new BnwNewEraNativeXmlOwner(p));
 }catch(...){p->retired.store(true);throw;}
}
void BnwNewEraNativeXmlOwner::require_current()const{
 const auto p=impl_;Impl::Call call(p);
 try{p->window.require_current();const auto sequence=p->window.observed_sequence();p->validate(sequence);}
 catch(...){p->retired.store(true);throw;}
}
BnwNewEraNativeXmlOperands BnwNewEraNativeXmlOwner::operands()const{
 const auto p=impl_;Impl::Call call(p);
 try{
  p->window.require_current();const auto sequence=p->window.observed_sequence();p->validate(sequence);
  need(p->script&&p->definition,"Native NewEra captured operands absent");
  BnwNewEraNativeXmlOperands result{*p->script,*p->definition,{},sequence};
  for(const auto& file:p->files)result.files.push_back(file.identity);
  p->validate(sequence);return result;
 }catch(...){p->retired.store(true);throw;}
}
}
#endif
