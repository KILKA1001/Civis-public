struct detail::BnwNativeGlobalResourceWarmOwner::InstalledLateSpeechTail final
    :BnwNativeInstalledLateSpeechCoreDonor {
    using Warm=BnwNativeGlobalResourceWarmOwner;
    using Parent=BnwNativePackageDiscoveryOwner;
    using Global=InstalledGlobalSkinEffects;
    using Common=InstalledInitialSkinEffects;
    using Selection=InstalledInitialSkinSelection;
    using Loaded=BnwNativeStandalonePackageLoaded;
    using Bank=BnwNativeGlobalColorRegistryBank;
    using Policy=BnwNativeOwnedSkinBootPolicy;
    using Consumer=BnwNativeOwnedSkinConsumer;
    using Input=runtime::BnwSelectedLanguageInputOwner;
    using Boot=BnwNativeResourceBootSettingsOwner;
    using Instance=BnwNativeOwnedUiSkinPoststageInstance;
    using Owner=BnwNativeInstalledGlobalSlotOwner;
    struct Operation;
    struct ActiveCore;
    struct CoreReturn {
        std::weak_ptr<const void> package,actor,core;
        const void* raw_core=nullptr; // Compared as provenance only; never dereferenced outside the true loan.
        std::uint32_t flags=0;
        std::string path;
        std::shared_ptr<const void> directory_brand;
        bool eligible=false,directory_returned=false,physical_scope_returned=false;
    };
    enum class Phase {born,speech,physical_returned,speech_returned,mappings,returned,failed};
    const std::thread::id creator=std::this_thread::get_id();
    const std::weak_ptr<const Parent> parent;
    const std::weak_ptr<const void> warm,input,epoch,boot,catalog;
    const std::shared_ptr<Global> global;
    const std::shared_ptr<Common> common;
    const std::shared_ptr<Selection> selection;
    const std::weak_ptr<Bank> bank;
    const std::vector<std::shared_ptr<const Loaded>>* const actual_table;
    const std::size_t table_count;
    std::size_t next_core=0;
    std::vector<CoreReturn> core_returns;
    const void* namespace_identity=nullptr;
    Phase phase=Phase::born;
    bool failed=false,begin_returned=false,physical_post_returned=false;
    enum class WholeLoadedClose {pending,empty_actual_table,real_loan_returned};
    WholeLoadedClose whole_loaded_close=WholeLoadedClose::pending;
    bool header_returned=false,continuations_returned=false;
    bool mappings_attempted=false,mappings_returned=false;
    bool validating_return=false;
    const void* active_mapping_frame=nullptr;
    std::uint64_t speech_epoch=0;
    Operation* operation=nullptr;
    ActiveCore* active_core=nullptr;
    InstalledLateSpeechTail(const Parent::Impl&,const Warm&);
    InstalledLateSpeechTail(const InstalledLateSpeechTail&)=delete;
    static bool same(const auto& a,const auto& b){return !a.owner_before(b)&&!b.owner_before(a);}
    void global_history_without_callbacks(const Warm&,const Parent&,const Parent::Impl&)const;
    void base_identity_without_callbacks(const Warm&,const Parent&,const Parent::Impl&)const;
    void outside_speech_identity(const Warm&,const Parent&,const Parent::Impl&)const;
    void physical_post(const Parent&)const;
    void require_operation_identity_without_callbacks(const Parent&)const override;
    void require_active_core_identity_without_callbacks(const Parent&)const override;
    void require_active_core_current(const Parent&)const override;
    const void* active_core_identity_without_callbacks(const Parent&)const override;
    const std::shared_ptr<const void>& active_directory_occurrence_brand_without_callbacks(const Parent&)const override;
    const std::string& emitted_62ec20_path_without_callbacks(const Parent&)const override;
    void require_speech_return_without_callbacks(const Parent&)const override;
};

struct detail::BnwNativeGlobalResourceWarmOwner::InstalledLateSpeechTail::Operation final
    :Common::Continuation,Selection::Continuation {
    InstalledLateSpeechTail& data;
    Warm& source;
    const Parent& parent;
    const std::shared_ptr<Parent::Impl> storage;
    Parent::Impl& i;
    const Parent::Impl::HeaderGuard* header=nullptr;
    const Loaded::Loan* loan=nullptr; // Whole first-Loaded physical anchor, distinct from the current Core loan.
    const Bank::OwnedSkinOperation* bank_operation=nullptr;
    std::uint64_t serial=0;
    bool attached=false;
    Operation(InstalledLateSpeechTail&,Warm&,const Parent&);
    ~Operation();
    Operation(const Operation&)=delete;
    void attach_after_actual_begin();
    void identity()const;
    void current()const;
    void loan_identity(const Loaded::Loan&,std::size_t)const;
    void require_identity()const override;
    void require_operation_current()const override;
    void require_stage_identity(const OwnedCategoryStage&)const override;
    void require_stage_current(const OwnedCategoryStage&)const override;
    void require_read_identity(const void*)const override;
    void require_bank_current(const Bank&,const content::XmlNode*,std::uint64_t)const override;
    void require_effect_source_current(const Policy&,const void*)const override;
    void require_actual_inputs_current(const Input&,const Boot&)const override;
    // All factory/constraint/setter/exclusion/660/4691 roles retain their real
    // base defaults that refuse. This continuation issues no new Skin effect.
};

