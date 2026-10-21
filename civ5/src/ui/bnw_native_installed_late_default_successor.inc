// Included only by PackageDiscoveryOwner, after the genuine installed
// Default/Common publication source. No public producer/constructor exists.
struct BnwNativePackageDiscoveryOwner::Impl::InstalledLateDefaultSuccessor {
    using Op=detail::BnwNativeInstalledLateDefaultOperation;
    using Attempt=detail::BnwNativeInstalledLateDefaultArchiveAttempt;
    using Record=decltype(DefaultRecords{}.records)::mapped_type;
    const std::thread::id thread=std::this_thread::get_id();
    const std::weak_ptr<const BnwNativePackageDiscoveryOwner> parent;
    const std::weak_ptr<const detail::BnwNativeGlobalResourceWarmOwner> warm;
    const std::shared_ptr<const detail::BnwNativeInstalledDefaultCommonSource> origin;
    const std::shared_ptr<const BnwNativeInstallationInputOwner> input;
    const std::shared_ptr<const void> epoch,brand;
    const DefaultState* const actual_default;
    const assets::ResourceMountTable* const table;
    const detail::BnwNativeCurrentDefaultSource* const default_source;
    const detail::BnwNativeColdFilesystemOwner* const cold;
    const detail::BnwNativeArchiveEntryAdmissionArena* const arena;
    const detail::BnwNativeArchiveBasenameCacheOwner* const cache;
    const detail::BnwNativeGlobalArchivePrefixOwner* const prefixes;
    const detail::BnwNativeSharedFirstLineage* const first;
    const std::map<std::uint32_t,std::shared_ptr<DefaultLooseNativeNode>> original_loose;
    using OriginBinding=detail::BnwNativeInstalledDefaultCommonSource::Impl::Binding;
    const std::map<std::uint32_t,const OriginBinding*> original_rows;
    enum class Phase { speech,returned,failed };
    Phase phase=Phase::speech;
    const detail::BnwNativeInstalledLateSpeechCoreDonor* speech=nullptr;
    const Op* active_operation=nullptr;
    bool directory_entered=false;
    struct ArchiveEffect {
        enum class Phase { issued,request,lookup,construction,header,prefix,entries,returned };
        const std::shared_ptr<const Attempt> attempt;
        Phase phase=Phase::issued;
        std::shared_ptr<const detail::BnwNativeColdArchiveRequest> request;
        std::shared_ptr<const detail::BnwNativeArchiveCacheLookupOwner> lookup;
        std::shared_ptr<const detail::BnwNativeArchiveConstruction> construction;
        std::shared_ptr<const detail::BnwNativeArchiveV6StreamHeaderOwner> header;
        std::shared_ptr<const detail::BnwNativeGlobalArchivePrefixRecord> prefix;
        std::shared_ptr<const detail::BnwNativeArchiveV6EntryAdmissionOwner> entries;
        std::shared_ptr<const detail::BnwNativeArchiveCachePublicationOwner> publication;
        std::optional<std::uint32_t> loader_return;
        explicit ArchiveEffect(std::shared_ptr<const Attempt> actual):attempt(std::move(actual)){}
    };
    struct DirectoryEffects {
        const void* const core_identity;
        const std::shared_ptr<const void> core_brand;
        const std::string query;
        const std::size_t root_ordinal;
        std::vector<LooseWalk> walks;
        std::vector<Stamp> stamps;
        std::vector<std::filesystem::path> absent;
        std::vector<std::shared_ptr<const detail::BnwNativeOriginalStatRead>> loose_stats;
        std::vector<std::uint32_t> loose_returns;
        std::vector<std::unique_ptr<ArchiveEffect>> archives;
        std::map<std::string,std::pair<std::uint32_t,std::u16string>> folder_prefixes;
        std::vector<native_loose_detail::LooseEntry> top_fpk_entries;
        DWORD top_fpk_end_error=ERROR_SUCCESS;
        ArchiveEffect* active_archive=nullptr;
        bool root_appended=false,loose_returned=false,archives_returned=false,returned=false;
        DirectoryEffects(const void* core,std::shared_ptr<const void> actual_brand,
            std::string actual_query,std::size_t ordinal)
            :core_identity(core),core_brand(std::move(actual_brand)),query(std::move(actual_query)),root_ordinal(ordinal){}
    };
    std::vector<std::unique_ptr<DirectoryEffects>> directories;
    std::size_t attempts=0,expected_rows=0,expected_roots=0;
    // Only the exact next owned store may be pending. This is a mutation fence,
    // never a cached currentness verdict; a caught failure cannot clear it.
    bool store_pending=false;
    std::uint32_t pending_crc=0;
    bool pending_existed=false;
    struct SelectedEffect {
        const std::uint32_t crc;
        const Record* const actual;
        const Record value;
        const std::string name,first_name;
        const std::shared_ptr<DefaultLooseNativeNode> loose;
        const std::size_t directory;
        SelectedEffect(std::uint32_t key,const Record* row,Record after,std::string actual_name,
            std::string first,std::shared_ptr<DefaultLooseNativeNode> node,std::size_t occurrence)
            :crc(key),actual(row),value(std::move(after)),name(std::move(actual_name)),first_name(std::move(first)),
             loose(std::move(node)),directory(occurrence){}
    };
    std::vector<std::unique_ptr<const SelectedEffect>> effects;
    std::map<std::uint32_t,std::size_t> last_effect;
    InstalledLateDefaultSuccessor(const Impl& i,const detail::BnwNativeInstalledLateSpeechCoreDonor& donor)
        :parent(i.self),warm(i.live_resource_query),origin(i.installed_default_common),input(i.installation_input),
         epoch(i.identity),brand(std::make_shared<const std::uint8_t>(0)),actual_default(i.default_state.get()),
         table(&i.resources),default_source(i.current_default_source.get()),cold(i.cold.get()),arena(i.entry_arena.get()),
         cache(i.archive_cache.get()),prefixes(i.prefixes.get()),first(i.first_lineage.get()),original_loose(i.default_state->loose_native),
         original_rows([&]{std::map<std::uint32_t,const OriginBinding*> rows;
             for(const auto& binding:origin->impl_->selected_bindings)
                 need(rows.emplace(binding.key,&binding).second,"Installed late Default original binding CRC duplicated");
             return rows;}()),speech(&donor),
         expected_rows(i.default_state->selected.records.size()){}
};

