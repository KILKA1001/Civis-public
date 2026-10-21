#include "civ5/runtime/bnw_net_message_debug_config_owner.hpp"
#include "civ5/runtime/detail/bnw_net_debug_config_subset.hpp"
#include "civ5/runtime/bnw_selected_language_input_owner.hpp"
#include "civ5/ui/bnw_native_resource_boot_settings_owner.hpp"
#include "civ5/platform/windows_path.hpp"
#include <atomic>
#include <fstream>
#include <locale>
#include <thread>
#include <vector>
#include <stdexcept>
namespace civ5::runtime {
namespace {
void need(bool b,const char* why){if(!b)throw std::runtime_error(why);}
bool same_identity(const FileIdentity& a,const FileIdentity& b){return a.bytes==b.bytes&&a.sha256==b.sha256;}
bool same(const BnwNetDebugConfigFileFacts& a,const BnwNetDebugConfigFileFacts& b){
 if(a.logical_path!=b.logical_path||a.selected_path!=b.selected_path||a.operand!=b.operand||a.file.has_value()!=b.file.has_value())return false;
 return !a.file||same_identity(*a.file,*b.file);
}
BnwNetDebugConfigFileFacts read_input(const std::filesystem::path& logical){
 BnwNetDebugConfigFileFacts f;f.logical_path=logical;f.selected_path=std::filesystem::weakly_canonical(logical);
 std::error_code error;auto status=std::filesystem::symlink_status(logical,error);
 need(!error||error==std::errc::no_such_file_or_directory,"Original/config source status unavailable");
 // Missing source is only an observed unavailable input. Native getter
 // failure preserves prior01, but this reader does not issue that getter return
 // or prove any existing manager state. No enabled-state setter exists here.
 if(status.type()==std::filesystem::file_type::not_found)return f;
 need(std::filesystem::is_regular_file(f.selected_path),"Net debug config is not a regular file");
 auto size=std::filesystem::file_size(f.selected_path);need(size<=1024*1024,"Net debug config source budget exceeded");
 std::ifstream in;in.imbue(std::locale::classic());in.open(platform::extended_windows_path(f.selected_path),std::ios::binary);
 need(bool(in),"Net debug actual config stream unavailable");std::vector<std::uint8_t> bytes(static_cast<std::size_t>(size));
 in.read(reinterpret_cast<char*>(bytes.data()),static_cast<std::streamsize>(bytes.size()));
 need(in.gcount()==static_cast<std::streamsize>(bytes.size())&&in.peek()==std::char_traits<char>::eof()&&!in.bad(),"Net debug config exact-byte read changed/failed");
 f.file=identify_bytes(bytes);
 auto flag=detail::net_debug_explicit_boolean(std::string_view(reinterpret_cast<const char*>(bytes.data()),bytes.size()));
 if(flag.has_value())f.operand=*flag?BnwNetDebugConfigOperand::explicit_nonzero:BnwNetDebugConfigOperand::explicit_zero;
 need(std::filesystem::weakly_canonical(logical)==f.selected_path,"Net debug selected config target changed while reading");
 return f;
}
}
struct BnwNetMessageDebugConfigOwner::Impl {
 const std::thread::id creator=std::this_thread::get_id();
 std::weak_ptr<const ui::BnwNativeResourceBootSettingsOwner> boot;
 const ui::BnwNativeResourceBootSettingsOwner* boot_identity=nullptr;
 std::shared_ptr<const BnwSelectedLanguageInputOwner> selected;
 BnwNetDebugConfigFacts facts;
 mutable std::atomic<bool> retired{false};mutable bool busy=false;
};
BnwNetMessageDebugConfigOwner::BnwNetMessageDebugConfigOwner(std::shared_ptr<Impl> p):impl_(std::move(p)){}
BnwNetMessageDebugConfigOwner::~BnwNetMessageDebugConfigOwner(){impl_->retired.store(true);}
std::shared_ptr<const BnwNetMessageDebugConfigOwner> BnwNetMessageDebugConfigOwner::capture_from_boot(
 const std::shared_ptr<const ui::BnwNativeResourceBootSettingsOwner>& actual){
 need(bool(actual),"Net debug actual Boot absent");actual->validate_source();
 auto p=std::make_shared<Impl>();p->boot=actual;p->boot_identity=actual.get();
 auto profile=actual->selected_language_profile_path_without_parent_callbacks();
 p->facts.original_profile=read_input(profile);
 p->selected=actual->selected_language_input();need(bool(p->selected),"Net debug genuine selected language/config source absent");
 p->selected->require_same_boot(*actual);auto selected=p->selected->source();
 if(selected.kind==BnwSelectedLanguageSourceKind::selected_ini){
  auto input=read_input(selected.config_ini);need(input.file&&same_identity(*input.file,selected.file),"Net debug selected config differs from actual selected input bytes");
  p->facts.selected_kind=input.selected_path==p->facts.original_profile.selected_path?BnwNetDebugSelectedConfigKind::same_original_profile:BnwNetDebugSelectedConfigKind::civis_selected_override;
  p->facts.selected_civis_config=std::move(input);
 }
 actual->validate_source();p->selected->require_same_boot(*actual);
 need(profile==actual->selected_language_profile_path_without_parent_callbacks()&&same(read_input(profile),p->facts.original_profile),"Net debug original profile changed during capture");
 auto out=std::shared_ptr<const BnwNetMessageDebugConfigOwner>(new BnwNetMessageDebugConfigOwner(std::move(p)));out->require_current();return out;
}
void BnwNetMessageDebugConfigOwner::require_current()const {
 const auto p=impl_;need(std::this_thread::get_id()==p->creator,"Net debug config foreign creator before source borrow");
 need(!p->retired.load(),"Net debug config owner retired");if(p->busy){p->retired.store(true);throw std::runtime_error("Net debug config reentry");}
 p->busy=true;struct End{Impl& p;~End(){p.busy=false;}}end{*p};
 try{
  auto boot=p->boot.lock();need(boot&&boot.get()==p->boot_identity,"Net debug original Boot substituted/expired");
  boot->validate_source();p->selected->require_same_boot(*boot);
  auto profile=boot->selected_language_profile_path_without_parent_callbacks();
  need(profile==p->facts.original_profile.logical_path&&same(read_input(profile),p->facts.original_profile),"Net debug original profile source changed");
  auto selected=p->selected->source();
  if(p->facts.selected_civis_config){
   need(selected.kind==BnwSelectedLanguageSourceKind::selected_ini,"Net debug selected source kind changed");
   auto input=read_input(selected.config_ini);need(same(input,*p->facts.selected_civis_config)&&input.file&&same_identity(*input.file,selected.file),"Net debug actual selected config bytes changed");
  }else need(selected.kind==BnwSelectedLanguageSourceKind::explicit_process_language,"Net debug language-only source replaced by a file");
  // No native manager/list has been published by this owner. Keep the genuine
  // unresolved bootstrap frontier even when the actual config operand is0.
  need(p->facts.manager_bootstrap_and_registered_log_list_required,"Net debug reader acquired an invented manager receipt");
  boot->validate_source();p->selected->require_current();
 }catch(...){p->retired.store(true);throw;}
}
BnwNetDebugConfigFacts BnwNetMessageDebugConfigOwner::source_facts()const{require_current();auto f=impl_->facts;require_current();return f;}
void BnwNetMessageDebugConfigOwner::require_same_boot(const ui::BnwNativeResourceBootSettingsOwner& actual)const{require_current();need(&actual==impl_->boot_identity,"Net debug different actual Boot");require_current();}
} // namespace civ5::runtime
