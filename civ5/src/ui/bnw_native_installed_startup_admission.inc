// Actual initial admission after returned installed Common and SAME adoption.
// It is independent of the Civis selected-save configured-activation policy.
namespace installed_startup_admission_detail {
struct Probe {
    std::string selector,output;
    std::filesystem::path io;
    DWORD attributes=INVALID_FILE_ATTRIBUTES,error=ERROR_SUCCESS;
    bool exists=false;
};
std::filesystem::path confined_path(const std::filesystem::path& root,const std::string& output) {
    need(!output.empty(),"Initial declared output unexpectedly empty after actual output producer");
    const std::filesystem::path path(native_loose_detail::acp_wide(output));
    need(!path.has_root_name()||path.is_absolute(),"Initial declared drive-relative path requires actual drive source");
    const auto selected=(path.is_absolute()?path:root/path).lexically_normal();
    const auto relative=selected.lexically_relative(root);
    need(!relative.empty()&&!relative.is_absolute()&&*relative.begin()!=std::filesystem::path(".."),
         "Initial declared path requires its actual source outside qualified installation");
    // Finite port source confinement, not a claim that Original rejects a
    // reparse point. Missing descendants still reach the actual OS probe.
    for(auto parent=selected.parent_path();parent!=root;parent=parent.parent_path()) {
        need(!parent.empty(),"Initial declared path lost original installation ancestor");
        SetLastError(ERROR_SUCCESS);
        const auto attributes=GetFileAttributesA(native_loose_detail::acp_path(parent).c_str());
        const auto error=GetLastError();
        if(attributes!=INVALID_FILE_ATTRIBUTES)
            need(!(attributes&FILE_ATTRIBUTE_REPARSE_POINT),"Initial declared reparse parent requires target source owner");
        else need(error==ERROR_FILE_NOT_FOUND||error==ERROR_PATH_NOT_FOUND,
                  "Initial declared ancestor failure requires actual source owner");
    }
    return selected;
}
Probe probe(const char* selector,const std::string& output,const std::filesystem::path& io) {
    Probe result{selector,output,io};SetLastError(ERROR_SUCCESS);
    result.attributes=GetFileAttributesA(native_loose_detail::acp_path(io).c_str());
    result.exists=result.attributes!=INVALID_FILE_ATTRIBUTES;
    result.error=result.exists?ERROR_SUCCESS:GetLastError();
    return result; // Native829560 accepts any valid attributes, even directory.
}
bool same_probe(const Probe& a,const Probe& b) {
    return a.io==b.io&&a.attributes==b.attributes&&a.error==b.error&&a.exists==b.exists;
}
}

