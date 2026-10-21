#include "bnw_native_primary32_loop_journal.hpp"
#include "bnw_native_archive_v6_entry_admission.hpp"
#include "bnw_native_callback_owner_guard.hpp"
#include "civ5/ui/bnw_native_package_discovery_owner.hpp"
#include <cerrno>
#include <chrono>
#include <cstdio>
#include <exception>
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
namespace civ5::ui::detail {
namespace {
void need(bool b,const char* m){if(!b)throw std::runtime_error(m);}
// Diagnostic only: no source/currentness/owner observation or receipt.
class BnwPrimary32AcceptanceTrace final {
    const long long ordinal_;
    std::chrono::steady_clock::time_point begin_{};
    int exceptions_=0;
 public:
    explicit BnwPrimary32AcceptanceTrace(std::size_t ordinal)noexcept:ordinal_(static_cast<long long>(ordinal)){
        const int saved_errno=errno;const DWORD saved_error=GetLastError();
        begin_=std::chrono::steady_clock::now();exceptions_=std::uncaught_exceptions();
        const auto stamp=std::chrono::duration_cast<std::chrono::milliseconds>(begin_.time_since_epoch()).count();
        std::fprintf(stderr,"[BNW_STAGE] BEGIN primary_default.primary32.journal.accept_step ordinal=%lld steady_ms=%lld elapsed_ms=0\n",
            ordinal_,static_cast<long long>(stamp));std::fflush(stderr);
        errno=saved_errno;SetLastError(saved_error);
    }
    BnwPrimary32AcceptanceTrace(const BnwPrimary32AcceptanceTrace&)=delete;
    BnwPrimary32AcceptanceTrace& operator=(const BnwPrimary32AcceptanceTrace&)=delete;
    ~BnwPrimary32AcceptanceTrace()noexcept{
        const int saved_errno=errno;const DWORD saved_error=GetLastError();
        const auto now=std::chrono::steady_clock::now();
        const auto stamp=std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
        const auto elapsed=std::chrono::duration_cast<std::chrono::milliseconds>(now-begin_).count();
        const int unwind=std::uncaught_exceptions()>exceptions_?1:0;
        std::fprintf(stderr,"[BNW_STAGE] END primary_default.primary32.journal.accept_step ordinal=%lld steady_ms=%lld elapsed_ms=%lld unwind=%d\n",
            ordinal_,static_cast<long long>(stamp),static_cast<long long>(elapsed),unwind);std::fflush(stderr);
        errno=saved_errno;SetLastError(saved_error);
    }
};
}
struct BnwNativePrimary32LoopJournalOwner::Impl {
    BnwNativeCallbackOwnerState state;
    std::weak_ptr<const BnwNativePrimary32LoopJournalOwner> self;
    std::weak_ptr<const BnwNativePackageDiscoveryOwner> parent;
    using Guard=BnwNativeCallbackOwnerGuard<BnwNativePrimary32LoopJournalOwner>;
    Guard guard()const noexcept{return Guard(state,self);}
    mutable BnwPrimary32JournalPolicyDiagnostic policy;
    mutable std::vector<std::unique_ptr<BnwNativePrimary32SourceBindings>> steps;
    template<class Check> static std::pair<std::size_t,std::uint32_t> validate_step(
        const BnwNativePrimary32SourceBindings& b,const BnwNativePrimary32SourceBindings* first,
        Check&& check) {
        need(b.cold&&b.prefixes&&b.arena&&b.cache&&b.request&&b.construction&&b.header&&b.prefix&&
             b.lookup&&b.entries&&b.publication&&b.source_step_brand,"Primary32 actual typed source step missing");check();
        if(first)need(b.cold.get()==first->cold.get()&&b.prefixes.get()==first->prefixes.get()&&
             b.arena.get()==first->arena.get()&&b.cache.get()==first->cache.get(),"Primary32 source parents replaced/late namespace/cache");
        b.cold->require_current();check();b.prefixes->require_current();check();
        b.arena->require_same_input(*b.cold);check();b.cache->require_same_arena(*b.arena);check();
        b.request->require_same_owner(*b.cold);check();const auto ordinal=b.request->source_ordinal();check();
        b.header->require_current();check();
        b.header->require_primary32_source_identities_without_callbacks(*b.request,*b.construction);check();
        const auto& header=b.prefix->header_owner();check();need(&header==b.header.get(),"Primary32 prefix has another Header");
        need(!b.lookup->hit(),"Positive Primary32 requires executed cache miss before constructor");check();
        b.entries->require_same_arena(*b.arena);check();
        // Immutable executed count, read immediately after FULL + controller check.
        const auto count=b.entries->admitted_count_after_validation_without_callbacks();check();
        need(count!=0,"Primary32 positive branch has zero actual admission; cache cannot be skipped successfully");
        b.publication->require_current();check();
        b.publication->require_primary32_construction_identity_without_callbacks(*b.construction);check();
        need(b.event_count==bnw_primary32_source_order.size()&&b.events==bnw_primary32_source_order,
             "Primary32 source checkpoint order incomplete");return {ordinal,count};
    }
    void validate_locked_kernel(const Guard& fence,const BnwNativePrimary32LoopJournalOwner& facade,
                                const std::shared_ptr<const BnwNativePackageDiscoveryOwner>& p)const {
        const auto check=[&](const BnwNativePrimary32SourceBindings& b){
            need(fence.valid()&&fence.valid_parent(p),"Primary32 journal/Package facade lost/reentered/foreign thread");
            p->require_primary32_retained_step_current(facade,b);};
        need(!policy.failed()&&policy.next()==steps.size(),"Primary32 journal retains terminal partial effects");
        for(const auto& s:steps){check(*s);validate_step(*s,steps.front().get(),[&]{check(*s);});}
        need(fence.valid()&&fence.valid_parent(p),"Primary32 parent facade lost after source callbacks");
    }
    void validate(const Guard& fence,const BnwNativePrimary32LoopJournalOwner& facade)const {
        const auto p=parent.lock();need(bool(p),"Primary32 source Package expired");
        validate_locked_kernel(fence,facade,p);
    }
    template<class Check> static std::pair<std::size_t,std::uint32_t> validate_step_for_default_publication(
        const BnwNativePrimary32SourceBindings& b,const BnwNativePrimary32SourceBindings* first,
        Check&& check,const BnwNativePackageDiscoveryOwner& source,
        const BnwNativePrimary32LoopJournalOwner& journal) {
        need(b.cold&&b.prefixes&&b.arena&&b.cache&&b.request&&b.construction&&b.header&&b.prefix&&
             b.lookup&&b.entries&&b.publication&&b.source_step_brand,"Primary32 actual typed source step missing");check();
        if(first)need(b.cold.get()==first->cold.get()&&b.prefixes.get()==first->prefixes.get()&&
             b.arena.get()==first->arena.get()&&b.cache.get()==first->cache.get(),"Primary32 source parents replaced/late namespace/cache");
        b.cold->require_current();check();b.prefixes->require_current();check();
        b.arena->require_same_input(*b.cold);check();b.cache->require_same_arena(*b.arena);check();
        b.request->require_same_owner(*b.cold);check();const auto ordinal=b.request->source_ordinal();check();
        b.header->require_current();check();
        b.header->require_primary32_source_identities_without_callbacks(*b.request,*b.construction);check();
        const auto& header=b.prefix->header_owner();check();need(&header==b.header.get(),"Primary32 prefix has another Header");
        need(!b.lookup->hit(),"Positive Primary32 requires executed cache miss before constructor");check();
        b.entries->require_same_arena(*b.arena);check();
        // Immutable executed count, read immediately after FULL + controller check.
        const auto count=b.entries->admitted_count_after_validation_without_callbacks();check();
        need(count!=0,"Primary32 positive branch has zero actual admission; cache cannot be skipped successfully");
        source.require_primary32_publication_fields_current(journal,b);check();
        need(b.event_count==bnw_primary32_source_order.size()&&b.events==bnw_primary32_source_order,
             "Primary32 source checkpoint order incomplete");return {ordinal,count};
    }
    void validate_for_default_publication(const Guard& fence,
        const BnwNativePrimary32LoopJournalOwner& facade,const BnwNativePackageDiscoveryOwner& source)const {
        const auto p=parent.lock();need(p&&p.get()==&source,"Primary32 publication has another actual Package");
        const auto check=[&](const BnwNativePrimary32SourceBindings& b){
            need(fence.valid()&&fence.valid_parent(p),"Primary32 publication journal/Package facade lost/reentered/foreign thread");
            p->require_primary32_retained_step_current(facade,b);};
        need(!policy.failed()&&policy.complete()&&policy.next()==steps.size()&&steps.size()==32,
             "Primary32 publication requires SAME complete positive immutable roster");
        for(const auto& s:steps){check(*s);
            validate_step_for_default_publication(*s,steps.front().get(),[&]{check(*s);},source,facade);}
        need(fence.valid()&&fence.valid_parent(p),"Primary32 publication parent facade lost after source callbacks");
    }

};
BnwNativePrimary32LoopJournalOwner::BnwNativePrimary32LoopJournalOwner(std::unique_ptr<Impl> i):impl_(std::move(i)){}
BnwNativePrimary32LoopJournalOwner::~BnwNativePrimary32LoopJournalOwner()=default;
std::shared_ptr<const BnwNativePrimary32LoopJournalOwner> BnwNativePrimary32LoopJournalOwner::acquire(
    const BnwNativePackageDiscoveryOwner& source) {
    // Root consumes creation BEFORE any callback. No fresh namespace/cache or
    // repeating first0: capture only Root's genuine completed existing first0.
    source.claim_primary32_journal_creation();
    auto impl=std::make_unique<Impl>();
    { BnwNativeCallbackConstructionGuard fence(impl->state);
    impl->parent=source.primary32_journal_source_identity();impl->steps.reserve(32);impl->policy.begin_attempt();
    auto b=source.capture_completed_primary32_first_step();
    const auto check=[&]{need(fence.valid(),"Primary32 unpublished journal reentered/foreign thread");
        source.require_primary32_pending_step_current(b);};check();
    const auto [ordinal,count]=Impl::validate_step(b,nullptr,check);need(ordinal==0,"Primary32 adoption must genuine existing ordinal0");
    auto step=std::unique_ptr<BnwNativePrimary32SourceBindings>(new BnwNativePrimary32SourceBindings(std::move(b)));
    // Last callbacks/checks before journal ownership stores.
    const auto staged_check=[&]{need(fence.valid(),"Primary32 unpublished journal retired");source.require_primary32_pending_step_current(*step);};
    Impl::validate_step(*step,nullptr,staged_check);
    impl->policy.complete_diagnostic(ordinal,step->events,count);impl->steps.push_back(std::move(step));
    } // Guard must die BEFORE moving its referenced Impl into throwing control-block allocation.
    auto out=std::shared_ptr<const BnwNativePrimary32LoopJournalOwner>(new BnwNativePrimary32LoopJournalOwner(std::move(impl)));
    out->impl_->self=out;return out;
}
void BnwNativePrimary32LoopJournalOwner::advance_one_step(const BnwNativePackageDiscoveryOwner& source)const {
    auto fence=impl_->guard();need(fence.valid(),"Primary32 loop facade lost/reentered/foreign thread");
    // Do not lock parent weak_ptr here: that transient pin could mask final
    // parent facade loss during actual Root scoped callbacks.
    need(impl_->parent.lock().get()==&source,"Primary32 loop has another Package");
    impl_->policy.begin_attempt();
    struct Failure {Impl& i;bool done=false;~Failure(){if(!done)i.policy.fail_attempt();}} failure{*impl_};
    const auto expected=impl_->policy.next();
    need(impl_->steps.size()==expected && expected<impl_->steps.capacity(),
         "Primary32 actual journal lacks its reserved retained step slot");
    auto b=source.produce_next_primary32_step(*this,expected);
    const auto check=[&]{need(fence.valid(),"Primary32 journal caught reentry/lost facade");
        source.require_primary32_pending_step_current(b);};check();
    auto step=std::unique_ptr<BnwNativePrimary32SourceBindings>(new BnwNativePrimary32SourceBindings(std::move(b)));
    const auto staged_check=[&]{need(fence.valid(),"Primary32 journal lost before commit");source.require_primary32_pending_step_current(*step);};
    staged_check();
    // Actual retained storage exists before the ONE full callbackful source
    // validation. Private guard sampling does not promise callback-count parity.
    const auto [ordinal,count]=[&]{
        if(source.has_installation_primary32_source_without_callbacks())
            return source.validate_installation_primary32_new_step_acceptance(*this,*step,*impl_->steps.front(),fence,expected);
        BnwPrimary32AcceptanceTrace trace(expected);
        return Impl::validate_step(*step,impl_->steps.front().get(),staged_check);
    }();
    need(ordinal==expected,"Primary32 actual ColdRequest skipped/replayed source ordinal");
    staged_check();
    impl_->policy.complete_diagnostic(ordinal,step->events,count);impl_->steps.push_back(std::move(step));failure.done=true;
}
void BnwNativePrimary32LoopJournalOwner::require_new_step_acceptance_frame_current(
    const BnwNativePackageDiscoveryOwner& source,const BnwNativePrimary32SourceBindings& b,
    const BnwNativePrimary32SourceBindings& first,const BnwNativeCallbackOwnerGuard<BnwNativePrimary32LoopJournalOwner>& fence,
    std::size_t expected)const{
    need(fence.valid(),"Primary32 journal lost before commit");
    need(!impl_->policy.failed()&&impl_->policy.next()==expected&&expected>0&&expected<32&&
         impl_->steps.size()==expected&&expected<impl_->steps.capacity()&&impl_->steps.front().get()==&first,
         "Primary32 new-step acceptance lost actual Journal counter/FIRST/reserved slot");
    source.require_primary32_pending_step_current(b);
}
std::pair<std::size_t,std::uint32_t>
BnwNativePrimary32LoopJournalOwner::validate_new_step_for_installation_acceptance(
    const BnwNativePackageDiscoveryOwner& source,const BnwNativePrimary32SourceBindings& b,
    const BnwNativePrimary32SourceBindings& first,const BnwNativeCallbackOwnerGuard<BnwNativePrimary32LoopJournalOwner>& fence,
    std::size_t expected)const{
    require_new_step_acceptance_frame_current(source,b,first,fence,expected);
    const auto staged_check=[&]{need(fence.valid(),"Primary32 journal lost before commit");source.require_primary32_pending_step_current(b);};
    BnwPrimary32AcceptanceTrace trace(expected);
    return Impl::validate_step(b,&first,staged_check); // Same ONE full supplier sequence/result; no callable allocation.
}
void BnwNativePrimary32LoopJournalOwner::require_current_for_default_publication(
    const BnwNativePackageDiscoveryOwner& source)const {
    auto fence=impl_->guard();impl_->validate_for_default_publication(fence,*this,source);
}
BnwNativePrimary32JournalFrameDiagnostic BnwNativePrimary32LoopJournalOwner::current_for_owned_primary_completion(
    const BnwNativePackageDiscoveryOwner& source,const BnwNativeCallbackOwnerGuard<BnwNativePackageDiscoveryOwner>& actual_fence)const {
    source.require_primary32_completion_observation_current(*this,actual_fence);
    auto fence=impl_->guard();need(fence.valid(),"Primary32 completion observer retired/reentered");
    // Existing complete32/SAME retained bindings and Publication FULL PRE/POST
    // supplier. This is the actual post32 consumer, not a Default scope grant.
    impl_->validate_for_default_publication(fence,*this,source);
    source.require_primary32_completion_observation_current(*this,actual_fence);
    need(fence.valid(),"Primary32 completion observer retired in source callbacks");
    return {impl_->policy.next(),impl_->policy.total(),impl_->policy.complete()};
}
BnwNativePrimary32PostpublishedJournalOperation::BnwNativePrimary32PostpublishedJournalOperation(
    const BnwNativePrimary32LoopJournalOwner& journal,
    const BnwNativeCallbackOwnerGuard<BnwNativePrimary32LoopJournalOwner>& fence,
    const std::shared_ptr<const BnwNativePackageDiscoveryOwner>& parent)noexcept:
    journal_(journal),fence_(fence),parent_(parent){}
void BnwNativePrimary32PostpublishedJournalOperation::require_current(
    const BnwNativePackageDiscoveryOwner& source,const BnwNativePrimary32LoopJournalOwner& journal)const{
    need(parent_&&parent_.get()==&source&&&journal==&journal_&&fence_.valid()&&fence_.valid_parent(parent_),
         "Primary32 journal/Package facade lost/reentered/foreign thread");
    const auto& i=*journal_.impl_;
    need(!i.policy.failed()&&i.policy.next()==i.steps.size(),"Primary32 journal retains terminal partial effects");
    need(i.policy.complete()&&i.steps.size()==32,"Postpublished Journal requires SAME completed actual roster");
}
BnwNativePrimary32JournalFrameDiagnostic BnwNativePrimary32PostpublishedJournalOperation::execute()const{
    journal_.impl_->validate_locked_kernel(fence_,journal_,parent_); // ONE unchanged ordinary FULL32 kernel.
    const auto& policy=journal_.impl_->policy;
    return {policy.next(),policy.total(),policy.complete()};
}
BnwNativePrimary32JournalFrameDiagnostic BnwNativePrimary32LoopJournalOwner::current_for_postpublished_prefix(
    const BnwNativePackageDiscoveryOwner& source,
    const BnwNativeCallbackOwnerGuard<BnwNativePackageDiscoveryOwner>& actual_fence)const{
    auto fence=impl_->guard();
    const auto p=impl_->parent.lock();need(bool(p),"Primary32 source Package expired"); // Original ONE keeper.
    BnwNativePrimary32PostpublishedJournalOperation operation(*this,fence,p);
    return source.validate_postpublished_primary32_journal(operation,actual_fence);
} // Operation, original keeper and Journal guard finish in their real order.
void BnwNativePrimary32LoopJournalOwner::require_current()const{auto f=impl_->guard();impl_->validate(f,*this);}
BnwNativePrimary32JournalFrameDiagnostic BnwNativePrimary32LoopJournalOwner::current()const{
    auto f=impl_->guard();impl_->validate(f,*this);return {impl_->policy.next(),impl_->policy.total(),impl_->policy.complete()};
}
}
