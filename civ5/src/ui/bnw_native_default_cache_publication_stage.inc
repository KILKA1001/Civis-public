// SAME default entry receipt owns this operation through the cache staging
// target's last destructor. No Header/Prefix public accessor is contracted.
struct BnwNativeArchiveV6EntryAdmissionOwner::DefaultCachePublicationStagingLoan::State final {
    const BnwNativeArchiveV6EntryAdmissionOwner& owner;
    const DefaultCachePublicationStagingLoan& loan;
    Impl& i;
    const BnwNativeArchiveEntryAdmissionArena& arena;
    const BnwNativeColdArchiveRequest& request;
    const std::function<void()>& controller;
    const std::thread::id creator=std::this_thread::get_id();
    const std::weak_ptr<const BnwNativeArchiveV6EntryAdmissionOwner> self;
    const std::weak_ptr<const BnwNativeArchiveEntryAdmissionArena> arena_control;
    const BnwNativeArchiveV6StreamHeaderOwner* const header_address;
    const BnwNativeGlobalArchivePrefixRecord* const prefix_address;
    const std::weak_ptr<const BnwNativeArchiveV6StreamHeaderOwner> header_control;
    const std::weak_ptr<const BnwNativeGlobalArchivePrefixRecord> prefix_control;
    const assets::FpkSource* const source_address;
    const std::weak_ptr<const assets::FpkSource> source_control;
    const std::uint32_t declared,admitted;
    const std::uint32_t* const return_address;
    const std::size_t return_count,ordinal;
    const Impl::Guard& fence; // Actual caller-owned Entry guard/pin, entered before allocation.
    std::uint64_t generation=0;
    bool registered=false,complete=false;
    std::optional<DefaultCachePublicationInputs> inputs;
    static Impl& guarded_impl(const BnwNativeArchiveV6EntryAdmissionOwner& facade,const Impl::Guard& held) {
        need(held.valid(),"Default cache staging Entry retired during State allocation");
        return *facade.impl_; // FIRST after allocation, before any State captures.
    }
    State(const BnwNativeArchiveV6EntryAdmissionOwner& facade,const DefaultCachePublicationStagingLoan& frame,
        const BnwNativeArchiveEntryAdmissionArena& actual_arena,const BnwNativeColdArchiveRequest& actual_request,
        const std::function<void()>& actual_controller,const Impl::Guard& held)
        :owner(facade),loan(frame),i(guarded_impl(facade,held)),arena(actual_arena),request(actual_request),controller(actual_controller),
         self(i.self),arena_control(i.arena),header_address(i.header.get()),prefix_address(i.prefix.get()),
         header_control(i.header),prefix_control(i.prefix),source_address(i.source.get()),source_control(i.source),
         declared(i.declared),admitted(i.admitted),return_address(i.entry_returns.data()),return_count(i.entry_returns.size()),
         ordinal(i.ordinal),fence(held) {
        need(fence.valid()&&controller&&!i.default_cache_stage_failed&&!i.active_default_cache_stage&&
             i.default_cache_stage_epoch!=UINT64_MAX,"Default cache staging receipt occupied/retired");
        generation=++i.default_cache_stage_epoch;i.active_default_cache_stage=&loan;registered=true;
        try {
            check();full(); // Original metadata entry FULL PRE.
            need(i.header&&i.prefix,"Native entry cache sources unavailable");
            need(admitted!=0,"Native loader result zero forbids archive cache insertion");
            const auto metadata=i.header->cache_publication_metadata(request,admitted,controller);check();
            full(); // Original metadata entry FULL POST, before fields/copies/allocation target.
            inputs.emplace(DefaultCachePublicationInputs{i.header,i.prefix,std::get<0>(metadata),admitted,
                std::get<1>(metadata),std::get<2>(metadata)});check();
            need(inputs->request==&request&&inputs->header.get()==header_address&&inputs->prefix.get()==prefix_address,
                 "Default cache staging metadata belongs to another actual source");
        }catch(...){i.default_cache_stage_failed=true;retire();throw;}
    }
    void identity()const {
        need(creator==std::this_thread::get_id()&&fence.valid(),"Default cache staging loan foreign/retired/reentered");
        need(registered&&!i.default_cache_stage_failed&&i.active_default_cache_stage==&loan&&
             generation&&i.default_cache_stage_epoch==generation&&!self.owner_before(i.self)&&!i.self.owner_before(self)&&
             !arena_control.owner_before(i.arena)&&!i.arena.owner_before(arena_control)&&
             i.header.get()==header_address&&!header_control.owner_before(i.header)&&!i.header.owner_before(header_control)&&
             i.prefix.get()==prefix_address&&!prefix_control.owner_before(i.prefix)&&!i.prefix.owner_before(prefix_control)&&
             i.source.get()==source_address&&!source_control.owner_before(i.source)&&!i.source.owner_before(source_control)&&
             i.declared==declared&&i.admitted==admitted&&i.ordinal==ordinal&&
             i.entry_returns.data()==return_address&&i.entry_returns.size()==return_count,
             "Default cache staging changed actual receipt/source/control/generation");
        const auto actual=i.arena.lock();
        need(actual&&actual.get()==&arena&&fence.valid_parent(actual),"Default cache staging lost outside Arena");
    }
    void check()const {
        identity();controller();identity(); // Actual private Parent/cache controller; never a SourceBool.
    }
    void full()const {
        check();
        // The returned Arena pin retires at this full-expression boundary.
        need(i.validate(fence).get()==&arena,"Native admission receipt has another Arena");check();
    }
    void finish() {
        check();full(); // Original postallocation Entry FULL, AFTER staging target destruction.
        need(inputs&&inputs->request==&request&&inputs->admitted==admitted&&
             inputs->header.get()==header_address&&inputs->prefix.get()==prefix_address,
             "Default cache staging changed its actual publication metadata");
        check();complete=true;
    }
    void retire()noexcept {
        if(registered){
            if(i.active_default_cache_stage!=&loan||i.default_cache_stage_epoch!=generation)i.default_cache_stage_failed=true;
            if(i.active_default_cache_stage==&loan)i.active_default_cache_stage=nullptr;
            registered=false;
        }
    }
    ~State(){if(!complete)i.default_cache_stage_failed=true;retire();}
};
BnwNativeArchiveV6EntryAdmissionOwner::DefaultCachePublicationStagingLoan::DefaultCachePublicationStagingLoan(
    const BnwNativeArchiveV6EntryAdmissionOwner& owner,const BnwNativeArchiveEntryAdmissionArena& arena,
    const BnwNativeColdArchiveRequest& request,const std::function<void()>& controller,
    const BnwNativeCallbackOwnerGuard<BnwNativeArchiveV6EntryAdmissionOwner>& held)
    :state_(std::make_unique<State>(owner,*this,arena,request,controller,held)){}
