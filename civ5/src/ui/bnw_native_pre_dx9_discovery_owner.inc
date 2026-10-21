// Genuine initial namespace and actual per-filename pipeline; no early Warm.
struct detail::BnwNativePreDx9DiscoveryOwner::Impl {
    const std::thread::id thread=std::this_thread::get_id();
    const std::weak_ptr<const BnwNativePackageDiscoveryOwner> parent;
    const std::shared_ptr<const BnwNativeInstallationInputOwner> input;
    const std::shared_ptr<DefaultState> initial_default;
    const assets::ResourceMountTable* const initial_table;
    const std::weak_ptr<const void> epoch;
    std::weak_ptr<BnwNativePreDx9DiscoveryOwner> self;
    std::atomic<bool> closed{false},failed{false},checking{false},busy{false};
    mutable std::size_t parent_pins=0;
    enum class Phase {unissued,enumerating,constructing,returned,adopted};
    Phase phase=Phase::unissued;
    std::shared_ptr<BnwNativeResourceDiscoveryActor> actor;
    std::weak_ptr<BnwNativeResourceDiscoveryActor> adopted_actor;
    std::weak_ptr<BnwNativeGlobalResourceWarmOwner> consumer;
    std::vector<std::shared_ptr<BnwNativePackageLoadActor>> receivers;
    std::vector<std::shared_ptr<BnwNativePackageDescriptorByteReader::Source>> streams;
    std::vector<std::shared_ptr<const BnwNativeStandalonePackageLoaded>> returned;
    std::vector<std::weak_ptr<const BnwNativePackageLoadActor>> receiver_identities;
    std::shared_ptr<const runtime::BnwSelectedLanguageInputOwner> language;
    const BnwNativePackageLoadActor* active_metadata_receiver=nullptr;
    Impl(const std::shared_ptr<const BnwNativePackageDiscoveryOwner>& actual,
        const std::shared_ptr<const BnwNativeInstallationInputOwner>& installation,
        const std::shared_ptr<DefaultState>& source,const assets::ResourceMountTable& table,
        const std::shared_ptr<const void>& identity)
        :parent(actual),input(installation),initial_default(source),initial_table(&table),epoch(identity){}
};
struct detail::BnwNativePreDx9DiscoveryOwner::ParentPin {
    const BnwNativePreDx9DiscoveryOwner& owner;
    const std::shared_ptr<const BnwNativePackageDiscoveryOwner> parent;
    static std::shared_ptr<const BnwNativePackageDiscoveryOwner> lock(const BnwNativePreDx9DiscoveryOwner& actual) {
        need(actual.impl_->thread==std::this_thread::get_id(),"Pre-DX9 Parent loan foreign creator");
        auto parent=actual.impl_->parent.lock();need(bool(parent),"Pre-DX9 initial Parent retired");
        need(parent->impl_->thread==std::this_thread::get_id(),"Pre-DX9 Parent foreign creator");return parent;
    }
    explicit ParentPin(const BnwNativePreDx9DiscoveryOwner& actual):owner(actual),parent(lock(actual)) {
        ++owner.impl_->parent_pins;
        try {parent->require_pre_dx9_discovery_current(owner);}
        catch(...){--owner.impl_->parent_pins;throw;}
    }
    ~ParentPin(){--owner.impl_->parent_pins;}
};
detail::BnwNativePreDx9DiscoveryOwner::BnwNativePreDx9DiscoveryOwner(std::unique_ptr<Impl> source)
    :impl_(std::move(source)){}