struct detail::BnwNativeGlobalResourceWarmOwner::InstalledLateSpeechTail::ActiveCore final {
    Operation& operation;
    const Loaded::Loan& loan;
    const std::size_t ordinal;
    std::uint32_t flags=0;
    const void* raw_core=nullptr;
    std::shared_ptr<const void> directory_brand;
    std::string emitted;
    bool flags_observed=false,emission_returned=false,directory_returned=false,body_returned=false;
    ActiveCore(Operation&,const Loaded::Loan&);
    ~ActiveCore();
    ActiveCore(const ActiveCore&)=delete;
    void identity()const;
    void current()const;
    void emit_actual_62ec20();
    void execute_actual_branch();
};

void detail::BnwNativeGlobalResourceWarmOwner::InstalledLateSpeechTail::global_history_without_callbacks(
    const Warm& source,const Parent& actual,const Parent::Impl& i)const {
    // Pure unchanged8k provenance only. NEW live/outside scope is separately
    // authenticated by this Tail; no old controller/current callback is used.
    const auto& before=*this->global;

    need(before.creator==std::this_thread::get_id()&&source.thread_==before.creator&&i.thread==before.creator&&!before.failed&&
         !source.closed_.load()&&!source.failed_.load()&&!i.header_poisoned&&!i.category_lineage_failed,
         "Installed global outside observation lost creator/current source");
    need(source.installed_global_skin_effects_.get()==global.get()&&global->returned&&
         source.installed_global_skin_effects_attempted_&&!global->operation&&!global->active_slot&&
         global->concrete_slot.expired()&&!global->slot_construction_active&&!global->factory_active&&
         !global->kernel_invoking&&global->kernel_returned&&global->physical_post_returned&&
         global->final_reconcile_returned&&!global->final_reconcile_active&&global->next_slot==2&&
         global->factory_step==Global::FactoryStep::complete&&global->factory_borrow_observed&&
         !global->borrow_check_active&&!global->invoking_selector,
         "Late Speech lost genuine retired8k execution history");
    need(i.installation_input&&!i.lease&&!i.rules&&i.live_resource_query.get()==&source&&same(i.self,before.parent)&&
         same(i.live_resource_query,before.warm)&&same(i.installation_input,before.input)&&same(i.identity,before.epoch)&&
         i.settings&&same(i.settings,before.boot)&&same(i.resource_catalog,before.catalog)&&before.closed&&before.previous&&before.common&&
         source.installed_initial_constraint_close_.get()==before.closed.get()&&before.closed->returned&&!before.closed->failed&&!before.closed->operation&&
         before.closed->call_returned&&!before.closed->closing&&before.closed->previous.get()==before.previous.get()&&
         before.previous.get()==source.installed_pc_skin_effects_.get()&&before.previous->returned&&!before.previous->failed&&!before.previous->operation&&
         before.previous->kernel_returned&&before.previous->physical_post_returned&&before.previous->finish_call_returned&&
         before.previous->publication&&before.previous->publication->returned&&!before.previous->publication->failed&&!before.previous->publication->operation&&
         before.previous->publication->common.get()==before.common.get()&&source.installed_initial_skin_effects_.get()==before.common.get()&&
         before.common->returned&&!before.common->failed&&!before.common->operation&&before.common->continuation==(operation&&operation->attached?operation:nullptr)&&before.common->kernel_returned&&
         before.common->selection&&before.common->selection.get()==source.installed_initial_skin_selection_.get()&&
         before.common->selection->prefix_returned&&!before.common->selection->failed&&!before.common->selection->operation&&before.common->selection->continuation==(operation&&operation->attached?operation:nullptr)&&
         before.common->selection->counter_returned&&before.common->selection->enabled_instances==0&&before.common->selection->color&&
         before.common->selection->color->returned&&!before.common->selection->color->failed&&!before.common->selection->color->operation&&
         !before.common->selection->color->constructor_loan&&!before.common->selection->color->active_base_read&&
         before.common->selection->color->admission&&before.common->selection->color->admission->returned&&!before.common->selection->color->admission->failed,
         "Installed global outside observation changed genuine E1/PC/Common ancestry");
    before.common->selection->color->admission->identity(source,actual,i); // Pure canonical outside-Parent accounting.
    need(before.returned_epoch&&source.owned_skin_operation_epoch_==(speech_epoch?speech_epoch:before.returned_epoch)&&(!speech_epoch||speech_epoch>before.returned_epoch)&&before.returned_epoch>before.previous->returned_epoch&&
         before.bank_operation_identity&&source.owned_skin_effect_operation_epoch_==before.common->returned_epoch&&
         source.owned_skin_effect_bank_operation_==before.common->bank_operation_identity&&
         source.owned_skin_effect_source_identity_==before.common->source_identity&&before.consumer&&before.consumer.get()==before.common->consumer.get()&&
         same(before.consumer,before.common->consumer)&&same(before.consumer,before.common->continuation_consumer)&&
         same(before.common->continuation_effects_source,before.consumer->source_)&&!before.common->continuation_effects_source.expired()&&
         before.consumer->source_.get()==before.common->source_identity&&before.consumer->thread_==before.creator&&!before.consumer->active_&&!before.consumer->failed_&&
         before.consumer->completed_slot_selectors_==before.returned_selectors&&before.policy&&before.policy.get()==before.closed->policy.get()&&
         before.policy.get()==before.previous->policy.get()&&before.policy.get()==before.common->policy.get()&&before.policy.get()==source.owned_skin_boot_policy_.get()&&
         before.policy->installed_source_&&before.policy->installed_source_->lock().get()==static_cast<const Policy::InstalledSource*>(before.common->selection.get())&&
         same(*before.policy->installed_source_,before.common->selection)&&same(before.common->selection->policy,before.policy)&&
         same(before.policy->input_,before.common->selection->language)&&same(before.policy->boot_,before.boot)&&before.policy->lease_.expired()&&
         before.policy->phase_==Policy::Phase::returned&&before.policy->actual_platform_=="PC"&&before.policy->installed_setters_returned_==3&&
         before.policy->installed_pc_publication_returned_&&before.policy->installed_pc_constraint_returned_&&before.policy->installed_pc_selection_returned_&&
         before.policy->installed_initial_close_attempted_&&before.policy->installed_initial_close_call_returned_&&
         before.policy->installed_constraints_.clear_returned()&&before.policy->installed_constraints_.release_returned()&&
         !before.policy->installed_constraints_.has_header()&&!before.policy->installed_global_read_failed_&&
         !before.policy->comparison_active_&&!before.policy->global_transition_active_&&i.default_state->live_enabled89==1&&
         !source.owned_skin_base_game_consumer_&&!source.owned_skin_global_kernel_,
         "Installed global outside observation substituted original controls/PC/Default history");
    const auto global=before.global_source_control.lock();
    need(global&&before.kernel&&before.kernel->thread_==before.creator&&!before.kernel->failed_&&!before.kernel->active_&&
         before.kernel->normal_returned_arguments_.size()==1&&before.kernel->normal_returned_arguments_.front()==-1&&before.kernel->source_.get()==before.global_source_identity&&
         global.get()==before.global_source_identity&&same(global,before.kernel->source_)&&!before.typed_effects_loan.expired(),
         "Installed global outside observation lacks actual whole Kernel/Source control return");
    const auto actual_bank=before.bank.lock();
    need(actual_bank&&i.global_color_bank.get()==actual_bank.get()&&source.owned_bank_receiver_.lock().get()==actual_bank.get()&&
         !actual_bank->closed_.load()&&
         actual_bank->phase_==Bank::Phase::roster_bound&&actual_bank->owned_source_.get()==source.owned_bank_source_identity_&&
         actual_bank->owned_roster_.size()==source.owned_skin_instances_.size()&&
         source.owned_skin_instance_ctor_driver_returned_&&source.owned_skin_occurrence_driver_returned_&&
         source.owned_skin_occurrences_.size()==source.owned_skin_instances_.size()&&
         before.final_instances.size()==source.owned_skin_instances_.size()&&
         before.common->selection->color->constructors.size()==source.owned_skin_instances_.size()&&
         before.initial_stage_count==before.previous->returned_stage_count&&before.initial_output_count==before.previous->returned_output_count&&
         before.initial_read_count==before.previous->returned_read_count&&before.returned_stage_count==source.owned_category_stages_.size()&&
         before.returned_output_count==source.owned_skin_output_receipts_.size()&&before.returned_read_count==source.owned_skin_read_receipts_.size()&&
         before.returned_stage_count>=before.initial_stage_count&&before.returned_output_count>=before.initial_output_count&&before.returned_read_count>=before.initial_read_count,
         "Installed global closed Bank/actual returned roster/read/namespace history changed");
    std::size_t trace=2;need(before.returned_selectors.size()>=2&&before.returned_selectors[0]==-1&&before.returned_selectors[1]==-1,
         "Installed global lost actual two-call prefix journal");
    for(unsigned slot=0;slot<2;++slot){const auto& row=before.slot_returns[slot];const auto& owner=row.owner;
        need(row.closed&&owner&&owner->thread_==before.creator&&owner->slot_==slot&&!owner->failed_&&
             owner->phase_==Owner::Phase::closed&&owner->body_attempted_&&owner->body_returned_&&owner->closed_returned_&&
             !owner->body_active_&&!owner->busy_&&!owner->active_read_&&!owner->constraints_&&!owner->indices_&&owner->control_.expired()&&
             row.setter_returns==(row.selected_nonbase?9u:3u),"Installed global temporary owners did not actually close/retire");
        if(row.selected_nonbase){need(before.returned_selectors.size()>=trace+2&&before.returned_selectors[trace]==static_cast<int>(slot)&&
             before.returned_selectors[trace+1]==static_cast<int>(slot),"Installed global optional660 journal changed");trace+=2;}
        else need(!row.common_changed&&!row.pc_changed,"Installed global absent660 manufactured bool observation");
    }
    const auto& last=before.slot_returns[1];need(trace==before.returned_selectors.size()&&before.native_changed==(last.disabled_changed||last.common_changed||last.pc_changed),
         "Installed global actual journal/last-slot return observation changed");
    for(std::size_t n=0;n<before.final_instances.size();++n){const auto instance=source.owned_skin_instances_[n];const auto& row=before.final_instances[n];
        const auto& ctor=before.common->selection->color->constructors[n];
        need(instance&&source.owned_skin_occurrences_[n]&&row.instance.lock().get()==instance.get()&&row.enabled==instance->enabled_&&
             actual_bank->owned_roster_[n].lock().get()==instance.get()&&ctor.instance.lock().get()==instance.get()&&
             ctor.occurrence.lock().get()==source.owned_skin_occurrences_[n].get()&&ctor.source==instance->constructor_source_.get(),
             "Installed global returned actual constructor/instance state changed");
        for(unsigned slot=0;slot<2;++slot)for(unsigned member=0;member<2;++member){const auto manager=instance->owned_resource_pairs_[slot][member];
            need(same(row.pairs[slot][member],manager)&&row.pairs[slot][member].lock().get()==manager.get()&&
                 (manager?row.current89[slot][member]==manager->current89:!row.current89[slot][member]),
                 "Installed global returned actual manager pair/+89 changed");}
    }
}

