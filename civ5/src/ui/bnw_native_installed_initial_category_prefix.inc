// Source6b: actual initial656440 package-major cursor, constructor and effects.
// The separate prefix leaves SAME pending6F84F0; only the full driver returns it.
struct detail::BnwNativeGlobalResourceWarmOwner::InstalledInitialCategoryDriver {
    using Warm=BnwNativeGlobalResourceWarmOwner;
    using Parent=BnwNativePackageDiscoveryOwner;
    using Loaded=BnwNativeStandalonePackageLoaded;
    using Stage=OwnedCategoryStage;
    const std::thread::id creator=std::this_thread::get_id();
    const std::weak_ptr<const Parent> parent;
    const std::weak_ptr<const void> warm,epoch,input,catalog;
    const std::shared_ptr<const InstalledStartupAdmission> admission;
    std::size_t ordinal=0;
    std::uint32_t category=0;
    bool failed=false,prefix_returned=false,native_loop_returned=false;
    struct ReturnedCall {std::size_t ordinal;std::uint32_t category;std::weak_ptr<const Stage> stage;};
    std::vector<ReturnedCall> returned_calls;
    struct ZeroReturn {std::size_t ordinal;std::uint32_t category,actual_flags;
        std::weak_ptr<const Loaded> package;};
    std::vector<ZeroReturn> zero_returns;
    std::weak_ptr<Stage> pending;
    struct Operation;
    Operation* operation=nullptr;
    InstalledInitialCategoryDriver(const Parent::Impl& i,const Warm& source)
        :parent(i.self),warm(i.live_resource_query),epoch(i.identity),input(i.installation_input),catalog(i.resource_catalog),
         admission(source.installed_startup_admission_){}
    static bool same(const auto& a,const auto& b){return !a.owner_before(b)&&!b.owner_before(a);}
};
struct detail::BnwNativeGlobalResourceWarmOwner::InstalledInitialCategoryDriver::Operation {
    InstalledInitialCategoryDriver& data;
    Warm& source;
    const Parent& parent;
    Parent::Impl& i;
    const std::thread::id creator=std::this_thread::get_id();
    const Parent::Impl::HeaderGuard* header=nullptr;
    const Loaded::Loan* loan=nullptr; // SAME real live stack Loaded/ResourceLoan only.
    const Stage* stage=nullptr;
    Operation(InstalledInitialCategoryDriver& d,Warm& w,const Parent& p,Parent::Impl& actual)
        :data(d),source(w),parent(p),i(actual) {
        need(creator==data.creator&&!data.operation,"Installed initial constructor operation replay/foreign creator");
        data.operation=this;
    }
    ~Operation(){if(data.operation==this)data.operation=nullptr;else{data.failed=true;i.header_poisoned=true;}}
    Operation(const Operation&)=delete;
    void identity()const {
        need(creator==std::this_thread::get_id()&&creator==data.creator&&i.thread==creator&&
             source.thread_==creator&&!data.failed&&!source.closed_.load()&&!source.failed_.load()&&
             !i.header_poisoned&&!i.category_lineage_failed&&data.operation==this,
             "Installed initial constructor creator/source/operation retired");
        need(source.installed_initial_category_driver_.get()==&data&&source.installed_initial_category_prefix_attempted_&&
             i.installation_input&&!i.lease&&!i.rules&&i.live_resource_query.get()==&source&&
             same(i.self,data.parent)&&same(i.live_resource_query,data.warm)&&same(i.identity,data.epoch)&&
             same(i.installation_input,data.input)&&data.admission&&data.admission->returned&&
             i.resource_catalog&&same(i.resource_catalog,data.catalog)&&
             !data.admission->busy&&!data.admission->failed&&
             source.installed_startup_admission_.get()==data.admission.get()&&
             !source.configured_activation_policy_returned_&&source.owned_activation_receipts_.empty()&&
             !source.active_owned_skin_operation_epoch_&&
             !i.active_completed_resource_operation&&!i.category_source&&!i.global_color_bank&&data.category<2&&
             data.ordinal<=source.returned_standalone_packages_.size(),
             "Installed initial constructor lost actual admission/Parent/Warm/cursor controls");
        data.admission->identity(source,parent,i); // Callback-free actual controls and accounted outside Parent.
        if(header)need(i.header_active&&i.active_header_fence==header&&header->valid()&&
             i.active_live_query_owner==&source,"Installed constructor lost SAME actual Header");
        else need(!i.header_active&&!i.active_live_query_owner,"Installed constructor borrowed unrelated Header");
        if(source.active_owned_category_preconstructor_){
            const auto& query=*source.active_owned_category_preconstructor_;
            need(header&&loan&&!stage&&query.resource_loan==&loan->resource&&query.ordinal==data.ordinal&&
                 query.category==data.category&&&query.header==header&&query.creator==creator,
                 "Installed preconstructor changed physical Header/cursor/ResourceLoan");
        }
        if(loan) {
            need(data.ordinal<data.admission->rows.size()&&data.ordinal<source.returned_standalone_packages_.size(),
                 "Installed constructor held loan escaped actual native cursor");
            const auto& row=data.admission->rows[data.ordinal];const auto& actual=source.returned_standalone_packages_[data.ordinal];
            need(loan->creator_thread==creator&&!loan->completed_source&&!loan->completed_cold&&
                 &loan->token==actual.get()&&loan->retained.get()==actual.get()&&same(loan->retained,row.package)&&
                 loan->actor&&same(loan->actor,row.actor)&&same(actual->actor_,row.actor),
                 "Installed constructor substituted SAME actual Loaded/receiver loan");
            const auto& load=loan->actor->state_;load->live();
            need(load->busy.load()&&load->active_resource_loan&&
                 load->active_resource_loan_address==&loan->resource&&same(loan->resource.state,load)&&
                 !loan->resource.completed_source&&!loan->resource.completed_cold&&
                 !load->actual_lookup_source&&load->pre_dx9_lookup_source&&load->adopted_warm&&
                 same(*load->pre_dx9_lookup_source,i.pre_dx9_discovery)&&same(*load->adopted_warm,i.live_resource_query)&&
                 same(load->descriptor_core,row.core)&&same(load->parsed_xml,row.parser)&&
                 same(load->descriptor_bytes,row.reader)&&same(load->metadata_input,row.language)&&
                 same(load->filename,row.filename)&&same(load->discovery,row.discovery)&&
                 same(load->descriptor_bytes->state_->source,row.stream)&&same(load->returned_loaded,row.package)&&
                 load->receiver.get()==actual->actual_receiver_&&load->completed_metadata_identity==row.metadata&&
                 load->descriptor_core->state_->fields.get()==row.fields&&
                 &loan->resource.actual_fields==row.fields&&&loan->resource.actual_document==row.document&&
                 row.fields->current_flags()==row.expected_flags&&row.fields->current_category_mask()==row.original_mask,
                 "Installed constructor lost actual active ResourceLoan/Core/Parser/Reader/current flags");
            const auto& core=load->descriptor_core->state_;
            const auto& parser=load->parsed_xml->state_;const auto& reader=load->descriptor_bytes->state_;
            need(core->thread==creator&&!core->closed.load()&&!core->failed.load()&&core->busy.load()&&
                 same(core->self,row.core)&&same(core->parser,row.parser)&&core->fields.get()==row.fields&&
                 core->metadata.get()==row.metadata&&core->document_identity==row.document&&
                 core->actual_settings&&same(*core->actual_settings,row.language)&&
                 parser->thread==creator&&!parser->closed.load()&&!parser->failed.load()&&parser->busy.load()&&
                 same(parser->self,row.parser)&&same(parser->reader,row.reader)&&parser->document.get()==row.document&&
                 reader->thread==creator&&!reader->closed.load()&&!reader->failed.load()&&reader->busy.load()&&
                 same(reader->reader,row.reader)&&same(reader->source,row.stream)&&reader->buffer&&
                 std::any_of(source.descriptor_sources_.begin(),source.descriptor_sources_.end(),[&](const auto& value){
                     return value&&same(value,row.stream);}),
                 "Installed constructor retired/substituted held Core/Parser/Reader/source phase");
        }
        if(stage)need(header&&loan&&source.active_owned_category_stage_==stage&&
             !source.owned_category_stages_.empty()&&source.owned_category_stages_.back().get()==stage&&
             data.pending.lock().get()==stage,"Installed constructor Stage escaped SAME actual operation");
    }
    void current()const {identity();source.require_current();if(loan)loan->require_current();identity();}
};

