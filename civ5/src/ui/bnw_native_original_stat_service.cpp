#include "bnw_native_original_stat_service.hpp"
#include "civ5/ui/bnw_native_installation_input_owner.hpp"
#include "bnw_native_common_source_validation_context.hpp"
#include "bnw_native_completed_cold_source_loan.hpp"
#include "civ5/ui/detail/bnw_native_original_stat_path.hpp"
#include "civ5/runtime/file_identity.hpp"
#include "civ5/assets/asset_source.hpp"
#include <algorithm>
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <stdexcept>
#include <sys/stat.h>
#include <tuple>
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

namespace civ5::ui::detail {
namespace {
void need(bool value,const char* message) { if(!value)throw std::runtime_error(message); }
static_assert(sizeof(void*)==8 && sizeof(struct _stat64i32)==48 &&
    offsetof(struct _stat64i32,st_size)==0x14 && offsetof(struct _stat64i32,st_mtime)==0x20);
std::string ascii_source(const std::filesystem::path& path) {
    const auto bytes=assets::path_utf8(path);
    need(std::all_of(bytes.begin(),bytes.end(),[](unsigned char c){return c<128 && c!=0;}),
         "Original stat service has no source CP_ACP path record owner for non-ASCII bytes");
    return bytes;
}
BnwNativeOriginalStatFields source_stat(const std::filesystem::path& path) {
    const auto bytes=ascii_source(path);
    struct _stat64i32 output{}; errno=0;
    BnwNativeOriginalStatFields result;
    result.return_value=_stat64i32(bytes.c_str(),&output);result.error_number=errno;
    if(result.return_value==0) { result.size=static_cast<std::int32_t>(output.st_size);result.mtime=output.st_mtime; }
    return result; // Initialised port buffer never supplies failed native fields.
}
HMODULE actual_stat_module() {
    HMODULE module=nullptr;
    need(GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
        reinterpret_cast<const wchar_t*>(&_stat64i32),&module),"Original stat loaded CRT unavailable");
    need(reinterpret_cast<std::uintptr_t>(GetProcAddress(module,"_stat64i32"))==
        reinterpret_cast<std::uintptr_t>(&_stat64i32),"Original stat import owner differs");
    return module;
}
std::filesystem::path actual_module_file(HMODULE module) {
    std::wstring path(32768,L'\0');
    const auto length=GetModuleFileNameW(module,path.data(),static_cast<DWORD>(path.size()));
    need(length && length<path.size(),"Original stat loaded CRT path incomplete");
    path.resize(length);return std::filesystem::canonical(path);
}
auto crt_file_identity(const BY_HANDLE_FILE_INFORMATION& value) {
    // Link count/access time may change without replacing or writing contents.
    // Volume/fileID, size, last-write time and attributes authenticate the
    // SAME ordinary file captured while denying write/delete sharing.
    return std::tuple(value.dwVolumeSerialNumber,value.nFileIndexHigh,value.nFileIndexLow,
        value.nFileSizeHigh,value.nFileSizeLow,value.ftLastWriteTime.dwHighDateTime,
        value.ftLastWriteTime.dwLowDateTime,value.dwFileAttributes);
}
void require_pinned_crt_file(HANDLE held,const BY_HANDLE_FILE_INFORMATION& captured,
                             const std::filesystem::path& path) {
    BY_HANDLE_FILE_INFORMATION current{};
    need(held!=INVALID_HANDLE_VALUE && GetFileInformationByHandle(held,&current) &&
         crt_file_identity(current)==crt_file_identity(captured),
         "Original stat held CRT source identity changed");
    // Opening the current named path authenticates routing as well as the
    // retained handle. A redirected/retargeted path cannot borrow old bytes.
    struct CurrentPath {
        HANDLE value=INVALID_HANDLE_VALUE;
        ~CurrentPath(){if(value!=INVALID_HANDLE_VALUE)CloseHandle(value);}
    } selected;
    selected.value=CreateFileW(path.c_str(),GENERIC_READ,FILE_SHARE_READ,nullptr,
                               OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);
    need(selected.value!=INVALID_HANDLE_VALUE && GetFileInformationByHandle(selected.value,&current) &&
         crt_file_identity(current)==crt_file_identity(captured),
         "Original stat loaded CRT path no longer selects held source");
    need(GetFileInformationByHandle(held,&current) &&
         crt_file_identity(current)==crt_file_identity(captured),
         "Original stat held CRT source changed during path authentication");
}
void no_custom_timezone() {
    SetLastError(ERROR_SUCCESS);
    need(!std::getenv("TZ") && GetEnvironmentVariableA("TZ",nullptr,0)==0 &&
        GetLastError()==ERROR_ENVVAR_NOT_FOUND,"Finite original stat proof excludes custom TZ");
}
void require_proven_timezone(const TIME_ZONE_INFORMATION& info,DWORD state) {
    static_assert(sizeof(TIME_ZONE_INFORMATION)==172);
    constexpr char proof[]="4cffffff520054005a002000320020002800370438043c0430042900000038003300320000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000520054005a0020003200200028003b04350442043e0429000000380033003100000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000c4ffffff";
    static_assert(sizeof(proof)==sizeof(info)*2+1);
    constexpr char hex[]="0123456789abcdef";
    need(state==TIME_ZONE_ID_UNKNOWN,"Finite original stat proof timezone state differs");
    const auto* bytes=reinterpret_cast<const unsigned char*>(&info);
    for(std::size_t j=0;j<sizeof(info);++j)
        need(proof[j*2]==hex[bytes[j]>>4] && proof[j*2+1]==hex[bytes[j]&15],
            "Finite original stat proof timezone structure differs");
}
}
struct BnwNativeOriginalStatService::Impl {
    std::shared_ptr<const BnwNativeEarlyInputLease> lease;
    std::shared_ptr<const ::civ5::ui::BnwNativeInstallationInputOwner> installation_input;
    std::weak_ptr<const BnwNativeOriginalStatService> self;
    std::filesystem::path original_root;
    BnwNativeOriginalStatSource source;
    HMODULE crt=nullptr;
    HANDLE crt_file=INVALID_HANDLE_VALUE;
    BY_HANDLE_FILE_INFORMATION crt_identity{};
    TIME_ZONE_INFORMATION timezone{};
    DWORD timezone_state=TIME_ZONE_ID_INVALID;
    mutable std::uint64_t issued=0;
    ~Impl() { if(crt_file!=INVALID_HANDLE_VALUE)CloseHandle(crt_file); }
    void validate() const {
        if(installation_input){
            need(!lease,"Original Stat installed domain cannot carry imported save lease");
            need(GetCurrentProcessId()==source.process_id&&GetCurrentThreadId()==source.thread_id&&GetACP()==source.code_page,
                 "Installed Original stat belongs to another process/thread/ACP");
            installation_input->require_current();no_custom_timezone();
            TIME_ZONE_INFORMATION now{};const auto state=GetTimeZoneInformation(&now);
            need(state==timezone_state&&std::memcmp(&now,&timezone,sizeof(now))==0,"Installed Original stat ambient timezone changed");
            need(actual_stat_module()==crt&&actual_module_file(crt)==source.loaded_crt,"Installed Original stat actual CRT module/path changed");
            require_pinned_crt_file(crt_file,crt_identity,source.loaded_crt);installation_input->require_current();return;
        }
        need(GetCurrentProcessId()==source.process_id && GetCurrentThreadId()==source.thread_id &&
            GetACP()==source.code_page,"Original stat service belongs to another process/thread/ACP");
        lease->require_current();no_custom_timezone();
        TIME_ZONE_INFORMATION now{};const auto state=GetTimeZoneInformation(&now);
        need(state==timezone_state && std::memcmp(&now,&timezone,sizeof(now))==0,
            "Original stat service ambient timezone changed");
        need(actual_stat_module()==crt && actual_module_file(crt)==source.loaded_crt,
             "Original stat loaded CRT module/path changed");
        require_pinned_crt_file(crt_file,crt_identity,source.loaded_crt);
        lease->require_current();
    }
    void validate_completed(const BnwNativeCompletedColdSourceLoan& loan)const {
        need(lease&&!installation_input,"Mandatory installed Original Stat completed loan before late source use");
        need(GetCurrentProcessId()==source.process_id&&GetCurrentThreadId()==source.thread_id&&
             GetACP()==source.code_page,"Completed Original stat foreign process/thread/ACP");
        loan.require_same_early_input(*lease);no_custom_timezone();
        TIME_ZONE_INFORMATION now{};const auto state=GetTimeZoneInformation(&now);
        need(state==timezone_state&&std::memcmp(&now,&timezone,sizeof(now))==0,
             "Completed Original stat ambient timezone changed");
        need(actual_stat_module()==crt&&actual_module_file(crt)==source.loaded_crt,
             "Completed Original stat loaded CRT module/path changed");
        require_pinned_crt_file(crt_file,crt_identity,source.loaded_crt);
        loan.require_same_early_input(*lease);
    }
};
// Same ambient/stat source checks; only the genuine input-currentness operand
// is supplied by an explicit completed Cold loan, never a public fallback.
// Original stat service/read fields remain the actual constructor-stage data.
struct BnwNativeOriginalStatRead::Impl {
    std::weak_ptr<const BnwNativeOriginalStatService> service;
    std::filesystem::path relative,absolute;
    std::uint64_t sequence=0;
    BnwNativeOriginalStatFields fields;
    std::shared_ptr<const BnwNativeOriginalStatService> owner() const {
        auto locked=service.lock();need(bool(locked),"Original stat read service expired");return locked;
    }
};
BnwNativeOriginalStatRead::BnwNativeOriginalStatRead(std::unique_ptr<Impl> i):impl_(std::move(i)) {}
BnwNativeOriginalStatRead::~BnwNativeOriginalStatRead()=default;
const BnwNativeOriginalStatFields& BnwNativeOriginalStatRead::current() const {
    impl_->owner()->require_current_read(*this);return impl_->fields;
}
const std::filesystem::path& BnwNativeOriginalStatRead::relative_source() const {
    (void)current();return impl_->relative;
}
std::uint64_t BnwNativeOriginalStatRead::sequence() const { (void)current();return impl_->sequence; }
void BnwNativeOriginalStatRead::require_same_service(const BnwNativeOriginalStatService& service) const {
    need(impl_->owner().get()==&service,"Original stat read requires SAME retained service");
    service.require_current_read(*this);
}
BnwNativeOriginalStatService::BnwNativeOriginalStatService(std::unique_ptr<Impl> i):impl_(std::move(i)) {}
BnwNativeOriginalStatService::~BnwNativeOriginalStatService()=default;
std::shared_ptr<const BnwNativeOriginalStatService> BnwNativeOriginalStatService::acquire(
    std::shared_ptr<const BnwNativeEarlyInputLease> lease) {
    need(bool(lease),"Original stat service requires genuine EARLY input lease");lease->require_current();
    auto i=std::make_unique<Impl>();i->lease=std::move(lease);i->original_root=i->lease->game_root();
    (void)ascii_source(i->original_root);
    i->source.process_id=GetCurrentProcessId();i->source.thread_id=GetCurrentThreadId();i->source.code_page=GetACP();
    need(i->source.code_page==1251,"Finite original stat proof covers ACP1251 only");no_custom_timezone();
    i->timezone_state=GetTimeZoneInformation(&i->timezone);require_proven_timezone(i->timezone,i->timezone_state);
    i->crt=actual_stat_module();i->source.loaded_crt=actual_module_file(i->crt);
    i->crt_file=CreateFileW(i->source.loaded_crt.c_str(),GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);
    need(i->crt_file!=INVALID_HANDLE_VALUE,"Original stat loaded CRT cannot be pinned read-only");
    need(GetFileInformationByHandle(i->crt_file,&i->crt_identity) &&
         (i->crt_identity.dwFileAttributes&(FILE_ATTRIBUTE_DIRECTORY|FILE_ATTRIBUTE_REPARSE_POINT))==0,
         "Original stat loaded CRT requires held direct regular-file identity");
    require_pinned_crt_file(i->crt_file,i->crt_identity,i->source.loaded_crt);
    i->source.loaded_crt_sha256=runtime::identify_file(i->source.loaded_crt).sha256;
    if(i->source.loaded_crt_sha256!="5c52e3a303baaac0e0af8bd9b96134993da34bc9d834a31ef37e1d2cdc7fe192"&&
       i->source.loaded_crt_sha256!="dedb7611e1abc1ff3852dcac3d4c93a048d729e25b061973b9969f22698f14ca")
        throw std::runtime_error("Original stat UCRT differs from finite VC90 differential source; loaded_crt="+assets::path_utf8(i->source.loaded_crt)+
                                 "; actual_sha256="+i->source.loaded_crt_sha256);
    i->validate();
    auto service=std::shared_ptr<const BnwNativeOriginalStatService>(new BnwNativeOriginalStatService(std::move(i)));
    service->impl_->self=service;service->require_current();return service;
}
std::shared_ptr<const BnwNativeOriginalStatService> BnwNativeOriginalStatService::acquire(
    std::shared_ptr<const ::civ5::ui::BnwNativeInstallationInputOwner> installation) {
    need(bool(installation),"Original Stat service requires genuine installed source");installation->require_current();
    auto i=std::make_unique<Impl>();i->installation_input=std::move(installation);i->original_root=i->installation_input->game_root();
    (void)ascii_source(i->original_root);
    i->source.process_id=GetCurrentProcessId();i->source.thread_id=GetCurrentThreadId();i->source.code_page=GetACP();
    need(i->source.code_page==1251,"Finite original stat proof covers ACP1251 only");no_custom_timezone();
    i->timezone_state=GetTimeZoneInformation(&i->timezone);require_proven_timezone(i->timezone,i->timezone_state);
    i->crt=actual_stat_module();i->source.loaded_crt=actual_module_file(i->crt);
    i->crt_file=CreateFileW(i->source.loaded_crt.c_str(),GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);
    need(i->crt_file!=INVALID_HANDLE_VALUE,"Original stat loaded CRT cannot be pinned read-only");
    need(GetFileInformationByHandle(i->crt_file,&i->crt_identity) &&
         (i->crt_identity.dwFileAttributes&(FILE_ATTRIBUTE_DIRECTORY|FILE_ATTRIBUTE_REPARSE_POINT))==0,
         "Original stat loaded CRT requires held direct regular-file identity");
    require_pinned_crt_file(i->crt_file,i->crt_identity,i->source.loaded_crt);
    i->source.loaded_crt_sha256=runtime::identify_file(i->source.loaded_crt).sha256;
    if(i->source.loaded_crt_sha256!="5c52e3a303baaac0e0af8bd9b96134993da34bc9d834a31ef37e1d2cdc7fe192"&&
       i->source.loaded_crt_sha256!="dedb7611e1abc1ff3852dcac3d4c93a048d729e25b061973b9969f22698f14ca")
        throw std::runtime_error("Original stat UCRT differs from finite VC90 differential source; loaded_crt="+assets::path_utf8(i->source.loaded_crt)+
                                 "; actual_sha256="+i->source.loaded_crt_sha256);
    i->validate();
    auto service=std::shared_ptr<const BnwNativeOriginalStatService>(new BnwNativeOriginalStatService(std::move(i)));
    service->impl_->self=service;service->require_current();return service;
}
void BnwNativeOriginalStatService::require_same_input(const ::civ5::ui::BnwNativeInstallationInputOwner& input)const{
    require_current();need(!impl_->lease&&impl_->installation_input.get()==&input,"Original stat has another real installation source");require_current();
}
void BnwNativeOriginalStatService::require_current() const { impl_->validate(); }
void BnwNativeOriginalStatService::require_same_input(const BnwNativeEarlyInputLease& lease) const {
    require_current();need(impl_->lease.get()==&lease,"Original stat requires SAME genuine input lease");
}
const BnwNativeOriginalStatSource& BnwNativeOriginalStatService::source() const { require_current();return impl_->source; }
std::shared_ptr<const BnwNativeOriginalStatRead> BnwNativeOriginalStatService::read_original_relative(
    const std::filesystem::path& relative) const {
    // Reject bad caller spelling before filesystem work. The retained root is
    // privately derived at acquire from the actual lease, never a caller alias.
    const auto source=confined_original_stat_path(impl_->original_root,relative);
    (void)ascii_source(source);require_current();
    auto read=std::make_unique<BnwNativeOriginalStatRead::Impl>();
    read->service=impl_->self;read->absolute=source;read->relative=relative.lexically_normal();
    read->fields=source_stat(source);require_current();
    need(source_stat(source)==read->fields,"Original stat source changed during read");
    need(impl_->issued!=std::numeric_limits<std::uint64_t>::max(),"Original stat read sequence exhausted");
    read->sequence=++impl_->issued;
    return std::shared_ptr<const BnwNativeOriginalStatRead>(new BnwNativeOriginalStatRead(std::move(read)));
}
void BnwNativeOriginalStatService::require_current_read(const BnwNativeOriginalStatRead& read) const {
    need(read.impl_->owner().get()==this && read.impl_->sequence && read.impl_->sequence<=impl_->issued,
        "Original stat read lacks SAME service sequence");require_current();
    need(source_stat(read.impl_->absolute)==read.impl_->fields,"Original stat read source metadata changed");
    require_current();
}