void BnwNativePackageDiscoveryOwner::Impl::fail_installed_late_default_without_callbacks()noexcept {
    if(installed_late_default)installed_late_default->phase=InstalledLateDefaultSuccessor::Phase::failed;
    header_poisoned=true;
}

struct detail::BnwNativeInstalledLateDefaultOperation::Impl {
    const BnwNativePackageDiscoveryOwner& parent;
    const std::shared_ptr<BnwNativePackageDiscoveryOwner::Impl> storage;
    const BnwNativeInstalledLateSpeechCoreDonor& donor;
    const std::shared_ptr<BnwNativePackageDiscoveryOwner::Impl::InstalledLateDefaultSuccessor> source;
    BnwNativePackageDiscoveryOwner::Impl::InstalledLateDefaultSuccessor::DirectoryEffects& directory;
    const BnwNativePackageDiscoveryOwner::Impl::HeaderGuard* const fence;
    bool complete=false;
    Impl(const BnwNativePackageDiscoveryOwner& actual,const BnwNativeInstalledLateSpeechCoreDonor& physical,
        BnwNativePackageDiscoveryOwner::Impl::InstalledLateDefaultSuccessor::DirectoryEffects& effects)
        :parent(actual),storage(actual.impl_),donor(physical),source(storage->installed_late_default),
         directory(effects),fence(storage->active_header_fence){}
};

namespace installed_late_default_detail {
// Diagnostic only: neither identity/current methods nor any writer reads it.
inline thread_local const void* observed_source=nullptr;
inline thread_local unsigned observed_phase=0;
struct Observation {
    const void* previous_source=observed_source;
    unsigned previous_phase=observed_phase;
    Observation(const void* actual,unsigned phase)noexcept{observed_source=actual;observed_phase=phase;}
    ~Observation(){observed_phase=previous_phase;observed_source=previous_source;}
    void phase(unsigned value)noexcept{observed_phase=value;}
};
inline bool same_handle(const assets::AssetHandle& a,const assets::AssetHandle& b) {
        const auto& p=a.candidate();const auto& q=b.candidate();
        return p.original_name==q.original_name&&p.normalized_name==q.normalized_name&&p.payload_size==q.payload_size&&
            p.provenance.kind==q.provenance.kind&&p.provenance.source_id==q.provenance.source_id&&
            p.provenance.source_relative_path==q.provenance.source_relative_path&&
            p.provenance.archive_relative_path==q.provenance.archive_relative_path&&
            a.requested_key().original_name()==b.requested_key().original_name()&&
            a.requested_key().normalized_name()==b.requested_key().normalized_name();
}
template<class Record> bool same_record(const Record& a,const Record& b) {
    if(a.index()!=b.index())return false;
    if(const auto* x=std::get_if<assets::AssetHandle>(&a))return same_handle(*x,std::get<assets::AssetHandle>(b));
    const auto& p=std::get<UnreadableLoose>(a);const auto& q=std::get<UnreadableLoose>(b);
    return p.native_path==q.native_path&&p.relative==q.relative&&p.error==q.error&&
        p.failed_original_stat.get()==q.failed_original_stat.get()&&
        !p.failed_original_stat.owner_before(q.failed_original_stat)&&!q.failed_original_stat.owner_before(p.failed_original_stat)&&
        p.receipt.relative_path==q.receipt.relative_path&&p.receipt.native_basename==q.receipt.native_basename&&
        p.receipt.bytes==q.receipt.bytes&&p.receipt.last_write_time_raw==q.receipt.last_write_time_raw&&
        p.receipt.attributes==q.receipt.attributes&&p.receipt.open_error==q.receipt.open_error;
}
}

unsigned bnw_native_installed_late_default_phase_for_test(const BnwNativePackageDiscoveryOwner& parent)noexcept {
    return parent.impl_->thread==std::this_thread::get_id()&&parent.impl_->installed_late_default.get()==
        installed_late_default_detail::observed_source?installed_late_default_detail::observed_phase:0;
}
bool bnw_native_installed_late_default_observer_active_for_test()noexcept {
    return installed_late_default_detail::observed_source!=nullptr||installed_late_default_detail::observed_phase!=0;
}

