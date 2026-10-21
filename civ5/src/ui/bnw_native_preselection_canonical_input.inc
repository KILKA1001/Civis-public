// Private synchronous input producer. It precedes selection and CreateFile;
// it is not the separate returned DescriptorStreamSource observation.
class detail::BnwNativeGlobalResourceWarmOwner::PreselectionCanonicalInputLoan final {
    friend class BnwNativeGlobalResourceWarmOwner;
    using Load=BnwNativePackageLoadActor;
    using Field=BnwNativePackageFStringField0;
    using Filename=BnwNativeResourceDiscoveryFilename;
    using Parent=BnwNativePackageDiscoveryOwner;
    BnwNativeGlobalResourceWarmOwner& warm;
    const std::shared_ptr<Load>& receiver;
    Parent::Impl& i;
    Load::State::Scope& load_scope;
    Field::State::Scope& field_scope;
    const std::shared_ptr<const Filename::Source> source;
    const std::shared_ptr<const InstalledDescriptorRow> row;
    const std::size_t ordinal;
    const BnwNativeResourceDiscoveryActor* const actual_discovery;
    const std::uint64_t revision;
    const std::weak_ptr<const void> warm_control,receiver_control,discovery_control,source_control,
        lease_control,settings_control,default_control,selected_control,first_control,epoch_control,row_control;
    const Parent::Impl::DirectoryParentLoan* const directory;
    const Parent::Impl::InstallationParentLoan* const installation;
    const std::size_t directory_pins;
    std::array<const BnwNativeCommon13Step*,13> common{};
    const std::uint8_t default_enabled;
    const std::uint64_t generation;
    Parent::Impl::HeaderGuard fence;
    bool alive=false,complete=false;
    std::string input;
    const char* input_address=nullptr;
    struct Query final {
        PreselectionCanonicalInputLoan& loan;
        const std::thread::id creator=std::this_thread::get_id();
        explicit Query(PreselectionCanonicalInputLoan& value):loan(value) {
            loan.identity();need(!loan.active_query,"Canonical input query invocation replay");
            loan.active_query=this;
        }
        ~Query(){loan.active_query=nullptr;}
        Query(const Query&)=delete;
    };
    const Query* active_query=nullptr;
    static std::size_t source_ordinal(const BnwNativeGlobalResourceWarmOwner& w,const Filename::Source& s) {
        need(w.discovery_actor_&&w.discovery_actor_->state_,"Canonical input Discovery absent");
        const auto& issued=w.discovery_actor_->state_->issued;
        const auto found=std::find_if(issued.begin(),issued.end(),[&](const auto& p){return p.get()==&s;});
        need(found!=issued.end(),"Canonical input Source has no actual factory ordinal");
        return static_cast<std::size_t>(found-issued.begin());
    }
    PreselectionCanonicalInputLoan(BnwNativeGlobalResourceWarmOwner& w,const std::shared_ptr<Load>& r,
            Parent::Impl& storage,Load::State::Scope& ls,Field::State::Scope& fs)
        :warm(w),receiver(r),i(storage),load_scope(ls),field_scope(fs),source(ls.owned->filename.lock()),
         row(source?([&]{const auto n=source_ordinal(w,*source);
             need(n<w.installed_descriptor_rows_.size(),"Canonical input installed row absent");
             return w.installed_descriptor_rows_[n];})():nullptr),
         ordinal(source?source_ordinal(w,*source):0),actual_discovery(w.discovery_actor_.get()),revision(i.registry.revision),
         warm_control(i.live_resource_query),receiver_control(r),discovery_control(w.discovery_actor_),source_control(source),
         lease_control(i.lease),settings_control(i.settings),default_control(i.current_default_source),
         selected_control(i.default_state),first_control(i.first_lineage),epoch_control(i.identity),row_control(row),
         directory(i.active_directory_parent_loan),installation(i.active_installation_parent_loan),
         directory_pins(i.directory_parent_internal_pin_count()),default_enabled(i.default_state->live_enabled89),
         generation(++ls.owned->canonical_input_epoch),fence(i.header_callback_state,i.self) {
        need(generation&&source&&row&&i.common_steps.size()==common.size()&&
             !i.header_active&&!i.active_header_fence&&!i.active_live_query_owner&&
             !w.busy_.load()&&!w.active_preselection_canonical_input_&&!ls.owned->active_canonical_input&&
             !fs.owner->active_canonical_input&&fence.valid(),"Canonical input producer occupied/retired");
        for(std::size_t n=0;n<common.size();++n)common[n]=i.common_steps[n].get();
        i.header_active=true;i.active_header_fence=&fence;i.active_live_query_owner=&warm;
        w.active_preselection_canonical_input_=this;
        ls.owned->active_canonical_input=this;ls.owned->active_canonical_input_generation=generation;
        fs.owner->active_canonical_input=this;fs.owner->active_canonical_input_generation=generation;alive=true;
        try {identity();}catch(...){retire();poison();throw;}
    }
    void poison()noexcept {
        warm.failed_.store(true);load_scope.owned->failed.store(true);field_scope.owner->failed.store(true);
        i.header_poisoned=true;
    }
    void retire()noexcept {
        active_query=nullptr;alive=false;
        warm.active_preselection_canonical_input_=nullptr;
        load_scope.owned->active_canonical_input=nullptr;load_scope.owned->active_canonical_input_generation=0;
        field_scope.owner->active_canonical_input=nullptr;field_scope.owner->active_canonical_input_generation=0;
        i.active_live_query_owner=nullptr;i.active_header_fence=nullptr;i.header_active=false;
    }
    void selected_identity()const {
        // Preserve CURRENT FIRST/provenance/stat-control checks without
        // an owned filename allocation inside this snapshot identity.
        const auto& row=*this->row;
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
        need(i.thread==std::this_thread::get_id()&&warm.thread_==i.thread,
             "Canonical input foreign creator"); // FIRST before all mutable observation.
        const auto same=[](const auto& value,const auto& weak){
            return value&&!value.owner_before(weak)&&!weak.owner_before(value);
        };
        need(alive&&!warm.closed_.load()&&!warm.failed_.load()&&!i.header_poisoned&&!i.category_lineage_failed&&
             !i.common_driver_failed&&!i.common_pending&&i.production_default_published&&i.common_driver_attempted&&
             same(i.live_resource_query,warm_control)&&i.live_resource_query.get()==&warm&&
             same(i.identity,epoch_control)&&warm.epoch_.get()==i.identity.get()&&
             !warm.parent_.owner_before(i.self)&&!i.self.owner_before(warm.parent_)&&
             same(i.lease,lease_control)&&same(i.settings,settings_control)&&
             same(i.default_state,selected_control)&&same(i.current_default_source,default_control)&&
             same(i.first_lineage,first_control)&&i.registry.revision==revision&&
             i.default_state->live_enabled89==default_enabled&&
             i.active_directory_parent_loan==directory&&i.active_installation_parent_loan==installation&&
             i.directory_parent_internal_pin_count()==directory_pins&&
             i.common_steps.size()==common.size(),"Canonical input changed Parent/namespace/Common controls");
        for(std::size_t n=0;n<common.size();++n)
            need(i.common_steps[n]&&i.common_steps[n].get()==common[n],"Canonical input Common journal replaced");
        need(i.header_active&&i.active_header_fence==&fence&&fence.valid()&&i.active_live_query_owner==&warm&&
             !i.active_descriptor_common_source_context&&warm.active_preselection_canonical_input_==this&&
             warm.owned_category_stages_.empty()&&!warm.active_owned_category_stage_&&
             !warm.active_owned_category_preconstructor_&&!warm.active_owned_skin_operation_epoch_&&
             !i.active_completed_resource_operation&&!i.category_source_attempted&&!i.category_source&&!i.global_color_bank,
             "Canonical input lost its own Header/phase");
        const auto& state=load_scope.owned;const auto& field=field_scope.owner;
        need(same(receiver,receiver_control)&&receiver->state_.get()==state.get()&&
             state->thread==i.thread&&!state->closed.load()&&!state->failed.load()&&state->busy.load()&&
             state->receiver&&state->receiver->state_.get()==field.get()&&field->thread==i.thread&&
             !field->closed.load()&&!field->failed.load()&&field->busy.load()&&field->copy_attempted&&
             state->active_canonical_input==this&&state->active_canonical_input_generation==generation&&
             field->active_canonical_input==this&&field->active_canonical_input_generation==generation&&
             !state->descriptor_bytes&&!state->parsed_xml&&!state->descriptor_core&&!state->active_resource_loan_address&&
             state->actual_lookup_source&&!state->actual_lookup_source->owner_before(warm_control)&&
             !warm_control.owner_before(*state->actual_lookup_source)&&
             same(warm.discovery_actor_,discovery_control)&&warm.discovery_actor_.get()==actual_discovery&&
             same(source,source_control)&&source->thread==i.thread&&
             state->filename.lock().get()==source.get()&&field->filename.lock().get()==source.get()&&
             source->actor.lock().get()==actual_discovery&&state->discovery.lock().get()==actual_discovery&&
             !state->filename.owner_before(source_control)&&!source_control.owner_before(state->filename)&&
             !field->filename.owner_before(source_control)&&!source_control.owner_before(field->filename)&&
             !state->discovery.owner_before(discovery_control)&&!discovery_control.owner_before(state->discovery)&&
             !source->actor.owner_before(discovery_control)&&!discovery_control.owner_before(source->actor),
             "Canonical input changed physical Load/Field/Source/lookup/scope generation");
        warm.discovery_actor_->state_->live();state->live();
        const auto& ds=*warm.discovery_actor_->state_;
        need(ds.origin==BnwNativeResourceDiscoveryActor::State::Origin::returned_common_warm&&
             !ds.warm.owner_before(warm_control)&&!warm_control.owner_before(ds.warm)&&
             ordinal<ds.issued.size()&&same(ds.issued[ordinal],source_control)&&ds.issued[ordinal].get()==source.get()&&
             ordinal<warm.installed_descriptor_rows_.size()&&same(warm.installed_descriptor_rows_[ordinal],row_control)&&
             warm.installed_descriptor_rows_[ordinal].get()==row.get()&&row->ordinal==ordinal&&
             source->native_acp_filename==row->filename&&
             std::any_of(warm.preload_receivers_.begin(),warm.preload_receivers_.end(),[&](const auto& p){
                 return p.get()==receiver.get()&&same(p,receiver_control);}),
             "Canonical input lost actual installed row/factory ancestry");
        selected_identity(); // SAME current maps/FIRST; no unused owned quote.
        need(warm.parent_pins_>0&&i.self.use_count()-static_cast<long>(i.directory_parent_internal_pin_count())>
             warm.parent_pins_+i.header_callback_state.internal_owner_pin_count(),"Canonical input lost outside Parent");
        if(input_address)need(input.data()==input_address&&field->field0==input,
             "Canonical input physical operand changed");
    }
    void refresh() {
        identity();load_scope.owned->current();identity(); // Genuine Source/Boot/Common callbacks retained.
        i.validate(true,false);identity(); // Fresh namespace POST after the last whole callback FULL.
    }
    std::unique_ptr<BnwNativeDescriptorSelectedStreamLoan> select() {
        refresh();input=field_scope.owner->field0;input_address=input.c_str();
        identity(); // Held physical input/address and source custody after own copy.
        std::unique_ptr<BnwNativeDescriptorSelectedStreamLoan> selected;std::size_t count=0;
        {
            Query query(*this);identity();
            if(warm.busy_.exchange(true)){warm.failed_.store(true);throw std::runtime_error("Canonical input actual query reentry");}
            struct Busy {BnwNativeGlobalResourceWarmOwner& w;~Busy(){w.busy_.store(false);}} busy{warm};
            const std::function<void(const BnwNativeArchiveRecordDiagnostic*,const std::u16string*,const std::string*)> target=
                [&](const auto* native,const auto* prefix,const auto* first){
                    identity();need(count++==0&&native,"Descriptor actual query MISS has no selected stream; filesystem fallback supplier remains open");
                    ParentPin retained(warm);
                    selected=retained.parent->lend_native_descriptor_stream_source(warm,native,prefix,first,input);
                };
            // The private call carries THIS real Query address. Generic with_native_record always passes null.
            ParentPin retained(warm);
            retained.parent->with_precategory_native_resource_record(warm,input.c_str(),target,&query);
            identity();need(count==1&&selected,"Descriptor actual query did not issue exactly one stream loan");
        } // Query and copied target retire BEFORE the following fresh FULL.
        refresh();complete=true;return selected;
    }
    ~PreselectionCanonicalInputLoan(){retire();if(!complete)poison();}
    PreselectionCanonicalInputLoan(const PreselectionCanonicalInputLoan&)=delete;
};