void detail::BnwNativeGlobalResourceWarmOwner::require_installed_initial_preconstructor_current(const OwnedCategoryPreconstructorOperation& query)const {
    need(thread_==std::this_thread::get_id(),"Installed preconstructor foreign creator");
    const auto data=installed_initial_category_driver_;
    need(data&&data->operation&&active_owned_category_preconstructor_==&query,
         "Installed preconstructor has no SAME actual driver operation");
    const auto& op=*data->operation;op.identity();
    need(!op.stage&&op.header&&op.loan&&&query.warm==this&&&query.parent==&op.i&&&query.header==op.header&&
         query.creator==data->creator&&query.ordinal==data->ordinal&&query.category==data->category&&
         query.resource_loan==&op.loan->resource&&query.resource_epoch==op.loan->actor->state_->active_resource_loan&&
         query.package.get()==&op.loan->token&&query.actor.get()==op.loan->actor.get()&&
         &query.fields==&op.loan->resource.actual_fields&&&query.document==&op.loan->resource.actual_document&&
         query.prior_stage_count==owned_category_stages_.size()&&
         std::all_of(owned_category_stages_.begin(),owned_category_stages_.end(),[](const auto& stage){
             return stage&&stage->step==OwnedCategoryStage::Step::name_registered;}),
         "Installed preconstructor changed actual Header/ResourceLoan/cursor/prior native returns");
}
void detail::BnwNativeGlobalResourceWarmOwner::require_installed_initial_category_stage_identity(const OwnedCategoryStage& stage)const {
    need(thread_==std::this_thread::get_id(),"Installed category Stage foreign creator");
    if(installed_color_skin_driver_){require_installed_color_skin_category_stage_identity(stage);return;}
    const auto data=installed_initial_category_driver_;
    need(data&&stage.installed_initial_driver&&
         !stage.installed_initial_driver->owner_before(data)&&!data.owner_before(*stage.installed_initial_driver)&&
         stage.installed_initial_driver->lock().get()==data.get()&&data->operation,
         "Installed category Stage has no SAME actual live operation");
    const auto& operation=*data->operation;operation.identity();
    const auto found=std::find_if(owned_category_stages_.begin(),owned_category_stages_.end(),[&](const auto& value){return value.get()==&stage;});
    need(found!=owned_category_stages_.end()&&operation.header&&operation.loan,
         "Installed category Stage escaped actual Header/Loaded operation");
    const bool current=operation.stage==&stage;
    if(current)need(stage.factory_ordinal==data->ordinal&&stage.category==data->category&&data->pending.lock().get()==&stage,
                   "Installed current Stage/cursor/pending source changed");
    else {
        const auto returned=std::find_if(data->returned_calls.begin(),data->returned_calls.end(),[&](const auto& call){
            return call.ordinal==stage.factory_ordinal&&call.category==stage.category&&call.stage.lock().get()==&stage;});
        need(returned!=data->returned_calls.end()&&stage.step==OwnedCategoryStage::Step::name_registered,
             "Installed prior Stage lacks actual normal5F6920 return");
        if(operation.stage){
            const auto active=std::find_if(owned_category_stages_.begin(),owned_category_stages_.end(),[&](const auto& value){return value.get()==operation.stage;});
            need(active!=owned_category_stages_.end()&&found<active,"Installed prior Stage is not earlier than actual current Stage");
        }else {
            const auto* pre=active_owned_category_preconstructor_;
            need(pre&&pre->active_query,"Installed prior Stage escaped actual preconstructor query");
            require_owned_category_preconstructor_query_without_callbacks(*pre,pre->active_query);
            need(static_cast<std::size_t>(found-owned_category_stages_.begin())<pre->prior_stage_count,
                 "Installed prior Stage is outside actual constructor cache prefix");
        }
    }
    need(stage.factory_ordinal<returned_standalone_packages_.size()&&stage.category<2&&
         stage.package.lock().get()==returned_standalone_packages_[stage.factory_ordinal].get()&&stage.document&&stage.source&&stage.namespace_channel&&
         stage.source->owned_warm_&&!stage.source->owned_warm_->owner_before(operation.i.live_resource_query)&&
         !operation.i.live_resource_query.owner_before(*stage.source->owned_warm_)&&
         stage.source->first_.get()==operation.i.first_lineage.get()&&stage.source->descriptor_==stage.factory_ordinal&&
         stage.source->category_==stage.category&&stage.namespace_channel->namespace_.get()==stage.source.get()&&
         stage.namespace_channel->cold_.get()==operation.i.cold.get()&&!stage.namespace_channel->lease_&&
         stage.namespace_channel->installation_input_.get()==operation.i.installation_input.get()&&
         stage.namespace_channel->first_.get()==operation.i.first_lineage.get()&&stage.namespace_channel->epoch_.get()==operation.i.identity.get(),
         "Installed category Stage lost real namespace/Cold/input/FIRST/ordinal state");
    const auto actor=returned_standalone_packages_[stage.factory_ordinal]->actor_.lock();
    need(bool(actor),"Installed category actual actor retired");
    const auto& slot=actor->state_->category_slots[stage.category];
    if(stage.selected_source)need(slot&&!slot->owner_before(stage.selected_source)&&
         !stage.selected_source.owner_before(*slot)&&stage.manager.lock()&&stage.manager.lock()->current89,
         "Installed receiver slot/manager replaced actual selected source");
    else need(current&&!slot,"Installed constructor exposed a slot before actual source creation");
}
void detail::BnwNativeGlobalResourceWarmOwner::require_installed_initial_category_stage_current(const OwnedCategoryStage& stage)const {
    need(thread_==std::this_thread::get_id(),"Installed Color/Skin Stage foreign creator");if(installed_color_skin_driver_){require_installed_color_skin_category_stage_current(stage);return;}
    require_installed_initial_category_stage_identity(stage);const auto data=installed_initial_category_driver_;
    auto& op=*data->operation;op.current();
    if(op.stage!=&stage){
        const auto package=returned_standalone_packages_[stage.factory_ordinal];
        package->with_actual_descriptor([&](const auto& loan){loan.require_current();
            data->admission->authenticate(*this,*package,loan,data->admission->rows[stage.factory_ordinal]);
            need(&loan.resource.actual_document==stage.document&&!(loan.resource.actual_fields.current_flags()&0x40U),
                 "Installed prior Stage lost actual retained Core/Parser admission");loan.require_current();});
    }
    require_installed_initial_category_stage_identity(stage);
}

