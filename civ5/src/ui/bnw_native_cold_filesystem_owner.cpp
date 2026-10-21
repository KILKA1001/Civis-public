#include "bnw_native_cold_filesystem_owner.hpp"
#include "civ5/ui/bnw_native_installation_input_owner.hpp"
#include "bnw_native_common_source_validation_context.hpp"
#include "bnw_native_common13_source.hpp"
#include "bnw_native_installed_late_default_operation.hpp"
#include <algorithm>
#include "bnw_native_package_category_source_owner.hpp"
#include "bnw_native_global_resource_warm_owner.hpp"
#include <set>
#include <limits>
#include <thread>
#include "civ5/ui/bnw_native_pinned_input_epoch.hpp"
#include "civ5/ui/detail/bnw_native_original_stat_path.hpp"
#include <cstdint>
#include <utility>
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

namespace civ5::ui::detail {
namespace {
void need(bool value,const char* message) {
    if(!value)throw std::runtime_error(message);
}
// No public scalar constructor/epoch input. These are fresh owned source fields,
// distinct from an assertion about memory/CWD of a running original process.
struct FreshColdEpoch final {
    const DWORD process=GetCurrentProcessId(),thread=GetCurrentThreadId(),acp=GetACP();
    const void* const mapped_stream_callback=nullptr;
};
std::filesystem::path primary_relative_io_path(const std::u16string& token) {
    std::wstring wide;wide.reserve(token.size());
    for(const auto value:token)wide.push_back(static_cast<wchar_t>(value));
    return std::filesystem::path(wide);
}
}
struct BnwNativeColdFilesystemOwner::Impl {
    std::shared_ptr<const BnwNativeEarlyInputLease> lease;
    std::shared_ptr<const ::civ5::ui::BnwNativeInstallationInputOwner> installation_input;
    std::weak_ptr<const BnwNativeColdFilesystemOwner> self;
    const std::unique_ptr<const FreshColdEpoch> epoch=std::make_unique<const FreshColdEpoch>();
    std::filesystem::path original_source_base,canonical_payload_base;
    mutable std::size_t completed_source_pins=0;
    mutable std::uint64_t completed_source_operation=0;
    mutable bool completed_source_entering=false,completed_source_failed=false;
    mutable const BnwNativeCompletedColdSourceLoan* active_completed_source=nullptr;
    mutable bool prefix_registry_claimed=false;
    mutable bool archive_header_started=false;
    mutable bool archive_request_started=false;
    mutable bool entry_admission_arena_claimed=false;
    // Strong opaque brands prevent replay/address reuse without a Cold/channel cycle.
    mutable std::set<std::shared_ptr<const void>,std::owner_less<std::shared_ptr<const void>>> category_request_brands;
    mutable std::set<std::shared_ptr<const void>,std::owner_less<std::shared_ptr<const void>>> common_request_brands;
    mutable std::set<std::shared_ptr<const void>,std::owner_less<std::shared_ptr<const void>>> late_default_request_brands;
    void validate() const {
        if(installation_input){
            need(!lease,"Cold installation source cannot carry an imported save lease");installation_input->require_current();
            need(GetCurrentProcessId()==epoch->process&&GetCurrentThreadId()==epoch->thread&&GetACP()==epoch->acp&&epoch->acp==1251&&
                 epoch->mapped_stream_callback==nullptr,"Installed Cold creator/ACP1251/null mapped callback source differs");
            installation_input->require_cold_roots_identity_without_callbacks(
                original_source_base,canonical_payload_base);
            installation_input->require_current();return;
        }
        need(lease!=nullptr,"Cold filesystem lacks genuine EARLY source lease");
        lease->require_current();
        need(GetCurrentProcessId()==epoch->process && GetCurrentThreadId()==epoch->thread &&
             GetACP()==epoch->acp && epoch->acp==1251,
             "Cold filesystem requires same process/thread/source ACP1251 epoch");
        need(epoch->mapped_stream_callback==nullptr,
             "Cold filesystem lacks owned direct-original callback branch");
        lease->require_cold_roots_profile_identity_without_callbacks(original_source_base,canonical_payload_base);
    }
};
struct BnwNativeColdArchiveRequest::Impl {
    std::weak_ptr<const BnwNativeColdFilesystemOwner> owner;
    const FreshColdEpoch* epoch=nullptr;
    std::size_t ordinal=0;
    std::weak_ptr<const BnwNativeCategoryArchiveChannel> category;
    bool is_category=false;
    std::weak_ptr<const BnwNativeCommon13ArchiveChannel> common;
    bool is_common=false;
    std::weak_ptr<const BnwNativeInstalledLateDefaultArchiveAttempt> late_default;
    bool is_late_default=false;
    std::string raw_ansi;
    std::u16string raw_utf16;
    std::filesystem::path relative,original_candidate,payload_candidate;
    std::shared_ptr<const BnwNativeColdFilesystemOwner> current_owner() const {
        auto locked=owner.lock();
        need(locked!=nullptr,"Cold archive request issuing epoch expired");
        locked->require_current();
        return locked;
    }
};
#include "bnw_native_installed_late_default_cold_request.inc"
BnwNativeCompletedColdSourceLoan::BnwNativeCompletedColdSourceLoan(
    const std::shared_ptr<const BnwNativeColdFilesystemOwner>& cold,
    const BnwNativeCompletedParentObservation::Loan& observation,
    const ::civ5::ui::BnwNativePinnedInputEpoch& epoch,const Source& source,std::uint64_t operation)
    :cold_(cold),observation_(observation),epoch_(epoch),source_(source),operation_(operation){}
void BnwNativeCompletedColdSourceLoan::require_current()const {
    need(thread_==std::this_thread::get_id(),"Completed Cold loan foreign creator thread");
    need(cold_&&GetCurrentProcessId()==cold_->impl_->epoch->process&&
         GetCurrentThreadId()==cold_->impl_->epoch->thread,"Completed Cold loan foreign process/thread");
    auto& i=*cold_->impl_;
    need(active_&&!i.completed_source_failed&&i.active_completed_source==this&&
         i.completed_source_operation==operation_&&i.completed_source_pins==1&&
         !cold_.owner_before(i.self)&&!i.self.owner_before(cold_)&&
         cold_.use_count()>static_cast<long>(i.completed_source_pins),
         "Completed Cold loan lost canonical operation/outside owner");
    if(checking_){i.completed_source_failed=true;throw std::runtime_error("Completed Cold source check reentry");}
    checking_=true;struct Exit {const BnwNativeCompletedColdSourceLoan& value;~Exit(){value.checking_=false;}} exit{*this};
    const auto check=[&]{
        need(active_&&!i.completed_source_failed&&i.active_completed_source==this&&
             i.completed_source_operation==operation_&&i.completed_source_pins==1&&
             cold_.use_count()>static_cast<long>(i.completed_source_pins),
             "Completed Cold actual source retired during check");
    };
    try {
        observation_.require_current();check();
        source_.require_actual_completed_cold_current(*cold_,observation_);check();
        epoch_.require_same_early_input(*i.lease);check();epoch_.require_current();check();
        need(GetACP()==i.epoch->acp&&i.epoch->acp==1251&&i.epoch->mapped_stream_callback==nullptr,
             "Completed Cold lost same ACP1251/null callback source");
        need(i.original_source_base==epoch_.game_root()&&i.canonical_payload_base==epoch_.canonical_payload_root()&&
             i.original_source_base.is_absolute()&&i.canonical_payload_base.is_absolute(),
             "Completed Cold original/canonical roots differ from genuine pinned epoch");check();
        const auto& profile=epoch_.profile();
        need(profile.can_activate&&profile.enabled_mods.empty()&&!profile.native_mods_require_port,
             "Completed Cold lost same pinned stock profile");check();
        epoch_.require_current();check();
        source_.require_actual_completed_cold_current(*cold_,observation_);check();
        observation_.require_current();check();
    }catch(...){i.completed_source_failed=true;throw;}
}
void BnwNativeCompletedColdSourceLoan::require_same_cold(const BnwNativeColdFilesystemOwner& actual)const {
    require_current();need(cold_.get()==&actual,"Completed Cold loan belongs to another actual Cold");require_current();
}
void BnwNativeCompletedColdSourceLoan::require_same_early_input(const ::civ5::ui::BnwNativeEarlyInputLease& actual)const {
    require_current();need(cold_->impl_->lease.get()==&actual,"Completed Cold loan has another original Early facade");
    epoch_.require_same_early_input(actual);require_current();
}
std::size_t BnwNativeCompletedColdSourceLoan::internal_cold_pin_count()const {
    require_current();return cold_->impl_->completed_source_pins;
}
void BnwNativeColdFilesystemOwner::require_same_completed_owner_control(
    const std::weak_ptr<const BnwNativeColdFilesystemOwner>& supplied,const BnwNativeCompletedColdSourceLoan& loan)const {
    loan.require_same_cold(*this);
    need(!supplied.expired()&&!supplied.owner_before(impl_->self)&&!impl_->self.owner_before(supplied),
         "Completed source has another original Cold factory control");
    loan.require_same_cold(*this);
}
void BnwNativeColdFilesystemOwner::with_same_completed_source(
    const BnwNativeCompletedParentObservation::Loan& observation,const BnwNativeCompletedColdSourceLoan::Source& source,
    const std::function<void(const BnwNativeCompletedColdSourceLoan&)>& input)const {
    need(GetCurrentProcessId()==impl_->epoch->process&&GetCurrentThreadId()==impl_->epoch->thread,
         "Completed Cold lender foreign process/thread");
    need(impl_->lease&&!impl_->installation_input,"Mandatory installed completed Cold source/loan variant before late use");
    auto& i=*impl_;
    if(i.completed_source_entering||i.active_completed_source){i.completed_source_failed=true;
        throw std::runtime_error("Completed Cold lender reentry");}
    need(!i.completed_source_failed&&!i.completed_source_pins&&
         i.completed_source_operation!=std::numeric_limits<std::uint64_t>::max(),
         "Completed Cold lender failed/pin or operation exhausted");
    const auto cold=i.self.lock();
    need(cold&&cold.get()==this&&!cold.owner_before(i.self)&&!i.self.owner_before(cold)&&cold.use_count()>1,
         "Completed Cold lender lost real canonical outside facade");
    ++i.completed_source_pins;i.completed_source_entering=true;
    struct Exit {Impl& i;~Exit(){
        if(i.active_completed_source)i.completed_source_failed=true;
        i.active_completed_source=nullptr;i.completed_source_entering=false;--i.completed_source_pins;
    }} exit{i};
    try {
        // The epoch reference is supplied solely by the genuine active Parent
        // observation; no incoming shared owner can substitute its control.
        observation.with_actual_pinned_input_epoch([&](const auto& epoch){
            BnwNativeCompletedColdSourceLoan loan(cold,observation,epoch,source,++i.completed_source_operation);
            i.active_completed_source=&loan;
            struct LoanExit {Impl& i;const BnwNativeCompletedColdSourceLoan* loan;~LoanExit(){
                if(i.active_completed_source!=loan)i.completed_source_failed=true;
                i.active_completed_source=nullptr;
            }} loan_exit{i,&loan};
            loan.require_current();{
                const auto consume=input;loan.require_current();need(bool(consume),"Completed Cold consumer missing");
                consume(loan);loan.require_current();
            }loan.require_current(); // Target destructor is inside actual epoch loan.
        });
        need(!i.completed_source_failed&&!i.active_completed_source&&cold.use_count()>1,
             "Completed Cold lender lost source after actual epoch callback");
    }catch(...){i.completed_source_failed=true;throw;}
}
void BnwNativeColdFilesystemOwner::require_current_completed_request(
    const BnwNativeColdArchiveRequest& request,const BnwNativeCompletedColdSourceLoan& loan)const {
    loan.require_same_cold(*this);
    // BEFORE dereferencing the supplied request, the real Parent Source must
    // prove immutable strong SAME journal/Header storage for its whole lender
    // through target COPY/invoke/destruction. A weak ledger alone cannot do so.
    loan.source_.require_actual_completed_request_current(request,*this,loan.observation_);loan.require_current();
    const auto& value=*request.impl_;
    need(!value.owner.expired()&&!value.owner.owner_before(loan.cold_)&&!loan.cold_.owner_before(value.owner)&&
         value.epoch==impl_->epoch.get(),"Completed Cold request changed actual source epoch/control");
    need(!value.is_late_default,"Completed Cold source cannot borrow an installed late Default request");
    if(value.is_common){
        const auto common=value.common.lock();need(bool(common)&&!value.is_category&&common->brand_&&
             impl_->common_request_brands.contains(common->brand_)&&common->cold_.get()==this&&
             common->lease_.get()==impl_->lease.get()&&value.ordinal==32+common->ordinal_&&
             value.raw_ansi==bnw_common13_raw_request(common->ordinal_),
             "Completed Common request changed actual channel/brand/ordinal/raw name");
    }else if(value.is_category){
        need(!value.category.expired(),"Completed Category request actual sealed channel retired");
        // Exact actual category ordinal/brand/manager membership belongs to
        // the private Parent Source hook above, not to guessed request fields.
    }else{
        need(value.ordinal<bnw_primary_archive_descriptors.size()&&
             value.raw_ansi==bnw_primary_archive_raw_ansi_literal(value.ordinal),
             "Completed primary request changed original descriptor ordinal/raw name");
    }
    need(value.raw_utf16==bnw_primary_archive_ascii_stream_token(value.raw_ansi)&&
         value.relative==primary_relative_io_path(value.raw_utf16)&&
         value.original_candidate==confined_original_stat_path(impl_->original_source_base,value.relative)&&
         value.payload_candidate==confined_original_stat_path(impl_->canonical_payload_base,value.relative),
         "Completed Cold request raw/UTF16/confined original/canonical lineage changed");
    loan.require_current();loan.source_.require_actual_completed_request_current(request,*this,loan.observation_);
    loan.require_current();
}
void BnwNativeColdArchiveRequest::with_current_completed_descriptor(const BnwNativeCompletedColdSourceLoan& loan,
    const std::function<void(const CompletedDescriptor&)>& input)const {
    loan.require_current();loan.cold_->require_current_completed_request(*this,loan);
    const CompletedDescriptor descriptor(*this,loan);
    {const auto consume=input;descriptor.require_current();need(bool(consume),"Completed request consumer missing");
        consume(descriptor);descriptor.require_current();
    }descriptor.require_current();
}
void BnwNativeColdArchiveRequest::CompletedDescriptor::require_current()const {
    loan_.require_current();loan_.cold_->require_current_completed_request(request_,loan_);
}
std::size_t BnwNativeColdArchiveRequest::CompletedDescriptor::source_ordinal()const {require_current();return request_.impl_->ordinal;}
const std::string& BnwNativeColdArchiveRequest::CompletedDescriptor::raw_relative_ansi_request()const {require_current();return request_.impl_->raw_ansi;}
const std::u16string& BnwNativeColdArchiveRequest::CompletedDescriptor::raw_utf16_stream_name_candidate()const {require_current();return request_.impl_->raw_utf16;}
const std::filesystem::path& BnwNativeColdArchiveRequest::CompletedDescriptor::original_relative_io_token()const {require_current();return request_.impl_->relative;}
const std::filesystem::path& BnwNativeColdArchiveRequest::CompletedDescriptor::original_absolute_io_candidate()const {require_current();return request_.impl_->original_candidate;}
const std::filesystem::path& BnwNativeColdArchiveRequest::CompletedDescriptor::canonical_payload_io_candidate()const {require_current();return request_.impl_->payload_candidate;}

BnwNativeColdFilesystemOwner::BnwNativeColdFilesystemOwner(std::unique_ptr<Impl> impl)
    :impl_(std::move(impl)) {}
BnwNativeColdFilesystemOwner::~BnwNativeColdFilesystemOwner()=default;
std::shared_ptr<const BnwNativeColdFilesystemOwner> BnwNativeColdFilesystemOwner::acquire(
    std::shared_ptr<const BnwNativeEarlyInputLease> lease) {
    need(lease!=nullptr,"Cold filesystem cannot be created from missing source");
    lease->require_current();
    auto impl=std::make_unique<Impl>();impl->lease=std::move(lease);
    // Genuine lease pins exact original CivilizationV.exe; this is its parent
    // logical source base, not GetModuleFileNameA of Civis or ambient real CWD.
    impl->original_source_base=impl->lease->game_root();
    impl->canonical_payload_base=impl->lease->canonical_payload_root();
    impl->validate();
    auto owner=std::shared_ptr<const BnwNativeColdFilesystemOwner>(
        new BnwNativeColdFilesystemOwner(std::move(impl)));
    owner->impl_->self=owner;owner->require_current();return owner;
}
std::shared_ptr<const BnwNativeColdFilesystemOwner> BnwNativeColdFilesystemOwner::acquire(
    std::shared_ptr<const ::civ5::ui::BnwNativeInstallationInputOwner> installation){
    need(bool(installation),"Cold filesystem requires genuine installation input");installation->require_current();
    auto impl=std::make_unique<Impl>();impl->installation_input=std::move(installation);
    impl->original_source_base=impl->installation_input->game_root();impl->canonical_payload_base=impl->installation_input->canonical_payload_root();
    impl->validate();auto owner=std::shared_ptr<const BnwNativeColdFilesystemOwner>(new BnwNativeColdFilesystemOwner(std::move(impl)));
    owner->impl_->self=owner;owner->require_current();return owner;
}
void BnwNativeColdFilesystemOwner::require_same_input(const ::civ5::ui::BnwNativeInstallationInputOwner& input)const{
    require_current();need(!impl_->lease&&impl_->installation_input.get()==&input,"Cold filesystem has another real installation source");require_current();
}
const std::shared_ptr<const ::civ5::ui::BnwNativeInstallationInputOwner>& BnwNativeColdFilesystemOwner::installation_input_for_source()const{
    require_current();need(!(impl_->lease&&impl_->installation_input),"Cold has conflicting genuine source domains");return impl_->installation_input;
}
void BnwNativeColdFilesystemOwner::require_current() const { impl_->validate(); }
void BnwNativeColdFilesystemOwner::claim_initial_prefix_registry() const {
    require_fresh_direct_original_branch();
    need(!impl_->prefix_registry_claimed && !impl_->archive_header_started,
         "Cold initial shared prefix registry already claimed or header phase started");
    // Consume before allocation: failure cannot recreate a late empty registry.
    impl_->prefix_registry_claimed=true;
}
void BnwNativeColdFilesystemOwner::claim_initial_entry_admission_arena() const {
    require_fresh_direct_original_branch();
    need(impl_->prefix_registry_claimed && !impl_->archive_request_started && !impl_->archive_header_started &&
         !impl_->entry_admission_arena_claimed,
         "Native entry arena requires initial prefix registry and precedes every request/header, once");
    impl_->entry_admission_arena_claimed=true;
}
void BnwNativeColdFilesystemOwner::require_and_mark_archive_header_phase() const {
    require_fresh_direct_original_branch();
    need(impl_->prefix_registry_claimed,
         "Cold archive header requires the genuine initial shared prefix registry");
    // This closes fresh-registry creation; distinct archive headers remain valid.
    impl_->archive_header_started=true;
}
void BnwNativeColdFilesystemOwner::require_before_any_archive_request_and_header_without_callbacks() const {
    need(GetCurrentProcessId()==impl_->epoch->process && GetCurrentThreadId()==impl_->epoch->thread,
         "Cold initial cache claim foreign process/thread");
    need(impl_->prefix_registry_claimed && impl_->entry_admission_arena_claimed &&
         !impl_->archive_request_started && !impl_->archive_header_started,
         "Cold archive cache must precede every request and Header");
}
void BnwNativeColdFilesystemOwner::require_same_input(const BnwNativeEarlyInputLease& input) const {
    // Success below is only a factory-stored identity comparison.
    require_current();need(impl_->lease.get()==&input,"Cold filesystem belongs to another EARLY lease");
}
void BnwNativeColdFilesystemOwner::require_fresh_direct_original_branch() const {
    require_current();need(impl_->epoch->mapped_stream_callback==nullptr,
        "Cold source mapped-stream callback branch unavailable");
    if(impl_->installation_input)require_current();
}
const std::shared_ptr<const BnwNativeEarlyInputLease>& BnwNativeColdFilesystemOwner::early_input_lease() const {
    require_current();need(bool(impl_->lease),"Installed Cold requires actual installation source; imported Early borrower unavailable");return impl_->lease;
}
void BnwNativeColdFilesystemOwner::require_initial_policy_lease_identity_without_callbacks(
    const BnwNativeEarlyInputLease& expected)const {
    // Factory-stored lease identity only; no source/currentness verdict or pin.
    need(bool(impl_->lease),"Installed Cold requires actual installation source; imported Early borrower unavailable");
    need(impl_->lease.get()==&expected,"Default namespace constructor has another lease");
}
std::shared_ptr<const BnwNativeColdArchiveRequest> BnwNativeColdFilesystemOwner::primary_request_at_source_ordinal(
    std::size_t ordinal) const {
    need(GetCurrentProcessId()==impl_->epoch->process && GetCurrentThreadId()==impl_->epoch->thread,
         "Cold archive request foreign process/thread");
    need(ordinal<bnw_primary_archive_descriptors.size(),"Cold archive request ordinal outside source32");
    impl_->archive_request_started=true; // BEFORE first lease/provider callback.
    require_fresh_direct_original_branch();
    auto request=std::make_unique<BnwNativeColdArchiveRequest::Impl>();
    request->owner=impl_->self;request->epoch=impl_->epoch.get();request->ordinal=ordinal;
    request->raw_ansi=bnw_primary_archive_raw_ansi_literal(ordinal);
    request->raw_utf16=bnw_primary_archive_ascii_stream_token(request->raw_ansi);
    request->relative=primary_relative_io_path(request->raw_utf16);
    request->original_candidate=confined_original_stat_path(impl_->original_source_base,request->relative);
    request->payload_candidate=confined_original_stat_path(impl_->canonical_payload_base,request->relative);
    auto out=std::shared_ptr<const BnwNativeColdArchiveRequest>(
        new BnwNativeColdArchiveRequest(std::move(request)));
    require_current_request(*out);return out;
}
std::shared_ptr<const BnwNativeColdArchiveRequest> BnwNativeColdFilesystemOwner::request_from_category(
    std::shared_ptr<const BnwNativeCategoryArchiveChannel> channel) const {
    need(channel && channel->brand_, "Category archive request lacks genuine sealed attempt brand");
    need(GetCurrentProcessId()==impl_->epoch->process && GetCurrentThreadId()==impl_->epoch->thread,
         "Category archive request foreign process/thread");
    need(impl_->category_request_brands.insert(channel->brand_).second,
         "Category archive attempt request already issued"); // consume BEFORE callbacks
    impl_->archive_request_started=true;
    require_fresh_direct_original_branch();channel->require_active_request();
    // The SAME active saved Warm check just completed; only the immutable
    // Cold link remains. No allocation/callback separates these observations.
    if(channel->owned_warm_&&impl_->lease&&!impl_->installation_input&&channel->lease_&&!channel->installation_input_)
        need(channel->cold_.get()==this,"Category archive foreign Cold epoch");
    else channel->require_same_input(*this);
    auto request=std::make_unique<BnwNativeColdArchiveRequest::Impl>();
    request->owner=impl_->self;request->epoch=impl_->epoch.get();request->category=channel;request->is_category=true;
    request->ordinal=channel->source_ordinal();request->raw_ansi=channel->raw_relative_ansi_request();
    request->raw_utf16=channel->raw_utf16_stream_name_candidate();
    request->relative=channel->original_relative_io_token();
    request->original_candidate=confined_original_stat_path(impl_->original_source_base,request->relative);
    request->payload_candidate=confined_original_stat_path(impl_->canonical_payload_base,request->relative);
    need(request->raw_utf16==bnw_primary_archive_ascii_stream_token(request->raw_ansi) &&
         request->relative==primary_relative_io_path(request->raw_utf16) &&
         request->original_candidate==channel->original_absolute_io_candidate() &&
         request->payload_candidate==channel->canonical_payload_io_candidate(),
         "Category retained raw/UTF16/original/canonical request routes differ");
    auto out=std::shared_ptr<const BnwNativeColdArchiveRequest>(new BnwNativeColdArchiveRequest(std::move(request)));
    require_current_request(*out);channel->require_active_request();return out;
}
std::shared_ptr<const BnwNativeColdArchiveRequest> BnwNativeColdFilesystemOwner::request_from_owned_category(
    std::shared_ptr<const BnwNativeCategoryArchiveChannel> channel,const BnwNativeGlobalResourceWarmOwner& warm,
    const ::civ5::ui::BnwNativePackageDiscoveryOwner& parent)const {
    need(channel && channel->brand_, "Category archive request lacks genuine sealed attempt brand");
    need(GetCurrentProcessId()==impl_->epoch->process && GetCurrentThreadId()==impl_->epoch->thread,
         "Category archive request foreign process/thread");
    need(impl_->category_request_brands.insert(channel->brand_).second,
         "Category archive attempt request already issued"); // consume BEFORE callbacks
    impl_->archive_request_started=true;
    require_fresh_direct_original_branch();
    // Consume and the original Cold FULL precede even callable/context creation.
    const auto source_fence=[&]{
        need(channel&&channel->cold_.get()==this&&impl_->lease&&!impl_->installation_input&&
             impl_->archive_request_started&&impl_->category_request_brands.find(channel->brand_)!=impl_->category_request_brands.end(),
             "Owned archive factory lost SAME consumed Cold/channel source");
    };
    const auto mint=[&]{
        channel->require_active_request();
        // The SAME active saved Warm check just completed; only the immutable
        // Cold link remains. No allocation/callback separates these observations.
        if(channel->owned_warm_&&impl_->lease&&!impl_->installation_input&&channel->lease_&&!channel->installation_input_)
            need(channel->cold_.get()==this,"Category archive foreign Cold epoch");
        else channel->require_same_input(*this);
        auto request=std::make_unique<BnwNativeColdArchiveRequest::Impl>();
        request->owner=impl_->self;request->epoch=impl_->epoch.get();request->category=channel;request->is_category=true;
        request->ordinal=channel->source_ordinal();request->raw_ansi=channel->raw_relative_ansi_request();
        request->raw_utf16=channel->raw_utf16_stream_name_candidate();
        request->relative=channel->original_relative_io_token();
        request->original_candidate=confined_original_stat_path(impl_->original_source_base,request->relative);
        request->payload_candidate=confined_original_stat_path(impl_->canonical_payload_base,request->relative);
        need(request->raw_utf16==bnw_primary_archive_ascii_stream_token(request->raw_ansi) &&
             request->relative==primary_relative_io_path(request->raw_utf16) &&
             request->original_candidate==channel->original_absolute_io_candidate() &&
             request->payload_candidate==channel->canonical_payload_io_candidate(),
             "Category retained raw/UTF16/original/canonical request routes differ");
        auto out=std::shared_ptr<const BnwNativeColdArchiveRequest>(new BnwNativeColdArchiveRequest(std::move(request)));
        require_current_request(*out);channel->require_active_request();return out;
    };
    std::shared_ptr<const BnwNativeColdArchiveRequest> out;
    warm.with_owned_archive_factory_common_scope(parent,*this,channel,source_fence,[&]{out=mint();});
    return out; // Real Common POST completed before the actual caller can publish a receipt.
}
std::shared_ptr<const BnwNativeColdArchiveRequest> BnwNativeColdFilesystemOwner::request_from_common(
    std::shared_ptr<const BnwNativeCommon13ArchiveChannel> channel)const {
    need(channel && channel->brand_,"Common request lacks actual Root channel");
    need(GetCurrentProcessId()==impl_->epoch->process && GetCurrentThreadId()==impl_->epoch->thread,
         "Common request foreign process/thread");
    need(impl_->common_request_brands.insert(channel->brand_).second,"Common request already consumed");
    impl_->archive_request_started=true; // BEFORE callbacks or allocation
    channel->require_active();require_fresh_direct_original_branch();channel->require_active();
    need(channel->cold_.get()==this && channel->lease_.get()==impl_->lease.get() && channel->installation_input_.get()==impl_->installation_input.get() &&
         (static_cast<bool>(impl_->lease)!=static_cast<bool>(impl_->installation_input)),"Common request has another Cold/lease");
    auto request=std::make_unique<BnwNativeColdArchiveRequest::Impl>();
    request->owner=impl_->self;request->epoch=impl_->epoch.get();request->common=channel;request->is_common=true;
    request->ordinal=32+channel->ordinal_;request->raw_ansi=bnw_common13_raw_request(channel->ordinal_);
    request->raw_utf16=bnw_primary_archive_ascii_stream_token(request->raw_ansi);
    request->relative=primary_relative_io_path(request->raw_utf16);
    request->original_candidate=confined_original_stat_path(impl_->original_source_base,request->relative);
    request->payload_candidate=confined_original_stat_path(impl_->canonical_payload_base,request->relative);
    auto out=std::shared_ptr<const BnwNativeColdArchiveRequest>(new BnwNativeColdArchiveRequest(std::move(request)));
    require_current_request(*out);channel->require_active();return out;
}

void BnwNativeColdFilesystemOwner::require_request_ordinal_identity_without_callbacks(
    const BnwNativeColdArchiveRequest& request,std::size_t ordinal)const {
    // Factory-owned Request links/ordinal have no post-issue writers. This
    // method returns no fields and replaces no public Request current check.
    const auto& value=*request.impl_;
    need(!value.owner.expired()&&!value.owner.owner_before(impl_->self)&&
         !impl_->self.owner_before(value.owner)&&value.epoch==impl_->epoch.get(),
         "Cold archive request has another source epoch");
    need(!value.is_category&&!value.is_common&&!value.is_late_default&&value.ordinal==ordinal,
         "Default projection request ordinal differs");
}
void BnwNativeColdFilesystemOwner::require_current_request(const BnwNativeColdArchiveRequest& request) const {
    require_current();const auto& value=*request.impl_;
    const auto parent=value.owner.lock();
    need(parent.get()==this && value.epoch==impl_->epoch.get(),
         "Cold archive request has another source epoch");
    if(value.is_late_default) {
        const auto attempt=value.late_default.lock();
        need(attempt&&!value.is_common&&!value.is_category&&
             impl_->late_default_request_brands.contains(attempt->privatebrand_without_callbacks()),
             "Late Default request lost its actual issued source brand");
        attempt->require_same_input(*this);
        need(value.ordinal==attempt->source_ordinal()&&value.raw_ansi==attempt->raw_relative_ansi_request()&&
             value.raw_utf16==attempt->raw_utf16_stream_name_candidate()&&
             value.raw_utf16==bnw_primary_archive_ascii_stream_token(value.raw_ansi)&&
             value.relative==attempt->original_relative_io_token()&&value.relative==primary_relative_io_path(value.raw_utf16)&&
             value.original_candidate==confined_original_stat_path(impl_->original_source_base,value.relative)&&
             value.payload_candidate==confined_original_stat_path(impl_->canonical_payload_base,value.relative)&&
             value.original_candidate==attempt->original_absolute_io_candidate()&&
             value.payload_candidate==attempt->canonical_payload_io_candidate(),
             "Late Default request changed SAME sealed source/raw/physical lineage");
        require_current();attempt->require_current();return;
    }
    if(value.is_common) {
        const auto common=value.common.lock();need(bool(common),"Common request actual Root channel expired");
        common->require_current();
        need(!value.is_category && common->cold_.get()==this && common->lease_.get()==impl_->lease.get() && common->installation_input_.get()==impl_->installation_input.get() &&
             (static_cast<bool>(impl_->lease)!=static_cast<bool>(impl_->installation_input)) &&
             value.ordinal==32+common->ordinal_ && value.raw_ansi==bnw_common13_raw_request(common->ordinal_) &&
             value.raw_utf16==bnw_primary_archive_ascii_stream_token(value.raw_ansi) &&
             value.relative==primary_relative_io_path(value.raw_utf16) &&
             value.original_candidate==confined_original_stat_path(impl_->original_source_base,value.relative) &&
             value.payload_candidate==confined_original_stat_path(impl_->canonical_payload_base,value.relative),
             "Common request changed actual receiver/ordinal/raw route");
        require_current();common->require_current();return;
    }

    if(value.is_category) {
        const auto category=value.category.lock();need(bool(category),"Category request authentic attempt expired");
        category->require_same_input(*this);
        need(value.ordinal==category->source_ordinal() &&
             value.raw_ansi==category->raw_relative_ansi_request() &&
             value.raw_utf16==category->raw_utf16_stream_name_candidate() &&
             value.raw_utf16==bnw_primary_archive_ascii_stream_token(value.raw_ansi) &&
             value.relative==category->original_relative_io_token() &&
             value.relative==primary_relative_io_path(value.raw_utf16) &&
             value.original_candidate==confined_original_stat_path(impl_->original_source_base,value.relative) &&
             value.payload_candidate==confined_original_stat_path(impl_->canonical_payload_base,value.relative) &&
             value.original_candidate==category->original_absolute_io_candidate() &&
             value.payload_candidate==category->canonical_payload_io_candidate(),
             "Category archive request lost SAME sealed raw/physical lineage");
        require_current();return;
    }
    need(value.raw_ansi==bnw_primary_archive_raw_ansi_literal(value.ordinal) &&
         value.raw_utf16==bnw_primary_archive_ascii_stream_token(value.raw_ansi) &&
         value.relative==primary_relative_io_path(value.raw_utf16) &&
         value.original_candidate==confined_original_stat_path(impl_->original_source_base,value.relative) &&
         value.payload_candidate==confined_original_stat_path(impl_->canonical_payload_base,value.relative),
         "Cold request raw source/stream/physical candidate lineage changed");
    require_current();
}
BnwNativeColdArchiveRequest::BnwNativeColdArchiveRequest(std::unique_ptr<Impl> impl)
    :impl_(std::move(impl)) {}
BnwNativeColdArchiveRequest::~BnwNativeColdArchiveRequest()=default;
void BnwNativeColdArchiveRequest::require_same_category_channel(const BnwNativeCategoryArchiveChannel& channel)const {
    require_current();channel.require_current();
    need(impl_->is_category && impl_->category.lock().get()==&channel,"Category request belongs to another retained attempt");
}
void BnwNativeColdArchiveRequest::require_current() const {
    const auto owner=impl_->current_owner();owner->require_current_request(*this);
}
void BnwNativeColdArchiveRequest::require_same_owner(const BnwNativeColdFilesystemOwner& owner) const {
    require_current();
    // The first FULL has just validated the actual issuing Cold and raw route.
    // Qualify only the SAME installed Primary factory; every other origin or
    // owner mismatch keeps its original second FULL and refusal ordering.
    if(!impl_->is_category&&!impl_->is_common&&!impl_->is_late_default&&
       impl_->ordinal<bnw_primary_archive_descriptors.size()&&owner.impl_->installation_input&&!owner.impl_->lease&&
       impl_->epoch==owner.impl_->epoch.get()&&
       !impl_->owner.owner_before(owner.impl_->self)&&!owner.impl_->self.owner_before(impl_->owner)){
        owner.require_request_ordinal_identity_without_callbacks(*this,impl_->ordinal);return;
    }
    owner.require_current_request(*this);
}
std::size_t BnwNativeColdArchiveRequest::source_ordinal() const { require_current();return impl_->ordinal; }
const std::string& BnwNativeColdArchiveRequest::raw_relative_ansi_request() const { require_current();return impl_->raw_ansi; }
const std::u16string& BnwNativeColdArchiveRequest::raw_utf16_stream_name_candidate() const { require_current();return impl_->raw_utf16; }
const std::filesystem::path& BnwNativeColdArchiveRequest::original_relative_io_token() const { require_current();return impl_->relative; }
const std::filesystem::path& BnwNativeColdArchiveRequest::original_absolute_io_candidate() const { require_current();return impl_->original_candidate; }
const std::filesystem::path& BnwNativeColdArchiveRequest::canonical_payload_io_candidate() const { require_current();return impl_->payload_candidate; }
void BnwNativeColdArchiveRequest::require_native_prefix_registration() const {
    require_current();throw std::runtime_error("Cold raw stream candidate awaits native stream/header and SAME global prefix lookup/append");
}
void BnwNativeColdArchiveRequest::require_native_archive_cache_admission() const {
    require_current();throw std::runtime_error("Cold source request awaits ordered prefix/entry/FIRST/index admission before native archive cache insertion");
}

void BnwNativeCompletedColdSourceLoan::require_same_completed_common_channel(const BnwNativeCommon13ArchiveChannel& channel,
    const BnwNativeColdFilesystemOwner& cold)const {
    need(thread_==std::this_thread::get_id(),"Completed Common source loan foreign creator thread");
    // Complete actual active/generation/canonical Source bookend before the
    // pure Common hook; no borrowed channel Impl is dereferenced here.
    require_same_cold(cold);
    source_.require_actual_completed_common_channel_current(channel,observation_);
    require_same_cold(cold);
}
void BnwNativeColdFilesystemOwner::require_common_source_current(BnwNativeCommonSourceValidationContext& context)const {
    const auto check=[&]{context.check();need(impl_->lease&&!impl_->self.expired()&&
        GetCurrentProcessId()==impl_->epoch->process&&GetCurrentThreadId()==impl_->epoch->thread&&
        GetACP()==impl_->epoch->acp&&impl_->epoch->acp==1251&&impl_->epoch->mapped_stream_callback==nullptr,
        "Common Cold actual creator/epoch/direct branch changed");};
    check();auto visit=context.enter(*this,impl_->self);
    if(visit.work()==BnwNativeCommonSourceValidationContext::Work::validate_source) {
        impl_->lease->require_common_source_current(context);check();
        need(impl_->original_source_base==impl_->lease->game_root_common_source(context)&&
             impl_->canonical_payload_base==impl_->lease->canonical_payload_root_common_source(context)&&
             impl_->original_source_base.is_absolute()&&impl_->canonical_payload_base.is_absolute(),
             "Common Cold original/canonical source routing changed");check();
        const auto& profile=impl_->lease->profile_common_source(context);check();
        need(profile.can_activate&&profile.enabled_mods.empty()&&!profile.native_mods_require_port,
             "Common Cold genuine stock profile changed");
        impl_->lease->require_common_source_current(context);check();
    }
    check();visit.complete();
}
void BnwNativeColdFilesystemOwner::require_same_input_common_source(const BnwNativeEarlyInputLease& input,BnwNativeCommonSourceValidationContext& context)const {
    require_common_source_current(context);need(impl_->lease.get()==&input,"Common Cold has another Early source");context.check();
}
void BnwNativeColdFilesystemOwner::require_fresh_direct_original_branch_common_source(BnwNativeCommonSourceValidationContext& context)const {
    require_common_source_current(context);need(impl_->epoch->mapped_stream_callback==nullptr,"Common Cold mapped callback unavailable");context.check();
}
const std::shared_ptr<const BnwNativeEarlyInputLease>& BnwNativeColdFilesystemOwner::early_input_lease_common_source(BnwNativeCommonSourceValidationContext& context)const {
    require_common_source_current(context);context.require_borrowed(*this,impl_->self);return impl_->lease;
}
void BnwNativeColdFilesystemOwner::require_current_request_common_source(const BnwNativeColdArchiveRequest& request,BnwNativeCommonSourceValidationContext& context)const {
    require_common_source_current(context);const auto& value=*request.impl_;
    const auto check=[&]{context.check();need(!value.owner.expired()&&!value.owner.owner_before(impl_->self)&&
        !impl_->self.owner_before(value.owner)&&value.epoch==impl_->epoch.get(),"Common request has another Cold factory/epoch");};
    check();need(!value.is_category,"Common source validation cannot borrow category request");
    need(!value.is_late_default,"Common source validation cannot borrow installed late Default request");
    std::weak_ptr<const void> issuer=impl_->self;
    if(value.is_common) {
        const auto common=value.common.lock();need(bool(common),"Common request actual channel expired");
        common->require_common_source_current(context);check();
        need(common->brand_&&common->cold_.get()==this&&common->lease_.get()==impl_->lease.get()&&
             value.ordinal==32+common->ordinal_&&value.raw_ansi==bnw_common13_raw_request(common->ordinal_),
             "Common request actual channel/ordinal/brand changed");issuer=common->brand_;
    } else need(value.raw_ansi==bnw_primary_archive_raw_ansi_literal(value.ordinal),"Common validation primary request literal changed");
    auto visit=context.enter(request,issuer,value.ordinal);check();
    // This private Request's routing members and its Cold bases have no
    // post-publication writers. The SAME Parent-minted source visit performs
    // their lexical reconstruction once in each actual PRE/POST phase.
    // Every Cold/channel/epoch/brand/controller check above remains live.
    if(visit.work()==BnwNativeCommonSourceValidationContext::Work::validate_source) {
        need(value.raw_utf16==bnw_primary_archive_ascii_stream_token(value.raw_ansi)&&
             value.relative==primary_relative_io_path(value.raw_utf16)&&
             value.original_candidate==confined_original_stat_path(impl_->original_source_base,value.relative)&&
             value.payload_candidate==confined_original_stat_path(impl_->canonical_payload_base,value.relative),
             "Common request immutable raw/stream/original routing changed");
    }
    check();
    visit.complete();
}
void BnwNativeColdArchiveRequest::require_common_source_current(BnwNativeCommonSourceValidationContext& context)const {
    context.check();const auto owner=impl_->owner.lock();need(bool(owner),"Common request issuing Cold expired");
    owner->require_current_request_common_source(*this,context);context.check();
}
void BnwNativeColdArchiveRequest::require_same_owner_common_source(const BnwNativeColdFilesystemOwner& owner,BnwNativeCommonSourceValidationContext& context)const {
    require_common_source_current(context);owner.require_current_request_common_source(*this,context);context.check();
}
const std::string& BnwNativeColdArchiveRequest::raw_relative_ansi_request_common_source(BnwNativeCommonSourceValidationContext& context)const {require_common_source_current(context);return impl_->raw_ansi;}
const std::u16string& BnwNativeColdArchiveRequest::raw_utf16_stream_name_candidate_common_source(BnwNativeCommonSourceValidationContext& context)const {require_common_source_current(context);return impl_->raw_utf16;}
const std::filesystem::path& BnwNativeColdArchiveRequest::original_relative_io_token_common_source(BnwNativeCommonSourceValidationContext& context)const {require_common_source_current(context);return impl_->relative;}
const std::filesystem::path& BnwNativeColdArchiveRequest::original_absolute_io_candidate_common_source(BnwNativeCommonSourceValidationContext& context)const {require_common_source_current(context);return impl_->original_candidate;}
const std::filesystem::path& BnwNativeColdArchiveRequest::canonical_payload_io_candidate_common_source(BnwNativeCommonSourceValidationContext& context)const {require_common_source_current(context);return impl_->payload_candidate;}
}