detail::BnwNativeGlobalResourceWarmOwner::InstalledLateSpeechTail::InstalledLateSpeechTail(
    const Parent::Impl& i,const Warm& source)
    :parent(i.self),warm(i.live_resource_query),input(i.installation_input),epoch(i.identity),
     boot(i.settings),catalog(i.resource_catalog),global(source.installed_global_skin_effects_),
     common(global?global->common:nullptr),selection(common?common->selection:nullptr),
     bank(global?global->bank:std::weak_ptr<Bank>{}),actual_table(&source.returned_standalone_packages_),
     table_count(source.returned_standalone_packages_.size()){}

void detail::BnwNativeGlobalResourceWarmOwner::InstalledLateSpeechTail::base_identity_without_callbacks(
    const Warm& source,const Parent& actual,const Parent::Impl& i)const {
    need(creator==std::this_thread::get_id()&&source.thread_==creator&&i.thread==creator&&
         !failed&&phase!=Phase::failed&&!source.closed_.load()&&!source.failed_.load()&&
         !i.header_poisoned&&!i.category_lineage_failed&&source.installed_late_speech_tail_.get()==this&&
         source.installed_late_speech_tail_attempted_&&i.installation_input&&!i.lease&&!i.rules&&
         i.live_resource_query.get()==&source&&same(i.self,parent)&&!parent.expired()&&
         same(i.live_resource_query,warm)&&same(i.installation_input,input)&&same(i.identity,epoch)&&
         i.settings&&same(i.settings,boot)&&same(i.resource_catalog,catalog)&&
         global&&source.installed_global_skin_effects_.get()==global.get()&&global->returned&&
         common&&common.get()==global->common.get()&&selection&&selection.get()==common->selection.get()&&
         selection->color&&selection->color->admission&&
         actual_table==&source.returned_standalone_packages_&&actual_table->size()==table_count&&
         source.early_receiver_driver_returned_&&source.common_gameplay_driver_returned_&&
         next_core<=table_count&&core_returns.size()<=table_count,
         "Late Speech changed genuine source/actual Core table/control");
    const auto& admission=*selection->color->admission;
    need(admission.rows.size()==table_count,"Late Speech changed actual registered Core extent");
    for(std::size_t n=0;n<table_count;++n){const auto& token=(*actual_table)[n];const auto& row=admission.rows[n];
        need(token&&same(token,row.package)&&!row.package.expired()&&same(token->actor_,row.actor)&&
             token->actual_metadata_==row.metadata,"Late Speech substituted real605080 table entry");
    }
    if(begin_returned)need(namespace_identity&&i.installed_late_default.get()==namespace_identity,
         "Late Speech changed SAME actual Default successor control");
    global_history_without_callbacks(source,actual,i);
}