bool detail::BnwNativeGlobalResourceWarmOwner::construct_installed_initial_category_current() {
    const auto data=installed_initial_category_driver_;
    need(data&&data->operation&&data->pending.expired()&&data->ordinal<returned_standalone_packages_.size(),
         "Installed constructor helper lacks actual cursor operation");
    auto& operation=*data->operation;auto& i=operation.i;
            const auto package=returned_standalone_packages_[data->ordinal];bool zero=false;
            package->with_actual_descriptor([&](const auto& held){
                need(!operation.loan,"Installed constructor attempted nested Loaded loan");operation.loan=&held;
                struct LoanEnd {InstalledInitialCategoryDriver::Operation& operation;~LoanEnd(){operation.loan=nullptr;}} loan_end{operation};
                operation.current();const auto& fields=held.resource.actual_fields;
                if(fields.current_flags()&0x40U) {
                    // Actual5F6920 zero branch occurs BEFORE Paks/slot effects.
                    data->zero_returns.push_back({data->ordinal,data->category,fields.current_flags(),package});
                    zero=true;operation.current();return;
                }
                need(!held.actor->state_->category_slots[data->category],
                     "Mandatory actual767410 existing-pair enable before initial constructor replay");
                std::vector<std::string> paks;
                BnwNativePackageDiscoveryOwner::Impl::HeaderGuard fence(i.header_callback_state,i.self);
                need(fence.valid(),"Installed namespace constructor Parent retired/reentered");
                struct Active {
                    BnwNativePackageDiscoveryOwner::Impl& i;BnwNativeGlobalResourceWarmOwner& warm;
                    InstalledInitialCategoryDriver::Operation& operation;
                    Active(BnwNativePackageDiscoveryOwner::Impl& value,BnwNativeGlobalResourceWarmOwner& source,
                        InstalledInitialCategoryDriver::Operation& actual,const BnwNativePackageDiscoveryOwner::Impl::HeaderGuard& held)
                        :i(value),warm(source),operation(actual){need(!i.header_active&&!i.active_live_query_owner,
                            "Installed constructor Header reentry");operation.header=&held;
                        i.header_active=true;i.active_header_fence=&held;i.active_live_query_owner=&warm;}
                    ~Active(){operation.stage=nullptr;operation.header=nullptr;warm.active_owned_category_stage_=nullptr;
                        warm.initializing_owned_namespace_=nullptr;i.active_live_query_owner=nullptr;i.active_header_fence=nullptr;i.header_active=false;}
                } active{i,*this,operation,fence};
                operation.current();
                {
                    OwnedCategoryPreconstructorOperation preconstructor(*this,i,fence,package,held.actor,data->ordinal,data->category,
                        &held.resource,held.resource.actual_fields,held.resource.actual_document);
                    preconstructor.first();
                    BnwNativePackageResourceCheckpoint::with_outputs_from_resource_loan(held.actual_resource_loan(),data->category,0,
                        BnwNativePackageResourceCheckpoint::Query::paks,[&](int,const auto& outputs){operation.current();paks=outputs;operation.current();});
                    preconstructor.first();
                }
                operation.current();const auto self=i.live_resource_query;
                auto stage=std::make_shared<OwnedCategoryStage>();stage->installed_initial_driver=data;
                stage->package=package;stage->document=&held.resource.actual_document;stage->factory_ordinal=data->ordinal;
                stage->category=data->category;stage->paks=std::move(paks);
                stage->source=std::shared_ptr<BnwNativeCategoryNamespaceLineage>(new BnwNativeCategoryNamespaceLineage(
                    i.self,self,package,i.first_lineage,data->ordinal,data->category));
                stage->namespace_channel=std::shared_ptr<const BnwNativeCategoryNamespaceChannel>(new BnwNativeCategoryNamespaceChannel(
                    self,i.cold,i.installation_input,i.first_lineage,stage->source,i.identity));
                owned_category_stages_.push_back(stage);data->pending=stage;operation.stage=stage.get();
                active_owned_category_stage_=stage.get();initializing_owned_namespace_=stage->namespace_channel.get();
                stage->arena=BnwNativeArchiveEntryAdmissionArena::acquire_category(stage->namespace_channel);
                stage->cache=BnwNativeArchiveBasenameCacheOwner::acquire_category(stage->arena,stage->namespace_channel);
                initializing_owned_namespace_=nullptr;operation.current();
                const auto occurrence=static_cast<std::uint64_t>(owned_category_stages_.size());
                stage->manager=i.resource_catalog->retain_resource_manager_from_same_owned_package(package,*stage->document,occurrence,stage->category);
                stage->selected_source=std::shared_ptr<OwnedSelectedCategorySource>(new OwnedSelectedCategorySource(*held.actor,stage->category,self,stage));
                held.actor->state_->category_slots[stage->category]=std::weak_ptr<const BnwNativeSelectedCategoryManagerSource>(stage->selected_source);
                stage->step=OwnedCategoryStage::Step::constructor_attached;
                require_installed_initial_category_stage_current(*stage);operation.current();
                //822A50 namespace/arena/cache/manager returned;6F84F0 archive
                //operations and remaining5F6920 effects have not returned.
            });operation.current();
    return zero; // Actual flags40 false-return branch; a constructor is still pending effects.
}
void detail::BnwNativeGlobalResourceWarmOwner::prepare_installed_initial_category_prefix() {
    need(thread_==std::this_thread::get_id(),"Installed initial category entry foreign creator");
    if(installed_initial_category_prefix_attempted_){failed_.store(true);throw std::runtime_error("Installed initial category prefix replay/reentry");}
    installed_initial_category_prefix_attempted_=true; // Negative occurrence before callback/pin/allocation.
    try {
        ParentPin retained(*this);const auto storage=retained.parent->impl_;auto& i=*storage;
        require_installed_startup_admission_returned();
        need(!i.header_active&&!installed_initial_category_driver_&&owned_category_stages_.empty()&&
             !configured_activation_policy_returned_&&!active_owned_category_stage_&&!active_owned_category_preconstructor_&&
             !active_owned_skin_operation_epoch_,"Installed initial category requires genuine admission before any constructor");
        const auto data=std::make_shared<InstalledInitialCategoryDriver>(i,*this);installed_initial_category_driver_=data;
        struct Failure {BnwNativePackageDiscoveryOwner::Impl& i;std::shared_ptr<InstalledInitialCategoryDriver> data;bool done=false;
            ~Failure(){if(!done){i.header_poisoned=true;data->failed=true;}}} failure{i,data};
        InstalledInitialCategoryDriver::Operation operation(*data,*this,*retained.parent,i);
        operation.current();BnwPrimaryDefaultStageTrace trace("Installed.InitialCategory.constructor-prefix");
        while(data->ordinal<returned_standalone_packages_.size()&&data->pending.expired()) {
            const bool zero=construct_installed_initial_category_current();
            if(zero){if(data->category==0)data->category=1;else{data->category=0;++data->ordinal;}}
            else need(!data->pending.expired(),"Installed constructor did not retain its actual pending namespace");
        }
        i.frame.next_resource_dependency=data->pending.expired()?
            "Actual656440 zero-branch cursor exhausted;52D590/Skin source still mandatory":
            "Mandatory SAME pending6F84F0 Paks archive operations then5F6920 remaining effects before next per-package category";
        operation.current();data->prefix_returned=true;failure.done=true;
    }catch(...){failed_.store(true);throw;}
}
void BnwNativePackageDiscoveryOwner::advance_installation_initial_category_prefix()const {
    const auto storage=impl_;auto& i=*storage;
    need(i.thread==std::this_thread::get_id(),"Installed category prefix Parent foreign creator");
    const auto warm=i.live_resource_query;
    need(i.installation_input&&!i.lease&&!i.rules&&warm&&i.installed_default_common,
         "Installed category prefix requires genuine installed Common/Warm source");
    try{warm->prepare_installed_initial_category_prefix();}
    catch(...){warm->failed_.store(true);i.header_poisoned=true;throw;}
}

