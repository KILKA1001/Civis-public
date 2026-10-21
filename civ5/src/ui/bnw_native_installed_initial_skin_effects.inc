// Actual first628BE0/Common660330 only. Source8a's counter/constraint history
// remains immutable; this is a fresh Header/Loaded/Bank effect operation.
struct detail::BnwNativeGlobalResourceWarmOwner::InstalledInitialSkinEffects {
    using Warm=BnwNativeGlobalResourceWarmOwner;
    using Parent=BnwNativePackageDiscoveryOwner;
    using Loaded=BnwNativeStandalonePackageLoaded;
    using Bank=BnwNativeGlobalColorRegistryBank;
    using Policy=BnwNativeOwnedSkinBootPolicy;
    using Input=runtime::BnwSelectedLanguageInputOwner;
    using Boot=BnwNativeResourceBootSettingsOwner;
    using Selection=InstalledInitialSkinSelection;
    using Consumer=BnwNativeOwnedSkinConsumer;
    const std::thread::id creator=std::this_thread::get_id();
    const std::weak_ptr<const Parent> parent;
    const std::weak_ptr<const void> warm,input,epoch,boot,catalog;
    const std::shared_ptr<Selection> selection;
    const std::shared_ptr<Policy> policy;
    const std::weak_ptr<Bank> bank;
    const std::size_t initial_stage_count;
    bool failed=false,kernel_returned=false,returned=false,native_changed=false;
    const void* source_identity=nullptr;
    const Bank::OwnedSkinOperation* bank_operation_identity=nullptr;
    std::shared_ptr<const Consumer> consumer;
    // A later real invocation borrows this genuine first return; it never
    // writes operation back to the departed Source8b stack object.
    class Continuation {
        friend class BnwNativeGlobalResourceWarmOwner;
      protected:
        Continuation()=default;
      public:
        virtual ~Continuation()=default;
        Continuation(const Continuation&)=delete;
        virtual void require_identity()const=0;
        virtual void require_operation_current()const=0;
        virtual void require_stage_identity(const OwnedCategoryStage&)const=0;
        virtual void require_stage_current(const OwnedCategoryStage&)const=0;
        virtual void require_read_identity(const void*)const=0;
        virtual void require_bank_current(const Bank&,const content::XmlNode*,std::uint64_t)const=0;
        virtual void require_effect_source_current(const Policy&,const void*)const=0;
        virtual void require_actual_global_effects_factory_current(const Policy&,const Consumer&,const void*)const {
            throw std::runtime_error("Actual global EffectsSource factory needs NEW genuine global caller");
        }
        virtual void observe_actual_global_set_read(const BnwNativeOwnedUiSkinPoststageInstance&,
            const std::optional<std::string>&)const {
            throw std::runtime_error("Actual67 set observation needs SAME new global selection iteration");
        }
        virtual void require_actual_global_exclusion_current(const BnwNativeOwnedUiSkinPoststageInstance&,unsigned)const {
            throw std::runtime_error("Actual4691 exclusion needs SAME new global slot/index owner");
        }
        virtual void require_actual_global_effects_borrow_current(const Policy&,const Consumer&,const void*)const {
            throw std::runtime_error("Actual global EffectsSource loan needs NEW genuine global caller");
        }
    };
    const Continuation* continuation=nullptr;
    std::weak_ptr<Consumer> continuation_consumer; // Minted only by the actual mutable8b allocation/return issuer.
    // Typed weak control minted from SAME concrete Source local at actual8b return.
    // No downcast, substitute Source or strong external Parent ownership.
    std::weak_ptr<Parent::OwnedSkinEffectsSource> continuation_effects_source;