detail::BnwNativeGlobalResourceWarmOwner::InstalledLateSpeechTail::Operation::Operation(
    InstalledLateSpeechTail& value,Warm& owner,const Parent& actual)
    :data(value),source(owner),parent(actual),storage(actual.impl_),i(*storage){
    need(data.creator==std::this_thread::get_id()&&!data.operation&&data.phase==Phase::born&&
         !data.active_core&&data.common&&data.selection&&!data.common->continuation&&!data.selection->continuation,
         "Late Speech operation replay or source substitution");
    data.operation=this;
}

void detail::BnwNativeGlobalResourceWarmOwner::InstalledLateSpeechTail::Operation::attach_after_actual_begin(){
    identity();need(data.begin_returned&&data.namespace_identity&&!attached&&
         !data.common->continuation&&!data.selection->continuation&&!header&&!loan&&!bank_operation,
         "Late Speech resource continuation must follow the real sole8k lender/begin");
    data.common->continuation=this;data.selection->continuation=this;attached=true;data.phase=Phase::speech;
    identity();
}

detail::BnwNativeGlobalResourceWarmOwner::InstalledLateSpeechTail::Operation::~Operation(){
    // Retained Impl owns cleanup even after the genuine facade owner is lost.
    // No provider, Parent acquisition, or departed context callback is made.
    if(data.active_core||header||loan||bank_operation||serial){data.failed=true;i.header_poisoned=true;}
    if(attached){
        if(data.common->continuation==this&&data.selection->continuation==this){
            data.common->continuation=nullptr;data.selection->continuation=nullptr;
            if(data.phase==Phase::physical_returned&&!data.failed)data.continuations_returned=true;
        }else{data.failed=true;i.header_poisoned=true;}
    }
    if(data.operation==this)data.operation=nullptr;else{data.failed=true;i.header_poisoned=true;}
    if(data.failed){data.phase=Phase::failed;source.failed_.store(true);i.header_poisoned=true;}
}