void detail::BnwNativeGlobalResourceWarmOwner::require_preselection_canonical_query_without_callbacks(
        const PreselectionCanonicalInputLoan& loan,const void* invocation,const char* raw)const {
    loan.identity();
    need(&loan.warm==this&&active_preselection_canonical_input_==&loan&&busy_.load()&&
         invocation&&loan.active_query==invocation&&loan.active_query->creator==thread_&&
         &loan.active_query->loan==&loan&&raw==loan.input_address,
         "Canonical input query lacks the actual private invocation/physical operand");
}

std::unique_ptr<detail::BnwNativeDescriptorSelectedStreamLoan>
detail::BnwNativeGlobalResourceWarmOwner::select_preselection_canonical_input(
        const std::shared_ptr<BnwNativePackageLoadActor>& receiver,const ParentPin& retained) {
    require_current();need(receiver&&receiver->state_->receiver,"Canonical input physical receiver absent");
    need(&retained.source==this,"Canonical input actual issuer Parent pin differs");
    auto& i=*retained.parent->impl_;
    const auto state=receiver->state_;
    const auto held_field_state=state->receiver->state_; // Genuine receiver storage, never an outside facade grant.
    const auto* const actual_field=state->receiver.get();
    const auto* const actual_field_state=actual_field->state_.get();
    const auto source=state->filename;const auto discovery=state->discovery;
    const auto* const actual_source=state->filename.lock().get();
    const auto* const actual_discovery=discovery_actor_.get();
    const std::weak_ptr<const void> wc=i.live_resource_query,rc=receiver,ec=i.identity,
        dc=i.default_state,fc=i.first_lineage,lc=i.lease,sc=i.settings,cc=i.current_default_source;
    const auto revision=i.registry.revision;const auto enabled=i.default_state->live_enabled89;
    const auto* const directory=i.active_directory_parent_loan;
    const auto* const installation=i.active_installation_parent_loan;
    const auto pins=i.directory_parent_internal_pin_count();
    std::array<const BnwNativeCommon13Step*,13> common{};
    need(i.common_steps.size()==common.size(),"Canonical input retired journal absent");
    for(std::size_t n=0;n<common.size();++n)common[n]=i.common_steps[n].get();
    std::unique_ptr<BnwNativeDescriptorSelectedStreamLoan> result;
    try {
        {
            BnwNativePackageLoadActor::State::Scope load(state);
            BnwNativePackageFStringField0::State::Scope field(state->receiver->state_);
            PreselectionCanonicalInputLoan loan(*this,receiver,i,load,field);
            result=loan.select();
        } // Loan/input/Source/Header/Load/Field scopes retire before any stream construction.
        need(!i.header_active&&!i.active_header_fence&&!i.active_live_query_owner&&!active_preselection_canonical_input_&&
             !state->busy.load()&&!state->active_canonical_input&&state->receiver&&
             !state->receiver->state_->busy.load()&&!state->receiver->state_->active_canonical_input,
             "Canonical input actual scopes failed to retire");
        state->current(); // Fresh real Source + namespace FULL after ALL query-input target/scope DTORs.
        i.validate(false,false); // Fresh namespace POST after the final State.current Boot/Common callback tail.
        need(thread_==std::this_thread::get_id()&&i.thread==thread_,"Canonical input final POST foreign creator");
        const auto same=[](const auto& p,const auto& w){return p&&!p.owner_before(w)&&!w.owner_before(p);};
        need(!closed_.load()&&!failed_.load()&&!i.header_poisoned&&!i.category_lineage_failed&&!i.common_driver_failed&&
             !i.common_pending&&!state->closed.load()&&!state->failed.load()&&
             same(i.live_resource_query,wc)&&i.live_resource_query.get()==this&&same(i.identity,ec)&&epoch_.get()==i.identity.get()&&
             same(receiver,rc)&&receiver->state_.get()==state.get()&&state->receiver.get()==actual_field&&
             actual_field->state_.get()==actual_field_state&&actual_field_state==held_field_state.get()&&
             !actual_field_state->closed.load()&&!actual_field_state->failed.load()&&
             !state->busy.load()&&!actual_field_state->busy.load()&&!state->active_canonical_input&&
             !actual_field_state->active_canonical_input&&!active_preselection_canonical_input_&&!busy_.load()&&
             !state->descriptor_bytes&&!state->parsed_xml&&!state->descriptor_core&&!state->active_resource_loan_address&&
             !i.header_active&&!i.active_header_fence&&!i.active_live_query_owner&&
             !state->filename.owner_before(source)&&!source.owner_before(state->filename)&&
             !actual_field_state->filename.owner_before(source)&&!source.owner_before(actual_field_state->filename)&&
             same(discovery_actor_,discovery)&&discovery_actor_.get()==actual_discovery&&
             state->filename.lock().get()==actual_source&&actual_field_state->filename.lock().get()==actual_source&&
             state->actual_lookup_source&&
             !state->actual_lookup_source->owner_before(wc)&&!wc.owner_before(*state->actual_lookup_source)&&
             same(i.default_state,dc)&&same(i.first_lineage,fc)&&same(i.lease,lc)&&same(i.settings,sc)&&
             same(i.current_default_source,cc)&&i.registry.revision==revision&&i.default_state->live_enabled89==enabled&&
             i.active_directory_parent_loan==directory&&i.active_installation_parent_loan==installation&&
             i.directory_parent_internal_pin_count()==pins&&i.common_steps.size()==common.size()&&
             result&&actual_field_state->field0==result->receiver_acp_,"Canonical input retired during final source POST");
        for(std::size_t n=0;n<common.size();++n)need(i.common_steps[n]&&i.common_steps[n].get()==common[n],
             "Canonical input retired Common journal changed");
        const auto filename=source.lock();need(filename&&discovery_actor_,"Canonical input retired actual filename");
        const auto ordinal=PreselectionCanonicalInputLoan::source_ordinal(*this,*filename);
        need(ordinal<installed_descriptor_rows_.size()&&installed_descriptor_rows_[ordinal]&&
             installed_descriptor_rows_[ordinal]->filename==filename->native_acp_filename&&
             std::any_of(preload_receivers_.begin(),preload_receivers_.end(),[&](const auto& p){return p.get()==receiver.get()&&same(p,rc);}),
             "Canonical input final factory/row membership changed");
        discovery_actor_->state_->live();state->live();
        const auto quote=installed_descriptor_selected_quote(*installed_descriptor_rows_[ordinal]);
        need(quote.filename==installed_descriptor_rows_[ordinal]->selected.filename&&
             i.self.use_count()-static_cast<long>(i.directory_parent_internal_pin_count())>
                 parent_pins_+i.header_callback_state.internal_owner_pin_count(),
             "Canonical input final FIRST/outside Parent changed");
        return result;
    }catch(...){failed_.store(true);state->failed.store(true);held_field_state->failed.store(true);i.header_poisoned=true;throw;}
}