    struct FinalInstance {
        std::weak_ptr<const BnwNativeOwnedUiSkinPoststageInstance> instance;
        std::array<std::uint8_t,2> enabled{};
        std::array<std::array<std::weak_ptr<const void>,2>,2> pairs;
        std::array<std::array<std::optional<std::uint8_t>,2>,2> current89;
    };
    std::vector<FinalInstance> final_instances;
    std::size_t returned_stage_count=0,returned_output_count=0,returned_read_count=0;
    std::uint64_t returned_epoch=0;
    struct Operation;
    Operation* operation=nullptr;
    InstalledInitialSkinEffects(const Parent::Impl& i,const Warm& source)
        :parent(i.self),warm(i.live_resource_query),input(i.installation_input),epoch(i.identity),
         boot(i.settings),catalog(i.resource_catalog),selection(source.installed_initial_skin_selection_),
         policy(selection->policy.lock()),bank(selection->color->bank),initial_stage_count(source.owned_category_stages_.size()){}
    static bool same(const auto& a,const auto& b){return !a.owner_before(b)&&!b.owner_before(a);}
};
struct detail::BnwNativeGlobalResourceWarmOwner::InstalledInitialSkinEffects::Operation final
    :Selection::Continuation {
    InstalledInitialSkinEffects& data;
    Warm& source;
    const Parent& parent;
    Parent::Impl& i;
    const Parent::Impl::HeaderGuard* header=nullptr;
    const Loaded::Loan* loan=nullptr;
    const Bank::OwnedSkinOperation* bank_operation=nullptr;
    std::uint64_t serial=0;
    Operation(InstalledInitialSkinEffects& actual,Warm& owner,const Parent& facade,Parent::Impl& storage)
        :data(actual),source(owner),parent(facade),i(storage){
        need(data.creator==std::this_thread::get_id()&&!data.operation&&!data.selection->continuation,
            "Installed first660330 replay/foreign actual continuation");
        data.operation=this;data.selection->continuation=this;
    }
    ~Operation(){
        if(data.operation==this&&data.selection->continuation==this){data.selection->continuation=nullptr;data.operation=nullptr;}
        else {data.failed=true;i.header_poisoned=true;}
    }
    Operation(const Operation&)=delete;
        void loan_identity(const Loaded::Loan& held,std::size_t ordinal)const {
        need(ordinal<data.selection->color->admission->rows.size()&&ordinal<source.returned_standalone_packages_.size(),
             "Installed Color/Skin actual loan ordinal escaped factory");
        const auto& row=data.selection->color->admission->rows[ordinal];const auto& actual=source.returned_standalone_packages_[ordinal];
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
        need(data.creator==std::this_thread::get_id()&&source.thread_==data.creator&&i.thread==data.creator&&
             !data.failed&&!data.returned&&!source.closed_.load()&&!source.failed_.load()&&
             !i.header_poisoned&&!i.category_lineage_failed&&data.operation==this&&
             data.selection->continuation==this&&source.installed_initial_skin_effects_.get()==&data&&
             source.installed_initial_skin_effects_attempted_,"Installed first Common actual effect frame retired");
        const auto& selection=*data.selection;const auto& color=*selection.color;
        need(i.installation_input&&!i.lease&&!i.rules&&i.live_resource_query.get()==&source&&
             same(i.self,data.parent)&&same(i.live_resource_query,data.warm)&&same(i.installation_input,data.input)&&
             same(i.identity,data.epoch)&&i.settings&&same(i.settings,data.boot)&&same(i.resource_catalog,data.catalog)&&
             source.installed_initial_skin_selection_.get()==&selection&&selection.prefix_returned&&!selection.failed&&
             !selection.operation&&!selection.policy_construction_active&&selection.counter_returned&&selection.enabled_instances==0&&
             selection.pending==Selection::Pending::common660330&&selection.color.get()==source.installed_color_skin_driver_.get()&&
             color.returned&&!color.failed&&!color.operation&&!color.constructor_loan&&!color.constructing&&!color.active_base_read&&
             color.phase==InstalledColorSkinDriver::Phase::returned&&color.admission&&color.admission->returned&&!color.admission->failed&&
             color.categories&&color.categories->native_loop_returned&&!color.categories->failed&&!color.categories->operation&&
             color.categories->pending.expired()&&!i.active_completed_resource_operation&&!i.category_source&&
             !source.active_owned_category_preconstructor_&&!source.configured_activation_policy_returned_,
             "Installed first Common lost actual Input/Boot/Source8a zero gate/52D590 normal return");
        color.admission->identity(source,parent,i); // Canonical accounted outside-Parent proof, no new strong facade edge.
        need(data.policy&&source.owned_skin_boot_policy_.get()==data.policy.get()&&same(selection.policy,data.policy)&&
             data.policy->installed_source_&&data.policy->installed_source_->lock().get()==
                 static_cast<const Policy::InstalledSource*>(data.selection.get())&&same(*data.policy->installed_source_,data.selection)&&
             data.policy->phase_==Policy::Phase::common_active&&data.policy->installed_setters_returned_==2&&
             data.policy->installed_constraints_.size()==2&&data.policy->installed_constraints_.at("set")=="BaseGame"&&
             data.policy->installed_constraints_.at("platform")=="Common"&&data.policy->actual_platform_.empty()&&data.policy->lease_.expired()&&
             same(data.policy->input_,selection.language)&&same(selection.language,color.admission->rows.front().language),
             "Installed first Common substituted original two constraint writes/actual language/policy source");
        const auto actual=data.bank.lock();
        need(actual&&i.global_color_bank.get()==actual.get()&&source.owned_bank_receiver_.lock().get()==actual.get()&&
             source.owned_bank_source_attachment_returned_&&actual->owned_source_.get()==source.owned_bank_source_identity_&&
             actual->thread_==data.creator&&!actual->closed_.load()&&actual->phase_==Bank::Phase::roster_bound&&
             actual->owned_roster_.size()==source.owned_skin_instances_.size()&&source.owned_skin_instance_ctor_driver_returned_&&
             source.owned_skin_occurrence_driver_returned_&&color.constructors.size()==source.owned_skin_instances_.size()&&
             source.owned_skin_occurrences_.size()==source.owned_skin_instances_.size()&&!color.admission->rows.empty()&&
             selection.observed.size()==source.owned_skin_instances_.size(),
             "Installed first Common lost SAME actual Bank/constructed roster");
        for(std::size_t n=0;n<source.owned_skin_instances_.size();++n){
            const auto instance=source.owned_skin_instances_[n];const auto occurrence=source.owned_skin_occurrences_[n];
            need(instance&&occurrence&&instance->thread_==data.creator&&color.constructors[n].instance.lock().get()==instance.get()&&
                 color.constructors[n].occurrence.lock().get()==occurrence.get()&&actual->owned_roster_[n].lock().get()==instance.get()&&
                 color.constructors[n].source==instance->constructor_source_.get()&&
                 selection.observed[n].instance.lock().get()==instance.get()&&
                 selection.observed[n].constructor==instance->constructor_source_.get()&&
                 selection.observed[n].enabled==std::array<std::uint8_t,2>{0,0},
                 "Installed first Common lost actual constructor/724CC0 history");
        }
        if(header)need(serial&&i.header_active&&i.active_header_fence==header&&header->valid()&&
             i.active_live_query_owner==&source&&source.active_owned_skin_operation_epoch_==serial,
             "Installed first Common lost SAME actual new Header/epoch");
        else need(!serial&&!i.header_active&&!i.active_live_query_owner&&!source.active_owned_skin_operation_epoch_,
             "Installed first Common borrowed another retired Header");
        if(loan){
            need(header&&source.active_owned_skin_resource_loan_==&loan->resource&&
                 source.active_owned_skin_package_.lock().get()==&loan->token&&
                 source.active_owned_skin_resource_epoch_==loan->actor->state_->active_resource_loan,
                 "Installed first Common lost genuine outer Loaded/ResourceLoan");
            loan_identity(*loan,0);
        }else need(!source.active_owned_skin_resource_loan_,"Installed first Common orphaned actual resource loan");
        if(bank_operation)need(header&&loan&&actual->active_owned_skin_operation_==bank_operation&&actual->busy_.load()&&
             bank_operation->scope.bank.get()==actual.get()&&data.bank_operation_identity==bank_operation,
             "Installed first Common substituted real owning Bank operation");
        if(data.source_identity)need(source.owned_skin_effect_source_identity_==data.source_identity&&
             source.owned_skin_effect_operation_epoch_==data.returned_epoch&&
             source.owned_skin_effect_bank_operation_==data.bank_operation_identity,
             "Installed first Common actual protected Source registration changed");
        else need(!source.owned_skin_effect_source_identity_&&!source.owned_skin_effect_operation_epoch_&&
             !source.owned_skin_effect_bank_operation_,"Installed first Common inherited an unrelated effect Source");
        if(data.consumer)need(!data.continuation_effects_source.expired()&&
             same(data.continuation_effects_source,data.consumer->source_),
             "Installed first Common lost SAME typed EffectsSource return control");
        else need(data.continuation_effects_source.expired(),
             "Installed first Common typed Source loan minted before actual Consumer return");
    }
    void current()const {identity();source.require_current();if(loan)loan->require_current();identity();}
    void require_actual_inputs_current(const Input& actual,const Boot& actual_boot)const override {
        identity();const auto language=data.selection->language.lock();
        need(language&&language.get()==&actual&&i.settings.get()==&actual_boot&&same(i.settings,data.boot),
             "Installed first Common policy callback has another actual Input/Boot");identity();
    }
};

