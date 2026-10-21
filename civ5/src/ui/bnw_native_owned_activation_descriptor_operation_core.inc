// SAME actual held Loaded/Load/Core/Parser/Reader operation. No ambient query
// consumes this pointer, and no generic accessor/currentness route is changed.
struct detail::BnwNativeGlobalResourceWarmOwner::OwnedActivationDescriptorOperation::State final {
    using Warm=BnwNativeGlobalResourceWarmOwner;
    using Loaded=BnwNativeStandalonePackageLoaded;
    Warm& warm;
    BnwNativePackageDiscoveryOwner::Impl& parent;
    const Loaded& package;
    const Loaded::Loan& loan;
    const std::function<void()>& parent_current;
    const std::thread::id creator=std::this_thread::get_id();
    const BnwNativePackageDiscoveryOwner::Impl::HeaderGuard* const fence;
    const BnwNativeCommonSourceValidationContext* const common;
    const BnwNativePackageDiscoveryOwner::Impl::DirectoryParentLoan* const directory;
    const BnwNativePackageDiscoveryOwner::Impl::InstallationParentLoan* const installation;
    const std::size_t ordinal;
    const std::uint64_t resource_epoch;
    const std::weak_ptr<const void> load_control,core_control,parser_control,reader_control,
        stream_control,input_control,filename_control,discovery_control,buffer_control;
    const BnwNativePackageDescriptorCoreWriter::Fields* const actual_fields;
    const std::optional<std::array<std::uint8_t,16>> policy_guid;
    const BnwNativePackageXmlParserOwner::Document* const actual_document;
    const BnwNativePackageDescriptorCoreWriter::Metadata* const actual_metadata;
    const void* const actual_load;
    const void* const actual_core;
    const void* const actual_parser;
    const void* const actual_reader;
    const void* const actual_input;
    const void* actual_stream_address=nullptr;
    std::size_t source_ordinal=0;
    std::weak_ptr<const void> issued_row_control;
    std::uint32_t expected_mask;
    std::shared_ptr<OwnedActivationReceipt> row;
    bool expected_selected_gameplay=false;
    std::array<std::optional<OwnedActivationReceipt::Transition>,2> expected_transitions{};
    std::size_t transition_count=0;
    bool pure=false,complete=false;