void detail::BnwNativeGlobalResourceWarmOwner::drive_installed_initial_categories() {
    need(thread_==std::this_thread::get_id(),"Installed initial category driver foreign creator");
    if(installed_initial_category_driver_attempted_){failed_.store(true);throw std::runtime_error("Installed initial category driver replay/reentry");}
    installed_initial_category_driver_attempted_=true;
    try {
        if(!installed_initial_category_prefix_attempted_)prepare_installed_initial_category_prefix();
        ParentPin retained(*this);const auto storage=retained.parent->impl_;auto& i=*storage;
        const auto data=installed_initial_category_driver_;
        need(data&&data->prefix_returned&&!data->failed&&!data->native_loop_returned&&!data->operation,
             "Installed initial category driver lacks actual returned prefix/cursor");
        struct Failure {BnwNativePackageDiscoveryOwner::Impl& i;std::shared_ptr<InstalledInitialCategoryDriver> data;bool done=false;
            ~Failure(){if(!done){data->failed=true;i.header_poisoned=true;}}} failure{i,data};
        InstalledInitialCategoryDriver::Operation operation(*data,*this,*retained.parent,i);
        operation.current();BnwPrimaryDefaultStageTrace trace("Installed.InitialCategory.full656440");
        while(data->ordinal<returned_standalone_packages_.size()) {
            operation.current();
            if(data->pending.expired()) {
                const bool zero=construct_installed_initial_category_current();operation.current();
                if(zero){if(data->category==0)data->category=1;else{data->category=0;++data->ordinal;}continue;}
            }
            const auto stage=data->pending.lock();
            need(stage&&stage->installed_initial_driver&&stage->factory_ordinal==data->ordinal&&stage->category==data->category&&
                 stage->step==OwnedCategoryStage::Step::constructor_attached,
                 "Installed initial driver lost SAME actual pending6F84F0 constructor");
            const auto package=returned_standalone_packages_[data->ordinal];
            package->with_actual_descriptor([&](const auto& held){
                need(!operation.loan,"Installed initial effects nested outer Loaded loan");operation.loan=&held;
                struct LoanEnd {InstalledInitialCategoryDriver::Operation& op;~LoanEnd(){op.loan=nullptr;}} loan_end{operation};
                operation.current();
                BnwNativePackageDiscoveryOwner::Impl::HeaderGuard fence(i.header_callback_state,i.self);
                need(fence.valid(),"Installed initial effects Parent retired/reentered");
                struct Active {
                    BnwNativePackageDiscoveryOwner::Impl& i;BnwNativeGlobalResourceWarmOwner& warm;
                    InstalledInitialCategoryDriver::Operation& op;
                    Active(BnwNativePackageDiscoveryOwner::Impl& value,BnwNativeGlobalResourceWarmOwner& w,
                        InstalledInitialCategoryDriver::Operation& actual,const BnwNativePackageDiscoveryOwner::Impl::HeaderGuard& header,
                        const OwnedCategoryStage& stage):i(value),warm(w),op(actual){
                        need(!i.header_active&&!i.active_live_query_owner&&!warm.active_owned_category_stage_,"Installed initial effects Header reentry");
                        op.header=&header;op.stage=&stage;i.header_active=true;i.active_header_fence=&header;
                        i.active_live_query_owner=&warm;warm.active_owned_category_stage_=&stage;
                    }
                    ~Active(){op.stage=nullptr;op.header=nullptr;warm.active_owned_category_stage_=nullptr;
                        warm.initializing_owned_namespace_=nullptr;i.active_live_query_owner=nullptr;i.active_header_fence=nullptr;i.header_active=false;}
                } active{i,*this,operation,fence,*stage};
                operation.current();require_installed_initial_category_stage_current(*stage);
                apply_owned_native_paks_list(*stage,stage->paks);
                require_installed_initial_category_stage_current(*stage);stage->step=OwnedCategoryStage::Step::paks_effects;
                finish_installed_initial_category_effects(stage);
                require_installed_initial_category_stage_current(*stage);
                stage->step=OwnedCategoryStage::Step::name_registered;operation.current();
            });operation.current(); // Actual target destruction and whole Loaded POST precede native return journal.
            data->returned_calls.push_back({data->ordinal,data->category,stage});
            data->pending.reset();
            if(data->category==0)data->category=1;else{data->category=0;++data->ordinal;}
        }
        need(data->pending.expired()&&data->zero_returns.size()+data->returned_calls.size()==2*returned_standalone_packages_.size(),
             "Installed656440 real false/effect return roster incomplete");
        need(std::all_of(owned_category_stages_.begin(),owned_category_stages_.end(),[](const auto& stage){
            return stage&&stage->installed_initial_driver&&stage->step==OwnedCategoryStage::Step::name_registered;}),
             "Installed656440 actual category effects still pending");
        i.frame.next_resource_dependency="Actual initial656440 per-package0/1 effects returned; mandatory52D590 Color/Skin and subsequent completed startup suppliers";
        operation.current();data->native_loop_returned=true;failure.done=true;
    }catch(...){failed_.store(true);throw;}
}
void BnwNativePackageDiscoveryOwner::drive_installation_initial_categories()const {
    const auto storage=impl_;auto& i=*storage;
    need(i.thread==std::this_thread::get_id(),"Installed656440 Parent foreign creator");
    const auto warm=i.live_resource_query;
    need(i.installation_input&&!i.lease&&!i.rules&&warm&&i.installed_default_common,
         "Installed656440 requires actual installed Common/Warm source");
    try{warm->drive_installed_initial_categories();warm->require_installed_initial_categories_returned();}
    catch(...){warm->failed_.store(true);i.header_poisoned=true;throw;}
}