void BnwNativePackageDiscoveryOwner::require_installed_late_default_identity_without_callbacks()const {
    const auto& i=*impl_;need(i.thread==std::this_thread::get_id(),"Installed late Default foreign creator");
    const auto source=i.installed_late_default;
    need(source&&source->thread==i.thread&&source->phase!=Impl::InstalledLateDefaultSuccessor::Phase::failed&&
         !i.header_poisoned&&!i.category_lineage_failed&&i.installation_input&&!i.lease&&!i.rules&&
         !source->parent.owner_before(i.self)&&!i.self.owner_before(source->parent)&&
         source->origin.get()==i.installed_default_common.get()&&source->input.get()==i.installation_input.get()&&
         source->epoch.get()==i.identity.get()&&source->actual_default==i.default_state.get()&&source->table==&i.resources&&
         source->default_source==i.current_default_source.get()&&source->cold==i.cold.get()&&
         source->arena==i.entry_arena.get()&&source->cache==i.archive_cache.get()&&source->prefixes==i.prefixes.get()&&
         source->first==i.first_lineage.get()&&
         i.default_state->selected.source_first_.get()==i.first_lineage.get()&&
         i.default_state->speech_roots6c.size()==source->expected_roots,
         "Installed late Default lost SAME source/Default/Arena/cache/FIRST controls");
    // Immutable original controls and history only. It deliberately does not
    // call successor/selected extent, Arena or Attempt, so this seam is acyclic.
    require_installed_default_common_origin_without_callbacks(*source->origin);
    const auto warm=source->warm.lock();
    need(warm&&i.live_resource_query.get()==warm.get()&&!warm->closed_.load()&&!warm->failed_.load()&&
         !warm->parent_.owner_before(i.self)&&!i.self.owner_before(warm->parent_)&&i.pre_dx9_discovery,
         "Installed late Default actual Warm/Parent retired");
    require_pre_dx9_discovery_current(*i.pre_dx9_discovery);
    const auto actual_rows=source->expected_rows+
        (source->store_pending&&!source->pending_existed&&i.default_state->selected.records.contains(source->pending_crc)?1u:0u);
    need(actual_rows==i.default_state->selected.records.size()&&
         i.default_state->selected.record_names.size()==actual_rows&&
         i.default_state->selected.bucket_order.live_nodes()==actual_rows,
         "Installed late Default changed rows outside its genuine writer");
    if(source->phase==Impl::InstalledLateDefaultSuccessor::Phase::speech){
        need(source->speech,"Installed late Default lost its actual Speech operation");
        source->speech->require_operation_identity_without_callbacks(*this);
    }else need(!source->speech&&!source->active_operation&&!source->directory_entered&&!source->store_pending,
         "Installed late Default retained a departed physical operation");
}

void BnwNativePackageDiscoveryOwner::validate_installed_late_default_namespace_without_callbacks()const {
    require_installed_late_default_observation_without_callbacks();
    const auto& i=*impl_;const auto& source=*i.installed_late_default;
    need(!source.store_pending,"Installed late Default selected store did not complete");
    const auto& original=*source.origin->impl_;
    const auto check_row=[&](std::uint32_t crc,const auto* expected,const auto& value){
        const auto found=i.default_state->selected.records.find(crc);
        need(found!=i.default_state->selected.records.end()&&&found->second==expected&&
             installed_late_default_detail::same_record(found->second,value),
             "Installed late Default row has no unchanged origin or actual writer effect");
    };
    for(const auto& binding:original.selected_bindings)if(!source.last_effect.contains(binding.key)){
        check_row(binding.key,binding.actual,binding.returned_value);
        const auto before=original.returned_names.find(binding.key);const auto now=i.default_state->selected.record_names.find(binding.key);
        need(before!=original.returned_names.end()&&now!=i.default_state->selected.record_names.end()&&now->second==before->second,
             "Installed late Default changed an unchanged original record name");
    }
    for(const auto& [crc,node]:source.original_loose)if(!source.last_effect.contains(crc)){
        const auto row=i.default_state->selected.records.find(crc);const auto now=i.default_state->loose_native.find(crc);
        need(row!=i.default_state->selected.records.end()&&now!=i.default_state->loose_native.end()&&node&&
             now->second.get()==node.get()&&node->selected_entry_==&row->second,
             "Installed late Default changed an unchanged original loose witness");
    }
    const auto& first=i.first_lineage->storage_.names();
    for(const auto& [crc,name]:original.returned_first){const auto found=first.find(crc);
        need(found!=first.end()&&found->second==name,"Installed late Default overwrote original global FIRST history");}
    for(const auto& [crc,index]:source.last_effect){
        need(index<source.effects.size()&&source.effects[index]&&source.effects[index]->crc==crc,
             "Installed late Default selected effect ledger foreign");
        const auto& effect=*source.effects[index];check_row(crc,effect.actual,effect.value);
        const auto name=i.default_state->selected.record_names.find(crc);const auto inherited=first.find(crc);
        need(name!=i.default_state->selected.record_names.end()&&name->second==effect.name&&
             inherited!=first.end()&&inherited->second==effect.first_name,
             "Installed late Default changed selected name/global FIRST outside actual store");
        const auto loose=i.default_state->loose_native.find(crc);
        need(effect.loose?(loose!=i.default_state->loose_native.end()&&loose->second.get()==effect.loose.get()&&
                          loose->second->selected_entry_==effect.actual):loose==i.default_state->loose_native.end(),
             "Installed late Default selected loose/archive witness changed");
    }
    std::size_t original_rows=0;
    for(const auto& binding:original.selected_bindings)if(!source.last_effect.contains(binding.key))++original_rows;
    need(original_rows+source.last_effect.size()==source.expected_rows,
         "Installed late Default contains a row with no actual producer");
}

void BnwNativePackageDiscoveryOwner::require_installed_late_default_observation_without_callbacks()const {
    require_installed_late_default_identity_without_callbacks();
    need(!impl_->installed_late_default->store_pending,
         "Installed late Default observation during unfinished selected store");
}

