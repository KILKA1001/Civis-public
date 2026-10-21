#include "civ5/ui/bnw_audio_sound_data_owner.hpp"
#ifdef _WIN32
#include <atomic>
#include <optional>
#include <stdexcept>
#include <thread>
#include <utility>
namespace civ5::ui {
namespace {
void need(bool value,const char* why){if(!value)throw std::runtime_error(why);}
const content::XmlNode* unique(const content::XmlNode& row,std::string_view name){
 const content::XmlNode* result=nullptr;
 for(const auto& child:row.children)if(child.name==name){
  need(!result,"Actual SoundData duplicate authored field");result=&child;
 }
 return result;
}
BnwSoundDataFieldOperand operand(const content::XmlNode& row,std::string_view name){
 const auto* field=unique(row,name);
 if(!field)return {};
 need(field->attributes.empty()&&field->children.empty(),"Actual SoundData field is not scalar");
 return {field->text.empty()?BnwSoundDataFieldForm::empty:BnwSoundDataFieldForm::authored_text,field->text};
}
std::string required(const content::XmlNode& row,std::string_view name){
 const auto field=operand(row,name);
 need(field.form==BnwSoundDataFieldForm::authored_text,"Actual SoundData required authored identity absent");
 return field.text;
}
bool native_boolean(const BnwSoundDataFieldOperand& field){
 // Original835120 returns false without writing its output for length0;
 // the row constructor's zero survives. No XML-presence-as-true rule.
 if(field.form==BnwSoundDataFieldForm::absent||field.form==BnwSoundDataFieldForm::empty)return false;
 need(!field.text.empty(),"Native SoundData authored Boolean text missing");
 const auto first=field.text.front();
 // Original accepts its first ASCII t/T/f/F byte directly. The separate
 // locale/istream numeric branch is not silently treated as false here.
 if(first=='t'||first=='T')return true;
 if(first=='f'||first=='F')return false;
 throw std::runtime_error("Native SoundData Boolean numeric/locale branch is not admitted");
}
BnwNativeSoundDataDefinition parse_native(const BnwSoundDataSourceOperands& source){
 BnwNativeSoundDataDefinition result;
 // Closed literals from original manager registry. This parser does not
 // claim that the manager itself has completed its startup side effects.
 if(source.load_type=="RESIDENT")result.resolved_load_type=0;
 else if(source.load_type=="DYNAMIC_RES")result.resolved_load_type=1;
 else if(source.load_type=="STREAMED")result.resolved_load_type=2;
 else throw std::runtime_error("Native SoundData load-type registry operand unsupported");
 result.dont_cache=native_boolean(source.dont_cache);
 result.only_load_one_variation=native_boolean(source.only_load_one_variation);
 return result;
}
BnwSoundDataSourceOperands extract(const BnwNewEraNativeXmlOperands& xml){
 const auto& definition=xml.definition;
 need(definition.row.name=="SoundData","Actual SoundData row type differs");
 const auto sound=required(definition.row,"SoundID");
 need(sound=="SND_INTERFACE_NEW_ERA"&&required(xml.script.row,"SoundID")==sound,
      "Actual NewEra script and SoundData identities differ");
 return {sound,required(definition.row,"Filename"),required(definition.row,"LoadType"),
         operand(definition.row,"bDontCache"),operand(definition.row,"bOnlyLoadOneVariationEachTime"),
         definition.authored_defaults,definition.file,definition.row_ordinal,xml.actual_activation_sequence};
}
}
struct BnwAudioSoundDataOwner::Impl {
 const std::thread::id thread=std::this_thread::get_id();
 std::atomic<bool> retired{false};bool busy=false;
 const std::shared_ptr<const BnwNewEraNativeXmlOwner> xml;
 const std::shared_ptr<const BnwAudioNativeFilenameProbes> probes;
 const BnwSoundDataSourceOperands source;
 const BnwNativeSoundDataDefinition parsed;
 std::optional<BnwAudioInspectedVariationCount> count_cache;
 Impl(std::shared_ptr<const BnwNewEraNativeXmlOwner> owned_xml,
      std::shared_ptr<const BnwAudioNativeFilenameProbes> owned_probes,
      BnwSoundDataSourceOperands operands)
  :xml(std::move(owned_xml)),probes(std::move(owned_probes)),source(std::move(operands)),parsed(parse_native(source)){}
 void check()const{need(!retired.load(),"Actual SoundData source consumer retired");}
 struct Call {
  std::shared_ptr<Impl> p;
  explicit Call(std::shared_ptr<Impl> value):p(std::move(value)){
   need(p->thread==std::this_thread::get_id(),"Actual SoundData source consumer foreign creator thread");p->check();
   if(p->busy){p->retired.store(true);throw std::runtime_error("Actual SoundData source consumer reentry");}
   p->busy=true;
  }
  ~Call(){p->busy=false;}
 };
 void validate()const{
  check();xml->require_current();check();probes->require_current();check();
  const auto current=extract(xml->operands());check();
  need(current.sound_id==source.sound_id&&current.filename==source.filename&&current.load_type==source.load_type&&
       current.defining_file.sha256==source.defining_file.sha256&&current.defining_file.bytes==source.defining_file.bytes&&
       current.defining_file.basename==source.defining_file.basename&&current.defining_row==source.defining_row&&
       current.actual_activation_sequence==source.actual_activation_sequence&&
       current.dont_cache.form==source.dont_cache.form&&current.dont_cache.text==source.dont_cache.text&&
       current.only_load_one_variation.form==source.only_load_one_variation.form&&
       current.only_load_one_variation.text==source.only_load_one_variation.text,
       "Actual SoundData source operands/identity changed");
  probes->require_current();check();xml->require_current();check();
 }
};
BnwAudioSoundDataOwner::BnwAudioSoundDataOwner(std::shared_ptr<Impl> p):impl_(std::move(p)){}
BnwAudioSoundDataOwner::~BnwAudioSoundDataOwner(){impl_->retired.store(true);}
std::shared_ptr<const BnwAudioSoundDataOwner> BnwAudioSoundDataOwner::from_actual_activation(
 std::shared_ptr<const BnwAudioCompletedResourceOwner> resource,
 const BnwAudioWindowActivationLoan& borrowed,
 const graphics::BnwOwnedCameraRendererWindowLease::Loan& physical){
 const auto loan=borrowed;loan.require_current();
 // Both child factories independently authenticate actual completed source,
 // physical HWND and immutable activation before source callbacks. This owner
 // never accepts caller XML/count/defaults as a source grant.
 const auto xml=BnwNewEraNativeXmlOwner::from_actual_activation(resource,loan,physical);
 loan.require_current();physical.require_current();
 const auto probes=BnwAudioNativeFilenameProbes::from_actual_activation(resource,loan,physical);
 loan.require_current();physical.require_current();
 auto p=std::make_shared<Impl>(xml,probes,extract(xml->operands()));Impl::Call call(p);
 try{
  p->validate();loan.require_current();physical.require_current();
  return std::shared_ptr<const BnwAudioSoundDataOwner>(new BnwAudioSoundDataOwner(p));
 }catch(...){p->retired.store(true);throw;}
}
void BnwAudioSoundDataOwner::require_current()const{
 const auto p=impl_;Impl::Call call(p);
 try{p->validate();}catch(...){p->retired.store(true);throw;}
}
BnwSoundDataSourceOperands BnwAudioSoundDataOwner::source_operands()const{
 const auto p=impl_;Impl::Call call(p);
 try{p->validate();auto result=p->source;p->validate();return result;}
 catch(...){p->retired.store(true);throw;}
}
BnwNativeSoundDataDefinition BnwAudioSoundDataOwner::native_definition()const{
 const auto p=impl_;Impl::Call call(p);
 try{p->validate();const auto result=p->parsed;p->validate();return result;}
 catch(...){p->retired.store(true);throw;}
}
BnwSoundDataCountObservation BnwAudioSoundDataOwner::inspect_unprefixed_variation_count()const{
 const auto p=impl_;Impl::Call call(p);
 try{
  p->validate();
  if(p->count_cache){auto result=BnwSoundDataCountObservation{*p->count_cache,true};p->validate();return result;}
  //85fbd0 computes extension-ordered count on a cold filename cache entry.
  // This is a source-bound Civis count cache only; it does not issue Original
  // manager slot identity, prefix, shuffled variant or8611f0 load completion.
  auto inspected=p->probes->inspect_uncached_variation_count(p->source.filename);
  p->validate();
  need(inspected.queried_stem==p->source.filename&&
       inspected.actual_activation_sequence==p->source.actual_activation_sequence,
       "Actual SoundData count observation activation differs");
  need(inspected.count<=16&&((inspected.count==0)==inspected.selected_extension.empty()),
       "Actual SoundData inspected count/extension relation differs");
  // Own the caller-visible copy before committing. Allocation/source failure
  // before this point leaves the cache unpublished and retires this consumer.
  auto returned=inspected;
  p->count_cache.emplace(std::move(inspected));
  return {std::move(returned),false};
 }catch(...){p->retired.store(true);throw;}
}
void BnwAudioSoundDataOwner::require_native_voice_ready()const{
 const auto p=impl_;Impl::Call call(p);
 try{p->validate();}catch(...){p->retired.store(true);throw;}
 throw std::runtime_error("Native SoundData voice requires Boot-issued manager prefix/RNG history,8611f0 resource load and actual voice/device admission");
}
}
#endif
