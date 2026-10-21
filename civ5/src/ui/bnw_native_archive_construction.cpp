#include "bnw_native_archive_construction.hpp"
#include "civ5/ui/bnw_native_installation_input_owner.hpp"
#include "bnw_native_common_source_validation_context.hpp"
#include "bnw_native_completed_cold_source_loan.hpp"
#include "bnw_native_callback_owner_guard.hpp"
#include "bnw_native_archive_registry_source.hpp"
#include "bnw_native_package_category_source_owner.hpp"
#include "civ5/ui/detail/bnw_native_original_stat_path.hpp"
#include <clocale>
#include <tuple>
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
namespace civ5::ui::detail {
namespace {
void need(bool value,const char* message){if(!value)throw std::runtime_error(message);}
void source_locale(){const auto* locale=std::setlocale(LC_CTYPE,nullptr);need(locale&&std::string_view(locale)=="C","Finite ASCII native cache lower requires actual CRT C locale");}
bool same_entry(const assets::FpkEntry& a,const assets::FpkEntry& b){
    return std::tie(a.ordinal,a.original_name,a.normalized_name,a.directory_record_offset,a.metadata_offset,
        a.padding_length,a.file_time_raw,a.unknown_raw,a.payload_offset,a.payload_length,a.direct_metadata_was_valid)==
        std::tie(b.ordinal,b.original_name,b.normalized_name,b.directory_record_offset,b.metadata_offset,
        b.padding_length,b.file_time_raw,b.unknown_raw,b.payload_offset,b.payload_length,b.direct_metadata_was_valid);
}
struct PinnedArchive {
    HANDLE handle=INVALID_HANDLE_VALUE;
    BY_HANDLE_FILE_INFORMATION identity{};
    explicit PinnedArchive(const std::filesystem::path& path){
        handle=CreateFileW(path.c_str(),GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);
        need(handle!=INVALID_HANDLE_VALUE,"Archive source metadata cannot be pinned read-only");
        if(!GetFileInformationByHandle(handle,&identity)){CloseHandle(handle);handle=INVALID_HANDLE_VALUE;throw std::runtime_error("Archive source file identity unavailable");}
        if((identity.dwFileAttributes&(FILE_ATTRIBUTE_DIRECTORY|FILE_ATTRIBUTE_REPARSE_POINT))!=0){CloseHandle(handle);handle=INVALID_HANDLE_VALUE;throw std::runtime_error("Archive source object requires direct regular file");}
    }
    ~PinnedArchive(){if(handle!=INVALID_HANDLE_VALUE)CloseHandle(handle);}
    void validate()const{
        BY_HANDLE_FILE_INFORMATION now{};need(GetFileInformationByHandle(handle,&now),"Archive pinned source disappeared");
        need(std::tie(now.dwVolumeSerialNumber,now.nFileIndexHigh,now.nFileIndexLow,now.nFileSizeHigh,now.nFileSizeLow,
                      now.ftLastWriteTime.dwHighDateTime,now.ftLastWriteTime.dwLowDateTime)==
             std::tie(identity.dwVolumeSerialNumber,identity.nFileIndexHigh,identity.nFileIndexLow,identity.nFileSizeHigh,identity.nFileSizeLow,
                      identity.ftLastWriteTime.dwHighDateTime,identity.ftLastWriteTime.dwLowDateTime),"Archive pinned source identity changed");
    }
};
}
struct BnwNativeArchiveConstruction::Impl {
    BnwNativeCallbackOwnerState callback_state;
    std::weak_ptr<const BnwNativeArchiveConstruction> self;
    using Guard=BnwNativeCallbackOwnerGuard<BnwNativeArchiveConstruction>;
    Guard guard()const noexcept{return Guard(callback_state,self);}
    std::shared_ptr<const BnwNativeEarlyInputLease> lease;
    std::shared_ptr<const ::civ5::ui::BnwNativeInstallationInputOwner> installation_input;
    std::weak_ptr<const BnwNativeColdFilesystemOwner> installation_cold;
    std::shared_ptr<const BnwNativeColdArchiveRequest> installation_request;
    std::shared_ptr<const BnwNativeOriginalStatService> service;
    std::shared_ptr<const BnwNativeOriginalStatRead> read;
    std::filesystem::path original,canonical,relative;
    std::string basename;
    std::int64_t checked_mtime=0; //Source83C7D4, then checked83C957 assignment.
    //Actual83C7B0 ctor store [ESI+280]=1 belongs to this genuine
    //constructed archive. Only actual cached67B4E0 below mutates the byte.
    mutable std::uint8_t preference280=1;
    bool prepare_attempted=false;
    bool header_claimed=false,header_prefix_registered=false;
    const BnwNativeArchiveV6StreamHeaderOwner* prepared_header=nullptr;
    const BnwNativeGlobalArchivePrefixRecord* prepared_prefix=nullptr;
    std::weak_ptr<const BnwNativeGlobalArchivePrefixRecord> committed_prefix;
    std::unique_ptr<PinnedArchive> original_pin,canonical_pin;
    std::shared_ptr<const assets::FpkSource> original_metadata,canonical_payload;
    template<class Fence> void validate(const Fence& fence)const{
        const auto check=[&]{need(fence.valid(),"Archive callback owner lost/reentered/foreign thread");};
        check();if(installation_input){
            need(!lease&&installation_request,"Installed Archive source has conflicting/missing original domain");
            installation_input->require_current();check();service->require_same_input(*installation_input);check();
            const auto cold=installation_cold.lock();need(bool(cold),"Installed Archive lost actual Cold");
            cold->require_same_input(*installation_input);check();installation_request->require_same_owner(*cold);check();
            need(fence.valid_parent(cold),"Installed Archive persistent Cold lost");
            need(original==installation_request->original_absolute_io_candidate()&&
                 canonical==installation_request->canonical_payload_io_candidate()&&
                 relative==installation_request->original_relative_io_token().lexically_normal(),
                 "Installed Archive physical/canonical/relative request source changed");check();
        }else{
        check();lease->require_current();check();
        service->require_same_input(*lease);check();
        }
        read->require_same_service(*service);check();source_locale();
        const auto& fields=read->current();check();
        need(archive_checked_stat_mtime(fields)==checked_mtime,
             "Archive retained checked time differs from SAME constructor-stage read");
        if(original_pin)original_pin->validate();if(canonical_pin)canonical_pin->validate();
        if(installation_input){installation_input->require_current();check();}else{lease->require_current();check();}
    }
    template<class Fence>void validate_completed(const Fence& fence,const BnwNativeCompletedColdSourceLoan& loan)const {
        const auto check=[&]{need(fence.valid(),"Completed Archive callback owner lost/reentered/foreign thread");};
        check();need(lease&&!installation_input,"Mandatory installed completed Archive source before late use");
        check();loan.require_same_early_input(*lease);check();
        service->require_same_input_completed(*lease,loan);check();
        read->require_same_service_completed(*service,loan);check();source_locale();
        const auto& fields=read->current_completed(loan);check();
        need(archive_checked_stat_mtime(fields)==checked_mtime,
             "Completed Archive checked time differs from actual constructor-stage read");
        if(original_pin)original_pin->validate();if(canonical_pin)canonical_pin->validate();
        loan.require_same_early_input(*lease);check();source_locale();
    }
};
BnwNativeArchiveConstruction::BnwNativeArchiveConstruction(std::unique_ptr<Impl> i):impl_(std::move(i)){}
BnwNativeArchiveConstruction::~BnwNativeArchiveConstruction()=default;
std::shared_ptr<const BnwNativeArchiveConstruction> BnwNativeArchiveConstruction::begin(
    std::shared_ptr<const BnwNativeEarlyInputLease> lease,
    std::shared_ptr<const BnwNativeOriginalStatService> service,const std::filesystem::path& relative){
    need(lease&&service,"Archive construction requires genuine EARLY lease and actual stat service");
    auto i=std::make_unique<Impl>();
    {
        BnwNativeCallbackConstructionGuard fence(i->callback_state);
        const auto check=[&]{need(fence.valid(),"Archive construction reentered/foreign thread");};
        check();lease->require_current();check();
        service->require_same_input(*lease);check();source_locale();
        const auto original_root=lease->game_root();check();
        const auto payload_root=lease->canonical_payload_root();check();
        i->lease=std::move(lease);i->service=std::move(service);
        i->original=confined_original_stat_path(original_root,relative);
        i->canonical=confined_original_stat_path(payload_root,relative);
        i->relative=relative.lexically_normal();
        i->basename=archive_cache_basename_ascii(assets::path_utf8(i->original));
        // Keep actual read-before-both-opens; callback refusals publish no read.
        auto read=i->service->read_original_relative(relative);check();
        const auto& fields=read->current();check();
        const auto mtime=archive_checked_stat_mtime(fields);
        i->read=std::move(read);i->checked_mtime=mtime;i->validate(fence);
    }
    auto owner=std::shared_ptr<const BnwNativeArchiveConstruction>(
        new BnwNativeArchiveConstruction(std::move(i)));
    owner->impl_->self=owner;return owner;
}
std::shared_ptr<const BnwNativeArchiveConstruction> BnwNativeArchiveConstruction::begin(
    std::shared_ptr<const BnwNativeColdFilesystemOwner> cold,
    std::shared_ptr<const ::civ5::ui::BnwNativeInstallationInputOwner> installation,
    std::shared_ptr<const BnwNativeOriginalStatService> service,
    std::shared_ptr<const BnwNativeColdArchiveRequest> request){
    need(cold&&installation&&service&&request,"Installed Archive construction needs genuine Cold/input/Stat/request");
    auto i=std::make_unique<Impl>();
    {BnwNativeCallbackConstructionGuard fence(i->callback_state);
        const auto check=[&]{need(fence.valid()&&cold.use_count()>1,"Installed Archive construction lost real Cold/reentered");};
        check();cold->require_same_input(*installation);check();request->require_same_owner(*cold);check();
        service->require_same_input(*installation);check();installation->require_current();check();source_locale();
        i->installation_cold=cold;i->installation_input=std::move(installation);i->installation_request=std::move(request);
        i->service=std::move(service);
        i->relative=i->installation_request->original_relative_io_token().lexically_normal();check();
        i->original=i->installation_request->original_absolute_io_candidate();check();
        i->canonical=i->installation_request->canonical_payload_io_candidate();check();
        i->basename=archive_cache_basename_ascii(assets::path_utf8(i->original));
        // Actual original83C7D4 stat and83C957 checked assignment BEFORE both
        // original/canonical opens; request and SAME service remain retained.
        auto read=i->service->read_original_relative(i->relative);check();const auto& fields=read->current();check();
        i->checked_mtime=archive_checked_stat_mtime(fields);i->read=std::move(read);i->validate(fence);
    }
    auto owner=std::shared_ptr<const BnwNativeArchiveConstruction>(new BnwNativeArchiveConstruction(std::move(i)));
    owner->impl_->self=owner;return owner;
}
void BnwNativeArchiveConstruction::require_same_input(const ::civ5::ui::BnwNativeInstallationInputOwner& input,
    const BnwNativeOriginalStatService& service)const{
    auto fence=impl_->guard();impl_->validate(fence);
    need(!impl_->lease&&impl_->installation_input.get()==&input&&impl_->service.get()==&service,
         "Installed Archive metadata requires SAME genuine input/service");
}
bool BnwNativeArchiveConstruction::checked_preference280()const {
    auto& i=*impl_;auto fence=i.guard();i.validate(fence);return i.preference280!=0;
}
void BnwNativeArchiveConstruction::disable_preference280_from_cached_category(
    const BnwNativeArchiveV6StreamHeaderOwner& header,const BnwNativeCategoryNamespaceChannel& channel)const {
    //Caller is SAME selected cached publication, not a caller bool/Stage copy.
    channel.require_current();header.require_current();
    need(&header.construction()==this,"Native preference setter has another actual archive construction");
    auto& i=*impl_;auto fence=i.guard();i.validate(fence);
    need(i.header_claimed&&i.header_prefix_registered&&i.prepared_header==&header&&
         !i.committed_prefix.expired(),"Native cached preference setter lost actual completed header/prefix");
    i.preference280=0; //Exact67B4E0(current emitted Skin/Paks vector,0).
    i.validate(fence);
}
void BnwNativeArchiveConstruction::claim_header_before_entry_metadata()const{
    auto& i=*impl_;auto fence=i.guard();i.validate(fence);
    need(!i.prepare_attempted&&!i.header_claimed,"Archive source header phase already claimed/entry metadata attempted");
    need(fence.valid(),"Archive callback owner lost before header claim");
    i.header_claimed=true; // One-way BEFORE header open/read; no retry on failure.
}
void BnwNativeArchiveConstruction::require_shared_prefix_commit_available(
    const BnwNativeArchiveV6StreamHeaderOwner& header,
    const BnwNativeGlobalArchivePrefixRecord& record)const{
    auto& i=*impl_;auto fence=i.guard();i.validate(fence);
    need(i.header_claimed&&!i.prepare_attempted&&!i.header_prefix_registered&&
         !i.prepared_header&&!i.prepared_prefix,"Archive source shared-prefix phase unavailable/already prepared");
    // Full Record/Header current checks belong OUTSIDE this busy guard, in the
    // registry's genuine preflight operation. This private helper checks only
    // prepared native brands/phase/parents, never recurses through Header.current.
    record.require_prepared_identity_for(*this,header);
    need(fence.valid(),"Archive callback owner lost before prefix brands");
    i.prepared_header=&header;i.prepared_prefix=&record;
}
void BnwNativeArchiveConstruction::record_shared_prefix_before_entry_metadata(
    const BnwNativeArchiveV6StreamHeaderOwner& header,
    const std::shared_ptr<const BnwNativeGlobalArchivePrefixRecord>& record)const noexcept{
    static_assert(noexcept(std::declval<std::weak_ptr<const BnwNativeGlobalArchivePrefixRecord>&>()=
                           std::declval<const std::shared_ptr<const BnwNativeGlobalArchivePrefixRecord>&>()));
    auto& i=*impl_;
    // Actual registry keeps these exact typed strong brands across preflight,
    // native prefix store and completion, with NO external call in between.
    // Mismatch records no constructor completion: claimed prepare stays closed.
    // No callbacks/current/hash/allocations or fallible work after prefix store.
    if(!record||i.header_prefix_registered||i.prepared_header!=&header||
       i.prepared_prefix!=record.get())return;
    i.committed_prefix=record;i.header_prefix_registered=true;
}
void BnwNativeArchiveConstruction::prepare_original_and_canonical_metadata()const{
    auto& i=*impl_;auto fence=i.guard();i.validate(fence);need(!i.prepare_attempted,"Archive direct metadata preparation already attempted");
    i.prepare_attempted=true;
    std::shared_ptr<const BnwNativeGlobalArchivePrefixRecord> committed;
    if(i.header_claimed){
        committed=i.committed_prefix.lock();
        need(i.header_prefix_registered&&committed&&i.prepared_header&&
             i.prepared_prefix==committed.get(),"Archive claimed header lacks SAME committed shared prefix before entry reads");
        committed->require_published_identity_for(*this);
        need(fence.valid(),"Archive callback owner lost before source entry metadata");
    }
    //Failure is terminal for this staged object; callers cannot retry it after
    //a different open epoch and retain the preceding checked stat observation.
    auto original_pin=std::make_unique<PinnedArchive>(i.original);
    i.read->require_same_service(*i.service);
    need(fence.valid(),"Archive callback owner lost before original metadata open");
    const auto original=assets::FpkSource::open(i.original,assets::path_utf8(i.relative));
    need(fence.valid(),"Archive callback owner lost after original metadata open");
    const auto* os=std::get_if<std::shared_ptr<assets::FpkSource>>(&original);
    need(os,"Direct original archive parser unsupported; native open/loader failure owner required");
    //83CEAA requires first byte0;83CEBF takes the following transform byte.
    //Existing parser's nonzero transform interpretation is not admitted here.
    need((*os)->filename_scrambling()==0&&(*os)->extra_header_byte()==0,
         "Direct source v6 nonzero transform byte requires original filename codec");
    auto canonical_pin=std::make_unique<PinnedArchive>(i.canonical);
    const auto canonical=assets::FpkSource::open(i.canonical,assets::path_utf8(i.relative));
    need(fence.valid(),"Archive callback owner lost after canonical metadata open");
    const auto* cs=std::get_if<std::shared_ptr<assets::FpkSource>>(&canonical);
    need(cs&&(*os)->filename_scrambling()==(*cs)->filename_scrambling()&&
         (*os)->extra_header_byte()==(*cs)->extra_header_byte()&&(*os)->directory_end()==(*cs)->directory_end()&&
         (*os)->entries().size()==(*cs)->entries().size()&&
         std::equal((*os)->entries().begin(),(*os)->entries().end(),(*cs)->entries().begin(),same_entry),
         "Canonical archive directory metadata differs from actual original source");
    need(original_pin->identity.nFileSizeHigh==canonical_pin->identity.nFileSizeHigh&&
         original_pin->identity.nFileSizeLow==canonical_pin->identity.nFileSizeLow,
         "Canonical archive payload byte length differs from actual original source");
    original_pin->validate();canonical_pin->validate();
    // One final source FULL remains between local pin checks. Removing the
    // adjacent duplicate reduces observations; no source effect is moved.
    i.validate(fence);original_pin->validate();canonical_pin->validate();
    if(i.header_claimed){
        // Final source parent/phase/raw brands after ALL external validation,
        // immediately before the noexcept metadata stores. No Record.current.
        need(committed&&i.header_prefix_registered&&i.prepared_prefix==committed.get(),
             "Archive committed shared prefix changed before metadata stores");
        committed->require_published_identity_for(*this);
    }
    need(fence.valid(),"Archive callback owner lost before metadata stores");
    i.original_pin=std::move(original_pin);i.canonical_pin=std::move(canonical_pin);
    i.original_metadata=*os;i.canonical_payload=*cs;
    // No external callback after these noexcept ownership stores.
}
void BnwNativeArchiveConstruction::require_current()const{
    auto fence=impl_->guard();impl_->validate(fence);
}
void BnwNativeArchiveConstruction::require_same_input(const BnwNativeEarlyInputLease& lease,
    const BnwNativeOriginalStatService& service)const{
    auto fence=impl_->guard();impl_->validate(fence);
    need(impl_->lease.get()==&lease&&impl_->service.get()==&service,
         "Archive metadata requires SAME input/service owners");
}
const BnwNativeOriginalStatRead& BnwNativeArchiveConstruction::stat_before_open()const{
    auto fence=impl_->guard();impl_->validate(fence);return *impl_->read;
}
std::int64_t BnwNativeArchiveConstruction::checked_original_mtime()const{
    auto fence=impl_->guard();impl_->validate(fence);return impl_->checked_mtime;
}
const std::filesystem::path& BnwNativeArchiveConstruction::direct_original_path_candidate()const{
    auto fence=impl_->guard();impl_->validate(fence);return impl_->original;
}
const std::filesystem::path&
BnwNativeArchiveConstruction::direct_original_path_candidate_after_validation_without_callbacks()const noexcept {
    return impl_->original;
}
const std::string& BnwNativeArchiveConstruction::direct_archive_basename_candidate()const{
    auto fence=impl_->guard();impl_->validate(fence);return impl_->basename;
}
const std::shared_ptr<const assets::FpkSource>& BnwNativeArchiveConstruction::canonical_payload_source()const{
    auto fence=impl_->guard();impl_->validate(fence);
    need(bool(impl_->canonical_payload),"Archive direct metadata/payload preparation incomplete");
    return impl_->canonical_payload;
}
void BnwNativeArchiveConstruction::require_canonical_payload_identity_without_callbacks(
    const assets::FpkSource* expected)const {
    // Assigned once by successful direct metadata preparation. preference280
    // changes a different field; it cannot replace this retained Source.
    need(bool(impl_->canonical_payload),"Archive direct metadata/payload preparation incomplete");
    need(impl_->canonical_payload.get()==expected,"Default projection cached payload differs from actual constructor");
}
void BnwNativeArchiveConstruction::require_primary32_cached_fields_identity_without_callbacks(
    std::int64_t mtime,const assets::FpkSource* canonical)const {
    // Constructor-stage time and prepared canonical Source have no later
    // writers. Called only inside the immediate Publication identity check.
    need(impl_->checked_mtime==mtime,"Primary32 original cached time differs");
    need(bool(impl_->canonical_payload),"Archive direct metadata/payload preparation incomplete");
    need(impl_->canonical_payload.get()==canonical,"Primary32 canonical source identity differs");
}

std::pair<std::int64_t,std::shared_ptr<const assets::FpkSource>>
BnwNativeArchiveConstruction::cache_publication_metadata(
    const std::function<void()>& controller)const {
    auto fence=impl_->guard();
    const auto check=[&]{need(fence.valid(),"Archive publication metadata facade lost/reentered/foreign thread");controller();};
    check();impl_->validate(fence);check();
    // No provider/callback between these immutable constructor-stage fields.
    const auto mtime=impl_->checked_mtime;check();
    const auto& canonical=impl_->canonical_payload;
    need(bool(canonical),"Archive direct metadata/payload preparation incomplete");check();
    impl_->validate(fence);check();
    need(mtime==impl_->checked_mtime,"Archive publication checked time changed within source fence");
    // Copy payload storage after LAST callback; it is not facade authority.
    return {mtime,canonical};
}

void BnwNativeArchiveConstruction::require_current_completed(const BnwNativeCompletedColdSourceLoan& loan)const {
    auto fence=impl_->guard();impl_->validate_completed(fence,loan);
}
void BnwNativeArchiveConstruction::require_same_input_completed(const BnwNativeEarlyInputLease& input,
    const BnwNativeOriginalStatService& service,const BnwNativeCompletedColdSourceLoan& loan)const {
    auto fence=impl_->guard();impl_->validate_completed(fence,loan);
    need(impl_->lease.get()==&input&&impl_->service.get()==&service,
         "Completed Archive metadata requires same actual input/service");
}
const BnwNativeOriginalStatRead& BnwNativeArchiveConstruction::stat_before_open_completed(
    const BnwNativeCompletedColdSourceLoan& loan)const {
    auto fence=impl_->guard();impl_->validate_completed(fence,loan);return *impl_->read;
}
std::int64_t BnwNativeArchiveConstruction::checked_original_mtime_completed(const BnwNativeCompletedColdSourceLoan& loan)const {
    auto fence=impl_->guard();impl_->validate_completed(fence,loan);return impl_->checked_mtime;
}
bool BnwNativeArchiveConstruction::checked_preference280_completed(const BnwNativeCompletedColdSourceLoan& loan)const {
    auto fence=impl_->guard();impl_->validate_completed(fence,loan);return impl_->preference280!=0;
}
const std::filesystem::path& BnwNativeArchiveConstruction::direct_original_path_candidate_completed(
    const BnwNativeCompletedColdSourceLoan& loan)const {
    auto fence=impl_->guard();impl_->validate_completed(fence,loan);return impl_->original;
}
const std::string& BnwNativeArchiveConstruction::direct_archive_basename_candidate_completed(
    const BnwNativeCompletedColdSourceLoan& loan)const {
    auto fence=impl_->guard();impl_->validate_completed(fence,loan);return impl_->basename;
}
const std::shared_ptr<const assets::FpkSource>& BnwNativeArchiveConstruction::canonical_payload_source_completed(
    const BnwNativeCompletedColdSourceLoan& loan)const {
    auto fence=impl_->guard();impl_->validate_completed(fence,loan);
    need(bool(impl_->canonical_payload),"Completed Archive actual metadata/payload preparation absent");return impl_->canonical_payload;
}

void BnwNativeArchiveConstruction::require_common_source_current(BnwNativeCommonSourceValidationContext& context)const {
    auto fence=impl_->guard();
    const auto check=[&]{need(fence.valid(),"Common constructed source facade lost/reentered/foreign thread");context.check();};
    check();need(impl_->lease&&impl_->service&&impl_->read,"Common construction lost actual stat source");
    auto visit=context.enter(*this,impl_->self);
    impl_->lease->require_common_source_current(context);check();
    impl_->service->require_same_input_common_source(*impl_->lease,context);check();
    impl_->read->require_same_service_common_source(*impl_->service,context);check();source_locale();
    const auto& fields=impl_->read->current_common_source(context);check();
    need(archive_checked_stat_mtime(fields)==impl_->checked_mtime,
         "Common construction retained checked time differs from actual stat");
    if(visit.work()==BnwNativeCommonSourceValidationContext::Work::validate_source) {
        if(impl_->original_pin)impl_->original_pin->validate();
        if(impl_->canonical_pin)impl_->canonical_pin->validate();
    }
    impl_->lease->require_common_source_current(context);check();source_locale();visit.complete();
}
void BnwNativeArchiveConstruction::require_same_input_common_source(const BnwNativeEarlyInputLease& lease,
    const BnwNativeOriginalStatService& service,BnwNativeCommonSourceValidationContext& context)const {
    require_common_source_current(context);auto fence=impl_->guard();
    need(fence.valid()&&impl_->lease.get()==&lease&&impl_->service.get()==&service,
         "Common construction has foreign actual lease/service");context.require_borrowed(*this,impl_->self);
}
const std::filesystem::path& BnwNativeArchiveConstruction::direct_original_path_candidate_common_source(
    BnwNativeCommonSourceValidationContext& context)const {
    require_common_source_current(context);auto fence=impl_->guard();need(fence.valid(),"Common construction path borrower retired");
    context.require_borrowed(*this,impl_->self);return impl_->original;
}
const BnwNativeOriginalStatRead& BnwNativeArchiveConstruction::stat_before_open_common_source(
    BnwNativeCommonSourceValidationContext& context)const {
    require_common_source_current(context);auto fence=impl_->guard();need(fence.valid()&&impl_->read,"Common construction stat borrower retired");
    context.require_borrowed(*this,impl_->self);return *impl_->read;
}
std::int64_t BnwNativeArchiveConstruction::checked_original_mtime_common_source(BnwNativeCommonSourceValidationContext& context)const {
    require_common_source_current(context);auto fence=impl_->guard();need(fence.valid(),"Common construction time borrower retired");
    context.require_borrowed(*this,impl_->self);return impl_->checked_mtime;
}
const std::shared_ptr<const assets::FpkSource>& BnwNativeArchiveConstruction::canonical_payload_source_common_source(
    BnwNativeCommonSourceValidationContext& context)const {
    require_common_source_current(context);auto fence=impl_->guard();need(fence.valid(),"Common construction payload source borrower retired");
    context.require_borrowed(*this,impl_->self);return impl_->canonical_payload;
}

}