void detail::BnwNativeGlobalResourceWarmOwner::require_installed_initial_skin_effects_identity()const {
    need(thread_==std::this_thread::get_id(),"Installed first Common identity foreign creator");
    const auto data=installed_initial_skin_effects_;
    if(data&&data->continuation){data->continuation->require_identity();return;}
    need(data&&data->operation,"Installed first Common has no actual live effect operation");data->operation->identity();
}
void detail::BnwNativeGlobalResourceWarmOwner::require_installed_initial_skin_effects_current()const {
    need(thread_==std::this_thread::get_id(),"Installed resource continuation foreign creator");
    const auto data=installed_initial_skin_effects_;
    if(data&&data->continuation){data->continuation->require_operation_current();return;}
    require_installed_initial_skin_effects_identity();installed_initial_skin_effects_->operation->current();
}
void BnwNativePackageDiscoveryOwner::require_native_owned_skin_operation_current()const {
    need(impl_->thread==std::this_thread::get_id(),"Owned Skin source selector foreign creator");const auto& i=*impl_;
    if(i.installation_input){
        need(i.live_resource_query&&!i.lease&&!i.rules,"Installed Skin operation cannot borrow saved context");
        i.live_resource_query->require_installed_initial_skin_effects_current();return;
    }
    require_native_owned_skin_operation_current(i.saved_skin_lease(),i.resources);
}
const std::filesystem::path& BnwNativePackageDiscoveryOwner::installed_initial_skin_effects_root()const {
    need(impl_->thread==std::this_thread::get_id(),"Installed Skin root foreign creator");
    const auto warm=impl_->live_resource_query;need(bool(warm),"Installed Skin root actual Warm absent");
    warm->require_installed_initial_skin_effects_identity();return warm->installed_initial_skin_effects_->selection->color->admission->root;
}
void BnwNativePackageDiscoveryOwner::require_installed_initial_skin_effect_bank_current(
    const detail::BnwNativeGlobalColorRegistryBank& actual,const content::XmlNode* document,std::uint64_t ordinal)const {
    need(impl_->thread==std::this_thread::get_id(),"Installed first Common Bank foreign creator");
    const auto& i=*impl_;const auto warm=i.live_resource_query;need(bool(warm),"Installed first Common Bank actual Warm absent");
    if(const auto data=warm->installed_initial_skin_effects_;data&&data->continuation){
        data->continuation->require_bank_current(actual,document,ordinal);return;
    }
    warm->require_installed_initial_skin_effects_identity();const auto data=warm->installed_initial_skin_effects_;
    const auto& op=*data->operation;
    need(op.header&&op.loan&&!document&&ordinal==0&&data->bank.lock().get()==&actual&&i.global_color_bank.get()==&actual&&
         actual.thread_==i.thread&&!actual.closed_.load()&&actual.phase_==detail::BnwNativeGlobalColorRegistryBank::Phase::roster_bound&&
         actual.epoch_.get()==i.identity.get()&&detail::BnwNativeGlobalResourceWarmOwner::InstalledInitialSkinEffects::same(actual.parent_,i.self)&&
         actual.resource_catalog_.get()==i.resource_catalog.get()&&actual.query_default_source_.lock().get()==i.current_default_source.get()&&
         actual.query_default_state_.lock().get()==i.default_state.get()&&actual.query_default_records_==&i.default_state->selected&&
         actual.live_resource_query_.lock().get()==warm.get()&&actual.owned_source_.get()==warm->owned_bank_source_identity_,
         "Installed first Common Bank lost actual Input/Default/catalog/Loaded/Header identity");
}
void detail::BnwNativeGlobalResourceWarmOwner::require_installed_initial_skin_effect_stage_identity(const OwnedCategoryStage& stage)const {
    need(thread_==std::this_thread::get_id(),"Installed resource continuation foreign creator");
    const auto continuation_source=installed_initial_skin_effects_;
    if(continuation_source&&continuation_source->continuation){continuation_source->continuation->require_stage_identity(stage);return;}
    require_installed_initial_skin_effects_identity();const auto data=installed_initial_skin_effects_;const auto& op=*data->operation;
    const auto& admission=*data->selection->color->admission;
    need(op.header&&op.loan&&stage.factory_ordinal<returned_standalone_packages_.size()&&stage.category<3&&
         std::any_of(owned_category_stages_.begin(),owned_category_stages_.end(),[&](const auto& v){return v.get()==&stage;})&&
         stage.package.lock().get()==returned_standalone_packages_[stage.factory_ordinal].get()&&
         stage.document==admission.rows[stage.factory_ordinal].document&&stage.source&&stage.namespace_channel&&
         stage.source->owned_warm_&&stage.namespace_channel->owned_warm_&&
         InstalledInitialSkinEffects::same(*stage.source->owned_warm_,data->warm)&&
         InstalledInitialSkinEffects::same(*stage.namespace_channel->owned_warm_,data->warm)&&
         stage.source->descriptor_==stage.factory_ordinal&&stage.source->category_==stage.category&&
         stage.source->first_.get()==op.i.first_lineage.get()&&stage.namespace_channel->namespace_.get()==stage.source.get()&&
         !stage.namespace_channel->lease_&&stage.namespace_channel->installation_input_.get()==op.i.installation_input.get()&&
         stage.namespace_channel->cold_.get()==op.i.cold.get()&&stage.namespace_channel->first_.get()==op.i.first_lineage.get()&&
         stage.namespace_channel->epoch_.get()==op.i.identity.get(),
         "Installed effect Stage lost actual Header/Loaded/namespace/Input/FIRST controls");
    const auto manager=stage.manager.lock();
    if(stage.category<2){
        const auto categories=data->selection->color->categories;
        const auto actor=returned_standalone_packages_[stage.factory_ordinal]->actor_.lock();
        need(stage.installed_initial_driver&&!stage.installed_skin_effects_driver&&
             stage.installed_initial_driver->lock().get()==categories.get()&&InstalledInitialSkinEffects::same(*stage.installed_initial_driver,categories)&&
             stage.step==OwnedCategoryStage::Step::name_registered&&manager&&stage.arena&&stage.cache&&stage.loose_source&&stage.selected_source&&
             actor&&actor->state_->category_slots[stage.category]&&
             InstalledInitialSkinEffects::same(*actor->state_->category_slots[stage.category],stage.selected_source)&&
             std::any_of(categories->returned_calls.begin(),categories->returned_calls.end(),[&](const auto& row){
                 return row.ordinal==stage.factory_ordinal&&row.category==stage.category&&row.stage.lock().get()==&stage;}),
             "Installed effect borrowed initial category without its genuine Source6 return");
    }else {
        const auto instance=stage.skin_instance.lock();const auto occurrence=stage.skin_occurrence.lock();
        need(!stage.installed_initial_driver&&stage.installed_skin_effects_driver&&
             stage.installed_skin_effects_driver->lock().get()==data.get()&&InstalledInitialSkinEffects::same(*stage.installed_skin_effects_driver,data)&&
             instance&&occurrence&&stage.skin_slot<2&&stage.skin_member==0&&
             occurrence->roster_index_<owned_skin_instances_.size()&&
             owned_skin_instances_[occurrence->roster_index_].get()==instance.get()&&
             owned_skin_occurrences_[occurrence->roster_index_].get()==occurrence.get()&&
             data->selection->color->constructors[occurrence->roster_index_].source==instance->constructor_source_.get()&&
             occurrence->package_.lock().get()==stage.package.lock().get()&&occurrence->document_==stage.document&&
             occurrence->factory_ordinal_==stage.factory_ordinal&&occurrence->epoch_.lock().get()==epoch_.get(),
             "Installed UISkin Stage substituted its actual constructor/occurrence/slot origin");
        if(manager)need(instance->owned_resource_pairs_[stage.skin_slot][0].get()==manager.get()&&stage.arena&&stage.cache&&
             (stage.selected_source||(active_owned_category_stage_==&stage&&stage.step==OwnedCategoryStage::Step::paks_queried)),
             "Installed UISkin Stage is not actual pair member/Arena/cache");
        else need(active_owned_category_stage_==&stage&&stage.step==OwnedCategoryStage::Step::paks_queried&&
             !instance->owned_resource_pairs_[stage.skin_slot][0],
             "Installed UISkin Stage has no actual in-progress lazy manager construction");
    }
}
void detail::BnwNativeGlobalResourceWarmOwner::require_installed_initial_skin_effect_stage_current(const OwnedCategoryStage& stage)const {
    need(thread_==std::this_thread::get_id(),"Installed resource continuation foreign creator");
    const auto continuation_source=installed_initial_skin_effects_;
    if(continuation_source&&continuation_source->continuation){continuation_source->continuation->require_stage_current(stage);return;}
    require_installed_initial_skin_effect_stage_identity(stage);const auto data=installed_initial_skin_effects_;
    data->operation->current();const auto package=returned_standalone_packages_[stage.factory_ordinal];
    package->with_actual_descriptor([&](const auto& held){held.require_current();
        data->selection->color->admission->authenticate(*this,*package,held,
            data->selection->color->admission->rows[stage.factory_ordinal]);
        need(&held.resource.actual_document==stage.document,"Installed UISkin Stage Parser source changed");
        if(stage.category==2){const auto occurrence=stage.skin_occurrence.lock();const auto& doc=held.resource.actual_document;
            need(occurrence&&!doc.roots.empty(),"Installed UISkin Stage has no actual nthUISkin source");
            const BnwNativePackageXmlParserOwner::Node* node=nullptr;auto remaining=occurrence->occurrence_ordinal_;
            for(const auto& child:doc.roots.front()->children)if(BnwNativePackageResourceCheckpoint::range(doc,child->name)=="UISkin"){
                if(remaining){--remaining;continue;}node=child.get();break;}
            need(node==occurrence->node_,"Installed UISkin Stage actual nthUISkin changed");
        }held.require_current();});
    replay_owned_category_stage_io_current(stage);require_installed_initial_skin_effect_stage_identity(stage);
}
void detail::BnwNativeGlobalResourceWarmOwner::require_installed_skin_read_receipt_identity(const void* address)const {
    need(thread_==std::this_thread::get_id(),"Installed resource continuation foreign creator");
    const auto continuation_source=installed_initial_skin_effects_;
    if(continuation_source&&continuation_source->continuation){continuation_source->continuation->require_read_identity(address);return;}
    require_installed_initial_skin_effects_identity();const auto data=installed_initial_skin_effects_;const auto& op=*data->operation;
    const auto found=std::find_if(owned_skin_read_receipts_.begin(),owned_skin_read_receipts_.end(),[&](const auto& v){return v.get()==address;});
    need(found!=owned_skin_read_receipts_.end(),"Installed slot immutable bytes lack genuine read receipt");const auto& read=**found;
    need(read.installed_driver&&read.installed_driver->lock().get()==data.get()&&
         InstalledInitialSkinEffects::same(*read.installed_driver,data)&&read.operation&&op.header&&op.loan&&op.bank_operation,
         "Installed slot input origin/actual effect read loan expired");
    const auto operation=read.operation;const auto occurrence=operation->occurrence.lock();const auto query=operation->query;
    need(occurrence&&query&&query->reader_bound&&operation->issue_epoch==op.serial&&operation->slot<2&&
         (operation->property=="Style"||operation->property=="Color")&&operation->actual_default==op.i.default_state.get()&&
         operation->actual_default_source==op.i.current_default_source.get()&&operation->actual_bank_source==owned_bank_source_identity_&&
         query->occurrence.lock().get()==occurrence.get()&&query->slot==operation->slot&&query->property==operation->property&&
         query->filter==0x80&&query->issue_epoch==op.serial&&query->actual_document==occurrence->document_&&
         query->actual_node==occurrence->node_&&std::find(owned_skin_output_receipts_.begin(),owned_skin_output_receipts_.end(),query)!=owned_skin_output_receipts_.end()&&
         std::find(owned_skin_read_operations_.begin(),owned_skin_read_operations_.end(),operation)!=owned_skin_read_operations_.end()&&
         read.query_ordinal<query->actual_outputs.size()&&query->actual_outputs[read.query_ordinal]==read.emitted_filename&&
         occurrence->factory_ordinal_<returned_standalone_packages_.size()&&
         occurrence->package_.lock().get()==returned_standalone_packages_[occurrence->factory_ordinal_].get()&&
         occurrence->document_==data->selection->color->admission->rows[occurrence->factory_ordinal_].document&&
         occurrence->roster_index_<owned_skin_occurrences_.size()&&owned_skin_occurrences_[occurrence->roster_index_].get()==occurrence.get()&&
         (!operation->restoration_returned?(active_owned_skin_read_operation_.get()==operation.get()&&op.i.default_state->live_enabled89==0):
             (op.i.default_state->live_enabled89==1||(active_owned_skin_read_operation_&&op.i.default_state->live_enabled89==0&&
              active_owned_skin_read_operation_->actual_default==op.i.default_state.get()&&active_owned_skin_read_operation_->issue_epoch==op.serial))),
         "Installed slot immutable bytes lost real returned query/ordinal/Default toggle/constructor identity");
    if(!read.open_error)need(read.identity.has_value(),"Installed slot bytes were not read from actual physical stream");
    op.identity();
}