struct detail::BnwNativeGlobalResourceWarmOwner::InstalledStartupAdmission {
    using Warm=BnwNativeGlobalResourceWarmOwner;
    using Parent=BnwNativePackageDiscoveryOwner;
    using Loaded=BnwNativeStandalonePackageLoaded;
    using Fields=BnwNativePackageDescriptorCoreWriter::Fields;
    const std::thread::id creator=std::this_thread::get_id();
    const std::weak_ptr<const Parent> parent;
    const std::weak_ptr<const void> warm,epoch,input,initial,common;
    const assets::ResourceMountTable* const table;
    const std::filesystem::path root;
    const UINT code_page=GetACP();
    const std::uint64_t initial_revision;
    bool busy=true,failed=false,returned=false;
    struct Row {
        std::size_t ordinal=0;
        std::weak_ptr<const void> package,actor,core,parser,reader,stream,filename,discovery,language;
        const Fields* fields=nullptr;
        const BnwNativePackageXmlParserOwner::Document* document=nullptr;
        const BnwNativePackageDescriptorCoreWriter::Metadata* metadata=nullptr;
        std::uint32_t expected_flags=0,original_mask=0;
        bool valid=false,failed_zero_pair_return=false;
        std::vector<std::pair<std::string,int>> actual_queries;
        std::vector<installed_startup_admission_detail::Probe> probes;
    };
    std::vector<Row> rows;
    struct Tablet {std::size_t ordinal;const Fields* fields;std::uint32_t before,after;};
    std::optional<Tablet> tablet;
    InstalledStartupAdmission(const Parent::Impl& i)
        :parent(i.self),warm(i.live_resource_query),epoch(i.identity),input(i.installation_input),
         initial(i.pre_dx9_discovery),common(i.installed_default_common),table(&i.resources),
         root(i.original.parent_path()),initial_revision(i.registry.revision){}
    static bool same(const auto& value,const auto& control) {
        return !value.owner_before(control)&&!control.owner_before(value);
    }
    void identity(const Warm& source,const Parent& actual,const Parent::Impl& i)const {
        need(creator==std::this_thread::get_id()&&i.thread==creator&&source.thread_==creator&&
             !failed&&!source.closed_.load()&&!source.failed_.load()&&!i.header_poisoned&&!i.category_lineage_failed,
             "Installed admission actual creator/source retired");
        need(source.installed_startup_admission_.get()==this&&source.installed_startup_admission_attempted_&&
             i.installation_input&&!i.lease&&!i.rules&&same(i.self,parent)&&
             i.live_resource_query.get()==&source&&same(i.live_resource_query,warm)&&same(i.identity,epoch)&&
             same(i.installation_input,input)&&i.pre_dx9_discovery&&same(i.pre_dx9_discovery,initial)&&
             source.pre_dx9_owner_.get()==i.pre_dx9_discovery.get()&&same(source.pre_dx9_owner_,initial)&&
             i.installed_default_common&&same(i.installed_default_common,common)&&
             source.installed_common_source_.get()==i.installed_default_common.get()&&same(source.installed_common_source_,common)&&
             &i.resources==table&&GetACP()==code_page&&i.registry.revision>=initial_revision&&
             source.early_receiver_driver_returned_&&!source.configured_activation_policy_returned_&&
             source.returned_standalone_packages_.size()==source.preload_receivers_.size()&&
             source.descriptor_sources_.size()==source.preload_receivers_.size(),
             "Installed admission lost SAME actual Parent/Common/pre-DX9 adopted roster");
        actual.require_pre_dx9_discovery_current(*i.pre_dx9_discovery); // Canonical accounted outside-parent test.
    }
    static Row capture(std::size_t ordinal,const Loaded& token,const Loaded::Loan& loan) {
        Row row;row.ordinal=ordinal;row.package=loan.retained;row.actor=loan.actor;
        const auto& load=loan.actor->state_;
        row.core=load->descriptor_core;row.parser=load->parsed_xml;row.reader=load->descriptor_bytes;
        row.stream=load->descriptor_bytes->state_->source;row.filename=load->filename;
        row.discovery=load->discovery;row.language=load->metadata_input;
        row.fields=&loan.fields();row.document=&loan.document();row.metadata=token.actual_metadata_;
        row.expected_flags=row.fields->current_flags();row.original_mask=row.fields->current_category_mask();
        return row;
    }
    void authenticate(const Warm& source,const Loaded& token,const Loaded::Loan& loan,const Row& row)const {
        need(creator==std::this_thread::get_id()&&loan.creator_thread==creator&&
             !loan.completed_source&&!loan.completed_cold&&&loan.token==&token&&
             loan.retained.get()==&token&&same(loan.retained,row.package)&&loan.actor&&same(loan.actor,row.actor)&&
             row.ordinal<source.returned_standalone_packages_.size()&&
             source.returned_standalone_packages_[row.ordinal].get()==&token&&
             same(source.returned_standalone_packages_[row.ordinal],row.package)&&
             row.ordinal<source.preload_receivers_.size()&&same(source.preload_receivers_[row.ordinal],row.actor),
             "Installed admission escaped SAME held actual returned Loaded/actor/ordinal");
        const auto& load=loan.actor->state_;
        need(load->busy.load()&&load->active_resource_loan&&load->active_resource_loan_address==&loan.resource&&
             same(loan.resource.state,load)&&!loan.resource.completed_source&&!loan.resource.completed_cold&&
             !load->actual_lookup_source&&load->pre_dx9_lookup_source&&load->adopted_warm&&
             same(*load->pre_dx9_lookup_source,initial)&&same(*load->adopted_warm,warm)&&
             load->matches_warm_consumer_without_callbacks(warm)&&
             same(load->descriptor_core,row.core)&&same(load->parsed_xml,row.parser)&&
             same(load->descriptor_bytes,row.reader)&&same(load->filename,row.filename)&&
             same(load->discovery,row.discovery)&&same(load->metadata_input,row.language)&&
             same(load->descriptor_bytes->state_->source,row.stream)&&
             load->receiver.get()==token.actual_receiver_&&load->completed_metadata_identity==row.metadata&&
             same(load->returned_loaded,row.package)&&load->descriptor_core->state_->fields.get()==row.fields&&
             &loan.fields()==row.fields&&&loan.document()==row.document&&
             load->descriptor_core->state_->metadata.get()==row.metadata&&
             row.fields->current_flags()==row.expected_flags&&row.fields->current_category_mask()==row.original_mask,
             "Installed admission substituted actual pre-DX9 ResourceLoan/Core/Parser/Reader/flags");
    }
};