BnwNativeArchiveV6EntryAdmissionOwner::DefaultCachePublicationStagingLoan::~DefaultCachePublicationStagingLoan()=default;
const BnwNativeArchiveV6EntryAdmissionOwner::DefaultCachePublicationInputs&
BnwNativeArchiveV6EntryAdmissionOwner::DefaultCachePublicationStagingLoan::inputs_without_callbacks()const {
    state_->check();need(bool(state_->inputs),"Default cache staging actual metadata return absent");return *state_->inputs;
}
void BnwNativeArchiveV6EntryAdmissionOwner::DefaultCachePublicationStagingLoan::require_current_without_callbacks()const {
    state_->check();
}
void BnwNativeArchiveV6EntryAdmissionOwner::with_default_cache_publication_stage(
    const BnwNativeArchiveEntryAdmissionArena& arena,const BnwNativeColdArchiveRequest& request,
    const std::function<void()>& controller,
    std::function<void(const DefaultCachePublicationStagingLoan&)> input)const {
    // Foreign entry is rejected before try/poison and before State allocation/captures.
    // This one canonical Entry pin retains the owner through every exception,
    // transferred target DTOR, final FULL and actual State retirement.
    auto fence=impl_->guard();need(fence.valid(),"Default cache staging Entry foreign/retired/reentered");
    try {
        DefaultCachePublicationStagingLoan loan(*this,arena,request,controller,fence);
        loan.require_current_without_callbacks();
        {
            const auto consume=std::move(input);loan.require_current_without_callbacks();
            need(bool(consume),"Default cache staging actual consumer absent");
            consume(loan);loan.require_current_without_callbacks();
        } // Entire transferred staging target dies BEFORE final real Entry FULL; parameter is now empty.
        loan.state_->finish();
    }catch(...){impl_->default_cache_stage_failed=true;throw;}
}
void BnwNativeArchiveV6EntryAdmissionOwner::require_default_cache_stage_retired_without_callbacks()const {
    auto fence=impl_->guard();
    need(fence.valid()&&!impl_->default_cache_stage_failed&&!impl_->active_default_cache_stage,
         "Default cache staging failed or did not retire its genuine owner loan");
}