void BnwNativePackageDiscoveryOwner::with_installed_first_common_skin_effect_bank(const std::function<void(
    const std::shared_ptr<detail::BnwNativeGlobalColorRegistryBank>&,
    const std::shared_ptr<detail::BnwNativeOwnedSkinBootPolicy>&)>& input)const {
    need(impl_->thread==std::this_thread::get_id(),"Installed first Common target foreign creator");
    const auto warm=impl_->live_resource_query;need(bool(warm),"Installed first Common target actual Warm absent");
    detail::BnwNativeGlobalResourceWarmOwner::ParentPin retained(*warm);
    const auto check=[&]{warm->require_installed_initial_skin_effects_current();const auto data=warm->installed_initial_skin_effects_;
        need(data->operation->header&&data->operation->loan&&data->operation->bank_operation,
             "Installed first Common target lacks genuine Header/Loaded/Bank scope");data->policy->require_current();};
    check();{
        const auto consume=input;check();need(bool(consume),"Installed first Common actual kernel callback absent");
        const auto data=warm->installed_initial_skin_effects_;const auto bank=data->bank.lock();
        need(bool(bank),"Installed first Common retained Bank retired");consume(bank,data->policy);check();
    }check(); // Copied target REALLY destroyed while accounted Parent pin lives.
}
void BnwNativePackageDiscoveryOwner::record_installed_first_common_skin_consumer(
    const std::shared_ptr<detail::BnwNativeOwnedSkinConsumer>& consumer,
    const std::shared_ptr<OwnedSkinEffectsSource>& source,bool changed)const {
    require_native_owned_skin_operation_current();const auto warm=impl_->live_resource_query;const auto data=warm->installed_initial_skin_effects_;
    need(data&&!data->kernel_returned&&!data->consumer&&data->continuation_effects_source.expired()&&source&&consumer&&
         !source.owner_before(consumer->source_)&&!consumer->source_.owner_before(source)&&
         consumer->thread_==impl_->thread&&!consumer->active_&&!consumer->failed_&&
         consumer->source_.get()==data->source_identity&&consumer->completed_slot_selectors_==std::vector<int>{-1}&&
         data->operation->bank_operation&&data->operation->bank_operation==warm->owned_skin_effect_bank_operation_&&
         !warm->active_owned_skin_read_operation_&&impl_->default_state->live_enabled89==1&&
         !warm->owned_skin_base_game_consumer_&&!warm->owned_skin_global_kernel_,
         "Installed first660330 has no actual inactive successful SAME Source/kernel return");
    consumer->require_current();data->policy->require_current();
    data->continuation_effects_source=source; // Sole genuine typed mint, after inactive successful660 return.
    data->consumer=consumer;data->native_changed=changed;data->kernel_returned=true;
    data->continuation_consumer=consumer;
    for(const auto& instance:warm->owned_skin_instances_){
        detail::BnwNativeGlobalResourceWarmOwner::InstalledInitialSkinEffects::FinalInstance row;row.instance=instance;row.enabled=instance->enabled_;
        for(unsigned slot=0;slot<2;++slot)for(unsigned member=0;member<2;++member){const auto manager=instance->owned_resource_pairs_[slot][member];
            row.pairs[slot][member]=manager;if(manager)row.current89[slot][member]=manager->current89;}
        data->final_instances.push_back(std::move(row));
    }
    data->returned_stage_count=warm->owned_category_stages_.size();data->returned_output_count=warm->owned_skin_output_receipts_.size();
    data->returned_read_count=warm->owned_skin_read_receipts_.size();require_native_owned_skin_operation_current();
}
void detail::BnwNativeGlobalResourceWarmOwner::drive_installed_first_common_skin() {
    using Parent=BnwNativePackageDiscoveryOwner;
    need(thread_==std::this_thread::get_id(),"Installed first660330 driver foreign creator");
    if(installed_initial_skin_effects_attempted_){failed_.store(true);throw std::runtime_error("Installed first660330 replay/reentry");}
    installed_initial_skin_effects_attempted_=true;
    try {
        ParentPin retained(*this);const auto storage=retained.parent->impl_;auto& i=*storage;
        require_installed_initial_skin_selection_prefix_returned();
        need(!installed_initial_skin_effects_&&installed_initial_skin_selection_->enabled_instances==0&&
             installed_initial_skin_selection_->pending==InstalledInitialSkinSelection::Pending::common660330&&
             owned_skin_boot_policy_&&!owned_skin_effect_source_identity_&&!owned_skin_base_game_consumer_&&!owned_skin_global_kernel_&&
             owned_skin_output_receipts_.empty()&&owned_skin_read_operations_.empty()&&owned_skin_read_receipts_.empty()&&
             !active_owned_skin_read_operation_&&!active_owned_category_stage_&&!active_owned_category_preconstructor_&&
             !i.header_active&&!active_owned_skin_operation_epoch_&&owned_skin_operation_epoch_!=UINT64_MAX,
             "Installed first660330 requires genuine zero-gate/Common pending return before effects");
        const auto data=std::make_shared<InstalledInitialSkinEffects>(i,*this);installed_initial_skin_effects_=data;
        struct Failure {Parent::Impl& i;std::shared_ptr<InstalledInitialSkinEffects> data;bool done=false;
            ~Failure(){if(!done){data->failed=true;i.header_poisoned=true;}}} failure{i,data};
        {
            InstalledInitialSkinEffects::Operation operation(*data,*this,*retained.parent,i);operation.current();
            BnwPrimaryDefaultStageTrace trace("Installed.SkinSelection.firstCommon660330");
            {
                Parent::Impl::HeaderGuard fence(i.header_callback_state,i.self);need(fence.valid(),"Installed first Common actual Header retired/reentered");
                struct Active {Parent::Impl& i;BnwNativeGlobalResourceWarmOwner& source;InstalledInitialSkinEffects::Operation& op;
                    Active(Parent::Impl& actual,BnwNativeGlobalResourceWarmOwner& warm,InstalledInitialSkinEffects::Operation& operation,
                        const Parent::Impl::HeaderGuard& guard):i(actual),source(warm),op(operation){
                        need(!i.header_active&&!i.active_live_query_owner&&!source.active_owned_skin_operation_epoch_,"Installed first Common Header reentry");
                        op.header=&guard;op.serial=++source.owned_skin_operation_epoch_;op.data.returned_epoch=op.serial;
                        source.active_owned_skin_operation_epoch_=op.serial;i.header_active=true;i.active_header_fence=&guard;i.active_live_query_owner=&source;}
                    ~Active(){source.active_owned_skin_operation_epoch_=0;op.header=nullptr;op.serial=0;
                        i.active_live_query_owner=nullptr;i.active_header_fence=nullptr;i.header_active=false;}
                } active{i,*this,operation,fence};
                returned_standalone_packages_.front()->with_actual_descriptor([&](const auto& held){
                    operation.loan=&held;active_owned_skin_package_=held.retained;active_owned_skin_resource_loan_=&held.resource;
                    active_owned_skin_resource_epoch_=held.actor->state_->active_resource_loan;
                    struct LoanExit {BnwNativeGlobalResourceWarmOwner& source;InstalledInitialSkinEffects::Operation& op;
                        ~LoanExit(){source.restore_owned_skin_default_after_failure();op.loan=nullptr;source.active_owned_skin_package_.reset();
                            source.active_owned_skin_resource_loan_=nullptr;source.active_owned_skin_resource_epoch_=0;}} loan_exit{*this,operation};
                    operation.current();const auto bank=data->bank.lock();need(bool(bank),"Installed first Common actual Bank retired");
                    {
                        BnwNativeGlobalColorRegistryBank::OwnedSkinOperation bank_operation(bank);
                        operation.bank_operation=&bank_operation;data->bank_operation_identity=&bank_operation;
                        struct End {InstalledInitialSkinEffects::Operation& op;~End(){op.bank_operation=nullptr;}} end{operation};
                        operation.current();retained.parent->run_installed_first_common_skin_kernel();operation.current();
                        need(data->kernel_returned&&!active_owned_skin_read_operation_&&i.default_state->live_enabled89==1&&
                             !active_owned_category_stage_&&!initializing_owned_namespace_,
                             "Installed first Common kernel left actual slot/Default operation unfinished");
                        for(const auto& read:owned_skin_read_receipts_)retained.parent->require_native_owned_skin_read_receipt_current(read.get());
                        for(const auto& stage:owned_category_stages_)require_installed_initial_skin_effect_stage_current(*stage);
                        operation.current();
                    }
                    need(!bank->active_owned_skin_operation_&&!bank->busy_.load(),"Installed first Common owning Bank Scope did not return");
                    operation.current();
                });operation.current();need(fence.valid(),"Installed first Common whole Loaded POST lost actual Header");
            }
            operation.current();need(data->kernel_returned&&data->consumer&&!operation.header&&!operation.loan&&!operation.bank_operation,
                "Installed first Common lacks actual kernel/target/Loaded/Header normal return");
        }
        need(!data->operation&&!data->selection->continuation&&!active_owned_skin_operation_epoch_&&!active_owned_skin_resource_loan_,
             "Installed first Common actual effect continuation failed to retire");
        require_current();data->returned=true;
        i.frame.next_resource_dependency="Actual first Common660330(-1)/462C20 returned; genuine next6415F0 PC publication, second660330,4691E0 and late Default tail remain mandatory";
        failure.done=true;
    }catch(...){failed_.store(true);throw;}
}
void detail::BnwNativeGlobalResourceWarmOwner::require_installed_first_common_skin_returned()const {
    need(thread_==std::this_thread::get_id(),"Installed first Common normal-return lender foreign creator");
    ParentPin retained(*this);const auto storage=retained.parent->impl_;const auto data=installed_initial_skin_effects_;
    try {
        need(data&&data->returned&&!data->failed&&!data->operation&&!data->continuation&&!data->selection->continuation&&data->kernel_returned&&data->consumer&&
             !storage->header_active&&!active_owned_skin_operation_epoch_&&!active_owned_skin_resource_loan_&&!active_owned_skin_read_operation_&&
             !active_owned_category_stage_&&!initializing_owned_namespace_&&data->returned_epoch==owned_skin_operation_epoch_&&
             data->selection.get()==installed_initial_skin_selection_.get()&&data->selection->prefix_returned&&!data->selection->failed&&
             data->selection->enabled_instances==0&&data->selection->pending==InstalledInitialSkinSelection::Pending::common660330&&
             data->policy.get()==owned_skin_boot_policy_.get()&&data->policy->phase_==BnwNativeOwnedSkinBootPolicy::Phase::common_active&&
             data->policy->installed_setters_returned_==2&&data->policy->installed_constraints_.size()==2&&
             data->policy->installed_constraints_.at("set")=="BaseGame"&&data->policy->installed_constraints_.at("platform")=="Common"&&
             data->policy->actual_platform_.empty()&&data->consumer->thread_==thread_&&!data->consumer->active_&&!data->consumer->failed_&&
             !data->continuation_effects_source.expired()&&
             InstalledInitialSkinEffects::same(data->continuation_effects_source,data->consumer->source_)&&
             data->consumer->source_.get()==data->source_identity&&data->source_identity==owned_skin_effect_source_identity_&&
             data->consumer->completed_slot_selectors_==std::vector<int>{-1}&&data->bank_operation_identity==owned_skin_effect_bank_operation_&&
             owned_skin_effect_operation_epoch_==data->returned_epoch&&storage->default_state->live_enabled89==1&&
             data->returned_stage_count==owned_category_stages_.size()&&data->initial_stage_count<=data->returned_stage_count&&
             data->returned_output_count==owned_skin_output_receipts_.size()&&data->returned_read_count==owned_skin_read_receipts_.size()&&
             data->final_instances.size()==owned_skin_instances_.size()&&!owned_skin_base_game_consumer_&&!owned_skin_global_kernel_,
             "Installed first Common has no genuine closed successful kernel/target/physical operation return");
        const auto bank=data->bank.lock();
        need(bank&&storage->global_color_bank.get()==bank.get()&&owned_bank_receiver_.lock().get()==bank.get()&&
             bank->phase_==BnwNativeGlobalColorRegistryBank::Phase::roster_bound&&!bank->closed_.load()&&!bank->busy_.load()&&
             !bank->active_owned_skin_operation_&&bank->owned_source_.get()==owned_bank_source_identity_&&
             bank->owned_roster_.size()==owned_skin_instances_.size(),"Installed first Common returned Bank/roster changed");
        data->selection->color->admission->identity(*this,*retained.parent,*storage);
        for(std::size_t n=0;n<owned_skin_instances_.size();++n){const auto instance=owned_skin_instances_[n];const auto& row=data->final_instances[n];
            need(instance&&row.instance.lock().get()==instance.get()&&row.enabled==instance->enabled_&&
                 bank->owned_roster_[n].lock().get()==instance.get()&&
                 data->selection->color->constructors[n].source==instance->constructor_source_.get(),
                 "Installed first Common returned actual instance/constructor/enabled fields changed");
            for(unsigned slot=0;slot<2;++slot)for(unsigned member=0;member<2;++member){const auto manager=instance->owned_resource_pairs_[slot][member];
                need(InstalledInitialSkinEffects::same(row.pairs[slot][member],manager)&&row.pairs[slot][member].lock().get()==manager.get()&&
                     (manager?row.current89[slot][member]==manager->current89:!row.current89[slot][member]),
                     "Installed first Common returned actual pair/+89 changed");}
        }
        require_current(); // No retired Consumer/Source/Reader or Source7 frame callback.
    }catch(...){if(data)data->failed=true;failed_.store(true);storage->header_poisoned=true;throw;}
}
void BnwNativePackageDiscoveryOwner::drive_installation_first_common_skin()const {
    const auto storage=impl_;auto& i=*storage;need(i.thread==std::this_thread::get_id(),"Installed first Common Parent foreign creator");
    const auto warm=i.live_resource_query;need(i.installation_input&&!i.lease&&!i.rules&&warm,
        "Installed first Common requires actual installed Input/Warm");
    try{warm->drive_installed_first_common_skin();warm->require_installed_first_common_skin_returned();}
    catch(...){warm->failed_.store(true);i.header_poisoned=true;throw;}
}