void detail::BnwNativeGlobalResourceWarmOwner::require_installed_initial_categories_returned()const {
    need(thread_==std::this_thread::get_id(),"Installed656440 return borrow foreign creator");
    ParentPin retained(*this);const auto storage=retained.parent->impl_;const auto issuer=storage->live_resource_query;
    const auto data=installed_initial_category_driver_;
    try {
        need(data&&data->native_loop_returned&&!data->failed&&!data->operation&&
             data->ordinal==returned_standalone_packages_.size()&&data->category==0&&data->pending.expired()&&
             data->returned_calls.size()==owned_category_stages_.size()&&
             data->zero_returns.size()+data->returned_calls.size()==2*returned_standalone_packages_.size(),
             "Installed656440 has no actual complete per-package native return roster");
        require_installed_startup_admission_returned();data->admission->identity(*this,*retained.parent,*storage);
        const auto completed_identity=[&]{for(const auto& call:data->returned_calls){
            const auto stage=call.stage.lock();
            need(call.ordinal<returned_standalone_packages_.size()&&call.category<2&&stage&&
                 stage->installed_initial_driver&&stage->installed_initial_driver->lock().get()==data.get()&&
                 stage->factory_ordinal==call.ordinal&&stage->category==call.category&&
                 stage->step==OwnedCategoryStage::Step::name_registered&&stage->arena&&stage->cache&&stage->selected_source&&
                 stage->loose_source&&stage->source&&stage->namespace_channel&&
                 std::any_of(owned_category_stages_.begin(),owned_category_stages_.end(),[&](const auto& value){return value==stage;}),
                 "Installed656440 lost actual retained completed category source");
            const auto package=returned_standalone_packages_[call.ordinal];const auto actor=package->actor_.lock();
            const auto manager=stage->manager.lock();const auto& channel=*stage->namespace_channel;
            need(stage->package.lock().get()==package.get()&&
                 !stage->package.owner_before(package)&&!package.owner_before(stage->package)&&
                 stage->document==data->admission->rows[call.ordinal].document&&actor&&manager&&manager->current89&&
                 actor->state_->category_slots[call.category]&&
                 !actor->state_->category_slots[call.category]->owner_before(stage->selected_source)&&
                 !stage->selected_source.owner_before(*actor->state_->category_slots[call.category])&&
                 stage->source->owned_warm_&&channel.owned_warm_&&
                 !stage->source->owned_warm_->owner_before(issuer)&&!issuer.owner_before(*stage->source->owned_warm_)&&
                 !channel.owned_warm_->owner_before(issuer)&&!issuer.owner_before(*channel.owned_warm_)&&
                 stage->source->descriptor_==call.ordinal&&stage->source->category_==call.category&&
                 stage->source->first_.get()==storage->first_lineage.get()&&channel.namespace_.get()==stage->source.get()&&
                 channel.cold_.get()==storage->cold.get()&&!channel.lease_&&
                 channel.installation_input_.get()==storage->installation_input.get()&&
                 channel.first_.get()==storage->first_lineage.get()&&channel.epoch_.get()==storage->identity.get()&&
                 stage->loose_source->channel_.get()==&channel&&!stage->loose_source->lease_&&
                 stage->loose_source->installation_input_.get()==storage->installation_input.get(),
                 "Installed656440 retained namespace/input/manager/selected-slot source changed");
        }
        for(const auto& call:data->zero_returns){
            need(call.ordinal<returned_standalone_packages_.size()&&call.category<2&&
                 call.package.lock().get()==returned_standalone_packages_[call.ordinal].get()&&
                 (call.actual_flags&0x40U)&&call.actual_flags==data->admission->rows[call.ordinal].expected_flags,
                 "Installed656440 false return is not actual flags40 package/call");
        }};
        completed_identity();require_current();data->admission->identity(*this,*retained.parent,*storage);completed_identity();
    }catch(...){if(data)data->failed=true;failed_.store(true);storage->header_poisoned=true;throw;}
}

