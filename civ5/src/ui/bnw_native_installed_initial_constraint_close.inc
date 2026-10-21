// E1 only: real initial constraints clear + actual owned C++ header release.
// Canonical PC text survives. No global/resource/Bank/window authority issued.
struct detail::BnwNativeGlobalResourceWarmOwner::InstalledInitialConstraintClose {
    using Warm=BnwNativeGlobalResourceWarmOwner;
    using Parent=BnwNativePackageDiscoveryOwner;
    using Policy=BnwNativeOwnedSkinBootPolicy;
    using Selection=InstalledInitialSkinSelection;
    using Input=runtime::BnwSelectedLanguageInputOwner;
    using Boot=BnwNativeResourceBootSettingsOwner;
    const std::thread::id creator=std::this_thread::get_id();
    const std::weak_ptr<const Parent> parent;
    const std::weak_ptr<const void> warm,input,epoch,boot,catalog;
    const std::shared_ptr<InstalledPcSkinEffects> previous;
    const std::shared_ptr<Policy> policy;
    bool failed=false,closing=false,call_returned=false,returned=false;
    struct Operation;
    Operation* operation=nullptr;
    InstalledInitialConstraintClose(const Parent::Impl& i,const Warm& source)
        :parent(i.self),warm(i.live_resource_query),input(i.installation_input),epoch(i.identity),boot(i.settings),
         catalog(i.resource_catalog),previous(source.installed_pc_skin_effects_),policy(previous->policy){}
    static bool same(const auto& a,const auto& b){return !a.owner_before(b)&&!b.owner_before(a);}
    void identity(const Warm& source,const Parent& actual,const Parent::Impl& i)const {
        need(creator==std::this_thread::get_id()&&source.thread_==creator&&i.thread==creator&&!failed&&
             !source.closed_.load()&&!source.failed_.load()&&!i.header_poisoned&&!i.category_lineage_failed,
             "Installed initial constraint close source/creator retired");
        need(source.installed_initial_constraint_close_.get()==this&&source.installed_initial_constraint_close_attempted_&&
             i.installation_input&&!i.lease&&!i.rules&&i.live_resource_query.get()==&source&&same(i.self,parent)&&
             same(i.live_resource_query,warm)&&same(i.installation_input,input)&&same(i.identity,epoch)&&i.settings&&same(i.settings,boot)&&
             same(i.resource_catalog,catalog)&&previous&&previous.get()==source.installed_pc_skin_effects_.get()&&
             previous->returned&&!previous->failed&&!previous->operation&&previous->setter_returned&&previous->kernel_returned&&
             previous->physical_post_returned&&previous->finish_call_returned&&!previous->invoking&&!previous->finishing&&
             !previous->publication->common->continuation&&!previous->publication->operation&&previous->publication->returned&&
             !previous->publication->failed&&previous->publication->common->returned&&!previous->publication->common->failed&&
             !previous->publication->common->operation&&previous->publication->common->selection->prefix_returned&&
             !previous->publication->common->selection->failed&&!previous->publication->common->selection->operation&&
             !i.header_active&&!i.active_header_fence&&!i.active_live_query_owner&&!i.active_completed_resource_operation&&
             !source.active_owned_skin_operation_epoch_&&!source.active_owned_skin_resource_loan_&&!source.active_owned_skin_read_operation_&&
             !source.active_owned_category_stage_&&!source.active_owned_category_preconstructor_&&!source.initializing_owned_namespace_&&!source.busy_.load()&&
             previous->returned_epoch==source.owned_skin_operation_epoch_,
             "Installed6B9530 lost genuine closed PC history or borrowed a retired resource operation");
        previous->publication->common->selection->color->admission->identity(source,actual,i);
        need(policy&&policy.get()==source.owned_skin_boot_policy_.get()&&policy.get()==previous->policy.get()&&
             policy->installed_source_&&policy->installed_source_->lock().get()==
                 static_cast<const Policy::InstalledSource*>(previous->publication->common->selection.get())&&
             same(*policy->installed_source_,previous->publication->common->selection)&&policy->lease_.expired()&&
             same(policy->input_,previous->publication->common->selection->language)&&same(policy->boot_,boot)&&
             policy->phase_==Policy::Phase::returned&&policy->actual_platform_=="PC"&&policy->installed_pc_publication_returned_&&
             policy->installed_pc_constraint_returned_&&policy->installed_pc_selection_returned_&&policy->installed_setters_returned_==3&&
             !policy->comparison_active_&&!policy->global_transition_active_&&previous->consumer&&
             previous->consumer.get()==previous->publication->common->consumer.get()&&same(previous->consumer,previous->publication->common->consumer)&&
             same(previous->consumer,previous->publication->common->continuation_consumer)&&!previous->consumer->active_&&!previous->consumer->failed_&&
             previous->consumer->completed_slot_selectors_==std::vector<int>{-1,-1}&&
             previous->consumer->source_.get()==source.owned_skin_effect_source_identity_&&
             source.owned_skin_effect_operation_epoch_==previous->publication->common->returned_epoch&&
             source.owned_skin_effect_bank_operation_==previous->publication->common->bank_operation_identity&&
             previous->returned_stage_count==source.owned_category_stages_.size()&&previous->returned_read_count==source.owned_skin_read_receipts_.size()&&
             previous->returned_output_count==source.owned_skin_output_receipts_.size()&&previous->final_instances.size()==source.owned_skin_instances_.size()&&
             !source.owned_skin_base_game_consumer_&&!source.owned_skin_global_kernel_&&i.default_state->live_enabled89==1,
             "Installed6B9530 substituted real PC owner/Consumer/source/return controls");
        const auto bank=previous->bank.lock();need(bank&&i.global_color_bank.get()==bank.get()&&source.owned_bank_receiver_.lock().get()==bank.get()&&
             !bank->closed_.load()&&!bank->busy_.load()&&!bank->active_owned_skin_operation_&&bank->phase_==BnwNativeGlobalColorRegistryBank::Phase::roster_bound&&
             bank->owned_roster_.size()==source.owned_skin_instances_.size(),"Installed6B9530 changed closed Bank/roster");
        for(std::size_t n=0;n<source.owned_skin_instances_.size();++n){const auto instance=source.owned_skin_instances_[n];const auto& row=previous->final_instances[n];
            need(instance&&row.instance.lock().get()==instance.get()&&row.enabled==instance->enabled_&&bank->owned_roster_[n].lock().get()==instance.get(),
                 "Initial constraint close changed actual PC instance/enabled state");
            for(unsigned slot=0;slot<2;++slot)for(unsigned member=0;member<2;++member){const auto manager=instance->owned_resource_pairs_[slot][member];
                need(same(row.pairs[slot][member],manager)&&row.pairs[slot][member].lock().get()==manager.get()&&
                     (manager?row.current89[slot][member]==manager->current89:!row.current89[slot][member]),"Initial constraint close changed actual pair/+89");}}
        const auto& owner=policy->installed_constraints_;
        if(call_returned||returned)need(policy->installed_initial_close_attempted_&&policy->installed_initial_close_call_returned_&&
             !owner.has_header()&&owner.clear_returned()&&owner.release_returned(),"Installed initial constraint owner has no real clear/release return");
        else if(closing){
            need(policy->installed_initial_close_attempted_,"Installed6B9530 actual close occurrence unconsumed");
            if(owner.has_header())need(!owner.release_returned()&&(owner.clear_returned()?owner.empty():
                 (owner.size()==2&&owner.at("set")=="BaseGame"&&owner.at("platform")=="PC")),"Installed6B9530 initial clear state changed");
            else need(owner.clear_returned()&&owner.release_returned(),"Installed initial header disappeared before actual clear/release");
        }else need(!policy->installed_initial_close_attempted_&&!policy->installed_initial_close_call_returned_&&owner.has_header()&&
             !owner.clear_returned()&&!owner.release_returned()&&owner.size()==2&&owner.at("set")=="BaseGame"&&owner.at("platform")=="PC",
             "Installed initial constraints changed before actual6B9530");
    }
};
struct detail::BnwNativeGlobalResourceWarmOwner::InstalledInitialConstraintClose::Operation final :Selection::Continuation {
    InstalledInitialConstraintClose& data;Warm& source;const Parent& parent;Parent::Impl& i;
    Operation(InstalledInitialConstraintClose& value,Warm& owner,const Parent& facade,Parent::Impl& storage)
        :data(value),source(owner),parent(facade),i(storage){
        need(data.creator==std::this_thread::get_id()&&!data.operation&&!data.previous->publication->common->selection->continuation,
             "Installed initial close continuation replay/foreign creator");
        data.operation=this;data.previous->publication->common->selection->continuation=this;
    }
    ~Operation(){if(data.operation==this&&data.previous->publication->common->selection->continuation==this){
        data.previous->publication->common->selection->continuation=nullptr;data.operation=nullptr;
    }else{data.failed=true;i.header_poisoned=true;}}
    Operation(const Operation&)=delete;
    void identity()const {need(data.creator==std::this_thread::get_id(),"Installed6B9530 operation foreign creator");
        need(data.operation==this&&!data.returned&&data.previous->publication->common->selection->continuation==this,"Installed6B9530 new operation retired");
        data.identity(source,parent,i);}
    void current()const {identity();source.require_current();identity();data.policy->require_current();identity();}
    void require_actual_inputs_current(const Input& actual,const Boot& actual_boot)const override {
        identity();const auto language=data.previous->publication->common->selection->language.lock();
        need(language&&language.get()==&actual&&i.settings.get()==&actual_boot&&same(i.settings,data.boot),"Initial close changed actual Input/Boot");identity();
    }
    void require_actual_initial_constraint_close_current(const Policy& actual)const override {
        identity();need(data.closing&&!data.call_returned&&data.policy.get()==&actual,"Initial close write outside genuine new closure caller");identity();
    }
};
void detail::BnwNativeGlobalResourceWarmOwner::InstalledInitialSkinSelection::require_actual_initial_constraint_close_current(const Policy& actual)const {
    need(creator==std::this_thread::get_id(),"Installed initial close source foreign creator");
    need(prefix_returned&&!failed&&counter_returned&&enabled_instances==0&&continuation&&!operation,
         "Initial close lacks real new caller continuation");continuation->require_actual_initial_constraint_close_current(actual);
}
void detail::BnwNativeGlobalResourceWarmOwner::close_installed_initial_constraints() {
    need(thread_==std::this_thread::get_id(),"Installed6B9530 driver foreign creator");
    if(installed_initial_constraint_close_attempted_){failed_.store(true);throw std::runtime_error("Installed6B9530 replay/reentry");}
    installed_initial_constraint_close_attempted_=true;
    try {
        ParentPin retained(*this);const auto storage=retained.parent->impl_;auto& i=*storage;
        require_installed_pc_skin_returned(); // ONLY before actual initial-map mutation/new continuation.
        need(!installed_initial_constraint_close_&&!installed_pc_skin_effects_->publication->common->selection->continuation,
             "Installed6B9530 requires genuine closed actual PC return");
        const auto data=std::make_shared<InstalledInitialConstraintClose>(i,*this);installed_initial_constraint_close_=data;
        struct Failure {BnwNativePackageDiscoveryOwner::Impl& i;std::shared_ptr<InstalledInitialConstraintClose> data;bool done=false;
            ~Failure(){if(!done){data->failed=true;i.header_poisoned=true;}}} failure{i,data};
        {
            InstalledInitialConstraintClose::Operation operation(*data,*this,*retained.parent,i);operation.current();
            BnwPrimaryDefaultStageTrace trace("Installed.SkinSelection.initial6B9530-close");
            {data->closing=true;struct End {InstalledInitialConstraintClose& d;~End(){d.closing=false;}} end{*data};
                data->policy->close_installed_initial_constraints_6b9530();data->call_returned=true;operation.current();}
            operation.current();
        }
        need(!data->operation&&!data->previous->publication->common->selection->continuation&&!data->closing&&data->call_returned,
             "Installed6B9530 actual clear/header caller failed to retire");
        data->identity(*this,*retained.parent,i);require_current();data->returned=true;
        i.frame.next_resource_dependency="Actual initial6B9530 entries clear and owned caller header release returned; SAME pending4691E0(-1) with NEW per-slot constraint/index owners before late Default/terminal suppliers";
        failure.done=true;
    }catch(...){failed_.store(true);throw;}
}
void detail::BnwNativeGlobalResourceWarmOwner::require_installed_initial_constraint_close_returned()const {
    need(thread_==std::this_thread::get_id(),"Installed6B9530 normal-return lender foreign creator");
    ParentPin retained(*this);const auto storage=retained.parent->impl_;const auto data=installed_initial_constraint_close_;
    try {need(data&&data->returned&&!data->failed&&!data->operation&&!data->closing&&data->call_returned&&
             !data->previous->publication->common->selection->continuation,"Initial close has no real closed container/header return");
        data->identity(*this,*retained.parent,*storage);require_current();
    }catch(...){if(data)data->failed=true;failed_.store(true);storage->header_poisoned=true;throw;}
}
void BnwNativePackageDiscoveryOwner::close_installation_initial_constraints()const {
    const auto storage=impl_;auto& i=*storage;need(i.thread==std::this_thread::get_id(),"Installed6B9530 Parent foreign creator");
    const auto warm=i.live_resource_query;need(i.installation_input&&!i.lease&&!i.rules&&warm,"Installed6B9530 requires actual installed Input/Warm");
    try{warm->close_installed_initial_constraints();warm->require_installed_initial_constraint_close_returned();}
    catch(...){warm->failed_.store(true);i.header_poisoned=true;throw;}
}
