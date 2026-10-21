// Actual late Speech physical driver; same-FS memory close is in the next include.
void BnwNativePackageDiscoveryOwner::require_installed_mapping_bootstrap_before_late_tail()const {
    const auto storage=impl_;auto& i=*storage;
    need(i.thread==std::this_thread::get_id(),"Late mapping predecessor foreign creator");
    const auto warm=i.live_resource_query;const auto fs=i.filesystem_mappings;
    need(warm&&fs&&warm->installed_global_skin_effects_,"Late tail has no genuine early FS/global predecessor");
    // These are pure predecessor observations BEFORE begin/mutation, not a
    // second strict8k lender or an old callback after namespace changes.
    warm->installed_global_skin_effects_->outside_identity(*warm,*this,i);
    need(fs->parent_identity_==this&&detail::BnwNativeFilesystemMappingOwner::same(fs->parent_,i.self)&&
         detail::BnwNativeFilesystemMappingOwner::same(fs->input_,i.installation_input)&&
         detail::BnwNativeFilesystemMappingOwner::same(fs->cold_,i.cold),
         "Late tail substituted actual early FS/Input/Parent/Cold controls");
    fs->require_mapping_bootstrap_returned_current();
    warm->installed_global_skin_effects_->outside_identity(*warm,*this,i);
}

void detail::BnwNativeGlobalResourceWarmOwner::InstalledLateSpeechTail::physical_post(const Parent& parent)const {
    need(operation&&operation->header&&operation->bank_operation&&(operation->loan||table_count==0)&&
         !active_core&&!operation->source.active_owned_skin_read_operation_&&
         !operation->source.active_owned_category_stage_&&!operation->source.initializing_owned_namespace_&&
         operation->i.default_state->live_enabled89==1,
         "Late Speech FULL POST lacks NEW owner or retains actual Core/old effect");
    operation->current();
    for(const auto& read:operation->source.owned_skin_read_receipts_)
        parent.require_native_owned_skin_read_receipt_current(read.get());
    for(const auto& stage:operation->source.owned_category_stages_){
        need(stage&&stage->step==OwnedCategoryStage::Step::name_registered,"Late Speech retained Stage never returned");
        operation->require_stage_current(*stage);
    }
    operation->current();
}

