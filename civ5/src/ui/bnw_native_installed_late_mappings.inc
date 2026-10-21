// Actual33 native source literal sites; these are arguments to real append calls.
const char16_t* detail::BnwNativeFilesystemMappingOwner::LateMappingsOperation::literal_for_actual_site(std::size_t site){
    switch(site){
    case 0:return u"Logs"; // Original call 004B4EDC
    case 1:return u"Saves"; // Original call 004B4EEF
    case 2:return u"Saves\\hotseat"; // Original call 004B4F02
    case 3:return u"Saves\\hotseat\\auto"; // Original call 004B4F15
    case 4:return u"Saves\\multi"; // Original call 004B4F28
    case 5:return u"Saves\\multi\\auto"; // Original call 004B4F3B
    case 6:return u"Saves\\single"; // Original call 004B4F4E
    case 7:return u"Saves\\single\\auto"; // Original call 004B4F61
    case 8:return u"Saves\\single\\quick"; // Original call 004B4F74
    case 9:return u"Saves\\pbem"; // Original call 004B4F87
    case 10:return u"Saves\\pbem\\auto"; // Original call 004B4F9A
    case 11:return u"Saves\\pitboss"; // Original call 004B4FAD
    case 12:return u"Saves\\pitboss\\auto"; // Original call 004B4FC0
    case 13:return u"ModdedSaves"; // Original call 004B4FD3
    case 14:return u"ModdedSaves\\single"; // Original call 004B4FE6
    case 15:return u"ModdedSaves\\single\\auto"; // Original call 004B4FF9
    case 16:return u"ModdedSaves\\single\\auto\\prev"; // Original call 004B500C
    case 17:return u"ModdedSaves\\single\\quick"; // Original call 004B501F
    case 18:return u"ModUserData"; // Original call 004B5032
    case 19:return u"cache"; // Original call 004B5045
    case 20:return u"cache\\downloads"; // Original call 004B5058
    case 21:return u"cache\\images"; // Original call 004B506B
    case 22:return u"Maps"; // Original call 004B507E
    case 23:return u"MODS"; // Original call 004B5091
    case 24:return u"Replays"; // Original call 004B50A4
    case 25:return u"ScreenShots"; // Original call 004B50B7
    case 26:return u"TransferredMaps"; // Original call 004B50CA
    case 27:return u"Saves\\hotseat\\auto\\prev"; // Original call 004B50DD
    case 28:return u"Saves\\multi\\auto\\prev"; // Original call 004B50F0
    case 29:return u"Saves\\single\\auto\\prev"; // Original call 004B5103
    case 30:return u"Saves\\pbem\\auto\\prev"; // Original call 004B5116
    case 31:return u"Saves\\pitboss\\auto\\prev"; // Original call 004B5129
    case 32:return u"Text"; // Original call 004B513C
    default:throw std::runtime_error("Late mapping source ordinal is outside actual33 call sites");
    }
}