    State(Warm& value,BnwNativePackageDiscoveryOwner::Impl& impl,const Loaded& token,
        const Loaded::Loan& held,const std::function<void()>& current)
        :warm(value),parent(impl),package(token),loan(held),parent_current(current),
         fence(impl.active_header_fence),common(impl.active_descriptor_common_source_context),
         directory(impl.active_directory_parent_loan),installation(impl.active_installation_parent_loan),
         ordinal(value.owned_activation_receipts_.size()),resource_epoch(held.resource.epoch),
         load_control(held.actor),core_control(held.actor->state_->descriptor_core),
         parser_control(held.actor->state_->parsed_xml),reader_control(held.actor->state_->descriptor_bytes),
         stream_control(held.actor->state_->descriptor_bytes->state_->source),
         input_control(held.actor->state_->metadata_input),filename_control(held.actor->state_->filename),
         discovery_control(held.actor->state_->discovery),
         buffer_control(held.actor->state_->descriptor_bytes->state_->buffer),actual_fields(&held.resource.actual_fields),
         policy_guid(held.resource.actual_fields.written_guid),
         actual_document(&held.resource.actual_document),actual_metadata(token.actual_metadata_),
         actual_load(held.actor.get()),actual_core(held.actor->state_->descriptor_core.get()),
         actual_parser(held.actor->state_->parsed_xml.get()),actual_reader(held.actor->state_->descriptor_bytes.get()),
         actual_input(held.actor->state_->metadata_input.get()),
         expected_mask(held.resource.actual_fields.current_category_mask()) {
        need(bool(warm.discovery_actor_),"Activation descriptor genuine Discovery absent");
        const auto& issued=warm.discovery_actor_->state_->issued;
        const auto found=std::find_if(issued.begin(),issued.end(),[&](const auto& source){
            return source&&!source.owner_before(filename_control)&&!filename_control.owner_before(source);});
        need(found!=issued.end(),"Activation descriptor Source is outside genuine issued vector");
        source_ordinal=static_cast<std::size_t>(found-issued.begin());
        need(source_ordinal<warm.installed_descriptor_rows_.size(),"Activation descriptor installed row absent");
        issued_row_control=warm.installed_descriptor_rows_[source_ordinal];
        const auto stream=std::find_if(warm.descriptor_sources_.begin(),warm.descriptor_sources_.end(),
            [&](const auto& source){return source&&same(source,stream_control);});
        need(stream!=warm.descriptor_sources_.end(),"Activation descriptor canonical stream absent");
        actual_stream_address=stream->get();
    }
    static bool same(const auto& value,const std::weak_ptr<const void>& control) {
        return !value.owner_before(control)&&!control.owner_before(value)&&!control.expired();
    }
    void selected_identity(const Warm::InstalledDescriptorRow& row)const {
        // SAME CURRENT map/FIRST/stat-control checks, without allocating
        // the discarded quote's owned filename inside this identity guard.
        need(warm.thread_==std::this_thread::get_id()&&row.thread==warm.thread_&&!warm.closed_.load()&&!warm.failed_.load(),
             "Installed quote Warm/row creator retired");
        ParentPin retained(warm);const auto& i=*retained.parent->impl_;
        need(i.thread==warm.thread_&&!i.header_poisoned&&!i.category_lineage_failed&&i.live_resource_query.get()==&warm&&
             row.ordinal<warm.installed_descriptor_rows_.size()&&warm.installed_descriptor_rows_[row.ordinal].get()==&row&&
             i.first_lineage&&row.first_owner.get()==i.first_lineage.get()&&
             !row.first_owner.owner_before(i.first_lineage)&&!i.first_lineage.owner_before(row.first_owner)&&
             row.canonical_selected&&row.native_node&&row.native_node->original_stat_,
             "Installed quote lacks original canonical receipt/FIRST storage");
        const auto& names=i.first_lineage->names(); // Existing pure Parent identity gate, counted temporary lock.
        const auto selected=i.default_state->selected.records.find(row.selected.key);
        const auto named=names.find(row.selected.key);
        const auto node=i.default_state->loose_native.find(row.selected.key);
        need(selected!=i.default_state->selected.records.end()&&named!=names.end()&&
             node!=i.default_state->loose_native.end()&&node->second,
             "Installed quote lost current selected node/FIRST");
        // Current-find BEFORE dereferencing any map node. A later genuine
        // publication may copy/move Default/FIRST maps; saved addresses are not
        // authority. Authenticate immutable handle provenance and stat controls.
        const auto* handle=std::get_if<assets::AssetHandle>(&selected->second);
        const auto& expected=row.canonical_selected->candidate();const auto& old=*row.native_node;
        const auto& actual=*node->second;
        need(handle&&handle->candidate().provenance.kind==assets::SourceKind::loose&&
             handle->candidate().provenance.source_id==expected.provenance.source_id&&
             handle->candidate().provenance.source_relative_path==expected.provenance.source_relative_path&&
             handle->candidate().original_name==expected.original_name&&handle->candidate().normalized_name==expected.normalized_name&&
             handle->candidate().payload_size==expected.payload_size&&named->second==row.selected.filename&&
             actual.selected_entry_==&selected->second&&actual.original_stat_.get()==old.original_stat_.get()&&
             !actual.original_stat_.owner_before(old.original_stat_)&&!old.original_stat_.owner_before(actual.original_stat_)&&
             actual.word_.crc==old.word_.crc&&actual.word_.length==old.word_.length&&actual.word_.offset==old.word_.offset&&
             actual.word_.packed_prefix==old.word_.packed_prefix&&actual.prefix_index_==old.prefix_index_&&
             actual.original_folder_query_==old.original_folder_query_&&actual.original_find_basename_==old.original_find_basename_&&
             actual.registered_prefix_==old.registered_prefix_,
             "Installed quote current winner/control/prefix/stat differs from sealed occurrence");
    }
    void identity()const {
        // FIRST creator/live/active checks precede every saved ref use.
        need(creator==std::this_thread::get_id()&&!complete&&
             warm.active_owned_activation_descriptor_&&
             warm.active_owned_activation_descriptor_->state.get()==this,
             "Activation descriptor foreign/retired/replaced lexical loan");
        need(parent.active_directory_parent_loan==directory&&parent.active_installation_parent_loan==installation,
             "Activation descriptor replaced actual outer Directory/Installation lender");
        parent_current();parent.require_active_header_without_callbacks();
        need(fence&&common&&parent.active_header_fence==fence&&fence->valid()&&
             parent.active_descriptor_common_source_context==common&&parent.active_live_query_owner==&warm&&
             !warm.closed_.load()&&!warm.failed_.load()&&!warm.configured_activation_policy_returned_&&
             warm.early_receiver_driver_returned_&&warm.owned_category_stages_.empty()&&
             !warm.active_owned_category_stage_&&!warm.active_owned_category_preconstructor_&&
             !warm.active_owned_skin_operation_epoch_&&!parent.active_completed_resource_operation,
             "Activation descriptor changed real Header/Common/source phase");
        common->require_owned_activation_identity_without_callbacks(parent.self,parent.lease);
        need(loan.creator_thread==creator&&!loan.completed_source&&!loan.completed_cold&&
             &loan.token==&package&&loan.retained.get()==&package&&loan.retained.use_count()>1&&
             loan.actor&&loan.actor.get()==actual_load&&loan.actor.use_count()>1&&same(loan.actor,load_control)&&
             same(package.actor_,load_control)&&!package.self_.owner_before(loan.retained)&&
             !loan.retained.owner_before(package.self_)&&ordinal<warm.returned_standalone_packages_.size()&&
             warm.returned_standalone_packages_[ordinal].get()==&package&&
             !warm.returned_standalone_packages_[ordinal].owner_before(package.self_)&&
             !package.self_.owner_before(warm.returned_standalone_packages_[ordinal]),
             "Activation descriptor changed genuine returned package/receiver/ordinal");
        need(std::any_of(warm.preload_receivers_.begin(),warm.preload_receivers_.end(),[&](const auto& actor){
            return actor&&actor.get()==actual_load&&same(actor,load_control);}),
             "Activation descriptor lost actual factory-retained Load receiver");
        const auto& load=loan.actor->state_;load->live();
        need(load->busy.load()&&load->active_resource_loan==resource_epoch&&resource_epoch&&
             load->active_resource_loan_address==&loan.resource&&same(loan.resource.state,load)&&
             !loan.resource.completed_source&&!loan.resource.completed_cold&&
             load->actual_lookup_source&&same(*load->actual_lookup_source,parent.live_resource_query)&&
             same(load->descriptor_core,core_control)&&same(load->parsed_xml,parser_control)&&
             same(load->descriptor_bytes,reader_control)&&same(load->metadata_input,input_control)&&
             load->descriptor_core.get()==actual_core&&load->parsed_xml.get()==actual_parser&&
             load->descriptor_bytes.get()==actual_reader&&load->metadata_input.get()==actual_input&&
             same(load->filename,filename_control)&&same(load->discovery,discovery_control)&&
             load->receiver.get()==package.actual_receiver_&&load->completed_metadata_identity==actual_metadata&&
             !load->returned_loaded.owner_before(package.self_)&&!package.self_.owner_before(load->returned_loaded),
             "Activation descriptor lost SAME active ResourceLoan/Core/Source custody");
        need(std::none_of(load->category_slots.begin(),load->category_slots.end(),[](const auto& slot){return bool(slot);}),
             "Civis policy requires actual untouched constructor +48 and null manager slots");
        const auto& core=load->descriptor_core->state_;
        const auto& parser=load->parsed_xml->state_;const auto& reader=load->descriptor_bytes->state_;
        need(core->thread==creator&&!core->closed.load()&&!core->failed.load()&&core->busy.load()&&
             same(core->self,core_control)&&same(core->parser,parser_control)&&core->fields.get()==actual_fields&&
             core->metadata.get()==actual_metadata&&core->document_identity==actual_document&&
             core->actual_settings&&same(*core->actual_settings,input_control)&&
             parser->thread==creator&&!parser->closed.load()&&!parser->failed.load()&&parser->busy.load()&&
             same(parser->self,parser_control)&&same(parser->reader,reader_control)&&parser->document.get()==actual_document&&
             reader->thread==creator&&!reader->closed.load()&&!reader->failed.load()&&reader->busy.load()&&
             same(reader->reader,reader_control)&&same(reader->source,stream_control)&&reader->source.use_count()>1&&reader->buffer&&
             same(reader->buffer,buffer_control)&&
             std::any_of(warm.descriptor_sources_.begin(),warm.descriptor_sources_.end(),[&](const auto& source){
                 return source&&same(source,stream_control);}),
             "Activation descriptor retired/replaced held Core/Parser/Reader/provider");
        const auto stream=std::find_if(warm.descriptor_sources_.begin(),warm.descriptor_sources_.end(),
            [&](const auto& source){return source&&same(source,stream_control);});
        const auto* actual_stream=dynamic_cast<const DescriptorStreamSource*>(stream->get());
        need(actual_stream&&actual_stream==actual_stream_address&&actual_stream->thread_==creator&&!actual_stream->invalid_.load()&&
             same(actual_stream->warm_,parent.live_resource_query)&&same(actual_stream->receiver_,load_control)&&
             actual_stream->field_==package.actual_receiver_&&actual_stream->loan_,
             "Activation descriptor canonical native provider retired/substituted");
        need(warm.discovery_actor_&&same(warm.discovery_actor_,discovery_control),
             "Activation descriptor replaced actual Discovery control");
        const auto& discovery=warm.discovery_actor_->state_;discovery->live();
        need(!discovery->busy.load()&&
             discovery->origin==BnwNativeResourceDiscoveryActor::State::Origin::returned_common_warm&&
             same(discovery->warm,parent.live_resource_query)&&source_ordinal<discovery->issued.size()&&
             source_ordinal<warm.installed_descriptor_rows_.size(),
             "Activation descriptor changed actual installed Discovery/Source ordinal");
        const auto& source=discovery->issued[source_ordinal];
        const auto& issued_row=warm.installed_descriptor_rows_[source_ordinal];
        need(source&&same(source,filename_control)&&source->thread==creator&&
             same(source->actor,discovery_control)&&issued_row&&same(issued_row,issued_row_control)&&issued_row->thread==creator&&
             source->native_acp_filename==issued_row->filename,
             "Activation descriptor substituted actual Source/row/control");
        selected_identity(*issued_row);
        need(actual_fields->written_guid==policy_guid,
             "Activation descriptor changed held immutable GUID snapshot");
        need(actual_fields->current_category_mask()==expected_mask&&
             ((row&&warm.owned_activation_receipts_.size()==ordinal+1&&
               warm.owned_activation_receipts_[ordinal].get()==row.get()&&row->ordinal==ordinal&&
               row->document==actual_document&&row->expected_mask==expected_mask&&
               row->selected_gameplay==expected_selected_gameplay&&row->transitions.size()==transition_count&&
               !row->package.owner_before(package.self_)&&!package.self_.owner_before(row->package)&&
               !row->effects_returned[0]&&!row->effects_returned[1])||
              (!row&&warm.owned_activation_receipts_.size()==ordinal)),
             "Activation descriptor actual mask/receipt occurrence changed during FULL callback");
        if(row)for(std::size_t n=0;n<transition_count;++n) {
            const auto& expected=*expected_transitions[n];const auto& actual=row->transitions[n];
            need(actual.category==expected.category&&actual.before==expected.before&&
                 actual.after==expected.after&&actual.enabled==expected.enabled,
                 "Activation descriptor replaced its actual returned transition record");
        }
    }
    void require_pure()const {identity();need(pure,"Activation descriptor borrow crossed an unchecked effect boundary");}
    void after_own_effect() {
        // The fixed initial policy consumes the held Core/GUID snapshot.
        // Receipt/vector/mask bookkeeping issues no Source/provider IO.
        // FULL entry/exit remain; reentry/replacement/effect drift refuse here.
        need(!pure,"Activation descriptor own effect did not close its borrow");
        identity();pure=true;
    }
    void refresh() {
        pure=false;identity();
        // No grant from the Header bit. Generic Loaded/Source/Reader/provider
        // FULL remains real, then SAME retained metadata identity/live source checks.
        loan.require_current();identity();parent.validate(true,false,nullptr,&warm);identity();pure=true;
    }
    void fail()const noexcept {
        warm.failed_.store(true);parent.header_poisoned=true;
        loan.actor->state_->failed.store(true);
    }
};
detail::BnwNativeGlobalResourceWarmOwner::OwnedActivationDescriptorOperation::OwnedActivationDescriptorOperation(
    BnwNativeGlobalResourceWarmOwner& warm,const BnwNativeStandalonePackageLoaded& package,
    const BnwNativeStandalonePackageLoaded::Loan& loan,const std::function<void()>& current) {
    need(warm.thread_==std::this_thread::get_id(),"Activation descriptor foreign constructor");
    if(warm.active_owned_activation_descriptor_) {
        warm.active_owned_activation_descriptor_->state->fail();
        throw std::runtime_error("Activation descriptor lexical reentry");
    }
    ParentPin retained(warm);
    // The real consumer is already inside Loaded's actual callback FULL PRE.
    // Capture witnesses before the last fresh callbacks so replacements refuse.
    state=std::make_unique<State>(warm,*retained.parent->impl_,package,loan,current);
    warm.active_owned_activation_descriptor_=this;
    try {state->refresh();}catch(...){state->fail();warm.active_owned_activation_descriptor_=nullptr;throw;}
}
detail::BnwNativeGlobalResourceWarmOwner::OwnedActivationDescriptorOperation::~OwnedActivationDescriptorOperation() {
    if(!state)return;
    if(!state->complete)state->fail();
    if(state->warm.active_owned_activation_descriptor_==this)state->warm.active_owned_activation_descriptor_=nullptr;
    else state->fail();
    // row keeper destruction follows retirement; the original callback's FULL
    // POST still runs after this operation and its actor/row keepers are gone.
}
const detail::BnwNativePackageDescriptorCoreWriter::Fields&
detail::BnwNativeGlobalResourceWarmOwner::OwnedActivationDescriptorOperation::fields()const {
    state->require_pure();return *state->actual_fields;
}
std::optional<std::array<std::uint8_t,16>>
detail::BnwNativeGlobalResourceWarmOwner::OwnedActivationDescriptorOperation::guid_snapshot()const {
    state->require_pure();return state->policy_guid;
}
const detail::BnwNativePackageXmlParserOwner::Document&
detail::BnwNativeGlobalResourceWarmOwner::OwnedActivationDescriptorOperation::document()const {
    state->require_pure();return *state->actual_document;
}
std::shared_ptr<detail::BnwNativeGlobalResourceWarmOwner::OwnedActivationReceipt>
detail::BnwNativeGlobalResourceWarmOwner::OwnedActivationDescriptorOperation::new_receipt() {
    state->require_pure();need(!state->row,"Activation descriptor receipt creation replay");state->pure=false;
    auto row=std::make_shared<OwnedActivationReceipt>();
    state->after_own_effect(); // Check held snapshot custody after own allocation.
    row->package=state->package.self_;row->document=state->actual_document;row->ordinal=state->ordinal;
    return row;
}
void detail::BnwNativeGlobalResourceWarmOwner::OwnedActivationDescriptorOperation::publish(
    const std::shared_ptr<OwnedActivationReceipt>& row) {
    state->require_pure();need(!state->row&&row&&row->ordinal==state->ordinal&&
        row->document==state->actual_document&&row->expected_mask==state->expected_mask&&
        !row->package.owner_before(state->package.self_)&&!state->package.self_.owner_before(row->package),
        "Activation descriptor publication has another actual receipt");
    state->pure=false;state->warm.owned_activation_receipts_.push_back(row);state->row=row;
    state->expected_selected_gameplay=row->selected_gameplay;
    state->after_own_effect(); // Check actual receipt/vector occurrence.
}
void detail::BnwNativeGlobalResourceWarmOwner::OwnedActivationDescriptorOperation::disable(std::uint32_t category) {
    state->require_pure();need(category<2&&state->row,"Activation descriptor disable lacks actual Common/Gameplay receipt");
    const auto& fields=*state->actual_fields;const auto before=fields.current_category_mask();
    const auto bit=std::uint32_t{1}<<category;
    if(!(before&bit))return; // Same native no-transition branch, no effect grant.
    const auto after=before&~bit;state->pure=false;
    need(state->transition_count<state->expected_transitions.size(),"Activation descriptor initial transition replay");
    fields.live_category_mask_=after;state->row->expected_mask=after;state->expected_mask=after;
    state->row->transitions.push_back({category,before,after,false});
    state->expected_transitions[state->transition_count++]=OwnedActivationReceipt::Transition{category,before,after,false};
    state->after_own_effect(); // Check actual mask/transition before another read.
    need(!state->loan.actor->state_->category_slots[category],
         "Civis initial disable cannot replace genuine505A90 pair effects");
    state->require_pure();
}
void detail::BnwNativeGlobalResourceWarmOwner::OwnedActivationDescriptorOperation::finish() {
    state->require_pure();state->refresh();state->identity();state->pure=false;state->complete=true;
}
void detail::BnwNativeGlobalResourceWarmOwner::with_owned_activation_initial_descriptor(
    const BnwNativeStandalonePackageLoaded& package,const std::function<void()>& current,
    const std::function<void(OwnedActivationDescriptorOperation&)>& consume) {
    need(thread_==std::this_thread::get_id(),"Activation descriptor issuer foreign creator");
    if(active_owned_activation_descriptor_) {
        active_owned_activation_descriptor_->state->fail();
        throw std::runtime_error("Activation descriptor issuer reentry");
    }
    need(bool(current)&&bool(consume),"Activation descriptor issuer absent");
    package.with_actual_descriptor([&](const auto& loan){
        {
            auto target=consume;
            {
                OwnedActivationDescriptorOperation operation(*this,package,loan,current);
                target(operation);operation.finish();
            } // Loan permission retires BEFORE the copied target is destroyed.
        }
        loan.require_current(); // Kept root callback FULL, before real lease effect.
        ParentPin retained(*this);retained.parent->impl_->lease->require_current();
        need(retained.parent->impl_->active_header_fence&&retained.parent->impl_->active_header_fence->valid(),
             "Civis profile source fence retired");
    }); // Existing actual Loaded/Core/Parser/Reader FULL POST follows callback return.
}
