// Actual next6415F0(PC) only, after genuine closed first Common660330.
// This new publication operation grants no Header/Loaded/Bank/resource query.
struct detail::BnwNativeGlobalResourceWarmOwner::InstalledPcPublication {
    using Warm=BnwNativeGlobalResourceWarmOwner;
    using Parent=BnwNativePackageDiscoveryOwner;
    using Policy=BnwNativeOwnedSkinBootPolicy;
    using Selection=InstalledInitialSkinSelection;
    using Input=runtime::BnwSelectedLanguageInputOwner;
    using Boot=BnwNativeResourceBootSettingsOwner;
    const std::thread::id creator=std::this_thread::get_id();
    const std::weak_ptr<const Parent> parent;
    const std::weak_ptr<const void> warm,input,epoch,boot,catalog;
    const std::shared_ptr<InstalledInitialSkinEffects> common;
    const std::shared_ptr<Policy> policy;
    bool failed=false,publishing=false,call_returned=false,returned=false;
    struct Operation;
    Operation* operation=nullptr;
    InstalledPcPublication(const Parent::Impl& i,const Warm& source)
        :parent(i.self),warm(i.live_resource_query),input(i.installation_input),epoch(i.identity),boot(i.settings),
         catalog(i.resource_catalog),common(source.installed_initial_skin_effects_),policy(common->policy){}
    static bool same(const auto& a,const auto& b){return !a.owner_before(b)&&!b.owner_before(a);}
    void closed_common_identity(const Warm&,const Parent&,const Parent::Impl&)const;
    void publication_identity()const;
};
void detail::BnwNativeGlobalResourceWarmOwner::InstalledPcPublication::closed_common_identity(
    const Warm& source,const Parent& actual,const Parent::Impl& i)const {
    need(creator==std::this_thread::get_id()&&source.thread_==creator&&i.thread==creator&&
         !failed&&!source.closed_.load()&&!source.failed_.load()&&!i.header_poisoned&&!i.category_lineage_failed,
         "Installed PC actual source/creator retired");
    need(source.installed_pc_publication_.get()==this&&source.installed_pc_publication_attempted_&&
         i.installation_input&&!i.lease&&!i.rules&&i.live_resource_query.get()==&source&&
         same(i.self,parent)&&same(i.live_resource_query,warm)&&same(i.installation_input,input)&&same(i.identity,epoch)&&
         i.settings&&same(i.settings,boot)&&same(i.resource_catalog,catalog)&&common&&
         common.get()==source.installed_initial_skin_effects_.get()&&common->returned&&!common->failed&&
         !common->operation&&common->kernel_returned&&common->consumer&&common->selection&&
         common->selection.get()==source.installed_initial_skin_selection_.get()&&common->selection->prefix_returned&&
         !common->selection->failed&&!common->selection->operation&&!common->selection->policy_construction_active&&
         common->selection->counter_returned&&common->selection->enabled_instances==0&&
         common->selection->pending==Selection::Pending::common660330&&
         common->selection->color.get()==source.installed_color_skin_driver_.get()&&common->selection->color->returned&&
         !common->selection->color->failed&&!common->selection->color->operation&&!common->selection->color->constructor_loan&&
         !common->selection->color->active_base_read&&common->selection->color->admission&&
         common->selection->color->admission->returned&&!common->selection->color->admission->failed&&
         !i.header_active&&!i.active_header_fence&&!i.active_live_query_owner&&!i.active_completed_resource_operation&&
         !i.category_source&&!source.active_owned_skin_operation_epoch_&&!source.active_owned_skin_resource_loan_&&
         !source.active_owned_skin_read_operation_&&!source.active_owned_category_stage_&&
         !source.active_owned_category_preconstructor_&&!source.initializing_owned_namespace_&&!source.busy_.load(),
         "Installed6415F0 lost genuine closed first Common/628BE0/52D590 history or borrowed a resource operation");
    common->selection->color->admission->identity(source,actual,i); // Same canonical accounted outside-Parent controls.
    need(policy&&policy.get()==common->policy.get()&&policy.get()==source.owned_skin_boot_policy_.get()&&
         same(common->selection->policy,policy)&&policy->installed_source_&&
         policy->installed_source_->lock().get()==static_cast<const Policy::InstalledSource*>(common->selection.get())&&
         same(*policy->installed_source_,common->selection)&&same(policy->input_,common->selection->language)&&
         same(policy->boot_,boot)&&policy->lease_.expired()&&policy->installed_setters_returned_==2&&
         policy->installed_constraints_.size()==2&&policy->installed_constraints_.at("set")=="BaseGame"&&
         policy->installed_constraints_.at("platform")=="Common"&&!policy->comparison_active_&&!policy->global_transition_active_&&
         common->consumer->thread_==creator&&!common->consumer->active_&&!common->consumer->failed_&&
         common->consumer->source_.get()==common->source_identity&&common->source_identity==source.owned_skin_effect_source_identity_&&
         common->consumer->completed_slot_selectors_==std::vector<int>{-1}&&
         common->bank_operation_identity==source.owned_skin_effect_bank_operation_&&
         common->returned_epoch==source.owned_skin_operation_epoch_&&common->returned_epoch==source.owned_skin_effect_operation_epoch_&&
         common->returned_stage_count==source.owned_category_stages_.size()&&
         common->returned_output_count==source.owned_skin_output_receipts_.size()&&
         common->returned_read_count==source.owned_skin_read_receipts_.size()&&
         !source.owned_skin_base_game_consumer_&&!source.owned_skin_global_kernel_&&i.default_state->live_enabled89==1,
         "Installed6415F0 substituted actual policy/real first Consumer return or changed Common constraints/Default");
    const auto bank=common->bank.lock();
    need(bank&&i.global_color_bank.get()==bank.get()&&source.owned_bank_receiver_.lock().get()==bank.get()&&
         !bank->closed_.load()&&!bank->busy_.load()&&!bank->active_owned_skin_operation_&&
         bank->phase_==BnwNativeGlobalColorRegistryBank::Phase::roster_bound&&
         bank->owned_source_.get()==source.owned_bank_source_identity_&&
         bank->owned_roster_.size()==source.owned_skin_instances_.size()&&
         common->final_instances.size()==source.owned_skin_instances_.size()&&
         common->selection->color->constructors.size()==source.owned_skin_instances_.size(),
         "Installed6415F0 lost actual closed Bank/instance roster");
    for(std::size_t n=0;n<source.owned_skin_instances_.size();++n){
        const auto instance=source.owned_skin_instances_[n];const auto& row=common->final_instances[n];
        need(instance&&row.instance.lock().get()==instance.get()&&row.enabled==instance->enabled_&&
             bank->owned_roster_[n].lock().get()==instance.get()&&
             common->selection->color->constructors[n].source==instance->constructor_source_.get(),
             "Installed6415F0 changed actual first Common constructor/instance/enabled return");
        for(unsigned slot=0;slot<2;++slot)for(unsigned member=0;member<2;++member){const auto manager=instance->owned_resource_pairs_[slot][member];
            need(same(row.pairs[slot][member],manager)&&row.pairs[slot][member].lock().get()==manager.get()&&
                 (manager?row.current89[slot][member]==manager->current89:!row.current89[slot][member]),
                 "Installed6415F0 changed actual first Common pair/+89 state");}
    }
}
void detail::BnwNativeGlobalResourceWarmOwner::InstalledPcPublication::publication_identity()const {
    need(creator==std::this_thread::get_id(),"Installed PC publication state foreign creator");
    if(call_returned||returned)need(policy->installed_pc_publication_attempted_&&policy->installed_pc_publication_returned_&&
         policy->installed_pc_copied_bytes_==2&&policy->actual_platform_=="PC"&&policy->phase_==Policy::Phase::pc_published,
         "Installed6415F0 has no actual successful copy/publication return");
    else if(publishing){
        need(policy->installed_pc_publication_attempted_,"Installed6415F0 actual write occurrence unconsumed");
        if(policy->installed_pc_copied_bytes_==0)need(policy->actual_platform_.empty()&&
             policy->phase_==Policy::Phase::common_active&&!policy->installed_pc_publication_returned_,
             "Installed6415F0 PC appeared before genuine copy");
        else need(policy->installed_pc_copied_bytes_==2&&policy->actual_platform_=="PC"&&
             (policy->phase_==Policy::Phase::common_active||policy->phase_==Policy::Phase::pc_published)&&
             (!policy->installed_pc_publication_returned_||policy->phase_==Policy::Phase::pc_published),
             "Installed6415F0 intermediate copy/phase source changed");
    }else need(!policy->installed_pc_publication_attempted_&&!policy->installed_pc_publication_returned_&&
         !policy->installed_pc_copied_bytes_&&policy->actual_platform_.empty()&&policy->phase_==Policy::Phase::common_active,
         "Installed6415F0 actual PC was published before its operation");
}
struct detail::BnwNativeGlobalResourceWarmOwner::InstalledPcPublication::Operation final
    :Selection::Continuation {
    InstalledPcPublication& data;
    Warm& source;
    const Parent& parent;
    Parent::Impl& i;
    Operation(InstalledPcPublication& value,Warm& owner,const Parent& facade,Parent::Impl& actual)
        :data(value),source(owner),parent(facade),i(actual){
        need(data.creator==std::this_thread::get_id()&&!data.operation&&!data.common->selection->continuation,
             "Installed PC actual publication continuation replay/foreign creator");
        data.operation=this;data.common->selection->continuation=this;
    }
    ~Operation(){
        if(data.operation==this&&data.common->selection->continuation==this){data.common->selection->continuation=nullptr;data.operation=nullptr;}
        else {data.failed=true;i.header_poisoned=true;}
    }
    Operation(const Operation&)=delete;
    void identity()const {
        need(data.creator==std::this_thread::get_id(),"Installed6415F0 operation foreign creator");
        need(data.operation==this&&data.common->selection->continuation==this&&!data.returned,
             "Installed6415F0 actual publication operation retired");
        data.closed_common_identity(source,parent,i);data.publication_identity();
    }
    void current()const {identity();source.require_current();identity();data.policy->require_current();identity();}
    void require_actual_inputs_current(const Input& actual,const Boot& actual_boot)const override {
        identity();const auto language=data.common->selection->language.lock();
        need(language&&language.get()==&actual&&i.settings.get()==&actual_boot&&same(i.settings,data.boot),
             "Installed PC publication substituted actual Input/Boot");identity();
    }
    void require_actual_pc_publication_current(const Policy& actual)const override {
        identity();need(data.publishing&&!data.call_returned&&data.policy.get()==&actual&&data.operation==this,
             "Installed6415F0 write outside its new actual post-Common publication call");identity();
    }
};
void detail::BnwNativeGlobalResourceWarmOwner::InstalledInitialSkinSelection::require_actual_pc_publication_current(const Policy& actual)const {
    need(creator==std::this_thread::get_id(),"Installed PC source foreign creator");
    need(prefix_returned&&!failed&&counter_returned&&enabled_instances==0&&pending==Pending::common660330&&
         continuation&&!operation&&!policy_construction_active,
         "Installed PC source has no actual post-prefix publication continuation");
    continuation->require_actual_pc_publication_current(actual);
}
void detail::BnwNativeGlobalResourceWarmOwner::publish_installed_pc() {
    need(thread_==std::this_thread::get_id(),"Installed6415F0 driver foreign creator");
    if(installed_pc_publication_attempted_){failed_.store(true);throw std::runtime_error("Installed6415F0 replay/reentry");}
    installed_pc_publication_attempted_=true; // Negative occurrence before callbacks/pins/allocation.
    try {
        ParentPin retained(*this);const auto storage=retained.parent->impl_;auto& i=*storage;
        require_installed_first_common_skin_returned(); // Genuine old closed lender ONLY before PC mutation/new continuation.
        need(!installed_pc_publication_&&installed_initial_skin_effects_&&owned_skin_boot_policy_&&
             !installed_initial_skin_effects_->selection->continuation,
             "Installed6415F0 must start from actual first Common normal return");
        const auto data=std::make_shared<InstalledPcPublication>(i,*this);installed_pc_publication_=data;
        struct Failure {BnwNativePackageDiscoveryOwner::Impl& i;std::shared_ptr<InstalledPcPublication> data;bool done=false;
            ~Failure(){if(!done){data->failed=true;i.header_poisoned=true;}}} failure{i,data};
        {
            InstalledPcPublication::Operation operation(*data,*this,*retained.parent,i);operation.current();
            BnwPrimaryDefaultStageTrace trace("Installed.SkinSelection.publishPC6415F0");
            {
                need(!data->publishing&&!data->call_returned,"Installed6415F0 actual call replay");
                data->publishing=true;
                struct End {InstalledPcPublication& data;~End(){data.publishing=false;}} end{*data};
                data->policy->publish_installed_pc_6415f0();
                data->call_returned=true;operation.current();
            }
            operation.current();
        }
        need(!data->operation&&!data->common->selection->continuation&&!data->publishing&&data->call_returned,
             "Installed6415F0 publication operation failed to return/retire");
        data->closed_common_identity(*this,*retained.parent,i);data->publication_identity();require_current();
        data->returned=true;
        i.frame.next_resource_dependency="Actual6415F0(PC) text publication returned; SAME pending778E40(platform,PC) then second660330, constraint clear,4691E0 and late Default tail remain mandatory";
        failure.done=true;
    }catch(...){failed_.store(true);throw;}
}
void detail::BnwNativeGlobalResourceWarmOwner::require_installed_pc_publication_returned()const {
    need(thread_==std::this_thread::get_id(),"Installed PC normal-return lender foreign creator");
    ParentPin retained(*this);const auto storage=retained.parent->impl_;const auto data=installed_pc_publication_;
    try {
        need(data&&data->returned&&!data->failed&&data->call_returned&&!data->publishing&&!data->operation&&
             !data->common->selection->continuation,
             "Installed PC has no actual closed6415F0 publication return");
        data->closed_common_identity(*this,*retained.parent,*storage);data->publication_identity();require_current();
        // No retired Consumer/policy/Reader/Source8b operation callback.
    }catch(...){if(data)data->failed=true;failed_.store(true);storage->header_poisoned=true;throw;}
}
void BnwNativePackageDiscoveryOwner::advance_installation_pc_publication()const {
    const auto storage=impl_;auto& i=*storage;need(i.thread==std::this_thread::get_id(),"Installed6415F0 Parent foreign creator");
    const auto warm=i.live_resource_query;need(i.installation_input&&!i.lease&&!i.rules&&warm,
        "Installed6415F0 requires actual installed Input/Warm");
    try{warm->publish_installed_pc();warm->require_installed_pc_publication_returned();}
    catch(...){warm->failed_.store(true);i.header_poisoned=true;throw;}
}
