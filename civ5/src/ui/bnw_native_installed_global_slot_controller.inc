// Concrete context is fully constructed BEFORE binding it and allocating Owner.
struct detail::BnwNativeGlobalResourceWarmOwner::InstalledGlobalSkinEffects::ActiveSlot final
    :SlotContext,BnwNativeInstalledGlobalSlotOwner::Control,
     std::enable_shared_from_this<ActiveSlot> {
    using Owner=BnwNativeInstalledGlobalSlotOwner;
    using Key=Owner::Key;
    using OwnerPhase=Owner::Phase;
    enum class Role {idle,constructing,setter,match_enumeration,match_predicate,append_index,index_reset,
        selection,base_compare,exclusion,bank,selected660,post,closing,closed};
    const std::weak_ptr<InstalledGlobalSkinEffects> frame;
    const unsigned slot;
    const std::uint64_t serial;
    std::shared_ptr<Owner> owner;
    mutable const Owner* constructing_owner=nullptr;
    bool constructing=true,failed=false,post_returned=false,closed=false;
    Role role=Role::constructing;
    unsigned step=0,setter_returns=0;
    Key write_key=Key::set;
    std::string write_value;
    std::size_t cursor=0;
    std::optional<std::size_t> current_index;
    mutable std::optional<std::size_t> last_set_index;
    mutable std::optional<std::string> selected;
    mutable bool set_observed=false;
    bool base_checked=false,base_result=false,match_result=false;
    bool disabled_changed=false,common_changed=false,pc_changed=false;
    ActiveSlot(const std::shared_ptr<InstalledGlobalSkinEffects>& actual,std::uint64_t epoch,unsigned actual_slot)
        :SlotContext(actual,epoch,actual_slot),frame(actual),slot(actual_slot),serial(epoch) {}
    ActiveSlot(const ActiveSlot&)=delete;
    std::shared_ptr<InstalledGlobalSkinEffects> actual_frame()const {
        need(!failed,"Installed global concrete slot failed");
        const auto data=frame.lock();need(data&&data->creator==std::this_thread::get_id()&&!data->failed&&data->operation&&
            data->operation->serial==serial&&data->active_slot==this&&data->next_slot==slot,
            "Installed global concrete slot lost actual frame/creator/serial");return data;
    }
    void require_pure_identity()const override {
        const auto data=actual_frame();
        need(data->operation->header&&data->operation->loan&&data->operation->bank_operation&&data->kernel_invoking&&
             data->kernel&&data->kernel->active_&&slot<2&&step<=9,
             "Installed global slot is not inside actual coupled physical kernel");
        if(constructing)need(role==Role::constructing&&data->slot_construction_active&&!closed,
             "Installed global slot construction phase changed");
        else need(owner&&owner->thread_==data->creator&&owner->slot_==slot&&!owner->failed_&&
             (role==Role::closing||role==Role::closed||owner->phase_==OwnerPhase::live),
             "Installed global slot lost SAME actual owner/live phase");
        // No owner.current, provider, metadata or I/O: only authentic raw structure.
    }
    void require_trace_identity(const Consumer& consumer)const override {
        const auto data=actual_frame();need(data->consumer.get()==&consumer&&!consumer.failed_,
            "Installed global slot trace is not SAME Consumer");
        const auto& expected=data->returned_selectors;const auto& actual=consumer.completed_slot_selectors_;
        if(role==Role::selected660&&data->invoking_selector) {
            need(*data->invoking_selector==static_cast<int>(slot),"Installed global actual selector changed");
            const bool after=actual.size()==expected.size()+1&&std::equal(expected.begin(),expected.end(),actual.begin())&&
                actual.back()==static_cast<int>(slot);
            need(actual==expected||after,"Installed global active660 changed actual journal");
        }else need(!consumer.active_&&actual==expected&&!data->invoking_selector,
            "Installed global slot has no real inactive Consumer return");
    }
    bool nonbase_selected()const {
        if(!selected)return false;
        need(set_observed&&base_checked,"Installed global selected set lacks real getter/comparison witness");
        return !base_result;
    }
    void scope_identity(const Owner& actual)const {
        require_pure_identity();const auto data=actual_frame();
        const auto control=actual.control_.lock();
        need(control&&control.get()==static_cast<const Owner::Control*>(this)&&actual.thread_==data->creator&&
             actual.slot_==slot&&!actual.failed_,"Installed global substituted owner/control");
        if(constructing){
            if(!constructing_owner)constructing_owner=&actual;
            need(constructing_owner==&actual&&(actual.phase_==OwnerPhase::constructing||actual.phase_==OwnerPhase::live)&&
                 !actual.body_attempted_&&!actual.body_active_,"Installed global construction substituted actual owner");
        }else need(owner.get()==&actual,"Installed global changed actual per-slot owner");
    }
    void require_actual_scope_current(const Owner& actual)const override {
        scope_identity(actual);const auto data=actual_frame();data->operation->current();scope_identity(actual);
    }
    void require_actual_slot_construction_current(unsigned actual_slot)const override {
        require_pure_identity();need(constructing&&actual_slot==slot&&role==Role::constructing,
            "Installed global map/list allocation is not actual slot construction");
        actual_frame()->operation->current();require_pure_identity();
    }
    void require_actual_constraint_write_current(const Owner& actual,Key key,std::string_view value)const override {
        scope_identity(actual);need(!constructing&&role==Role::setter&&key==write_key&&
             value.substr(0,value.find('\0'))==write_value&&actual.phase_==OwnerPhase::live&&actual.body_active_,
             "Installed778E40 lacks exact actual setter/source affinity");
        actual_frame()->operation->current();scope_identity(actual);
    }
    void require_actual_index_reset_current(const Owner& actual)const override {
        scope_identity(actual);need(role==Role::index_reset&&step==0&&setter_returns==2,
             "Installed5E5490 is not actual first6FA reset1");actual_frame()->operation->current();scope_identity(actual);
    }
    void require_actual_index_append_current(const Owner& actual,std::size_t index)const override {
        scope_identity(actual);const auto data=actual_frame();
        need(role==Role::append_index&&match_result&&current_index&&*current_index==index&&index==cursor&&
             index<data->operation->source.owned_skin_instances_.size(),
             "Installed6FA append lacks current real matched dense occurrence");data->operation->current();scope_identity(actual);
    }
    void require_actual_constraint_read_current(const Owner& actual)const override {
        scope_identity(actual);const auto data=actual_frame();
        need((role==Role::match_predicate&&current_index&&*current_index==cursor)||
             (role==Role::selected660&&data->invoking_selector&&*data->invoking_selector==static_cast<int>(slot)&&data->consumer->active_),
             "Installed64F970 has no actual6FA/660 slot read affinity");data->operation->current();scope_identity(actual);
    }
    void require_actual_temporary_close_current(const Owner& actual)const override {
        scope_identity(actual);const auto data=actual_frame();
        need(role==Role::closing&&post_returned&&!data->consumer->active_&&!data->invoking_selector&&
             step==(nonbase_selected()?9u:7u)&&setter_returns==(nonbase_selected()?9u:3u)&&
             !data->operation->source.active_owned_skin_read_operation_&&!data->operation->source.active_owned_category_stage_&&
             data->operation->i.default_state->live_enabled89==1,
             "Installed global temporaries close before actual slot/effects/targets POST");
        data->operation->current();scope_identity(actual);
    }
    void invalidate_actual_global_operation()const noexcept override {
        const auto data=frame.lock();
        if(data&&data->creator==std::this_thread::get_id()) {
            data->failed=true;
            if(data->operation){data->operation->source.failed_.store(true);data->operation->i.header_poisoned=true;}
        }
    }
    void read_actual_constraints(const Policy& policy,
        const std::function<void(const std::string&,const std::string&)>& input)const override {
        const auto data=actual_frame();require_pure_identity();need(data->policy.get()==&policy&&owner,
            "Installed global matcher changed actual policy/owner");
        owner->with_actual_constraint_read([&](const auto& loan){
            loan->with_actual_entries([&](const auto& actual){
                {const auto consume=input;data->operation->current();need(bool(consume),"Installed actual slot matcher target absent");
                    consume(actual.at("set"),actual.at("platform"));data->operation->current();}
                data->operation->current(); // real copied matcher target destroyed while SAME read loan lives
            });
        });data->operation->current();
    }
    void observe_actual_set(const Instance& instance,const std::optional<std::string>& value)const override {
        const auto data=actual_frame();require_pure_identity();
        need(role==Role::selection&&step==2&&current_index&&*current_index==cursor&&
             cursor<data->operation->source.owned_skin_instances_.size()&&
             data->operation->source.owned_skin_instances_[cursor].get()==&instance&&
             (!last_set_index||*last_set_index<cursor),"Installed67 getter observation is not actual current selection iteration");
        // Only this real getter issuer records the latest qualifying read. No selector is recomputed here.
        selected=value;set_observed=true;last_set_index=cursor;
    }
    void require_actual_exclusion(const Instance& instance,unsigned actual_slot)const override {
        const auto data=actual_frame();require_pure_identity();
        need(role==Role::exclusion&&actual_slot==slot&&current_index&&*current_index==cursor&&owner&&owner->indices_&&
             cursor<data->operation->source.owned_skin_instances_.size()&&
             data->operation->source.owned_skin_instances_[cursor].get()==&instance&&
             std::find(owner->indices_->begin(),owner->indices_->end(),cursor)==owner->indices_->end(),
             "Installed global disable lacks actual excluded dense instance/list/slot");
    }
    void require_selected_resource_identity(const BnwNativeOwnedSkinOccurrence& occurrence,unsigned actual_slot)const override {
        const auto data=actual_frame();require_pure_identity();
        const auto& source=data->operation->source;
        need(role==Role::selected660&&actual_slot==slot&&data->invoking_selector&&
             *data->invoking_selector==static_cast<int>(slot)&&data->consumer->active_&&
             occurrence.roster_index_<source.owned_skin_occurrences_.size()&&
             source.owned_skin_occurrences_[occurrence.roster_index_].get()==&occurrence,
             "Installed global Stage/read is not SAME actual selected660 occurrence/slot");
    }
    void open_owner() {
        require_pure_identity();const std::shared_ptr<const Owner::Control> control=shared_from_this();
        owner=std::shared_ptr<Owner>(new Owner(control,slot));
        need(owner.get()==constructing_owner,"Installed global allocated another actual owner");
        constructing=false;role=Role::idle;actual_frame()->slot_construction_active=false;
        require_pure_identity();owner->require_current();
    }
    void setter(Key key,const std::string& value) {
        require_pure_identity();need(role==Role::idle,"Installed global overlapping actual setter");
        role=Role::setter;write_key=key;write_value=value;
        owner->set_actual_constraint_778e40(key,value);++setter_returns;
        role=Role::idle;write_value.clear();actual_frame()->operation->current();
    }
};

