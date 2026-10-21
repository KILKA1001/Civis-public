#include "civ5/ui/bnw_new_era_native_xml_owner.hpp"
#ifdef _WIN32
#include <set>
#include <stdexcept>
#include <type_traits>

using namespace civ5::ui;
static_assert(!std::is_default_constructible_v<BnwNewEraNativeXmlOwner>);
static_assert(!std::is_copy_constructible_v<BnwNewEraNativeXmlOwner>);
static_assert(!std::is_default_constructible_v<BnwAudioWindowActivationLoan::SourceHandle>);
namespace {
void check(bool value,const char* why){if(!value)throw std::runtime_error(why);}
const civ5::content::XmlNode& exact(const civ5::content::XmlNode& row,const char* key){
 const civ5::content::XmlNode* result=nullptr;
 for(const auto& child:row.children)if(child.name==key){check(!result,"Duplicate actual field");result=&child;}
 check(result!=nullptr,"Required actual field missing");return *result;
}
}
// AUTHORED UNRUN. Root calls this only inside the actual bound WM_ACTIVATE,
// after obtaining the genuine completed Parent/SelectedRead source. An XML
// fixture or true callback cannot manufacture that loan or resource owner.
void verify_actual_new_era_native_xml(
 std::shared_ptr<const BnwAudioCompletedResourceOwner> source,
 const BnwAudioWindowActivationLoan& actual_loan,
 const civ5::graphics::BnwOwnedCameraRendererWindowLease::Loan& physical){
 actual_loan.require_current();
 const auto owner=BnwNewEraNativeXmlOwner::from_actual_activation(source,actual_loan,physical);
 const auto actual=owner->operands();actual_loan.require_current();
 check(actual.actual_activation_sequence==actual_loan.sequence(),"Actual activation source differs");
 check(exact(actual.script.row,"ScriptID").text=="AS2D_INTERFACE_NEW_ERA","Selected NewEra ScriptID differs");
 check(exact(actual.script.row,"SoundID").text=="SND_INTERFACE_NEW_ERA","Selected NewEra SoundID differs");
 check(exact(actual.script.row,"SoundType").text=="GAME_SFX","Selected NewEra category differs");
 check(exact(actual.script.row,"iMinVolume").text=="75"&&exact(actual.script.row,"iMaxVolume").text=="75","Selected authored volume operands differ");
 check(exact(actual.script.row,"bDontTriggerDuplicates").text=="True"&&exact(actual.script.row,"iDontPlayMoreThan").text=="1","Selected authored duplicate policy differs");
 check(exact(actual.definition.row,"SoundID").text=="SND_INTERFACE_NEW_ERA"&&exact(actual.definition.row,"Filename").text=="NotificationNewEra","Selected definition differs");
 check(exact(actual.definition.row,"bDontCache").text.empty()&&exact(actual.definition.row,"bOnlyLoadOneVariationEachTime").text.empty(),"Empty Boolean source operands were invented");
 check(actual.files.size()>2,"Declared XML dependencies were omitted");
 std::set<std::size_t> ordinals;
 for(const auto& file:actual.files){check(file.bytes>0&&file.sha256.size()==64,"Actual source identity absent");check(ordinals.insert(file.declaration_ordinal).second,"Duplicate source declaration ordinal");}
 const auto second=owner->operands();
 check(second.script.file.sha256==actual.script.file.sha256&&second.definition.file.sha256==actual.definition.file.sha256,"Actual selected sources changed between reads");
 owner->require_current();actual_loan.require_current();
}
// Also invoked only with real loan. No borrowed/raw ResourceMountTable or
// fabricated proof can bypass the canonical completed source requirement.
void verify_actual_new_era_missing_source(const BnwAudioWindowActivationLoan& loan,
 const civ5::graphics::BnwOwnedCameraRendererWindowLease::Loan& physical){
 bool refused=false;
 try{(void)BnwNewEraNativeXmlOwner::from_actual_activation(nullptr,loan,physical);}catch(...){refused=true;}
 check(refused,"Missing actual completed source admitted");
}
#endif