void BnwNativePackageDiscoveryOwner::require_installed_late_default_selected_crc_without_callbacks(std::uint32_t crc)const {
    require_installed_late_default_observation_without_callbacks();const auto& i=*impl_;const auto& source=*i.installed_late_default;
    const auto row=i.default_state->selected.records.find(crc);
    const auto latest=source.last_effect.find(crc);const auto old=source.original_rows.find(crc);
    const Impl::InstalledLateDefaultSuccessor::Record* expected=nullptr;const void* address=nullptr;
    std::shared_ptr<DefaultLooseNativeNode> expected_loose;
    const std::string* expected_name=nullptr;
    if(latest!=source.last_effect.end()){
        need(latest->second<source.effects.size()&&source.effects[latest->second]&&source.effects[latest->second]->crc==crc,
             "Installed late Default selected CRC has a foreign actual effect");
        const auto& effect=*source.effects[latest->second];expected=&effect.value;address=effect.actual;
        expected_name=&effect.name;expected_loose=effect.loose;
    }else if(old!=source.original_rows.end()){
        expected=&old->second->returned_value;address=old->second->actual;
        const auto name=source.origin->impl_->returned_names.find(crc);
        need(name!=source.origin->impl_->returned_names.end(),"Installed late Default original selected name missing");
        expected_name=&name->second;const auto node=source.original_loose.find(crc);
        if(node!=source.original_loose.end())expected_loose=node->second;
    }
    const auto name=i.default_state->selected.record_names.find(crc);const auto node=i.default_state->loose_native.find(crc);
    if(!expected){need(row==i.default_state->selected.records.end()&&name==i.default_state->selected.record_names.end()&&
         node==i.default_state->loose_native.end(),"Installed late Default absent CRC acquired an unissued row");return;}
    need(row!=i.default_state->selected.records.end()&&&row->second==address&&
         installed_late_default_detail::same_record(row->second,*expected)&&
         name!=i.default_state->selected.record_names.end()&&expected_name&&name->second==*expected_name,
         "Installed late Default selected CRC changed outside its actual origin/store");
    need(expected_loose?(node!=i.default_state->loose_native.end()&&node->second.get()==expected_loose.get()&&
                         node->second->selected_entry_==&row->second):node==i.default_state->loose_native.end(),
         "Installed late Default selected CRC changed its actual loose witness");
}

void BnwNativePackageDiscoveryOwner::fail_installed_late_default_without_callbacks()const noexcept {
    impl_->fail_installed_late_default_without_callbacks();
}
void BnwNativePackageDiscoveryOwner::require_installed_late_default_writer_idle_without_callbacks()const {
    const auto& i=*impl_;const auto& warm=i.live_resource_query;
    need(warm&&!warm->busy_.load()&&!warm->installed_enumeration_&&!warm->active_owned_skin_read_operation_&&
         !i.active_descriptor_common_source_context&&!i.active_completed_resource_operation&&!i.active_completed_source_opening&&
         (!i.category_source||!i.category_source->native_record_borrow_active_),
         "Installed late Default writer overlaps an actual query/read loan");
}

bool BnwNativePackageDiscoveryOwner::installed_late_default_selected_is_archive_without_callbacks(
    const detail::BnwNativeInstalledLateDefaultOperation& op,std::uint32_t crc)const {
    op.require_fence_without_callbacks(*this);require_installed_late_default_selected_crc_without_callbacks(crc);
    const auto& records=impl_->default_state->selected.records;const auto found=records.find(crc);
    const auto* handle=found==records.end()?nullptr:std::get_if<assets::AssetHandle>(&found->second);
    return handle&&handle->candidate().provenance.kind==assets::SourceKind::fpk;
}
void BnwNativePackageDiscoveryOwner::begin_installed_late_default_selected_store_without_callbacks(
    const detail::BnwNativeInstalledLateDefaultOperation& op,std::uint32_t crc)const {
    op.require_fence_without_callbacks(*this);require_installed_late_default_selected_crc_without_callbacks(crc);
    auto& source=*impl_->installed_late_default;
    need(!source.store_pending,"Installed late Default selected writer reentry");
    source.pending_crc=crc;source.pending_existed=impl_->default_state->selected.records.contains(crc);source.store_pending=true;
}
void BnwNativePackageDiscoveryOwner::record_installed_late_default_selected_store(
    const detail::BnwNativeInstalledLateDefaultOperation& op,std::uint32_t crc)const {
    op.require_fence_without_callbacks(*this);auto& i=*impl_;auto& source=*i.installed_late_default;
    need(source.store_pending&&source.pending_crc==crc,"Installed late Default store has no SAME actual pending CRC");
    const auto row=i.default_state->selected.records.find(crc);const auto name=i.default_state->selected.record_names.find(crc);
    const auto& first=i.first_lineage->storage_.names();const auto inherited=first.find(crc);
    need(row!=i.default_state->selected.records.end()&&name!=i.default_state->selected.record_names.end()&&inherited!=first.end()&&
         i.default_state->selected.records.size()==source.expected_rows+(source.pending_existed?0:1),
         "Installed late Default selected store changed another row/count/FIRST");
    const auto loose=i.default_state->loose_native.find(crc);
    const auto* handle=std::get_if<assets::AssetHandle>(&row->second);
    const bool archive=handle&&handle->candidate().provenance.kind==assets::SourceKind::fpk;
    need(!archive||loose==i.default_state->loose_native.end(),"Installed late archive retained a replaced loose witness");
    if(loose!=i.default_state->loose_native.end())need(loose->second&&loose->second->word_.crc==crc&&
         loose->second->selected_entry_==&row->second&&loose->second->original_stat_,
         "Installed late loose store lacks SAME native node/stat/selected row");
    auto effect=std::make_unique<const Impl::InstalledLateDefaultSuccessor::SelectedEffect>(crc,&row->second,row->second,
        name->second,inherited->second,loose==i.default_state->loose_native.end()?nullptr:loose->second,
        op.impl_->directory.root_ordinal);
    op.require_fence_without_callbacks(*this);
    const auto index=source.effects.size();source.effects.push_back(std::move(effect));source.last_effect.insert_or_assign(crc,index);
    op.require_fence_without_callbacks(*this);
    source.expected_rows+=source.pending_existed?0:1;source.store_pending=false;
    i.frame.default_indexed_names=i.default_state->selected.records.size();
    require_installed_late_default_selected_crc_without_callbacks(crc);
}

