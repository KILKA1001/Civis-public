// Actual628BE0 prefix: read real724CC0 enabled-instance count, publish the
// two Common constraints on zero. The actual first660330 remains pending.
struct detail::BnwNativeGlobalResourceWarmOwner::InstalledInitialSkinSelection final
    :BnwNativeOwnedSkinBootPolicy::InstalledSource {
    using Warm=BnwNativeGlobalResourceWarmOwner;
    using Parent=BnwNativePackageDiscoveryOwner;
    using Policy=BnwNativeOwnedSkinBootPolicy;
    using Input=runtime::BnwSelectedLanguageInputOwner;
    using Boot=BnwNativeResourceBootSettingsOwner;
    using Instance=BnwNativeOwnedUiSkinPoststageInstance;
    const std::thread::id creator=std::this_thread::get_id();
    const std::weak_ptr<const Parent> parent;
    const std::weak_ptr<const void> warm,installation,epoch,boot,language,catalog;
    const std::shared_ptr<const InstalledColorSkinDriver> color;
    bool failed=false,prefix_returned=false,counter_returned=false,policy_construction_active=false;
    enum class Pending {none,common660330,global4691e0};
    Pending pending=Pending::none;
    struct Observation {
        std::weak_ptr<const Instance> instance;
        const void* constructor=nullptr;
        std::array<std::uint8_t,2> enabled{};
    };
    std::vector<Observation> observed;
    std::uint32_t enabled_instances=0;
    std::weak_ptr<Policy> policy;
    // A new actual effect stack may consume the real pending call. It never
    // revives this retired counter-only frame or resets prefix history.
    class Continuation {
        friend class BnwNativeGlobalResourceWarmOwner;
      protected:
        Continuation()=default;
      public:
        virtual ~Continuation()=default;
        Continuation(const Continuation&)=delete;
        virtual void require_actual_inputs_current(const Input&,const Boot&)const=0;
        virtual void with_actual_global_constraint_read(const Policy&,
            const std::function<void(const std::string&,const std::string&)>&)const {
            throw std::runtime_error("Actual global constraint read needs NEW slot-owning continuation");
        }

        virtual void require_actual_initial_constraint_close_current(const Policy&)const {
            throw std::runtime_error("Actual initial constraint close requires its new caller continuation");
        }
        virtual void require_actual_pc_constraint_current(const Policy&)const {
            throw std::runtime_error("Actual PC setter requires new resource continuation");
        }
        virtual void require_actual_pc_selection_return_current(const Policy&)const {
            throw std::runtime_error("Actual second660330 requires new invocation return continuation");
        }
        virtual void require_actual_pc_publication_current(const Policy&)const {
            throw std::runtime_error("Actual PC publication requires its new genuine operation after first Common return");
        }
    };
    const Continuation* continuation=nullptr;
    struct Operation;
    Operation* operation=nullptr;
    InstalledInitialSkinSelection(const Parent::Impl& i,const Warm& source,const std::shared_ptr<const Input>& actual)
        :parent(i.self),warm(i.live_resource_query),installation(i.installation_input),epoch(i.identity),
         boot(i.settings),language(actual),catalog(i.resource_catalog),color(source.installed_color_skin_driver_){}
    static bool same(const auto& a,const auto& b){return !a.owner_before(b)&&!b.owner_before(a);}
    void require_actual_inputs_current(const Input&,const Boot&)const override;
    void require_actual_pc_publication_current(const Policy&)const override;
    void require_actual_pc_constraint_current(const Policy&)const override;
    void require_actual_pc_selection_return_current(const Policy&)const override;
    void require_actual_initial_constraint_close_current(const Policy&)const override;
    void with_actual_global_constraint_read(const Policy&,
        const std::function<void(const std::string&,const std::string&)>&)const override;
};
struct detail::BnwNativeGlobalResourceWarmOwner::InstalledInitialSkinSelection::Operation {
    InstalledInitialSkinSelection& data;
    Warm& source;
    const Parent& parent;
    Parent::Impl& i;
    Operation(InstalledInitialSkinSelection& actual,Warm& owner,const Parent& facade,Parent::Impl& storage)
        :data(actual),source(owner),parent(facade),i(storage){
        need(data.creator==std::this_thread::get_id()&&!data.operation,"Installed628BE0 prefix replay/foreign creator");
        data.operation=this;
    }
    ~Operation(){if(data.operation==this)data.operation=nullptr;else{data.failed=true;i.header_poisoned=true;}}
    Operation(const Operation&)=delete;
    void identity()const {
        need(data.creator==std::this_thread::get_id()&&source.thread_==data.creator&&i.thread==data.creator&&
             !data.failed&&!data.prefix_returned&&!source.closed_.load()&&!source.failed_.load()&&
             !i.header_poisoned&&!i.category_lineage_failed&&data.operation==this,
             "Installed628BE0 prefix actual creator/source/frame retired");
        need(source.installed_initial_skin_selection_.get()==&data&&source.installed_initial_skin_selection_attempted_&&
             i.installation_input&&!i.lease&&!i.rules&&i.live_resource_query.get()==&source&&
             same(i.self,data.parent)&&same(i.live_resource_query,data.warm)&&same(i.installation_input,data.installation)&&
             same(i.identity,data.epoch)&&i.settings&&same(i.settings,data.boot)&&same(i.resource_catalog,data.catalog)&&
             data.color&&data.color.get()==source.installed_color_skin_driver_.get()&&data.color->returned&&!data.color->failed&&
             !data.color->operation&&!data.color->constructor_loan&&!data.color->active_base_read&&
             data.color->phase==InstalledColorSkinDriver::Phase::returned&&data.color->admission&&
             !data.color->admission->rows.empty()&&same(data.language,data.color->admission->rows.front().language)&&
             !i.header_active&&!i.active_live_query_owner&&!i.active_completed_resource_operation&&
             !source.active_owned_skin_operation_epoch_&&!source.active_owned_skin_resource_loan_&&
             !source.active_owned_category_stage_&&!source.active_owned_category_preconstructor_&&!source.busy_.load(),
             "Installed628BE0 prefix lost actual Input/Boot/metadata/52D590 normal-return origin");
        data.color->admission->identity(source,parent,i); // Pure canonical accounted outside-Parent check.
        const auto bank=data.color->bank.lock();
        need(bank&&i.global_color_bank.get()==bank.get()&&source.owned_bank_receiver_.lock().get()==bank.get()&&
             source.owned_bank_source_attachment_returned_&&bank->owned_source_.get()==source.owned_bank_source_identity_&&
             !bank->closed_.load()&&bank->phase_==BnwNativeGlobalColorRegistryBank::Phase::roster_bound&&
             source.owned_skin_instance_ctor_driver_returned_&&source.owned_skin_occurrence_driver_returned_&&
             data.color->constructors.size()==source.owned_skin_instances_.size()&&
             bank->owned_roster_.size()==source.owned_skin_instances_.size(),
             "Installed724CC0 has another actual Bank/constructed native roster");
        if(data.policy_construction_active)need(data.policy.expired()&&!source.owned_skin_boot_policy_,
             "Installed policy constructor escaped actual fresh registration");
        else if(const auto policy=data.policy.lock())need(source.owned_skin_boot_policy_.get()==policy.get()&&
             same(source.owned_skin_boot_policy_,data.policy),"Installed policy has another genuine issuer/control");
        else need(!source.owned_skin_boot_policy_,"Installed policy registration retired/replaced");
    }
    void current()const {identity();source.require_current();identity();}
    std::uint32_t read_actual_enabled_count_724cc0() {
        current();need(!data.counter_returned&&data.observed.empty(),"Installed724CC0 count replay");
        std::uint32_t result=0;
        const auto bank=data.color->bank.lock();need(bool(bank),"Installed724CC0 actual Bank retired");
        need(source.owned_skin_instances_.size()<=UINT32_MAX,"Installed724CC0 actual roster exceeds native counter");
        for(std::size_t n=0;n<source.owned_skin_instances_.size();++n){
            identity();const auto instance=source.owned_skin_instances_[n];const auto& returned=data.color->constructors[n];
            need(instance&&instance->thread_==data.creator&&returned.instance.lock().get()==instance.get()&&
                 bank->owned_roster_[n].lock().get()==instance.get()&&
                 returned.source==instance->constructor_source_.get(),
                 "Installed724CC0 instance lacks SAME actual52D590 constructor/roster source");
            const std::array<std::uint8_t,2> fields{instance->enabled_[0],instance->enabled_[1]};
            data.observed.push_back({instance,instance->constructor_source_.get(),fields});
            if(fields[0]!=0||fields[1]!=0)++result; // Actual+24 OR+25, not total roster length.
            identity();
        }
        current();data.enabled_instances=result;data.counter_returned=true;return result;
    }
};
void detail::BnwNativeGlobalResourceWarmOwner::InstalledInitialSkinSelection::require_actual_inputs_current(
    const Input& actual_input,const Boot& actual_boot)const {
    need(creator==std::this_thread::get_id(),"Installed Skin policy source foreign creator");
    if(continuation){
        need(prefix_returned&&!failed&&counter_returned&&enabled_instances==0&&pending==Pending::common660330,
            "Installed effect continuation lacks actual initial zero-gate/Common pending return");
        continuation->require_actual_inputs_current(actual_input,actual_boot);return;
    }
    need(operation,"Installed Skin policy has no SAME actual prefix/continuation frame");operation->identity();
    const auto captured=language.lock();
    need(counter_returned&&enabled_instances==0&&captured&&captured.get()==&actual_input&&
         operation->i.settings.get()==&actual_boot&&same(operation->i.settings,boot),
         "Installed Skin policy is outside actual724CC0 zero return/Input/Boot controls");
    operation->identity();
}