std::shared_ptr<detail::BnwNativeGlobalResourceWarmOwner::InstalledGlobalSkinEffects>
detail::BnwNativeGlobalResourceWarmOwner::InstalledGlobalSkinEffects::actual(const Parent& parent) {
    const auto storage=parent.impl_;need(storage->thread==std::this_thread::get_id(),"Installed global bridge foreign creator");
    const auto warm=storage->live_resource_query;const auto data=warm?warm->installed_global_skin_effects_:nullptr;
    need(data&&data->operation&&!data->failed&&!data->returned,"Installed global bridge lacks actual live frame");
    data->operation->current();return data;
}
std::shared_ptr<detail::BnwNativeGlobalResourceWarmOwner::InstalledGlobalSkinEffects::ActiveSlot>
detail::BnwNativeGlobalResourceWarmOwner::InstalledGlobalSkinEffects::actual_slot()const {
    const auto slot=concrete_slot.lock();
    need(slot&&active_slot==slot.get()&&slot->frame.lock().get()==this,"Installed global bridge has no SAME concrete slot");
    slot->require_pure_identity();return slot;
}
void detail::BnwNativeGlobalResourceWarmOwner::InstalledGlobalSkinEffects::physical_post(const Parent& parent)const {
    need(operation&&operation->header&&operation->loan&&operation->bank_operation&&consumer&&!consumer->active_&&
         !invoking_selector&&!operation->source.active_owned_skin_read_operation_&&!operation->source.active_owned_category_stage_&&
         !operation->source.initializing_owned_namespace_&&operation->i.default_state->live_enabled89==1,
         "Installed global physical POST retains active Consumer/Default/Stage/read");
    operation->current();
    for(const auto& read:operation->source.owned_skin_read_receipts_)parent.require_native_owned_skin_read_receipt_current(read.get());
    for(const auto& stage:operation->source.owned_category_stages_){
        need(stage&&stage->step==OwnedCategoryStage::Step::name_registered,"Installed global POST has unfinished actual namespace");
        operation->require_stage_current(*stage);
    }
    operation->current();
}
void BnwNativePackageDiscoveryOwner::with_installed_global_skin_inputs(const std::function<void(
    const std::weak_ptr<OwnedSkinEffectsSource>&,
    const std::shared_ptr<detail::BnwNativeOwnedSkinConsumer>&,
    const std::shared_ptr<detail::BnwNativeOwnedSkinBootPolicy>&)>& input)const {
    using Data=detail::BnwNativeGlobalResourceWarmOwner::InstalledGlobalSkinEffects;
    const auto data=Data::actual(*this);
    need(data->operation->header&&data->operation->loan&&data->operation->bank_operation&&!data->factory_active&&
         !data->kernel&&!data->kernel_invoking&&!data->kernel_returned&&data->factory_step==Data::FactoryStep::fresh,
         "Installed global factory wrapper lacks NEW physical/unique actual call");
    data->factory_active=true;
    struct Exit {Data& data;~Exit(){data.factory_active=false;}} exit{*data};
    {const auto consume=input;data->operation->current();need(bool(consume),"Installed global actual kernel target absent");
        consume(data->common->continuation_effects_source,data->consumer,data->policy);data->operation->current();}
    data->operation->current(); // actual copied kernel supplier target destroyed before POST
    need(data->kernel_returned&&!data->kernel_invoking,"Installed global actual kernel supplier omitted normal return");
}
void BnwNativePackageDiscoveryOwner::record_installed_global_source(
    const std::shared_ptr<const void>& source,const std::shared_ptr<OwnedSkinGlobalEffectsLoan>& loan)const {
    using Data=detail::BnwNativeGlobalResourceWarmOwner::InstalledGlobalSkinEffects;const auto data=Data::actual(*this);
    need(data->factory_active&&data->factory_step==Data::FactoryStep::complete&&source&&loan&&
         !data->global_source_identity&&data->global_source_control.expired()&&data->typed_effects_loan.expired()&&!data->kernel,
         "Installed global Source registration lacks genuine typed factory return");
    data->global_source_identity=source.get();data->global_source_control=source;data->typed_effects_loan=loan;
    data->operation->current();
}
void BnwNativePackageDiscoveryOwner::record_installed_global_kernel(
    const std::shared_ptr<detail::BnwNativeOwnedSkinGlobalSelectionKernel>& kernel)const {
    using Data=detail::BnwNativeGlobalResourceWarmOwner::InstalledGlobalSkinEffects;const auto data=Data::actual(*this);
    const auto source=data->global_source_control.lock();
    need(data->factory_active&&source&&kernel&&!data->kernel&&kernel->thread_==data->creator&&!kernel->active_&&!kernel->failed_&&
         kernel->source_.get()==data->global_source_identity&&source.get()==kernel->source_.get()&&
         Data::same(source,kernel->source_)&&!data->typed_effects_loan.expired()&&kernel->normal_returned_arguments_.empty(),
         "Installed global Kernel is not SAME actual new Source/constructor return");
    data->kernel=kernel;data->operation->current();
}
void BnwNativePackageDiscoveryOwner::require_installed_global_source_current(const void* actual_source)const {
    using Data=detail::BnwNativeGlobalResourceWarmOwner::InstalledGlobalSkinEffects;const auto data=Data::actual(*this);
    const auto source=data->global_source_control.lock();
    need(source&&actual_source&&source.get()==actual_source&&data->global_source_identity==actual_source&&
         !data->typed_effects_loan.expired()&&(data->factory_active||data->kernel_invoking||data->kernel_returned),
         "Installed global Source lost exact actual registration/physical activity");data->operation->current();
}
void BnwNativePackageDiscoveryOwner::begin_installed_global_kernel(
    const std::shared_ptr<detail::BnwNativeOwnedSkinGlobalSelectionKernel>& kernel)const {
    using Data=detail::BnwNativeGlobalResourceWarmOwner::InstalledGlobalSkinEffects;const auto data=Data::actual(*this);
    need(data->factory_active&&data->kernel.get()==kernel.get()&&!kernel->active_&&!kernel->failed_&&
         !data->kernel_invoking&&!data->kernel_returned&&data->next_slot==0&&!data->active_slot,
         "Installed global real invocation replay/before genuine factory");
    data->kernel_invoking=true;data->operation->current();
}
void BnwNativePackageDiscoveryOwner::record_installed_global_kernel_return(
    const std::shared_ptr<detail::BnwNativeOwnedSkinGlobalSelectionKernel>& kernel,bool changed)const {
    using Data=detail::BnwNativeGlobalResourceWarmOwner::InstalledGlobalSkinEffects;const auto data=Data::actual(*this);
    need(data->kernel.get()==kernel.get()&&data->kernel_invoking&&!data->kernel_returned&&!kernel->active_&&!kernel->failed_&&
         kernel->normal_returned_arguments_==std::vector<int>{-1}&&data->next_slot==2&&!data->active_slot&&
         data->concrete_slot.expired()&&data->final_reconcile_returned&&!data->consumer->active_&&!data->invoking_selector&&
         data->consumer->completed_slot_selectors_==data->returned_selectors,
         "Installed global result lacks actual whole coupled Kernel/inactive journal/slot return");
    for(unsigned slot=0;slot<2;++slot){const auto& row=data->slot_returns[slot];const auto& owner=row.owner;
        need(row.closed&&owner&&owner->phase_==Data::Owner::Phase::closed&&owner->closed_returned_&&owner->body_returned_&&
             !owner->failed_&&!owner->constraints_&&!owner->indices_&&!owner->active_read_&&!owner->body_active_&&
             row.setter_returns==(row.selected_nonbase?9u:3u),"Installed global result lacks real disposed slot map/list");
    }
    const auto& last=data->slot_returns[1];
    need(changed==(last.disabled_changed||last.common_changed||last.pc_changed),
         "Installed global bool is not actual last-slot observation");
    data->native_changed=changed;data->kernel_returned=true;data->kernel_invoking=false;data->operation->current();
}
void BnwNativePackageDiscoveryOwner::with_installed_global_skin_slot(unsigned slot,
    const std::function<void(std::vector<std::size_t>&)>& input)const {
    using Data=detail::BnwNativeGlobalResourceWarmOwner::InstalledGlobalSkinEffects;const auto data=Data::actual(*this);
    need(data->kernel_invoking&&data->kernel&&data->kernel->active_&&slot<2&&data->next_slot==slot&&
         !data->slot_construction_active&&!data->active_slot&&data->concrete_slot.expired()&&!data->slot_returns[slot].closed,
         "Installed global slot requires real ordered creating-kernel call");
    data->slot_construction_active=true;
    struct CreationExit {Data& data;~CreationExit(){data.slot_construction_active=false;}} creation_exit{*data};
    const auto context=std::shared_ptr<Data::ActiveSlot>(new Data::ActiveSlot(data,data->returned_epoch,slot));
    data->active_slot=context.get();data->concrete_slot=context;
    struct Detach {Data& data;~Detach(){data.active_slot=nullptr;data.concrete_slot.reset();data.slot_construction_active=false;}} detach{*data};
    context->open_owner();
    context->owner->with_actual_slot_body([&](auto& indices){
        {const auto consume=input;data->operation->current();need(bool(consume),"Installed global actual slot target absent");
            consume(indices);data->operation->current();}
        context->role=Data::ActiveSlot::Role::post;
        data->physical_post(*this);context->post_returned=true;context->role=Data::ActiveSlot::Role::closing;
    });
    context->owner->require_closed_returned_current();context->closed=true;context->role=Data::ActiveSlot::Role::closed;
    auto& result=data->slot_returns[slot];result.owner=context->owner;result.closed=true;
    result.selected_nonbase=context->nonbase_selected();result.setter_returns=context->setter_returns;
    result.disabled_changed=context->disabled_changed;result.common_changed=context->common_changed;result.pc_changed=context->pc_changed;
    data->active_slot=nullptr;data->concrete_slot.reset();++data->next_slot;data->operation->current();
}
void BnwNativePackageDiscoveryOwner::with_installed_global_match_pass(const std::string& set,const std::string& platform,
    bool reset,std::vector<std::size_t>& indices,const std::function<void()>& input)const {
    using Data=detail::BnwNativeGlobalResourceWarmOwner::InstalledGlobalSkinEffects;using Role=Data::ActiveSlot::Role;
    const auto data=Data::actual(*this);const auto context=data->actual_slot();
    need(context->role==Role::idle,"Installed global match pass reentry");context->owner->require_actual_index_alias(indices);
    const auto step=context->step;
    if(step==0){need(set=="BaseGame"&&platform=="Common"&&reset,"Installed global first6FA substituted BaseGame/Common/reset1");
        context->setter(Data::Owner::Key::set,"BaseGame");context->setter(Data::Owner::Key::platform,"Common");}
    else if(step==1){need(set=="BaseGame"&&platform==data->policy->actual_platform_&&!reset,"Installed global base PC6FA substituted actual platform");
        context->setter(Data::Owner::Key::platform,data->policy->actual_platform_);}
    else if(step==3){need(context->nonbase_selected()&&context->selected&&set==*context->selected&&platform=="Common"&&!reset,
        "Installed global selected6FA lacks real67/Common witness");
        context->setter(Data::Owner::Key::set,*context->selected);context->setter(Data::Owner::Key::platform,"Common");}
    else {need(step==4&&context->nonbase_selected()&&context->selected&&set==*context->selected&&
        platform==data->policy->actual_platform_&&!reset,"Installed global selected PC6FA out of actual order");
        context->setter(Data::Owner::Key::platform,data->policy->actual_platform_);}
    if(reset){context->role=Role::index_reset;context->owner->reset_actual_indices_5e5490();}
    context->role=Role::match_enumeration;context->cursor=0;context->current_index.reset();
    {const auto consume=input;data->operation->current();need(bool(consume),"Installed global real6FA enumerator absent");
        consume();data->operation->current();}
    data->operation->current();need(context->role==Role::match_enumeration&&!context->current_index&&
        context->cursor==data->operation->source.owned_skin_instances_.size(),"Installed6FA omitted actual dense roster/predicate");
    context->role=Role::idle;context->step=step+1;data->operation->current();
}
void BnwNativePackageDiscoveryOwner::begin_installed_global_match_index(std::size_t index)const {
    using Data=detail::BnwNativeGlobalResourceWarmOwner::InstalledGlobalSkinEffects;const auto data=Data::actual(*this);const auto context=data->actual_slot();
    need(context->role==Data::ActiveSlot::Role::match_enumeration&&!context->current_index&&index==context->cursor&&
        index<data->operation->source.owned_skin_instances_.size(),"Installed6FA predicate is not current actual dense index");
    context->current_index=index;context->match_result=false;context->role=Data::ActiveSlot::Role::match_predicate;data->operation->current();
}
void BnwNativePackageDiscoveryOwner::record_installed_global_match_index(std::size_t index,bool matches)const {
    using Data=detail::BnwNativeGlobalResourceWarmOwner::InstalledGlobalSkinEffects;const auto data=Data::actual(*this);const auto context=data->actual_slot();
    need(context->role==Data::ActiveSlot::Role::match_predicate&&context->current_index&&*context->current_index==index&&
         index==context->cursor,"Installed6FA has no real current predicate return");
    context->match_result=matches;
    if(matches){context->role=Data::ActiveSlot::Role::append_index;context->owner->append_actual_index_6fa850(index);}
    context->current_index.reset();++context->cursor;context->role=Data::ActiveSlot::Role::match_enumeration;data->operation->current();
}
void BnwNativePackageDiscoveryOwner::with_installed_global_kernel_roster(const std::function<void(
    const std::shared_ptr<const detail::BnwNativeOwnedUiSkinPoststageInstance>&,
    const std::shared_ptr<const detail::BnwNativeOwnedSkinOccurrence>&)>& input)const {
    using Data=detail::BnwNativeGlobalResourceWarmOwner::InstalledGlobalSkinEffects;using Role=Data::ActiveSlot::Role;
    const auto data=Data::actual(*this);const auto context=data->actual_slot();
    need(context->role==Role::idle,"Installed global actual roster reentry");
    const bool selecting=context->step==2;
    need(selecting||(context->step==3&&!context->nonbase_selected())||context->step==5,
         "Installed global roster is not real67/exclusion frontier");
    context->role=selecting?Role::selection:Role::exclusion;context->cursor=0;context->current_index.reset();
    const auto writes_before=data->operation->source.owned_skin_state_writes_.size();
    {const auto consume=input;data->operation->current();need(bool(consume),"Installed global actual roster target absent");
        with_native_owned_skin_constructed_roster([&](const auto& roster){
            for(const auto& value:roster){const auto instance=value;
                need(instance&&context->cursor<data->operation->source.owned_skin_instances_.size()&&
                     data->operation->source.owned_skin_instances_[context->cursor].get()==instance.get(),"Installed global actual roster changed dense order");
                std::shared_ptr<const detail::BnwNativeOwnedSkinOccurrence> token;
                with_native_owned_skin_occurrences([&](const auto& actual){if(&instance->constructor_source_->actual_occurrence()==actual.get()){
                    need(!token,"Installed global actual token repeated");token=actual;}});
                need(token&&data->operation->source.owned_skin_occurrences_[context->cursor].get()==token.get(),"Installed global actual instance/token publication changed");
                context->current_index=context->cursor;data->operation->current();consume(instance,token);data->operation->current();
                context->current_index.reset();++context->cursor;
            }
        });data->operation->current();}
    data->operation->current();need(context->cursor==data->operation->source.owned_skin_instances_.size()&&!context->current_index,
        "Installed global real roster target omitted/aborted iteration");
    if(!selecting){const auto& writes=data->operation->source.owned_skin_state_writes_;
        for(std::size_t n=writes_before;n<writes.size();++n){const auto& receipt=writes[n];const auto instance=receipt->instance.lock();
            need(instance&&receipt->slot==context->slot&&receipt->issue_epoch==data->returned_epoch&&
                 receipt->actual_effect_source==data->consumer->source_.get()&&instance->enabled_[context->slot]==0,
                 "Installed global exclusion observation lacks actual disable receipt");}
        context->disabled_changed=writes.size()!=writes_before;
    }
    context->role=Role::idle;context->step=selecting?3u:6u;data->operation->current();
}
void BnwNativePackageDiscoveryOwner::begin_installed_global_base_compare(const std::string& value)const {
    using Data=detail::BnwNativeGlobalResourceWarmOwner::InstalledGlobalSkinEffects;const auto data=Data::actual(*this);const auto context=data->actual_slot();
    need(context->role==Data::ActiveSlot::Role::idle&&context->step==3&&context->selected&&*context->selected==value&&
         context->set_observed&&!context->base_checked,"Installed global BaseGame compare lacks SAME actual67 witness");
    context->role=Data::ActiveSlot::Role::base_compare;data->operation->current();
}
void BnwNativePackageDiscoveryOwner::record_installed_global_base_compare(const std::string& value,bool base)const {
    using Data=detail::BnwNativeGlobalResourceWarmOwner::InstalledGlobalSkinEffects;const auto data=Data::actual(*this);const auto context=data->actual_slot();
    need(context->role==Data::ActiveSlot::Role::base_compare&&context->selected&&*context->selected==value&&!context->base_checked,
         "Installed global BaseGame compare has no actual current result");
    context->base_checked=true;context->base_result=base;context->role=Data::ActiveSlot::Role::idle;data->operation->current();
}
void BnwNativePackageDiscoveryOwner::prepare_installed_global_660(const std::string& set,const std::string& platform,int slot)const {
    using Data=detail::BnwNativeGlobalResourceWarmOwner::InstalledGlobalSkinEffects;const auto data=Data::actual(*this);const auto context=data->actual_slot();
    need(context->role==Data::ActiveSlot::Role::idle&&(context->step==7||context->step==8)&&
         context->nonbase_selected()&&context->selected&&set==*context->selected&&slot==static_cast<int>(context->slot)&&
         !data->consumer->active_&&!data->invoking_selector,"Installed global660 lacks genuine selected slot/native call frontier");
    if(context->step==7){need(platform=="Common","Installed global first660 substituted Common");
        context->setter(Data::Owner::Key::set,*context->selected); // Repeated actual778E40, even unchanged value.
        context->setter(Data::Owner::Key::platform,"Common");}
    else {need(platform==data->policy->actual_platform_,"Installed global second660 substituted canonical PC");
        context->setter(Data::Owner::Key::platform,data->policy->actual_platform_);}
    context->role=Data::ActiveSlot::Role::selected660;data->invoking_selector=slot;data->operation->current();
}
void BnwNativePackageDiscoveryOwner::record_installed_global_660_return(
    const std::shared_ptr<detail::BnwNativeOwnedSkinConsumer>& consumer,int slot,bool changed)const {
    using Data=detail::BnwNativeGlobalResourceWarmOwner::InstalledGlobalSkinEffects;const auto data=Data::actual(*this);const auto context=data->actual_slot();
    auto after=data->returned_selectors;after.push_back(slot);
    need(context->role==Data::ActiveSlot::Role::selected660&&data->invoking_selector&&*data->invoking_selector==slot&&
         slot==static_cast<int>(context->slot)&&consumer.get()==data->consumer.get()&&Data::same(consumer,data->consumer)&&
         !consumer->active_&&!consumer->failed_&&consumer->completed_slot_selectors_==after,
         "Installed global660 has no actual inactive SAME Consumer normal return");
    // Observe genuine journal only; only Consumer writes its own completed_slot_selectors_.
    data->returned_selectors.push_back(slot);
    if(context->step==7)context->common_changed=changed;else{need(context->step==8,"Installed global second660 return order changed");context->pc_changed=changed;}
    ++context->step;data->invoking_selector.reset();context->role=Data::ActiveSlot::Role::idle;data->operation->current();
}
void BnwNativePackageDiscoveryOwner::reconcile_installed_global_bank()const {
    using Data=detail::BnwNativeGlobalResourceWarmOwner::InstalledGlobalSkinEffects;const auto data=Data::actual(*this);
    const auto bank=data->bank.lock();need(bank&&data->operation->bank_operation&&data->kernel_invoking&&data->kernel&&data->kernel->active_,
        "Installed global462 lacks actual coupled owning Bank");
    if(data->active_slot){const auto context=data->actual_slot();
        need(context->role==Data::ActiveSlot::Role::idle&&context->step==6,"Installed global slot462 out of order");
        context->role=Data::ActiveSlot::Role::bank;bank->reconcile_owned_colors_462c20();data->operation->current();
        context->step=7;context->role=Data::ActiveSlot::Role::idle;
    }else{
        need(data->next_slot==2&&data->slot_returns[0].closed&&data->slot_returns[1].closed&&!data->final_reconcile_active&&!data->final_reconcile_returned,
            "Installed global final462 before both actual temporary closes/replayed");
        data->final_reconcile_active=true;
        struct Exit {Data& data;~Exit(){data.final_reconcile_active=false;}} exit{*data};
        bank->reconcile_owned_colors_462c20();data->operation->current();data->final_reconcile_returned=true;
    }data->operation->current();
}