void detail::BnwNativeGlobalResourceWarmOwner::InstalledLateSpeechTail::Operation::identity()const {
    data.base_identity_without_callbacks(source,parent,i);
    need(data.operation==this&&!data.failed&&data.phase!=Phase::speech_returned&&
         data.phase!=Phase::mappings&&data.phase!=Phase::returned,
         "Late Speech actual stack operation is retired or replaced");
    if(attached)need(data.begin_returned&&data.common->continuation==this&&data.selection->continuation==this,
         "Late Speech lost NEW real resource continuation");
    else need(!data.common->continuation&&!data.selection->continuation&&!header&&!loan&&!bank_operation,
         "Late Speech borrowed physical scope before actual begin");
    if(header)need(attached&&serial&&header->valid()&&i.header_active&&i.active_header_fence==header&&
         i.active_live_query_owner==&source&&source.active_owned_skin_operation_epoch_==serial&&
         source.owned_skin_operation_epoch_==serial&&data.speech_epoch==serial&&serial>data.global->returned_epoch,
         "Late Speech lost NEW actual Header/epoch");
    else need(!serial&&!i.header_active&&!i.active_header_fence&&!i.active_live_query_owner&&
         !source.active_owned_skin_operation_epoch_,"Late Speech borrowed a departed/unrelated Header");
    if(loan){
        need(header&&data.table_count&&source.active_owned_skin_resource_loan_==&loan->resource&&
             source.active_owned_skin_package_.lock().get()==&loan->token&&
             source.active_owned_skin_resource_epoch_==loan->actor->state_->active_resource_loan,
             "Late Speech lost real whole first Loaded anchor");loan_identity(*loan,0);
    }else need(!source.active_owned_skin_resource_loan_&&source.active_owned_skin_package_.expired()&&
         !source.active_owned_skin_resource_epoch_,"Late Speech orphaned Loaded anchor");
    if(bank_operation){const auto bank=data.bank.lock();
        need(header&&(loan||data.table_count==0)&&bank&&i.global_color_bank.get()==bank.get()&&
             bank->busy_.load()&&bank->active_owned_skin_operation_==bank_operation&&
             bank_operation->scope.bank.get()==bank.get(),"Late Speech lost NEW actual owning Bank");
    }
    if(data.active_core){const auto& core=*data.active_core;
        need(header&&loan&&bank_operation&&&core.operation==this&&core.ordinal==data.next_core&&
             core.ordinal<data.table_count,"Late Speech Core is not actual current table invocation");
        loan_identity(core.loan,core.ordinal);
    }
    need(!source.active_owned_skin_read_operation_&&!source.active_owned_category_stage_&&
         !source.active_owned_category_preconstructor_&&!source.initializing_owned_namespace_&&
         !i.category_source&&!i.active_completed_resource_operation&&i.default_state->live_enabled89==1,
         "Late Speech cannot borrow a pending old Skin/read/namespace effect");
}

void detail::BnwNativeGlobalResourceWarmOwner::InstalledLateSpeechTail::Operation::current()const {
    identity();const auto* const core=data.active_core;
    source.require_current();identity();
    if(loan){loan->require_current();identity();}
    if(core){need(data.active_core==core,"Late Speech changed actual Core during source callback");
        core->loan.require_current();identity();}
}
void detail::BnwNativeGlobalResourceWarmOwner::InstalledLateSpeechTail::Operation::require_identity()const {identity();}
void detail::BnwNativeGlobalResourceWarmOwner::InstalledLateSpeechTail::Operation::require_operation_current()const {current();}