void BnwNativeOriginalStatService::require_current_completed(const BnwNativeCompletedColdSourceLoan& loan)const {
    impl_->validate_completed(loan);
}
void BnwNativeOriginalStatService::require_same_input_completed(const BnwNativeEarlyInputLease& input,
    const BnwNativeCompletedColdSourceLoan& loan)const {
    loan.require_same_early_input(input);require_current_completed(loan);
    need(impl_->lease.get()==&input,"Completed Original stat belongs to another original lease");
    require_current_completed(loan);
}
void BnwNativeOriginalStatService::require_current_read_completed(const BnwNativeOriginalStatRead& read,
    const BnwNativeCompletedColdSourceLoan& loan)const {
    loan.require_current();
    need(read.impl_->owner().get()==this&&read.impl_->sequence&&read.impl_->sequence<=impl_->issued,
         "Completed Original stat read lost same service sequence");require_current_completed(loan);
    need(read.impl_->absolute==confined_original_stat_path(impl_->original_root,read.impl_->relative),
         "Completed Original stat retained relative/absolute route differs");
    need(source_stat(read.impl_->absolute)==read.impl_->fields,"Completed Original stat original metadata changed");
    require_current_completed(loan);
}
const BnwNativeOriginalStatFields& BnwNativeOriginalStatRead::current_completed(
    const BnwNativeCompletedColdSourceLoan& loan)const {
    loan.require_current();const auto service=impl_->owner();service->require_current_read_completed(*this,loan);
    loan.require_current();return impl_->fields;
}
const std::filesystem::path& BnwNativeOriginalStatRead::relative_source_completed(
    const BnwNativeCompletedColdSourceLoan& loan)const {
    (void)current_completed(loan);return impl_->relative;
}
void BnwNativeOriginalStatRead::require_same_service_completed(const BnwNativeOriginalStatService& service,
    const BnwNativeCompletedColdSourceLoan& loan)const {
    loan.require_current();need(impl_->owner().get()==&service,"Completed Original stat read has another service");
    service.require_current_read_completed(*this,loan);loan.require_current();
}
void BnwNativeOriginalStatService::require_common_source_current(BnwNativeCommonSourceValidationContext& context)const {
    const auto check=[&]{context.check();need(!impl_->self.expired()&&impl_->lease&&
        GetCurrentProcessId()==impl_->source.process_id&&GetCurrentThreadId()==impl_->source.thread_id&&
        GetACP()==impl_->source.code_page,"Common Original stat actual creator/ACP/service changed");};
    check();auto visit=context.enter(*this,impl_->self);
    if(visit.work()==BnwNativeCommonSourceValidationContext::Work::validate_source) {
        impl_->lease->require_common_source_current(context);check();no_custom_timezone();
        TIME_ZONE_INFORMATION now{};const auto state=GetTimeZoneInformation(&now);
        need(state==impl_->timezone_state&&std::memcmp(&now,&impl_->timezone,sizeof(now))==0,
             "Common Original stat timezone changed");
        need(actual_stat_module()==impl_->crt&&actual_module_file(impl_->crt)==impl_->source.loaded_crt,
             "Common Original stat actual CRT module/path changed");
        require_pinned_crt_file(impl_->crt_file,impl_->crt_identity,impl_->source.loaded_crt);
        impl_->lease->require_common_source_current(context);check();
    }
    check();visit.complete();
}
void BnwNativeOriginalStatService::require_same_input_common_source(const BnwNativeEarlyInputLease& input,BnwNativeCommonSourceValidationContext& context)const {
    require_common_source_current(context);need(impl_->lease.get()==&input,"Common Original stat has another Early source");context.check();
}
void BnwNativeOriginalStatService::require_current_read_common_source(const BnwNativeOriginalStatRead& read,BnwNativeCommonSourceValidationContext& context)const {
    const auto check=[&]{context.check();need(!read.impl_->service.expired()&&
        !read.impl_->service.owner_before(impl_->self)&&!impl_->self.owner_before(read.impl_->service)&&
        read.impl_->sequence&&read.impl_->sequence<=impl_->issued,"Common Original stat read service/sequence changed");};
    check();auto visit=context.enter(read,read.impl_->service,read.impl_->sequence);
    require_common_source_current(context);check();
    if(visit.work()==BnwNativeCommonSourceValidationContext::Work::validate_source) {
        need(source_stat(read.impl_->absolute)==read.impl_->fields,"Common Original stat actual retained fields changed");check();
        require_common_source_current(context);check();
    }
    check();visit.complete();
}
void BnwNativeOriginalStatRead::require_common_source_current(BnwNativeCommonSourceValidationContext& context)const {
    context.check();const auto service=impl_->owner();service->require_current_read_common_source(*this,context);context.check();
}
const BnwNativeOriginalStatFields& BnwNativeOriginalStatRead::current_common_source(BnwNativeCommonSourceValidationContext& context)const {
    require_common_source_current(context);context.require_borrowed(*this,impl_->service,impl_->sequence);return impl_->fields;
}
const std::filesystem::path& BnwNativeOriginalStatRead::relative_source_common_source(BnwNativeCommonSourceValidationContext& context)const {
    require_common_source_current(context);context.require_borrowed(*this,impl_->service,impl_->sequence);return impl_->relative;
}
void BnwNativeOriginalStatRead::require_same_service_common_source(const BnwNativeOriginalStatService& service,BnwNativeCommonSourceValidationContext& context)const {
    context.check();{const auto actual=impl_->owner();need(actual.get()==&service,"Common Original stat read has another service");}
    service.require_current_read_common_source(*this,context);context.check();
}
}
