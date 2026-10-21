#include "bnw_native_archive_registry_source.hpp"
#include "civ5/ui/bnw_native_installation_input_owner.hpp"
#include "bnw_native_common_source_validation_context.hpp"
#include "bnw_native_completed_cold_source_loan.hpp"
#include "bnw_native_callback_owner_guard.hpp"
#include <algorithm>
#include <clocale>
#include <cstring>
#include <tuple>
#include <utility>
#include <thread>
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
namespace civ5::ui::detail {
namespace {
void need(bool value,const char* message) {if(!value)throw std::runtime_error(message);}
std::uint64_t byte_length(const BY_HANDLE_FILE_INFORMATION& info) {
    return (static_cast<std::uint64_t>(info.nFileSizeHigh)<<32)|info.nFileSizeLow;
}
auto file_identity(const BY_HANDLE_FILE_INFORMATION& v) {
    return std::tuple(v.dwVolumeSerialNumber,v.nFileIndexHigh,v.nFileIndexLow,v.nFileSizeHigh,v.nFileSizeLow,
                      v.ftLastWriteTime.dwHighDateTime,v.ftLastWriteTime.dwLowDateTime,v.dwFileAttributes);
}
std::array<std::uint8_t,14> read_source_header(HANDLE handle) {
    LARGE_INTEGER begin{};need(SetFilePointerEx(handle,begin,nullptr,FILE_BEGIN),"Native header stream seek unavailable");
    std::array<std::uint8_t,14> bytes{};std::size_t offset=0;
    // Exact source83CE60 4/4/1/1, then83C9D0 first4. No entry directory read.
    for(const DWORD count:{4u,4u,1u,1u,4u}) {
        DWORD actual=0;need(ReadFile(handle,bytes.data()+offset,count,&actual,nullptr)&&actual==count,
                           "Native v6 source header short/failed read");offset+=count;
    }
    return bytes;
}
struct HeaderFile final {
    HANDLE pin=INVALID_HANDLE_VALUE,stream=INVALID_HANDLE_VALUE;
    BY_HANDLE_FILE_INFORMATION identity{};
    std::array<std::uint8_t,14> bytes{};
    ~HeaderFile() {if(stream!=INVALID_HANDLE_VALUE)CloseHandle(stream);if(pin!=INVALID_HANDLE_VALUE)CloseHandle(pin);}
    HeaderFile(const HeaderFile&)=delete;HeaderFile& operator=(const HeaderFile&)=delete;
    HeaderFile()=default;
    void open(const std::filesystem::path& candidate) {
        // Support pin freezes read identity. Source read stream uses native
        //829210 mode1 sharing/flags; absolute route does NOT replace raw GetName.
        pin=CreateFileW(candidate.c_str(),GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);
        need(pin!=INVALID_HANDLE_VALUE,"Header original/canonical source pin unavailable");
        need(GetFileInformationByHandle(pin,&identity) &&
             (identity.dwFileAttributes&(FILE_ATTRIBUTE_DIRECTORY|FILE_ATTRIBUTE_REPARSE_POINT))==0,
             "Header source requires pinned direct regular file");
        // Reject directory reparse routing outside the genuine confined source
        // candidate. Comparison below concerns physical IO, never raw prefixes.
        std::wstring final(32768,L'\0');
        const auto length=GetFinalPathNameByHandleW(pin,final.data(),static_cast<DWORD>(final.size()),FILE_NAME_NORMALIZED|VOLUME_NAME_DOS);
        need(length>0&&length<final.size(),"Header source final physical path unavailable");final.resize(length);
        if(final.starts_with(L"\\\\?\\"))final.erase(0,4);
        const auto expected=candidate.lexically_normal().native();
        need(_wcsicmp(final.c_str(),expected.c_str())==0,"Header physical source escaped retained candidate through alias");
        stream=CreateFileW(candidate.c_str(),GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE,nullptr,OPEN_EXISTING,0x08000080,nullptr);
        need(stream!=INVALID_HANDLE_VALUE,"Native mode1 source header stream open failed");
        BY_HANDLE_FILE_INFORMATION now{};need(GetFileInformationByHandle(stream,&now)&&file_identity(now)==file_identity(identity),
                                             "Header pin and actual read stream identities differ");
        bytes=read_source_header(stream); // Source stream remains at14 for future entry reader.
        validate();
    }
    void validate() const {
        BY_HANDLE_FILE_INFORMATION now{};
        need(pin!=INVALID_HANDLE_VALUE && stream!=INVALID_HANDLE_VALUE &&
             GetFileInformationByHandle(pin,&now)&&file_identity(now)==file_identity(identity),
             "Header pinned physical source changed");
        need(GetFileInformationByHandle(stream,&now)&&file_identity(now)==file_identity(identity),
             "Header source read stream identity changed");
        // Guard reads use separate pin cursor, preserving actual source stream
        // position14; no entry parse or source stream rewind follows current().
        need(read_source_header(pin)==bytes,"Header retained source bytes changed");
    }
};
std::u16string actual_acp_stream_name(const std::string& raw) {
    const int count=MultiByteToWideChar(CP_ACP,0,raw.c_str(),-1,nullptr,0);
    need(count>0,"Native CP_ACP stream name conversion failed");
    std::wstring wide(static_cast<std::size_t>(count),L'\0');
    need(MultiByteToWideChar(CP_ACP,0,raw.c_str(),-1,wide.data(),count)==count,"Native stream name conversion changed");
    std::u16string out;out.reserve(wide.size()-1);
    for(std::size_t n=0;n+1<wide.size();++n)out.push_back(wide[n]==L'/'?u'\\':static_cast<char16_t>(wide[n]));
    return out;
}
}
struct BnwNativeArchiveV6StreamHeaderOwner::Impl {
    const std::thread::id creator_thread=std::this_thread::get_id();
    BnwNativeCallbackOwnerState callback_state;
    std::weak_ptr<const BnwNativeArchiveV6StreamHeaderOwner> self;
    using Guard=BnwNativeCallbackOwnerGuard<BnwNativeArchiveV6StreamHeaderOwner>;
    Guard guard() const noexcept {return Guard(callback_state,self);}
    std::weak_ptr<const BnwNativeColdFilesystemOwner> cold;
    std::shared_ptr<const BnwNativeEarlyInputLease> lease;
    std::shared_ptr<const ::civ5::ui::BnwNativeInstallationInputOwner> installation_input;
    std::shared_ptr<const BnwNativeColdArchiveRequest> request;
    std::shared_ptr<const BnwNativeArchiveConstruction> construction;
    std::shared_ptr<const BnwNativeOriginalStatService> service;
    HeaderFile original,canonical;
    BnwArchiveV6HeaderFields fields;
    std::u16string raw_name;
    mutable bool prefix_attempted=false;
    template<class Fence> void validate(const Fence& fence) const {
        const auto check=[&]{need(fence.valid(),"V6 header facade lost/reentered/foreign thread");};
        check();need(request&&construction&&service,"V6 header requires genuine request/construction/stat owners");
        const auto owner=cold.lock();need(bool(owner),"V6 header cold source epoch expired");
        const auto parents=[&]{check();need(fence.valid_parent(owner),"V6 header cold parent lost");};
        if(installation_input){need(!lease,"Installed Header cannot carry imported save lease");
            owner->require_same_input(*installation_input);parents();request->require_same_owner(*owner);parents();
            service->require_same_input(*installation_input);parents();construction->require_same_input(*installation_input,*service);parents();
        }else{
        owner->require_same_input(*lease);parents();request->require_same_owner(*owner);parents();
        service->require_same_input(*lease);parents();construction->require_same_input(*lease,*service);parents();
        }
        const auto physical=construction->direct_original_path_candidate_after_validation_without_callbacks();parents();
        const auto expected=request->original_absolute_io_candidate();parents();
        const auto& stat=construction->stat_before_open();parents();
        const auto relative=stat.relative_source();parents();
        const auto requested=request->original_relative_io_token();parents();
        need(physical==expected && relative.lexically_normal()==requested.lexically_normal(),
             "V6 header construction stat/source path differs from SAME raw request");
        stat.require_same_service(*service);parents();
        const auto raw=request->raw_relative_ansi_request();parents();
        const auto candidate=request->raw_utf16_stream_name_candidate();parents();
        need(raw_name==actual_acp_stream_name(raw) && raw_name==candidate,"V6 actual raw stream name changed");
        original.validate();canonical.validate();
        need(original.bytes==canonical.bytes && byte_length(original.identity)==byte_length(canonical.identity) &&
             fields==bnw_v6_header_fields(original.bytes),"V6 original/canonical header identity differs");
        request->require_current();parents();construction->require_current();parents();
    }
    template<class Fence>void validate_completed(const Fence& fence,const BnwNativeCompletedColdSourceLoan& loan,
        const BnwNativeColdFilesystemOwner& actual_cold)const {
        const auto check=[&]{need(fence.valid(),"Completed V6 header facade lost/reentered/foreign thread");};
        check();loan.require_same_cold(actual_cold);check();
        need(request&&construction&&service&&lease,"Completed V6 header original source owners absent");
        actual_cold.require_same_completed_owner_control(cold,loan);check();loan.require_same_early_input(*lease);check();
        service->require_same_input_completed(*lease,loan);check();
        construction->require_same_input_completed(*lease,*service,loan);check();
        // THIS immutable Header retains the original Request throughout target
        // COPY/invoke/destruction. Parent hook proves membership before Impl.
        request->with_current_completed_descriptor(loan,[&](const auto& descriptor){
            check();const auto physical=construction->direct_original_path_candidate_completed(loan);check();
            const auto expected=descriptor.original_absolute_io_candidate();check();
            const auto& stat=construction->stat_before_open_completed(loan);check();
            const auto relative=stat.relative_source_completed(loan);check();
            const auto requested=descriptor.original_relative_io_token();check();
            need(physical==expected&&relative.lexically_normal()==requested.lexically_normal(),
                 "Completed V6 header source/stat path differs from same actual request");
            stat.require_same_service_completed(*service,loan);check();
            const auto raw=descriptor.raw_relative_ansi_request();check();
            const auto candidate=descriptor.raw_utf16_stream_name_candidate();check();
            need(raw_name==actual_acp_stream_name(raw)&&raw_name==candidate,"Completed V6 actual raw stream name changed");
            original.validate();canonical.validate();
            need(original.bytes==canonical.bytes&&byte_length(original.identity)==byte_length(canonical.identity)&&
                 fields==bnw_v6_header_fields(original.bytes),"Completed V6 original/canonical header identity differs");
            descriptor.require_current();check();
        });check();construction->require_current_completed(loan);check();
        actual_cold.require_same_completed_owner_control(cold,loan);loan.require_same_early_input(*lease);check();
    }
};
BnwNativeArchiveV6StreamHeaderOwner::BnwNativeArchiveV6StreamHeaderOwner(std::unique_ptr<Impl> impl):impl_(std::move(impl)) {}
BnwNativeArchiveV6StreamHeaderOwner::~BnwNativeArchiveV6StreamHeaderOwner()=default;
std::shared_ptr<const BnwNativeArchiveV6StreamHeaderOwner> BnwNativeArchiveV6StreamHeaderOwner::acquire(
    std::shared_ptr<const BnwNativeColdFilesystemOwner> cold,
    std::shared_ptr<const BnwNativeColdArchiveRequest> request,
    std::shared_ptr<const BnwNativeArchiveConstruction> construction,
    std::shared_ptr<const BnwNativeOriginalStatService> service) {
    need(cold&&request&&construction&&service,"V6 header missing genuine source inputs");
    cold->require_current();request->require_same_owner(*cold);
    const auto installation=cold->installation_input_for_source();
    if(installation){construction->require_same_input(*installation,*service);}else{
    construction->require_same_input(*cold->early_input_lease(),*service);
    }
    request->require_current();service->require_current();construction->require_current();
    need(construction->direct_original_path_candidate()==request->original_absolute_io_candidate(),
         "V6 header construction belongs to another original physical route");
    construction->stat_before_open().require_same_service(*service);
    cold->require_and_mark_archive_header_phase();
    // Root private one-way claim refuses already-attempted entry metadata and
    // consumes this stage before any file open/read. Failure is terminal.
    construction->claim_header_before_entry_metadata();
    auto impl=std::make_unique<Impl>();impl->cold=cold;impl->installation_input=installation;
    if(!installation){impl->lease=cold->early_input_lease();}
    impl->request=std::move(request);impl->construction=std::move(construction);impl->service=std::move(service);
    impl->raw_name=actual_acp_stream_name(impl->request->raw_relative_ansi_request());
    impl->original.open(impl->request->original_absolute_io_candidate());
    impl->fields=bnw_v6_header_fields(impl->original.bytes);
    impl->canonical.open(impl->request->canonical_payload_io_candidate());
    {
        BnwNativeCallbackConstructionGuard fence(impl->callback_state);impl->validate(fence);
    }
    auto out=std::shared_ptr<const BnwNativeArchiveV6StreamHeaderOwner>(new BnwNativeArchiveV6StreamHeaderOwner(std::move(impl)));
    out->impl_->self=out;return out;
}
void BnwNativeArchiveV6StreamHeaderOwner::require_current() const {auto fence=impl_->guard();impl_->validate(fence);}
void BnwNativeArchiveV6StreamHeaderOwner::require_default_projection_request_identity_without_callbacks(
    const BnwNativeColdFilesystemOwner& cold,std::size_t ordinal)const {
    // request/construction are assigned before Header publication and never
    // replaced. Root has just run this Header's FULL current + Parent check.
    need(bool(impl_->request),"V6 header requires genuine request/construction/stat owners");
    cold.require_request_ordinal_identity_without_callbacks(*impl_->request,ordinal);
}
void BnwNativeArchiveV6StreamHeaderOwner::require_constructed_payload_identity_without_callbacks(
    const assets::FpkSource* canonical)const {
    need(bool(impl_->construction),"V6 header requires genuine request/construction/stat owners");
    impl_->construction->require_canonical_payload_identity_without_callbacks(canonical);
}
void BnwNativeArchiveV6StreamHeaderOwner::require_primary32_source_identities_without_callbacks(
    const BnwNativeColdArchiveRequest& request,const BnwNativeArchiveConstruction& construction)const {
    // Both links are fixed before publication. No allocation/callback follows
    // the Journal's FULL Header validation and retained-step check.
    need(impl_->request.get()==&request,"Primary32 Header has another source request");
    need(impl_->construction.get()==&construction,"Primary32 Header has another constructor");
}
BnwNativeCachedPayloadReadResult BnwNativeArchiveV6StreamHeaderOwner::read_retained_original_cached_stream(
    std::uint32_t offset,std::span<std::uint8_t> output)const {
    auto fence=impl_->guard();impl_->validate(fence);
    need(output.size()<=UINT32_MAX&&std::uint64_t(offset)+output.size()<=byte_length(impl_->original.identity),
         "Native cached source read exceeds actual retained Original stream extent");
    BnwNativeCachedPayloadReadResult result;
    if(!output.empty()){
        LARGE_INTEGER position{};position.QuadPart=offset;
        if(!SetFilePointerEx(impl_->original.stream,position,nullptr,FILE_BEGIN))result.error=GetLastError();
        else {DWORD count=0;if(!ReadFile(impl_->original.stream,output.data(),DWORD(output.size()),&count,nullptr))result.error=GetLastError();
            result.count=count;}
    }
    impl_->validate(fence);return result;
}
const BnwNativeColdArchiveRequest& BnwNativeArchiveV6StreamHeaderOwner::source_request() const {auto fence=impl_->guard();impl_->validate(fence);return *impl_->request;}
const BnwNativeArchiveConstruction& BnwNativeArchiveV6StreamHeaderOwner::construction() const {auto fence=impl_->guard();impl_->validate(fence);return *impl_->construction;}
const BnwNativeOriginalStatService& BnwNativeArchiveV6StreamHeaderOwner::original_stat_service() const {auto fence=impl_->guard();impl_->validate(fence);return *impl_->service;}
const std::u16string& BnwNativeArchiveV6StreamHeaderOwner::raw_stream_name() const {auto fence=impl_->guard();impl_->validate(fence);return impl_->raw_name;}
const BnwArchiveV6HeaderFields& BnwNativeArchiveV6StreamHeaderOwner::header_fields() const {auto fence=impl_->guard();impl_->validate(fence);return impl_->fields;}
std::uint64_t BnwNativeArchiveV6StreamHeaderOwner::original_byte_length() const {auto fence=impl_->guard();impl_->validate(fence);return byte_length(impl_->original.identity);}
std::tuple<const BnwNativeColdArchiveRequest*,std::int64_t,std::shared_ptr<const assets::FpkSource>>
BnwNativeArchiveV6StreamHeaderOwner::cache_publication_metadata(
    const BnwNativeColdArchiveRequest& expected,std::uint32_t admitted,
    const std::function<void()>& controller)const {
    auto fence=impl_->guard();
    const auto check=[&]{need(fence.valid(),"V6 publication metadata facade lost/reentered/foreign thread");controller();};
    check();impl_->validate(fence);check();
    const auto* request=impl_->request.get();check();
    need(request==&expected,"Native entry came from another lookup request");
    // Preserve positive-loader refusal BEFORE obtaining publication metadata.
    need(admitted!=0,"Native loader result zero forbids archive cache insertion");
    const auto metadata=impl_->construction->cache_publication_metadata(controller);check();
    impl_->validate(fence);check();
    need(impl_->request.get()==request,"V6 publication original request changed within source fence");
    return {request,metadata.first,metadata.second};
}

void BnwNativeArchiveV6StreamHeaderOwner::claim_prefix_attempt() const {
    auto fence=impl_->guard();impl_->validate(fence);need(!impl_->prefix_attempted,"V6 header prefix stage already attempted");impl_->prefix_attempted=true;
}
void BnwNativeArchiveV6StreamHeaderOwner::require_native_entry_admission_for_cache() const {
    auto fence=impl_->guard();impl_->validate(fence);throw std::runtime_error("V6 declared header count is not native83B2A0 entry admission; per-namespace cache publication remains unavailable");
}
struct BnwNativeGlobalArchivePrefixOwner::Impl {
    BnwNativeCallbackOwnerState callback_state;
    std::weak_ptr<const BnwNativeGlobalArchivePrefixOwner> self;
    using Guard=BnwNativeCallbackOwnerGuard<BnwNativeGlobalArchivePrefixOwner>;
    Guard guard() const noexcept {return Guard(callback_state,self);}
    std::weak_ptr<const BnwNativeColdFilesystemOwner> cold;
    std::shared_ptr<const BnwNativeEarlyInputLease> lease;
    std::shared_ptr<const ::civ5::ui::BnwNativeInstallationInputOwner> installation_input;
    mutable BnwGlobalPrefixKernelDiagnostic prefixes;
    mutable const BnwNativeGlobalArchivePrefixRecord* operation_record=nullptr;
    mutable const Guard* operation_fence=nullptr;
    mutable bool active=false;
    // One internal facade pin normally, plus ONE Record-wrapper registry pin
    // during entry. No callers can supply this budget or an active fence.
    mutable long internal_registry_pins=1;
    void check(const Guard& fence) const {
        need(fence.valid() && bnw_prefix_persistent_facade_diagnostic(self.use_count(),internal_registry_pins),
             "Shared prefix facade lost/reentered/foreign thread");
    }
    std::shared_ptr<const BnwNativeColdFilesystemOwner> current_cold(const Guard& fence) const {
        check(fence);auto owner=cold.lock();need(bool(owner),"Shared prefix cold source epoch expired");
        if(installation_input){need(!lease,"Installed resource source cannot carry imported save lease");
            owner->require_same_input(*installation_input);check(fence);
        }else {owner->require_same_input(*lease);check(fence);}
        need(fence.valid_parent(owner),"Shared prefix cold parent lost");
        owner->require_fresh_direct_original_branch();check(fence);
        need(fence.valid_parent(owner),"Shared prefix cold parent lost");return owner;
    }
    void validate_record(const BnwNativeGlobalArchivePrefixRecord&,bool,const Guard&) const;
    void current_cold_completed(const Guard& fence,const BnwNativeCompletedColdSourceLoan& loan,
        const BnwNativeColdFilesystemOwner& actual_cold)const {
        check(fence);need(lease&&!installation_input,"Mandatory installed completed Prefix source before late use");
        actual_cold.require_same_completed_owner_control(cold,loan);check(fence);
        loan.require_same_early_input(*lease);check(fence);
    }
    void validate_record_completed(const BnwNativeGlobalArchivePrefixOwner&,const BnwNativeGlobalArchivePrefixRecord&,
        const Guard&,const BnwNativeCompletedColdSourceLoan&,const BnwNativeColdFilesystemOwner&)const;
};
struct BnwNativeGlobalArchivePrefixRecord::Impl {
    const std::thread::id creator_thread=std::this_thread::get_id();
    BnwNativeCallbackOwnerState callback_state;
    std::weak_ptr<const BnwNativeGlobalArchivePrefixRecord> self;
    using Guard=BnwNativeCallbackOwnerGuard<BnwNativeGlobalArchivePrefixRecord>;
    Guard guard() const noexcept {return Guard(callback_state,self);}
    std::weak_ptr<const BnwNativeGlobalArchivePrefixOwner> owner;
    const BnwNativeGlobalArchivePrefixOwner* owner_identity=nullptr;
    std::shared_ptr<const BnwNativeArchiveV6StreamHeaderOwner> header;
    std::u16string raw;
    std::uint32_t index=0;
    bool published=false;
    mutable BnwArchivePrefixPhaseDiagnostic phase;
    mutable const Guard* entry_fence=nullptr;
    std::shared_ptr<const BnwNativeGlobalArchivePrefixOwner> current_owner() const {
        auto locked=owner.lock();need(bool(locked),"Shared source prefix registry expired");
        need(locked.get()==owner_identity,"Shared prefix registry brand changed");return locked;
    }
};
void BnwNativeGlobalArchivePrefixOwner::Impl::validate_record(
    const BnwNativeGlobalArchivePrefixRecord& record,bool published,const Guard& fence) const {
    const auto parent=current_cold(fence);const auto& value=*record.impl_;
    need(value.owner_identity==self.lock().get() && value.owner.lock().get()==value.owner_identity,
         "Prefix record belongs to another shared registry");
    value.header->require_current();check(fence);need(fence.valid_parent(parent),"Prefix cold parent lost");
    const auto& request=*value.header->impl_->request;check(fence);
    request.require_same_owner(*parent);check(fence);
    const auto raw=value.header->raw_stream_name();check(fence);
    need(value.raw==raw,"Shared prefix is not SAME actual raw stream name");
    if(published) {
        need(value.published && value.index<prefixes.count() && prefixes.name(value.index)==value.raw,
             "Shared raw prefix has not been source published");
    } else {
        need(!value.published && active && operation_record==&record,
             "Shared prefix prepared record lacks active source commit");
        const auto existing=prefixes.lookup(value.raw);
        need(value.index==(existing?*existing:prefixes.count()),"Prepared shared prefix order/index changed");
    }
    check(fence);need(fence.valid_parent(parent),"Prefix cold parent lost");
}
BnwNativeGlobalArchivePrefixOwner::BnwNativeGlobalArchivePrefixOwner(std::unique_ptr<Impl> impl):impl_(std::move(impl)) {}
BnwNativeGlobalArchivePrefixOwner::~BnwNativeGlobalArchivePrefixOwner()=default;
std::shared_ptr<const BnwNativeGlobalArchivePrefixOwner> BnwNativeGlobalArchivePrefixOwner::acquire(
    std::shared_ptr<const BnwNativeColdFilesystemOwner> cold) {
    need(bool(cold),"Shared prefix requires genuine fresh cold epoch");cold->require_current();
    cold->claim_initial_prefix_registry();
    auto impl=std::make_unique<Impl>();impl->cold=cold;impl->installation_input=cold->installation_input_for_source();
    if(!impl->installation_input){impl->lease=cold->early_input_lease();}
    auto owner=std::shared_ptr<const BnwNativeGlobalArchivePrefixOwner>(new BnwNativeGlobalArchivePrefixOwner(std::move(impl)));
    owner->impl_->self=owner;owner->require_current();return owner;
}
void BnwNativeGlobalArchivePrefixOwner::require_current() const {
    auto fence=impl_->guard();impl_->current_cold(fence);
}
void BnwNativeGlobalArchivePrefixOwner::require_same_input(const BnwNativeColdFilesystemOwner& cold) const {
    auto fence=impl_->guard();const auto current=impl_->current_cold(fence);
    need(current.get()==&cold,"Shared prefix has another cold epoch");impl_->check(fence);
}
void BnwNativeGlobalArchivePrefixOwner::require_record(const BnwNativeGlobalArchivePrefixRecord& record,bool published) const {
    auto fence=impl_->guard();impl_->validate_record(record,published,fence);
}
std::shared_ptr<const BnwNativeGlobalArchivePrefixRecord> BnwNativeGlobalArchivePrefixOwner::register_header_prefix(
    std::shared_ptr<const BnwNativeArchiveV6StreamHeaderOwner> header) const {
    static_assert(noexcept(std::declval<const BnwNativeArchiveConstruction&>().record_shared_prefix_before_entry_metadata(
        std::declval<const BnwNativeArchiveV6StreamHeaderOwner&>(),
        std::declval<const std::shared_ptr<const BnwNativeGlobalArchivePrefixRecord>&>())));
    auto fence=impl_->guard();impl_->check(fence);
    need(header!=nullptr&&!impl_->active,"Shared prefix source registration absent/reentrant");
    need(header->impl_->installation_input.get()==impl_->installation_input.get()&&
         header->impl_->lease.get()==impl_->lease.get()&&!(impl_->lease&&impl_->installation_input)&&
         (bool(impl_->lease)||bool(impl_->installation_input)),
         "Shared Prefix constructor has another genuine import/installation source");
    struct Active {
        Impl& impl;
        Active(Impl& v,const Impl::Guard& f):impl(v){impl.active=true;impl.operation_fence=&f;}
        ~Active(){impl.active=false;impl.operation_record=nullptr;impl.operation_fence=nullptr;}
    } active(*impl_,fence);
    const auto cold=impl_->current_cold(fence);
    const auto& request=header->source_request();impl_->check(fence);
    request.require_same_owner(*cold);impl_->check(fence);
    header->claim_prefix_attempt();impl_->check(fence);
    auto staged=impl_->prefixes;const auto raw=header->raw_stream_name();impl_->check(fence);
    const auto index=staged.lookup_or_append_successful_allocation(raw);
    auto value=std::make_unique<BnwNativeGlobalArchivePrefixRecord::Impl>();
    value->owner=impl_->self;value->owner_identity=this;value->header=header;value->raw=raw;value->index=index;
    auto out=std::shared_ptr<const BnwNativeGlobalArchivePrefixRecord>(new BnwNativeGlobalArchivePrefixRecord(std::move(value)));
    out->impl_->self=out;impl_->operation_record=out.get();
    out->impl_->phase.seal_prepared(out->bound_identity_without_callbacks());
    impl_->validate_record(*out,false,fence);
    out->impl_->phase.mark_prepared_current(out->bound_identity_without_callbacks());
    // Strong typed owners and RAW reference captured before preflight. No
    // public Header accessor is evaluated after the shared prefix store.
    const auto construction=header->impl_->construction;
    const auto& bound=*construction;
    bound.require_shared_prefix_commit_available(*header,*out);
    impl_->check(fence);need(fence.valid_parent(cold),"Prefix cold parent lost before store");
    out->require_prepared_identity_for(bound,*header);
    // All throwing/callback/allocation work is above. Native prefix persists
    // even if later entry admission0 drops this archive.
    impl_->prefixes.swap(staged);out->impl_->published=true;
    out->impl_->phase.publish_after_source_store();
    bound.record_shared_prefix_before_entry_metadata(*header,out);
    return out;
}
std::uint32_t BnwNativeGlobalArchivePrefixOwner::register_skin_source_prefix(
    std::u16string raw,std::function<void()> source_current)const {
    return register_loose_source_prefix(std::move(raw),std::move(source_current));
}
std::u16string BnwNativeGlobalArchivePrefixOwner::read_registered_loose_prefix(std::uint32_t index)const {
    auto fence=impl_->guard();const auto cold=impl_->current_cold(fence);
    need(index<impl_->prefixes.count(),"Native loose record shared prefix index unregistered");
    const auto result=impl_->prefixes.name(index);
    impl_->check(fence);need(fence.valid_parent(cold),"Native loose prefix read lost original cold owner");
    return result;
}
std::uint32_t BnwNativeGlobalArchivePrefixOwner::register_loose_source_prefix(
    std::u16string raw,std::function<void()> source_current)const {
    //83A570/83A640/83A5E0/83A6B0 use THIS retained global prefix table.
    //Only SAME Package may call this with its actual Skin query/file guard.
    const auto keepalive=impl_->self.lock();
    need(keepalive&&source_current&&!impl_->active,"Skin prefix owner absent/reentered");
    auto fence=impl_->guard();
    struct Active {
        Impl& i;
        explicit Active(Impl& value):i(value){i.active=true;i.internal_registry_pins=2;}
        ~Active(){i.internal_registry_pins=1;i.active=false;}
    } active(*impl_);
    impl_->check(fence);const auto cold=impl_->current_cold(fence);
    source_current();impl_->check(fence);
    need(fence.valid_parent(cold),"Skin global prefix cold source lost");
    auto staged=impl_->prefixes;
    const auto index=staged.lookup_or_append_successful_allocation(raw);
    need(index<0x1000u,"Skin native packed prefix index domain exceeded");
    source_current();impl_->check(fence);
    need(fence.valid_parent(cold),"Skin global prefix source lost before native store");
    //Same exact UTF16 equality/append, never a local per-Skin counter.
    //Prefix registration persists even if following file/FPK admission fails.
    impl_->prefixes.swap(staged);
    source_current();impl_->check(fence);
    need(fence.valid_parent(cold),"Skin global prefix source lost after store");
    return index;
}
BnwNativeGlobalArchivePrefixRecord::BnwNativeGlobalArchivePrefixRecord(std::unique_ptr<Impl> impl):impl_(std::move(impl)) {}
BnwNativeGlobalArchivePrefixRecord::~BnwNativeGlobalArchivePrefixRecord()=default;
void BnwNativeGlobalArchivePrefixRecord::require_current() const {
    auto fence=impl_->guard();need(fence.valid(),"Prefix record facade lost/reentered/foreign thread");
    const auto owner=impl_->current_owner();owner->require_record(*this,true);
    need(fence.valid_parent(owner),"Prefix record facade/registry lost during currentness");
}
void BnwNativeGlobalArchivePrefixRecord::require_same_owner(const BnwNativeGlobalArchivePrefixOwner& expected) const {
    auto fence=impl_->guard();need(fence.valid(),"Prefix record facade lost/reentered/foreign thread");
    const auto owner=impl_->current_owner();need(owner.get()==&expected,"Prefix record has another registry");
    owner->require_record(*this,true);need(fence.valid_parent(owner),"Prefix record registry lost");
}
const BnwNativeArchiveV6StreamHeaderOwner&
BnwNativeGlobalArchivePrefixRecord::header_owner_after_validation_without_callbacks()const {
    return *impl_->header; // Assigned only before Prefix publication; retained by SAME record.
}
void BnwNativeGlobalArchivePrefixRecord::require_header_identity_without_callbacks(
    const BnwNativeArchiveV6StreamHeaderOwner& expected)const {
    // header is assigned before the actual prefix publication, never replaced.
    need(impl_->header.get()==&expected,"Default projection prefix has foreign Header");
}
std::uint32_t BnwNativeGlobalArchivePrefixRecord::source_index_after_validation_without_callbacks()const {
    // Assigned once before publication. Snapshot already retained this record
    // and performed its FULL SAME-registry validation + closed Root check.
    return impl_->index;
}
const BnwNativeArchiveV6StreamHeaderOwner& BnwNativeGlobalArchivePrefixRecord::header_owner() const {
    auto fence=impl_->guard();need(fence.valid(),"Prefix record facade lost/reentered/foreign thread");
    const auto owner=impl_->current_owner();owner->require_record(*this,true);
    need(fence.valid_parent(owner),"Prefix record registry lost before borrowed Header");return *impl_->header;
}
std::uint32_t BnwNativeGlobalArchivePrefixRecord::source_index() const {
    auto fence=impl_->guard();need(fence.valid(),"Prefix record facade lost/reentered/foreign thread");
    const auto owner=impl_->current_owner();owner->require_record(*this,true);
    need(fence.valid_parent(owner),"Prefix record registry lost before index");return impl_->index;
}
const std::u16string& BnwNativeGlobalArchivePrefixRecord::raw_prefix() const {
    auto fence=impl_->guard();need(fence.valid(),"Prefix record facade lost/reentered/foreign thread");
    const auto owner=impl_->current_owner();owner->require_record(*this,true);
    need(fence.valid_parent(owner),"Prefix record registry lost before borrowed raw name");return impl_->raw;
}
BnwArchivePrefixIdentityDiagnostic BnwNativeGlobalArchivePrefixRecord::bound_identity_without_callbacks() const {
    // No weak.lock transient pin hides last external registry/cold-owner loss.
    need(!impl_->owner.expired() && impl_->owner_identity,"Prefix raw identity registry expired");
    const auto& registry=*impl_->owner_identity->impl_;const auto& header=*impl_->header->impl_;
    const auto cold=header.cold.lock();
    need(cold && registry.cold.lock().get()==cold.get(),"Prefix raw identity has another/expired cold epoch");
    need(header.lease.get()==registry.lease.get()&&header.installation_input.get()==registry.installation_input.get()&&
         !(header.lease&&header.installation_input)&&(bool(header.lease)||bool(header.installation_input))&&
         header.construction&&header.service,"Prefix raw identity has another genuine import/installation source owner");
    return {impl_->owner_identity,this,impl_->header.get(),header.construction.get(),cold.get(),header.lease.get(),
            header.service.get(),header.installation_input.get()};
}
void BnwNativeGlobalArchivePrefixRecord::require_prepared_identity_for(
    const BnwNativeArchiveConstruction& construction,const BnwNativeArchiveV6StreamHeaderOwner& header) const {
    need(!impl_->owner.expired(),"Prepared prefix registry expired");
    const auto& registry=*impl_->owner_identity->impl_;
    need(registry.operation_fence && registry.active && registry.operation_record==this,
         "Prepared prefix source operation retired/another record");
    registry.check(*registry.operation_fence);
    need(registry.cold.use_count()>1,"Prepared prefix only transient cold parent remains");
    need(!impl_->published && impl_->header.get()==&header && header.impl_->construction.get()==&construction,
         "Prepared prefix has another actual header/construction");
    const auto identity=bound_identity_without_callbacks();impl_->phase.require_prepared_current(identity);
    const auto existing=registry.prefixes.lookup(impl_->raw);
    need(impl_->index==(existing?*existing:registry.prefixes.count()) && impl_->raw==header.impl_->raw_name,
         "Prepared prefix raw slot/order changed");
}
void BnwNativeGlobalArchivePrefixRecord::require_published_identity_for(const BnwNativeArchiveConstruction& construction) const {
    need(!impl_->owner.expired(),"Published prefix registry expired");
    const auto& registry=*impl_->owner_identity->impl_;
    need(registry.operation_fence && registry.active && registry.operation_record==this &&
         impl_->entry_fence && impl_->entry_fence->valid(),"Published prefix entry operation retired/reentered");
    registry.check(*registry.operation_fence);
    need(registry.cold.use_count()>1,"Published prefix only transient cold parent remains");
    need(impl_->published && impl_->header->impl_->construction.get()==&construction &&
         impl_->index<registry.prefixes.count() && registry.prefixes.name(impl_->index)==impl_->raw &&
         impl_->raw==impl_->header->impl_->raw_name,"Published prefix has different current slot/header/construction");
    impl_->phase.require_entry_current(bound_identity_without_callbacks());
}
void BnwNativeGlobalArchivePrefixRecord::prepare_bound_construction_metadata() const {
    auto fence=impl_->guard();need(fence.valid(),"Prefix entry facade lost/reentered/foreign thread");
    // Consume once BEFORE first callback; caught reentry still poisons fence.
    impl_->phase.start_entry(bound_identity_without_callbacks());
    struct Entry {Impl& i;explicit Entry(Impl& value,const Impl::Guard& f):i(value){i.entry_fence=&f;}
        ~Entry(){i.entry_fence=nullptr;i.phase.retire_entry();}} entry(*impl_,fence);
    const auto owner=impl_->current_owner();auto& registry=*owner->impl_;
    auto registry_fence=registry.guard();
    need(registry_fence.valid(),"Prefix entry registry reentered/foreign thread");
    struct RegistryEntry {
        BnwNativeGlobalArchivePrefixOwner::Impl& i;
        RegistryEntry(BnwNativeGlobalArchivePrefixOwner::Impl& value,
                      const BnwNativeGlobalArchivePrefixOwner::Impl::Guard& f,
                      const BnwNativeGlobalArchivePrefixRecord* record):i(value) {
            i.internal_registry_pins=2;i.active=true;i.operation_record=record;i.operation_fence=&f;
        }
        ~RegistryEntry(){i.internal_registry_pins=1;i.active=false;i.operation_record=nullptr;i.operation_fence=nullptr;}
    } registry_entry(registry,registry_fence,this);
    registry.check(registry_fence);
    const auto cold=registry.cold.lock();need(bool(cold),"Prefix entry cold parent expired");
    registry.validate_record(*this,true,registry_fence);
    need(fence.valid() && registry_fence.valid_parent(cold),"Prefix entry facade/cold parent lost before Constructor fence");
    registry.check(registry_fence);
    impl_->phase.mark_entry_current(bound_identity_without_callbacks());
    const auto header=impl_->header;const auto construction=header->impl_->construction;
    require_published_identity_for(*construction);
    construction->prepare_original_and_canonical_metadata();
    // Callback-free checks only; never Record.current/Header.current here.
    require_published_identity_for(*construction);
}

void BnwNativeArchiveV6StreamHeaderOwner::require_same_retained_request_identity_without_callbacks(
    const BnwNativeColdArchiveRequest& request,const std::weak_ptr<const BnwNativeColdArchiveRequest>& factory)const {
    need(impl_->creator_thread==std::this_thread::get_id(),"Header retained request identity foreign creator thread");
    // Do not acquire a second CallbackOwnerGuard: a real completed Header
    // validator can already own it while its pure Parent membership checks run.
    // Original impl.request is strong storage; no Request Impl is dereferenced.
    need(!impl_->self.expired()&&impl_->request&&impl_->request.get()==&request&&!factory.expired()&&
         !impl_->request.owner_before(factory)&&!factory.owner_before(impl_->request),
         "Header does not retain same actual factory request/control");
}
const BnwNativeArchiveV6StreamHeaderOwner& BnwNativeGlobalArchivePrefixRecord::retained_header_identity_without_callbacks()const {
    need(impl_->creator_thread==std::this_thread::get_id(),"Prefix retained Header identity foreign creator thread");
    need(!impl_->self.expired()&&!impl_->owner.expired()&&impl_->owner_identity&&impl_->published&&impl_->header,
         "Published Prefix actual retained Header identity unavailable");
    return *impl_->header; // Genuine original strong anchor, not a copied DTO.
}

// Every completed entry uses a genuine same-stack loan and retained actual
// parent references. Initial public methods and producers remain unchanged.
void BnwNativeArchiveV6StreamHeaderOwner::require_current_completed(const BnwNativeCompletedColdSourceLoan& loan,
    const BnwNativeColdFilesystemOwner& cold)const {auto fence=impl_->guard();impl_->validate_completed(fence,loan,cold);}
const BnwNativeColdArchiveRequest& BnwNativeArchiveV6StreamHeaderOwner::source_request_completed(const BnwNativeCompletedColdSourceLoan& loan,
    const BnwNativeColdFilesystemOwner& cold)const {auto fence=impl_->guard();impl_->validate_completed(fence,loan,cold);return *impl_->request;}
const BnwNativeArchiveConstruction& BnwNativeArchiveV6StreamHeaderOwner::construction_completed(const BnwNativeCompletedColdSourceLoan& loan,
    const BnwNativeColdFilesystemOwner& cold)const {auto fence=impl_->guard();impl_->validate_completed(fence,loan,cold);return *impl_->construction;}
const BnwNativeOriginalStatService& BnwNativeArchiveV6StreamHeaderOwner::original_stat_service_completed(const BnwNativeCompletedColdSourceLoan& loan,
    const BnwNativeColdFilesystemOwner& cold)const {auto fence=impl_->guard();impl_->validate_completed(fence,loan,cold);return *impl_->service;}
const std::u16string& BnwNativeArchiveV6StreamHeaderOwner::raw_stream_name_completed(const BnwNativeCompletedColdSourceLoan& loan,
    const BnwNativeColdFilesystemOwner& cold)const {auto fence=impl_->guard();impl_->validate_completed(fence,loan,cold);return impl_->raw_name;}
const BnwArchiveV6HeaderFields& BnwNativeArchiveV6StreamHeaderOwner::header_fields_completed(const BnwNativeCompletedColdSourceLoan& loan,
    const BnwNativeColdFilesystemOwner& cold)const {auto fence=impl_->guard();impl_->validate_completed(fence,loan,cold);return impl_->fields;}
std::uint64_t BnwNativeArchiveV6StreamHeaderOwner::original_byte_length_completed(const BnwNativeCompletedColdSourceLoan& loan,
    const BnwNativeColdFilesystemOwner& cold)const {auto fence=impl_->guard();impl_->validate_completed(fence,loan,cold);return byte_length(impl_->original.identity);}
BnwNativeCachedPayloadReadResult BnwNativeArchiveV6StreamHeaderOwner::read_retained_original_cached_stream_completed(
    std::uint32_t offset,std::span<std::uint8_t> output,const BnwNativeCompletedColdSourceLoan& loan,
    const BnwNativeColdFilesystemOwner& cold)const {
    need(impl_->creator_thread==std::this_thread::get_id(),"Completed cached read foreign creator thread");
    auto fence=impl_->guard();impl_->validate_completed(fence,loan,cold);
    need(output.size()<=UINT32_MAX&&std::uint64_t(offset)+output.size()<=byte_length(impl_->original.identity),
         "Completed native cached read exceeds actual retained Original stream extent");
    BnwNativeCachedPayloadReadResult result;
    if(!output.empty()) {
        LARGE_INTEGER position{};position.QuadPart=offset;
        // Full SAME Header/request/file/epoch checks do not seek the retained
        // source stream: HeaderFile guard reads use the separate pin cursor.
        impl_->validate_completed(fence,loan,cold);
        const BOOL positioned=SetFilePointerEx(impl_->original.stream,position,nullptr,FILE_BEGIN);
        const DWORD seek_error=positioned?0:GetLastError();
        // Capture the actual native error before any source validator changes
        // thread last-error. Failure and successful seek share this bookend.
        impl_->validate_completed(fence,loan,cold);
        if(!positioned)result.error=seek_error;
        else {
            DWORD count=0;
            impl_->validate_completed(fence,loan,cold);
            const BOOL read=ReadFile(impl_->original.stream,output.data(),DWORD(output.size()),&count,nullptr);
            const DWORD read_error=read?0:GetLastError();
            result.error=read_error;result.count=count;
            impl_->validate_completed(fence,loan,cold);
        }
    }
    impl_->validate_completed(fence,loan,cold);return result;
}
void BnwNativeGlobalArchivePrefixOwner::Impl::validate_record_completed(
    const BnwNativeGlobalArchivePrefixOwner& actual_registry,const BnwNativeGlobalArchivePrefixRecord& record,
    const Guard& fence,const BnwNativeCompletedColdSourceLoan& loan,const BnwNativeColdFilesystemOwner& actual_cold)const {
    current_cold_completed(fence,loan,actual_cold);
    need(record.impl_->creator_thread==std::this_thread::get_id(),"Completed Prefix record foreign creator thread");
    const auto& value=*record.impl_;
    need(value.owner_identity==&actual_registry&&!value.owner.expired()&&!value.owner.owner_before(self)&&
         !self.owner_before(value.owner)&&value.header,"Completed Prefix record has another registry/control/Header");
    value.header->require_current_completed(loan,actual_cold);check(fence);
    const auto raw=value.header->raw_stream_name_completed(loan,actual_cold);check(fence);
    need(value.raw==raw&&value.published&&value.index<prefixes.count()&&prefixes.name(value.index)==value.raw,
         "Completed Prefix actual published raw slot/Header changed");
    current_cold_completed(fence,loan,actual_cold);
}
void BnwNativeGlobalArchivePrefixOwner::require_current_completed(const BnwNativeCompletedColdSourceLoan& loan,
    const BnwNativeColdFilesystemOwner& cold)const {auto fence=impl_->guard();impl_->current_cold_completed(fence,loan,cold);}
void BnwNativeGlobalArchivePrefixOwner::require_same_input_completed(const BnwNativeColdFilesystemOwner& cold,
    const BnwNativeCompletedColdSourceLoan& loan)const {require_current_completed(loan,cold);}
void BnwNativeGlobalArchivePrefixOwner::require_record_completed(const BnwNativeGlobalArchivePrefixRecord& record,
    const BnwNativeCompletedColdSourceLoan& loan,const BnwNativeColdFilesystemOwner& cold)const {
    auto fence=impl_->guard();impl_->validate_record_completed(*this,record,fence,loan,cold);
}
std::u16string BnwNativeGlobalArchivePrefixOwner::read_registered_loose_prefix_completed(std::uint32_t index,
    const BnwNativeCompletedColdSourceLoan& loan,const BnwNativeColdFilesystemOwner& cold)const {
    auto fence=impl_->guard();impl_->current_cold_completed(fence,loan,cold);
    need(index<impl_->prefixes.count(),"Completed loose prefix index unregistered");
    const auto result=impl_->prefixes.name(index);impl_->current_cold_completed(fence,loan,cold);return result;
}
void BnwNativeGlobalArchivePrefixRecord::require_current_completed(const BnwNativeCompletedColdSourceLoan& loan,
    const BnwNativeColdFilesystemOwner& cold,const BnwNativeGlobalArchivePrefixOwner& registry)const {
    auto fence=impl_->guard();need(fence.valid(),"Completed Prefix record facade lost/reentered/foreign thread");
    need(impl_->owner_identity==&registry&&!impl_->owner.expired()&&!impl_->owner.owner_before(registry.impl_->self)&&
         !registry.impl_->self.owner_before(impl_->owner),"Completed Prefix expected registry/control differs");
    registry.require_record_completed(*this,loan,cold);
    need(fence.valid(),"Completed Prefix record lost after actual source checks");
}
const BnwNativeArchiveV6StreamHeaderOwner& BnwNativeGlobalArchivePrefixRecord::header_owner_completed(const BnwNativeCompletedColdSourceLoan& loan,
    const BnwNativeColdFilesystemOwner& cold,const BnwNativeGlobalArchivePrefixOwner& registry)const {
    auto fence=impl_->guard();need(fence.valid(),"Completed Prefix record facade lost/reentered/foreign thread");
    need(impl_->owner_identity==&registry&&!impl_->owner.expired()&&!impl_->owner.owner_before(registry.impl_->self)&&
         !registry.impl_->self.owner_before(impl_->owner),"Completed Prefix expected registry/control differs");
    registry.require_record_completed(*this,loan,cold);
    need(fence.valid(),"Completed Prefix record lost after actual source checks");
    return *impl_->header;
}
std::uint32_t BnwNativeGlobalArchivePrefixRecord::source_index_completed(const BnwNativeCompletedColdSourceLoan& loan,
    const BnwNativeColdFilesystemOwner& cold,const BnwNativeGlobalArchivePrefixOwner& registry)const {
    auto fence=impl_->guard();need(fence.valid(),"Completed Prefix record facade lost/reentered/foreign thread");
    need(impl_->owner_identity==&registry&&!impl_->owner.expired()&&!impl_->owner.owner_before(registry.impl_->self)&&
         !registry.impl_->self.owner_before(impl_->owner),"Completed Prefix expected registry/control differs");
    registry.require_record_completed(*this,loan,cold);
    need(fence.valid(),"Completed Prefix record lost after actual source checks");
    return impl_->index;
}
const std::u16string& BnwNativeGlobalArchivePrefixRecord::raw_prefix_completed(const BnwNativeCompletedColdSourceLoan& loan,
    const BnwNativeColdFilesystemOwner& cold,const BnwNativeGlobalArchivePrefixOwner& registry)const {
    auto fence=impl_->guard();need(fence.valid(),"Completed Prefix record facade lost/reentered/foreign thread");
    need(impl_->owner_identity==&registry&&!impl_->owner.expired()&&!impl_->owner.owner_before(registry.impl_->self)&&
         !registry.impl_->self.owner_before(impl_->owner),"Completed Prefix expected registry/control differs");
    registry.require_record_completed(*this,loan,cold);
    need(fence.valid(),"Completed Prefix record lost after actual source checks");
    return impl_->raw;
}

void BnwNativeArchiveV6StreamHeaderOwner::require_common_source_current(BnwNativeCommonSourceValidationContext& context)const {
    auto fence=impl_->guard();
    const auto check=[&]{need(fence.valid(),"Common header facade lost/reentered/foreign thread");context.check();};
    check();need(impl_->request&&impl_->construction&&impl_->service&&impl_->lease,"Common header lost actual source owners");
    const auto cold=impl_->cold.lock();need(bool(cold),"Common header actual Cold expired");
    const auto parents=[&]{check();need(fence.valid_parent(cold),"Common header actual Cold lost");};
    auto visit=context.enter(*this,impl_->self);
    cold->require_same_input_common_source(*impl_->lease,context);parents();
    impl_->request->require_same_owner_common_source(*cold,context);parents();
    impl_->service->require_same_input_common_source(*impl_->lease,context);parents();
    impl_->construction->require_same_input_common_source(*impl_->lease,*impl_->service,context);parents();
    const auto physical=impl_->construction->direct_original_path_candidate_common_source(context);parents();
    const auto expected=impl_->request->original_absolute_io_candidate_common_source(context);parents();
    const auto& stat=impl_->construction->stat_before_open_common_source(context);parents();
    const auto relative=stat.relative_source_common_source(context);parents();
    const auto requested=impl_->request->original_relative_io_token_common_source(context);parents();
    need(physical==expected&&relative.lexically_normal()==requested.lexically_normal(),"Common header actual construction/request stat path differs");
    stat.require_same_service_common_source(*impl_->service,context);parents();
    const auto raw=impl_->request->raw_relative_ansi_request_common_source(context);parents();
    const auto candidate=impl_->request->raw_utf16_stream_name_candidate_common_source(context);parents();
    need(impl_->raw_name==actual_acp_stream_name(raw)&&impl_->raw_name==candidate,"Common actual raw stream name changed");
    if(visit.work()==BnwNativeCommonSourceValidationContext::Work::validate_source) {
        impl_->original.validate();impl_->canonical.validate();
    }
    need(impl_->original.bytes==impl_->canonical.bytes&&byte_length(impl_->original.identity)==byte_length(impl_->canonical.identity)&&
         impl_->fields==bnw_v6_header_fields(impl_->original.bytes),"Common original/canonical header identity differs");
    impl_->request->require_common_source_current(context);parents();
    impl_->construction->require_common_source_current(context);parents();visit.complete();
}
const BnwNativeColdArchiveRequest& BnwNativeArchiveV6StreamHeaderOwner::source_request_common_source(
    BnwNativeCommonSourceValidationContext& context)const {
    require_common_source_current(context);auto fence=impl_->guard();need(fence.valid()&&impl_->request,"Common Header Request borrower retired");
    context.require_borrowed(*this,impl_->self);return *impl_->request;
}
const std::u16string& BnwNativeArchiveV6StreamHeaderOwner::raw_stream_name_common_source(
    BnwNativeCommonSourceValidationContext& context)const {
    require_common_source_current(context);auto fence=impl_->guard();need(fence.valid(),"Common Header raw name borrower retired");
    context.require_borrowed(*this,impl_->self);return impl_->raw_name;
}
void BnwNativeGlobalArchivePrefixOwner::require_common_source_current(BnwNativeCommonSourceValidationContext& context)const {
    auto fence=impl_->guard();const auto check=[&]{impl_->check(fence);context.check();};
    check();const auto cold=impl_->cold.lock();need(bool(cold),"Common prefix registry actual Cold expired");
    auto visit=context.enter(*this,impl_->self);
    cold->require_same_input_common_source(*impl_->lease,context);check();need(fence.valid_parent(cold),"Common prefix registry actual Cold lost");
    cold->require_fresh_direct_original_branch_common_source(context);check();need(fence.valid_parent(cold),"Common prefix registry actual Cold lost");
    visit.complete();
}
void BnwNativeGlobalArchivePrefixOwner::require_same_input_common_source(const BnwNativeColdFilesystemOwner& expected,
    BnwNativeCommonSourceValidationContext& context)const {
    require_common_source_current(context);auto fence=impl_->guard();impl_->check(fence);context.require_borrowed(*this,impl_->self);
    need(!impl_->cold.expired()&&impl_->cold.lock().get()==&expected,"Common prefix registry has foreign actual Cold");
}
void BnwNativeGlobalArchivePrefixOwner::require_record_common_source(const BnwNativeGlobalArchivePrefixRecord& record,
    bool published,BnwNativeCommonSourceValidationContext& context)const {
    require_common_source_current(context);auto fence=impl_->guard();
    const auto check=[&]{impl_->check(fence);context.check();};check();context.require_borrowed(*this,impl_->self);
    const auto cold=impl_->cold.lock();need(bool(cold),"Common prefix actual Cold expired");
    const auto& value=*record.impl_;
    need(value.owner_identity==this&&!value.owner.owner_before(impl_->self)&&!impl_->self.owner_before(value.owner)&&
         !value.owner.expired(),"Common prefix record has foreign registry control");
    value.header->require_common_source_current(context);check();need(fence.valid_parent(cold),"Common prefix actual Cold lost");
    const auto& request=value.header->source_request_common_source(context);check();
    request.require_same_owner_common_source(*cold,context);check();
    const auto raw=value.header->raw_stream_name_common_source(context);check();
    need(value.raw==raw,"Common prefix differs from SAME actual stream name");
    if(published) {
        need(value.published&&value.index<impl_->prefixes.count()&&impl_->prefixes.name(value.index)==value.raw,
             "Common shared raw prefix is not source published");
    }else{
        need(!value.published&&impl_->active&&impl_->operation_record==&record,"Common prepared prefix lacks active actual commit");
        const auto existing=impl_->prefixes.lookup(value.raw);
        need(value.index==(existing?*existing:impl_->prefixes.count()),"Common prepared prefix order/index changed");
    }
    check();need(fence.valid_parent(cold),"Common prefix actual Cold lost");
}
void BnwNativeGlobalArchivePrefixRecord::require_common_source_current(BnwNativeCommonSourceValidationContext& context)const {
    auto fence=impl_->guard();need(fence.valid(),"Common prefix record facade lost/reentered/foreign thread");context.check();
    const auto owner=impl_->current_owner();
    need(!impl_->owner.owner_before(owner)&&!owner.owner_before(impl_->owner),"Common prefix actual registry control changed");
    auto visit=context.enter(*this,impl_->self);owner->require_record_common_source(*this,true,context);
    need(fence.valid_parent(owner),"Common prefix facade/registry lost during source validation");context.check();visit.complete();
}
void BnwNativeGlobalArchivePrefixRecord::require_same_owner_common_source(const BnwNativeGlobalArchivePrefixOwner& expected,
    BnwNativeCommonSourceValidationContext& context)const {
    require_common_source_current(context);auto fence=impl_->guard();need(fence.valid(),"Common prefix owner borrower retired");
    context.require_borrowed(*this,impl_->self);need(!impl_->owner.expired()&&impl_->owner_identity==&expected,"Common prefix has foreign actual registry");
}
const BnwNativeArchiveV6StreamHeaderOwner& BnwNativeGlobalArchivePrefixRecord::header_owner_common_source(
    BnwNativeCommonSourceValidationContext& context)const {
    require_common_source_current(context);auto fence=impl_->guard();need(fence.valid()&&impl_->header,"Common prefix Header borrower retired");
    context.require_borrowed(*this,impl_->self);return *impl_->header;
}
std::uint32_t BnwNativeGlobalArchivePrefixRecord::source_index_common_source(BnwNativeCommonSourceValidationContext& context)const {
    require_common_source_current(context);auto fence=impl_->guard();need(fence.valid(),"Common prefix index borrower retired");
    context.require_borrowed(*this,impl_->self);return impl_->index;
}
const std::u16string& BnwNativeGlobalArchivePrefixRecord::raw_prefix_common_source(BnwNativeCommonSourceValidationContext& context)const {
    require_common_source_current(context);auto fence=impl_->guard();need(fence.valid(),"Common prefix raw borrower retired");
    context.require_borrowed(*this,impl_->self);return impl_->raw;
}

}