void detail::BnwNativeGlobalResourceWarmOwner::finish_installed_initial_category_effects(const std::shared_ptr<OwnedCategoryStage>& stage) {
    const auto data=installed_initial_category_driver_;
    need(data&&data->operation&&data->operation->stage==stage.get(),"Installed final effects lack actual current Stage operation");
    auto& i=data->operation->i;
        resume_owned_directory_file_effects(*stage);
        const auto package=stage->package.lock();need(bool(package),"Owned native final effects package retired");
        std::vector<std::string> skin_paks;
        const auto count=BnwNativePackageResourceCheckpoint::count_from_loaded_762400(*package,2);
        for(std::uint32_t occurrence=0;occurrence<count;++occurrence)
            BnwNativePackageResourceCheckpoint::with_outputs_from_loaded(*package,2,occurrence,
                BnwNativePackageResourceCheckpoint::Query::skin_paks,[&](int,const auto& actual){
                    skin_paks.insert(skin_paks.end(),actual.begin(),actual.end());require_owned_category_stage_current(*stage);});
        //Only the freshly cleared-and-filled Skin/Paks vector is passed to
        //67B4E0. Neither earlier Paks nor File outputs are this operand.
        for(const auto& declared:skin_paks){
            require_owned_category_stage_current(*stage);
            const auto key=archive_cache_basename_ascii(std::string(declared.c_str()));
            const auto selected=stage->namespace_channel->cache_storage().lookup(key);
            if(!selected)continue; //Actual inspected82AE20 namespace cache MISS.
            const auto publication=*selected;need(bool(publication),"Native67B4E0 cached archive is null");
            publication->disable_preference280_from_category(*stage->namespace_channel);
            auto stem=std::string(declared.c_str());const auto slash=stem.find_last_of('\\');
            const auto colon=slash==std::string::npos?stem.find(':'):std::string::npos;
            const auto split=slash==std::string::npos?colon:slash;if(split!=std::string::npos)stem.erase(0,split+1);
            const auto dot=stem.find_last_of('.');if(dot!=std::string::npos)stem.resize(dot);
            const auto low=archive_cache_basename_ascii("Resource\\DX9_LOW\\"+stem+"DX9Low.fpk");
            if(const auto actual_low=stage->namespace_channel->cache_storage().lookup(low)){
                need(bool(*actual_low),"Native67B4E0 cached LOW archive is null");
                (*actual_low)->disable_preference280_from_category(*stage->namespace_channel);
            }
            require_owned_category_stage_current(*stage);
        }
        const auto manager=stage->manager.lock();need(bool(manager),"Owned native Name registration manager retired");
        package->with_actual_descriptor([&](const auto& loan){
            loan.require_current();static constexpr const char* labels[]={"Common","Gameplay"};
            need(stage->category<2,"Ordinary namespace final writer cannot dispatch UISkin");
            const auto actual_name=std::string(loan.metadata().name.c_str())+"."+labels[stage->category];
            i.resource_catalog->set_name84(manager,actual_name.c_str());require_owned_category_stage_current(*stage);
            i.resource_catalog->set_group7c(manager,0);require_owned_category_stage_current(*stage);
            i.resource_catalog->set_priority80(manager,0);require_owned_category_stage_current(*stage);
            i.resource_catalog->register_document(manager);require_owned_category_stage_current(*stage);loan.require_current();
        });
}