void BnwNativePackageDiscoveryOwner::commit_installed_late_default_archive_projection(
    const detail::BnwNativeInstalledLateDefaultOperation& op,
    const detail::BnwNativeInstalledLateDefaultArchiveAttempt& attempt,
    const detail::BnwNativeArchiveV6StreamHeaderOwner& header,const detail::BnwNativeGlobalArchivePrefixRecord& prefix,
    const std::shared_ptr<const assets::FpkSource>& source,const assets::FpkEntry& entry,const assets::AssetHandle& handle,
    const detail::BnwNativeArchiveRecordDiagnostic& words)const {
    op.require_fence_without_callbacks(*this);auto& i=*impl_;
    const auto crc=detail::bnw_native_admission_ascii_crc(entry.original_name);
    require_installed_late_default_attempt_identity_without_callbacks(attempt,true);
    need(i.installed_late_default->store_pending&&i.installed_late_default->pending_crc==crc,
         "Installed late archive projection has no actual pending native store");
    i.entry_arena->require_installed_late_archive_record_identity_without_callbacks(attempt,header,prefix,source,entry,handle,words);
    op.require_fence_without_callbacks(*this);
    auto& records=i.default_state->selected;const auto old=records.records.find(crc);
    const auto old_name=records.record_names.find(crc);
    if(old_name!=records.record_names.end()&&old_name->second!=entry.original_name)
        records.collisions.push_back({crc,old_name->second,entry.original_name,2,true});
    if(old!=records.records.end())records.bucket_order.erase_first(crc);
    need(records.bucket_order.insert(crc,crc)==1,"Installed late archive selected CRC/order duplicate");
    const auto& first=i.first_lineage->storage_.names();const auto inherited=first.find(crc);
    need(inherited!=first.end(),"Installed late archive native store did not execute global FIRST");
    records.first_record_names.try_emplace(crc,inherited->second);
    records.records.insert_or_assign(crc,handle);records.record_names.insert_or_assign(crc,entry.original_name);
    i.default_state->loose_native.erase(crc);
    record_installed_late_default_selected_store(op,crc);
}

detail::BnwNativeInstalledLateDefaultOperation::BnwNativeInstalledLateDefaultOperation(
    const BnwNativePackageDiscoveryOwner& parent,const BnwNativeInstalledLateSpeechCoreDonor& donor) {
    auto& i=*parent.impl_;const auto source=i.installed_late_default;
    need(source&&source->phase==BnwNativePackageDiscoveryOwner::Impl::InstalledLateDefaultSuccessor::Phase::speech&&
         source->speech==&donor&&source->directory_entered&&!source->active_operation&&!source->store_pending,
         "Installed late directory has no new actual Core operation");
    parent.require_installed_late_default_identity_without_callbacks();
    i.require_active_header_without_callbacks();donor.require_active_core_identity_without_callbacks(parent);
    const auto* core=donor.active_core_identity_without_callbacks(parent);
    need(core,"Installed late directory has no retained actual Core identity");
    const auto& occurrence=donor.active_directory_occurrence_brand_without_callbacks(parent);
    need(bool(occurrence),"Installed late directory has no actual physical invocation control");
    for(const auto& previous:source->directories)need(previous->core_brand.owner_before(occurrence)||
         occurrence.owner_before(previous->core_brand),"Installed late Default directory invocation already consumed");
    auto directory=std::make_unique<BnwNativePackageDiscoveryOwner::Impl::InstalledLateDefaultSuccessor::DirectoryEffects>(
        core,occurrence,donor.emitted_62ec20_path_without_callbacks(parent),
        i.default_state->speech_roots6c.size());
    donor.require_active_core_identity_without_callbacks(parent);
    need(directory->core_identity==donor.active_core_identity_without_callbacks(parent)&&
         !directory->core_brand.owner_before(donor.active_directory_occurrence_brand_without_callbacks(parent))&&
         !donor.active_directory_occurrence_brand_without_callbacks(parent).owner_before(directory->core_brand)&&
         directory->query==donor.emitted_62ec20_path_without_callbacks(parent),
         "Installed late Core/path changed during owned operand construction");
    auto state=std::make_unique<Impl>(parent,donor,*directory);
    donor.require_active_core_identity_without_callbacks(parent);
    parent.require_installed_late_default_identity_without_callbacks();
    source->directories.push_back(std::move(directory));
    donor.require_active_core_identity_without_callbacks(parent);
    impl_=std::move(state);source->active_operation=this;
    try{require_fence_without_callbacks(parent);}
    catch(...){if(source->active_operation==this)source->active_operation=nullptr;
        i.fail_installed_late_default_without_callbacks();throw;}
}
detail::BnwNativeInstalledLateDefaultOperation::~BnwNativeInstalledLateDefaultOperation() {
    if(!impl_)return;
    // Storage, not a Parent facade, owns cleanup after the last outside owner.
    auto& i=*impl_->storage;auto& source=*impl_->source;
    if(source.active_operation==this)source.active_operation=nullptr;
    else source.phase=BnwNativePackageDiscoveryOwner::Impl::InstalledLateDefaultSuccessor::Phase::failed;
    if(!impl_->complete||source.store_pending){
        source.phase=BnwNativePackageDiscoveryOwner::Impl::InstalledLateDefaultSuccessor::Phase::failed;
        i.header_poisoned=true;
    }
}
void detail::BnwNativeInstalledLateDefaultOperation::require_fence_without_callbacks(
    const BnwNativePackageDiscoveryOwner& parent)const {
    need(parent.impl_->thread==std::this_thread::get_id(),"Installed late directory foreign creator");
    need(impl_&&&impl_->parent==&parent&&parent.impl_.get()==impl_->storage.get(),
         "Installed late directory borrowed another Parent operand");
    const auto& i=*impl_->storage;const auto& source=*impl_->source;
    need(i.installed_late_default.get()==&source&&source.active_operation==this&&source.speech==&impl_->donor&&
         source.directory_entered&&source.phase==BnwNativePackageDiscoveryOwner::Impl::InstalledLateDefaultSuccessor::Phase::speech&&
         i.active_header_fence==impl_->fence&&impl_->fence&&impl_->fence->valid()&&i.default_state->live_enabled89==1,
         "Installed late directory lost actual source/Core/Header operation");
    i.require_active_header_without_callbacks();parent.require_installed_late_default_identity_without_callbacks();
    parent.require_installed_late_default_writer_idle_without_callbacks();
    impl_->donor.require_active_core_identity_without_callbacks(parent);
    need(impl_->directory.core_identity==impl_->donor.active_core_identity_without_callbacks(parent)&&
         !impl_->directory.core_brand.owner_before(impl_->donor.active_directory_occurrence_brand_without_callbacks(parent))&&
         !impl_->donor.active_directory_occurrence_brand_without_callbacks(parent).owner_before(impl_->directory.core_brand)&&
         impl_->directory.query==impl_->donor.emitted_62ec20_path_without_callbacks(parent),
         "Installed late directory changed the actual emitted Core operand");
}
void detail::BnwNativeInstalledLateDefaultOperation::require_active_core_current(
    const BnwNativePackageDiscoveryOwner& parent)const {
    require_fence_without_callbacks(parent);impl_->source->input->require_current();require_fence_without_callbacks(parent);
    impl_->donor.require_active_core_current(parent);require_fence_without_callbacks(parent);
}
const std::string& detail::BnwNativeInstalledLateDefaultOperation::emitted_speech_path(
    const BnwNativePackageDiscoveryOwner& parent)const {
    require_fence_without_callbacks(parent);return impl_->directory.query;
}
unsigned detail::BnwNativeInstalledLateDefaultOperation::normal_default_loose_mode(
    const BnwNativePackageDiscoveryOwner& parent)const {
    require_fence_without_callbacks(parent);return parent.installed_late_default_normal_loose_mode(*this);
}
unsigned BnwNativePackageDiscoveryOwner::installed_late_default_normal_loose_mode(
    const detail::BnwNativeInstalledLateDefaultOperation& op)const {
    op.require_fence_without_callbacks(*this);const auto mode=impl_->settings->normal_default_loose_mode(*this);
    op.require_fence_without_callbacks(*this);
    need(mode==1,"Installed late Default Viewer/mode6 producer is not owned");return mode;
}
const detail::BnwNativeColdFilesystemOwner& detail::BnwNativeInstalledLateDefaultOperation::cold_without_callbacks(
    const BnwNativePackageDiscoveryOwner& parent)const {
    require_fence_without_callbacks(parent);return *impl_->storage->cold;
}
const std::weak_ptr<const BnwNativePackageDiscoveryOwner>&
detail::BnwNativeInstalledLateDefaultOperation::parent_without_callbacks()const{return impl_->source->parent;}
const std::shared_ptr<const void>& detail::BnwNativeInstalledLateDefaultOperation::source_brand_without_callbacks()const{return impl_->source->brand;}
const std::shared_ptr<const void>& detail::BnwNativeInstalledLateDefaultOperation::core_brand_without_callbacks()const{return impl_->directory.core_brand;}
const void* detail::BnwNativeInstalledLateDefaultOperation::core_identity_without_callbacks()const{return impl_->directory.core_identity;}
std::size_t detail::BnwNativeInstalledLateDefaultOperation::next_archive_ordinal_without_callbacks()const{return impl_->source->attempts;}

