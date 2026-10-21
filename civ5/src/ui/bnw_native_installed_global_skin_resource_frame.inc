// E2 actual global frame; PRIVATE8k adds the coupled physical driver and real return observer.
struct detail::BnwNativeGlobalResourceWarmOwner::InstalledGlobalSkinEffects
    :std::enable_shared_from_this<InstalledGlobalSkinEffects> {
    using Warm=BnwNativeGlobalResourceWarmOwner;
    using Parent=BnwNativePackageDiscoveryOwner;
    using Loaded=BnwNativeStandalonePackageLoaded;
    using Bank=BnwNativeGlobalColorRegistryBank;
    using Policy=BnwNativeOwnedSkinBootPolicy;
    using Consumer=BnwNativeOwnedSkinConsumer;
    using Kernel=BnwNativeOwnedSkinGlobalSelectionKernel;
    using Common=InstalledInitialSkinEffects;
    using Pc=InstalledPcSkinEffects;
    using Close=InstalledInitialConstraintClose;
    using Selection=InstalledInitialSkinSelection;
    using Input=runtime::BnwSelectedLanguageInputOwner;
    using Boot=BnwNativeResourceBootSettingsOwner;
    using Instance=BnwNativeOwnedUiSkinPoststageInstance;
    using Owner=BnwNativeInstalledGlobalSlotOwner;
    struct ActiveSlot;
    struct SlotReturn {
        std::shared_ptr<Owner> owner;
        bool closed=false,selected_nonbase=false,disabled_changed=false,common_changed=false,pc_changed=false;
        unsigned setter_returns=0;
    };
    std::array<SlotReturn,2> slot_returns;
    std::weak_ptr<ActiveSlot> concrete_slot;
    bool returned=false,native_changed=false,final_reconcile_active=false,final_reconcile_returned=false;
    std::size_t returned_stage_count=0,returned_output_count=0,returned_read_count=0;
    std::vector<Common::FinalInstance> final_instances;
    std::weak_ptr<const void> global_source_control;
    static std::shared_ptr<InstalledGlobalSkinEffects> actual(const Parent&);
    std::shared_ptr<ActiveSlot> actual_slot()const;
    void physical_post(const Parent&)const;
    void outside_identity(const Warm&,const Parent&,const Parent::Impl&)const;
    struct Operation;
    class SlotContext {
        friend class BnwNativeGlobalResourceWarmOwner;
        friend class ::civ5::ui::BnwNativePackageDiscoveryOwner;
        friend struct Operation;
        const std::weak_ptr<InstalledGlobalSkinEffects> owner_;
        const std::thread::id creator_=std::this_thread::get_id();
        const std::uint64_t serial_;
        const unsigned slot_;
      protected:
        SlotContext(const std::shared_ptr<InstalledGlobalSkinEffects>&,std::uint64_t,unsigned);
      public:
        virtual ~SlotContext();
        SlotContext(const SlotContext&)=delete;
        // PURE identity/trace only: no I/O, provider, metadata or owner.current.
        virtual void require_pure_identity()const=0;
        virtual void require_trace_identity(const Consumer&)const=0;
        virtual void require_selected_resource_identity(const BnwNativeOwnedSkinOccurrence&,unsigned)const=0;
        // Future actual implementation MUST use SAME8f1 slot owner/read loan.
        virtual void read_actual_constraints(const Policy&,
            const std::function<void(const std::string&,const std::string&)>&)const=0;
        virtual void observe_actual_set(const Instance&,const std::optional<std::string>&)const=0;
        virtual void require_actual_exclusion(const Instance&,unsigned)const=0;
    };
    const std::thread::id creator=std::this_thread::get_id();
    const std::weak_ptr<const Parent> parent;
    const std::weak_ptr<const void> warm,input,epoch,boot,catalog;
    const std::shared_ptr<Close> closed;
    const std::shared_ptr<Pc> previous;
    const std::shared_ptr<Common> common;
    const std::shared_ptr<Policy> policy;
    const std::shared_ptr<Consumer> consumer;
    const std::weak_ptr<Bank> bank;
    const std::size_t initial_stage_count,initial_output_count,initial_read_count;
    bool failed=false,factory_active=false,kernel_invoking=false,kernel_returned=false,physical_post_returned=false;
    enum class FactoryStep {fresh,checking_pre,pre_checked,checking_post,complete};
    FactoryStep factory_step=FactoryStep::fresh;
    bool factory_borrow_observed=false,borrow_check_active=false;
    std::uint64_t returned_epoch=0;
    const Bank::OwnedSkinOperation* bank_operation_identity=nullptr;
    std::shared_ptr<Kernel> kernel;
    const void* global_source_identity=nullptr;
    std::weak_ptr<Parent::OwnedSkinGlobalEffectsLoan> typed_effects_loan;
    // Updated only by future actual660 return issuer, not by copying a plan.
    std::vector<int> returned_selectors{-1,-1};
    std::optional<int> invoking_selector;
    bool slot_construction_active=false;
    unsigned next_slot=0;
    const SlotContext* active_slot=nullptr;
    Operation* operation=nullptr;
    InstalledGlobalSkinEffects(const Parent::Impl& i,const Warm& source)
        :parent(i.self),warm(i.live_resource_query),input(i.installation_input),epoch(i.identity),
         boot(i.settings),catalog(i.resource_catalog),closed(source.installed_initial_constraint_close_),
         previous(closed?closed->previous:nullptr),common(previous&&previous->publication?previous->publication->common:nullptr),
         policy(closed?closed->policy:nullptr),consumer(previous?previous->consumer:nullptr),
         bank(previous?previous->bank:std::weak_ptr<Bank>{}),
         initial_stage_count(source.owned_category_stages_.size()),
         initial_output_count(source.owned_skin_output_receipts_.size()),initial_read_count(source.owned_skin_read_receipts_.size()) {}
    InstalledGlobalSkinEffects(const InstalledGlobalSkinEffects&)=delete;
    InstalledGlobalSkinEffects& operator=(const InstalledGlobalSkinEffects&)=delete;
    static bool same(const auto& a,const auto& b){return !a.owner_before(b)&&!b.owner_before(a);}
};
struct detail::BnwNativeGlobalResourceWarmOwner::InstalledGlobalSkinEffects::Operation final
    :Common::Continuation,Selection::Continuation {
    InstalledGlobalSkinEffects& data;
    Warm& source;
    const Parent& parent;
    Parent::Impl& i;
    const Parent::Impl::HeaderGuard* header=nullptr;
    const Loaded::Loan* loan=nullptr;
    const Bank::OwnedSkinOperation* bank_operation=nullptr;
    std::uint64_t serial=0;
    Operation(InstalledGlobalSkinEffects& actual,Warm& warm,const Parent& facade,Parent::Impl& storage)
        :data(actual),source(warm),parent(facade),i(storage) {
        need(data.creator==std::this_thread::get_id()&&data.common&&data.common->selection&&
             !data.operation&&!data.common->continuation&&!data.common->selection->continuation,
             "Installed global resource continuation replay/foreign origin");
        data.operation=this;data.common->continuation=this;data.common->selection->continuation=this;
    }
    ~Operation(){
        if(data.active_slot||header||loan||bank_operation){data.failed=true;i.header_poisoned=true;}
        if(data.operation==this&&data.common->continuation==this&&data.common->selection->continuation==this){
            data.common->selection->continuation=nullptr;data.common->continuation=nullptr;data.operation=nullptr;
        }else{data.failed=true;i.header_poisoned=true;}
    }
    Operation(const Operation&)=delete;
    const SlotContext& actual_slot_identity()const {
        const auto* slot=data.active_slot;
        need(slot&&serial&&slot->creator_==data.creator&&slot->serial_==serial&&slot->slot_<2&&slot->slot_==data.next_slot&&
             slot->owner_.lock().get()==&data&&same(slot->owner_,source.installed_global_skin_effects_),
             "Installed global resource requires SAME sealed actual slot context");
        slot->require_pure_identity();return *slot;
    }
    void policy_identity()const {
        const auto& policy=*data.policy;
        need(policy.phase_==Policy::Phase::returned&&policy.installed_initial_close_attempted_&&
             policy.installed_initial_close_call_returned_&&policy.installed_constraints_.clear_returned()&&
             policy.installed_constraints_.release_returned()&&!policy.installed_constraints_.has_header()&&
             policy.installed_setters_returned_==3&&policy.actual_platform_=="PC"&&
             policy.installed_pc_publication_returned_&&policy.installed_pc_constraint_returned_&&
             policy.installed_pc_selection_returned_&&!policy.installed_global_read_failed_&&
             !policy.global_transition_active_,"Installed global lost real closed initial constraints/PC history");
    }
    void identity()const {
        need(data.creator==std::this_thread::get_id()&&source.thread_==data.creator&&i.thread==data.creator&&
             !data.failed&&!data.returned&&!source.closed_.load()&&!source.failed_.load()&&!i.header_poisoned&&!i.category_lineage_failed&&
             data.operation==this&&source.installed_global_skin_effects_.get()==&data&&source.installed_global_skin_effects_attempted_,
             "Installed global resource controller retired/foreign/unregistered");
        need(data.closed&&data.previous&&data.previous->publication&&data.common&&data.common->selection&&
             data.common->selection->color&&data.policy&&data.consumer,
             "Installed global resource lacks genuine E1/PC/Common ancestry");
        const auto& closed=*data.closed;const auto& pc=*data.previous;const auto& common=*data.common;
        const auto& selection=*common.selection;const auto& color=*selection.color;
        need(i.installation_input&&!i.lease&&!i.rules&&i.live_resource_query.get()==&source&&same(i.self,data.parent)&&
             same(i.live_resource_query,data.warm)&&same(i.installation_input,data.input)&&same(i.identity,data.epoch)&&
             i.settings&&same(i.settings,data.boot)&&same(i.resource_catalog,data.catalog)&&
             data.closed.get()==source.installed_initial_constraint_close_.get()&&closed.returned&&!closed.failed&&
             !closed.operation&&!closed.closing&&closed.call_returned&&closed.previous.get()==data.previous.get()&&
             data.previous.get()==source.installed_pc_skin_effects_.get()&&pc.returned&&!pc.failed&&!pc.operation&&
             pc.kernel_returned&&pc.physical_post_returned&&pc.finish_call_returned&&pc.publication&&
             pc.publication->returned&&!pc.publication->failed&&!pc.publication->operation&&
             pc.publication->common.get()==data.common.get()&&data.common.get()==source.installed_initial_skin_effects_.get()&&
             common.returned&&!common.failed&&!common.operation&&common.kernel_returned&&common.continuation==this&&
             data.initial_stage_count==pc.returned_stage_count&&data.initial_output_count==pc.returned_output_count&&
             data.initial_read_count==pc.returned_read_count&&source.owned_category_stages_.size()>=data.initial_stage_count&&
             source.owned_skin_output_receipts_.size()>=data.initial_output_count&&source.owned_skin_read_receipts_.size()>=data.initial_read_count&&
             selection.continuation==this&&&selection==source.installed_initial_skin_selection_.get()&&
             selection.prefix_returned&&!selection.failed&&!selection.operation&&selection.counter_returned&&selection.enabled_instances==0&&
             color.returned&&!color.failed&&!color.operation&&!color.constructor_loan&&!color.active_base_read&&
             color.admission&&color.admission->returned&&!color.admission->failed&&
             !i.active_completed_resource_operation&&!i.category_source&&!source.active_owned_category_preconstructor_,
             "Installed global resource substituted closed history/canonical inputs or revived old operation");
        color.admission->identity(source,parent,i); // Pure canonical outside-Parent accounting, no retired callbacks.
        need(data.policy.get()==closed.policy.get()&&data.policy.get()==pc.policy.get()&&data.policy.get()==common.policy.get()&&
             data.policy.get()==source.owned_skin_boot_policy_.get()&&same(selection.policy,data.policy)&&
             data.policy->installed_source_&&data.policy->installed_source_->lock().get()==static_cast<const Policy::InstalledSource*>(common.selection.get())&&
             same(*data.policy->installed_source_,common.selection)&&same(data.policy->input_,selection.language)&&same(data.policy->boot_,data.boot)&&
             data.policy->lease_.expired()&&common.consumer.get()==data.consumer.get()&&same(common.consumer,data.consumer)&&
             same(common.continuation_consumer,data.consumer)&&same(common.continuation_effects_source,data.consumer->source_)&&
             !common.continuation_effects_source.expired()&&data.consumer->thread_==data.creator&&!data.consumer->failed_&&
             data.consumer->source_.get()==common.source_identity&&common.source_identity==source.owned_skin_effect_source_identity_&&
             common.returned_epoch==source.owned_skin_effect_operation_epoch_&&common.bank_operation_identity==source.owned_skin_effect_bank_operation_&&
             source.owned_skin_instance_ctor_driver_returned_&&source.owned_skin_occurrence_driver_returned_&&
             source.owned_skin_occurrences_.size()==source.owned_skin_instances_.size()&&
             color.constructors.size()==source.owned_skin_instances_.size()&&!source.owned_skin_base_game_consumer_&&!source.owned_skin_global_kernel_,
             "Installed global resource lost original actual Source/Consumer/control or changed saved/old epoch");
        if(data.active_slot)actual_slot_identity().require_trace_identity(*data.consumer);
        else need(!data.consumer->active_&&data.consumer->completed_slot_selectors_==data.returned_selectors,
             "Installed global resource has no actual inactive Consumer return journal");
        if(data.kernel) {
            const auto global=data.global_source_control.lock();
            need(global&&global.get()==data.global_source_identity&&same(global,data.kernel->source_)&&
                 data.global_source_identity&&data.kernel->source_.get()==data.global_source_identity&&
                 data.kernel->thread_==data.creator&&!data.kernel->failed_,"Installed global changed actual Kernel/Source control");
            if(data.kernel_returned)need(!data.kernel->active_&&data.kernel->normal_returned_arguments_==std::vector<int>{-1},
                 "Installed global kernel flag has no actual whole4691 return");
        }else need(!data.kernel_invoking&&!data.kernel_returned,"Installed global frame manufactured absent kernel return");
        const auto bank=data.bank.lock();
        need(bank&&i.global_color_bank.get()==bank.get()&&source.owned_bank_receiver_.lock().get()==bank.get()&&
             !bank->closed_.load()&&bank->phase_==Bank::Phase::roster_bound&&
             bank->owned_source_.get()==source.owned_bank_source_identity_&&bank->owned_roster_.size()==source.owned_skin_instances_.size(),
             "Installed global resource lost actual Bank/roster");
        for(std::size_t n=0;n<source.owned_skin_instances_.size();++n){const auto instance=source.owned_skin_instances_[n];
            need(instance&&source.owned_skin_occurrences_[n]&&bank->owned_roster_[n].lock().get()==instance.get()&&
                 color.constructors[n].instance.lock().get()==instance.get()&&
                 color.constructors[n].occurrence.lock().get()==source.owned_skin_occurrences_[n].get()&&
                 color.constructors[n].source==instance->constructor_source_.get(),"Installed global changed actual constructor roster identity");
        } // Old enabled/pair/+89 snapshots are history; actual global effects may change current values.
        policy_identity();
        if(header)need(serial&&i.header_active&&i.active_header_fence==header&&header->valid()&&i.active_live_query_owner==&source&&
             source.active_owned_skin_operation_epoch_==serial&&source.owned_skin_operation_epoch_==serial&&
             data.returned_epoch==serial&&serial>pc.returned_epoch,"Installed global lost NEW genuine physical Header/epoch");
        else need(!serial&&!i.header_active&&!i.active_live_query_owner&&!source.active_owned_skin_operation_epoch_,
             "Installed global borrowed retired/unrelated Header");
        if(loan){need(header&&source.active_owned_skin_resource_loan_==&loan->resource&&
             source.active_owned_skin_package_.lock().get()==&loan->token&&
             source.active_owned_skin_resource_epoch_==loan->actor->state_->active_resource_loan,
             "Installed global lost NEW actual physical ResourceLoan");loan_identity(*loan,0);}
        else need(!source.active_owned_skin_resource_loan_,"Installed global orphaned resource loan");
        if(bank_operation)need(header&&loan&&bank->busy_.load()&&bank->active_owned_skin_operation_==bank_operation&&
             bank_operation->scope.bank.get()==bank.get()&&data.bank_operation_identity==bank_operation,
             "Installed global lost NEW owning Bank operation");
    }
    void current()const {identity();source.require_current();if(loan)loan->require_current();identity();}
    void require_identity()const override {identity();}
    void require_operation_current()const override {current();}
    void require_actual_global_effects_factory_current(const Policy& policy,const Consumer& consumer,const void* actual_source)const override {
        identity();need(header&&loan&&bank_operation&&data.factory_active&&!data.kernel_invoking&&!data.kernel&&
             data.policy.get()==&policy&&data.consumer.get()==&consumer&&actual_source==data.common->source_identity&&
             !consumer.active_&&consumer.completed_slot_selectors_==std::vector<int>{-1,-1},
             "Installed global factory lacks its actual NEW physical mint scope");
        const auto prior=data.factory_step;
        if(!(prior==InstalledGlobalSkinEffects::FactoryStep::fresh||
             (prior==InstalledGlobalSkinEffects::FactoryStep::pre_checked&&data.factory_borrow_observed))){
            data.failed=true;throw std::runtime_error("Installed global actual factory replay/reentry/missing loan borrow");}
        data.factory_step=prior==InstalledGlobalSkinEffects::FactoryStep::fresh?
            InstalledGlobalSkinEffects::FactoryStep::checking_pre:InstalledGlobalSkinEffects::FactoryStep::checking_post;
        try {current();need(data.factory_step==(prior==InstalledGlobalSkinEffects::FactoryStep::fresh?
                 InstalledGlobalSkinEffects::FactoryStep::checking_pre:InstalledGlobalSkinEffects::FactoryStep::checking_post),
                 "Installed global factory role changed during currentness");
            data.factory_step=prior==InstalledGlobalSkinEffects::FactoryStep::fresh?
                InstalledGlobalSkinEffects::FactoryStep::pre_checked:InstalledGlobalSkinEffects::FactoryStep::complete;
        }catch(...){data.failed=true;throw;}
    }
    void require_actual_global_effects_borrow_current(const Policy& policy,const Consumer& consumer,const void* actual_source)const override {
        identity();need(header&&loan&&bank_operation&&data.policy.get()==&policy&&data.consumer.get()==&consumer&&
             actual_source==data.common->source_identity&&
             ((data.factory_active&&(data.factory_step==InstalledGlobalSkinEffects::FactoryStep::pre_checked||
                                    data.factory_step==InstalledGlobalSkinEffects::FactoryStep::complete))||
              (data.kernel_invoking&&data.kernel)||data.kernel_returned),
             "Installed global loan lacks SAME actual physical/factory/kernel scope");
        if(data.borrow_check_active){data.failed=true;throw std::runtime_error("Installed global borrow role reentry");}
        data.borrow_check_active=true;
        struct Exit {InstalledGlobalSkinEffects& data;~Exit(){data.borrow_check_active=false;}} exit{data};
        try {current();if(data.factory_active&&data.factory_step==InstalledGlobalSkinEffects::FactoryStep::pre_checked)
                data.factory_borrow_observed=true;
        }catch(...){data.failed=true;throw;}
    }
    void with_actual_global_constraint_read(const Policy& policy,
        const std::function<void(const std::string&,const std::string&)>& input)const override {
        identity();need(header&&loan&&bank_operation&&data.policy.get()==&policy&&data.kernel_invoking&&
             data.kernel&&data.kernel->active_,"Installed global constraint read has no actual coupled kernel");
        const auto* slot=&actual_slot_identity();current();
        // Actual slot implementation must copy target ONLY inside SAME8f1 read loan.
        slot->read_actual_constraints(policy,input);current();
        need(data.active_slot==slot,"Installed global slot changed during actual constraint read");
    }
    void observe_actual_global_set_read(const Instance& instance,const std::optional<std::string>& value)const override {
        identity();need(header&&loan&&bank_operation&&data.kernel_invoking&&data.kernel&&data.kernel->active_,
             "Installed67 observation lacks actual coupled kernel");
        const auto* slot=&actual_slot_identity();current();slot->observe_actual_set(instance,value);current();
        need(data.active_slot==slot,"Installed67 observation changed actual slot");
    }
    void require_actual_global_exclusion_current(const Instance& instance,unsigned actual_slot)const override {
        identity();need(header&&loan&&bank_operation&&data.kernel_invoking&&data.kernel&&data.kernel->active_,
             "Installed global exclusion lacks actual coupled kernel");
        const auto* slot=&actual_slot_identity();current();slot->require_actual_exclusion(instance,actual_slot);current();
        need(data.active_slot==slot,"Installed global exclusion changed actual slot");
    }
    void require_selected_resource_current(const BnwNativeOwnedSkinOccurrence& occurrence,unsigned actual_slot)const {
        identity();need(header&&loan&&bank_operation&&data.kernel_invoking&&data.kernel&&data.kernel->active_&&
             data.consumer->active_&&data.invoking_selector&&*data.invoking_selector==static_cast<int>(actual_slot),
             "Installed global resource issuer is not actual selected660 invocation");
        const auto* slot=&actual_slot_identity();slot->require_selected_resource_identity(occurrence,actual_slot);current();
        need(data.active_slot==slot&&slot->slot_==actual_slot,"Installed global selected resource changed slot");
    }
        void loan_identity(const Loaded::Loan& held,std::size_t ordinal)const {
        need(ordinal<data.common->selection->color->admission->rows.size()&&ordinal<source.returned_standalone_packages_.size(),
             "Installed Color/Skin actual loan ordinal escaped factory");
        const auto& row=data.common->selection->color->admission->rows[ordinal];const auto& actual=source.returned_standalone_packages_[ordinal];
        need(held.creator_thread==data.creator&&!held.completed_source&&!held.completed_cold&&
             &held.token==actual.get()&&held.retained.get()==actual.get()&&same(held.retained,row.package)&&
             held.actor&&same(held.actor,row.actor)&&same(actual->actor_,row.actor),
             "Installed Color/Skin substituted actual Loaded/receiver loan");
        const auto& load=held.actor->state_;load->live();
        need(load->busy.load()&&load->active_resource_loan&&load->active_resource_loan_address==&held.resource&&
             same(held.resource.state,load)&&!held.resource.completed_source&&!held.resource.completed_cold&&
             !load->actual_lookup_source&&load->pre_dx9_lookup_source&&load->adopted_warm&&
             same(*load->pre_dx9_lookup_source,i.pre_dx9_discovery)&&same(*load->adopted_warm,i.live_resource_query)&&
             same(load->descriptor_core,row.core)&&same(load->parsed_xml,row.parser)&&same(load->descriptor_bytes,row.reader)&&
             same(load->metadata_input,row.language)&&same(load->filename,row.filename)&&same(load->discovery,row.discovery)&&
             same(load->descriptor_bytes->state_->source,row.stream)&&same(load->returned_loaded,row.package)&&
             load->receiver.get()==actual->actual_receiver_&&load->completed_metadata_identity==row.metadata&&
             load->descriptor_core->state_->fields.get()==row.fields&&&held.resource.actual_fields==row.fields&&
             &held.resource.actual_document==row.document&&row.fields->current_flags()==row.expected_flags&&
             row.fields->current_category_mask()==row.original_mask,
             "Installed Color/Skin lost actual ResourceLoan/Core/Parser/Reader/flags");
        const auto& core=load->descriptor_core->state_;const auto& parser=load->parsed_xml->state_;
        const auto& reader=load->descriptor_bytes->state_;
        need(core->thread==data.creator&&!core->closed.load()&&!core->failed.load()&&core->busy.load()&&
             same(core->self,row.core)&&same(core->parser,row.parser)&&core->fields.get()==row.fields&&
             core->metadata.get()==row.metadata&&core->document_identity==row.document&&
             core->actual_settings&&same(*core->actual_settings,row.language)&&
             parser->thread==data.creator&&!parser->closed.load()&&!parser->failed.load()&&parser->busy.load()&&
             same(parser->self,row.parser)&&same(parser->reader,row.reader)&&parser->document.get()==row.document&&
             reader->thread==data.creator&&!reader->closed.load()&&!reader->failed.load()&&reader->busy.load()&&
             same(reader->reader,row.reader)&&same(reader->source,row.stream)&&reader->buffer,
             "Installed Color/Skin retired held physical Core/Parser/Reader operation");
    }
    void require_actual_inputs_current(const Input& actual,const Boot& boot)const override {
        identity();const auto language=data.common->selection->language.lock();
        need(language&&language.get()==&actual&&i.settings.get()==&boot&&same(i.settings,data.boot),"Installed global new invocation substituted Input/Boot");identity();
    }
    void require_bank_current(const Bank& actual,const content::XmlNode* document,std::uint64_t ordinal)const override {
        identity();need(header&&loan&&!document&&ordinal==0&&data.bank.lock().get()==&actual&&i.global_color_bank.get()==&actual&&
             actual.thread_==data.creator&&actual.epoch_.get()==i.identity.get()&&same(actual.parent_,i.self)&&
             actual.resource_catalog_.get()==i.resource_catalog.get()&&actual.query_default_source_.lock().get()==i.current_default_source.get()&&
             actual.query_default_state_.lock().get()==i.default_state.get()&&actual.query_default_records_==&i.default_state->selected&&
             actual.live_resource_query_.lock().get()==&source,"Installed global Bank borrower lost actual controls");identity();
    }
    void require_effect_source_current(const Policy& actual,const void* actual_source)const override {
        identity();need(header&&loan&&bank_operation&&data.policy.get()==&actual&&actual_source&&
             actual_source==data.common->source_identity&&data.consumer->source_.get()==actual_source,
             "Installed global Source is not SAME retained protected Source under NEW Bank/Loaded owner");
        data.policy->require_current();identity();
    }
    void require_stage_current(const OwnedCategoryStage& stage)const override {
        const auto actual_bank=data.bank.lock();const auto* owning_bank=bank_operation;
        const auto actual_serial=serial;
        const auto owning_scope=[&]{
            identity();need(header&&loan&&actual_bank&&owning_bank&&bank_operation==owning_bank&&
                 actual_serial&&serial==actual_serial&&source.active_owned_skin_operation_epoch_==actual_serial&&
                 actual_bank->busy_.load()&&actual_bank->active_owned_skin_operation_==owning_bank&&
                 owning_bank->scope.bank.get()==actual_bank.get(),
                 "Installed global Stage IO requires SAME live owning Bank PRE/POST");
        };
        owning_scope();require_stage_identity(stage);current();const auto package=source.returned_standalone_packages_[stage.factory_ordinal];
        package->with_actual_descriptor([&](const auto& held){held.require_current();const auto& admission=*data.common->selection->color->admission;
            admission.authenticate(source,*package,held,admission.rows[stage.factory_ordinal]);
            need(&held.resource.actual_document==stage.document,"Installed global Stage actual Parser changed");
            if(stage.category==2){const auto occurrence=stage.skin_occurrence.lock();const auto& doc=held.resource.actual_document;
                need(occurrence&&!doc.roots.empty(),"Installed global Stage lacks real nthUISkin");
                const BnwNativePackageXmlParserOwner::Node* node=nullptr;auto remaining=occurrence->occurrence_ordinal_;
                for(const auto& child:doc.roots.front()->children)if(BnwNativePackageResourceCheckpoint::range(doc,child->name)=="UISkin"){
                    if(remaining){--remaining;continue;}node=child.get();break;}
                need(node==occurrence->node_,"Installed global Stage changed actual nthUISkin");}held.require_current();});
        source.replay_owned_category_stage_io_current(stage);require_stage_identity(stage);owning_scope();
    }
    void require_stage_identity(const OwnedCategoryStage& stage)const override {
        identity();const auto& common=*data.common;const auto& admission=*common.selection->color->admission;
        const auto found=std::find_if(source.owned_category_stages_.begin(),source.owned_category_stages_.end(),[&](const auto& v){return v.get()==&stage;});
        need(header&&loan&&found!=source.owned_category_stages_.end()&&stage.factory_ordinal<source.returned_standalone_packages_.size()&&stage.category<3&&
             stage.package.lock().get()==source.returned_standalone_packages_[stage.factory_ordinal].get()&&stage.document==admission.rows[stage.factory_ordinal].document&&
             stage.source&&stage.namespace_channel&&stage.source->owned_warm_&&stage.namespace_channel->owned_warm_&&
             same(*stage.source->owned_warm_,data.warm)&&same(*stage.namespace_channel->owned_warm_,data.warm)&&
             stage.source->descriptor_==stage.factory_ordinal&&stage.source->category_==stage.category&&stage.source->first_.get()==i.first_lineage.get()&&
             stage.namespace_channel->namespace_.get()==stage.source.get()&&!stage.namespace_channel->lease_&&
             stage.namespace_channel->installation_input_.get()==i.installation_input.get()&&stage.namespace_channel->cold_.get()==i.cold.get()&&
             stage.namespace_channel->first_.get()==i.first_lineage.get()&&stage.namespace_channel->epoch_.get()==i.identity.get(),
             "Installed global Stage lost actual namespace/Loaded/Input/FIRST controls");
        const auto index=static_cast<std::size_t>(found-source.owned_category_stages_.begin());const auto manager=stage.manager.lock();
        if(stage.category<2){const auto categories=common.selection->color->categories;const auto actor=source.returned_standalone_packages_[stage.factory_ordinal]->actor_.lock();
            need(index<common.returned_stage_count&&stage.installed_initial_driver&&!stage.installed_skin_effects_driver&&
                 !stage.installed_pc_skin_effects_driver&&!stage.installed_global_skin_effects_driver&&
                 stage.installed_initial_driver->lock().get()==categories.get()&&same(*stage.installed_initial_driver,categories)&&
                 stage.step==OwnedCategoryStage::Step::name_registered&&manager&&stage.arena&&stage.cache&&stage.loose_source&&stage.selected_source&&
                 actor&&actor->state_->category_slots[stage.category]&&same(*actor->state_->category_slots[stage.category],stage.selected_source)&&
                 std::any_of(categories->returned_calls.begin(),categories->returned_calls.end(),[&](const auto& row){return row.ordinal==stage.factory_ordinal&&row.category==stage.category&&row.stage.lock().get()==&stage;}),
                 "Installed global initial Stage lacks genuine Source6 provenance");
        }else{
            const auto instance=stage.skin_instance.lock();const auto occurrence=stage.skin_occurrence.lock();
            need(!stage.installed_initial_driver&&instance&&occurrence&&stage.skin_slot<2&&stage.skin_member==0&&
                 occurrence->roster_index_<source.owned_skin_instances_.size()&&source.owned_skin_instances_[occurrence->roster_index_].get()==instance.get()&&
                 source.owned_skin_occurrences_[occurrence->roster_index_].get()==occurrence.get()&&
                 common.selection->color->constructors[occurrence->roster_index_].source==instance->constructor_source_.get()&&
                 occurrence->package_.lock().get()==stage.package.lock().get()&&occurrence->document_==stage.document&&
                 occurrence->factory_ordinal_==stage.factory_ordinal&&occurrence->epoch_.lock().get()==i.identity.get(),
                 "Installed global UISkin Stage lost actual constructor/occurrence/slot");
            if(stage.installed_global_skin_effects_driver)need(!stage.installed_skin_effects_driver&&!stage.installed_pc_skin_effects_driver&&
                 index>=data.initial_stage_count&&stage.installed_global_skin_effects_driver->lock().get()==&data&&
                 same(*stage.installed_global_skin_effects_driver,source.installed_global_skin_effects_),
                 "Installed global Stage lost distinct actual new issuer");
            else if(stage.installed_pc_skin_effects_driver)need(!stage.installed_skin_effects_driver&&
                 index>=common.returned_stage_count&&index<data.initial_stage_count&&
                 stage.installed_pc_skin_effects_driver->lock().get()==data.previous.get()&&
                 same(*stage.installed_pc_skin_effects_driver,data.previous),"Installed global retained PC Stage lacks real provenance");
            else need(stage.installed_skin_effects_driver&&index<common.returned_stage_count&&
                 stage.installed_skin_effects_driver->lock().get()==&common&&same(*stage.installed_skin_effects_driver,data.common),
                 "Installed global retained Common Stage lacks real provenance");
            if(manager)need(instance->owned_resource_pairs_[stage.skin_slot][0].get()==manager.get()&&stage.arena&&stage.cache&&
                 (stage.selected_source||(source.active_owned_category_stage_==&stage&&stage.step==OwnedCategoryStage::Step::paks_queried)),
                 "Installed global Stage is not actual manager pair/Arena/cache");
            else need(stage.installed_global_skin_effects_driver&&source.active_owned_category_stage_==&stage&&
                 stage.step==OwnedCategoryStage::Step::paks_queried&&!instance->owned_resource_pairs_[stage.skin_slot][0],
                 "Installed global manager is not genuine pending lazy construction");
        }
    }
    void require_read_identity(const void* address)const override {
        identity();const auto& common=*data.common;
        const auto found=std::find_if(source.owned_skin_read_receipts_.begin(),source.owned_skin_read_receipts_.end(),[&](const auto& v){return v.get()==address;});
        need(found!=source.owned_skin_read_receipts_.end()&&header&&loan&&bank_operation,
             "Installed global input lacks actual NEW physical owner/read receipt");
        const auto index=static_cast<std::size_t>(found-source.owned_skin_read_receipts_.begin());const auto& read=**found;
        const auto operation=read.operation;const auto occurrence=operation?operation->occurrence.lock():nullptr;const auto query=operation?operation->query:nullptr;
        if(read.installed_global_driver)need(!read.installed_driver&&!read.installed_pc_driver&&index>=data.initial_read_count&&
             read.installed_global_driver->lock().get()==&data&&same(*read.installed_global_driver,source.installed_global_skin_effects_)&&
             operation&&operation->issue_epoch==serial,"Installed global input lost genuine new slot issuer/epoch");
        else if(read.installed_pc_driver)need(!read.installed_driver&&index>=common.returned_read_count&&index<data.initial_read_count&&
             read.installed_pc_driver->lock().get()==data.previous.get()&&same(*read.installed_pc_driver,data.previous)&&
             operation&&operation->issue_epoch==data.previous->returned_epoch&&operation->restoration_returned,
             "Installed global retained PC input lacks actual restored provenance");
        else need(read.installed_driver&&index<common.returned_read_count&&read.installed_driver->lock().get()==&common&&
             same(*read.installed_driver,data.common)&&operation&&operation->issue_epoch==common.returned_epoch&&operation->restoration_returned,
             "Installed global retained Common input lacks actual restored provenance");
        if(operation&&!operation->restoration_returned)need(data.active_slot&&data.kernel_invoking&&data.kernel&&
             data.kernel->active_&&data.consumer->active_&&data.invoking_selector&&
             *data.invoking_selector==static_cast<int>(operation->slot),
             "Installed global unrestored read lacks actual selected660 slot invocation");
        need(occurrence&&query&&query->reader_bound&&operation->slot<2&&(operation->property=="Style"||operation->property=="Color")&&
             operation->actual_default==i.default_state.get()&&operation->actual_default_source==i.current_default_source.get()&&
             operation->actual_bank_source==source.owned_bank_source_identity_&&query->occurrence.lock().get()==occurrence.get()&&
             query->slot==operation->slot&&query->property==operation->property&&query->filter==0x80&&query->issue_epoch==operation->issue_epoch&&
             query->actual_document==occurrence->document_&&query->actual_node==occurrence->node_&&
             std::find(source.owned_skin_output_receipts_.begin(),source.owned_skin_output_receipts_.end(),query)!=source.owned_skin_output_receipts_.end()&&
             std::find(source.owned_skin_read_operations_.begin(),source.owned_skin_read_operations_.end(),operation)!=source.owned_skin_read_operations_.end()&&
             read.query_ordinal<query->actual_outputs.size()&&query->actual_outputs[read.query_ordinal]==read.emitted_filename&&
             occurrence->factory_ordinal_<source.returned_standalone_packages_.size()&&
             occurrence->package_.lock().get()==source.returned_standalone_packages_[occurrence->factory_ordinal_].get()&&
             occurrence->document_==common.selection->color->admission->rows[occurrence->factory_ordinal_].document&&
             occurrence->roster_index_<source.owned_skin_occurrences_.size()&&source.owned_skin_occurrences_[occurrence->roster_index_].get()==occurrence.get()&&
             (!operation->restoration_returned?(read.installed_global_driver&&source.active_owned_skin_read_operation_.get()==operation.get()&&
                 operation->issue_epoch==serial&&i.default_state->live_enabled89==0):
                 (i.default_state->live_enabled89==1||(source.active_owned_skin_read_operation_&&i.default_state->live_enabled89==0&&
                  source.active_owned_skin_read_operation_->actual_default==i.default_state.get()&&source.active_owned_skin_read_operation_->issue_epoch==serial))),
             "Installed global input lost actual query/ordinal/occurrence/Default toggle affinity");
        if(!read.open_error)need(read.identity.has_value(),"Installed global bytes have no physical stream identity");identity();
    }
};
detail::BnwNativeGlobalResourceWarmOwner::InstalledGlobalSkinEffects::SlotContext::SlotContext(
    const std::shared_ptr<InstalledGlobalSkinEffects>& owner,std::uint64_t serial,unsigned slot)
    :owner_(owner),serial_(serial),slot_(slot) {
    need(owner&&owner->creator==std::this_thread::get_id()&&slot<2&&owner->operation&&
         owner->operation->serial==serial&&serial&&owner->operation->header&&owner->operation->loan&&
         owner->operation->bank_operation&&owner->kernel_invoking&&owner->kernel&&owner->kernel->active_&&
         owner->slot_construction_active&&owner->next_slot==slot&&!owner->active_slot&&
         owner->operation->source.installed_global_skin_effects_.get()==owner.get(),
         "Installed global slot context requires actual creating-slot/kernel/physical frame");
}
detail::BnwNativeGlobalResourceWarmOwner::InstalledGlobalSkinEffects::SlotContext::~SlotContext() {
    // Future concrete scope must clear its binding before destruction. Make a
    // missed exceptional cleanup fail closed without a dangling raw slot pointer.
    if(const auto owner=owner_.lock();owner&&owner->active_slot==this){
        owner->failed=true;owner->active_slot=nullptr;
    }
}
void detail::BnwNativeGlobalResourceWarmOwner::require_installed_global_selected_resource_current(
    const BnwNativeOwnedSkinOccurrence& occurrence,unsigned slot)const {
    need(thread_==std::this_thread::get_id(),"Installed global resource issuer foreign creator");
    const auto data=installed_global_skin_effects_;
    need(data&&data->operation,"Installed global resource has no NEW live physical controller");
    data->operation->require_selected_resource_current(occurrence,slot);
}