void detail::BnwNativeGlobalResourceWarmOwner::InstalledLateSpeechTail::Operation::require_bank_current(
    const Bank& actual,const content::XmlNode* document,std::uint64_t ordinal)const {
    // Constructor-safe metadata role. Full Stage IO below additionally requires
    // this operation's actually constructed Bank PRE/POST owner pointer.
    identity();need(header&&(loan||data.table_count==0)&&!document&&ordinal==0&&
         data.bank.lock().get()==&actual&&i.global_color_bank.get()==&actual&&actual.thread_==data.creator&&
         actual.epoch_.get()==i.identity.get()&&same(actual.parent_,i.self)&&
         actual.resource_catalog_.get()==i.resource_catalog.get()&&
         actual.query_default_source_.lock().get()==i.current_default_source.get()&&
         actual.query_default_state_.lock().get()==i.default_state.get()&&
         actual.query_default_records_==&i.default_state->selected&&actual.live_resource_query_.lock().get()==&source,
         "Late Speech Bank metadata lost actual source controls");identity();
}
void detail::BnwNativeGlobalResourceWarmOwner::InstalledLateSpeechTail::Operation::require_effect_source_current(
    const Policy& actual,const void* actual_source)const {
    identity();need(header&&loan&&bank_operation&&data.global->policy.get()==&actual&&
         actual_source==data.common->source_identity&&data.global->consumer&&
         data.global->consumer->source_.get()==actual_source,
         "Late Speech retained Source has no NEW physical owner");
    actual.require_current();identity();
}
void detail::BnwNativeGlobalResourceWarmOwner::InstalledLateSpeechTail::Operation::require_actual_inputs_current(
    const Input& actual,const Boot& boot)const {
    identity();const auto language=data.selection->language.lock();
    need(language&&language.get()==&actual&&i.settings.get()==&boot&&same(i.settings,data.boot),
         "Late Speech retained policy changed actual Input/Boot");identity();
}

void detail::BnwNativeGlobalResourceWarmOwner::InstalledLateSpeechTail::Operation::loan_identity(const Loaded::Loan& held,std::size_t ordinal)const {
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

void detail::BnwNativeGlobalResourceWarmOwner::InstalledLateSpeechTail::Operation::require_stage_identity(const OwnedCategoryStage& stage)const {
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
                 index>=data.global->initial_stage_count&&stage.installed_global_skin_effects_driver->lock().get()==data.global.get()&&
                 same(*stage.installed_global_skin_effects_driver,data.global),
                 "Installed global Stage lost distinct actual new issuer");
            else if(stage.installed_pc_skin_effects_driver)need(!stage.installed_skin_effects_driver&&
                 index>=common.returned_stage_count&&index<data.global->initial_stage_count&&
                 stage.installed_pc_skin_effects_driver->lock().get()==data.global->previous.get()&&
                 same(*stage.installed_pc_skin_effects_driver,data.global->previous),"Installed global retained PC Stage lacks real provenance");
            else need(stage.installed_skin_effects_driver&&index<common.returned_stage_count&&
                 stage.installed_skin_effects_driver->lock().get()==&common&&same(*stage.installed_skin_effects_driver,data.common),
                 "Installed global retained Common Stage lacks real provenance");
            if(manager)need(instance->owned_resource_pairs_[stage.skin_slot][0].get()==manager.get()&&stage.arena&&stage.cache&&
                 stage.selected_source,
                 "Installed global Stage is not actual manager pair/Arena/cache");
            else need(false,"Late Speech retained Stage has no genuine completed manager");
        }
    }

void detail::BnwNativeGlobalResourceWarmOwner::InstalledLateSpeechTail::Operation::require_stage_current(const OwnedCategoryStage& stage)const {
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

void detail::BnwNativeGlobalResourceWarmOwner::InstalledLateSpeechTail::Operation::require_read_identity(const void* address)const {
        identity();const auto& common=*data.common;
        const auto found=std::find_if(source.owned_skin_read_receipts_.begin(),source.owned_skin_read_receipts_.end(),[&](const auto& v){return v.get()==address;});
        need(found!=source.owned_skin_read_receipts_.end()&&header&&loan&&bank_operation,
             "Installed global input lacks actual NEW physical owner/read receipt");
        const auto index=static_cast<std::size_t>(found-source.owned_skin_read_receipts_.begin());const auto& read=**found;
        const auto operation=read.operation;const auto occurrence=operation?operation->occurrence.lock():nullptr;const auto query=operation?operation->query:nullptr;
        if(read.installed_global_driver)need(!read.installed_driver&&!read.installed_pc_driver&&index>=data.global->initial_read_count&&
             read.installed_global_driver->lock().get()==data.global.get()&&same(*read.installed_global_driver,data.global)&&
             operation&&operation->issue_epoch==data.global->returned_epoch&&operation->restoration_returned,"Installed global input lost genuine new slot issuer/epoch");
        else if(read.installed_pc_driver)need(!read.installed_driver&&index>=common.returned_read_count&&index<data.global->initial_read_count&&
             read.installed_pc_driver->lock().get()==data.global->previous.get()&&same(*read.installed_pc_driver,data.global->previous)&&
             operation&&operation->issue_epoch==data.global->previous->returned_epoch&&operation->restoration_returned,
             "Installed global retained PC input lacks actual restored provenance");
        else need(read.installed_driver&&index<common.returned_read_count&&read.installed_driver->lock().get()==&common&&
             same(*read.installed_driver,data.common)&&operation&&operation->issue_epoch==common.returned_epoch&&operation->restoration_returned,
             "Installed global retained Common input lacks actual restored provenance");
        need(operation&&operation->restoration_returned,"Late Speech cannot borrow an unfinished prior Skin read");
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
             i.default_state->live_enabled89==1&&!source.active_owned_skin_read_operation_,
             "Installed global input lost actual query/ordinal/occurrence/Default toggle affinity");
        if(!read.open_error)need(read.identity.has_value(),"Installed global bytes have no physical stream identity");identity();
    }