void detail::BnwNativeGlobalResourceWarmOwner::prepare_installed_startup_admission() {
    need(thread_==std::this_thread::get_id(),"Installed startup admission foreign creator");
    if(installed_startup_admission_attempted_){failed_.store(true);throw std::runtime_error("Installed startup admission attempt replay/reentry");}
    installed_startup_admission_attempted_=true; // Before first callback/Parent pin/issuer allocation.
    try {
        ParentPin retained(*this);const auto storage=retained.parent->impl_;auto& i=*storage;
        need(i.installation_input&&!i.lease&&!i.rules&&!i.header_active&&pre_dx9_owner_&&installed_common_source_&&
             early_receiver_driver_returned_&&!configured_activation_policy_returned_&&owned_activation_receipts_.empty()&&
             owned_category_stages_.empty()&&!active_owned_category_stage_&&!active_owned_skin_operation_epoch_&&
             !i.category_source_attempted&&!i.category_source&&!i.global_color_bank&&!installed_startup_admission_,
             "Installed startup admission requires returned SAME Common/Warm/adoption before any category producer");
        const auto data=std::make_shared<InstalledStartupAdmission>(i);installed_startup_admission_=data;
        struct Exit {std::shared_ptr<InstalledStartupAdmission> data;~Exit(){data->busy=false;}} exit{data};
        struct Failure {BnwNativePackageDiscoveryOwner::Impl& i;std::shared_ptr<InstalledStartupAdmission> data;bool complete=false;
            ~Failure(){if(!complete){data->failed=true;i.header_poisoned=true;}}} failure{i,data};
        const auto current=[&]{data->identity(*this,*retained.parent,i);require_current();data->identity(*this,*retained.parent,i);};
        current();BnwPrimaryDefaultStageTrace trace("Installed.StartupAdmission");
        data->rows.reserve(returned_standalone_packages_.size());
        std::array<std::uint8_t,16> tablet_guid{};
        need(BnwNativePackageDescriptorCoreWriter::guid("8871e748-29a4-4910-8c57-8c99e32d0167",tablet_guid),
             "Original fixed Tablet GUID arithmetic unavailable");
        // Exact first matching actual GUID search, before ALL686D80 decisions.
        for(std::size_t ordinal=0;ordinal<returned_standalone_packages_.size()&&!data->tablet;++ordinal) {
            const auto package=returned_standalone_packages_[ordinal];require_returned_standalone_package_loaded_current(*package);current();
            package->with_actual_descriptor([&](const auto& loan){
                loan.require_current();current();auto row=InstalledStartupAdmission::capture(ordinal,*package,loan);
                data->authenticate(*this,*package,loan,row);
                need(std::none_of(loan.actor->state_->category_slots.begin(),loan.actor->state_->category_slots.end(),[](const auto& slot){return bool(slot);}),
                     "Initial Tablet admission encountered existing category pair");
                if(!(row.fields->current_flags()&0x40U)&&row.fields->written_guid&&*row.fields->written_guid==tablet_guid) {
                    const auto before=row.expected_flags;const auto after=before|0x20U;
                    loan.actor->state_->descriptor_core->state_->fields->startup_live_flags_=after;
                    row.expected_flags=after;data->tablet=InstalledStartupAdmission::Tablet{ordinal,row.fields,before,after};
                }
                loan.require_current();current();data->authenticate(*this,*package,loan,row);
            });current();
        }
        for(std::size_t ordinal=0;ordinal<returned_standalone_packages_.size();++ordinal) {
            const auto package=returned_standalone_packages_[ordinal];require_returned_standalone_package_loaded_current(*package);current();
            package->with_actual_descriptor([&](const auto& loan){
                loan.require_current();current();auto row=InstalledStartupAdmission::capture(ordinal,*package,loan);
                const auto check=[&]{loan.require_current();current();data->authenticate(*this,*package,loan,row);};
                check();need(row.original_mask==0xffffffffU&&
                     std::none_of(loan.actor->state_->category_slots.begin(),loan.actor->state_->category_slots.end(),[](const auto& slot){return bool(slot);}),
                     "Initial686D80 requires real untouched constructor mask and null pairs");
                bool valid=true;
                for(const auto* selector:{"GameCore","Paks"}) {
                    if(!valid)break; // Original does not query Paks after GameCore failure.
                    BnwNativePackageResourceCheckpoint::with_initial_all_outputs_54b8d0(loan.actual_resource_loan(),selector,
                        [&](int raw_count,const std::vector<std::string>& outputs){
                            check();row.actual_queries.emplace_back(selector,raw_count);
                            for(const auto& output:outputs) {
                                const auto declared=installed_startup_admission_detail::confined_path(data->root,output);check();
                                auto observed=installed_startup_admission_detail::probe(selector,output,declared);row.probes.push_back(observed);check();
                                if(observed.exists)continue;
                                if(runtime::ascii_lower(assets::path_utf8(declared.extension()))==".fpk") {
                                    for(const auto* prefix:{"Resource/DX9","Resource/Common"}) {
                                        const auto fallback=data->root/prefix/(declared.stem().native()+std::filesystem::path(".fpk").native());
                                        const auto confined=installed_startup_admission_detail::confined_path(data->root,native_loose_detail::acp_path(fallback));check();
                                        observed=installed_startup_admission_detail::probe(selector,output,confined);row.probes.push_back(observed);check();
                                        if(observed.exists)break;
                                    }
                                }
                                if(!observed.exists){valid=false;break;}
                            }
                            check();
                        });check();
                }
                row.valid=valid;
                if(!valid) {
                    // Actual +44|40, then real505A90(-1) zero branch on SAME
                    // constructor slots. A nonnull pair needs its real writer.
                    row.expected_flags|=0x40U;loan.actor->state_->descriptor_core->state_->fields->startup_live_flags_=row.expected_flags;
                    check();need(std::none_of(loan.actor->state_->category_slots.begin(),loan.actor->state_->category_slots.end(),[](const auto& slot){return bool(slot);}),
                         "Mandatory actual505A90 existing-pair disable source before failed admission return");
                    row.failed_zero_pair_return=true;check();
                }
                data->rows.push_back(std::move(row));loan.require_current();current();
            });current();
        }
        need(data->rows.size()==returned_standalone_packages_.size(),"Installed admission actual decision roster incomplete");
        // Full actual POST once: fields/controls and every actual probe. These
        // are retained admission results, not a claim that every row is valid.
        for(const auto& row:data->rows) {
            const auto package=returned_standalone_packages_[row.ordinal];
            package->with_actual_descriptor([&](const auto& loan){loan.require_current();current();data->authenticate(*this,*package,loan,row);
                need(row.valid||row.failed_zero_pair_return,"Failed installed admission omitted actual disable return");
                for(const auto& before:row.probes){const auto after=installed_startup_admission_detail::probe(before.selector.c_str(),before.output,before.io);
                    current();need(installed_startup_admission_detail::same_probe(before,after),"Installed admission actual OS result changed before return");}
                loan.require_current();current();data->authenticate(*this,*package,loan,row);
            });current();
        }
        i.frame.next_resource_dependency="Mandatory actual656440 per-package category0 then1 constructors/effects before52D590; installed admission alone is not category/Skin readiness";
        current();data->returned=true;failure.complete=true; // No callback/allocation after actual normal-return marker.
    }catch(...){failed_.store(true);throw;}
}

