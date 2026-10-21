#include "bnw_native_common_source_validation_context.hpp"
#include "bnw_native_archive_basename_cache.hpp"
#include "bnw_native_global_resource_warm_owner.hpp"
#include "civ5/ui/bnw_native_installation_input_owner.hpp"
#include "bnw_native_completed_cold_source_loan.hpp"
#include "bnw_native_package_category_source_owner.hpp"
#include "bnw_native_archive_v6_entry_admission.hpp"
#include "bnw_native_callback_owner_guard.hpp"
#include "civ5/ui/bnw_native_package_discovery_owner.hpp"
#include <clocale>
#include <thread>
namespace civ5::ui::detail {
namespace {
void need(bool v,const char* m){if(!v)throw std::runtime_error(m);}
void ascii_locale(){const auto* s=std::setlocale(LC_CTYPE,nullptr);need(s&&std::string_view(s)=="C","Native cache requires C _strlwr scope");}
}
struct BnwNativeArchiveBasenameCacheOwner::Impl {
    const std::thread::id creator_thread=std::this_thread::get_id();
    BnwNativeCallbackOwnerState state;
    std::weak_ptr<const BnwNativeArchiveBasenameCacheOwner> self;
    using Guard=BnwNativeCallbackOwnerGuard<BnwNativeArchiveBasenameCacheOwner>;
    Guard guard()const noexcept{return Guard(state,self);}
    std::weak_ptr<const BnwNativeArchiveEntryAdmissionArena> arena;
    std::weak_ptr<const BnwNativeColdFilesystemOwner> cold;
    std::shared_ptr<const BnwNativeEarlyInputLease> lease;
    std::shared_ptr<const ::civ5::ui::BnwNativeInstallationInputOwner> installation_input;
    using Cache=BnwArchiveBasenameCacheStoragePolicy<std::shared_ptr<const BnwNativeArchiveCachePublicationOwner>>;
    mutable Cache default_cache;
    Cache& cache; // Category aliases the SAME retained namespace +38 store.
    const Cache* const original_store=&cache;
    std::shared_ptr<const BnwNativeCategoryNamespaceChannel> category_namespace;
    Impl():cache(default_cache){}
    explicit Impl(Cache& actual):cache(actual){}
    void check(const Guard& f)const {need(f.valid(),"Native archive cache facade lost/reentered/foreign thread");}
    std::shared_ptr<const BnwNativeArchiveEntryAdmissionArena> validate(const Guard& f)const {
        check(f);auto a=arena.lock();need(bool(a),"Native cache default Arena expired");
        a->require_current();check(f);need(f.valid_parent(a),"Native cache persistent Arena lost");
        auto c=cold.lock();need(bool(c),"Native cache Cold expired");if(installation_input){need(!lease,"Installed resource source cannot carry imported save lease");
            c->require_same_input(*installation_input);check(f);
        }else {c->require_same_input(*lease);check(f);}
        need(f.valid_parent(c),"Native cache persistent Cold lost");a->require_same_input(*c);check(f);
        need(f.valid_parent(a)&&f.valid_parent(c),"Native cache parents lost after callbacks");ascii_locale();return a;
    }
    std::shared_ptr<const BnwNativeArchiveEntryAdmissionArena> validate_common(const Guard& f,BnwNativeCommonSourceValidationContext& context)const {
        need(lease&&!installation_input,"Mandatory installed common cache source before late use");
        check(f);need(!category_namespace,"Common scope requires actual initial basename cache");auto a=arena.lock();need(bool(a),"Native cache default Arena expired");
        a->require_common_source_current(context);check(f);need(f.valid_parent(a),"Native cache persistent Arena lost");
        auto c=cold.lock();need(bool(c),"Native cache Cold expired");c->require_same_input_common_source(*lease,context);check(f);
        need(f.valid_parent(c),"Native cache persistent Cold lost");a->require_same_input_common_source(*c,context);check(f);
        need(f.valid_parent(a)&&f.valid_parent(c),"Native cache parents lost after callbacks");ascii_locale();return a;
    }
    void validate_completed(const Guard& f,const BnwNativeCompletedColdSourceLoan& loan,
        const BnwNativeColdFilesystemOwner& actual_cold,const BnwNativeArchiveEntryAdmissionArena& actual_arena)const {
        check(f);need(lease&&!installation_input,"Mandatory installed completed cache source before late use");
        actual_cold.require_same_completed_owner_control(cold,loan);check(f);
        loan.require_same_early_input(*lease);check(f);
        actual_arena.require_same_completed_owner_control(arena,loan,actual_cold);check(f);
        // Category cache still aliases the genuine namespace +38 store; its
        // currentness is validated explicitly by the same actual Arena route.
        if(category_namespace)category_namespace->require_current_completed(loan,actual_cold);
        check(f);ascii_locale();actual_cold.require_same_completed_owner_control(cold,loan);check(f);
        actual_arena.require_same_completed_owner_control(arena,loan,actual_cold);check(f);
    }
};
struct BnwNativeArchiveCacheLookupOwner::Impl {
    BnwNativeCallbackOwnerState state;
    std::weak_ptr<const BnwNativeArchiveCacheLookupOwner> self;
    using Guard=BnwNativeCallbackOwnerGuard<BnwNativeArchiveCacheLookupOwner>;
    Guard guard()const noexcept{return Guard(state,self);}
    std::weak_ptr<const BnwNativeArchiveBasenameCacheOwner> cache;
    std::shared_ptr<const BnwNativeColdArchiveRequest> request;
    std::string key;
    bool hit=false;
    mutable bool publication_attempted=false;
    void validate(const Guard& f)const {
        need(f.valid(),"Native cache lookup facade lost/reentered/foreign thread");
        auto c=cache.lock();need(bool(c),"Native lookup cache expired");c->require_current();
        need(f.valid()&&f.valid_parent(c),"Native lookup persistent cache lost");request->require_current();
        need(f.valid()&&f.valid_parent(c),"Native lookup cache lost after request callback");
    }
};
struct BnwNativeArchiveCachePublicationOwner::Impl {
    const std::thread::id creator_thread=std::this_thread::get_id();
    BnwNativeCallbackOwnerState state;
    std::weak_ptr<const BnwNativeArchiveCachePublicationOwner> self;
    using Guard=BnwNativeCallbackOwnerGuard<BnwNativeArchiveCachePublicationOwner>;
    Guard guard()const noexcept{return Guard(state,self);}
    std::weak_ptr<const BnwNativeArchiveBasenameCacheOwner> cache;
    std::shared_ptr<const BnwNativeArchiveV6EntryAdmissionOwner> entries;
    std::shared_ptr<const BnwNativeArchiveV6StreamHeaderOwner> header;
    std::shared_ptr<const BnwNativeGlobalArchivePrefixRecord> prefix;
    std::shared_ptr<const assets::FpkSource> canonical;
    std::string key;
    std::int64_t mtime=0;
    bool published=false;
    void validate(const Guard& f,const BnwNativeArchiveCachePublicationOwner& facade)const {
        const auto check=[&]{need(f.valid()&&published,"Native cache publication facade lost/reentered/unpublished");};
        check();auto c=cache.lock();need(bool(c),"Native publication cache expired");c->require_current();check();
        need(f.valid_parent(c),"Native publication persistent cache lost");
        const auto selected=c->impl_->cache.lookup(key);need(selected&&selected->get()==&facade,"Native archive cache has another selected value");
        entries->require_current();check();header->require_current();check();prefix->require_current();check();
        need(f.valid_parent(c),"Native publication cache lost after source callbacks");
        const auto again=c->impl_->cache.lookup(key);need(again&&again->get()==&facade,"Native archive cache selection changed");
    }
    // Original83B130 mode6/7 reads the cached archive +258/+25C words.
    // The successful publication owns those admission-time fields until this
    // exact namespace cache removes/replaces the actual publication. This is
    // not a claim that the archive path still has its old filesystem contents.
    void validate_retained_mtime(const Guard& f,const BnwNativeArchiveCachePublicationOwner& facade)const {
        const auto check=[&]{need(f.valid()&&published,
            "Retained archive time facade lost/reentered/unpublished");};
        check();auto c=cache.lock();need(bool(c),"Retained archive time namespace expired");
        c->require_current();check();
        need(f.valid_parent(c)&&c->impl_->original_store==&c->impl_->cache&&
             !cache.owner_before(c->impl_->self)&&!c->impl_->self.owner_before(cache),
             "Retained archive time lost original namespace cache/control");
        need(entries&&header&&prefix&&canonical,
             "Retained archive time lost owned admission source storage");
        const auto* selected=c->impl_->cache.lookup(key);
        need(selected&&selected->get()==&facade&&
             !selected->owner_before(self)&&!self.owner_before(*selected),
             "Retained archive time cache selects another publication/control");
        check(); // No callout/allocation separates this live slot check and the scalar read.
    }
    void validate_common(const Guard& f,const BnwNativeArchiveCachePublicationOwner& facade,BnwNativeCommonSourceValidationContext& context)const {
        const auto check=[&]{need(f.valid()&&published,"Native cache publication facade lost/reentered/unpublished");};
        check();auto c=cache.lock();need(bool(c),"Native publication cache expired");c->require_common_source_current(context);check();
        need(f.valid_parent(c),"Native publication persistent cache lost");
        const auto selected=c->impl_->cache.lookup(key);need(selected&&selected->get()==&facade&&!selected->owner_before(self)&&!self.owner_before(*selected),"Native archive cache has another selected value");
        entries->require_common_source_current(context);check();header->require_common_source_current(context);check();prefix->require_common_source_current(context);check();
        need(f.valid_parent(c),"Native publication cache lost after source callbacks");
        const auto again=c->impl_->cache.lookup(key);need(again&&again->get()==&facade&&!again->owner_before(self)&&!self.owner_before(*again),"Native archive cache selection changed");
    }
    void validate_completed(const Guard& f,const BnwNativeArchiveCachePublicationOwner& facade,
        const BnwNativeCompletedColdSourceLoan& loan,const BnwNativeColdFilesystemOwner& cold,
        const BnwNativeArchiveEntryAdmissionArena& arena,const BnwNativeGlobalArchivePrefixOwner& registry,
        const BnwNativeArchiveBasenameCacheOwner& actual_cache)const {
        const auto check=[&]{need(f.valid()&&published,"Completed publication lost/reentered/unpublished/foreign thread");};
        check();actual_cache.require_same_completed_owner_control(cache,loan,cold,arena);check();
        const auto selected=actual_cache.impl_->cache.lookup(key);
        need(selected&&selected->get()==&facade&&!selected->owner_before(self)&&!self.owner_before(*selected),
            "Completed actual archive cache selects another publication/control");
        need(entries&&header&&prefix&&canonical,"Completed publication original source ancestry absent");
        entries->require_current_completed(loan,cold,arena,registry);check();
        header->require_current_completed(loan,cold);check();prefix->require_current_completed(loan,cold,registry);check();
        const auto ancestry=entries->bound_archive_cache_sources_completed(loan,cold,arena,registry);check();
        need(ancestry.first.get()==header.get()&&ancestry.second.get()==prefix.get(),
            "Completed publication is another actual Entry/Header/Prefix source");
        actual_cache.require_same_completed_owner_control(cache,loan,cold,arena);check();
        const auto again=actual_cache.impl_->cache.lookup(key);
        need(again&&again->get()==&facade&&!again->owner_before(self)&&!self.owner_before(*again),
            "Completed actual archive cache selection/control changed");
    }
};
BnwNativeArchiveBasenameCacheOwner::BnwNativeArchiveBasenameCacheOwner(std::unique_ptr<Impl> i):impl_(std::move(i)){}
BnwNativeArchiveBasenameCacheOwner::~BnwNativeArchiveBasenameCacheOwner()=default;
std::shared_ptr<const BnwNativeArchiveBasenameCacheOwner> BnwNativeArchiveBasenameCacheOwner::acquire(
    std::shared_ptr<const BnwNativeArchiveEntryAdmissionArena> arena,const BnwNativePackageDiscoveryOwner& source) {
    need(bool(arena),"Native cache requires genuine default Arena");source.require_archive_cache_initialization_current(*arena);
    auto cold=arena->claim_initial_archive_basename_cache();source.require_archive_cache_initialization_current(*arena);
    auto i=std::make_unique<Impl>();i->arena=arena;i->cold=cold;i->installation_input=cold->installation_input_for_source();
    if(!i->installation_input){i->lease=cold->early_input_lease();}
    source.require_archive_cache_initialization_current(*arena);ascii_locale();
    need(arena.use_count()>1&&cold.use_count()>1,"Native cache parents only temporary factory pins");
    auto out=std::shared_ptr<const BnwNativeArchiveBasenameCacheOwner>(new BnwNativeArchiveBasenameCacheOwner(std::move(i)));
    out->impl_->self=out;return out;
}
void BnwNativeArchiveBasenameCacheOwner::require_common_source_current(BnwNativeCommonSourceValidationContext& context)const {
    auto fence=impl_->guard();need(fence.valid(),"Common cache facade lost/reentered/foreign thread");
    auto visit=context.enter(*this,impl_->self);
    impl_->validate_common(fence,context);
    need(fence.valid(),"Common cache facade changed");visit.complete();
}
void BnwNativeArchiveBasenameCacheOwner::require_current()const{auto f=impl_->guard();impl_->validate(f);}
void BnwNativeArchiveBasenameCacheOwner::require_same_arena(const BnwNativeArchiveEntryAdmissionArena& expected)const{
    auto f=impl_->guard();const auto a=impl_->validate(f);need(a.get()==&expected,"Native cache has another Arena");
}
std::shared_ptr<const BnwNativeArchiveCacheLookupOwner> BnwNativeArchiveBasenameCacheOwner::lookup_before_constructor(
    std::shared_ptr<const BnwNativeColdArchiveRequest> request,const BnwNativePackageDiscoveryOwner& source)const {
    need(bool(request),"Native cache lookup requires source request");auto f=impl_->guard();
    const auto a=impl_->arena.lock();need(bool(a),"Native lookup Arena expired");
    const auto check=[&]{impl_->check(f);need(f.valid_parent(a),"Native lookup Arena persistent owner lost");
        source.require_archive_cache_lookup_current(*this,*a,*request);};check();
    impl_->validate(f);check();auto cold=impl_->cold.lock();need(bool(cold),"Native lookup Cold expired");
    request->require_same_owner(*cold);check();need(f.valid_parent(cold),"Native lookup Cold persistent owner lost");
    const auto raw=request->raw_relative_ansi_request();check();const auto key=archive_cache_basename_ascii(raw);
    need(!key.empty(),"Finite primary archive basename empty");ascii_locale();
    auto i=std::make_unique<BnwNativeArchiveCacheLookupOwner::Impl>();i->cache=impl_->self;i->request=request;i->key=key;
    i->hit=impl_->cache.lookup(key)!=nullptr;
    auto out=std::shared_ptr<const BnwNativeArchiveCacheLookupOwner>(new BnwNativeArchiveCacheLookupOwner(std::move(i)));
    out->impl_->self=out;check();return out;
}
std::shared_ptr<const BnwNativeArchiveCachePublicationOwner> BnwNativeArchiveBasenameCacheOwner::publish_after_entries(
    std::shared_ptr<const BnwNativeArchiveCacheLookupOwner> lookup,
    std::shared_ptr<const BnwNativeArchiveV6EntryAdmissionOwner> entries,
    const BnwNativePackageDiscoveryOwner& source)const {
    need(lookup&&entries,"Native cache publication requires actual lookup/entry receipt");auto f=impl_->guard();
    const auto a=impl_->arena.lock();need(bool(a),"Native cache publication Arena expired");
    const auto check=[&]{impl_->check(f);need(f.valid_parent(a),"Native publication Arena lost");
        source.require_archive_cache_publication_current(*this,*a,*lookup,*entries);};check();
    need(lookup->impl_->cache.lock().get()==this && !lookup->impl_->publication_attempted,
         "Native publication lookup has another cache/already consumed");
    lookup->impl_->publication_attempted=true; // BEFORE first validation callback.
    impl_->validate(f);check();
    // Do not call Lookup.current while holding its Cache fence (reentrant).
    lookup->impl_->request->require_current();check();need(!lookup->impl_->hit,"Native cache hit must skip constructor/publication");
    need(impl_->cache.lookup(lookup->impl_->key)==nullptr,"Native cache changed after pre-constructor miss");
    // Closed operation-local borrower. Validators still invoke real native
    // currentness callbacks; fewer repeated validations are intentional.
    const std::function<void()> controller=check;
    std::shared_ptr<const BnwNativeArchiveCachePublicationOwner> out;
    std::optional<Impl::Cache> staged;
    // Allocate/prepare SAME fields under the genuine Entry-owned staging loan.
    // It preserves all three Entry FULLs, including the final source POST
    // after this copied staging target's destruction.
    const auto stage=[&](const BnwNativeArchiveV6EntryAdmissionOwner::DefaultCachePublicationStagingLoan& loan) {
        loan.require_current_without_callbacks();const auto& inputs=loan.inputs_without_callbacks();check();
        const auto& header=inputs.header;const auto& prefix=inputs.prefix;
        need(header&&prefix,"Native entry cache sources unavailable");
        need(inputs.request==lookup->impl_->request.get(),"Native entry came from another lookup request");
        const auto count=inputs.admitted;check();
        need(archive_cache_positive_loader_diagnostic(count),"Native loader result zero forbids archive cache insertion");
        const auto mtime=inputs.mtime;check();const auto canonical=inputs.canonical;check();
        auto i=std::make_unique<BnwNativeArchiveCachePublicationOwner::Impl>();i->cache=impl_->self;
        i->entries=entries;i->header=header;i->prefix=prefix;i->canonical=canonical;i->mtime=mtime;i->key=lookup->impl_->key;
        out=std::shared_ptr<const BnwNativeArchiveCachePublicationOwner>(new BnwNativeArchiveCachePublicationOwner(std::move(i)));
        out->impl_->self=out;
        staged.emplace(impl_->cache); // SAME original copy position, after actual out/control-block construction.
        need(staged->insert_unique(lookup->impl_->key,out),"Native82B590 existing key cannot overwrite");
        loan.require_current_without_callbacks();check();
    };
    entries->with_default_cache_publication_stage(*a,*lookup->impl_->request,controller,stage);check();
    entries->require_default_cache_stage_retired_without_callbacks();check();ascii_locale();
    need(impl_->cache.lookup(lookup->impl_->key)==nullptr,"Native publication changed before store");check();
    impl_->cache.swap(*staged);out->impl_->published=true;return out;
}
std::shared_ptr<const BnwNativeArchiveBasenameCacheOwner> BnwNativeArchiveBasenameCacheOwner::acquire_category(
    std::shared_ptr<const BnwNativeArchiveEntryAdmissionArena> arena,
    std::shared_ptr<const BnwNativeCategoryNamespaceChannel> channel) {
    need(arena&&channel,"Category cache requires retained namespace/Arena");
    channel->require_initialization();arena->require_same_category_channel(*channel);
    auto cold=arena->claim_category_archive_basename_cache(*channel);
    channel->require_initialization();
    auto& actual=channel->cache_storage();need(actual.size()==0,"Category initial cache already populated");
    auto i=std::make_unique<Impl>(actual);i->arena=arena;i->cold=cold;
    if(channel->installation_input_){
        i->installation_input=cold->installation_input_for_source();
        need(i->installation_input&&i->installation_input.get()==channel->installation_input_.get()&&
             !i->installation_input.owner_before(channel->installation_input_)&&!channel->installation_input_.owner_before(i->installation_input),
             "Installed category cache has another actual Cold/input source");
    }else i->lease=cold->early_input_lease();
    i->category_namespace=channel;channel->require_initialization();ascii_locale();
    need(arena.use_count()>1&&cold.use_count()>1,"Category cache requires persistent actual parents");
    auto out=std::shared_ptr<const BnwNativeArchiveBasenameCacheOwner>(new BnwNativeArchiveBasenameCacheOwner(std::move(i)));
    out->impl_->self=out;return out;
}
std::shared_ptr<const BnwNativeArchiveCacheLookupOwner> BnwNativeArchiveBasenameCacheOwner::lookup_category_before_constructor(
    std::shared_ptr<const BnwNativeColdArchiveRequest> request,std::shared_ptr<const BnwNativeCategoryArchiveChannel> channel)const {
    need(request&&channel,"Category cache lookup requires genuine request/channel");auto f=impl_->guard();
    const auto a=impl_->arena.lock();need(bool(a),"Native lookup Arena expired");
    const auto check=[&]{impl_->check(f);need(f.valid_parent(a),"Native lookup Arena persistent owner lost");
        channel->require_active_request();
        need(impl_->category_namespace && channel->namespace_channel()==impl_->category_namespace,
             "Category lookup has another namespace/cache");
        a->require_same_category_channel(*impl_->category_namespace);impl_->check(f);
        request->require_same_category_channel(*channel);impl_->check(f);};check();
    impl_->validate(f);check();auto cold=impl_->cold.lock();need(bool(cold),"Native lookup Cold expired");
    request->require_same_owner(*cold);check();need(f.valid_parent(cold),"Native lookup Cold persistent owner lost");
    const auto raw=request->raw_relative_ansi_request();check();const auto key=archive_cache_basename_ascii(raw);
    need(!key.empty(),"Finite primary archive basename empty");ascii_locale();
    auto i=std::make_unique<BnwNativeArchiveCacheLookupOwner::Impl>();i->cache=impl_->self;i->request=request;i->key=key;
    i->hit=impl_->cache.lookup(key)!=nullptr;
    auto out=std::shared_ptr<const BnwNativeArchiveCacheLookupOwner>(new BnwNativeArchiveCacheLookupOwner(std::move(i)));
    out->impl_->self=out;check();return out;
}
std::shared_ptr<const BnwNativeArchiveCacheLookupOwner> BnwNativeArchiveBasenameCacheOwner::lookup_owned_category_before_constructor(
    std::shared_ptr<const BnwNativeColdArchiveRequest> request,std::shared_ptr<const BnwNativeCategoryArchiveChannel> channel,
    const BnwNativeGlobalResourceWarmOwner& warm,const ::civ5::ui::BnwNativePackageDiscoveryOwner& parent)const {
    need(request&&channel,"Category cache lookup requires genuine request/channel");auto f=impl_->guard();
    const auto a=impl_->arena.lock();need(bool(a),"Native lookup Arena expired");
    const auto* const store=impl_->original_store;const auto store_size=impl_->cache.size();
    const std::weak_ptr<const BnwNativeCategoryNamespaceChannel> namespace_control=impl_->category_namespace;
    const auto source_fence=[&]{
        impl_->check(f);need(f.valid_parent(a),"Native lookup Arena persistent owner lost");
        need(store==&impl_->cache&&impl_->original_store==store&&impl_->cache.size()==store_size&&
             impl_->category_namespace&&!impl_->category_namespace.owner_before(namespace_control)&&
             !namespace_control.owner_before(impl_->category_namespace)&&
             !impl_->arena.owner_before(a)&&!a.owner_before(impl_->arena),
             "Owned archive lookup changed SAME cache store/Arena/namespace");
    };source_fence(); // The original facade guard/pure parent check precede PRE callbacks.
    const auto lookup=[&]{
        const auto check=[&]{impl_->check(f);need(f.valid_parent(a),"Native lookup Arena persistent owner lost");
            channel->require_active_request();
            need(impl_->category_namespace && channel->namespace_channel()==impl_->category_namespace,
                 "Category lookup has another namespace/cache");
            a->require_same_category_channel(*impl_->category_namespace);impl_->check(f);
            request->require_same_category_channel(*channel);impl_->check(f);};check();
        impl_->validate(f);check();auto cold=impl_->cold.lock();need(bool(cold),"Native lookup Cold expired");
        request->require_same_owner(*cold);check();need(f.valid_parent(cold),"Native lookup Cold persistent owner lost");
        const auto raw=request->raw_relative_ansi_request();check();const auto key=archive_cache_basename_ascii(raw);
        need(!key.empty(),"Finite primary archive basename empty");ascii_locale();
        auto i=std::make_unique<BnwNativeArchiveCacheLookupOwner::Impl>();i->cache=impl_->self;i->request=request;i->key=key;
        i->hit=impl_->cache.lookup(key)!=nullptr;
        auto out=std::shared_ptr<const BnwNativeArchiveCacheLookupOwner>(new BnwNativeArchiveCacheLookupOwner(std::move(i)));
        out->impl_->self=out;check();return out;
    };
    std::shared_ptr<const BnwNativeArchiveCacheLookupOwner> out;
    warm.with_owned_archive_lookup_common_scope(parent,*this,request,channel,source_fence,[&]{out=lookup();});
    return out;
}
std::shared_ptr<const BnwNativeArchiveCachePublicationOwner> BnwNativeArchiveBasenameCacheOwner::publish_category_after_entries(
    std::shared_ptr<const BnwNativeArchiveCacheLookupOwner> lookup,
    std::shared_ptr<const BnwNativeArchiveV6EntryAdmissionOwner> entries,
    std::shared_ptr<const BnwNativeCategoryArchiveChannel> channel)const {
    need(lookup&&entries&&channel,"Category cache publication requires genuine lookup/entries/channel");auto f=impl_->guard();
    const auto a=impl_->arena.lock();need(bool(a),"Native cache publication Arena expired");
    const auto check=[&]{impl_->check(f);need(f.valid_parent(a),"Native publication Arena lost");
        channel->require_active_request();
        need(impl_->category_namespace && channel->namespace_channel()==impl_->category_namespace,
             "Category publication has another namespace/cache");
        a->require_same_category_channel(*impl_->category_namespace);impl_->check(f);
        lookup->impl_->request->require_same_category_channel(*channel);impl_->check(f);};check();
    need(lookup->impl_->cache.lock().get()==this && !lookup->impl_->publication_attempted,
         "Native publication lookup has another cache/already consumed");
    lookup->impl_->publication_attempted=true; // BEFORE first validation callback.
    impl_->validate(f);check();
    // Do not call Lookup.current while holding its Cache fence (reentrant).
    lookup->impl_->request->require_current();check();need(!lookup->impl_->hit,"Native cache hit must skip constructor/publication");
    need(impl_->cache.lookup(lookup->impl_->key)==nullptr,"Native cache changed after pre-constructor miss");
    entries->require_same_arena(*a);check();
    const auto bound=entries->bound_archive_cache_sources();check();const auto& header=bound.first;const auto& prefix=bound.second;
    need(header&&prefix,"Native entry cache sources unavailable");
    const auto& request=header->source_request();check();need(&request==lookup->impl_->request.get(),"Native entry came from another lookup request");
    const auto& actual_header=prefix->header_owner();check();need(&actual_header==header.get(),"Native cache prefix has another Header");
    const auto count=entries->admitted_count();check();
    need(archive_cache_positive_loader_diagnostic(count),"Native loader result zero forbids archive cache insertion");
    const auto& construction=header->construction();check();const auto mtime=construction.checked_original_mtime();check();
    const auto canonical=construction.canonical_payload_source();check();
    auto i=std::make_unique<BnwNativeArchiveCachePublicationOwner::Impl>();i->cache=impl_->self;
    i->entries=entries;i->header=header;i->prefix=prefix;i->canonical=canonical;i->mtime=mtime;i->key=lookup->impl_->key;
    auto out=std::shared_ptr<const BnwNativeArchiveCachePublicationOwner>(new BnwNativeArchiveCachePublicationOwner(std::move(i)));
    out->impl_->self=out;
    auto staged=impl_->cache;need(staged.insert_unique(lookup->impl_->key,out),"Native82B590 existing key cannot overwrite");
    // All allocation above. Revalidate sources and actual parent fence after
    // LAST callback, then perform only noexcept source-store/brand writes.
    entries->require_current();check();header->require_current();check();prefix->require_current();check();ascii_locale();
    need(impl_->cache.lookup(lookup->impl_->key)==nullptr,"Native publication changed before store");check();
    impl_->cache.swap(staged);out->impl_->published=true;return out;
}
BnwNativeArchiveCacheLookupOwner::BnwNativeArchiveCacheLookupOwner(std::unique_ptr<Impl> i):impl_(std::move(i)){}
BnwNativeArchiveCacheLookupOwner::~BnwNativeArchiveCacheLookupOwner()=default;
void BnwNativeArchiveCacheLookupOwner::require_current()const{auto f=impl_->guard();impl_->validate(f);}
bool BnwNativeArchiveCacheLookupOwner::hit()const{auto f=impl_->guard();impl_->validate(f);return impl_->hit;}
bool BnwNativeArchiveCacheLookupOwner::hit_owned_category(const BnwNativeArchiveBasenameCacheOwner& cache,
    const std::shared_ptr<const BnwNativeColdArchiveRequest>& request,
    const std::shared_ptr<const BnwNativeCategoryArchiveChannel>& channel,
    const BnwNativeGlobalResourceWarmOwner& warm,const ::civ5::ui::BnwNativePackageDiscoveryOwner& parent)const {
    auto f=impl_->guard();
    // Original Lookup validation prefix: its nested Cache guard/FULL is not moved.
    need(f.valid(),"Native cache lookup facade lost/reentered/foreign thread");
    auto c=impl_->cache.lock();need(bool(c),"Native lookup cache expired");c->require_current();
    need(f.valid()&&f.valid_parent(c),"Native lookup persistent cache lost");
    const auto* const store=c->impl_->original_store;const auto store_size=c->impl_->cache.size();
    const auto source_fence=[&]{
        need(f.valid()&&f.valid_parent(c),"Native lookup persistent cache lost");
        need(c.get()==&cache&&request&&impl_->request.get()==request.get()&&
             !impl_->request.owner_before(request)&&!request.owner_before(impl_->request)&&
             store==&c->impl_->cache&&c->impl_->original_store==store&&c->impl_->cache.size()==store_size,
             "Owned archive hit lost SAME lookup/request/cache store");
    };source_fence();
    bool hit=false;
    warm.with_owned_archive_lookup_common_scope(parent,cache,request,channel,source_fence,[&]{
        impl_->request->require_current();
        need(f.valid()&&f.valid_parent(c),"Native lookup cache lost after request callback");
        hit=impl_->hit;
    });
    return hit; // A real lookup result, never a supplied completeness flag.
}

//One actual counted facade fence retains the published immutable source
//tuple. No additional Cache/Header/Entry/Prefix facade pins are introduced.
class BnwNativeArchiveCachePublicationOwner::Primary32ReadLoan final {
    const BnwNativeArchiveCachePublicationOwner& owner_;
    const Impl& i_;
    Impl::Guard fence_;
    const std::weak_ptr<const BnwNativeArchiveCachePublicationOwner> self_;
    const std::weak_ptr<const BnwNativeArchiveBasenameCacheOwner> cache_;
    const BnwNativeArchiveV6EntryAdmissionOwner* const entries_;
    const BnwNativeArchiveV6StreamHeaderOwner* const header_;
    const BnwNativeGlobalArchivePrefixRecord* const prefix_;
    const std::shared_ptr<const assets::FpkSource>& canonical_;
    const assets::FpkSource* const canonical_address_;
    const std::weak_ptr<const assets::FpkSource> canonical_control_;
    const std::int64_t mtime_;
    const std::string key_;
  public:
    explicit Primary32ReadLoan(const BnwNativeArchiveCachePublicationOwner& owner):
        owner_(owner),i_(*owner.impl_),fence_(i_.state,i_.self),
        self_(i_.self),cache_(i_.cache),entries_(i_.entries.get()),header_(i_.header.get()),
        prefix_(i_.prefix.get()),canonical_(i_.canonical),canonical_address_(i_.canonical.get()),
        canonical_control_(i_.canonical),mtime_(i_.mtime),key_(i_.key) {
        check_without_callbacks();
    }
    Primary32ReadLoan(const Primary32ReadLoan&)=delete;
    Primary32ReadLoan& operator=(const Primary32ReadLoan&)=delete;
    Primary32ReadLoan(Primary32ReadLoan&&)=delete;
    Primary32ReadLoan& operator=(Primary32ReadLoan&&)=delete;
    void check_without_callbacks()const {
        need(i_.creator_thread==std::this_thread::get_id()&&fence_.valid()&&i_.published,
             "Primary32 publication read loan lost/reentered/foreign/unpublished");
        need(!self_.owner_before(i_.self)&&!i_.self.owner_before(self_)&&
             !cache_.owner_before(i_.cache)&&!i_.cache.owner_before(cache_)&&
             entries_==i_.entries.get()&&header_==i_.header.get()&&prefix_==i_.prefix.get()&&
             canonical_address_==canonical_.get()&&!canonical_control_.owner_before(canonical_)&&
             !canonical_.owner_before(canonical_control_)&&mtime_==i_.mtime&&key_==i_.key,
             "Primary32 publication immutable actual source tuple changed");
    }
    void full_current()const {
        check_without_callbacks();i_.validate(fence_,owner_);check_without_callbacks();
    }
    void require_same_time(std::int64_t actual)const {
        check_without_callbacks();need(actual==mtime_,"Primary32 original cached time differs");
    }
    void require_same_payload(const std::shared_ptr<const assets::FpkSource>& actual)const {
        check_without_callbacks();
        need(actual.get()==canonical_address_&&!actual.owner_before(canonical_control_)&&
             !canonical_control_.owner_before(actual),"Primary32 canonical source identity/control differs");
    }
};
void BnwNativeArchiveCachePublicationOwner::require_primary32_construction_fields(
    const BnwNativeArchiveConstruction& construction,const std::function<void()>& input)const {
    need(impl_->creator_thread==std::this_thread::get_id(),"Primary32 publication borrower foreign creator thread");
    Primary32ReadLoan loan(*this);
    input();loan.check_without_callbacks();
    loan.full_current();loan.check_without_callbacks();
    {
        //COPY/invoke/destruction occur under the SAME actual facade loan.
        //Only the private Parent's callback-free retained-step controller is supplied.
        const auto check=input;loan.check_without_callbacks();check();loan.check_without_callbacks();
        const auto actual_time=construction.checked_original_mtime();check();loan.check_without_callbacks();
        loan.require_same_time(actual_time);
        const auto actual_payload=construction.canonical_payload_source();check();loan.check_without_callbacks();
        loan.require_same_payload(actual_payload);
        check();loan.check_without_callbacks();
    } //Private copied controller dies BEFORE full source/file POST validation.
    loan.check_without_callbacks();input();loan.check_without_callbacks();
    loan.full_current();input();loan.check_without_callbacks();
}
BnwNativeArchiveCachePublicationOwner::BnwNativeArchiveCachePublicationOwner(std::unique_ptr<Impl> i):impl_(std::move(i)){}
BnwNativeArchiveCachePublicationOwner::~BnwNativeArchiveCachePublicationOwner()=default;
void BnwNativeArchiveCachePublicationOwner::require_common_source_current(BnwNativeCommonSourceValidationContext& context)const {
    auto fence=impl_->guard();need(fence.valid(),"Common cache facade lost/reentered/foreign thread");
    auto visit=context.enter(*this,impl_->self);
    impl_->validate_common(fence,*this,context);
    need(fence.valid(),"Common cache facade changed");visit.complete();
}
void BnwNativeArchiveCachePublicationOwner::require_current()const{auto f=impl_->guard();impl_->validate(f,*this);}
void BnwNativeArchiveCachePublicationOwner::require_default_projection_payload_identity_without_callbacks(
    const BnwNativeArchiveV6StreamHeaderOwner& header)const {
    // Header/canonical links are fixed before the actual cache store. No
    // callback or allocation separates Root's FULL validation and this read.
    need(impl_->header.get()==&header,"Default projection publication has foreign Header");
    header.require_constructed_payload_identity_without_callbacks(impl_->canonical.get());
}
void BnwNativeArchiveCachePublicationOwner::require_primary32_construction_identity_without_callbacks(
    const BnwNativeArchiveConstruction& construction)const {
    // Published fields are fixed before the real cache store. This void check
    // neither returns payload storage nor acquires any Parent/facade pin.
    construction.require_primary32_cached_fields_identity_without_callbacks(impl_->mtime,impl_->canonical.get());
}
std::int64_t BnwNativeArchiveCachePublicationOwner::checked_original_mtime()const{auto f=impl_->guard();impl_->validate_retained_mtime(f,*this);return impl_->mtime;}
bool BnwNativeArchiveCachePublicationOwner::checked_preference280()const {
    auto fence=impl_->guard();impl_->validate(fence,*this);
    const auto result=impl_->header->construction().checked_preference280();
    impl_->validate(fence,*this);return result;
}
void BnwNativeArchiveCachePublicationOwner::disable_preference280_from_category(
    const BnwNativeCategoryNamespaceChannel& channel)const {
    channel.require_current();auto fence=impl_->guard();impl_->validate(fence,*this);
    const auto cache=impl_->cache.lock();need(bool(cache),"Native preference publication cache retired");
    need(cache->impl_->category_namespace.get()==&channel,
         "Native67B4E0 selected publication has another actual namespace cache");
    const auto selected=cache->impl_->cache.lookup(impl_->key);
    need(selected&&selected->get()==this,"Native67B4E0 cache key no longer selects this actual archive");
    impl_->header->construction().disable_preference280_from_cached_category(*impl_->header,channel);
    impl_->validate(fence,*this);need(fence.valid_parent(cache),"Native preference actual persistent cache retired");
}
std::shared_ptr<const assets::FpkSource> BnwNativeArchiveCachePublicationOwner::canonical_payload_source()const{
    auto f=impl_->guard();impl_->validate(f,*this);return impl_->canonical;
}

void BnwNativeArchiveBasenameCacheOwner::require_current_completed(const BnwNativeCompletedColdSourceLoan& loan,
    const BnwNativeColdFilesystemOwner& cold,const BnwNativeArchiveEntryAdmissionArena& arena)const {
    auto fence=impl_->guard();impl_->validate_completed(fence,loan,cold,arena);
}
void BnwNativeArchiveBasenameCacheOwner::require_same_arena_completed(const BnwNativeArchiveEntryAdmissionArena& arena,
    const BnwNativeCompletedColdSourceLoan& loan,const BnwNativeColdFilesystemOwner& cold)const {
    require_current_completed(loan,cold,arena);
}
void BnwNativeArchiveBasenameCacheOwner::require_same_completed_owner_control(
    const std::weak_ptr<const BnwNativeArchiveBasenameCacheOwner>& expected,const BnwNativeCompletedColdSourceLoan& loan,
    const BnwNativeColdFilesystemOwner& cold,const BnwNativeArchiveEntryAdmissionArena& arena)const {
    loan.require_same_cold(cold);auto fence=impl_->guard();impl_->validate_completed(fence,loan,cold,arena);
    need(!expected.expired()&&!expected.owner_before(impl_->self)&&!impl_->self.owner_before(expected),
        "Completed cache has another original factory control");impl_->validate_completed(fence,loan,cold,arena);
}
void BnwNativeArchiveBasenameCacheOwner::require_current_lookup_completed(const BnwNativeArchiveCacheLookupOwner& lookup,
    const BnwNativeCompletedColdSourceLoan& loan,const BnwNativeColdFilesystemOwner& cold,
    const BnwNativeArchiveEntryAdmissionArena& arena)const {
    auto fence=impl_->guard();impl_->validate_completed(fence,loan,cold,arena);
    const auto& value=*lookup.impl_;
    need(!value.cache.expired()&&!value.cache.owner_before(impl_->self)&&!impl_->self.owner_before(value.cache)&&value.request,
        "Completed lookup has another actual cache/control/request");
    // THIS immutable Lookup retains Request through descriptor target lifetime.
    value.request->with_current_completed_descriptor(loan,[&](const auto& descriptor){
        descriptor.require_current();impl_->check(fence);
        need(value.key==archive_cache_basename_ascii(descriptor.raw_relative_ansi_request()),
            "Completed lookup key differs from actual original source request");descriptor.require_current();impl_->check(fence);
    });impl_->validate_completed(fence,loan,cold,arena);
}
void BnwNativeArchiveCacheLookupOwner::require_current_completed(const BnwNativeCompletedColdSourceLoan& loan,
    const BnwNativeColdFilesystemOwner& cold,const BnwNativeArchiveEntryAdmissionArena& arena,
    const BnwNativeArchiveBasenameCacheOwner& cache)const {
    auto fence=impl_->guard();need(fence.valid(),"Completed lookup facade lost/reentered/foreign thread");
    cache.require_current_lookup_completed(*this,loan,cold,arena);
    need(fence.valid(),"Completed lookup facade lost after source checks");
}
bool BnwNativeArchiveCacheLookupOwner::hit_completed(const BnwNativeCompletedColdSourceLoan& loan,
    const BnwNativeColdFilesystemOwner& cold,const BnwNativeArchiveEntryAdmissionArena& arena,
    const BnwNativeArchiveBasenameCacheOwner& cache)const {
    auto fence=impl_->guard();need(fence.valid(),"Completed lookup facade lost/reentered/foreign thread");
    cache.require_current_lookup_completed(*this,loan,cold,arena);
    need(fence.valid(),"Completed lookup facade lost after source checks");return impl_->hit;
}
void BnwNativeArchiveCachePublicationOwner::require_current_completed(const BnwNativeCompletedColdSourceLoan& loan,
    const BnwNativeColdFilesystemOwner& cold,const BnwNativeArchiveEntryAdmissionArena& arena,
    const BnwNativeGlobalArchivePrefixOwner& registry,const BnwNativeArchiveBasenameCacheOwner& cache)const {
    auto fence=impl_->guard();impl_->validate_completed(fence,*this,loan,cold,arena,registry,cache);
}
std::int64_t BnwNativeArchiveCachePublicationOwner::checked_original_mtime_completed(const BnwNativeCompletedColdSourceLoan& loan,
    const BnwNativeColdFilesystemOwner& cold,const BnwNativeArchiveEntryAdmissionArena& arena,
    const BnwNativeGlobalArchivePrefixOwner& registry,const BnwNativeArchiveBasenameCacheOwner& cache)const {
    auto fence=impl_->guard();impl_->validate_completed(fence,*this,loan,cold,arena,registry,cache);return impl_->mtime;
}
std::shared_ptr<const assets::FpkSource> BnwNativeArchiveCachePublicationOwner::canonical_payload_source_completed(const BnwNativeCompletedColdSourceLoan& loan,
    const BnwNativeColdFilesystemOwner& cold,const BnwNativeArchiveEntryAdmissionArena& arena,
    const BnwNativeGlobalArchivePrefixOwner& registry,const BnwNativeArchiveBasenameCacheOwner& cache)const {
    auto fence=impl_->guard();impl_->validate_completed(fence,*this,loan,cold,arena,registry,cache);return impl_->canonical;
}
bool BnwNativeArchiveCachePublicationOwner::checked_preference280_completed(const BnwNativeCompletedColdSourceLoan& loan,
    const BnwNativeColdFilesystemOwner& cold,const BnwNativeArchiveEntryAdmissionArena& arena,
    const BnwNativeGlobalArchivePrefixOwner& registry,const BnwNativeArchiveBasenameCacheOwner& cache)const {
    auto fence=impl_->guard();impl_->validate_completed(fence,*this,loan,cold,arena,registry,cache);
    const auto result=impl_->header->construction_completed(loan,cold).checked_preference280_completed(loan);
    impl_->validate_completed(fence,*this,loan,cold,arena,registry,cache);return result;
}

void BnwNativeArchiveBasenameCacheOwner::require_same_cached_publication_identity_without_callbacks(
    const BnwNativeArchiveCachePublicationOwner& publication)const {
    // No new Guard: a genuine completed Cache validator may already hold it.
    // Check immutable creator identities before reading either mutable store.
    need(std::this_thread::get_id()==impl_->creator_thread,
        "Completed cached publication identity on foreign Cache creator thread");
    need(std::this_thread::get_id()==publication.impl_->creator_thread,
        "Completed cached publication identity on foreign Publication creator thread");
    const auto& actual=*impl_;const auto& value=*publication.impl_;
    need(!actual.self.expired()&&actual.original_store==&actual.cache,
        "Completed cached publication lost original Cache factory/store identity");
    need(value.published&&!value.self.expired()&&!value.cache.expired()&&
        !value.cache.owner_before(actual.self)&&!actual.self.owner_before(value.cache),
        "Completed cached publication has another original Cache factory control");
    const auto* selected=actual.cache.lookup(value.key);
    need(selected&&selected->get()==&publication&&!selected->owner_before(value.self)&&
        !value.self.owner_before(*selected),
        "Completed original cache key selects another Publication pointer/factory control");
}
}
