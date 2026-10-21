// E2b2 bridge. No global driver or accepting continuation is supplied here.
std::uint64_t BnwNativePackageDiscoveryOwner::require_installed_global_effects_join(
    const detail::BnwNativeOwnedSkinConsumer& consumer,const detail::BnwNativeOwnedSkinBootPolicy& policy,
    const void* actual_source,InstalledGlobalBorrowCall call,std::uint64_t expected_serial)const {
    const auto storage=impl_;const auto& i=*storage;
    need(i.thread==std::this_thread::get_id(),"Installed global effects join foreign creator");
    const auto warm=i.live_resource_query;
    need(warm&&i.installation_input&&!i.lease&&!i.rules,"Installed global effects join lacks actual installed origin");
    using Close=detail::BnwNativeGlobalResourceWarmOwner::InstalledInitialConstraintClose;
    const auto closed=warm->installed_initial_constraint_close_;
    need(closed&&closed->returned&&!closed->failed&&!closed->operation&&!closed->closing&&closed->call_returned&&
         Close::same(i.self,closed->parent)&&Close::same(i.live_resource_query,closed->warm)&&
         Close::same(i.installation_input,closed->input)&&Close::same(i.identity,closed->epoch)&&
         Close::same(i.settings,closed->boot)&&Close::same(i.resource_catalog,closed->catalog)&&
         closed->policy.get()==&policy&&warm->owned_skin_boot_policy_.get()==&policy&&
         policy.thread_==i.thread&&policy.phase_==detail::BnwNativeOwnedSkinBootPolicy::Phase::returned&&
         policy.installed_source_&&policy.lease_.expired()&&policy.installed_initial_close_call_returned_&&
         policy.installed_constraints_.clear_returned()&&policy.installed_constraints_.release_returned()&&
         !policy.installed_constraints_.has_header()&&policy.installed_setters_returned_==3&&policy.actual_platform_=="PC"&&
         !policy.installed_global_read_failed_,"Installed global effects join lost genuine closed E1/PC history");
    const auto pc=closed->previous;
    need(pc&&pc.get()==warm->installed_pc_skin_effects_.get()&&pc->returned&&!pc->failed&&!pc->operation&&
         pc->publication&&pc->publication->common&&pc->consumer.get()==&consumer,
         "Installed global effects join has another genuine PC/Consumer ancestry");
    const auto common=pc->publication->common;const auto selection=common->selection;
    need(common.get()==warm->installed_initial_skin_effects_.get()&&common->returned&&!common->failed&&!common->operation&&
         common->consumer.get()==&consumer&&Close::same(common->consumer,consumer.weak_from_this())&&
         consumer.thread_==i.thread&&!consumer.failed_&&actual_source&&consumer.source_.get()==actual_source&&
         common->source_identity==actual_source&&warm->owned_skin_effect_source_identity_==actual_source&&
         !common->continuation_effects_source.expired()&&Close::same(common->continuation_effects_source,consumer.source_)&&
         selection&&selection.get()==warm->installed_initial_skin_selection_.get()&&selection->prefix_returned&&
         !selection->failed&&!selection->operation&&selection->enabled_instances==0&&
         selection->continuation&&common->continuation&&
         policy.installed_source_->lock().get()==static_cast<const detail::BnwNativeOwnedSkinBootPolicy::InstalledSource*>(selection.get()),
         "Installed global effects join lacks SAME original Source and NEW dual continuations");
    const auto bank=pc->bank.lock();
    need(!warm->failed_.load()&&!warm->closed_.load()&&!i.header_poisoned&&!i.category_lineage_failed&&
         i.header_active&&i.active_header_fence&&i.active_live_query_owner==warm.get()&&
         warm->active_owned_skin_operation_epoch_&&warm->active_owned_skin_operation_epoch_>pc->returned_epoch&&
         warm->active_owned_skin_resource_loan_&&!warm->active_owned_skin_package_.expired()&&
         bank&&bank.get()==i.global_color_bank.get()&&warm->owned_bank_receiver_.lock().get()==bank.get()&&
         !bank->closed_.load()&&bank->busy_.load()&&bank->active_owned_skin_operation_&&
         bank->active_owned_skin_operation_->scope.bank.get()==bank.get()&&
         bank->phase_==detail::BnwNativeGlobalColorRegistryBank::Phase::roster_bound,
         "Installed global effects join requires NEW real physical Header/Loaded/Bank frame");
    const auto* continuation=common->continuation;const auto* selection_continuation=selection->continuation;
    const auto serial=warm->active_owned_skin_operation_epoch_;
    need(serial==warm->owned_skin_operation_epoch_&&
         (expected_serial?serial==expected_serial:call==InstalledGlobalBorrowCall::factory),
         "Installed global effects loan belongs to another actual operation epoch");
    const auto* header=i.active_header_fence;const auto* resource=warm->active_owned_skin_resource_loan_;
    const auto* bank_operation=bank->active_owned_skin_operation_;
    if(call==InstalledGlobalBorrowCall::factory) {
        need(!consumer.active_&&consumer.completed_slot_selectors_==std::vector<int>{-1,-1},
             "Installed global effects factory before real inactive PC return");
        continuation->require_actual_global_effects_factory_current(policy,consumer,actual_source);
    }else continuation->require_actual_global_effects_borrow_current(policy,consumer,actual_source);
    // The new actual continuation must bind SAME Selection/resource scopes,
    // full physical/current metadata, actual call affinity and outside owner.
    // All older contexts default-refuse; mere dual-pointer presence cannot pass.
    need(common->continuation==continuation&&selection->continuation==selection_continuation&&
         warm->active_owned_skin_operation_epoch_==serial&&warm->owned_skin_operation_epoch_==serial&&
         i.header_active&&i.active_header_fence==header&&
         i.active_live_query_owner==warm.get()&&warm->active_owned_skin_resource_loan_==resource&&
         bank->active_owned_skin_operation_==bank_operation&&bank->busy_.load()&&!warm->failed_.load()&&
         !i.header_poisoned&&!common->failed&&!closed->failed&&!policy.installed_global_read_failed_,
         "Installed global effects join retired during actual continuation callback");
    return serial; // Minted from real current frame; never sufficient without the full join.
}
void detail::BnwNativeGlobalResourceWarmOwner::InstalledInitialSkinSelection::with_actual_global_constraint_read(
    const Policy& actual,const std::function<void(const std::string&,const std::string&)>& input)const {
    need(creator==std::this_thread::get_id(),"Installed global constraint source foreign creator");
    const auto captured=policy.lock();
    need(prefix_returned&&!failed&&counter_returned&&enabled_instances==0&&!operation&&continuation&&
         captured&&captured.get()==&actual&&actual.installed_initial_close_call_returned_&&
         !actual.installed_constraints_.has_header()&&actual.installed_constraints_.release_returned(),
         "Installed global match requires its NEW actual slot-read continuation");
    const auto* active=continuation;
    active->with_actual_global_constraint_read(actual,input);
    need(!failed&&continuation==active&&policy.lock().get()==&actual,
         "Installed global constraint read continuation retired during callback");
}