void BnwNativePackageDiscoveryOwner::begin_installed_late_default_speech(
    const detail::BnwNativeGlobalResourceWarmOwner& warm,const detail::BnwNativeInstalledLateSpeechCoreDonor& donor)const {
    const auto storage=impl_;auto& i=*storage;
    need(i.thread==std::this_thread::get_id()&&i.installation_input&&!i.lease&&!i.rules&&
         i.installed_default_common&&i.live_resource_query.get()==&warm,
         "Installed late Default requires genuine published installed Warm");
    if(i.installed_late_default_attempted){fail_installed_late_default_without_callbacks();
        throw std::runtime_error("Installed late Default Speech attempt already consumed/reentered");}
    i.installed_late_default_attempted=true;
    struct Failure{std::shared_ptr<Impl> storage;bool done=false;
        ~Failure(){if(!done)storage->fail_installed_late_default_without_callbacks();}}failure{storage};
    donor.require_operation_identity_without_callbacks(*this);
    // ONE genuine closed 8k lender, before any Default+6C or selected mutation.
    warm.require_installed_global_skin_returned();
    donor.require_operation_identity_without_callbacks(*this);
    require_installed_default_common_identity_without_callbacks(*i.installed_default_common);
    auto source=std::make_shared<Impl::InstalledLateDefaultSuccessor>(i,donor);
    donor.require_operation_identity_without_callbacks(*this);
    need(!i.installed_late_default&&i.default_state->speech_roots6c.empty(),
         "Installed late Default source/root receiver already exists");
    i.installed_late_default=std::move(source);
    validate_installed_late_default_namespace_without_callbacks();failure.done=true;
}
void BnwNativePackageDiscoveryOwner::register_installed_late_default_core_directory(
    const detail::BnwNativeInstalledLateSpeechCoreDonor& donor)const {
    const auto storage=impl_;auto& i=*storage;
    require_installed_late_default_identity_without_callbacks();auto& source=*i.installed_late_default;
    if(source.directory_entered||source.active_operation||source.store_pending){fail_installed_late_default_without_callbacks();
        throw std::runtime_error("Installed late Default directory/selected operation reentry");}
    need(source.speech==&donor,"Installed late Default directory borrowed another Speech donor");
    require_installed_late_default_writer_idle_without_callbacks();
    source.directory_entered=true;
    struct EntryEnd{Impl::InstalledLateDefaultSuccessor& source;~EntryEnd(){source.directory_entered=false;}}entry{source};
    struct Failure{std::shared_ptr<Impl> storage;bool done=false;
        ~Failure(){if(!done)storage->fail_installed_late_default_without_callbacks();}}failure{storage};
    {
        detail::BnwNativeInstalledLateDefaultOperation op(*this,donor);
        op.require_active_core_current(*this);validate_installed_late_default_namespace_without_callbacks();
        const auto mode=op.normal_default_loose_mode(*this);need(mode==1,"Installed late Default normal1 source changed");
        op.require_active_core_current(*this);
        installed_late_default_detail::Observation observed(&source,1);
        // SAME manager+6C receiver, before normal1 loose and top *.FPK. Native
        // root append is real even when the authenticated directory is absent.
        i.default_state->speech_roots6c.push_back(op.emitted_speech_path(*this));
        ++source.expected_roots;op.impl_->directory.root_appended=true;
        op.require_active_core_current(*this);
        execute_installed_late_default_loose(op);
        op.require_active_core_current(*this);
        need(op.impl_->directory.loose_returned,"Installed late Default loose producer did not return");
        observed.phase(2);
        execute_installed_late_default_archive(op);
        op.require_active_core_current(*this);
        need(op.impl_->directory.archives_returned&&!op.impl_->directory.active_archive,
             "Installed late Default incidental FPK producer did not return");
        for(const auto& walk:op.impl_->directory.walks)
            need(loose_walk_matches(walk),"Installed late Default loose enumeration changed before directory return");
        for(const auto& before:op.impl_->directory.stamps)
            need(stamp(before.path)==before,"Installed late Default selected loose mirror changed before directory return");
        op.require_active_core_current(*this);validate_installed_late_default_namespace_without_callbacks();
        op.impl_->directory.returned=true;op.impl_->complete=true;
    }
    require_installed_late_default_identity_without_callbacks();failure.done=true;
}
void BnwNativePackageDiscoveryOwner::finish_installed_late_default_speech(
    const detail::BnwNativeInstalledLateSpeechCoreDonor& donor)const {
    const auto storage=impl_;auto& i=*storage;
    require_installed_late_default_identity_without_callbacks();
    struct Failure{std::shared_ptr<Impl> storage;bool done=false;
        ~Failure(){if(!done)storage->fail_installed_late_default_without_callbacks();}}failure{storage};
    auto& source=*i.installed_late_default;
    need(source.phase==Impl::InstalledLateDefaultSuccessor::Phase::speech&&source.speech==&donor&&
         !source.active_operation&&!source.directory_entered&&!source.store_pending&&!i.header_active,
         "Installed late Default Speech returned before actual physical scope retirement");
    // This is an outside-close method. It must not invoke ActiveCore current
    // after that loan/Bank/Loaded/Header has retired. Donor object is still live.
    donor.require_speech_return_without_callbacks(*this);
    i.installation_input->require_current();donor.require_speech_return_without_callbacks(*this);
    validate_installed_late_default_namespace_without_callbacks();
    for(const auto& directory:source.directories)need(directory&&directory->root_appended&&directory->returned&&
        directory->loose_returned&&directory->archives_returned&&!directory->active_archive,
        "Installed late Default Speech lacks an actual directory producer return");
    for(const auto& directory:source.directories)for(const auto& archive:directory->archives)
        need(archive&&archive->phase==Impl::InstalledLateDefaultSuccessor::ArchiveEffect::Phase::returned&&archive->loader_return,
             "Installed late Default Speech lacks a genuine native FPK/cache/absence return");
    source.speech=nullptr;source.phase=Impl::InstalledLateDefaultSuccessor::Phase::returned;
    require_installed_late_default_identity_without_callbacks();failure.done=true;
}