void detail::BnwNativeGlobalResourceWarmOwner::prepare_installed_initial_skin_selection_prefix() {
    need(thread_==std::this_thread::get_id(),"Installed628BE0 prefix foreign creator");
    if(installed_initial_skin_selection_attempted_){failed_.store(true);throw std::runtime_error("Installed628BE0 prefix replay/reentry");}
    installed_initial_skin_selection_attempted_=true; // Negative occurrence before callbacks/pins/allocation.
    try {
        ParentPin retained(*this);const auto storage=retained.parent->impl_;auto& i=*storage;
        require_installed_color_skin_returned();
        need(!installed_initial_skin_selection_&&!owned_skin_boot_policy_&&i.installation_input&&!i.lease&&!i.rules&&i.settings&&
             !i.header_active&&!active_owned_skin_operation_epoch_&&!active_owned_category_stage_&&
             !i.active_completed_resource_operation,"Installed628BE0 requires real52D590 return before actual counter/constraints");
        const auto actual_input=borrow_selected_language_input();
        const auto data=std::make_shared<InstalledInitialSkinSelection>(i,*this,actual_input);installed_initial_skin_selection_=data;
        struct Failure {BnwNativePackageDiscoveryOwner::Impl& i;std::shared_ptr<InstalledInitialSkinSelection> data;bool done=false;
            ~Failure(){if(!done){data->failed=true;i.header_poisoned=true;}}} failure{i,data};
        {
            InstalledInitialSkinSelection::Operation operation(*data,*this,*retained.parent,i);
            operation.current();BnwPrimaryDefaultStageTrace trace("Installed.SkinSelection.initial628BE0-prefix");
            const auto actual_count=operation.read_actual_enabled_count_724cc0();
            if(actual_count==0){
                data->policy_construction_active=true;
                const auto policy=BnwNativeOwnedSkinBootPolicy::capture_installed(actual_input,i.settings,data);
                data->policy_construction_active=false;data->policy=policy;owned_skin_boot_policy_=policy;
                policy->require_current();policy->publish_installed_common_778e40();policy->require_current();
                data->pending=InstalledInitialSkinSelection::Pending::common660330;
                i.frame.next_resource_dependency="Actual628BE0 zero gate and set=BaseGame/platform=Common returned; mandatory SAME pending first660330(-1) before PC/global/late Default tail";
            }else {
                data->pending=InstalledInitialSkinSelection::Pending::global4691e0;
                i.frame.next_resource_dependency="Actual628BE0 enabled-instance nonzero gate skipped Common/PC; mandatory genuine pending4691E0(-1) source before late Default tail";
            }
            operation.current();
        }
        require_installed_color_skin_returned(); // Fresh FULL POST after actual frame/target destruction.
        need(!data->operation&&data->counter_returned&&data->pending!=InstalledInitialSkinSelection::Pending::none,
             "Installed628BE0 prefix has no actual counter/branch normal return");
        data->prefix_returned=true;failure.done=true;
    }catch(...){failed_.store(true);throw;}
}
void detail::BnwNativeGlobalResourceWarmOwner::require_installed_initial_skin_selection_prefix_returned()const {
    need(thread_==std::this_thread::get_id(),"Installed628BE0 prefix normal-return lender foreign creator");
    ParentPin retained(*this);const auto storage=retained.parent->impl_;const auto data=installed_initial_skin_selection_;
    try {
        need(data&&data->prefix_returned&&!data->failed&&!data->operation&&!data->continuation&&!data->policy_construction_active&&
             data->counter_returned&&data->color.get()==installed_color_skin_driver_.get()&&
             data->observed.size()==owned_skin_instances_.size()&&!storage->header_active&&!active_owned_skin_operation_epoch_,
             "Installed628BE0 prefix has no actual normal counter/branch return");
        require_installed_color_skin_returned();std::uint32_t count=0;
        for(std::size_t n=0;n<data->observed.size();++n){const auto& row=data->observed[n];const auto instance=owned_skin_instances_[n];
            need(instance&&row.instance.lock().get()==instance.get()&&row.constructor==instance->constructor_source_.get()&&
                 row.enabled==std::array<std::uint8_t,2>{instance->enabled_[0],instance->enabled_[1]},
                 "Installed724CC0 observed actual fields/constructor changed before pending call");
            if(row.enabled[0]!=0||row.enabled[1]!=0)++count;
        }
        need(count==data->enabled_instances,"Installed724CC0 actual return changed");
        if(count==0){
            const auto policy=data->policy.lock();
            need(data->pending==InstalledInitialSkinSelection::Pending::common660330&&policy&&
                 owned_skin_boot_policy_.get()==policy.get()&&policy->installed_source_&&
                 policy->installed_source_->lock().get()==static_cast<const BnwNativeOwnedSkinBootPolicy::InstalledSource*>(data.get())&&
                 !policy->installed_source_->owner_before(data)&&!data.owner_before(*policy->installed_source_)&&
                 policy->phase_==BnwNativeOwnedSkinBootPolicy::Phase::common_active&&policy->installed_setters_returned_==2&&
                 policy->installed_constraints_.size()==2&&policy->installed_constraints_.at("set")=="BaseGame"&&
                 policy->installed_constraints_.at("platform")=="Common"&&policy->actual_platform_.empty()&&policy->lease_.expired(),
                 "Installed Common pending call lost actual two native constraint writes/source");
        }else need(data->pending==InstalledInitialSkinSelection::Pending::global4691e0&&data->policy.expired()&&!owned_skin_boot_policy_,
             "Installed nonzero gate fabricated Common/PC constraint writes");
        require_current();
    }catch(...){if(data)data->failed=true;failed_.store(true);storage->header_poisoned=true;throw;}
}
void BnwNativePackageDiscoveryOwner::advance_installation_initial_skin_selection_prefix()const {
    const auto storage=impl_;auto& i=*storage;
    need(i.thread==std::this_thread::get_id(),"Installed628BE0 prefix Parent foreign creator");
    const auto warm=i.live_resource_query;need(i.installation_input&&!i.lease&&!i.rules&&warm,
        "Installed628BE0 prefix requires actual installed Input/Warm");
    try{warm->prepare_installed_initial_skin_selection_prefix();warm->require_installed_initial_skin_selection_prefix_returned();}
    catch(...){warm->failed_.store(true);i.header_poisoned=true;throw;}
}
