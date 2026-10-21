#include "civ5/runtime/bnw_net_message_debug_manager_owner.hpp"
#include "civ5/ui/bnw_native_resource_boot_settings_owner.hpp"
#include "civ5/ui/selected_bnw_preview_host.hpp"
#include "civ5/platform/windows_path.hpp"
#include <atomic>
#include <mutex>
#include <thread>
#include <vector>
#include <stdexcept>
#include <string>
#include <algorithm>
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <shlobj.h>
namespace civ5::runtime {
namespace {
void need(bool b,const char* why){if(!b)throw std::runtime_error(why);}
std::filesystem::path actual_own_log_path(){
 wchar_t local[MAX_PATH]{};
 need(SHGetSpecialFolderPathW(nullptr,local,CSIDL_LOCAL_APPDATA,FALSE)!=FALSE,
      "Civis actual LocalAppData source unavailable");
 need(std::filesystem::is_directory(local),"Civis actual LocalAppData directory absent");
 return std::filesystem::absolute(std::filesystem::path(local)/L"Civis"/L"Logs"/
  std::to_wstring(GetCurrentProcessId())/L"net_message_debug.log").lexically_normal();
}
void reject_reparse(const std::filesystem::path& p){
 auto attributes=GetFileAttributesW(platform::extended_windows_path(p).c_str());
 need(attributes!=INVALID_FILE_ATTRIBUTES&&!(attributes&FILE_ATTRIBUTE_REPARSE_POINT),"Civis log path has an unknown reparse/absent component");
}
struct Log final {
 const DWORD process=GetCurrentProcessId();const std::thread::id creator=std::this_thread::get_id();
 const std::string name="net_message_debug.log";
 const std::uint32_t flags=0; // SDK default, flush on each actual write.
 const std::filesystem::path path;
 std::recursive_mutex lock;
 HANDLE file=INVALID_HANDLE_VALUE;
 std::uint64_t bytes_written=0,write_calls=0,flush_calls=0;
 std::uint64_t actual_expected_end=0;
 explicit Log(std::filesystem::path actual):path(std::move(actual)){}
 void require_creator()const{need(process==GetCurrentProcessId()&&creator==std::this_thread::get_id(),"Civis retained Log foreign process/creator");}
 void open(){
  require_creator();std::lock_guard guard(lock);if(file!=INVALID_HANDLE_VALUE)return;
  need(path==actual_own_log_path(),"Civis log OS-directory identity changed before open");
  // Actual owned filesystem bootstrap. No original profile/log is modified.
  const auto pid=path.parent_path(),logs=pid.parent_path(),civis=logs.parent_path();
  // Validate each actual parent before creating its child. Never traverse an
  // existing redirect while bootstrapping our own directory.
  reject_reparse(civis.parent_path());
  for(const auto& directory:{civis,logs,pid}){
   const auto attributes=GetFileAttributesW(platform::extended_windows_path(directory).c_str());
   if(attributes==INVALID_FILE_ATTRIBUTES){
    const auto error=GetLastError();need(error==ERROR_FILE_NOT_FOUND||error==ERROR_PATH_NOT_FOUND,"Civis log directory status unavailable");
    std::filesystem::create_directory(directory);
   }
   reject_reparse(directory);need(std::filesystem::is_directory(directory),"Civis owned log parent is not a directory");
  }
  const auto existing=GetFileAttributesW(platform::extended_windows_path(path).c_str());
  if(existing!=INVALID_FILE_ATTRIBUTES)reject_reparse(path);
  else {const auto error=GetLastError();need(error==ERROR_FILE_NOT_FOUND,"Civis log file status unavailable");}
  const auto full=platform::extended_windows_path(path);
  auto handle=CreateFileW(full.c_str(),GENERIC_READ|GENERIC_WRITE,FILE_SHARE_READ,nullptr,
    OPEN_ALWAYS,FILE_ATTRIBUTE_NORMAL|FILE_FLAG_OPEN_REPARSE_POINT,nullptr);
  need(handle!=INVALID_HANDLE_VALUE,"Civis actual log stream open failed");
  struct Close{HANDLE h;~Close(){if(h!=INVALID_HANDLE_VALUE)CloseHandle(h);}}rollback{handle};
  LARGE_INTEGER zero{};need(SetFilePointerEx(handle,zero,nullptr,FILE_END)!=FALSE,"Civis actual log append seek failed");
  LARGE_INTEGER size{};need(GetFileSizeEx(handle,&size)!=FALSE&&size.QuadPart>=0,"Civis actual log size unavailable");
  actual_expected_end=static_cast<std::uint64_t>(size.QuadPart);
  BY_HANDLE_FILE_INFORMATION identity{};
  need(GetFileInformationByHandle(handle,&identity)!=FALSE&&identity.nNumberOfLinks==1&&!(identity.dwFileAttributes&(FILE_ATTRIBUTE_REPARSE_POINT|FILE_ATTRIBUTE_DIRECTORY)),"Civis opened log is not an ordinary file");
  file=handle;rollback.h=INVALID_HANDLE_VALUE;
  // Authenticate the actual open handle before the first possible write.
  require_actual_stream();
 }
 void close(){
  require_creator();std::lock_guard guard(lock);if(file==INVALID_HANDLE_VALUE)return;
  need(FlushFileBuffers(file)!=FALSE,"Civis actual Log close flush failed");++flush_calls;
  need(CloseHandle(file)!=FALSE,"Civis actual Log close failed");file=INVALID_HANDLE_VALUE;
 }
 void require_actual_stream(){
  require_creator();std::lock_guard guard(lock);
  need(file!=INVALID_HANDLE_VALUE,"Civis actual log stream closed");
  std::vector<wchar_t> final_name(32768);
  const auto count=GetFinalPathNameByHandleW(file,final_name.data(),static_cast<DWORD>(final_name.size()),FILE_NAME_NORMALIZED|VOLUME_NAME_DOS);
  need(count&&count<final_name.size(),"Civis actual stream filename unavailable");
  const auto expected=platform::extended_windows_path(path).native();
  need(CompareStringOrdinal(final_name.data(),static_cast<int>(count),expected.c_str(),static_cast<int>(expected.size()),TRUE)==CSTR_EQUAL,
       "Civis actual stream handle no longer names the owned log path");
  LARGE_INTEGER size{};need(GetFileSizeEx(file,&size)!=FALSE&&size.QuadPart>=0&&static_cast<std::uint64_t>(size.QuadPart)==actual_expected_end,
       "Civis actual stream size changed outside its recorded real writes");
 }
 void write(std::string_view message){
  require_creator();std::lock_guard guard(lock);require_actual_stream();
  std::size_t at=0;
  while(at<message.size()){
   const auto size=static_cast<DWORD>(std::min<std::size_t>(message.size()-at,MAXDWORD));DWORD actual=0;
   ++write_calls;const auto ok=WriteFile(file,message.data()+at,size,&actual,nullptr);
   need(actual<=size,"Civis actual net log returned invalid progress");
   need(bytes_written<=UINT64_MAX-actual&&actual_expected_end<=UINT64_MAX-actual,"Civis actual log byte counter overflow");
   bytes_written+=actual;actual_expected_end+=actual;at+=actual; // Retain even a real partial failed write.
   need(ok!=FALSE&&actual>0,"Civis actual net log WriteFile failed/zero progress");
  }
  need(FlushFileBuffers(file)!=FALSE,"Civis actual net log flush failed");++flush_calls;
 }
 ~Log(){
  if(file!=INVALID_HANDLE_VALUE){
   if(creator!=std::this_thread::get_id())std::terminate();
   // Exception cleanup still closes the real OS resource. A failed flush is
   // never upgraded into delivery success; the caller already failed closed.
   FlushFileBuffers(file);CloseHandle(file);
  }
 }
};
}
struct BnwNetMessageDebugManagerOwner::Impl {
 const DWORD process=GetCurrentProcessId();const std::thread::id creator=std::this_thread::get_id();
 std::weak_ptr<const BnwNetMessageDebugManagerOwner> self;
 std::weak_ptr<const ui::BnwNativeResourceBootSettingsOwner> boot;
 const ui::BnwNativeResourceBootSettingsOwner* boot_identity=nullptr;
 std::shared_ptr<const BnwNetMessageDebugConfigOwner> config;
 // This is a concrete new standalone manager and its actual registry, created
 // by our closed Boot factory. It is NOT a copy/claim of the original PE list.
 std::vector<std::unique_ptr<Log>> registered_logs;
 bool enabled=true; // Actual constructor writes original fresh default01.
 BnwNetDebugSelectedConfigKind lane=BnwNetDebugSelectedConfigKind::process_language_without_config;
 BnwNetDebugConfigOperand operand=BnwNetDebugConfigOperand::unavailable_or_key_missing;
 const std::filesystem::path path=actual_own_log_path();
 mutable bool busy=false;mutable std::atomic<bool> retired{false};
 void require_creator()const{need(process==GetCurrentProcessId()&&creator==std::this_thread::get_id(),"Civis manager foreign creator before registry borrow");}
 void disable(){
  require_creator();for(auto& log:registered_logs)log->close();enabled=false;
 }
 void enable(){
  require_creator();enabled=true;for(auto& log:registered_logs)log->open();
 }
 Log& get_net_log(){
  require_creator();for(auto& log:registered_logs)if(log->name=="net_message_debug.log")return *log;
  auto created=std::make_unique<Log>(path);created->open();
  // Publish only the genuinely created/opened exact stream. Failed open keeps
  // its actual partial filesystem effects but grants no registry/delivery ACK.
  registered_logs.push_back(std::move(created));return *registered_logs.back();
 }
};
BnwNetMessageDebugManagerOwner::BnwNetMessageDebugManagerOwner(std::shared_ptr<Impl> p):impl_(std::move(p)){}
BnwNetMessageDebugManagerOwner::~BnwNetMessageDebugManagerOwner(){if(impl_->creator!=std::this_thread::get_id())std::terminate();impl_->retired.store(true);}
std::shared_ptr<const BnwNetMessageDebugManagerOwner> BnwNetMessageDebugManagerOwner::capture_from_boot(
 const std::shared_ptr<const ui::BnwNativeResourceBootSettingsOwner>& actual){
 need(bool(actual),"Civis net manager actual Boot absent");actual->validate_source();
 auto p=std::make_shared<Impl>();p->boot=actual;p->boot_identity=actual.get();
 p->config=actual->selected_net_debug_config();need(bool(p->config),"Civis net manager actual config owner absent");
 p->config->require_same_boot(*actual);const auto facts=p->config->source_facts();p->lane=facts.selected_kind;
 // Original profile and standalone override are distinct sources. Our manager
 // may consume the actual selected standalone operand; this NEVER proves the
 // original manager/list's current flag or path/format parity.
 p->operand=facts.selected_civis_config?facts.selected_civis_config->operand:facts.original_profile.operand;
 if(p->operand==BnwNetDebugConfigOperand::explicit_zero)p->disable();
 else if(p->operand==BnwNetDebugConfigOperand::explicit_nonzero)p->enable();
 // Failed/missing getter input performs no toggle: actual constructor01 stays.
 actual->validate_source();p->config->require_same_boot(*actual);
 auto out=std::shared_ptr<const BnwNetMessageDebugManagerOwner>(new BnwNetMessageDebugManagerOwner(std::move(p)));
 out->impl_->self=out;out->require_current();return out;
}
void BnwNetMessageDebugManagerOwner::require_source_bookends()const {
 const auto p=impl_;p->require_creator();need(!p->retired.load(),"Civis net manager retired during source bookend");
 auto actual=p->boot.lock();need(actual&&actual.get()==p->boot_identity,"Civis net manager original Boot expired/substituted");
 actual->validate_source();p->config->require_same_boot(*actual);
 const auto facts=p->config->source_facts();const auto operand=facts.selected_civis_config?facts.selected_civis_config->operand:facts.original_profile.operand;
 need(facts.selected_kind==p->lane&&operand==p->operand&&p->path==actual_own_log_path(),"Civis net manager actual source/path changed");
 for(const auto& log:p->registered_logs){log->require_creator();need(log->path==p->path&&log->flags==0,"Civis retained Log source replaced");
  need(p->enabled==(log->file!=INVALID_HANDLE_VALUE),"Civis actual enabled state/registered stream lifetime differs");
  if(p->enabled)log->require_actual_stream();}
 actual->validate_source();
}
void BnwNetMessageDebugManagerOwner::require_current()const {
 const auto p=impl_;p->require_creator();need(!p->retired.load(),"Civis net manager retired");
 need(!p->busy,"Civis net manager reentry/active sink cannot mint currentness");
 require_source_bookends();
}
BnwNetDebugOwnManagerFacts BnwNetMessageDebugManagerOwner::source_facts()const{
 require_current();BnwNetDebugOwnManagerFacts f;const auto p=impl_;
 f.own_manager_constructed=true;f.enabled=p->enabled;f.actual_own_log_path=p->path;
 f.registrations=p->registered_logs.size();f.config_lane=p->lane;f.operand=p->operand;
 for(const auto& log:p->registered_logs){f.write_calls+=log->write_calls;f.written_bytes+=log->bytes_written;f.flush_calls+=log->flush_calls;}
 require_current();return f;
}
void BnwNetMessageDebugManagerOwner::require_same_boot(const ui::BnwNativeResourceBootSettingsOwner& actual)const{
 require_current();need(&actual==impl_->boot_identity,"Civis net manager belongs to different genuine Boot");require_current();
}
BnwNetDebugOwnDelivery BnwNetMessageDebugManagerOwner::submit_owned_message(std::string_view message)const {
 require_current();auto p=impl_;p->busy=true;
 struct End{Impl& p;~End(){p.busy=false;}}end{*p};
 try{
  // Native Utility checks enabled before GetLog/c_str/submission. An actual
  // disabled owned manager returns without stream/registration/file effects.
  if(!p->enabled){require_source_bookends();return BnwNetDebugOwnDelivery::actual_disabled_return;}
  auto& log=p->get_net_log();log.write(message);require_source_bookends();
  // Real bytes were stored/flushed. Original timestamp/path parity is separate.
  return BnwNetDebugOwnDelivery::actual_file_write_and_flush;
 }catch(...){p->retired.store(true);throw;}
}
BnwNetDebugOwnDelivery BnwNetMessageDebugManagerOwner::submit_game_turn_message(std::int32_t turn)const {
 const std::string message="Game Turn "+std::to_string(turn)+"\n";
 return submit_owned_message(message);
}
BnwNetDebugOwnDelivery BnwNetMessageDebugManagerOwner::write_bootstrap_diagnostic()const {
 return submit_owned_message("Civis net-message logger bootstrap\n");
}
} // namespace civ5::runtime