void BnwNativePackageDiscoveryOwner::require_installed_late_default_attempt_identity_without_callbacks(
    const detail::BnwNativeInstalledLateDefaultArchiveAttempt& attempt,bool actual_request_entry)const {
    require_installed_late_default_identity_without_callbacks();const auto& i=*impl_;const auto& source=*i.installed_late_default;
    const auto same=[](const auto& a,const auto& b){return !a.owner_before(b)&&!b.owner_before(a);};
    need(same(attempt.parent_,i.self)&&same(attempt.cold_,i.cold)&&same(attempt.source_brand_,source.brand)&&
         attempt.source_brand_.get()==source.brand.get()&&attempt.request_brand_&&attempt.ordinal_<source.attempts,
         "Installed late archive request has another actual source/Parent/Cold/brand");
    const Impl::InstalledLateDefaultSuccessor::DirectoryEffects* directory=nullptr;
    const Impl::InstalledLateDefaultSuccessor::ArchiveEffect* effect=nullptr;
    for(const auto& d:source.directories)for(const auto& a:d->archives)if(a&&a->attempt.get()==&attempt){
        need(!effect,"Installed late archive request aliases two actual attempts");directory=d.get();effect=a.get();}
    need(directory&&effect&&same(directory->core_brand,attempt.core_brand_)&&
         directory->core_identity==attempt.core_identity_&&directory->root_appended&&
         directory->root_ordinal<i.default_state->speech_roots6c.size()&&
         i.default_state->speech_roots6c[directory->root_ordinal]==directory->query,
         "Installed late archive request lost its retained actual Core/root occurrence");
    if(actual_request_entry){
        need(source.active_operation&&&source.active_operation->impl_->directory==directory&&
             directory->active_archive==effect&&!directory->archives_returned&&
             effect->phase!=Impl::InstalledLateDefaultSuccessor::ArchiveEffect::Phase::returned&&
             (effect->request||effect->phase==Impl::InstalledLateDefaultSuccessor::ArchiveEffect::Phase::issued),
             "Installed late archive source is outside its genuine active FindA operation");
        source.active_operation->require_fence_without_callbacks(*this);
    }else{
        if(!effect->request)need(source.active_operation&&&source.active_operation->impl_->directory==directory&&
             directory->active_archive==effect&&effect->phase==Impl::InstalledLateDefaultSuccessor::ArchiveEffect::Phase::issued,
             "Installed late archive current precedes its genuine request mint");
        if(source.active_operation&&&source.active_operation->impl_->directory==directory){
            need(directory->active_archive==effect||effect->phase==Impl::InstalledLateDefaultSuccessor::ArchiveEffect::Phase::returned,
                 "Installed late archive request is outside its actual source phase");
            source.active_operation->require_fence_without_callbacks(*this);
        }else need(directory->returned&&effect->phase==Impl::InstalledLateDefaultSuccessor::ArchiveEffect::Phase::returned,
             "Installed late archive request retained an incomplete departed operation");
    }
}
void BnwNativePackageDiscoveryOwner::require_installed_late_default_attempt_current(
    const detail::BnwNativeInstalledLateDefaultArchiveAttempt& attempt)const {
    const auto storage=impl_;
    struct Failure{std::shared_ptr<Impl> storage;bool done=false;
        ~Failure(){if(!done)storage->fail_installed_late_default_without_callbacks();}}failure{storage};
    require_installed_late_default_attempt_identity_without_callbacks(attempt,false);
    need(!storage->installed_late_default->store_pending,
         "Installed late archive FULL during unfinished selected store");
    storage->installation_input->require_current();
    require_installed_late_default_attempt_identity_without_callbacks(attempt,false);
    storage->settings->validate_source();
    require_installed_late_default_attempt_identity_without_callbacks(attempt,false);failure.done=true;
}
void detail::BnwNativeInstalledLateDefaultArchiveAttempt::require_active_request()const {
    auto parent=parent_.lock();need(bool(parent),"Installed late archive actual Parent retired");
    const auto storage=parent->impl_;
    need(storage->thread==std::this_thread::get_id(),"Installed late archive request foreign creator");
    BnwNativePackageDiscoveryOwner::Impl::InstallationParentLoan loan(*storage,std::move(parent));
    loan.parent->require_installed_late_default_attempt_identity_without_callbacks(*this,true);
    loan.parent->require_installed_late_default_attempt_current(*this);
    loan.parent->require_installed_late_default_attempt_identity_without_callbacks(*this,true);
}
void detail::BnwNativeInstalledLateDefaultArchiveAttempt::require_current()const {
    auto parent=parent_.lock();need(bool(parent),"Installed late archive actual Parent retired");
    const auto storage=parent->impl_;
    need(storage->thread==std::this_thread::get_id(),"Installed late archive request foreign creator");
    BnwNativePackageDiscoveryOwner::Impl::InstallationParentLoan loan(*storage,std::move(parent));
    loan.parent->require_installed_late_default_attempt_current(*this);
}
void detail::BnwNativeInstalledLateDefaultArchiveAttempt::require_same_input(const BnwNativeColdFilesystemOwner& cold)const {
    require_current();
    const auto actual=cold_.lock();need(actual&&actual.get()==&cold,"Installed late archive has another actual Cold");
}
std::size_t detail::BnwNativeInstalledLateDefaultArchiveAttempt::source_ordinal()const{require_current();return ordinal_;}
const std::string& detail::BnwNativeInstalledLateDefaultArchiveAttempt::raw_relative_ansi_request()const{require_current();return relative_ansi_;}
const std::u16string& detail::BnwNativeInstalledLateDefaultArchiveAttempt::raw_utf16_stream_name_candidate()const{require_current();return raw_stream_;}
const std::filesystem::path& detail::BnwNativeInstalledLateDefaultArchiveAttempt::original_relative_io_token()const{require_current();return relative_;}
const std::filesystem::path& detail::BnwNativeInstalledLateDefaultArchiveAttempt::original_absolute_io_candidate()const{require_current();return original_;}
const std::filesystem::path& detail::BnwNativeInstalledLateDefaultArchiveAttempt::canonical_payload_io_candidate()const{require_current();return canonical_;}
const std::shared_ptr<const void>& detail::BnwNativeInstalledLateDefaultArchiveAttempt::privatebrand_without_callbacks()const{return request_brand_;}