detail::BnwNativeGlobalResourceWarmOwner::InstalledLateSpeechTail::ActiveCore::ActiveCore(
    Operation& actual,const Loaded::Loan& held)
    :operation(actual),loan(held),ordinal(actual.data.next_core){
    operation.identity();need(!operation.data.active_core&&operation.header&&operation.loan&&
         operation.bank_operation&&ordinal<operation.data.table_count,
         "Late Speech ActiveCore lacks actual table/Loaded/Bank scope");
    operation.loan_identity(loan,ordinal);operation.data.active_core=this;
    try {
        current();flags=loan.fields().current_flags();flags_observed=true;
        raw_core=loan.actor->state_->descriptor_core.get();current();identity();
    }catch(...){operation.data.active_core=nullptr;operation.data.failed=true;
        operation.i.header_poisoned=true;throw;}
}
detail::BnwNativeGlobalResourceWarmOwner::InstalledLateSpeechTail::ActiveCore::~ActiveCore(){
    auto& data=operation.data;
    if(data.active_core!=this||!body_returned){data.failed=true;operation.i.header_poisoned=true;}
    if(data.active_core==this)data.active_core=nullptr;
    // C++ then destroys emitted/brand while the same actual Loan is still held.
    // The table cursor advances only after with_actual_descriptor fully returns.
}
void detail::BnwNativeGlobalResourceWarmOwner::InstalledLateSpeechTail::ActiveCore::identity()const {
    operation.identity();need(operation.data.active_core==this&&ordinal==operation.data.next_core&&
         !body_returned&&ordinal<operation.data.table_count,
         "Late Speech borrowed a retired or foreign actual Core invocation");
    operation.loan_identity(loan,ordinal);
    if(flags_observed)need(flags==loan.resource.actual_fields.current_flags()&&raw_core&&
         raw_core==loan.actor->state_->descriptor_core.get(),"Late Speech current Core flags/control changed");
    if(emission_returned)need((flags&1u)!=0&&directory_brand&&!emitted.empty(),
         "Late Speech path has no actual eligible62EC20 invocation");
    else need(!directory_returned,"Late Speech manufactured directory return before actual path");
}
void detail::BnwNativeGlobalResourceWarmOwner::InstalledLateSpeechTail::ActiveCore::current()const {
    identity();operation.current();identity();loan.require_current();identity();
}
void detail::BnwNativeGlobalResourceWarmOwner::InstalledLateSpeechTail::ActiveCore::execute_actual_branch(){
    current();need(flags_observed&&!directory_returned&&!body_returned,
         "Late Speech actual Core branch replay");
    if((flags&1u)!=0){
        emit_actual_62ec20();current();
        operation.parent.register_installed_late_default_core_directory(operation.data);
        current();directory_returned=true;
    }else need(!emission_returned&&!directory_brand&&emitted.empty(),
         "Late Speech ineligible Core manufactured path/effect");
    current();
    CoreReturn observed;observed.package=loan.retained;observed.actor=loan.actor;
    observed.core=loan.actor->state_->descriptor_core;observed.raw_core=raw_core;observed.flags=flags;
    observed.path=emitted;observed.directory_brand=directory_brand;observed.eligible=(flags&1u)!=0;
    observed.directory_returned=directory_returned;
    const auto before=operation.data.core_returns.size();
    need(before==ordinal,"Late Speech actual Core return order changed");
    operation.data.core_returns.push_back(std::move(observed));current();
    need(operation.data.core_returns.size()==before+1,"Late Speech actual Core observation append failed");
    body_returned=true;
}

