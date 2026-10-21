// Actual installed52D590 only: reset/base Color, package48C800, then462C20.
// This source grants no later660330 or completed-resource capability.
struct detail::BnwNativeGlobalResourceWarmOwner::InstalledColorSkinDriver {
    using Warm=BnwNativeGlobalResourceWarmOwner;
    using Parent=BnwNativePackageDiscoveryOwner;
    using Loaded=BnwNativeStandalonePackageLoaded;
    using Stage=OwnedCategoryStage;
    const std::thread::id creator=std::this_thread::get_id();
    const std::weak_ptr<const Parent> parent;
    const std::weak_ptr<const void> warm,input,epoch,catalog;
    const std::shared_ptr<const InstalledInitialCategoryDriver> categories;
    const std::shared_ptr<const InstalledStartupAdmission> admission;
    std::weak_ptr<BnwNativeGlobalColorRegistryBank> bank;
    enum class Phase {base,constructors,reconcile,returned};
    Phase phase=Phase::base;
    bool failed=false,returned=false,base_stream_returned=false,base_load_returned=false,base_native_true=false,reconcile_returned=false;
    struct PackageReturn {std::size_t ordinal;std::uint32_t count;std::size_t first,last;std::weak_ptr<const Loaded> package;};
    std::vector<PackageReturn> packages;
    struct ConstructorReturn {
        std::weak_ptr<const BnwNativeOwnedSkinOccurrence> occurrence;
        std::weak_ptr<const BnwNativeOwnedUiSkinPoststageInstance> instance;
        const void* source=nullptr;
    };
    std::vector<ConstructorReturn> constructors;
    std::uint32_t nonzero_packages=0;
    std::size_t constructor_ordinal=0;
    const Loaded::Loan* constructor_loan=nullptr;
    const BnwNativeOwnedSkinOccurrence* constructing=nullptr;
    const void* active_base_read=nullptr;
    struct ReadAttempt {bool selected=false;DWORD error=0;};
    std::vector<ReadAttempt> read_attempts;
    struct Operation;
    Operation* operation=nullptr;
    InstalledColorSkinDriver(const Parent::Impl& i,const Warm& source)
        :parent(i.self),warm(i.live_resource_query),input(i.installation_input),epoch(i.identity),catalog(i.resource_catalog),
         categories(source.installed_initial_category_driver_),admission(source.installed_startup_admission_){}
    static bool same(const auto& a,const auto& b){return !a.owner_before(b)&&!b.owner_before(a);}
};
struct detail::BnwNativeGlobalResourceWarmOwner::InstalledColorSkinDriver::Operation {
    InstalledColorSkinDriver& data;
    Warm& source;
    const Parent& parent;
    Parent::Impl& i;
    const Parent::Impl::HeaderGuard* header=nullptr;
    const Loaded::Loan* loan=nullptr;
    std::uint64_t serial=0;
    Operation(InstalledColorSkinDriver& d,Warm& w,const Parent& p,Parent::Impl& actual)
        :data(d),source(w),parent(p),i(actual){
        need(data.creator==std::this_thread::get_id()&&!data.operation,"Installed Color/Skin operation replay/foreign creator");
        data.operation=this;
    }
    ~Operation(){if(data.operation==this)data.operation=nullptr;else{data.failed=true;i.header_poisoned=true;}}
    Operation(const Operation&)=delete;
    void loan_identity(const Loaded::Loan& held,std::size_t ordinal)const {
        need(ordinal<data.admission->rows.size()&&ordinal<source.returned_standalone_packages_.size(),
             "Installed Color/Skin actual loan ordinal escaped factory");
        const auto& row=data.admission->rows[ordinal];const auto& actual=source.returned_standalone_packages_[ordinal];
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
    void identity()const {
        need(data.creator==std::this_thread::get_id()&&i.thread==data.creator&&source.thread_==data.creator&&
             !data.failed&&!data.returned&&!source.closed_.load()&&!source.failed_.load()&&
             !i.header_poisoned&&!i.category_lineage_failed&&data.operation==this,
             "Installed Color/Skin creator/source/operation retired");
        need(source.installed_color_skin_driver_.get()==&data&&source.installed_color_skin_attempted_&&
             i.installation_input&&!i.lease&&!i.rules&&i.live_resource_query.get()==&source&&
             same(i.self,data.parent)&&same(i.live_resource_query,data.warm)&&same(i.installation_input,data.input)&&
             same(i.identity,data.epoch)&&same(i.resource_catalog,data.catalog)&&
             data.categories&&data.categories.get()==source.installed_initial_category_driver_.get()&&
             data.categories->native_loop_returned&&!data.categories->failed&&!data.categories->operation&&
             data.categories->pending.expired()&&data.admission&&data.admission->returned&&!data.admission->failed&&
             data.admission.get()==source.installed_startup_admission_.get()&&
             !source.configured_activation_policy_returned_&&source.owned_activation_receipts_.empty()&&
             !i.category_source&&!i.active_completed_resource_operation&&!source.active_owned_category_stage_&&
             !source.active_owned_category_preconstructor_&&!source.active_owned_skin_read_operation_,
             "Installed Color/Skin lost SAME installed native category/admission controls");
        data.admission->identity(source,parent,i); // Pure controls + canonical accounted outside Parent.
        if(header)need(serial&&source.active_owned_skin_operation_epoch_==serial&&i.header_active&&
             i.active_header_fence==header&&header->valid()&&i.active_live_query_owner==&source,
             "Installed Color/Skin lost SAME actual Header/operation epoch");
        else need(!i.header_active&&!i.active_live_query_owner&&!source.active_owned_skin_operation_epoch_,
                  "Installed Color/Skin borrowed another Header");
        if(loan){
            need(header&&source.active_owned_skin_resource_loan_==&loan->resource&&
                 source.active_owned_skin_package_.lock().get()==&loan->token&&
                 source.active_owned_skin_resource_epoch_==loan->actor->state_->active_resource_loan,
                 "Installed Color/Skin lost physical outer Loaded/ResourceLoan");
            loan_identity(*loan,0);
        }else need(!source.active_owned_skin_resource_loan_,"Installed Color/Skin orphaned outer resource loan");
        if(data.constructor_loan){
            need(header&&loan&&data.phase==Phase::constructors,"Installed constructor escaped actual52D590 phase");
            loan_identity(*data.constructor_loan,data.constructor_ordinal);
        }
        if(data.constructing)need(data.constructor_loan&&!source.owned_skin_occurrences_.empty()&&
             source.owned_skin_occurrences_.back().get()==data.constructing&&
             data.constructing->factory_ordinal_==data.constructor_ordinal&&
             data.constructing->package_.lock().get()==&data.constructor_loan->token&&
             data.constructing->document_==&data.constructor_loan->resource.actual_document,
             "Installed constructor substituted current actual UISkin occurrence");
    }
    void current()const {identity();source.require_current();if(loan)loan->require_current();
        if(data.constructor_loan)data.constructor_loan->require_current();identity();}
};

void detail::BnwNativeGlobalResourceWarmOwner::require_installed_color_skin_identity()const {
    need(thread_==std::this_thread::get_id(),"Installed Color/Skin foreign creator");
    if(installed_initial_skin_effects_){require_installed_initial_skin_effects_identity();return;}
    const auto data=installed_color_skin_driver_;
    need(data&&data->operation,"Installed Color/Skin has no actual stack operation");data->operation->identity();
}
void detail::BnwNativeGlobalResourceWarmOwner::require_installed_color_skin_operation_current()const {
    need(thread_==std::this_thread::get_id(),"Installed Skin effect foreign creator");if(installed_initial_skin_effects_){require_installed_initial_skin_effects_current();return;}
    require_installed_color_skin_identity();installed_color_skin_driver_->operation->current();
}
void detail::BnwNativeGlobalResourceWarmOwner::require_installed_color_skin_category_stage_identity(const OwnedCategoryStage& stage)const {
    need(thread_==std::this_thread::get_id(),"Installed Skin effect Stage foreign creator");if(installed_initial_skin_effects_){require_installed_initial_skin_effect_stage_identity(stage);return;}
    require_installed_color_skin_identity();const auto& data=*installed_color_skin_driver_;
    need(data.operation->header&&data.operation->loan&&stage.installed_initial_driver&&
         stage.installed_initial_driver->lock().get()==data.categories.get()&&stage.factory_ordinal<returned_standalone_packages_.size()&&
         stage.category<2&&stage.step==OwnedCategoryStage::Step::name_registered&&
         std::any_of(data.categories->returned_calls.begin(),data.categories->returned_calls.end(),[&](const auto& row){
             return row.ordinal==stage.factory_ordinal&&row.category==stage.category&&row.stage.lock().get()==&stage;})&&
         std::any_of(owned_category_stages_.begin(),owned_category_stages_.end(),[&](const auto& value){return value.get()==&stage;}),
         "Installed Color/Skin borrowed a category without actual initial normal return");
    const auto actor=returned_standalone_packages_[stage.factory_ordinal]->actor_.lock();
    const auto manager=stage.manager.lock();
    need(actor&&manager&&stage.source&&stage.namespace_channel&&stage.arena&&stage.cache&&stage.loose_source&&stage.selected_source&&
         stage.package.lock().get()==returned_standalone_packages_[stage.factory_ordinal].get()&&
         stage.document==data.admission->rows[stage.factory_ordinal].document&&
         actor->state_->category_slots[stage.category]&&
         !actor->state_->category_slots[stage.category]->owner_before(stage.selected_source)&&
         !stage.selected_source.owner_before(*actor->state_->category_slots[stage.category])&&
         stage.source->owned_warm_&&stage.namespace_channel->owned_warm_&&
         InstalledColorSkinDriver::same(*stage.source->owned_warm_,data.warm)&&
         InstalledColorSkinDriver::same(*stage.namespace_channel->owned_warm_,data.warm)&&
         stage.source->descriptor_==stage.factory_ordinal&&stage.source->category_==stage.category&&
         stage.source->first_.get()==data.operation->i.first_lineage.get()&&
         stage.namespace_channel->namespace_.get()==stage.source.get()&&
         stage.namespace_channel->installation_input_.get()==data.operation->i.installation_input.get()&&
         !stage.namespace_channel->lease_&&stage.namespace_channel->cold_.get()==data.operation->i.cold.get()&&
         stage.namespace_channel->first_.get()==data.operation->i.first_lineage.get()&&
         stage.namespace_channel->epoch_.get()==data.operation->i.identity.get(),
         "Installed Color/Skin retained category/manager/selected slot changed");
}
void detail::BnwNativeGlobalResourceWarmOwner::require_installed_color_skin_category_stage_current(const OwnedCategoryStage& stage)const {
    need(thread_==std::this_thread::get_id(),"Installed Skin effect Stage foreign creator");if(installed_initial_skin_effects_){require_installed_initial_skin_effect_stage_current(stage);return;}
    require_installed_color_skin_category_stage_identity(stage);const auto data=installed_color_skin_driver_;
    data->operation->current();const auto package=returned_standalone_packages_[stage.factory_ordinal];
    package->with_actual_descriptor([&](const auto& held){held.require_current();
        data->admission->authenticate(*this,*package,held,data->admission->rows[stage.factory_ordinal]);
        need(&held.resource.actual_document==stage.document,"Installed Color/Skin category Parser source changed");held.require_current();});
    require_installed_color_skin_category_stage_identity(stage);
}
void BnwNativePackageDiscoveryOwner::require_installed_color_skin_operation_current()const {
    need(impl_->thread==std::this_thread::get_id(),"Installed52D590 Parent foreign creator");
    const auto warm=impl_->live_resource_query;need(warm&&impl_->installation_input&&!impl_->lease,
        "Installed52D590 requires actual installed Input/Warm");warm->require_installed_color_skin_operation_current();
}
void BnwNativePackageDiscoveryOwner::require_installed_color_skin_bank_current(
    const detail::BnwNativeGlobalColorRegistryBank& bank,const content::XmlNode* document,std::uint64_t ordinal)const {
    need(impl_->thread==std::this_thread::get_id(),"Installed Color Bank foreign creator");
    const auto& i=*impl_;const auto warm=i.live_resource_query;
    if(warm&&warm->installed_initial_skin_effects_){require_installed_initial_skin_effect_bank_current(bank,document,ordinal);return;}
    need(warm&&warm->installed_color_skin_driver_,"Installed Color Bank actual source absent");
    warm->require_installed_color_skin_identity();const auto& data=*warm->installed_color_skin_driver_;
    need(data.operation->header&&data.operation->loan&&i.global_color_bank.get()==&bank&&data.bank.lock().get()==&bank&&
         warm->owned_bank_receiver_.lock().get()==&bank&&bank.thread_==i.thread&&!bank.closed_.load()&&
         bank.phase_!=detail::BnwNativeGlobalColorRegistryBank::Phase::failed&&
         bank.epoch_.get()==i.identity.get()&&detail::BnwNativeGlobalResourceWarmOwner::InstalledColorSkinDriver::same(bank.parent_,i.self)&&
         bank.resource_catalog_.get()==i.resource_catalog.get()&&bank.query_default_source_.lock().get()==i.current_default_source.get()&&
         bank.query_default_state_.lock().get()==i.default_state.get()&&bank.query_default_records_==&i.default_state->selected&&
         bank.live_resource_query_.lock().get()==warm.get()&&warm->owned_bank_source_identity_&&
         (bank.owned_source_?bank.owned_source_.get()==warm->owned_bank_source_identity_:
             !warm->owned_bank_source_attachment_returned_&&bank.phase_==detail::BnwNativeGlobalColorRegistryBank::Phase::cold),
         "Installed Color Bank lost actual receiver/Input/Default/catalog/Loaded/Header controls");
    need(!document&&ordinal==0,"Installed Color source cannot borrow an early content-DOM document");
}
std::shared_ptr<detail::BnwNativeGlobalColorRegistryBank>
BnwNativePackageDiscoveryOwner::construct_installed_color_skin_bank_receiver()const {
    require_installed_color_skin_operation_current();auto& i=*impl_;const auto warm=i.live_resource_query;
    const auto data=warm->installed_color_skin_driver_;
    need(data->phase==detail::BnwNativeGlobalResourceWarmOwner::InstalledColorSkinDriver::Phase::base&&
         data->operation->loan&&!i.global_color_bank&&warm->owned_bank_receiver_.expired()&&
         warm->owned_skin_instances_.empty()&&warm->owned_skin_occurrences_.empty(),
         "Installed4D96C0 reset is not the actual fresh base-before-constructor operation");
    auto bank=std::shared_ptr<detail::BnwNativeGlobalColorRegistryBank>(new detail::BnwNativeGlobalColorRegistryBank(i.self,i.identity,i.resource_catalog));
    bank->query_default_source_=i.current_default_source;bank->query_default_state_=i.default_state;
    bank->query_default_records_=&i.default_state->selected;bank->live_resource_query_=warm;
    warm->owned_bank_receiver_=bank;data->bank=bank;i.global_color_bank=bank;return bank;
}

void detail::BnwNativeGlobalResourceWarmOwner::with_installed_color_skin_base_input(
    const std::function<void(const std::shared_ptr<const BnwNativeOwnedSkinSelectedInput>&)>& input) {
    require_installed_color_skin_operation_current();const auto data=installed_color_skin_driver_;
    auto& op=*data->operation;auto& i=op.i;
    need(data->phase==InstalledColorSkinDriver::Phase::base&&!data->base_stream_returned&&!data->active_base_read&&
         op.header&&op.loan&&i.global_color_bank&&owned_bank_source_attachment_returned_,
         "Installed601CA0 stream replay/outside actual reset/base call");
    struct ReadLoan {
        InstalledColorSkinDriver& data;
        InstalledColorSkinDriver::Operation& op;
        HANDLE file=INVALID_HANDLE_VALUE;
        BY_HANDLE_FILE_INFORMATION identity{};
        std::filesystem::path path;
        std::function<void()> selected_identity,selected_full;
        ReadLoan(InstalledColorSkinDriver& d,InstalledColorSkinDriver::Operation& p):data(d),op(p){
            need(!data.active_base_read,"Installed Color stream reentry");data.active_base_read=this;}
        ~ReadLoan(){if(file!=INVALID_HANDLE_VALUE)CloseHandle(file);
            if(data.active_base_read==this)data.active_base_read=nullptr;else{data.failed=true;op.i.header_poisoned=true;}}
        void current()const {
            op.identity();need(data.active_base_read==this&&data.phase==InstalledColorSkinDriver::Phase::base,
                "Installed Color stream lost actual read loan");
            if(selected_identity)selected_identity();op.identity();
        }
        void full_current()const {
            op.current();current();if(selected_full)selected_full();current();
            if(file!=INVALID_HANDLE_VALUE){
                BY_HANDLE_FILE_INFORMATION now{};
                need(GetFileInformationByHandle(file,&now)&&now.dwVolumeSerialNumber==identity.dwVolumeSerialNumber&&
                     now.nFileIndexHigh==identity.nFileIndexHigh&&now.nFileIndexLow==identity.nFileIndexLow&&
                     now.nFileSizeHigh==identity.nFileSizeHigh&&now.nFileSizeLow==identity.nFileSizeLow&&
                     now.ftLastWriteTime.dwHighDateTime==identity.ftLastWriteTime.dwHighDateTime&&
                     now.ftLastWriteTime.dwLowDateTime==identity.ftLastWriteTime.dwLowDateTime,
                     "Installed Color actual Original HANDLE identity changed");
                std::wstring final(32768,L'\0');const auto size=GetFinalPathNameByHandleW(file,final.data(),DWORD(final.size()),FILE_NAME_NORMALIZED|VOLUME_NAME_DOS);
                need(size&&size<final.size(),"Installed Color actual final handle path unavailable");final.resize(size);
                if(final.starts_with(L"\\\\?\\"))final.erase(0,4);
                need(::_wcsicmp(final.c_str(),path.lexically_normal().c_str())==0,"Installed Color actual handle routed through another alias");
            }
            current();op.current();
        }
        DWORD open(const std::filesystem::path& actual,std::optional<std::uint32_t> length={}) {
            path=actual;file=CreateFileW(path.c_str(),GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE,nullptr,OPEN_EXISTING,0x08000080,nullptr);
            if(file==INVALID_HANDLE_VALUE){const auto error=GetLastError();need(error!=0,"Installed Color failed open has no OS result");return error;}
            need(GetFileInformationByHandle(file,&identity)&&identity.nFileSizeHigh==0&&identity.nFileSizeLow<=8u*1024u*1024u&&
                 !(identity.dwFileAttributes&(FILE_ATTRIBUTE_DIRECTORY|FILE_ATTRIBUTE_REPARSE_POINT))&&
                 (!length||identity.nFileSizeLow==*length),"Installed Color Original stream extent/type outside finite supported source");
            full_current();return ERROR_SUCCESS;
        }
        std::vector<std::uint8_t> read()const {
            full_current();need(file!=INVALID_HANDLE_VALUE,"Installed Color read outside actual open HANDLE");
            std::vector<std::uint8_t> bytes(identity.nFileSizeLow);std::uint32_t cursor=0;
            while(cursor<bytes.size()){
                current();DWORD received=0;
                need(ReadFile(file,bytes.data()+cursor,DWORD(bytes.size()-cursor),&received,nullptr)&&received>0&&received<=bytes.size()-cursor,
                     "Installed Color Original read failed/short/no progress");cursor+=received;current();
            }
            full_current();return bytes;
        }
    };
    std::size_t callbacks=0;
    {
        const auto consume=input;op.current();need(bool(consume),"Installed Color input consumer absent");
        const auto lend=[&](ReadLoan& held,std::vector<std::uint8_t> bytes){
            held.full_current();need(callbacks++==0,"Installed601CA0 repeated actual input");
            const auto check=[&held]{held.current();};
            const auto affinity=[check](const BnwNativeOwnedSkinOccurrence&,unsigned,const char*){
                check();throw std::runtime_error("Actual base Color input is not a later Skin slot input");};
            {
                const auto selected=std::shared_ptr<const BnwNativeOwnedSkinSelectedInput>(new BnwNativeOwnedSkinSelectedInput(check,std::move(bytes),affinity));
                selected->require_current();consume(selected);selected->require_current();
            }
            held.full_current(); // Actual Reader/input destruction precedes physical FULL POST.
        };
        for(unsigned attempt=0;attempt<2&&callbacks==0;++attempt){
            bool miss=false;
            with_native_record("Assets/UI/ColorAtlas.xml",[&](const auto* node,const auto* prefix,const auto* first){
                op.current();if(!node){need(!prefix&&!first,"Installed Color native MISS supplied an output");miss=true;return;}
                need(prefix,"Installed Color selected HIT lacks actual prefix");
                const auto words=*node;const auto prefix_words=*prefix;
                const std::optional<std::string> first_words=first?std::optional<std::string>(*first):std::nullopt;
                need(words.length<=8u*1024u*1024u,"Installed Color selected extent exceeds finite reader bound");
                std::shared_ptr<OwnedCategoryStage> stage;
                std::shared_ptr<const BnwNativeCategoryLooseAdmissionReceipt> loose;
                std::shared_ptr<DefaultLooseNativeNode> default_loose;
                std::shared_ptr<const BnwNativeGlobalArchivePrefixRecord> archive_prefix;
                std::shared_ptr<const BnwNativeArchiveCachePublicationOwner> publication;
                std::shared_ptr<const assets::FpkSource> canonical_source;
                const assets::FpkEntry* entry=nullptr;std::size_t matches=0;
                for(const auto& candidate:owned_category_stages_){
                    require_installed_color_skin_category_stage_current(*candidate);op.current();
                    const auto native=candidate->arena->borrow_current_category_record_for_manager_query(*candidate->namespace_channel,words.crc);
                    if(native.record!=node)continue;
                    ++matches;stage=candidate;loose=native.loose;
                    if(loose)need(loose->source_.lock().get()==stage->loose_source.get()&&words.offset==0&&
                         words.length==std::uint32_t(loose->length_)&&(words.packed_prefix>>20)==loose->prefix_index_&&prefix_words==loose->prefix_,
                         "Installed Color selected loose node/receipt/source changed");
                    else {
                        const auto found=stage->archived_selected.find(words.crc);need(found!=stage->archived_selected.end(),"Installed Color selected archive lacks actual witness");
                        const auto& actual=found->second;need(actual.actual_entry&&native.source_entry==actual.actual_entry&&
                             native.source.get()==actual.actual_source.get()&&words.offset==actual.actual_entry->payload_offset&&words.length==actual.actual_entry->payload_length,
                             "Installed Color selected archive differs from actual admitted entry");
                        archive_prefix=actual.prefix;publication=actual.publication;canonical_source=native.source;entry=native.source_entry;
                    }
                }
                const auto actual_loose=i.default_state->loose_native.find(words.crc);
                if(actual_loose!=i.default_state->loose_native.end()&&actual_loose->second&&&actual_loose->second->word_==node){
                    ++matches;default_loose=actual_loose->second;
                    need(&i.default_state->selected.records.at(words.crc)==default_loose->selected_entry_&&words.offset==0&&
                         prefix_words==default_loose->registered_prefix_&&(words.packed_prefix>>20)==default_loose->prefix_index_,
                         "Installed Color Default loose node lost actual insertion");
                }
                if(!stage&&!default_loose){
                    const auto native=i.entry_arena->borrow_current_default_record_for_global_query(*i.current_default_source,words.crc);
                    if(native.record==node){
                        ++matches;canonical_source=native.source;entry=native.source_entry;
                        need(entry&&canonical_source&&native.handle&&words.offset==entry->payload_offset&&words.length==entry->payload_length,
                             "Installed Color Default archive lost genuine selected entry");
                        const auto find=[&](const auto& step){
                            if(!step->prefix||step->prefix->source_index()!=(words.packed_prefix>>20))return;
                            need(!archive_prefix,"Installed Color actual prefix aliases another archive attempt");
                            archive_prefix=step->prefix;publication=step->publication;
                            need(step->header&&publication&&&archive_prefix->header_owner()==step->header.get()&&
                                 publication->canonical_payload_source().get()==canonical_source.get(),
                                 "Installed Color Default archive lost actual publication/Header");
                        };
                        i.find_installed_color_primary_archive(words.packed_prefix>>20,canonical_source.get(),archive_prefix,publication);
                        for(const auto& step:i.common_steps)find(step);
                    }
                }
                need(matches==1,"Installed Color selected native pointer has absent/ambiguous actual source");
                ReadLoan held(*data,op);
                const auto* retained_header=archive_prefix?&archive_prefix->header_owner():nullptr;
                held.selected_identity=[&]{
                    need(*node==words&&*prefix==prefix_words&&(first?std::optional<std::string>(*first):std::nullopt)==first_words,
                         "Installed Color actual selected node/prefix/FIRST changed inside held read");
                    if(stage)require_installed_color_skin_category_stage_identity(*stage);
                    if(default_loose)need(i.default_state->loose_native.at(words.crc)==default_loose&&
                         &i.default_state->selected.records.at(words.crc)==default_loose->selected_entry_,
                         "Installed Color actual Default selected insertion changed inside held read");
                    if(archive_prefix)archive_prefix->require_header_identity_without_callbacks(*retained_header);
                };
                held.selected_full=[&]{
                    need(*node==words&&*prefix==prefix_words&&(first?std::optional<std::string>(*first):std::nullopt)==first_words,
                         "Installed Color selected node/prefix/FIRST changed during real read");
                    if(stage){
                        require_installed_color_skin_category_stage_current(*stage);
                        const auto now=stage->arena->borrow_current_category_record_for_manager_query(*stage->namespace_channel,words.crc);
                        need(now.record==node&&now.loose.get()==loose.get()&&(!entry||(now.source.get()==canonical_source.get()&&now.source_entry==entry)),
                             "Installed Color selected Stage pointer/entry replaced");
                    }
                    if(loose)loose->require_current();
                    if(default_loose){
                        need(i.default_state->loose_native.at(words.crc)==default_loose&&
                             &i.default_state->selected.records.at(words.crc)==default_loose->selected_entry_,
                             "Installed Color Default loose insertion replaced");
                        default_loose->original_stat_->require_same_service(*i.stat_service);
                        const auto stat=default_loose->original_stat_->current();
                        need(stat.return_value==0&&stat.size&&*stat.size>=0&&std::uint32_t(*stat.size)==words.length,
                             "Installed Color Default loose stat length changed");
                    }
                    if(archive_prefix){
                        archive_prefix->require_current();publication->require_current();
                        need(archive_prefix->raw_prefix()==prefix_words&&archive_prefix->source_index()==(words.packed_prefix>>20)&&
                             publication->canonical_payload_source().get()==canonical_source.get()&&entry&&
                             std::uint64_t(words.offset)+words.length<=archive_prefix->header_owner().original_byte_length()&&
                             std::uint64_t(words.offset)+words.length<=UINT32_MAX,
                             "Installed Color actual archive prefix/publication/extent changed");
                    }
                };
                held.full_current();data->read_attempts.push_back({true,0});
                if(archive_prefix){
                    std::vector<std::uint8_t> bytes(words.length);std::uint32_t cursor=0;
                    while(cursor<words.length){held.current();
                        const auto read=archive_prefix->header_owner().read_retained_original_cached_stream(words.offset+cursor,
                            std::span<std::uint8_t>(bytes).subspan(cursor));
                        need(read.error==0&&read.count>0&&read.count<=words.length-cursor,
                             "Installed Color actual cached Original payload read failed/short/no progress");cursor+=read.count;held.current();}
                    lend(held,std::move(bytes));held.full_current();
                }else {
                    need(loose||default_loose,"Installed Color selected source backend absent");
                    const auto reconstructed=BnwNativeSkinResourceQueryOutputProducer::canonical_filename_839fe0(prefix,first);
                    std::filesystem::path actual;
                    if(loose){const auto source=loose->source_.lock();need(bool(source),"Installed Color actual loose source retired");
                        actual=source->original_path(loose->query_+loose->basename_);
                        need(source->original_path(reconstructed)==actual,"Installed Color FIRST collision/path alias unsupported");}
                    else {
                        const std::filesystem::path relative(native_loose_detail::acp_wide(reconstructed));
                        need(!relative.has_root_path()&&relative.lexically_normal()==default_loose->original_stat_->relative_source().lexically_normal(),
                             "Installed Color Default reconstructed filename differs from actual stat source");
                        actual=native_category_source_detail::source_path(data->admission->root,relative);
                    }
                    const auto error=held.open(actual,words.length);data->read_attempts.back().error=error;
                    need(error==ERROR_SUCCESS,"Mandatory actual selected-HIT failed-open fallback source before installed Color continuation");
                    auto bytes=held.read();
                    if(loose){const auto identity=runtime::identify_bytes(bytes);
                        need(identity.bytes==loose->original_bytes_.bytes&&identity.sha256==loose->original_bytes_.sha256,
                             "Installed Color actual Original read differs from admitted loose bytes");}
                    lend(held,std::move(bytes));held.full_current();
                }
            });op.current();
            if(!miss)continue;
            // SAME mode1 two-NULL directOS route as the accepted initial
            // stream, now only AFTER this actual nonempty namespace's MISS.
            ReadLoan held(*data,op);
            const auto actual=installed_startup_admission_detail::confined_path(data->admission->root,"Assets/UI/ColorAtlas.xml");
            const auto error=held.open(actual);data->read_attempts.push_back({false,error});
            if(error==ERROR_SUCCESS){lend(held,held.read());held.full_current();}
        }
        if(callbacks==0){
            ReadLoan failed(*data,op);failed.full_current();
            need(data->read_attempts.size()==2&&std::all_of(data->read_attempts.begin(),data->read_attempts.end(),
                [](const auto& attempt){return !attempt.selected&&attempt.error!=ERROR_SUCCESS;}),
                "Installed Color native-false lacks both actual NULL/OS-open failures");
            ++callbacks;consume(nullptr);failed.full_current();
        }
        op.current();need(callbacks==1,"Installed601CA0 omitted/repeated actual input callback");
    }
    op.current();need(!data->active_base_read,"Installed Color read escaped physical loan teardown");data->base_stream_returned=true;
}
void BnwNativePackageDiscoveryOwner::with_installed_color_skin_base_input(const std::function<void(
    const std::shared_ptr<const detail::BnwNativeOwnedSkinSelectedInput>&)>& consume)const {
    require_installed_color_skin_operation_current();impl_->live_resource_query->with_installed_color_skin_base_input(consume);
}

void detail::BnwNativeGlobalResourceWarmOwner::drive_installed_color_skin() {
    need(thread_==std::this_thread::get_id(),"Installed52D590 driver foreign creator");
    if(installed_color_skin_attempted_){failed_.store(true);throw std::runtime_error("Installed52D590 replay/reentry");}
    installed_color_skin_attempted_=true;
    try {
        ParentPin retained(*this);const auto storage=retained.parent->impl_;auto& i=*storage;
        require_installed_initial_categories_returned();
        need(!installed_color_skin_driver_&&!i.header_active&&!i.global_color_bank&&
             !active_owned_skin_operation_epoch_&&!owned_skin_occurrence_driver_returned_&&!owned_skin_instance_ctor_driver_returned_&&
             owned_skin_occurrences_.empty()&&owned_skin_instances_.empty()&&owned_skin_extra_attribute_receipts_.empty()&&
             !returned_standalone_packages_.empty()&&returned_standalone_packages_.size()<=UINT32_MAX&&owned_skin_operation_epoch_!=UINT64_MAX,
             "Installed52D590 requires genuine initial categories before actual reset/constructors");
        const auto data=std::make_shared<InstalledColorSkinDriver>(i,*this);installed_color_skin_driver_=data;
        struct Failure {BnwNativePackageDiscoveryOwner::Impl& i;std::shared_ptr<InstalledColorSkinDriver> data;bool done=false;
            ~Failure(){if(!done){data->failed=true;i.header_poisoned=true;}}} failure{i,data};
        InstalledColorSkinDriver::Operation operation(*data,*this,*retained.parent,i);
        operation.current();BnwPrimaryDefaultStageTrace trace("Installed.ColorSkin.initial52D590");
        {
            BnwNativePackageDiscoveryOwner::Impl::HeaderGuard fence(i.header_callback_state,i.self);
            need(fence.valid(),"Installed52D590 Parent retired/reentered");
            struct Active {
                BnwNativePackageDiscoveryOwner::Impl& i;BnwNativeGlobalResourceWarmOwner& source;
                InstalledColorSkinDriver::Operation& operation;
                Active(BnwNativePackageDiscoveryOwner::Impl& actual,BnwNativeGlobalResourceWarmOwner& warm,
                    InstalledColorSkinDriver::Operation& op,const BnwNativePackageDiscoveryOwner::Impl::HeaderGuard& header)
                    :i(actual),source(warm),operation(op){
                    need(!i.header_active&&!i.active_live_query_owner&&!source.active_owned_skin_operation_epoch_,"Installed52D590 Header reentry");
                    operation.header=&header;operation.serial=++source.owned_skin_operation_epoch_;
                    source.active_owned_skin_operation_epoch_=operation.serial;
                    i.header_active=true;i.active_header_fence=&header;i.active_live_query_owner=&source;
                }
                ~Active(){source.active_owned_skin_operation_epoch_=0;operation.header=nullptr;operation.serial=0;
                    i.active_live_query_owner=nullptr;i.active_header_fence=nullptr;i.header_active=false;}
            } active{i,*this,operation,fence};
            returned_standalone_packages_.front()->with_actual_descriptor([&](const auto& held){
                operation.loan=&held;active_owned_skin_package_=held.retained;active_owned_skin_resource_loan_=&held.resource;
                active_owned_skin_resource_epoch_=held.actor->state_->active_resource_loan;
                struct LoanExit {BnwNativeGlobalResourceWarmOwner& source;InstalledColorSkinDriver::Operation& operation;
                    ~LoanExit(){operation.loan=nullptr;source.active_owned_skin_package_.reset();
                        source.active_owned_skin_resource_loan_=nullptr;source.active_owned_skin_resource_epoch_=0;}} loan_exit{*this,operation};
                operation.current();
                data->base_native_true=retained.parent->prepare_installed_color_skin_bank();
                need(data->base_stream_returned,"Installed601CA0 base call returned without actual stream read");
                data->base_load_returned=true;data->phase=InstalledColorSkinDriver::Phase::constructors;operation.current();
                for(std::size_t ordinal=0;ordinal<returned_standalone_packages_.size();++ordinal){
                    BnwPrimaryDefaultStageTrace package_trace("Installed.ColorSkin.package48C800",static_cast<long long>(ordinal));
                    const auto package=returned_standalone_packages_[ordinal];std::uint32_t count=0;
                    const auto first=owned_skin_instances_.size();data->constructor_ordinal=ordinal;
                    package->with_actual_descriptor([&](const auto& actual){
                        data->constructor_loan=&actual;
                        struct End {InstalledColorSkinDriver& data;~End(){data.constructing=nullptr;data.constructor_loan=nullptr;}} end{*data};
                        operation.current();count=BnwNativePackageResourceCheckpoint::count_from_loaded_762400(*package,2);
                        if(count==0){operation.current();return;} //Actual48C800 zero return before constructor work.
                        const auto& doc=actual.resource.actual_document;need(!doc.roots.empty(),"Installed48C800 actual Parser root absent");
                        std::uint32_t occurrence=0;
                        for(const auto& child:doc.roots.front()->children){
                            if(BnwNativePackageResourceCheckpoint::range(doc,child->name)!="UISkin")continue;
                            if(occurrence>=count)break;
                            auto token=std::shared_ptr<BnwNativeOwnedSkinOccurrence>(new BnwNativeOwnedSkinOccurrence);
                            token->warm_=i.live_resource_query;token->package_=package;token->epoch_=epoch_;token->document_=&doc;token->node_=child.get();
                            token->factory_ordinal_=ordinal;token->occurrence_ordinal_=occurrence++;token->roster_index_=owned_skin_occurrences_.size();
                            owned_skin_occurrences_.push_back(token);data->constructing=token.get();
                            token->require_current();operation.current();retained.parent->construct_installed_color_skin_instance(token);
                            need(owned_skin_instances_.size()==token->roster_index_+1,"Installed48C800 lost actual instance append");
                            const auto instance=owned_skin_instances_.back();
                            data->constructors.push_back({token,instance,instance->constructor_source_.get()});
                            token->require_current();operation.current();data->constructing=nullptr;
                        }
                        need(occurrence==count,"Installed48C800 count differs from SAME actual UISkin nodes");operation.current();
                    });operation.current();
                    need(owned_skin_instances_.size()-first==count,"Installed48C800 did not append its actual constructor returns");
                    data->packages.push_back({ordinal,count,first,owned_skin_instances_.size(),package});
                    if(count)++data->nonzero_packages;
                }
                // Both walks/real package calls have now returned, in actual
                // package order. This is not a pre-issued constructor license.
                owned_skin_occurrence_driver_returned_=true;
                complete_owned_skin_instance_constructors();operation.current();
                data->phase=InstalledColorSkinDriver::Phase::reconcile;
                retained.parent->bind_native_owned_color_bank_constructed_roster();operation.current();
                const auto bank=data->bank.lock();need(bool(bank),"Installed52D590 actual Color Bank retired");
                bank->reconcile_owned_colors_462c20();operation.current();data->reconcile_returned=true;
            });operation.current();need(fence.valid(),"Installed52D590 whole Loaded POST lost actual Header");
        }
        operation.current();need(data->packages.size()==returned_standalone_packages_.size()&&data->reconcile_returned,
            "Installed52D590 real package/final462C20 return roster incomplete");
        i.frame.next_resource_dependency="Actual initial52D590 base Color/48C800/462C20 returned; genuine later common/PC660330 and global selection/completed startup suppliers mandatory";
        data->phase=InstalledColorSkinDriver::Phase::returned;data->returned=true;failure.done=true;
    }catch(...){failed_.store(true);throw;}
}
void detail::BnwNativeGlobalResourceWarmOwner::require_installed_color_skin_returned()const {
    need(thread_==std::this_thread::get_id(),"Installed52D590 normal-return lender foreign creator");
    ParentPin retained(*this);const auto storage=retained.parent->impl_;const auto data=installed_color_skin_driver_;
    try {
        need(data&&data->returned&&!data->failed&&!data->operation&&!data->active_base_read&&!data->constructor_loan&&!data->constructing&&
             data->phase==InstalledColorSkinDriver::Phase::returned&&data->base_stream_returned&&data->base_load_returned&&data->reconcile_returned&&
             !storage->header_active&&!active_owned_skin_operation_epoch_&&!active_owned_skin_resource_loan_&&
             owned_skin_occurrence_driver_returned_&&owned_skin_instance_ctor_driver_returned_&&
             owned_skin_occurrences_.size()==owned_skin_instances_.size()&&data->constructors.size()==owned_skin_instances_.size()&&
             data->packages.size()==returned_standalone_packages_.size(),
             "Installed52D590 has no actual normal return/source teardown");
        require_installed_initial_categories_returned();std::size_t cursor=0;std::uint32_t positive=0;
        for(std::size_t ordinal=0;ordinal<data->packages.size();++ordinal){const auto& row=data->packages[ordinal];
            need(row.ordinal==ordinal&&row.package.lock().get()==returned_standalone_packages_[ordinal].get()&&
                 row.first==cursor&&row.last-row.first==row.count&&row.last<=owned_skin_instances_.size(),
                 "Installed52D590 package48C800 return journal changed");cursor=row.last;if(row.count)++positive;}
        const auto bank=data->bank.lock();
        need(cursor==owned_skin_instances_.size()&&positive==data->nonzero_packages&&bank&&storage->global_color_bank.get()==bank.get()&&
             owned_bank_receiver_.lock().get()==bank.get()&&owned_bank_source_attachment_returned_&&
             bank->owned_source_.get()==owned_bank_source_identity_&&!bank->closed_.load()&&
             bank->phase_==BnwNativeGlobalColorRegistryBank::Phase::roster_bound&&bank->owned_roster_.size()==owned_skin_instances_.size(),
             "Installed52D590 lost actual Bank/constructor roster/native return count");
        for(std::size_t n=0;n<owned_skin_instances_.size();++n)need(owned_skin_instances_[n]&&owned_skin_occurrences_[n]&&
             bank->owned_roster_[n].lock().get()==owned_skin_instances_[n].get()&&
             data->constructors[n].instance.lock().get()==owned_skin_instances_[n].get()&&
             data->constructors[n].occurrence.lock().get()==owned_skin_occurrences_[n].get()&&
             data->constructors[n].source==owned_skin_instances_[n]->constructor_source_.get(),
             "Installed52D590 retained constructor source/Bank order changed");
        require_current();
    }catch(...){if(data)data->failed=true;failed_.store(true);storage->header_poisoned=true;throw;}
}
void BnwNativePackageDiscoveryOwner::drive_installation_initial_color_skin()const {
    const auto storage=impl_;auto& i=*storage;
    need(i.thread==std::this_thread::get_id(),"Installed52D590 public entry foreign creator");
    const auto warm=i.live_resource_query;
    need(i.installation_input&&!i.lease&&!i.rules&&warm,"Installed52D590 requires actual installed Input/Warm");
    try{warm->drive_installed_color_skin();warm->require_installed_color_skin_returned();}
    catch(...){warm->failed_.store(true);i.header_poisoned=true;throw;}
}