void BnwNativePackageDiscoveryOwner::Impl::require_installed_late_default_bucket_order_without_callbacks(
    const std::vector<detail::NativeResourceBucketOrder::Node>& order)const {
    const auto source=installed_late_default;
    need(source&&source->thread==std::this_thread::get_id()&&!header_poisoned&&!category_lineage_failed&&
         source->phase!=InstalledLateDefaultSuccessor::Phase::failed&&!source->store_pending&&
         source->actual_default==default_state.get()&&source->expected_rows==order.size(),
         "Installed late discovery topology has no actual completed namespace store");
    const auto* projection=prepared_default_projection.get();
    for(const auto& node:order){
        const auto selected=default_state->selected.records.find(node.key);
        need(selected!=default_state->selected.records.end(),"Installed late discovery order lost a selected CRC");
        if(node.value==node.key)continue; // SAME old/owned NativeRecordIndex insertion, not a new Arena slot.
        need(projection&&projection->source&&!source->last_effect.contains(node.key),
             "Installed late changed record retained an original primary slot");
        const auto original=projection->source->selected.find(node.key);
        const auto* handle=std::get_if<assets::AssetHandle>(&selected->second);
        need(original!=projection->source->selected.end()&&original->second.actual_record_node&&
             original->second.actual_handle_node&&original->second.canonical_handle&&
             original->second.record.crc==node.key&&original->second.record.symbolic_slot==node.value&&
             *original->second.actual_record_node==original->second.record&&
             handle&&installed_late_default_detail::same_handle(*handle,*original->second.canonical_handle),
             "Installed late unchanged primary slot lost its actual native source");
    }
}