void detail::BnwNativeGlobalResourceWarmOwner::InstalledLateSpeechTail::outside_speech_identity(
    const Warm& source,const Parent& actual,const Parent::Impl& i)const {
    base_identity_without_callbacks(source,actual,i);
    need((phase==Phase::physical_returned||phase==Phase::speech_returned||phase==Phase::returned)&&
         begin_returned&&physical_post_returned&&header_returned&&continuations_returned&&
         !operation&&!active_core&&next_core==table_count&&core_returns.size()==table_count&&
         !i.header_active&&!i.active_header_fence&&!i.active_live_query_owner&&!i.active_completed_resource_operation&&
         !i.category_source&&!source.active_owned_skin_operation_epoch_&&!source.active_owned_skin_resource_loan_&&
         source.active_owned_skin_package_.expired()&&!source.active_owned_skin_resource_epoch_&&
         !source.active_owned_skin_read_operation_&&!source.active_owned_category_stage_&&
         !source.active_owned_category_preconstructor_&&!source.initializing_owned_namespace_&&!source.busy_.load()&&
         !common->continuation&&!selection->continuation&&
         whole_loaded_close==(table_count?WholeLoadedClose::real_loan_returned:WholeLoadedClose::empty_actual_table),
         "Late Speech outside observation precedes real physical/whole Loaded/Header/continuation return");
    const auto actual_bank=bank.lock();
    need(actual_bank&&!actual_bank->busy_.load()&&!actual_bank->active_owned_skin_operation_&&
         actual_bank->phase_==Bank::Phase::roster_bound,"Late Speech owning Bank did not genuinely retire");
    for(std::size_t n=0;n<core_returns.size();++n){const auto& row=core_returns[n];
        const auto& origin=selection->color->admission->rows[n];
        need(row.physical_scope_returned&&same(row.package,origin.package)&&same(row.actor,origin.actor)&&
             same(row.core,origin.core)&&!row.package.expired()&&!row.actor.expired()&&!row.core.expired()&&
             row.eligible==((row.flags&1u)!=0)&&row.directory_returned==row.eligible,
             "Late Speech lacks actual ascending table/Core scope return");
        // Raw pointer is provenance only. No departed Core or loan is invoked.
        if(row.eligible)need(row.directory_brand&&!row.path.empty(),"Late Speech eligible Core lacks real path/invocation control");
        else need(!row.directory_brand&&row.path.empty(),"Late Speech ineligible Core fabricated effect");
    }
}
void detail::BnwNativeGlobalResourceWarmOwner::InstalledLateSpeechTail::require_operation_identity_without_callbacks(
    const Parent& actual)const {
    const auto& i=*actual.impl_;const auto source=i.live_resource_query;
    need(source&&same(source,warm),"Late Speech donor has another actual Warm");
    if(operation){need(&operation->parent==&actual,"Late Speech donor borrowed another Parent");operation->identity();}
    else outside_speech_identity(*source,actual,i);
}
void detail::BnwNativeGlobalResourceWarmOwner::InstalledLateSpeechTail::require_active_core_identity_without_callbacks(
    const Parent& actual)const {
    require_operation_identity_without_callbacks(actual);
    need(operation&&active_core&&&active_core->operation==operation&&active_core->emission_returned&&
         active_core->directory_brand&&!active_core->body_returned,
         "Late Speech donor has no actual active emitted Core directory invocation");
    active_core->identity();
}
void detail::BnwNativeGlobalResourceWarmOwner::InstalledLateSpeechTail::require_active_core_current(const Parent& actual)const {
    require_active_core_identity_without_callbacks(actual);active_core->current();require_active_core_identity_without_callbacks(actual);
}
const void* detail::BnwNativeGlobalResourceWarmOwner::InstalledLateSpeechTail::active_core_identity_without_callbacks(
    const Parent& actual)const {
    require_active_core_identity_without_callbacks(actual);return active_core->raw_core;
}
const std::shared_ptr<const void>& detail::BnwNativeGlobalResourceWarmOwner::InstalledLateSpeechTail::active_directory_occurrence_brand_without_callbacks(
    const Parent& actual)const {
    require_active_core_identity_without_callbacks(actual);return active_core->directory_brand;
}
const std::string& detail::BnwNativeGlobalResourceWarmOwner::InstalledLateSpeechTail::emitted_62ec20_path_without_callbacks(
    const Parent& actual)const {
    require_active_core_identity_without_callbacks(actual);return active_core->emitted;
}
void detail::BnwNativeGlobalResourceWarmOwner::InstalledLateSpeechTail::require_speech_return_without_callbacks(const Parent& actual)const {
    const auto& i=*actual.impl_;const auto source=i.live_resource_query;
    need(source&&same(source,warm),"Late Speech close borrowed another actual Warm");
    outside_speech_identity(*source,actual,i);
}

void detail::BnwNativeGlobalResourceWarmOwner::InstalledLateSpeechTail::ActiveCore::emit_actual_62ec20(){
    current();need((flags&1u)!=0&&!emission_returned&&!directory_brand,
         "Late Speech62EC20 requires this actual eligible current Core occurrence");
    const auto receiver=loan.copied_receiver_for_62ec20();current();
    const auto text=std::string(receiver.c_str()); // Actual receiver C-prefix.
    std::string value;
    const auto slash=text.find_last_of('\\');
    if(slash!=std::string::npos){
        const auto colon=text.find(':');const auto begin=colon==std::string::npos?0:colon+1;
        need(begin<=slash+1,"Late Speech actual81D6B0 source extent unsupported");
        value=text.substr(begin,slash+1-begin);
    }
    if(value.empty()||value.back()!='\\')value+='\\';
    value+="Sounds\\Speech\\Shared";current();
    emitted=std::move(value);current();
    // Minted inside this genuine current loan/invocation, never aliasing
    // Parent or Core and never supplied as caller path/index/ready authority.
    directory_brand=std::make_shared<const std::uint8_t>(0);current();
    emission_returned=true;identity();
}