void detail::BnwNativeGlobalResourceWarmOwner::drive_installed_late_speech_and_mappings(){
    using Parent=BnwNativePackageDiscoveryOwner;using Data=InstalledLateSpeechTail;using Warm=BnwNativeGlobalResourceWarmOwner;
    need(thread_==std::this_thread::get_id(),"Late Speech driver foreign creator");
    if(installed_late_speech_tail_attempted_){failed_.store(true);throw std::runtime_error("Late Speech driver replay/reentry");}
    installed_late_speech_tail_attempted_=true; // Before callbacks/allocations.
    ParentPin retained(*this);const auto storage=retained.parent->impl_;auto& i=*storage;
    struct Failure {BnwNativeGlobalResourceWarmOwner& source;const std::shared_ptr<Parent::Impl> storage;bool done=false;
        ~Failure(){if(!done){source.failed_.store(true);storage->header_poisoned=true;
            if(source.installed_late_speech_tail_){source.installed_late_speech_tail_->failed=true;
                source.installed_late_speech_tail_->phase=Data::Phase::failed;}}}} failure{*this,storage};
    try {
        retained.parent->require_installed_mapping_bootstrap_before_late_tail();
        need(!installed_late_speech_tail_,"Late Speech data already exists");
        auto data=std::make_shared<Data>(i,*this);installed_late_speech_tail_=data;
        {
            Data::Operation operation(*data,*this,*retained.parent);
            operation.identity();
            // Loader begin owns the sole genuine old8k closed lender. New
            // resource continuations are still absent while it executes.
            retained.parent->begin_installed_late_default_speech(*this,*data);
            data->namespace_identity=i.installed_late_default.get();data->begin_returned=true;
            operation.attach_after_actual_begin();operation.current();
            {
                Parent::Impl::HeaderGuard fence(i.header_callback_state,i.self);
                need(fence.valid(),"Late Speech NEW Header retired/reentered");
                struct Active {
                    Data::Operation& op;
                    Active(Data::Operation& value,const Parent::Impl::HeaderGuard& fence):op(value){
                        need(!op.i.header_active&&!op.i.active_header_fence&&!op.i.active_live_query_owner&&
                             !op.source.active_owned_skin_operation_epoch_&&!op.header,
                             "Late Speech actual Header already has a resource owner");
                        const auto before=op.source.owned_skin_operation_epoch_;const auto next=before+1;
                        need(next>before&&next>op.data.global->returned_epoch,"Late Speech actual epoch overflow/reuse");
                        op.source.owned_skin_operation_epoch_=next;op.serial=next;op.data.speech_epoch=next;
                        op.header=&fence;op.source.active_owned_skin_operation_epoch_=next;
                        op.i.header_active=true;op.i.active_header_fence=&fence;op.i.active_live_query_owner=&op.source;
                    }
                    ~Active(){
                        if(op.loan||op.bank_operation||op.data.active_core){op.data.failed=true;op.i.header_poisoned=true;}
                        op.source.active_owned_skin_operation_epoch_=0;op.i.active_live_query_owner=nullptr;
                        op.i.active_header_fence=nullptr;op.i.header_active=false;op.header=nullptr;op.serial=0;
                    }
                } active{operation,fence};
                operation.current();
                const auto bank=data->bank.lock();need(bool(bank),"Late Speech actual Bank retired");
                const auto body=[&]{
                    {
                        BnwNativeGlobalColorRegistryBank::OwnedSkinOperation bank_operation(bank);
                        operation.bank_operation=&bank_operation;
                        struct EndBank {Data::Operation& op;~EndBank(){op.bank_operation=nullptr;}} end_bank{operation};
                        operation.current();data->physical_post(*retained.parent);
                        while(data->next_core<data->table_count){
                            const auto ordinal=data->next_core;
                            operation.current();const auto token=(*data->actual_table)[ordinal];
                            need(token&&Data::same(token,data->selection->color->admission->rows[ordinal].package),
                                 "Late Speech actual605080 cursor changed token/control");
                            token->with_actual_descriptor([&](const auto& held){
                                operation.current();operation.loan_identity(held,ordinal);
                                {
                                    Data::ActiveCore core(operation,held);core.execute_actual_branch();
                                } // Raw ActiveCore, path and brand members really retire before POST.
                                held.require_current();operation.current();
                            });
                            // The real whole per-Core loan and all callback
                            // targets/temporaries have now returned/died.
                            operation.current();need(!data->active_core&&data->core_returns.size()==ordinal+1&&
                                 !data->core_returns.back().physical_scope_returned,
                                 "Late Speech advanced before actual Core/target/destructor POST");
                            data->core_returns.back().physical_scope_returned=true;++data->next_core;operation.current();
                        }
                        data->physical_post(*retained.parent);data->physical_post_returned=true;
                    }
                    need(!bank->busy_.load()&&!bank->active_owned_skin_operation_,"Late Speech actual Bank scope did not return");
                    operation.current();
                };
                if(data->table_count){
                    data->actual_table->front()->with_actual_descriptor([&](const auto& held){
                        operation.loan=&held;active_owned_skin_package_=held.retained;
                        active_owned_skin_resource_loan_=&held.resource;
                        active_owned_skin_resource_epoch_=held.actor->state_->active_resource_loan;
                        struct EndLoan {Warm& source;Data::Operation& op;
                            ~EndLoan(){op.loan=nullptr;source.active_owned_skin_package_.reset();
                                source.active_owned_skin_resource_loan_=nullptr;source.active_owned_skin_resource_epoch_=0;}} end_loan{*this,operation};
                        operation.current();body();operation.current();
                    });
                    data->whole_loaded_close=Data::WholeLoadedClose::real_loan_returned;
                }else{
                    body(); // Genuine zero table; no synthetic first Loaded/Core.
                    data->whole_loaded_close=Data::WholeLoadedClose::empty_actual_table;
                }
                operation.current();need(fence.valid(),"Late Speech whole Loaded POST lost actual Header");
            }
            data->header_returned=true;operation.current();
            need(data->next_core==data->table_count&&data->physical_post_returned&&!data->active_core,
                 "Late Speech omitted actual Core calls/physical POST");
            data->phase=Data::Phase::physical_returned;
        } // Real new resource continuations and stack operation retire here.
        data->outside_speech_identity(*this,*retained.parent,i);
        retained.parent->finish_installed_late_default_speech(*data);
        data->phase=Data::Phase::speech_returned;
        data->outside_speech_identity(*this,*retained.parent,i);
        require_current();data->outside_speech_identity(*this,*retained.parent,i);
        // Separate genuine Header/memory-only FS operation, implemented in the
        // next include; never a reset-to-empty or copied33-row readiness grant.
        retained.parent->append_installed_late_mappings();
        need(data->mappings_returned&&data->phase==Data::Phase::returned,"Late mapping issuer did not really close");
        require_installed_late_speech_and_mappings_returned();failure.done=true;
    }catch(...){failed_.store(true);i.header_poisoned=true;throw;}
}