detail::BnwNativePreDx9DiscoveryOwner::~BnwNativePreDx9DiscoveryOwner(){impl_->closed.store(true);}
void detail::BnwNativePreDx9DiscoveryOwner::fail()const noexcept {impl_->failed.store(true);}
void BnwNativePackageDiscoveryOwner::require_pre_dx9_discovery_current(
    const detail::BnwNativePreDx9DiscoveryOwner& actual)const {
    const auto& b=*actual.impl_;const auto& i=*impl_;
    need(b.thread==std::this_thread::get_id()&&i.thread==b.thread,
         "Pre-DX9 namespace foreign creator thread");
    const auto same=[](const auto& first,const auto& second){
        return !first.owner_before(second)&&!second.owner_before(first);
    };
    need(!b.closed.load()&&!b.failed.load()&&!i.header_poisoned&&!i.category_lineage_failed&&
         i.pre_dx9_discovery.get()==&actual&&same(i.pre_dx9_discovery,b.self)&&
         same(i.installation_input,b.input)&&same(i.self,b.parent)&&same(i.identity,b.epoch)&&
         i.default_state.get()==b.initial_default.get()&&same(i.default_state,b.initial_default)&&
         &i.resources==b.initial_table,
         "Pre-DX9 original installation/Parent/Default controls retired or replaced");
    std::size_t internal=b.parent_pins+i.directory_parent_internal_pin_count()+
        i.header_callback_state.internal_owner_pin_count();
    if(i.live_resource_query)internal+=i.live_resource_query->parent_pins_;
    if(i.category_source)internal+=i.category_source->native_query_parent_pins_;
    if(i.global_color_bank&&i.global_color_bank->owned_source_)internal+=i.global_color_bank->internal_package_pins_;
    need(i.self.use_count()>static_cast<long>(internal),"Pre-DX9 lost last outside startup Parent owner");
}
void detail::BnwNativePreDx9DiscoveryOwner::require_current()const {
    need(impl_->thread==std::this_thread::get_id(),"Pre-DX9 source foreign creator");
    if(impl_->checking.exchange(true)){fail();throw std::runtime_error("Pre-DX9 source callback reentry");}
    struct Checking{Impl& i;~Checking(){i.checking.store(false);}} checking{*impl_};
    try {
        ParentPin retained(*this);impl_->input->require_current();
        retained.parent->require_pre_dx9_discovery_current(*this);
    }catch(...){fail();throw;}
}
void detail::BnwNativePreDx9DiscoveryOwner::require_source_identity(const void* source,std::size_t ordinal)const {
    require_current();const auto actor=impl_->actor?impl_->actor:impl_->adopted_actor.lock();
    need(actor&&actor->state_->origin==BnwNativeResourceDiscoveryActor::State::Origin::installation_pre_dx9&&
         !actor->state_->pre_dx9.owner_before(impl_->self)&&!impl_->self.owner_before(actor->state_->pre_dx9)&&
         ordinal<actor->state_->issued.size()&&actor->state_->issued[ordinal].get()==source,
         "Initial filename is not SAME actual ordered installation occurrence");
    actor->state_->live();
    if(impl_->phase==Impl::Phase::adopted){const auto warm=impl_->consumer.lock();
        need(bool(warm),"Initial Discovery lost its actual adopted Warm owner");
        require_adopted_factory_identity_without_callbacks(*warm);}
}
std::filesystem::path detail::BnwNativePreDx9DiscoveryOwner::game_root_for_language_source()const {
    require_current();ParentPin retained(*this);return retained.parent->impl_->original.parent_path();
}
void detail::BnwNativePreDx9DiscoveryOwner::require_core_platform_capture()const {
    require_current();const auto actor=impl_->active_metadata_receiver;
    need(impl_->phase==Impl::Phase::constructing&&impl_->busy.load()&&actor&&
         !impl_->receivers.empty()&&impl_->receivers.back().get()==actor&&actor->state_->busy.load()&&
         actor->state_->descriptor_bytes&&actor->state_->parsed_xml&&!actor->state_->descriptor_core&&
         actor->state_->pre_dx9_lookup_source,"Initial CRT source not minted by actual parsed-receiver Core call");
}
void detail::BnwNativePreDx9DiscoveryOwner::require_metadata_language_observation()const {
    require_current();const auto actor=impl_->active_metadata_receiver;
    need(impl_->phase==Impl::Phase::constructing&&impl_->busy.load()&&actor&&
         !impl_->receivers.empty()&&impl_->receivers.back().get()==actor&&actor->state_->busy.load()&&
         actor->state_->descriptor_core&&actor->state_->metadata_input&&
         actor->state_->metadata_input.get()==impl_->language.get()&&!actor->state_->completed_metadata_identity,
         "Initial language observation outside actual890EC0 metadata call");
}
std::shared_ptr<const runtime::BnwSelectedLanguageInputOwner>
detail::BnwNativePreDx9DiscoveryOwner::borrow_core_platform_input(const BnwNativePackageLoadActor& actor) {
    require_core_platform_capture();need(impl_->active_metadata_receiver==&actor,"Initial platform source belongs to another Core call");
    if(!impl_->language)impl_->language=runtime::BnwSelectedLanguageInputOwner::capture_from_initial_package_core(impl_->self.lock());
    impl_->language->require_current();require_core_platform_capture();return impl_->language;
}
void detail::BnwNativePreDx9DiscoveryOwner::require_same_metadata_input(const std::shared_ptr<const runtime::BnwSelectedLanguageInputOwner>& input)const {
    require_current();need(input&&impl_->language&&impl_->language.get()==input.get()&&
         !impl_->language.owner_before(input)&&!input.owner_before(impl_->language),
         "Initial metadata has another actual process/CRT source/control");
    input->require_current();require_current();
}
void detail::BnwNativePreDx9DiscoveryOwner::require_actual_boot_consumer(const BnwNativeResourceBootSettingsOwner& boot)const {
    require_current();ParentPin retained(*this);const auto& i=*retained.parent->impl_;
    need(impl_->phase==Impl::Phase::returned||impl_->phase==Impl::Phase::adopted,"Initial metadata has not returned before actual Boot");
    // This authenticates the real closed Source23 factory object, including its
    // self control, installation Parent and table. Do not borrow the completed
    // UserOptions route here: language is consumed DURING that actual creation.
    boot.require_installation_parent_without_callbacks(*retained.parent,i.self,i.resources);
}
void detail::BnwNativePreDx9DiscoveryOwner::require_completed_cold(const BnwNativeCompletedColdSourceLoan& loan,
    const BnwNativeColdFilesystemOwner& cold)const {
    require_current();ParentPin retained(*this);need(impl_->phase==Impl::Phase::adopted&&retained.parent->impl_->cold.get()==&cold,
        "Completed initial source has another actual namespace Cold owner");
    loan.require_same_cold(cold);require_current();
}
void detail::BnwNativePreDx9DiscoveryOwner::require_descriptor_source_identity(const BnwNativePackageLoadActor& actor,const void* source)const {
    require_current();
    if(impl_->phase==Impl::Phase::adopted){
        const auto warm=impl_->consumer.lock();need(bool(warm),"Initial stream Warm consumer retired");
        require_adopted_warm_identity_without_callbacks(*warm,actor.state_->actor);
        bool found=false;
        for(std::size_t n=0;n<warm->preload_receivers_.size();++n)
            if(warm->preload_receivers_[n].get()==&actor&&n<warm->descriptor_sources_.size()&&warm->descriptor_sources_[n].get()==source)found=true;
        need(found,"Initial stream is not SAME moved receiver/source pair");return;
    }
    bool found=false;
    for(std::size_t n=0;n<impl_->receivers.size();++n)
        if(impl_->receivers[n].get()==&actor&&n<impl_->streams.size()&&impl_->streams[n].get()==source)found=true;
    need(found,"Initial stream is not actual issuer-owned receiver/source pair");
}
void detail::BnwNativePreDx9DiscoveryOwner::require_returned_package_current(const BnwNativeStandalonePackageLoaded& token)const {
    require_current();need(impl_->phase!=Impl::Phase::adopted,"Initial token must use actual Warm membership after adoption");
    const auto issued=std::find_if(impl_->returned.begin(),impl_->returned.end(),[&](const auto& item){
        return item.get()==&token&&!item.owner_before(token.self_)&&!token.self_.owner_before(item);});
    need(issued!=impl_->returned.end(),"Initial token has no actual successful writer return");
    const auto actor=std::find_if(impl_->receivers.begin(),impl_->receivers.end(),[&](const auto& item){
        return !item.owner_before(token.actor_)&&!token.actor_.owner_before(item)&&
            item->state_->receiver.get()==token.actual_receiver_&&item->state_->completed_metadata_identity==token.actual_metadata_&&
            !item->state_->returned_loaded.owner_before(*issued)&&!issued->owner_before(item->state_->returned_loaded);});
    need(actor!=impl_->receivers.end(),"Initial metadata lost actual retained receiver/Core/return control");
    (*actor)->state_->current();require_current();
}
void detail::BnwNativePreDx9DiscoveryOwner::require_adopted_factory_identity_without_callbacks(
    const BnwNativeGlobalResourceWarmOwner& warm)const {
    need(impl_->thread==std::this_thread::get_id()&&warm.thread_==impl_->thread,
         "Initial adopted consumer identity foreign creator");
    const auto actual_consumer=impl_->consumer.lock();
    need(!impl_->closed.load()&&!impl_->failed.load()&&impl_->phase==Impl::Phase::adopted&&
         actual_consumer&&actual_consumer.get()==&warm&&warm.pre_dx9_owner_.get()==this&&
         !warm.closed_.load()&&!warm.failed_.load(),
         "Initial receiver lost its SAME actual Warm adoption");
    const auto discovery=impl_->adopted_actor.lock();
    need(discovery&&warm.discovery_actor_.get()==discovery.get()&&
         !warm.discovery_actor_.owner_before(discovery)&&!discovery.owner_before(warm.discovery_actor_)&&
         discovery->state_->origin==BnwNativeResourceDiscoveryActor::State::Origin::installation_pre_dx9&&
         !discovery->state_->pre_dx9.owner_before(impl_->self)&&!impl_->self.owner_before(discovery->state_->pre_dx9),
         "Initial consumer lost SAME actual moved Discovery factory/control");
    discovery->state_->live();
}
void detail::BnwNativePreDx9DiscoveryOwner::require_adopted_warm_identity_without_callbacks(
    const BnwNativeGlobalResourceWarmOwner& warm,const std::weak_ptr<const BnwNativePackageLoadActor>& actor)const {
    require_adopted_factory_identity_without_callbacks(warm);
    bool remembered=false,owned=false;
    for(const auto& item:impl_->receiver_identities)if(!item.owner_before(actor)&&!actor.owner_before(item))remembered=true;
    for(const auto& item:warm.preload_receivers_)if(item&&!item.owner_before(actor)&&!actor.owner_before(item))owned=true;
    need(remembered&&owned,"Initial receiver is not SAME actually moved factory object/control");
}
void detail::BnwNativePreDx9DiscoveryOwner::require_initial_default_miss(const BnwNativePackageLoadActor& receiver)const {
    require_current();ParentPin retained(*this);const auto& i=*retained.parent->impl_;
    const auto& state=receiver.state_;
    const auto check=[&]{
        retained.parent->require_pre_dx9_discovery_current(*this);
        need(impl_->phase==Impl::Phase::constructing&&impl_->busy.load()&&impl_->actor&&
             !impl_->receivers.empty()&&impl_->receivers.back().get()==&receiver&&
             !state->discovery.owner_before(impl_->actor)&&!impl_->actor.owner_before(state->discovery)&&
             !state->actor.owner_before(impl_->receivers.back())&&!impl_->receivers.back().owner_before(state->actor)&&
             state->receiver&&!state->actual_lookup_source&&!state->adopted_warm&&
             (!state->pre_dx9_lookup_source||(!state->pre_dx9_lookup_source->owner_before(impl_->self)&&!impl_->self.owner_before(*state->pre_dx9_lookup_source)))&&
             !state->descriptor_bytes&&!state->parsed_xml&&!state->descriptor_core,
             "Initial498940 lookup not from this actual new receiver/field0 occurrence");
        need(i.installation_input&&!i.lease&&!i.rules&&!i.header_attempted&&!i.header_active&&
             !i.primary32_attempted&&!i.installation_primary32_attempted&&!i.production_default_published&&
             !i.frame.primary_dx9_registration_complete&&!i.common_driver_attempted&&!i.live_resource_query&&
             !i.category_source&&!i.global_color_bank&&i.category_namespaces.empty()&&
             i.default_state->live_enabled89==1&&i.default_state->selected.records.empty()&&
             i.default_state->loose_native.empty()&&i.resources.mounts().empty()&&
             !i.cold&&!i.entry_arena&&!i.archive_cache&&!i.current_default_source,
             "Initial498940 lookup escaped actual empty Default/closed stack before Primary32");
    };
    check();const auto input=state->receiver->copied_input_for_498940_global_lookup();check();
    const auto query=detail::bnw_default_namespace_ascii_query_diagnostic(std::u16string(input.begin(),input.end()));check();
    const auto selected=i.default_state->selected.records.find(query.crc);
    need(selected==i.default_state->selected.records.end(),"Initial498940 unexpected real Default record requires its native selected-source lane");
    check(); // NULL is derived from the actual retained namespace, never passed by a caller.
}
std::shared_ptr<detail::BnwNativeResourceDiscoveryActor>
detail::BnwNativeResourceDiscoveryActor::create_pre_dx9(const std::shared_ptr<BnwNativePreDx9DiscoveryOwner>& source) {
    need(bool(source),"Pre-DX9 Discovery needs actual Parent issuer");source->require_current();
    auto out=std::shared_ptr<BnwNativeResourceDiscoveryActor>(new BnwNativeResourceDiscoveryActor(source));
    out->state_->actor=out;source->require_current();return out;
}
detail::BnwNativeResourceDiscoveryActor::Batch
detail::BnwNativeResourceDiscoveryActor::enumerate_pre_dx9_catalog_821760()const {
    const auto state=state_;State::Scope operation(state);
    need(state->origin==State::Origin::installation_pre_dx9&&!state->installed_attempted&&
         !state->default_attempted&&!state->global_attempted&&state->issued.empty(),
         "Pre-DX9 Discovery source replay or wrong domain");
    const auto actual=state->pre_dx9.lock();need(bool(actual),"Pre-DX9 actual issuer expired");
    actual->require_current();BnwNativePreDx9DiscoveryOwner::ParentPin retained(*actual);
    const auto& i=*retained.parent->impl_;
    need(actual->impl_->phase==BnwNativePreDx9DiscoveryOwner::Impl::Phase::enumerating&&
         i.installation_input&&!i.lease&&!i.rules&&!i.header_attempted&&!i.header_active&&
         !i.primary32_attempted&&!i.installation_primary32_attempted&&!i.production_default_published&&
         !i.frame.primary_dx9_registration_complete&&!i.common_driver_attempted&&!i.live_resource_query&&
         i.default_state->live_enabled89==1&&i.default_state->selected.records.empty()&&
         i.default_state->loose_native.empty()&&i.resources.mounts().empty()&&
         !i.cold&&!i.entry_arena&&!i.archive_cache&&!i.current_default_source,
         "Pre-DX9 enumeration requires the real initial empty Default before Primary32");
    state->installed_attempted=true;Batch batch;
    try {
        for(const auto& row:i.journal) {
            if(row.attributes&FILE_ATTRIBUTE_DIRECTORY)continue;
            actual->require_current();
            const auto filename=native_loose_detail::acp_path(std::filesystem::path("Assets")/row.relative);
            need(matches_factory_pattern(native_loose_detail::acp_path(row.relative.filename())),
                 "Pre-DX9 enumeration contains another pattern");
            const auto ordinal=state->issued.size();
            const std::weak_ptr<State> weak=state;
            const auto source=std::shared_ptr<const Filename::Source>(new Filename::Source(
                state->actor,filename,[weak,ordinal]{
                    const auto s=weak.lock();need(bool(s),"Pre-DX9 Discovery retired");State::Scope scope(s);
                    const auto owner=s->pre_dx9.lock();need(bool(owner),"Pre-DX9 Parent issuer retired");
                    need(ordinal<s->issued.size(),"Pre-DX9 filename no longer issued");
                    owner->require_source_identity(s->issued[ordinal].get(),ordinal);s->live();
                },[weak,ordinal](const BnwNativeCompletedColdSourceLoan& loan,const BnwNativeColdFilesystemOwner& cold){
                    const auto s=weak.lock();need(bool(s),"Completed initial Discovery retired");State::Scope scope(s);
                    const auto owner=s->pre_dx9.lock();need(bool(owner),"Completed initial issuer retired");
                    need(ordinal<s->issued.size(),"Completed initial filename not issued");
                    owner->require_source_identity(s->issued[ordinal].get(),ordinal);owner->require_completed_cold(loan,cold);s->live();
                },[weak,ordinal](const Filename::Source& source){
                    const auto s=weak.lock();need(bool(s),"Pre-DX9 constructor Discovery retired");
                    s->live();const auto owner=s->pre_dx9.lock();need(bool(owner),"Pre-DX9 constructor issuer retired");
                    owner->require_source_identity(&source,ordinal);s->live();
                },[weak]()noexcept{if(const auto s=weak.lock()){
                    s->failed.store(true);if(const auto owner=s->pre_dx9.lock())owner->fail();}}
            ));
            state->issued.push_back(source);batch.filenames.push_back(Filename(source));++batch.native_count;
            actual->require_current();
        }
        need(batch.native_count==state->issued.size(),"Pre-DX9 actual source count changed");return batch;
    }catch(...){state->failed.store(true);actual->fail();throw;}
}
class detail::BnwNativePreDx9DiscoveryOwner::DescriptorStreamSource final:public BnwNativePackageDescriptorByteReader::Source {
    friend class BnwNativePreDx9DiscoveryOwner;
    const std::thread::id thread_=std::this_thread::get_id();
    const std::weak_ptr<BnwNativePreDx9DiscoveryOwner> owner_;
    const std::weak_ptr<const BnwNativePackageLoadActor> receiver_;
    const BnwNativePackageFStringField0* const field_;
    const std::string raw_name_;
    std::string native_name_;
    std::filesystem::path original_io_;
    HANDLE handle_=nullptr;BY_HANDLE_FILE_INFORMATION identity_{};
    std::atomic<bool> invalid_{false},closed_{false},reading_{false};
    DWORD open_result_=1,os_open_error_=0,read_error_=0,size_=0,position_=0;
    void open_mode1_zero_buffer() {
        const auto opened=CreateFileW(original_io_.c_str(),GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE,
            nullptr,OPEN_EXISTING,0x08000080,nullptr);
        if(opened==INVALID_HANDLE_VALUE){os_open_error_=GetLastError();open_result_=1;return;}
        handle_=opened;open_result_=0;size_=GetFileSize(handle_,nullptr); // Actual828D10+22C DWORD cache.
        need(GetFileInformationByHandle(handle_,&identity_)&&identity_.nFileSizeHigh==0&&identity_.nFileSizeLow==size_&&
             !(identity_.dwFileAttributes&(FILE_ATTRIBUTE_DIRECTORY|FILE_ATTRIBUTE_REPARSE_POINT)),
             "Initial physical stream identity/ordinary-file source unavailable");
    }
    DescriptorStreamSource(const std::shared_ptr<BnwNativePreDx9DiscoveryOwner>& owner,
        const std::shared_ptr<BnwNativePackageLoadActor>& actor)
        :owner_(owner),receiver_(actor),field_(actor->state_->receiver.get()),
         raw_name_(field_->copied_input_for_498940_global_lookup()),native_name_(raw_name_) {
        try {
            owner->require_current();need(actor->state_->pre_dx9_lookup_source&&field_,"Initial OS stream lacks actual498940 source/field");
            original_io_=owner->actual_initial_descriptor_path(*actor);
            //820410: separated filename + actual8214D0 miss tries OS(flags|4).
            owner->require_initial_default_miss(*actor);open_mode1_zero_buffer();
            if(open_result_){
                //Only the second actual8211F0 NULL admits82058A OS fallback.
                owner->require_initial_default_miss(*actor);open_mode1_zero_buffer();
            }
            owner->require_current();
        }catch(...){close_native_stream();throw;}
    }
public:
    ~DescriptorStreamSource()override{close_native_stream();}
    void require_current()const override {
        need(thread_==std::this_thread::get_id()&&!invalid_.load(),"Initial OS source retired/foreign creator");
        const auto owner=owner_.lock();const auto actor=receiver_.lock();
        need(owner&&actor,"Initial OS source issuer/receiver retired");
        owner->require_descriptor_source_identity(*actor,this);
        need(actor->state_->receiver.get()==field_&&field_->copied_input_for_498940_global_lookup()==raw_name_,
             "Initial OS source changed actual field0 occurrence/text");
        if(handle_&&!closed_.load()){
            BY_HANDLE_FILE_INFORMATION now{};need(GetFileInformationByHandle(handle_,&now)&&
                now.dwVolumeSerialNumber==identity_.dwVolumeSerialNumber&&now.nFileIndexHigh==identity_.nFileIndexHigh&&
                now.nFileIndexLow==identity_.nFileIndexLow&&now.nFileSizeHigh==identity_.nFileSizeHigh&&
                now.nFileSizeLow==identity_.nFileSizeLow&&now.ftLastWriteTime.dwHighDateTime==identity_.ftLastWriteTime.dwHighDateTime&&
                now.ftLastWriteTime.dwLowDateTime==identity_.ftLastWriteTime.dwLowDateTime,
                "Initial actual open descriptor changed during owned read");
        }
        owner->require_current();
    }
    void require_current_completed(const BnwNativeCompletedColdSourceLoan& loan,const BnwNativeColdFilesystemOwner& cold)const override {
        require_current();const auto owner=owner_.lock();need(bool(owner),"Completed initial OS issuer retired");owner->require_completed_cold(loan,cold);require_current();
    }
    void require_same_receiver(const BnwNativePackageFStringField0& field)const override {require_current();need(&field==field_,"Initial OS source belongs to another actual receiver");}
    std::uint32_t native_open_error_code()const override {require_current();return open_result_;}
    std::uint32_t native_size_32()const override {require_current();return size_;}
    std::uint32_t native_read(std::span<std::uint8_t> bytes)override {
        require_current();need(!closed_.load()&&handle_&&open_result_==0,"Initial read outside actual open mode1 handle");
        if(reading_.exchange(true)){invalid_.store(true);throw std::runtime_error("Initial physical ReadFile reentry");}
        struct Exit{DescriptorStreamSource& source;~Exit(){source.reading_.store(false);}} exit{*this};
        need(bytes.size()<=MAXDWORD,"Initial physical read request exceeds native DWORD");
        DWORD actual=static_cast<DWORD>(bytes.size());const auto success=ReadFile(handle_,bytes.data(),actual,&actual,nullptr);
        read_error_=success?0:GetLastError();position_+=actual;require_current();return success?actual:0;
    }
    std::uint32_t native_read_error_code()const override {require_current();return read_error_;}
    void close_native_stream()noexcept override {if(closed_.exchange(true))return;if(handle_){CloseHandle(handle_);handle_=nullptr;}native_name_.clear();}
    void invalidate()noexcept override {invalid_.store(true);}
};
void detail::BnwNativePreDx9DiscoveryOwner::drive_initial_receivers() {
    need(impl_->thread==std::this_thread::get_id(),"Initial driver foreign creator");
    if(impl_->busy.exchange(true)){fail();throw std::runtime_error("Initial descriptor driver reentry");}
    struct Operation{Impl& i;~Operation(){i.busy.store(false);i.active_metadata_receiver=nullptr;}} operation{*impl_};
    try {
        require_current();need(impl_->phase==Impl::Phase::unissued&&impl_->actor&&impl_->receivers.empty(),"Initial factory replay/unissued");
        impl_->phase=Impl::Phase::enumerating;const auto batch=impl_->actor->enumerate_pre_dx9_catalog_821760();
        impl_->phase=Impl::Phase::constructing;
        for(const auto& filename:batch.filenames){
            const auto ordinal=static_cast<long long>(&filename-batch.filenames.data());
            BnwPrimaryDefaultStageTrace receiver_trace("PreDX9.receiver",ordinal);
            require_current();auto receiver=BnwNativePackageLoadActor::construct_preload_receiver_for_498940(*impl_->actor,filename);
            impl_->receivers.push_back(receiver);require_current();
            receiver->prepare_canonical_receiver_from_initial_default(impl_->self.lock());require_current();
            auto source=std::shared_ptr<BnwNativePackageDescriptorByteReader::Source>(new DescriptorStreamSource(impl_->self.lock(),receiver));
            impl_->streams.push_back(source);require_current();
            const auto retire_false=[&]{source->close_native_stream();source->invalidate();
                need(impl_->receivers.back().get()==receiver.get()&&impl_->streams.back().get()==source.get(),"Initial native false cleanup lost actual pair");
                impl_->streams.pop_back();impl_->receivers.pop_back();require_current();};
            {BnwPrimaryDefaultStageTrace trace("PreDX9.read",ordinal);if(!receiver->read_descriptor_bytes_before_834b30(source)){retire_false();continue;}}
            {BnwPrimaryDefaultStageTrace trace("PreDX9.parse",ordinal);if(!receiver->parse_descriptor_owned_bytes_834b30()){retire_false();continue;}}
            std::shared_ptr<const BnwNativeStandalonePackageLoaded> token;
            {
                struct MetadataSource{Impl& i;const BnwNativePackageLoadActor* previous;
                    MetadataSource(Impl& value,const BnwNativePackageLoadActor& actor):i(value),previous(value.active_metadata_receiver){need(!previous,"Initial metadata operation reentry");i.active_metadata_receiver=&actor;}
                    ~MetadataSource(){i.active_metadata_receiver=previous;}} actual_metadata(*impl_,*receiver);
                {BnwPrimaryDefaultStageTrace trace("PreDX9.core",ordinal);if(!receiver->write_descriptor_core_738f00()){retire_false();continue;}}
                {BnwPrimaryDefaultStageTrace trace("PreDX9.metadata",ordinal);token=receiver->write_standalone_descriptor_metadata_738f00();}
            }
            need(token&&token->actual_receiver_==receiver->state_->receiver.get()&&
                 token->actual_metadata_==receiver->state_->completed_metadata_identity,
                 "Initial factory returned another actual receiver/metadata occurrence");
            impl_->returned.push_back(token);require_returned_package_current(*token);require_current();
        }
        need(impl_->receivers.size()==impl_->streams.size()&&impl_->returned.size()==impl_->receivers.size(),
             "Initial success-only receiver/stream/metadata roster differs");
        impl_->phase=Impl::Phase::returned;require_current();
    }catch(...){fail();throw;}
}
void detail::BnwNativePreDx9DiscoveryOwner::adopt_into_actual_common_warm(
    const std::shared_ptr<BnwNativeGlobalResourceWarmOwner>& warm) {
    need(impl_->thread==std::this_thread::get_id(),"Initial adoption foreign creator");
    if(impl_->busy.exchange(true)){fail();throw std::runtime_error("Initial receiver adoption reentry");}
    struct Adoption{Impl& i;~Adoption(){i.busy.store(false);}} adoption{*impl_};
    require_current();ParentPin retained(*this);auto& i=*retained.parent->impl_;
    need(warm&&i.live_resource_query.get()==warm.get()&&!i.live_resource_query.owner_before(warm)&&!warm.owner_before(i.live_resource_query)&&
         !warm->parent_.owner_before(i.self)&&!i.self.owner_before(warm->parent_)&&
         warm->epoch_.get()==i.identity.get(),"Initial receiver transfer needs SAME actual startup Parent/Common Warm");
    warm->require_current(); // Genuine returned Common/quality, never an early Warm.
    need(impl_->phase==Impl::Phase::returned&&!warm->pre_dx9_owner_&&!warm->discovery_attempted_&&
         !warm->early_receiver_driver_returned_&&!warm->discovery_actor_&&warm->preload_receivers_.empty()&&
         warm->descriptor_sources_.empty()&&warm->returned_standalone_packages_.empty(),"Initial receiver adoption replay/occupied factory");
    if(impl_->language)warm->require_same_selected_language_input(*impl_->language);
    std::vector<std::weak_ptr<const BnwNativePackageLoadActor>> weak_roster;
    weak_roster.reserve(impl_->receivers.size());
    for(std::size_t n=0;n<impl_->receivers.size();++n){
        const auto& actor=impl_->receivers[n];const auto& token=impl_->returned[n];
        require_returned_package_current(*token);
        need(actor->state_->descriptor_bytes&&actor->state_->parsed_xml&&actor->state_->descriptor_core&&
             actor->state_->metadata_input&&actor->state_->pre_dx9_lookup_source&&!actor->state_->actual_lookup_source&&
             !actor->state_->adopted_warm&&!actor->state_->busy.load()&&!actor->state_->failed.load(),
             "Initial adoption contains unfinished/failed/rebound native receiver");
        weak_roster.emplace_back(actor);
    }
    require_current();warm->require_current();
    // From here through phase commit: standard same-allocator moves and weak assignments only.
    impl_->receiver_identities=std::move(weak_roster);impl_->adopted_actor=impl_->actor;impl_->consumer=warm;
    warm->pre_dx9_owner_=impl_->self.lock();warm->discovery_actor_=std::move(impl_->actor);
    warm->preload_receivers_=std::move(impl_->receivers);warm->descriptor_sources_=std::move(impl_->streams);
    warm->returned_standalone_packages_=std::move(impl_->returned);
    for(const auto& actor:warm->preload_receivers_)actor->state_->adopted_warm=warm;
    warm->discovery_attempted_=true;warm->early_receiver_driver_returned_=true;impl_->phase=Impl::Phase::adopted;
    try {require_current();warm->require_current();
        for(const auto& token:warm->returned_standalone_packages_)warm->require_returned_standalone_package_loaded_current(*token);
    }catch(...){fail();warm->failed_.store(true);i.header_poisoned=true;throw;}
}
std::shared_ptr<const runtime::BnwSelectedLanguageInputOwner>
BnwNativePackageDiscoveryOwner::borrow_initial_package_language_for_boot(const BnwNativeResourceBootSettingsOwner& actual)const {
    if(!impl_->pre_dx9_discovery)return {};
    const auto initial=impl_->pre_dx9_discovery;initial->require_actual_boot_consumer(actual);return initial->impl_->language;
}
std::shared_ptr<const BnwNativePackageDiscoveryOwner>
BnwNativePackageDiscoveryOwner::create_pre_dx9_installation(
    std::shared_ptr<const BnwNativeInstallationInputOwner> installation) {
    need(bool(installation),"Pre-DX9 actual installation input absent");
    installation=detail::BnwNativeFilesystemMappingOwner::actual_input_control(installation);
    installation->require_current();
    const auto filesystem=detail::BnwNativeFilesystemMappingOwner::create_fresh_installed(installation);
    auto i=std::make_shared<Impl>(installation->canonical_payload_root());
    i->filesystem_mappings=filesystem;
    i->installation_input=installation;i->shared=installation->canonical_payload_root();
    i->original=std::filesystem::canonical(installation->game_root()/"Assets");
    i->registry.identity=i->identity;i->registry.revision=1;
    i->journal=scan(i->original); // Actual821760/821FE0 file/recursive order, no catalog/manifest substitution.
    const auto out=std::shared_ptr<const BnwNativePackageDiscoveryOwner>(new BnwNativePackageDiscoveryOwner(i));
    i->self=out;filesystem->bind_parent(out);
    need(!i->header_poisoned,"FS bootstrap canonical Parent bind retired actual factory");
    auto impl=std::make_unique<detail::BnwNativePreDx9DiscoveryOwner::Impl>(out,installation,i->default_state,i->resources,i->identity);
    auto issuer=std::shared_ptr<detail::BnwNativePreDx9DiscoveryOwner>(new detail::BnwNativePreDx9DiscoveryOwner(std::move(impl)));
    issuer->impl_->self=issuer;i->pre_dx9_discovery=issuer;
    issuer->impl_->actor=detail::BnwNativeResourceDiscoveryActor::create_pre_dx9(issuer);
    issuer->require_current();return out;
}
void BnwNativePackageDiscoveryOwner::advance_pre_dx9_discovery_before_primary32()const {
    const auto storage=impl_;const auto issuer=storage->pre_dx9_discovery;
    need(storage->thread==std::this_thread::get_id()&&issuer,
         "Pre-DX9 constructor drive has no actual installation issuer");
    try{issuer->drive_initial_receivers();}
    catch(...){storage->header_poisoned=true;throw;}
}