detail::BnwNativeFilesystemMappingOwner::LateMappingsOperation::LateMappingsOperation(
    BnwNativeFilesystemMappingOwner& owner):owner_(owner){
    owner_.creator();owner_.require_mapping_bootstrap_returned_current();
    need(!owner_.late_writer_&&!owner_.late_mappings_,"Late mappings actual FS writer replay");
    // Actual pre-extent and prior values come from the SAME retained vectors.
    // Nothing initializes/replaces/clears the real vector at this late phase.
    auto history=std::make_unique<LateMappingsHistory>(owner_);owner_.source_current();
    need(history->initial_extent==owner_.general_.size(),"Late mappings pre-extent changed during copy");
    owner_.late_mappings_=std::move(history);owner_.late_writer_=this;identity();
}
detail::BnwNativeFilesystemMappingOwner::LateMappingsOperation::~LateMappingsOperation()noexcept {
    if(owner_.late_writer_!=this||!body_returned_){owner_.fail();if(owner_.late_mappings_)owner_.late_mappings_->failed=true;}
    if(owner_.late_writer_==this)owner_.late_writer_=nullptr;
    if(owner_.late_mappings_&&!owner_.failed_&&body_returned_)owner_.late_mappings_->operation_retired=true;
    // Retained actual owner/history only. No Parent/provider/borrower callback.
}
void detail::BnwNativeFilesystemMappingOwner::LateMappingsOperation::identity()const {
    owner_.creator();need(creator_==std::this_thread::get_id()&&!owner_.failed_&&owner_.late_writer_==this&&
         owner_.late_mappings_&&!owner_.late_mappings_->failed&&!owner_.late_mappings_->returned&&
         !owner_.active_&&!owner_.active_bootstrap_&&owner_.mapping_bootstrap_returned_&&
         owner_.bound_&&owner_.cold_bound_&&!owner_.parent_.expired()&&!owner_.input_.expired()&&!owner_.cold_.expired()&&
         owner_.late_mappings_->actual_roots.get()==owner_.roots_.get()&&same(owner_.late_mappings_->actual_roots,owner_.roots_)&&
         next_site_<=33&&owner_.late_mappings_->append_returns==next_site_&&
         owner_.general_.size()==owner_.late_mappings_->initial_extent+next_site_,
         "Late mappings lost SAME real operation/Roots/vector extent");
}
void detail::BnwNativeFilesystemMappingOwner::LateMappingsOperation::current()const {identity();owner_.source_current();identity();}
void detail::BnwNativeFilesystemMappingOwner::LateMappingsOperation::append_actual_next_site(){
    current();need(!body_returned_&&next_site_<33,"Late mapping source site replay/overrun");
    const auto old=owner_.general_.size();
    {
        Descriptor local;local.root=1;local.name=normalized(literal_for_actual_site(next_site_));
        need(!local.name.empty()&&local.shortcut.empty()&&local.cached.empty(),"Late mapping false/null constructor changed");
        current();owner_.general_.push_back(local);
        // Actual append has returned; the old element addresses were not kept.
        need(owner_.general_.size()==old+1,"Late mapping append did not return real growth");
        owner_.source_current();
    } // Real copied descriptor/local string destruction precedes observation.
    owner_.source_current();
    const auto& appended=owner_.general_.back();
    need(appended.root==1&&appended.name==normalized(literal_for_actual_site(next_site_))&&
         appended.shortcut.empty()&&appended.cached.empty(),"Late mapping actual appended payload changed");
    ++next_site_;++owner_.late_mappings_->append_returns;current();
}
void detail::BnwNativeFilesystemMappingOwner::LateMappingsOperation::finish_actual_body(){
    current();need(next_site_==33&&!body_returned_,"Late mappings actual source calls incomplete");
    owner_.late_mappings_identity_without_callbacks();current();
    owner_.late_mappings_->body_returned=true;body_returned_=true;
}
void detail::BnwNativeFilesystemMappingOwner::late_mappings_identity_without_callbacks()const {
    creator();need(!failed_&&late_mappings_&&!late_mappings_->failed&&mapping_bootstrap_returned_&&
         !active_&&!active_bootstrap_&&bound_&&cold_bound_&&!parent_.expired()&&!input_.expired()&&!cold_.expired(),
         "Late mapping history has no genuine same early FS origin");
    const auto& history=*late_mappings_;
    need(history.actual_roots.get()==roots_.get()&&same(history.actual_roots,roots_)&&
         history.earlier_raw_roots==roots_->raw&&history.earlier_physical_roots==roots_->physical&&
         history.append_returns==33&&general_.size()==history.initial_extent+history.append_returns&&
         history.earlier_general.size()==history.initial_extent&&config_.size()==history.earlier_config.size(),
         "Late mapping actual source appends changed earlier extent/Roots/Config");
    const auto equal=[](const Descriptor& a,const Descriptor& b){return a.root==b.root&&a.name==b.name&&
        a.shortcut==b.shortcut&&a.cached==b.cached;};
    for(std::size_t n=0;n<history.initial_extent;++n)need(equal(general_[n],history.earlier_general[n]),
         "Late mapping changed a real earlier descriptor/cache");
    for(std::size_t n=0;n<config_.size();++n)need(equal(config_[n],history.earlier_config[n]),
         "Late mapping changed actual Config record");
    for(std::size_t n=0;n<history.append_returns;++n){const auto& row=general_[history.initial_extent+n];
        need(row.root==1&&row.name==LateMappingsOperation::literal_for_actual_site(n)&&
             row.shortcut.empty()&&row.cached.empty(),"Late mapping source order/duplicates/false-null fields changed");
    }
}
void detail::BnwNativeFilesystemMappingOwner::require_late_mappings_returned_current()const {
    late_mappings_identity_without_callbacks();need(!late_writer_&&late_mappings_->body_returned&&
         late_mappings_->operation_retired&&late_mappings_->header_retired&&late_mappings_->returned,
         "Late mappings quoted before actual memory/Header/local scope return");
    source_current();late_mappings_identity_without_callbacks();
}