void detail::BnwNativeGlobalResourceWarmOwner::require_installed_startup_admission_returned()const {
    need(thread_==std::this_thread::get_id(),"Installed admission borrow foreign creator");
    ParentPin retained(*this);const auto storage=retained.parent->impl_;
    const auto data=installed_startup_admission_;
    const auto warm_keeper=storage->live_resource_query; // Issuer storage, never a Parent facade pin.
    try {
    need(data&&data->returned&&!data->busy&&!data->failed,"Installed admission has no actual normal return");
    data->identity(*this,*retained.parent,*storage);require_current();data->identity(*this,*retained.parent,*storage);
    for(const auto& row:data->rows) {
        const auto package=returned_standalone_packages_[row.ordinal];
        package->with_actual_descriptor([&](const auto& loan){loan.require_current();data->authenticate(*this,*package,loan,row);loan.require_current();});
        data->identity(*this,*retained.parent,*storage);
    }
    require_current();data->identity(*this,*retained.parent,*storage);
    }catch(...){if(data)data->failed=true;failed_.store(true);storage->header_poisoned=true;throw;}
}

void BnwNativePackageDiscoveryOwner::advance_installation_startup_admission()const {
    const auto storage=impl_;auto& i=*storage;
    need(i.thread==std::this_thread::get_id(),"Installed startup admission entry foreign creator");
    const auto warm=i.live_resource_query;
    need(i.installation_input&&!i.lease&&!i.rules&&warm&&i.installed_default_common,
         "Installed startup admission requires genuine installed returned Common issuer");
    try {warm->prepare_installed_startup_admission();warm->require_installed_startup_admission_returned();}
    catch(...){warm->failed_.store(true);i.header_poisoned=true;throw;}
}
