#include "civ5/ui/bnw_native_package_archive_time_owner.hpp"
#include "civ5/ui/bnw_native_early_input_lease.hpp"
#include "civ5/ui/detail/bnw_native_original_stat_path.hpp"
#include "civ5/runtime/file_identity.hpp"
#include "bnw_native_package_category_source_owner.hpp"
#include <algorithm>
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <sys/stat.h>
#include <tuple>
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
namespace civ5::ui {
namespace {
void need(bool v,const char* why){if(!v)throw std::runtime_error(why);}
static_assert(sizeof(void*)==8 && sizeof(struct _stat64i32)==48 &&
    offsetof(struct _stat64i32,st_size)==0x14 && offsetof(struct _stat64i32,st_mtime)==0x20);
std::string native_ascii(const std::filesystem::path& path){
    auto bytes=assets::path_utf8(path);
    need(std::all_of(bytes.begin(),bytes.end(),[](unsigned char c){return c<128;}),
         "Native category stat path requires source CP_ACP record owner");
    return bytes;
}
BnwNativeStat64Receipt source_stat(const std::filesystem::path& path){
    struct _stat64i32 output{};
    const auto bytes=native_ascii(path);errno=0;
    BnwNativeStat64Receipt receipt;
    receipt.return_value=_stat64i32(bytes.c_str(),&output);receipt.error_number=errno;
    if(receipt.return_value==0){receipt.size=static_cast<std::int32_t>(output.st_size);receipt.mtime=output.st_mtime;}
    // The port's buffer initialization is never native failed output authority.
    return receipt;
}
HMODULE stat_module(){
    HMODULE module=nullptr;
    need(GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
         reinterpret_cast<const wchar_t*>(&_stat64i32),&module),"Native stat port loaded module unavailable");
    need(reinterpret_cast<std::uintptr_t>(GetProcAddress(module,"_stat64i32"))==
         reinterpret_cast<std::uintptr_t>(&_stat64i32),"Native stat port import owner differs");
    return module;
}
bool same_entry(const assets::FpkEntry& a,const assets::FpkEntry& b){
    return std::tie(a.ordinal,a.original_name,a.normalized_name,a.directory_record_offset,a.metadata_offset,
        a.padding_length,a.file_time_raw,a.unknown_raw,a.payload_offset,a.payload_length,a.direct_metadata_was_valid)==
        std::tie(b.ordinal,b.original_name,b.normalized_name,b.directory_record_offset,b.metadata_offset,
        b.padding_length,b.file_time_raw,b.unknown_raw,b.payload_offset,b.payload_length,b.direct_metadata_was_valid);
}
// Independent VC90/UCRT field parity was measured only under this exact
// Windows timezone structure/state. No caller path/value/receipt can widen it.
void require_proven_timezone(const TIME_ZONE_INFORMATION& info,DWORD state){
    static_assert(sizeof(TIME_ZONE_INFORMATION)==172);
    constexpr char proof[]="4cffffff520054005a002000320020002800370438043c0430042900000038003300320000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000520054005a0020003200200028003b04350442043e0429000000380033003100000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000c4ffffff";
    static_assert(sizeof(proof)==sizeof(info)*2+1);
    constexpr char hex[]="0123456789abcdef";
    need(state==TIME_ZONE_ID_UNKNOWN,"Native finite stat field proof timezone state differs");
    const auto* bytes=reinterpret_cast<const unsigned char*>(&info);
    for(std::size_t j=0;j<sizeof(info);++j)
        need(proof[j*2]==hex[bytes[j]>>4] && proof[j*2+1]==hex[bytes[j]&15],
             "Native finite stat field proof timezone structure differs");
}
void no_custom_timezone(){
    SetLastError(ERROR_SUCCESS);
    need(!std::getenv("TZ") && GetEnvironmentVariableA("TZ",nullptr,0)==0 &&
         GetLastError()==ERROR_ENVVAR_NOT_FOUND,"Native finite stat field proof does not cover custom TZ");
}
}
struct BnwNativePackageArchiveTimeOwner::Impl{
    std::weak_ptr<const BnwNativePackageDiscoveryOwner> packages;
    std::shared_ptr<const BnwNativePackagePreSkinOwner> predecessor;
    std::shared_ptr<const BnwNativeEarlyInputLease> lease;
    BnwNativePackageArchiveTimeFrame frame;
    HMODULE crt=nullptr;TIME_ZONE_INFORMATION timezone{};DWORD timezone_state=TIME_ZONE_ID_INVALID;
    struct Cache{std::size_t attempt;std::shared_ptr<assets::FpkSource> original,canonical;};
    std::vector<Cache> caches;
    void validate(const BnwNativePackageArchiveTimeOwner* category_borrow=nullptr)const{
        need(GetCurrentProcessId()==frame.process_id && GetCurrentThreadId()==frame.thread_id &&
             GetACP()==frame.code_page,"Native stat metadata belongs to another process/thread/ACP");
        const auto parent=packages.lock();
        need(static_cast<bool>(parent),"Native stat metadata actual package parent expired");
        lease->require_current();
        if(category_borrow){parent->require_category_archive_borrow_current(*category_borrow);
            (void)predecessor->category_operation_frame(*parent);}
        else predecessor->require_same_resources(*parent);
        no_custom_timezone();
        TIME_ZONE_INFORMATION now{};const auto state=GetTimeZoneInformation(&now);
        need(state==timezone_state && std::memcmp(&now,&timezone,sizeof(now))==0,
             "Native stat source timezone changed");
        need(stat_module()==crt && runtime::identify_file(frame.loaded_crt).sha256==frame.loaded_crt_sha256,
             "Native stat port module/source changed");
        for(const auto& attempt:frame.attempts){
            need(source_stat(lease->game_root()/attempt.relative_source)==attempt.original,
                 "Native original archive stat source changed");
            if(attempt.archive_loaded)
                need(source_stat(lease->canonical_payload_root()/attempt.relative_source)==attempt.canonical_payload,
                     "Native canonical archive payload stat changed");
        }
        lease->require_current();
    }
};
BnwNativePackageArchiveTimeOwner::BnwNativePackageArchiveTimeOwner(std::unique_ptr<Impl> i):impl_(std::move(i)){}
BnwNativePackageArchiveTimeOwner::~BnwNativePackageArchiveTimeOwner()=default;
std::shared_ptr<const BnwNativePackageArchiveTimeOwner> BnwNativePackageArchiveTimeOwner::create(
    std::shared_ptr<const BnwNativePackagePreSkinOwner> predecessor,
    std::shared_ptr<const BnwNativePackageDiscoveryOwner> packages){
    need(predecessor && packages,"Native archive time requires actual predecessor/resources");
    predecessor->require_same_resources(*packages);
    auto i=std::make_unique<Impl>();i->packages=packages;i->predecessor=std::move(predecessor);
    i->lease=packages->early_input_lease();i->lease->require_current();
    need(packages->current().common_high_quality_registration_complete,
         "Native archive time requires SAME high Common source stage");
    i->frame.process_id=GetCurrentProcessId();i->frame.thread_id=GetCurrentThreadId();i->frame.code_page=GetACP();
    need(i->frame.code_page==1251,"Native stat finite field proof covers current ACP1251 only");
    no_custom_timezone();i->timezone_state=GetTimeZoneInformation(&i->timezone);
    need(i->timezone_state!=TIME_ZONE_ID_INVALID,"Native stat actual timezone unavailable");
    require_proven_timezone(i->timezone,i->timezone_state);
    i->crt=stat_module();std::wstring path(32768,L'\0');
    const auto n=GetModuleFileNameW(i->crt,path.data(),static_cast<DWORD>(path.size()));
    need(n && n<path.size(),"Native stat module path incomplete");path.resize(n);
    i->frame.loaded_crt=std::filesystem::canonical(path);
    i->frame.loaded_crt_sha256=runtime::identify_file(i->frame.loaded_crt).sha256;
    if(i->frame.loaded_crt_sha256!="5c52e3a303baaac0e0af8bd9b96134993da34bc9d834a31ef37e1d2cdc7fe192"&&
       i->frame.loaded_crt_sha256!="dedb7611e1abc1ff3852dcac3d4c93a048d729e25b061973b9969f22698f14ca")
        throw std::runtime_error("Native stat loaded x64 UCRT differs from finite VC90 differential source; loaded_crt="+assets::path_utf8(i->frame.loaded_crt)+
                                 "; actual_sha256="+i->frame.loaded_crt_sha256);
    for(const auto& row:i->predecessor->current().packages){
        if(!row.declared_inputs_valid)continue; //native flag40 skips manager construction.
        const auto descriptor=content::package_file(i->lease->game_root()/"Assets",row.descriptor);
        const auto& manifest=packages->retained_descriptor_for_pre_skin(row.descriptor);
        const auto& raw=packages->retained_descriptor_raw_for_category(row.descriptor);
        native_ui_skin_text_detail::require_captured_bytes(raw,runtime::identify_file(descriptor));
        const auto lexical=native_ui_skin_text_detail::retain_runs(manifest.document,raw);
        for(unsigned category=0;category<2;++category){
            ++i->frame.eligible_categories;
            for(const auto& declared:native_category_source_detail::category_values(
                    manifest.document,lexical,category,"Paks")){
                const auto value=std::filesystem::path(declared);
                need(!declared.empty() && declared.find_first_of("/\\") == std::string::npos &&
                     runtime::ascii_lower(assets::path_utf8(value.extension()))==".fpk",
                     "Native Paks basename parser requires source-owned path schema");
                //81DC40 first strips source backslash directory then truncates at
                //the FIRST dot through CRT strchr; accepted declarations are flat.
                const auto dot=declared.find('.');
                const auto name=native_ascii(std::filesystem::path(declared.substr(0,dot)));
                for(const auto* prefix:{"Resource/DX9/","Resource/Common/"}){
                    BnwNativePackageArchiveTimeRead read;
                    read.descriptor=row.descriptor;read.declared=declared;read.category=category;
                    read.relative_source=std::string(prefix)+name+".fpk";
                    const auto original=i->lease->game_root()/read.relative_source;
                    read.original=source_stat(original);
                    const auto attributes=GetFileAttributesA(native_ascii(original).c_str());
                    if(attributes==INVALID_FILE_ATTRIBUTES){
                        const auto error=GetLastError();
                        need(error==ERROR_FILE_NOT_FOUND || error==ERROR_PATH_NOT_FOUND,
                             "Native Paks archive open refusal requires OS failure owner");
                        i->frame.attempts.push_back(std::move(read));continue;
                    }
                    auto original_source=assets::FpkSource::open(original,read.relative_source);
                    const auto* os=std::get_if<std::shared_ptr<assets::FpkSource>>(&original_source);
                    need(os,"Native present Paks archive parser/open needs source owner");
                    const auto canonical=i->lease->canonical_payload_root()/read.relative_source;
                    read.canonical_payload=source_stat(canonical);
                    auto canonical_source=assets::FpkSource::open(canonical,read.relative_source);
                    const auto* cs=std::get_if<std::shared_ptr<assets::FpkSource>>(&canonical_source);
                    need(cs && (*os)->filename_scrambling()==(*cs)->filename_scrambling() &&
                         (*os)->extra_header_byte()==(*cs)->extra_header_byte() &&
                         (*os)->directory_end()==(*cs)->directory_end() && (*os)->entries().size()==(*cs)->entries().size() &&
                         std::equal((*os)->entries().begin(),(*os)->entries().end(),(*cs)->entries().begin(),same_entry),
                         "Native canonical Paks archive metadata differs from original");
                    need(source_stat(original)==read.original && source_stat(canonical)==read.canonical_payload,
                         "Native Paks stat source changed during archive capture");
                    need(!read.original.size || !read.canonical_payload.size ||
                         read.original.size==read.canonical_payload.size,
                         "Native canonical Paks payload byte length differs from original");
                    read.archive_loaded=true;
                    read.native_cached_archive_mtime=read.original.mtime.value_or(0);
                    i->caches.push_back({i->frame.attempts.size(),*os,*cs});
                    i->frame.attempts.push_back(std::move(read));++i->frame.archives_loaded;break;
                }
            }
        }
    }
    i->frame.archive_metadata_source_complete=true;i->validate();
    return std::shared_ptr<const BnwNativePackageArchiveTimeOwner>(new BnwNativePackageArchiveTimeOwner(std::move(i)));
}
const BnwNativePackageArchiveTimeFrame& BnwNativePackageArchiveTimeOwner::current()const{
    impl_->validate();return impl_->frame;
}
void BnwNativePackageArchiveTimeOwner::require_same_resources(const BnwNativePackageDiscoveryOwner& packages)const{
    impl_->validate();need(impl_->packages.lock().get()==&packages,"Native stat source requires SAME early package");
}
BnwNativeStat64Receipt BnwNativePackageArchiveTimeOwner::original_loose_stat(const std::filesystem::path& relative)const{
    impl_->validate();
    const auto source=detail::confined_original_stat_path(impl_->lease->game_root(),relative);
    const auto result=source_stat(source);impl_->validate();return result;
}
void BnwNativePackageArchiveTimeOwner::require_category_borrow_current()const {
    const auto parent=impl_->packages.lock();need(bool(parent),"Category archive predecessor expired");
    parent->require_category_archive_borrow_current(*this);impl_->validate(this);
    parent->require_category_archive_borrow_current(*this);
}
BnwNativeStat64Receipt BnwNativePackageArchiveTimeOwner::category_original_loose_stat(
    const std::filesystem::path& relative)const {
    require_category_borrow_current();
    const auto result=source_stat(detail::confined_original_stat_path(impl_->lease->game_root(),relative));
    require_category_borrow_current();return result;
}
}
