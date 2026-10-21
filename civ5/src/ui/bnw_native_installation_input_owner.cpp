#include "civ5/ui/bnw_native_installation_input_owner.hpp"
#include "bnw_native_filesystem_mapping_owner.hpp"
#include "civ5/ui/bnw_native_pinned_installation_epoch.hpp"
#include "civ5/platform/windows_path.hpp"
#include "bnw_native_callback_owner_guard.hpp"
#include <atomic>
#include <thread>
#include <stdexcept>
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
namespace civ5::ui {
namespace {
void need(bool b,const char* s){if(!b)throw std::runtime_error(s);}
bool same_file(const BY_HANDLE_FILE_INFORMATION& a,const BY_HANDLE_FILE_INFORMATION& b){
 return a.dwVolumeSerialNumber==b.dwVolumeSerialNumber&&a.nFileIndexHigh==b.nFileIndexHigh&&
 a.nFileIndexLow==b.nFileIndexLow&&a.nFileSizeHigh==b.nFileSizeHigh&&a.nFileSizeLow==b.nFileSizeLow&&
 a.ftLastWriteTime.dwHighDateTime==b.ftLastWriteTime.dwHighDateTime&&a.ftLastWriteTime.dwLowDateTime==b.ftLastWriteTime.dwLowDateTime;
}
struct PinnedFile {
 const std::filesystem::path path;HANDLE held=INVALID_HANDLE_VALUE;BY_HANDLE_FILE_INFORMATION original{};
 runtime::FileIdentity identity;
 explicit PinnedFile(std::filesystem::path p):path(std::move(p)){
  const auto physical=platform::extended_windows_path(path);
  held=CreateFileW(physical.c_str(),GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);
  need(held!=INVALID_HANDLE_VALUE,"Installation source cannot be locked read-only");
  try{need(GetFileInformationByHandle(held,&original)!=FALSE&&!(original.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY),
   "Installation source not an actual regular file");identity=runtime::identify_file(path);require_current();}
  catch(...){CloseHandle(held);held=INVALID_HANDLE_VALUE;throw;}
 }
 ~PinnedFile(){if(held!=INVALID_HANDLE_VALUE)CloseHandle(held);}
 void require_current()const{
  BY_HANDLE_FILE_INFORMATION now{},named{};need(held!=INVALID_HANDLE_VALUE&&GetFileInformationByHandle(held,&now)!=FALSE&&
   same_file(now,original),"Installation original locked source retired/changed");
  const auto physical=platform::extended_windows_path(path);
  const auto reopened=CreateFileW(physical.c_str(),GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);
  need(reopened!=INVALID_HANDLE_VALUE,"Installation pinned source path disappeared");
  const bool read=GetFileInformationByHandle(reopened,&named)!=FALSE;CloseHandle(reopened);
  need(read&&same_file(named,original),"Installation source path no longer names original locked file");
 }
 PinnedFile(const PinnedFile&)=delete;PinnedFile&operator=(const PinnedFile&)=delete;
};
bool same_client(const runtime::BnwStockLuaClientExtent& a,const runtime::BnwStockLuaClientExtent& b){
 return a.surface_owner==b.surface_owner&&a.revision==b.revision&&a.width==b.width&&a.height==b.height;
}
}
struct BnwNativeInstallationInputOwner::Impl {
 const std::thread::id thread=std::this_thread::get_id();const DWORD process=GetCurrentProcessId();
 std::filesystem::path installation,payload;HWND hwnd=nullptr;
 runtime::BnwStockLuaClientExtentProvider provider;runtime::BnwStockLuaClientExtent extent;
 content::Catalog discovered;std::vector<std::unique_ptr<PinnedFile>> files;
 std::weak_ptr<const BnwNativeInstallationInputOwner> actual_facade;
 mutable bool busy=false;mutable std::atomic<bool> retired{false};
 std::atomic<bool> facade_alive{true};mutable std::atomic<bool> strict_retired{false};
 mutable bool epoch_capture_active=false,epoch_capture_attempted=false;
 mutable std::weak_ptr<const BnwNativePinnedInstallationEpoch> actual_epoch;
 mutable const RetainedPrimary32Validation* retained_primary32=nullptr;
 void fixed_files()const{
  need(!retired.load()&&std::this_thread::get_id()==thread&&GetCurrentProcessId()==process,
   "Installation creator process/thread/source retired");
  need(std::filesystem::canonical(installation)==installation&&std::filesystem::canonical(payload)==payload&&
   std::filesystem::is_directory(installation/L"Assets/DLC")&&std::filesystem::is_directory(payload/L"Assets"),
   "Installation/canonical asset source root changed");
  for(const auto& file:files)file->require_current();
 }
 void validate_pinned_installation_source_epoch()const{
  // A foreign caller is refused before any mutation; source failures on the
  // creator thread permanently retire the SAME retained storage.
  need(std::this_thread::get_id()==thread&&GetCurrentProcessId()==process,
   "Pinned installation epoch foreign creator process/thread");
  try{fixed_files();}catch(...){retired.store(true);throw;}
 }
 void fixed_sources()const{
  need(!strict_retired.load()&&!retired.load()&&std::this_thread::get_id()==thread&&GetCurrentProcessId()==process,
   "Installation creator process/thread/source retired");
  need(IsWindow(hwnd)!=FALSE,"Installation actual owned window retired");DWORD pid=0;
  GetWindowThreadProcessId(hwnd,&pid);need(pid==process,"Installation window belongs to another process");
  RECT rect{};need(GetClientRect(hwnd,&rect)!=FALSE&&rect.right-rect.left==extent.width&&rect.bottom-rect.top==extent.height,
   "Installation current HWND extent changed");
  if(retained_primary32){
   // ONE actual Root operation has strict Input PRE/POST outside this borrower.
   // SAME admitted HANDLEs remain owned by Impl; their unrelated metadata and
   // named routing are observed at those boundaries. Transient changes wholly
   // inside the operation are intentionally not intermediate source refusals.
   retained_primary32->require_current(); // Original live controller PRE.
   retained_primary32->require_current(); // Original live controller POST.
  }else validate_pinned_installation_source_epoch();
 }
};
bool bnw_native_retained_primary32_borrower_active_for_test(
 const BnwNativeInstallationInputOwner& input)noexcept{
 const auto& i=*input.impl_;
 return std::this_thread::get_id()==i.thread&&GetCurrentProcessId()==i.process&&i.retained_primary32!=nullptr;
}
BnwNativeInstallationInputOwner::RetainedPrimary32Validation::RetainedPrimary32Validation(
 const BnwNativeInstallationInputOwner& input,const BnwNativePackageDiscoveryOwner& parent,
 const ParentFence& fence,Controller controller):input_(input),parent_(parent),fence_(fence),controller_(controller){
 auto& i=*input_.impl_;
 need(std::this_thread::get_id()==i.thread&&GetCurrentProcessId()==i.process,
  "Retained installation borrower foreign creator process/thread");
 need(i.facade_alive.load()&&!i.actual_facade.expired()&&!i.strict_retired.load()&&!i.retired.load(),
  "Retained installation borrower lost actual input facade");
 if(i.busy||i.retained_primary32){i.strict_retired.store(true);throw std::runtime_error("Retained installation borrower reentry");}
 need(controller_&&fence_.valid(),"Retained installation borrower lost actual Parent fence");
 controller_(parent_,fence_,input_);i.retained_primary32=this;
}
void BnwNativeInstallationInputOwner::RetainedPrimary32Validation::require_current()const{
 const auto& i=*input_.impl_;
 need(std::this_thread::get_id()==i.thread&&GetCurrentProcessId()==i.process,
  "Retained installation borrower foreign creator process/thread");
 need(i.retained_primary32==this&&i.facade_alive.load()&&!i.actual_facade.expired()&&
  !i.strict_retired.load()&&!i.retired.load()&&fence_.valid(),"Retained installation borrower retired/reentered");
 controller_(parent_,fence_,input_);
}
BnwNativeInstallationInputOwner::RetainedPrimary32Validation::~RetainedPrimary32Validation()noexcept{
 auto& i=*input_.impl_;
 // Root's existing guard/loan retain storage until this stack borrower unwinds.
 // Never invoke a controller or take a new owner pin during exception cleanup.
 if(std::this_thread::get_id()!=i.thread){i.strict_retired.store(true);return;}
 if(i.retained_primary32!=this)i.strict_retired.store(true);
 i.retained_primary32=nullptr;
}
void BnwNativeInstallationInputOwner::refuse_parent_current_during_retained_primary32_validation()const{
 const auto& i=*impl_;
 need(std::this_thread::get_id()==i.thread&&GetCurrentProcessId()==i.process,
  "Retained installation Parent.current refusal foreign creator process/thread");
 if(i.retained_primary32){i.strict_retired.store(true);
  throw std::runtime_error("Retained installation Parent.current reentry");}
}
BnwNativeInstallationInputOwner::BnwNativeInstallationInputOwner(std::shared_ptr<Impl> i):impl_(std::move(i)){}
BnwNativeInstallationInputOwner::~BnwNativeInstallationInputOwner(){impl_->facade_alive.store(false);}
// Facade lifetime and permanent source failure differ: only the genuine
// Parent-issued file epoch may retain these exact locks after facade death.
std::shared_ptr<const BnwNativeInstallationInputOwner> BnwNativeInstallationInputOwner::acquire(
 const std::filesystem::path& installation,const std::filesystem::path& payload,std::uintptr_t hwnd,
 runtime::BnwStockLuaClientExtentProvider provider){
 need(hwnd&&provider.current,"Installation actual HWND/client source missing");
 auto i=std::make_shared<Impl>();i->installation=std::filesystem::canonical(installation);
 i->payload=std::filesystem::canonical(payload);i->hwnd=reinterpret_cast<HWND>(hwnd);i->provider=std::move(provider);
 const auto extent=i->provider.current();need(extent&&!extent->surface_owner.empty()&&extent->revision&&extent->width>0&&extent->height>0,
  "Installation actual current client unavailable");i->extent=*extent;i->fixed_sources();
 const auto pin=[&](const auto& relative)->const runtime::FileIdentity&{
  i->files.emplace_back(std::make_unique<PinnedFile>(i->installation/relative));return i->files.back()->identity;
 };
 const auto& exe=pin(L"CivilizationV.exe");need(exe.bytes==10602040&&exe.sha256==
  "7c9f93d512c49a7ab2bc70f51c6caa10a82ddd697ac11d8a24ff1bb7ce7c3686","Installation original executable differs from qualified source");
 const auto& core=pin(L"Assets/DLC/Expansion2/CvGameCore_Expansion2.dll");need(core.bytes==4061696&&core.sha256==
  "c5483b5857a6fb7ef107af93c5d34282eb713472e2a84d0eb9d3fdc104e155a5","Installation BNW GameCore differs from qualified source");
 const auto discovered=content::discover_packages({i->installation/L"Assets/DLC"});
 need(discovered.diagnostics.empty(),"Actual installation catalog discovery failed");
 for(const auto& entry:discovered.packages)i->files.emplace_back(std::make_unique<PinnedFile>(entry.path));
 i->discovered=content::discover_packages({i->installation/L"Assets/DLC"});
 need(i->discovered.diagnostics.empty()&&i->discovered.packages.size()==discovered.packages.size(),"Installation catalog changed while locking source");
 for(std::size_t n=0;n<discovered.packages.size();++n)need(i->discovered.packages[n].path==discovered.packages[n].path,
  "Installation manifest order changed while locking source");
 i->fixed_sources();auto result=std::shared_ptr<const BnwNativeInstallationInputOwner>(new BnwNativeInstallationInputOwner(i));
 i->actual_facade=result;result->require_current();return result;
}
void BnwNativeInstallationInputOwner::require_current()const{
 const auto i=impl_;need(std::this_thread::get_id()==i->thread&&GetCurrentProcessId()==i->process,
  "Installation input foreign creator process/thread");
 const auto actual=i->actual_facade.lock();need(actual&&actual.get()==this,"Installation input lost original factory facade");
 need(i->facade_alive.load()&&!i->strict_retired.load()&&!i->retired.load(),"Installation input retired");
 if(i->busy){i->strict_retired.store(true);throw std::runtime_error("Installation client source reentry");}
 i->busy=true;struct End{Impl& i;~End(){i.busy=false;}}end{*i};
 try{i->fixed_sources();const auto client=i->provider.current();
  need(client&&same_client(*client,i->extent),"Installation exact client epoch changed");i->fixed_sources();
 }catch(...){i->strict_retired.store(true);throw;}
}
std::shared_ptr<const BnwNativeInstallationInputOwner>
detail::BnwNativeFilesystemMappingOwner::actual_input_control(
    const std::shared_ptr<const BnwNativeInstallationInputOwner>& supplied){
    need(bool(supplied),"FS bootstrap actual Input absent");const auto state=supplied->impl_;
    need(bool(state),"FS bootstrap actual Input storage absent");
    need(state->thread==std::this_thread::get_id()&&state->process==GetCurrentProcessId(),
         "FS bootstrap Input foreign creator before providers");
    const auto actual=state->actual_facade.lock();
    need(actual&&actual.get()==supplied.get()&&same(actual,supplied)&&actual->impl_.get()==state.get()&&
         state->facade_alive.load()&&!state->strict_retired.load()&&!state->retired.load(),
         "FS bootstrap supplied Input pointer/control is not its actual factory");
    return actual; // Pure canonical control gate; callers retain their real FULL Input bookends.
}

void BnwNativeInstallationInputOwner::require_cold_roots_identity_without_callbacks(
 const std::filesystem::path& original_root,const std::filesystem::path& canonical_root)const{
 const auto& input=*impl_; // Existing facade owns Impl; no additional owner pin.
 const auto current=[&]{
  need(std::this_thread::get_id()==input.thread&&GetCurrentProcessId()==input.process,
   "Installation input foreign creator process/thread");
  need(!input.actual_facade.expired(),"Installation input lost original factory facade");
  need(input.facade_alive.load()&&!input.strict_retired.load()&&!input.retired.load(),"Installation input retired");
 };
 current();
 need(original_root==input.installation&&canonical_root==input.payload&&
  original_root.is_absolute()&&canonical_root.is_absolute(),"Installed Cold SAME physical/logical source roots changed");
 current();
}
BnwNativeInstallationSourceObservation BnwNativeInstallationInputOwner::observe()const{
 require_current();BnwNativeInstallationSourceObservation out;out.installation=impl_->installation;out.canonical_assets=impl_->payload;
 out.client=impl_->extent;for(const auto& file:impl_->files)out.pinned_files.push_back(file->identity);require_current();return out;
}
const std::filesystem::path& BnwNativeInstallationInputOwner::game_root()const{require_current();return impl_->installation;}
const std::filesystem::path& BnwNativeInstallationInputOwner::canonical_payload_root()const{require_current();return impl_->payload;}
const content::Catalog& BnwNativeInstallationInputOwner::catalog()const{require_current();return impl_->discovered;}
std::uintptr_t BnwNativeInstallationInputOwner::actual_hwnd()const{require_current();return reinterpret_cast<std::uintptr_t>(impl_->hwnd);}
const runtime::BnwStockLuaClientExtentProvider& BnwNativeInstallationInputOwner::client_provider()const{require_current();return impl_->provider;}

struct BnwNativePinnedInstallationEpoch::Impl {
 const std::shared_ptr<const BnwNativeInstallationInputOwner::Impl> original;
 explicit Impl(std::shared_ptr<const BnwNativeInstallationInputOwner::Impl> input):original(std::move(input)){}
};
BnwNativePinnedInstallationEpoch::BnwNativePinnedInstallationEpoch(std::shared_ptr<const Impl> value):impl_(std::move(value)){}
BnwNativePinnedInstallationEpoch::~BnwNativePinnedInstallationEpoch()=default;
void BnwNativePinnedInstallationEpoch::require_current()const{
 const auto epoch=impl_;epoch->original->validate_pinned_installation_source_epoch();
 const auto actual=epoch->original->actual_epoch.lock();
 need(actual&&actual.get()==this,"Pinned installation epoch lost SAME actual private issuance");
 epoch->original->validate_pinned_installation_source_epoch();
}
void BnwNativePinnedInstallationEpoch::require_same_installation_input(const BnwNativeInstallationInputOwner& actual)const{
 require_current();need(impl_->original.get()==actual.impl_.get(),"Pinned installation epoch substituted original storage");require_current();
}
const std::filesystem::path& BnwNativePinnedInstallationEpoch::game_root()const{require_current();return impl_->original->installation;}
const std::filesystem::path& BnwNativePinnedInstallationEpoch::canonical_payload_root()const{require_current();return impl_->original->payload;}
const content::Catalog& BnwNativePinnedInstallationEpoch::catalog()const{require_current();return impl_->original->discovered;}
std::shared_ptr<const BnwNativePinnedInstallationEpoch>
BnwNativeInstallationInputOwner::capture_pinned_installation_epoch_after_parent_return()const{
 const auto i=impl_;need(std::this_thread::get_id()==i->thread&&GetCurrentProcessId()==i->process,
  "Installation post-return capture foreign creator process/thread");
 const auto actual=i->actual_facade.lock();need(actual&&actual.get()==this&&i->facade_alive.load(),
  "Installation post-return capture lost original factory facade");
 if(i->busy||i->epoch_capture_active){i->retired.store(true);throw std::runtime_error("Installation post-return capture reentry");}
 need(!i->strict_retired.load()&&!i->retired.load()&&!i->epoch_capture_attempted&&i->actual_epoch.expired(),"Installation post-return capture retired/replayed");
 i->epoch_capture_active=true;i->epoch_capture_attempted=true;
 struct Capture{Impl& input;bool accepted=false;~Capture(){input.epoch_capture_active=false;if(!accepted)input.retired.store(true);}}capture{*i};
 // No HWND, extent callback, caller bool or copied receipt is used here.
 // The private Parent callsite MUST be its genuine whole-operation return.
 i->validate_pinned_installation_source_epoch();
 const auto epoch=std::shared_ptr<const BnwNativePinnedInstallationEpoch>(new BnwNativePinnedInstallationEpoch(
  std::make_shared<const BnwNativePinnedInstallationEpoch::Impl>(i)));
 i->validate_pinned_installation_source_epoch();i->actual_epoch=epoch;
 epoch->require_same_installation_input(*this);i->validate_pinned_installation_source_epoch();
 capture.accepted=true;return epoch;
}
}
