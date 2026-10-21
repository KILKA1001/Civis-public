// Actual778E40(platform,PC) and the second660330 only. Old returned sources
// stay history. This owner holds a NEW physical resource invocation.
struct detail::BnwNativeGlobalResourceWarmOwner::InstalledPcSkinEffects {
    using Warm=BnwNativeGlobalResourceWarmOwner;
    using Parent=BnwNativePackageDiscoveryOwner;
    using Loaded=BnwNativeStandalonePackageLoaded;
    using Bank=BnwNativeGlobalColorRegistryBank;
    using Policy=BnwNativeOwnedSkinBootPolicy;
    using Consumer=BnwNativeOwnedSkinConsumer;
    using Common=InstalledInitialSkinEffects;
    using Publication=InstalledPcPublication;
    using Selection=InstalledInitialSkinSelection;
    using Input=runtime::BnwSelectedLanguageInputOwner;
    using Boot=BnwNativeResourceBootSettingsOwner;
    const std::thread::id creator=std::this_thread::get_id();
    const std::weak_ptr<const Parent> parent;
    const std::weak_ptr<const void> warm,input,epoch,boot,catalog;
    const std::shared_ptr<Publication> publication;
    const std::shared_ptr<Policy> policy;
    const std::shared_ptr<Consumer> consumer; // Internal actual owner locked from genuine8b mutable lender.
    const std::weak_ptr<Bank> bank;
    bool failed=false,setter_active=false,setter_returned=false,invoking=false,kernel_returned=false;
    bool physical_post_returned=false,finishing=false,finish_call_returned=false,returned=false,native_changed=false;
    std::uint64_t returned_epoch=0;
    const Bank::OwnedSkinOperation* bank_operation_identity=nullptr;
    std::vector<Common::FinalInstance> final_instances;
    std::size_t returned_stage_count=0,returned_output_count=0,returned_read_count=0;
    struct Operation;
    Operation* operation=nullptr;
    InstalledPcSkinEffects(const Parent::Impl& i,const Warm& source)
        :parent(i.self),warm(i.live_resource_query),input(i.installation_input),epoch(i.identity),boot(i.settings),
         catalog(i.resource_catalog),publication(source.installed_pc_publication_),policy(publication->policy),
         consumer(publication->common->continuation_consumer.lock()),bank(publication->common->bank){}
    static bool same(const auto& a,const auto& b){return !a.owner_before(b)&&!b.owner_before(a);}
};
struct detail::BnwNativeGlobalResourceWarmOwner::InstalledPcSkinEffects::Operation final
    :Common::Continuation,Selection::Continuation {
    InstalledPcSkinEffects& data;
    Warm& source;
    const Parent& parent;
    Parent::Impl& i;
    const Parent::Impl::HeaderGuard* header=nullptr;
    const Loaded::Loan* loan=nullptr;
    const Bank::OwnedSkinOperation* bank_operation=nullptr;
    std::uint64_t serial=0;
    Operation(InstalledPcSkinEffects& value,Warm& owner,const Parent& facade,Parent::Impl& storage)
        :data(value),source(owner),parent(facade),i(storage){
        need(data.creator==std::this_thread::get_id()&&!data.operation&&!data.publication->common->continuation&&
             !data.publication->common->selection->continuation,"Installed PC invocation continuation replay/foreign creator");
        data.operation=this;data.publication->common->continuation=this;data.publication->common->selection->continuation=this;
    }
    ~Operation(){
        if(data.operation==this&&data.publication->common->continuation==this&&data.publication->common->selection->continuation==this){
            data.publication->common->selection->continuation=nullptr;data.publication->common->continuation=nullptr;data.operation=nullptr;
        }else{data.failed=true;i.header_poisoned=true;}
    }
    Operation(const Operation&)=delete;
        void loan_identity(const Loaded::Loan& held,std::size_t ordinal)const {
        need(ordinal<data.publication->common->selection->color->admission->rows.size()&&ordinal<source.returned_standalone_packages_.size(),
             "Installed Color/Skin actual loan ordinal escaped factory");
        const auto& row=data.publication->common->selection->color->admission->rows[ordinal];const auto& actual=source.returned_standalone_packages_[ordinal];
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
    void policy_identity()const {
        const auto& p=*data.policy;
        need(p.installed_pc_publication_attempted_&&p.installed_pc_publication_returned_&&p.installed_pc_copied_bytes_==2&&
             p.actual_platform_=="PC"&&p.installed_constraints_.size()==2&&p.installed_constraints_.at("set")=="BaseGame"&&
             !p.global_transition_active_,"Installed PC lost actual6415F0/value/constraint source");
        if(!data.setter_active&&!data.setter_returned)need(!p.installed_pc_constraint_attempted_&&!p.installed_pc_constraint_returned_&&
             p.installed_setters_returned_==2&&p.installed_constraints_.at("platform")=="Common"&&p.phase_==Policy::Phase::pc_published,
             "Installed PC constraint appeared before genuine778E40");
        else {
            need(p.installed_pc_constraint_attempted_,"Installed PC setter lacks actual occurrence");
            if(data.setter_active&&!data.setter_returned&&p.installed_setters_returned_==2)
                need(!p.installed_pc_constraint_returned_&&p.installed_constraints_.at("platform")=="Common"&&p.phase_==Policy::Phase::pc_published,
                     "Installed PC existing-key setter lost actual pre-copy state");
            else {
                need(p.installed_setters_returned_==3&&p.installed_constraints_.at("platform")=="PC"&&
                     (p.phase_==Policy::Phase::pc_active||(data.kernel_returned&&p.installed_pc_selection_finish_attempted_&&p.phase_==Policy::Phase::returned)),
                     "Installed PC setter/call return changed actual key/phase");
                if(data.setter_returned)need(p.installed_pc_constraint_returned_,"Installed PC setter has no real call return");
            }
        }
        if(p.installed_pc_selection_finish_attempted_)need(data.kernel_returned&&data.setter_returned&&data.physical_post_returned,
             "Installed PC return attempt preceded actual second kernel");
        if(p.installed_pc_selection_returned_||data.finish_call_returned)need(data.kernel_returned&&p.phase_==Policy::Phase::returned&&
             p.installed_pc_selection_finish_attempted_&&p.installed_pc_selection_returned_,"Installed PC return has no actual phase transition");
    }
    void identity()const {
        need(data.creator==std::this_thread::get_id()&&source.thread_==data.creator&&i.thread==data.creator&&
             !data.failed&&!data.returned&&!source.closed_.load()&&!source.failed_.load()&&!i.header_poisoned&&!i.category_lineage_failed&&
             data.operation==this&&source.installed_pc_skin_effects_.get()==&data&&source.installed_pc_skin_effects_attempted_,
             "Installed PC actual new invocation source/creator retired");
        const auto& pc=*data.publication;const auto& common=*pc.common;const auto& selection=*common.selection;const auto& color=*selection.color;
        need(i.installation_input&&!i.lease&&!i.rules&&i.live_resource_query.get()==&source&&same(i.self,data.parent)&&
             same(i.live_resource_query,data.warm)&&same(i.installation_input,data.input)&&same(i.identity,data.epoch)&&
             i.settings&&same(i.settings,data.boot)&&same(i.resource_catalog,data.catalog)&&
             data.publication.get()==source.installed_pc_publication_.get()&&pc.returned&&!pc.failed&&pc.call_returned&&!pc.publishing&&!pc.operation&&
             pc.common.get()==source.installed_initial_skin_effects_.get()&&common.returned&&!common.failed&&!common.operation&&common.kernel_returned&&
             common.continuation==this&&selection.continuation==this&&&selection==source.installed_initial_skin_selection_.get()&&
             selection.prefix_returned&&!selection.failed&&!selection.operation&&selection.counter_returned&&selection.enabled_instances==0&&
             selection.pending==Selection::Pending::common660330&&selection.color.get()==source.installed_color_skin_driver_.get()&&
             color.returned&&!color.failed&&!color.operation&&!color.constructor_loan&&!color.constructing&&!color.active_base_read&&
             color.admission&&color.admission->returned&&!color.admission->failed&&
             !i.active_completed_resource_operation&&!i.category_source&&!source.active_owned_category_preconstructor_,
             "Installed PC lost actual closed6415F0/first Common/history/Input controls");
        color.admission->identity(source,parent,i);
        need(data.policy&&data.policy.get()==pc.policy.get()&&data.policy.get()==common.policy.get()&&
             data.policy.get()==source.owned_skin_boot_policy_.get()&&same(selection.policy,data.policy)&&
             data.policy->installed_source_&&data.policy->installed_source_->lock().get()==static_cast<const Policy::InstalledSource*>(common.selection.get())&&
             same(*data.policy->installed_source_,common.selection)&&same(data.policy->input_,selection.language)&&same(data.policy->boot_,data.boot)&&
             data.policy->lease_.expired()&&data.consumer&&common.consumer.get()==data.consumer.get()&&same(common.consumer,data.consumer)&&
             same(common.continuation_consumer,data.consumer)&&data.consumer->thread_==data.creator&&!data.consumer->failed_&&
             data.consumer->source_.get()==common.source_identity&&common.source_identity==source.owned_skin_effect_source_identity_&&
             common.returned_epoch==source.owned_skin_effect_operation_epoch_&&common.bank_operation_identity==source.owned_skin_effect_bank_operation_&&
             source.owned_skin_occurrence_driver_returned_&&source.owned_skin_instance_ctor_driver_returned_&&
             source.owned_skin_occurrences_.size()==source.owned_skin_instances_.size()&&color.constructors.size()==source.owned_skin_instances_.size()&&
             !source.owned_skin_base_game_consumer_&&!source.owned_skin_global_kernel_,
             "Installed PC substituted SAME actual mutable Consumer/control/Source/policy or reset old epoch");
        const auto& trace=data.consumer->completed_slot_selectors_;
        if(data.invoking)need(trace==std::vector<int>{-1}||trace==std::vector<int>{-1,-1},"Installed PC actual active invocation selector trace changed");
        else need(!data.consumer->active_&&trace==(data.kernel_returned?std::vector<int>{-1,-1}:std::vector<int>{-1}),
             "Installed PC lost inactive actual first/second call return");
        const auto actual=data.bank.lock();
        need(actual&&i.global_color_bank.get()==actual.get()&&source.owned_bank_receiver_.lock().get()==actual.get()&&
             source.owned_bank_source_attachment_returned_&&actual->owned_source_.get()==source.owned_bank_source_identity_&&
             !actual->closed_.load()&&actual->phase_==Bank::Phase::roster_bound&&actual->owned_roster_.size()==source.owned_skin_instances_.size(),
             "Installed PC lost actual Bank/native roster");
        for(std::size_t n=0;n<source.owned_skin_instances_.size();++n){const auto instance=source.owned_skin_instances_[n];
            need(instance&&source.owned_skin_occurrences_[n]&&actual->owned_roster_[n].lock().get()==instance.get()&&
                 color.constructors[n].instance.lock().get()==instance.get()&&
                 color.constructors[n].occurrence.lock().get()==source.owned_skin_occurrences_[n].get()&&
                 color.constructors[n].source==instance->constructor_source_.get(),"Installed PC changed actual constructed roster/source");
        } // Old enabled/+89 snapshots are history; genuine PC effects may change current fields.
        policy_identity();
        if(header)need(serial&&i.header_active&&i.active_header_fence==header&&header->valid()&&i.active_live_query_owner==&source&&
             source.active_owned_skin_operation_epoch_==serial&&data.returned_epoch==serial&&serial>common.returned_epoch,
             "Installed PC lost genuine NEW Header/epoch");
        else need(!serial&&!i.header_active&&!i.active_live_query_owner&&!source.active_owned_skin_operation_epoch_,"Installed PC borrowed retired Header");
        if(loan){need(header&&source.active_owned_skin_resource_loan_==&loan->resource&&source.active_owned_skin_package_.lock().get()==&loan->token&&
             source.active_owned_skin_resource_epoch_==loan->actor->state_->active_resource_loan,"Installed PC lost NEW physical ResourceLoan");loan_identity(*loan,0);}
        else need(!source.active_owned_skin_resource_loan_,"Installed PC orphaned resource loan");
        if(bank_operation)need(header&&loan&&actual->busy_.load()&&actual->active_owned_skin_operation_==bank_operation&&
             bank_operation->scope.bank.get()==actual.get()&&data.bank_operation_identity==bank_operation,
             "Installed PC lost NEW actual owning Bank Scope");
    }
    void current()const {identity();source.require_current();if(loan)loan->require_current();identity();}
    void require_identity()const override {identity();}
    void require_operation_current()const override {current();}
    void require_actual_inputs_current(const Input& actual,const Boot& boot)const override {
        identity();const auto language=data.publication->common->selection->language.lock();
        need(language&&language.get()==&actual&&i.settings.get()==&boot&&same(i.settings,data.boot),"Installed PC new invocation substituted Input/Boot");identity();
    }
    void require_actual_pc_constraint_current(const Policy& actual)const override {
        identity();need(data.setter_active&&!data.setter_returned&&!data.invoking&&!data.kernel_returned&&data.policy.get()==&actual&&
             header&&loan&&bank_operation,"Installed PC setter is outside its actual new resource owner");identity();
    }
    void require_actual_pc_selection_return_current(const Policy& actual)const override {
        identity();need(data.finishing&&data.kernel_returned&&data.physical_post_returned&&!data.invoking&&!data.finish_call_returned&&data.policy.get()==&actual&&
             !data.consumer->active_&&data.consumer->completed_slot_selectors_==std::vector<int>{-1,-1}&&header&&loan&&bank_operation,
             "Installed PC return is not the real inactive SAME Consumer second return");identity();
    }
    void require_bank_current(const Bank& actual,const content::XmlNode* document,std::uint64_t ordinal)const override {
        identity();need(header&&loan&&!document&&ordinal==0&&data.bank.lock().get()==&actual&&i.global_color_bank.get()==&actual&&
             actual.thread_==data.creator&&actual.epoch_.get()==i.identity.get()&&same(actual.parent_,i.self)&&
             actual.resource_catalog_.get()==i.resource_catalog.get()&&actual.query_default_source_.lock().get()==i.current_default_source.get()&&
             actual.query_default_state_.lock().get()==i.default_state.get()&&actual.query_default_records_==&i.default_state->selected&&
             actual.live_resource_query_.lock().get()==&source,"Installed PC Bank borrower lost actual controls");identity();
    }
    void require_effect_source_current(const Policy& actual,const void* actual_source)const override {
        identity();need(header&&loan&&bank_operation&&data.policy.get()==&actual&&actual_source&&
             actual_source==data.publication->common->source_identity&&data.consumer->source_.get()==actual_source,
             "Installed PC Source is not SAME retained protected Source under NEW Bank/Loaded owner");
        data.policy->require_current();identity();
    }
    void require_stage_identity(const OwnedCategoryStage& stage)const override {
        identity();const auto& common=*data.publication->common;const auto& admission=*common.selection->color->admission;
        const auto found=std::find_if(source.owned_category_stages_.begin(),source.owned_category_stages_.end(),[&](const auto& v){return v.get()==&stage;});
        need(header&&loan&&found!=source.owned_category_stages_.end()&&stage.factory_ordinal<source.returned_standalone_packages_.size()&&stage.category<3&&
             stage.package.lock().get()==source.returned_standalone_packages_[stage.factory_ordinal].get()&&stage.document==admission.rows[stage.factory_ordinal].document&&
             stage.source&&stage.namespace_channel&&stage.source->owned_warm_&&stage.namespace_channel->owned_warm_&&
             same(*stage.source->owned_warm_,data.warm)&&same(*stage.namespace_channel->owned_warm_,data.warm)&&
             stage.source->descriptor_==stage.factory_ordinal&&stage.source->category_==stage.category&&stage.source->first_.get()==i.first_lineage.get()&&
             stage.namespace_channel->namespace_.get()==stage.source.get()&&!stage.namespace_channel->lease_&&
             stage.namespace_channel->installation_input_.get()==i.installation_input.get()&&stage.namespace_channel->cold_.get()==i.cold.get()&&
             stage.namespace_channel->first_.get()==i.first_lineage.get()&&stage.namespace_channel->epoch_.get()==i.identity.get(),
             "Installed PC Stage lost actual namespace/Loaded/Input/FIRST controls");
        const auto index=static_cast<std::size_t>(found-source.owned_category_stages_.begin());const auto manager=stage.manager.lock();
        if(stage.category<2){const auto categories=common.selection->color->categories;const auto actor=source.returned_standalone_packages_[stage.factory_ordinal]->actor_.lock();
            need(index<common.returned_stage_count&&stage.installed_initial_driver&&!stage.installed_skin_effects_driver&&!stage.installed_pc_skin_effects_driver&&
                 stage.installed_initial_driver->lock().get()==categories.get()&&same(*stage.installed_initial_driver,categories)&&
                 stage.step==OwnedCategoryStage::Step::name_registered&&manager&&stage.arena&&stage.cache&&stage.loose_source&&stage.selected_source&&
                 actor&&actor->state_->category_slots[stage.category]&&same(*actor->state_->category_slots[stage.category],stage.selected_source)&&
                 std::any_of(categories->returned_calls.begin(),categories->returned_calls.end(),[&](const auto& row){return row.ordinal==stage.factory_ordinal&&row.category==stage.category&&row.stage.lock().get()==&stage;}),
                 "Installed PC initial Stage lacks genuine Source6 normal return");
        }else{
            const auto instance=stage.skin_instance.lock();const auto occurrence=stage.skin_occurrence.lock();
            need(!stage.installed_initial_driver&&instance&&occurrence&&stage.skin_slot<2&&stage.skin_member==0&&
                 occurrence->roster_index_<source.owned_skin_instances_.size()&&source.owned_skin_instances_[occurrence->roster_index_].get()==instance.get()&&
                 source.owned_skin_occurrences_[occurrence->roster_index_].get()==occurrence.get()&&
                 common.selection->color->constructors[occurrence->roster_index_].source==instance->constructor_source_.get()&&
                 occurrence->package_.lock().get()==stage.package.lock().get()&&occurrence->document_==stage.document&&
                 occurrence->factory_ordinal_==stage.factory_ordinal&&occurrence->epoch_.lock().get()==i.identity.get(),
                 "Installed PC UISkin Stage lost actual constructor/occurrence/slot");
            if(stage.installed_pc_skin_effects_driver)need(!stage.installed_skin_effects_driver&&index>=common.returned_stage_count&&
                 stage.installed_pc_skin_effects_driver->lock().get()==&data&&same(*stage.installed_pc_skin_effects_driver,source.installed_pc_skin_effects_),
                 "Installed PC Stage lost distinct actual new issuer");
            else need(stage.installed_skin_effects_driver&&index<common.returned_stage_count&&stage.installed_skin_effects_driver->lock().get()==&common&&
                 same(*stage.installed_skin_effects_driver,data.publication->common),"Installed PC retained Stage lacks real8b provenance");
            if(manager)need(instance->owned_resource_pairs_[stage.skin_slot][0].get()==manager.get()&&stage.arena&&stage.cache&&
                 (stage.selected_source||(source.active_owned_category_stage_==&stage&&stage.step==OwnedCategoryStage::Step::paks_queried)),
                 "Installed PC Stage is not actual manager pair/Arena/cache");
            else need(source.active_owned_category_stage_==&stage&&stage.step==OwnedCategoryStage::Step::paks_queried&&
                 !instance->owned_resource_pairs_[stage.skin_slot][0],"Installed PC manager is not genuine pending lazy construction");
        }
    }
    void require_stage_current(const OwnedCategoryStage& stage)const override {
        require_stage_identity(stage);current();const auto package=source.returned_standalone_packages_[stage.factory_ordinal];
        package->with_actual_descriptor([&](const auto& held){held.require_current();const auto& admission=*data.publication->common->selection->color->admission;
            admission.authenticate(source,*package,held,admission.rows[stage.factory_ordinal]);
            need(&held.resource.actual_document==stage.document,"Installed PC Stage actual Parser changed");
            if(stage.category==2){const auto occurrence=stage.skin_occurrence.lock();const auto& doc=held.resource.actual_document;
                need(occurrence&&!doc.roots.empty(),"Installed PC Stage lacks real nthUISkin");
                const BnwNativePackageXmlParserOwner::Node* node=nullptr;auto remaining=occurrence->occurrence_ordinal_;
                for(const auto& child:doc.roots.front()->children)if(BnwNativePackageResourceCheckpoint::range(doc,child->name)=="UISkin"){
                    if(remaining){--remaining;continue;}node=child.get();break;}
                need(node==occurrence->node_,"Installed PC Stage changed actual nthUISkin");}held.require_current();});
        source.replay_owned_category_stage_io_current(stage);require_stage_identity(stage);
    }
    void require_read_identity(const void* address)const override {
        identity();const auto& common=*data.publication->common;
        const auto found=std::find_if(source.owned_skin_read_receipts_.begin(),source.owned_skin_read_receipts_.end(),[&](const auto& v){return v.get()==address;});
        need(found!=source.owned_skin_read_receipts_.end()&&header&&loan&&bank_operation,"Installed PC input lacks actual NEW physical owner/read receipt");
        const auto index=static_cast<std::size_t>(found-source.owned_skin_read_receipts_.begin());const auto& read=**found;
        const auto operation=read.operation;const auto occurrence=operation?operation->occurrence.lock():nullptr;const auto query=operation?operation->query:nullptr;
        if(read.installed_pc_driver)need(!read.installed_driver&&index>=common.returned_read_count&&read.installed_pc_driver->lock().get()==&data&&
             same(*read.installed_pc_driver,source.installed_pc_skin_effects_)&&operation&&operation->issue_epoch==serial,
             "Installed PC input lost genuine new slot issuer/epoch");
        else need(read.installed_driver&&index<common.returned_read_count&&read.installed_driver->lock().get()==&common&&
             same(*read.installed_driver,data.publication->common)&&operation&&operation->issue_epoch==common.returned_epoch&&operation->restoration_returned,
             "Installed PC retained input lacks actual restored8b read provenance");
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
             (!operation->restoration_returned?(source.active_owned_skin_read_operation_.get()==operation.get()&&operation->issue_epoch==serial&&i.default_state->live_enabled89==0):
                 (i.default_state->live_enabled89==1||(source.active_owned_skin_read_operation_&&i.default_state->live_enabled89==0&&
                  source.active_owned_skin_read_operation_->actual_default==i.default_state.get()&&source.active_owned_skin_read_operation_->issue_epoch==serial))),
             "Installed PC input lost actual query/ordinal/occurrence/Default toggle affinity");
        if(!read.open_error)need(read.identity.has_value(),"Installed PC bytes have no actual physical stream identity");identity();
    }
};
void detail::BnwNativeGlobalResourceWarmOwner::InstalledInitialSkinSelection::require_actual_pc_constraint_current(const Policy& policy)const {
    need(creator==std::this_thread::get_id(),"Installed PC setter source foreign creator");
    need(prefix_returned&&!failed&&counter_returned&&enabled_instances==0&&continuation&&!operation,
         "Installed PC setter lacks actual new continuation");continuation->require_actual_pc_constraint_current(policy);
}
void detail::BnwNativeGlobalResourceWarmOwner::InstalledInitialSkinSelection::require_actual_pc_selection_return_current(const Policy& policy)const {
    need(creator==std::this_thread::get_id(),"Installed PC return source foreign creator");
    need(prefix_returned&&!failed&&counter_returned&&enabled_instances==0&&continuation&&!operation,
         "Installed PC return lacks actual new invocation continuation");continuation->require_actual_pc_selection_return_current(policy);
}
void BnwNativePackageDiscoveryOwner::with_installed_pc_skin_effect_bank(const std::function<void(
    const std::shared_ptr<detail::BnwNativeGlobalColorRegistryBank>&,
    const std::shared_ptr<detail::BnwNativeOwnedSkinBootPolicy>&,
    const std::shared_ptr<detail::BnwNativeOwnedSkinConsumer>&)>& input)const {
    need(impl_->thread==std::this_thread::get_id(),"Installed PC kernel target foreign creator");
    const auto warm=impl_->live_resource_query;need(warm&&warm->installed_pc_skin_effects_,"Installed PC kernel actual owner absent");
    detail::BnwNativeGlobalResourceWarmOwner::ParentPin retained(*warm);const auto data=warm->installed_pc_skin_effects_;
    const auto check=[&]{need(data->operation,"Installed PC kernel target retired");data->operation->current();
        need(data->operation->header&&data->operation->loan&&data->operation->bank_operation&&data->setter_returned&&
             data->policy->phase_==detail::BnwNativeOwnedSkinBootPolicy::Phase::pc_active,
             "Installed PC kernel lacks actual setter/new physical Bank owner");};
    check();{const auto consume=input;check();need(bool(consume),"Installed PC kernel callback absent");
        const auto bank=data->bank.lock();need(bool(bank),"Installed PC actual Bank retired");consume(bank,data->policy,data->consumer);check();}check();
}
void BnwNativePackageDiscoveryOwner::begin_installed_pc_skin_invocation(const std::shared_ptr<detail::BnwNativeOwnedSkinConsumer>& consumer)const {
    require_native_owned_skin_operation_current();const auto data=impl_->live_resource_query->installed_pc_skin_effects_;
    need(data&&data->operation&&data->operation->bank_operation&&data->setter_returned&&!data->invoking&&!data->kernel_returned&&
         consumer&&consumer.get()==data->consumer.get()&&detail::BnwNativeGlobalResourceWarmOwner::InstalledPcSkinEffects::same(consumer,data->consumer)&&
         !consumer->active_&&!consumer->failed_&&consumer->completed_slot_selectors_==std::vector<int>{-1},
         "Installed PC invocation is not SAME genuine first-return Consumer");data->invoking=true;
}
void BnwNativePackageDiscoveryOwner::record_installed_pc_skin_consumer(const std::shared_ptr<detail::BnwNativeOwnedSkinConsumer>& consumer,bool changed)const {
    require_native_owned_skin_operation_current();const auto warm=impl_->live_resource_query;const auto data=warm->installed_pc_skin_effects_;
    need(data&&data->operation&&data->operation->bank_operation&&data->invoking&&!data->kernel_returned&&data->setter_returned&&
         consumer&&consumer.get()==data->consumer.get()&&detail::BnwNativeGlobalResourceWarmOwner::InstalledPcSkinEffects::same(consumer,data->consumer)&&
         !consumer->active_&&!consumer->failed_&&consumer->source_.get()==data->publication->common->source_identity&&
         consumer->completed_slot_selectors_==std::vector<int>{-1,-1}&&!warm->active_owned_skin_read_operation_&&impl_->default_state->live_enabled89==1,
         "Installed PC has no actual inactive SAME Consumer second C++ normal return");
    consumer->require_current();data->native_changed=changed;data->kernel_returned=true;data->invoking=false;
    for(const auto& instance:warm->owned_skin_instances_){detail::BnwNativeGlobalResourceWarmOwner::InstalledInitialSkinEffects::FinalInstance row;
        row.instance=instance;row.enabled=instance->enabled_;
        for(unsigned slot=0;slot<2;++slot)for(unsigned member=0;member<2;++member){const auto manager=instance->owned_resource_pairs_[slot][member];
            row.pairs[slot][member]=manager;if(manager)row.current89[slot][member]=manager->current89;}data->final_instances.push_back(std::move(row));}
    data->returned_stage_count=warm->owned_category_stages_.size();data->returned_output_count=warm->owned_skin_output_receipts_.size();
    data->returned_read_count=warm->owned_skin_read_receipts_.size();require_native_owned_skin_operation_current();
}
void detail::BnwNativeGlobalResourceWarmOwner::drive_installed_pc_skin() {
    using Parent=BnwNativePackageDiscoveryOwner;
    need(thread_==std::this_thread::get_id(),"Installed PC driver foreign creator");
    if(installed_pc_skin_effects_attempted_){failed_.store(true);throw std::runtime_error("Installed PC second invocation replay/reentry");}
    installed_pc_skin_effects_attempted_=true;
    try {
        ParentPin retained(*this);const auto storage=retained.parent->impl_;auto& i=*storage;
        require_installed_pc_publication_returned(); // Actual old CLOSED lender, before new mutation/operation.
        need(!installed_pc_skin_effects_&&!installed_initial_skin_effects_->continuation&&
             !installed_initial_skin_effects_->selection->continuation&&owned_skin_operation_epoch_!=UINT64_MAX,
             "Installed PC requires actual returned6415F0 before genuine new resource invocation");
        const auto data=std::make_shared<InstalledPcSkinEffects>(i,*this);installed_pc_skin_effects_=data;
        struct Failure {Parent::Impl& i;std::shared_ptr<InstalledPcSkinEffects> data;bool done=false;
            ~Failure(){if(!done){data->failed=true;i.header_poisoned=true;}}} failure{i,data};
        {
            InstalledPcSkinEffects::Operation operation(*data,*this,*retained.parent,i);operation.current();
            BnwPrimaryDefaultStageTrace trace("Installed.SkinSelection.PC778E40-second660330");
            {
                Parent::Impl::HeaderGuard fence(i.header_callback_state,i.self);need(fence.valid(),"Installed PC NEW Header retired/reentered");
                struct Active {Parent::Impl& i;BnwNativeGlobalResourceWarmOwner& source;InstalledPcSkinEffects::Operation& op;
                    Active(Parent::Impl& p,BnwNativeGlobalResourceWarmOwner& w,InstalledPcSkinEffects::Operation& o,const Parent::Impl::HeaderGuard& f)
                        :i(p),source(w),op(o){need(!i.header_active&&!i.active_live_query_owner&&!source.active_owned_skin_operation_epoch_,"Installed PC Header reentry");
                        op.header=&f;op.serial=++source.owned_skin_operation_epoch_;op.data.returned_epoch=op.serial;source.active_owned_skin_operation_epoch_=op.serial;
                        i.header_active=true;i.active_header_fence=&f;i.active_live_query_owner=&source;}
                    ~Active(){source.active_owned_skin_operation_epoch_=0;op.header=nullptr;op.serial=0;i.active_live_query_owner=nullptr;i.active_header_fence=nullptr;i.header_active=false;}
                } active{i,*this,operation,fence};
                returned_standalone_packages_.front()->with_actual_descriptor([&](const auto& held){
                    operation.loan=&held;active_owned_skin_package_=held.retained;active_owned_skin_resource_loan_=&held.resource;
                    active_owned_skin_resource_epoch_=held.actor->state_->active_resource_loan;
                    struct EndLoan {BnwNativeGlobalResourceWarmOwner& source;InstalledPcSkinEffects::Operation& op;
                        ~EndLoan(){source.restore_owned_skin_default_after_failure();op.loan=nullptr;source.active_owned_skin_package_.reset();source.active_owned_skin_resource_loan_=nullptr;source.active_owned_skin_resource_epoch_=0;}} end_loan{*this,operation};
                    operation.current();const auto bank=data->bank.lock();need(bool(bank),"Installed PC actual Bank retired");
                    {
                        BnwNativeGlobalColorRegistryBank::OwnedSkinOperation bank_operation(bank);operation.bank_operation=&bank_operation;data->bank_operation_identity=&bank_operation;
                        struct EndBank {InstalledPcSkinEffects::Operation& op;~EndBank(){op.bank_operation=nullptr;}} end_bank{operation};
                        operation.current();
                        {data->setter_active=true;struct End {InstalledPcSkinEffects& d;~End(){d.setter_active=false;}} end{*data};
                            data->policy->publish_installed_pc_constraint_778e40();data->setter_returned=true;operation.current();}
                        retained.parent->run_installed_pc_skin_kernel();operation.current();
                        need(data->kernel_returned&&!data->invoking&&!active_owned_skin_read_operation_&&!active_owned_category_stage_&&!initializing_owned_namespace_&&
                             i.default_state->live_enabled89==1,"Installed PC kernel left actual resource/read operation unfinished");
                        for(const auto& read:owned_skin_read_receipts_)retained.parent->require_native_owned_skin_read_receipt_current(read.get());
                        for(const auto& stage:owned_category_stages_)operation.require_stage_current(*stage);
                        operation.current();data->physical_post_returned=true;
                        {data->finishing=true;struct End {InstalledPcSkinEffects& d;~End(){d.finishing=false;}} end{*data};
                            data->policy->finish_installed_pc_selection();data->finish_call_returned=true;operation.current();}
                    }
                    need(!bank->active_owned_skin_operation_&&!bank->busy_.load(),"Installed PC NEW owning Bank Scope failed to return");operation.current();
                });operation.current();need(fence.valid(),"Installed PC whole physical Loaded POST lost NEW Header");
            }
            operation.current();need(data->kernel_returned&&data->finish_call_returned&&!data->invoking&&!data->finishing&&
                 !operation.header&&!operation.loan&&!operation.bank_operation,"Installed PC actual invocation/target/physical owner did not return");
        }
        need(!data->operation&&!data->publication->common->continuation&&!data->publication->common->selection->continuation&&
             !active_owned_skin_operation_epoch_&&!active_owned_skin_resource_loan_,"Installed PC NEW continuation failed to retire");
        require_current();data->returned=true;
        i.frame.next_resource_dependency="Actual778E40(platform,PC)/second660330/462C20 returned; SAME pending6B9530 constraint clear then4691E0/global and late Default tail remain mandatory";
        failure.done=true;
    }catch(...){failed_.store(true);throw;}
}
void detail::BnwNativeGlobalResourceWarmOwner::require_installed_pc_skin_returned()const {
    need(thread_==std::this_thread::get_id(),"Installed PC normal-return lender foreign creator");
    ParentPin retained(*this);const auto storage=retained.parent->impl_;const auto data=installed_pc_skin_effects_;
    try {
        need(data&&data->returned&&!data->failed&&!data->operation&&!data->setter_active&&data->setter_returned&&data->kernel_returned&&data->physical_post_returned&&
             !data->invoking&&!data->finishing&&data->finish_call_returned&&!data->publication->common->continuation&&
             !data->publication->common->selection->continuation&&!storage->header_active&&!active_owned_skin_operation_epoch_&&
             !active_owned_skin_resource_loan_&&!active_owned_skin_read_operation_&&!active_owned_category_stage_&&!initializing_owned_namespace_&&
             data->returned_epoch==owned_skin_operation_epoch_&&data->returned_epoch>data->publication->common->returned_epoch&&
             data->consumer&&data->consumer.get()==data->publication->common->consumer.get()&&InstalledPcSkinEffects::same(data->consumer,data->publication->common->consumer)&&
             InstalledPcSkinEffects::same(data->consumer,data->publication->common->continuation_consumer)&&
             !data->consumer->active_&&!data->consumer->failed_&&data->consumer->completed_slot_selectors_==std::vector<int>{-1,-1}&&
             data->consumer->source_.get()==data->publication->common->source_identity&&data->consumer->source_.get()==owned_skin_effect_source_identity_&&
             owned_skin_effect_operation_epoch_==data->publication->common->returned_epoch&&
             owned_skin_effect_bank_operation_==data->publication->common->bank_operation_identity&&
             data->policy.get()==owned_skin_boot_policy_.get()&&data->policy->phase_==BnwNativeOwnedSkinBootPolicy::Phase::returned&&
             data->policy->installed_pc_publication_returned_&&data->policy->installed_pc_constraint_returned_&&data->policy->installed_pc_selection_returned_&&
             data->policy->actual_platform_=="PC"&&data->policy->installed_setters_returned_==3&&data->policy->installed_constraints_.size()==2&&
             data->policy->installed_constraints_.at("set")=="BaseGame"&&data->policy->installed_constraints_.at("platform")=="PC"&&
             data->returned_stage_count==owned_category_stages_.size()&&data->returned_output_count==owned_skin_output_receipts_.size()&&
             data->returned_read_count==owned_skin_read_receipts_.size()&&data->final_instances.size()==owned_skin_instances_.size()&&
             storage->default_state->live_enabled89==1&&!owned_skin_base_game_consumer_&&!owned_skin_global_kernel_,
             "Installed PC has no real closed SAME Consumer second invocation/physical read/owner return");
        const auto bank=data->bank.lock();
        need(bank&&storage->global_color_bank.get()==bank.get()&&owned_bank_receiver_.lock().get()==bank.get()&&!bank->closed_.load()&&!bank->busy_.load()&&
             !bank->active_owned_skin_operation_&&bank->phase_==BnwNativeGlobalColorRegistryBank::Phase::roster_bound&&
             bank->owned_source_.get()==owned_bank_source_identity_&&bank->owned_roster_.size()==owned_skin_instances_.size(),"Installed PC returned Bank/roster changed");
        data->publication->common->selection->color->admission->identity(*this,*retained.parent,*storage);
        for(std::size_t n=0;n<owned_skin_instances_.size();++n){const auto instance=owned_skin_instances_[n];const auto& row=data->final_instances[n];
            need(instance&&row.instance.lock().get()==instance.get()&&row.enabled==instance->enabled_&&bank->owned_roster_[n].lock().get()==instance.get()&&
                 data->publication->common->selection->color->constructors[n].source==instance->constructor_source_.get(),"Installed PC returned constructor/instance/enabled fields changed");
            for(unsigned slot=0;slot<2;++slot)for(unsigned member=0;member<2;++member){const auto manager=instance->owned_resource_pairs_[slot][member];
                need(InstalledPcSkinEffects::same(row.pairs[slot][member],manager)&&row.pairs[slot][member].lock().get()==manager.get()&&
                     (manager?row.current89[slot][member]==manager->current89:!row.current89[slot][member]),"Installed PC returned actual manager pair/+89 changed");}
        }
        require_current(); // Pure outside lender; never call departed Consumer/Policy/Reader callbacks.
    }catch(...){if(data)data->failed=true;failed_.store(true);storage->header_poisoned=true;throw;}
}
void BnwNativePackageDiscoveryOwner::drive_installation_pc_skin()const {
    const auto storage=impl_;auto& i=*storage;need(i.thread==std::this_thread::get_id(),"Installed PC Parent foreign creator");
    const auto warm=i.live_resource_query;need(i.installation_input&&!i.lease&&!i.rules&&warm,"Installed PC requires actual installed Input/Warm");
    try{warm->drive_installed_pc_skin();warm->require_installed_pc_skin_returned();}
    catch(...){warm->failed_.store(true);i.header_poisoned=true;throw;}
}