namespace civ5::runtime {
std::shared_ptr<const BnwNetMessageDebugManagerOwner>
BnwNetMessageDebugManagerOwner::retain_factory_owner_without_callbacks()const {
 const auto p=impl_;p->require_creator();need(!p->retired.load()&&!p->busy,"Actual net manager factory retired/busy");
 const auto actual=p->self.lock();need(actual&&actual.get()==this&&actual.use_count()>1,"Actual net manager outside factory owner retired");
 return actual;
}
void BnwNetMessageDebugManagerOwner::require_same_host(const ui::SelectedBnwPreviewHost& host)const {
 const auto p=impl_;p->require_creator();
 need(host.saved_database_thread_==std::this_thread::get_id(),"Net manager Host foreign creator before Parent/session borrow");
 const auto canonical=retain_factory_owner_without_callbacks();require_current();
 const auto package=host.early_package_;need(bool(package),"GameTurn net logger requires genuine Host retained resource Parent");
 const auto actual=p->boot.lock();need(actual&&actual.get()==p->boot_identity,"GameTurn net logger actual Boot retired");
 actual->require_same_resources(*package);
 need(host.early_package_==package&&host.saved_database_thread_==p->creator,"GameTurn net logger actual Host Parent/creator changed");
 require_current();const auto after=retain_factory_owner_without_callbacks();
 need(after.get()==canonical.get()&&!after.owner_before(canonical)&&!canonical.owner_before(after),"GameTurn net logger canonical manager control changed");
}
}