// Real private installed adapters call these only inside SAME serial-bound Loan.
// Older contexts cannot accept either actual67 read observation or exclusion.
void BnwNativePackageDiscoveryOwner::record_installed_global_skin_set_read(
    const std::shared_ptr<const detail::BnwNativeOwnedUiSkinPoststageInstance>& instance,
    const std::optional<std::string>& actual)const {
    const auto storage=impl_;const auto& i=*storage;
    need(i.thread==std::this_thread::get_id(),"Installed global set observation foreign creator");
    const auto warm=i.live_resource_query;
    need(warm&&instance&&warm->installed_initial_skin_effects_,"Installed global set observation lacks actual issuer");
    const auto common=warm->installed_initial_skin_effects_;
    const auto consumer=common->consumer;const auto policy=common->policy;
    const auto serial=warm->active_owned_skin_operation_epoch_;
    need(consumer&&policy&&serial,"Installed global set observation has no actual frame");
    const auto check=[&]{require_installed_global_effects_join(*consumer,*policy,consumer->source_.get(),
        InstalledGlobalBorrowCall::borrow,serial);};
    check();
    need(std::find(warm->owned_skin_instances_.begin(),warm->owned_skin_instances_.end(),instance)!=warm->owned_skin_instances_.end()&&
         instance->thread_==i.thread&&instance->owned_extra_attributes_issued_&&
         common->continuation,"Installed global set observation is not SAME constructed instance");
    const auto* continuation=common->continuation;
    // Issued only by the actual first-set getter after real transfer-list read.
    // Future controller must bind this exact instance/67 iteration/slot; text alone is not authority.
    continuation->observe_actual_global_set_read(*instance,actual);
    check();need(common->continuation==continuation,"Installed global set observer replaced actual continuation");
}
void BnwNativePackageDiscoveryOwner::disable_installed_global_skin_pair_then_instance(
    const std::shared_ptr<const detail::BnwNativeOwnedUiSkinPoststageInstance>& instance,unsigned slot)const {
    const auto storage=impl_;auto& i=*storage;
    need(i.thread==std::this_thread::get_id(),"Installed4691 disable foreign creator");
    const auto warm=i.live_resource_query;
    need(warm&&instance&&slot<2&&warm->installed_initial_skin_effects_&&
         std::find(warm->owned_skin_instances_.begin(),warm->owned_skin_instances_.end(),instance)!=warm->owned_skin_instances_.end(),
         "Installed4691 disable lacks SAME actual constructor roster/slot");
    const auto common=warm->installed_initial_skin_effects_;
    const auto consumer=common->consumer;const auto policy=common->policy;
    const auto serial=warm->active_owned_skin_operation_epoch_;
    need(consumer&&policy&&serial,"Installed4691 disable has no genuine new invocation");
    const auto pair=instance->owned_resource_pairs_[slot];
    const auto check=[&]{
        require_installed_global_effects_join(*consumer,*policy,consumer->source_.get(),InstalledGlobalBorrowCall::borrow,serial);
        const auto* continuation=common->continuation;
        need(continuation&&!consumer->active_,"Installed4691 disable is not actual global exclusion");
        continuation->require_actual_global_exclusion_current(*instance,slot);
        need(common->continuation==continuation,"Installed4691 disable continuation changed");
        instance->require_current();
        need(instance->owned_resource_pairs_[slot]==pair,"Actual767410 disable pair publication changed");
        for(const auto& manager:pair){if(!manager)continue;
            const auto found=std::find_if(warm->owned_category_stages_.begin(),warm->owned_category_stages_.end(),
                [&](const auto& stage){return stage&&stage->manager.lock().get()==manager.get();});
            need(found!=warm->owned_category_stages_.end(),"Actual767410 disable member lacks genuine namespace issuer");
            warm->require_owned_skin_stage_identity_without_callbacks(**found);
            warm->require_owned_category_stage_current(**found);
        }
        require_installed_global_effects_join(*consumer,*policy,consumer->source_.get(),InstalledGlobalBorrowCall::borrow,serial);
    };
    check();need(instance->enabled_[slot]!=0,"Actual4691 disable branch lacks enabled instance byte");
    //767410 visits each actual nullable pair member in native order. No first
    //member presence gate, replacement manager or synthetic namespace exists.
    for(unsigned member=0;member<2;++member){if(!pair[member])continue;
        i.resource_catalog->set_current89(pair[member],0);check();
    }
    //4691 writes the actual instance byte only AFTER767410 returns.
    instance->enabled_[slot]=0;check();
    auto receipt=std::make_shared<detail::BnwNativeGlobalResourceWarmOwner::OwnedSkinStateWrite>();
    receipt->instance=instance;receipt->slot=slot;receipt->issue_epoch=warm->active_owned_skin_operation_epoch_;
    receipt->actual_effect_source=consumer->source_.get();
    receipt->actual_bank_operation=i.global_color_bank->active_owned_skin_operation_;receipt->actual_pair=pair;
    warm->owned_skin_state_writes_.push_back(receipt);check();
}