// Actual E3b/54CE60 close only; no Bank/window/World32 completion is granted.
void BnwNativePackageDiscoveryOwner::append_installed_late_mappings()const {
    using Warm=detail::BnwNativeGlobalResourceWarmOwner;using Data=Warm::InstalledLateSpeechTail;
    using FS=detail::BnwNativeFilesystemMappingOwner;
    const auto storage=impl_;auto& i=*storage;need(i.thread==std::this_thread::get_id(),"Late mapping Parent foreign creator");
    const auto warm=i.live_resource_query;const auto fs=i.filesystem_mappings;
    need(warm&&fs&&warm->installed_late_speech_tail_,"Late mappings require SAME real FS/Speech source");
    const auto data=warm->installed_late_speech_tail_;
    if(data->mappings_attempted){data->failed=true;data->phase=Data::Phase::failed;
        warm->failed_.store(true);i.header_poisoned=true;fs->fail();
        throw std::runtime_error("Late mapping actual writer replay/reentry");}
    data->mappings_attempted=true;
    struct Failure {const std::shared_ptr<Impl> storage;const std::shared_ptr<Warm> warm;
        const std::shared_ptr<Data> data;const std::shared_ptr<FS> fs;bool done=false;
        ~Failure(){if(!done){data->failed=true;data->phase=Data::Phase::failed;fs->fail();
            warm->failed_.store(true);storage->header_poisoned=true;}}} failure{storage,warm,data,fs};
    try {
        data->outside_speech_identity(*warm,*this,i);
        need(data->phase==Data::Phase::speech_returned&&!data->active_mapping_frame,
             "Late mappings started before genuine E3a speech closure");
        require_installed_late_default_identity_without_callbacks();
        need(i.installed_late_default&&i.installed_late_default->phase==Impl::InstalledLateDefaultSuccessor::Phase::returned,
             "Late mappings lack real SAME Default successor return");
        fs->require_mapping_bootstrap_returned_current();data->outside_speech_identity(*warm,*this,i);
        data->phase=Data::Phase::mappings;
        {
            Impl::HeaderGuard fence(i.header_callback_state,i.self);
            need(fence.valid(),"Late mapping NEW memory Header retired/reentered");
            struct MemoryFrame {
                const BnwNativePackageDiscoveryOwner& parent;Impl& i;Warm& warm;Data& data;FS& fs;
                const Impl::HeaderGuard& fence;
                MemoryFrame(const BnwNativePackageDiscoveryOwner& actual,Impl& storage,Warm& source,Data& history,
                    FS& filesystem,const Impl::HeaderGuard& guard):parent(actual),i(storage),warm(source),data(history),fs(filesystem),fence(guard){
                    need(!data.active_mapping_frame&&!i.header_active&&!i.active_header_fence&&
                         !i.active_live_query_owner&&!warm.active_owned_skin_operation_epoch_&&
                         !warm.active_owned_skin_resource_loan_,"Late memory Header has an unrelated physical owner");
                    data.active_mapping_frame=this;i.header_active=true;i.active_header_fence=&fence;i.active_live_query_owner=&warm;
                }
                ~MemoryFrame(){
                    if(data.active_mapping_frame!=this||i.active_live_query_owner!=&warm){data.failed=true;i.header_poisoned=true;}
                    data.active_mapping_frame=nullptr;i.active_live_query_owner=nullptr;
                    i.active_header_fence=nullptr;i.header_active=false;
                }
                void identity()const {
                    data.base_identity_without_callbacks(warm,parent,i);
                    need(data.phase==Data::Phase::mappings&&data.mappings_attempted&&!data.mappings_returned&&
                         data.active_mapping_frame==this&&!data.operation&&!data.active_core&&
                         !data.common->continuation&&!data.selection->continuation&&
                         i.header_active&&i.active_header_fence==&fence&&fence.valid()&&i.active_live_query_owner==&warm&&
                         !warm.active_owned_skin_operation_epoch_&&!warm.active_owned_skin_resource_loan_&&
                         !warm.active_owned_skin_read_operation_&&!warm.active_owned_category_stage_&&
                         !i.global_color_bank->busy_.load()&&!i.global_color_bank->active_owned_skin_operation_&&
                         fs.parent_identity_==&parent&&FS::same(fs.parent_,i.self)&&FS::same(fs.input_,i.installation_input)&&
                         FS::same(fs.cold_,i.cold),"Late mapping memory writer lost its NEW real Header/source scope");
                    parent.require_installed_late_default_identity_without_callbacks();
                }
                void current()const {
                    identity();i.installation_input->require_current();identity();
                    need(i.settings->normal_default_loose_mode(parent)==1,"Late mapping changed actual normal Boot donor");identity();
                }
            } frame(*this,i,*warm,*data,*fs,fence);
            frame.current();
            {
                FS::LateMappingsOperation memory(*fs);frame.current();
                // Each call constructs/copies/appends/destroys a real descriptor
                // on the SAME existing list. Count is not the execution producer.
                for(std::size_t site=0;site<33;++site){frame.current();
                    need(memory.next_site_==site,"Late mapping actual source cursor changed");
                    memory.append_actual_next_site();frame.current();
                }
                memory.finish_actual_body();frame.current();
            } // Real memory operation retires before its owning Header.
            frame.current();need(!fs->late_writer_&&fs->late_mappings_&&fs->late_mappings_->operation_retired&&
                 fs->late_mappings_->body_returned,"Late mapping memory body/operation failed to return");
        } // MemoryFrame and genuine Header really retire.
        need(!i.header_active&&!i.active_header_fence&&!i.active_live_query_owner&&!data->active_mapping_frame&&
             fs->late_mappings_&&fs->late_mappings_->operation_retired&&!fs->late_writer_,
             "Late mapping outside quote precedes actual Header return");
        fs->late_mappings_->header_retired=true;
        data->base_identity_without_callbacks(*warm,*this,i);
        i.installation_input->require_current();data->base_identity_without_callbacks(*warm,*this,i);
        need(i.settings->normal_default_loose_mode(*this)==1,"Late mapping outside Boot source changed");
        fs->late_mappings_identity_without_callbacks();fs->source_current();
        data->base_identity_without_callbacks(*warm,*this,i);
        fs->late_mappings_->returned=true;data->mappings_returned=true;data->phase=Data::Phase::returned;
        failure.done=true;
    }catch(...){fs->fail();warm->failed_.store(true);i.header_poisoned=true;throw;}
}

void detail::BnwNativeGlobalResourceWarmOwner::require_installed_late_speech_and_mappings_returned()const {
    using Parent=BnwNativePackageDiscoveryOwner;using Data=InstalledLateSpeechTail;
    need(thread_==std::this_thread::get_id(),"Late tail outside lender foreign creator");
    ParentPin retained(*this);const auto storage=retained.parent->impl_;const auto data=installed_late_speech_tail_;
    if(!data||data->phase!=Data::Phase::returned||!data->mappings_returned){
        if(data){data->failed=true;data->phase=Data::Phase::failed;}
        failed_.store(true);storage->header_poisoned=true;
        throw std::runtime_error("Late tail has no genuine outer normal return");}
    if(data->validating_return){data->failed=true;data->phase=Data::Phase::failed;
        failed_.store(true);storage->header_poisoned=true;throw std::runtime_error("Late tail outside lender reentry");}
    data->validating_return=true;struct Exit {Data& data;~Exit(){data.validating_return=false;}} exit{*data};
    try {
        data->outside_speech_identity(*this,*retained.parent,*storage);
        need(!data->active_mapping_frame,"Late tail retained its departed memory context");
        retained.parent->require_installed_late_default_identity_without_callbacks();
        retained.parent->require_installed_late_mappings_returned_source();require_current();
        data->outside_speech_identity(*this,*retained.parent,*storage);
        retained.parent->require_installed_late_mappings_returned_source();
    }catch(...){data->failed=true;data->phase=Data::Phase::failed;failed_.store(true);storage->header_poisoned=true;throw;}
}

void BnwNativePackageDiscoveryOwner::require_installed_late_mappings_returned_source()const {
    const auto storage=impl_;auto& i=*storage;need(i.thread==std::this_thread::get_id(),"Late FS lender foreign creator");
    const auto warm=i.live_resource_query;const auto fs=i.filesystem_mappings;
    need(warm&&fs&&warm->installed_late_speech_tail_,"Late FS lender lost actual sources");
    const auto data=warm->installed_late_speech_tail_;
    try {
        data->outside_speech_identity(*warm,*this,i);
        need(data->phase==detail::BnwNativeGlobalResourceWarmOwner::InstalledLateSpeechTail::Phase::returned&&
             data->mappings_returned&&!data->active_mapping_frame,
             "Late FS lender quoted before actual mapping/Header close");
        fs->require_late_mappings_returned_current();data->outside_speech_identity(*warm,*this,i);
    }catch(...){data->failed=true;data->phase=detail::BnwNativeGlobalResourceWarmOwner::InstalledLateSpeechTail::Phase::failed;
        fs->fail();warm->failed_.store(true);i.header_poisoned=true;throw;}
}

void BnwNativePackageDiscoveryOwner::drive_installation_late_speech_and_mappings()const {
    const auto storage=impl_;need(storage->thread==std::this_thread::get_id(),"Late tail Parent foreign creator");
    const auto source=storage->live_resource_query;
    need(storage->installation_input&&!storage->lease&&!storage->rules&&source,"Late tail requires genuine installed Warm/Input");
    try{source->drive_installed_late_speech_and_mappings();source->require_installed_late_speech_and_mappings_returned();
        storage->frame.next_resource_dependency="Actual late Speech Default directory effects, SAME retained FS33 appends and specific54CE60 return1 closed; terminal/resource/window/World32 gates remain required";}
    catch(...){source->failed_.store(true);storage->header_poisoned=true;throw;}
}
