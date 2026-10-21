// Dedicated actual installation Common/DefaultLoose result. No saved lease.
struct detail::BnwNativeInstalledDefaultCommonSource::Impl {
    using Parent=BnwNativePackageDiscoveryOwner;
    using Record=decltype(detail::NativeResourceRecordIndex{}.records)::mapped_type;
    const std::thread::id thread=std::this_thread::get_id();
    const std::weak_ptr<const Parent> parent;
    const std::shared_ptr<const BnwNativeInstallationInputOwner> input;
    const std::shared_ptr<const void> epoch;
    const assets::ResourceMountTable* const table;
    const DefaultState* const actual_default;
    const Parent::Impl::InstallationCommonStorage* const common_storage;
    const std::weak_ptr<BnwNativePreDx9DiscoveryOwner> discovery;
    const std::shared_ptr<const BnwNativeResourceBootSettingsOwner> settings;
    const std::shared_ptr<const BnwNativeColdFilesystemOwner> cold;
    const std::shared_ptr<const BnwNativeOriginalStatService> stat_service;
    const std::shared_ptr<const BnwNativeGlobalArchivePrefixOwner> prefixes;
    const std::shared_ptr<const BnwNativeArchiveEntryAdmissionArena> arena;
    const std::shared_ptr<const BnwNativeArchiveBasenameCacheOwner> cache;
    const std::shared_ptr<const BnwNativeCurrentDefaultSource> default_source;
    const std::shared_ptr<BnwNativeSharedFirstLineage> first;
    std::weak_ptr<const BnwNativeInstalledDefaultCommonSource> self;
    enum class Phase {issued,scanning,scanned,publishing,returned};
    Phase phase=Phase::issued;
    std::atomic<bool> failed{false},busy{false},checking{false},metadata_checking{false};
    detail::NativeResourceRecordIndex records;
    std::map<std::uint32_t,std::shared_ptr<DefaultLooseNativeNode>> native_loose;
    BnwNativePackageDiscoveryFrame frame;
    std::vector<Stamp> stamps;
    std::vector<std::filesystem::path> absent;
    std::vector<LooseWalk> walks;
    UINT code_page=0;
    std::size_t completed_callers=0;
    struct Binding {std::uint32_t key;const Record* actual;Record returned_value;};
    std::vector<Binding> selected_bindings;
    std::map<std::uint32_t,std::string> returned_names,returned_first;
    std::vector<std::string> returned_registration_order;
    std::uint64_t publication_revision=0;
    struct NormalReturn {
        const std::weak_ptr<const BnwNativeInstalledDefaultCommonSource> source;
        const std::thread::id thread=std::this_thread::get_id();
        explicit NormalReturn(const std::shared_ptr<const BnwNativeInstalledDefaultCommonSource>& actual):source(actual){}
    };
    std::unique_ptr<const NormalReturn> returned_publication;
    const PublicationOperation* publication_operation=nullptr;
    const PublicationOperation* publication_validation=nullptr;
    struct Validation {
        const BnwNativeInstalledDefaultCommonSource& source;
        const Parent::Impl::HeaderGuard& fence;
        const std::thread::id thread=std::this_thread::get_id();
    };
    const Validation* active_validation=nullptr;
    Impl(const std::shared_ptr<const Parent>& actual,const Parent::Impl& i)
        :parent(actual),input(i.installation_input),epoch(i.identity),table(&i.resources),actual_default(i.default_state.get()),
         common_storage(i.installation_common_storage.get()),discovery(i.pre_dx9_discovery),settings(i.settings),
         cold(i.cold),stat_service(i.stat_service),prefixes(i.prefixes),arena(i.entry_arena),cache(i.archive_cache),
         default_source(i.current_default_source),first(i.first_lineage),
         records(i.installation_common_storage->records),frame(i.installation_common_storage->frame),
         stamps(i.resource_stamps),absent(i.installation_common_storage->absent){}
};
struct detail::BnwNativeInstalledDefaultCommonSource::PublicationOperation {
    const std::shared_ptr<const BnwNativeInstalledDefaultCommonSource> source;
    const std::thread::id thread=std::this_thread::get_id();
    explicit PublicationOperation(const std::shared_ptr<const BnwNativeInstalledDefaultCommonSource>& actual):source(actual){
        need(source&&source->impl_->thread==thread&&source->impl_->busy.load()&&!source->impl_->publication_operation,
             "Installed publication has no actual new driver scope");
        source->impl_->publication_operation=this;
    }
    ~PublicationOperation(){if(source->impl_->publication_operation==this)source->impl_->publication_operation=nullptr;
        else source->fail();}
    PublicationOperation(const PublicationOperation&)=delete;
};
detail::BnwNativeInstalledDefaultCommonSource::BnwNativeInstalledDefaultCommonSource(std::unique_ptr<Impl> value):impl_(std::move(value)){}
detail::BnwNativeInstalledDefaultCommonSource::~BnwNativeInstalledDefaultCommonSource()=default;
bool detail::BnwNativeInstalledDefaultCommonSource::publication_returned_without_callbacks()const {
    need(impl_->thread==std::this_thread::get_id(),"Installed Common publication observation foreign creator");
    return impl_->phase==Impl::Phase::returned&&!impl_->failed.load();
}
bool detail::BnwNativeInstalledDefaultCommonSource::driver_active_without_callbacks()const {
    need(impl_->thread==std::this_thread::get_id(),"Installed Common driver observation foreign creator");
    return impl_->busy.load()||impl_->checking.load()||impl_->metadata_checking.load();
}
bool detail::BnwNativeInstalledDefaultCommonSource::loose_returned_without_callbacks()const {
    need(impl_->thread==std::this_thread::get_id(),"Installed loose return observation foreign creator");
    return impl_->phase==Impl::Phase::scanned&&!impl_->failed.load();
}
void detail::BnwNativeInstalledDefaultCommonSource::fail()const noexcept {impl_->failed.store(true);}
void BnwNativePackageDiscoveryOwner::advance_installation_default_loose_source()const {
    const auto storage=impl_;auto& i=*storage;
    need(i.thread==std::this_thread::get_id()&&i.installation_input&&!i.lease&&!i.rules,
         "Installed DefaultLoose requires original installation creator domain");
    if(i.installed_default_common&&i.installed_default_common->driver_active_without_callbacks()){
        i.header_poisoned=true;i.installed_default_common->fail();throw std::runtime_error("Installed DefaultLoose driver reentry");
    }
    if(i.installed_default_loose_attempted&&!i.installed_default_common){
        i.header_poisoned=true;throw std::runtime_error("Installed DefaultLoose reentered before actual issuer construction");
    }
    need(!i.installed_default_loose_attempted,"Installed DefaultLoose attempt already consumed");
    i.installed_default_loose_attempted=true;
    struct AttemptFailure{std::shared_ptr<Impl> storage;bool done=false;~AttemptFailure(){if(!done)storage->header_poisoned=true;}} attempt_failure{storage};
    Impl::InstallationParentLoan retained(i);const auto& actual=retained.parent;
    need(actual&&actual.get()==this&&i.installation_has_outside_parent()&&i.pre_dx9_discovery&&
         i.installation_common_loop_returned&&i.installation_common_loop_attempted&&i.installation_common_storage&&
         i.common_steps.size()==detail::bnw_common13_descriptors.size()&&!i.common_pending&&!i.common_driver_failed&&
         !i.installed_default_common&&!i.live_resource_query&&!i.global_color_bank&&!i.category_source_attempted&&
         i.default_state->loose_native.empty(),
         "Installed DefaultLoose requires actual SAME returned Common13 and no later publication");
    i.validate_installation_prefix_current();
    auto source=std::shared_ptr<detail::BnwNativeInstalledDefaultCommonSource>(new detail::BnwNativeInstalledDefaultCommonSource(
        std::make_unique<detail::BnwNativeInstalledDefaultCommonSource::Impl>(actual,i)));
    source->impl_->self=source;i.installed_default_common=source;
    auto& data=*source->impl_;data.busy.store(true);data.phase=detail::BnwNativeInstalledDefaultCommonSource::Impl::Phase::scanning;
    struct Exit{detail::BnwNativeInstalledDefaultCommonSource::Impl& data;~Exit(){data.busy.store(false);}} exit{data};
    struct Failure{Impl& i;detail::BnwNativeInstalledDefaultCommonSource::Impl& data;bool done=false;
        ~Failure(){if(!done){data.failed.store(true);i.header_poisoned=true;}}}failure{i,data};
    {
        BnwPrimaryDefaultStageTrace trace("Installed.DefaultLoose");
        Impl::HeaderGuard fence(i.header_callback_state,i.self);need(fence.valid(),"Installed DefaultLoose Parent retired/reentered");
        struct Active{Impl& i;Active(Impl& value,const Impl::HeaderGuard& fence):i(value){i.header_active=true;i.active_header_fence=&fence;}
            ~Active(){i.active_header_fence=nullptr;i.header_active=false;}}active(i,fence);
        const auto common_current=[&]{
            i.require_active_header_without_callbacks();
            require_pre_dx9_discovery_current(*i.pre_dx9_discovery);
            need(i.installed_default_common.get()==source.get()&&!data.failed.load()&&data.busy.load()&&
                 data.phase==detail::BnwNativeInstalledDefaultCommonSource::Impl::Phase::scanning&&
                 i.installation_common_storage.get()==data.common_storage&&i.common_steps.size()==13&&!i.common_pending&&
                 !i.common_driver_failed&&i.settings.get()==data.settings.get()&&!i.live_resource_query&&
                 i.installation_has_outside_parent(),"Installed DefaultLoose actual operation/source retired or reentered");
            i.current_default_source->require_current(*actual);i.require_active_header_without_callbacks();
            i.cold->require_same_input(*i.installation_input);i.stat_service->require_same_input(*i.installation_input);
            const auto& boot=i.settings->common_operation_frame(*actual);
            need(boot.normal_launch_source_closed&&!boot.leader_viewer&&boot.effective_texture_quality&&*boot.effective_texture_quality!=0&&
                 i.frame.high_texture_quality_source_branch&&!i.low_state,"Installed DefaultLoose actual HIGH source unavailable");
            i.entry_arena->require_retained_common_projection_origin_without_callbacks(*i.prepared_default_projection->source,
                i.prepared_default_projection->records,i.common_steps,i.installation_common_storage->records);
            i.require_active_header_without_callbacks();need(fence.valid()&&i.installation_has_outside_parent(),"Installed DefaultLoose lost outside Parent");
        };
        common_current();
        auto& records=data.records;auto& native_loose=data.native_loose;auto& frame=data.frame;
        auto& stamps=data.stamps;auto& walks=data.walks;
        assets::LooseResourceIndex canonical(i.shared);
        using QueryFolder=detail::BnwNativeDefaultLooseQueryFolder;
        data.code_page=GetACP();
        // Actual DefaultLoose effect body is inserted below from the existing
        // proved source operation; only common_current has a distinct domain.
        // BEGIN_ACTUAL_DEFAULT_LOOSE_BODY
    const auto add_loose = [&](QueryFolder::Caller caller) {
        const QueryFolder folder(caller,*i.settings,*this);
        const auto query=folder.original_query();
        const auto mode=folder.native_mode(); //SAME retained normal startup operand, not a caller bool.
        const bool filters=folder.base_filters(),xml_override=folder.xml_override();
        const char* relative=query.c_str();
        common_current();
        frame.resource_registration_order.push_back(query);
        ++frame.loose_registration_requests;
        const auto root=i.original.parent_path()/std::filesystem::path(native_loose_detail::acp_wide(query));
        LooseWalk walk{root,filters,std::filesystem::exists(root),{},GetACP(),true};
        std::map<std::string,std::pair<std::uint32_t,std::u16string>> folder_prefixes;
        auto root_query=query;std::replace(root_query.begin(),root_query.end(),'/','\\');
        if(root_query.empty()||root_query.back()!='\\')root_query+='\\';
        const auto admit_entry=[&](const native_loose_detail::LooseEntry& entry) {
            if ((entry.attributes & FILE_ATTRIBUTE_DIRECTORY) != 0) return;
            ++frame.loose_file_registration_requests;
            const auto source = walk.root / entry.relative;
            const auto mapped = i.shared / relative / entry.relative;
            const auto extension = runtime::ascii_lower(assets::path_utf8(entry.relative.extension()));
            const unsigned actual_mode = xml_override && (extension == ".xml" || extension == ".ggxml") ? 2 : mode;
            const auto slash=entry.raw_relative.find_last_of('\\');
            const auto child=slash==std::string::npos?std::string{}:entry.raw_relative.substr(0,slash);
            const auto prefix=folder_prefixes.find(child);
            need(prefix!=folder_prefixes.end(),"Default loose FindA entry lacks actual folder registration");
            const auto original_relative=std::filesystem::path(native_loose_detail::acp_wide(query+'\\'+entry.raw_relative));
            common_current();const auto stat=i.stat_service->read_original_relative(original_relative);
            common_current();const auto fields=stat->current();
            if(fields.return_value!=0&&!fields.size&&!fields.mtime) {
                // FindA's ANSI spelling may not select a W filename (best-fit
                // conversion). Retain the failed actual CRT read and actual
                // reconstructed-path open failure, never Unicode/Find fields.
                common_current();const auto error=native_missing_open(source);
                need(error==ERROR_FILE_NOT_FOUND||error==ERROR_PATH_NOT_FOUND,
                     "Default unknown Original stat requires genuine missing reconstructed-path open");
                common_current();stat->require_same_service(*i.stat_service);common_current();
                need(stat->current()==fields,"Default failed Original stat changed before unreadable insertion");
                common_current();
                const auto key=DefaultRecords::source_hash(entry.native_name);
                // Actual finite modes0/1/2/4 use the existing deterministic
                // loose/archive predicate, never unchecked stat mtime. Preserve
                // records.insert FIRST/CRC/mode/bucket behavior without a new
                // blanket collision restriction. Mode6/7 remain unsupported.
                const auto relative_name=assets::path_utf8(std::filesystem::path(relative)/entry.relative);
                if(records.insert(entry.native_name,UnreadableLoose{source,relative_name,error,
                    {relative_name,entry.native_name,entry.bytes,entry.written,entry.attributes,error},stat},actual_mode)) {
                    ++frame.loose_files_registered;
                    native_loose.erase(key); // Never retain prior successful words for this selected unknown HIT.
                }
                common_current();need(stat->current()==fields,"Default failed Original stat changed after unreadable insertion");
                common_current();return;
            }
            if(!(fields.return_value==0&&fields.size&&*fields.size>=0&&fields.mtime))
                throw std::runtime_error(std::string("Default loose unchecked Original stat outputs unknown; cannot manufacture native length")+
                    "; relative="+assets::path_utf8(original_relative)+"; absolute="+assets::path_utf8(source)+
                    "; return="+std::to_string(fields.return_value)+"; errno="+std::to_string(fields.error_number)+
                    "; size="+(fields.size?std::to_string(*fields.size):std::string("UNKNOWN"))+
                    "; mtime="+(fields.mtime?std::to_string(*fields.mtime):std::string("UNKNOWN")));
            const auto publish_node=[&] {
                common_current();stat->require_same_service(*i.stat_service);
                need(stat->current()==fields,"Default native stat changed during selected insertion");
                const auto key=DefaultRecords::source_hash(entry.native_name);
                auto node=std::shared_ptr<DefaultLooseNativeNode>(new DefaultLooseNativeNode);
                node->word_.crc=key;node->word_.length=std::uint32_t(*fields.size);node->word_.offset=0;
                node->prefix_index_=prefix->second.first;
                node->word_.packed_prefix=node->prefix_index_<<20; //Only upper12 authenticated here.
                node->original_find_basename_=entry.native_name;
                node->original_folder_query_=child.empty()?root_query:root_query+child+'\\';
                node->registered_prefix_=prefix->second.second;node->original_stat_=stat;
                native_loose.insert_or_assign(key,std::move(node));common_current();
            };
            if (!std::filesystem::exists(source)) {
                const auto error = native_missing_open(source);
                need(error == ERROR_FILE_NOT_FOUND || error == ERROR_PATH_NOT_FOUND,
                     "Native loose unavailable source requires actual read failure owner");
                const auto relative_name = assets::path_utf8(std::filesystem::path(relative) / entry.relative);
                if (records.insert(entry.native_name, UnreadableLoose{source, relative_name, error,
                    {relative_name, entry.native_name, entry.bytes, entry.written, entry.attributes, error}}, actual_mode))
                    {++frame.loose_files_registered;publish_node();}
                return; // Retained indexed record, NOT a successful payload alias/skip.
            }
            need(std::filesystem::is_regular_file(mapped), "Canonical native loose mirror missing");
            const auto a = stamp(source), b = stamp(mapped);
            const auto original_id = runtime::identify_file(source);
            const auto mapped_id = runtime::identify_file(mapped);
            need(original_id.bytes == mapped_id.bytes && original_id.sha256 == mapped_id.sha256,
                 "Canonical native loose payload differs from original source");
            need(stamp(source) == a && stamp(mapped) == b, "Native loose source changed during registration");
            stamps.push_back(a); stamps.push_back(b);
            canonical.add_file(mapped);
            const auto asset = canonical.find(assets::AssetKey(assets::path_utf8(entry.relative.filename())));
            const auto* handle = std::get_if<assets::AssetHandle>(&asset);
            need(handle != nullptr, "Native loose canonical handle unavailable");
            if (records.insert(entry.native_name, *handle, actual_mode)) {
                ++frame.loose_files_registered;publish_node();
            }
        };
        if(walk.exists) {
            need(std::filesystem::is_directory(root),"Default loose native source is not a directory");
            native_loose_detail::NativeEnumeration enumeration;
            native_loose_detail::visit_loose_entries_with_folder(native_loose_detail::acp_path(root),filters,
                enumeration,[&](const std::string& raw,const std::string& name,DWORD attrs,std::uint64_t size,std::uint64_t written){
                    walk.entries.push_back({std::filesystem::path(native_loose_detail::acp_wide(raw)),name,attrs,size,written,raw});
                    admit_entry(walk.entries.back()); //Actual folder prefix -> FindA -> stat -> insertion before recursion.
                },[&](const std::string& child){
                    common_current();
                    const auto original=child.empty()?root_query:root_query+child+'\\';
                    const auto wide=native_loose_detail::acp_wide(original);
                    const std::u16string raw_prefix(wide.begin(),wide.end());
                    //83A720 registers exact original query folder BEFORE FindA;
                    //neither absolute I/O root nor AssetHandle path is authority.
                    const auto index=i.prefixes->register_loose_source_prefix(raw_prefix,common_current);
                    common_current();folder_prefixes.emplace(child,std::make_pair(index,raw_prefix));
                },"*.*");
        }
        common_current();
        walks.push_back(std::move(walk));
    };
        add_loose(QueryFolder::Caller::overrides);++data.completed_callers;
        add_loose(QueryFolder::Caller::assets);++data.completed_callers;
        add_loose(QueryFolder::Caller::shared_speech);++data.completed_callers;
        // END_ACTUAL_DEFAULT_LOOSE_BODY
        common_current();need(data.completed_callers==3&&frame.resource_registration_order.size()==48,
             "Installed DefaultLoose lacks actual original-order three caller returns");
        frame.native_loose_code_page=data.code_page;
        frame.indexed_unreadable_loose.clear();
        for(const auto& [key,record]:records.records)if(const auto* unavailable=std::get_if<UnreadableLoose>(&record)){
            need(native_missing_open(unavailable->native_path)==unavailable->error,"Installed actual unreadable record changed");
            frame.indexed_unreadable_loose.push_back(unavailable->receipt);
        }
        std::sort(frame.indexed_unreadable_loose.begin(),frame.indexed_unreadable_loose.end(),
            [](const auto& a,const auto& b){return a.relative_path<b.relative_path;});
        frame.default_indexed_names=records.records.size();frame.resource_crc_collisions=records.collisions;
        for(const auto& walk:walks)need(loose_walk_matches(walk),"Installed DefaultLoose source changed before actual return");
        for(const auto& stamp_before:stamps)need(stamp(stamp_before.path)==stamp_before,"Installed loose payload changed before actual return");
        common_current();need(GetACP()==data.code_page,"Installed actual native code page changed");
    }
    data.phase=detail::BnwNativeInstalledDefaultCommonSource::Impl::Phase::scanned;
    i.validate_installation_prefix_current();
    i.frame.next_resource_dependency="Mandatory actual installed final Default/Common publication before Warm/category/Skin";
    failure.done=true;attempt_failure.done=true;
}
void BnwNativePackageDiscoveryOwner::require_installed_default_common_origin_without_callbacks(
    const detail::BnwNativeInstalledDefaultCommonSource& source)const {
    const auto storage=impl_;const auto& i=*storage;const auto& data=*source.impl_;
    need(i.thread==std::this_thread::get_id()&&data.thread==i.thread,
         "Installed Common source identity foreign creator");
    const auto same=[](const auto& a,const auto& b){return !a.owner_before(b)&&!b.owner_before(a);};
    const auto discovery=data.discovery.lock();
    need(discovery&&i.pre_dx9_discovery.get()==discovery.get()&&same(i.pre_dx9_discovery,discovery)&&
         i.installed_default_common.get()==&source&&same(i.installed_default_common,data.self)&&
         !data.failed.load()&&data.phase==detail::BnwNativeInstalledDefaultCommonSource::Impl::Phase::returned&&
         i.installation_input&&same(i.installation_input,data.input)&&!i.lease&&!i.rules&&
         same(i.self,data.parent)&&same(i.identity,data.epoch)&&&i.resources==data.table&&
         i.default_state.get()==data.actual_default&&i.settings.get()==data.settings.get()&&same(i.settings,data.settings)&&
         i.installation_common_storage.get()==data.common_storage&&!i.header_poisoned&&!i.category_lineage_failed&&
         i.installation_common_loop_returned&&i.installation_common_loop_attempted&&i.common_driver_attempted&&!i.common_driver_failed&&
         i.common_steps.size()==13&&!i.common_pending&&i.current_default_source&&i.entry_arena&&i.archive_cache&&
         i.cold&&i.stat_service&&i.prefixes&&i.first_lineage&&i.resource_catalog&&i.production_default_published&&
         i.cold.get()==data.cold.get()&&same(i.cold,data.cold)&&
         i.stat_service.get()==data.stat_service.get()&&same(i.stat_service,data.stat_service)&&
         i.prefixes.get()==data.prefixes.get()&&same(i.prefixes,data.prefixes)&&
         i.entry_arena.get()==data.arena.get()&&same(i.entry_arena,data.arena)&&
         i.archive_cache.get()==data.cache.get()&&same(i.archive_cache,data.cache)&&
         i.current_default_source.get()==data.default_source.get()&&same(i.current_default_source,data.default_source)&&
         i.first_lineage.get()==data.first.get()&&same(i.first_lineage,data.first)&&
         i.frame.common_high_quality_registration_complete&&i.frame.high_texture_quality_source_branch&&
         !i.low_state&&!i.frame.low_resource_mount_present&&data.completed_callers==3&&
         i.live_resource_query&&i.live_resource_query->installed_common_source_.get()==&source&&
         !i.live_resource_query->failed_.load()&&!i.live_resource_query->closed_.load()&&
         same(i.live_resource_query->installed_common_source_,data.self)&&
         i.live_resource_query->epoch_.get()==i.identity.get()&&same(i.live_resource_query->parent_,i.self),
         "Installed Warm lost actual Common/three-loose/publication source controls");
    const bool actual_publication=data.publication_validation&&data.publication_validation==data.publication_operation&&
        data.publication_validation->source.get()==&source&&data.publication_validation->thread==i.thread&&data.busy.load();
    need((data.returned_publication&&data.returned_publication->thread==i.thread&&
          same(data.returned_publication->source,data.self))||actual_publication,
         "Installed namespace has no actual normal-return receipt or typed publication scope");
    // Source3a removes the duplicate installation-loan subtraction here.
    // The canonical pre-DX9 validator counts registered loans/Header/Warm/
    // Category/Bank pins; this does not add a hidden Parent facade keeper.
    require_pre_dx9_discovery_current(*discovery);
    need(i.registry.revision>=data.publication_revision&&data.publication_revision!=0&&
         i.default_state->selected.source_first_.get()==i.first_lineage.get()&&
         data.returned_registration_order.size()==48&&i.frame.resource_registration_order.size()>=48&&
         std::equal(data.returned_registration_order.begin(),data.returned_registration_order.end(),i.frame.resource_registration_order.begin()),
         "Installed final Default changed actual returned record set/order/FIRST");
    // Base Default writers are fenced before any new effects. Query winners
    // are still resolved against live manager/default objects; per-node checks
    // remain in the actual query. The whole returned map is checked only by
    // the private publication operation, not once per metadata fence.

}
void BnwNativePackageDiscoveryOwner::require_installed_default_common_identity_without_callbacks(
    const detail::BnwNativeInstalledDefaultCommonSource& source)const {
    require_installed_default_common_origin_without_callbacks(source);
    if(impl_->installed_late_default)require_installed_late_default_observation_without_callbacks();
    else need(impl_->default_state->selected.records.size()==source.impl_->selected_bindings.size(),
         "Installed final Default changed actual returned record set/order/FIRST");
}
void BnwNativePackageDiscoveryOwner::require_installed_default_common_active_current(
    const detail::BnwNativeInstalledDefaultCommonSource& source)const {
    const auto storage=impl_;auto& i=*storage;
    need(i.thread==std::this_thread::get_id(),"Installed metadata source foreign creator");
    const auto issuer=i.installed_default_common;auto& data=*source.impl_;
    i.require_active_header_without_callbacks();
    if(data.active_validation){
        need(data.checking.load()&&&data.active_validation->source==&source&&
             data.active_validation->thread==i.thread&&&data.active_validation->fence==i.active_header_fence&&
             data.active_validation->fence.valid(),"Installed callback changed SAME synchronous validation/Header scope");
        require_installed_default_common_identity_without_callbacks(source);return;
    }
    need(issuer&&issuer.get()==&source&&data.returned_publication&&!data.busy.load()&&!data.checking.load(),
         "Installed cached metadata borrow has no actual normal publication return");
    if(data.metadata_checking.exchange(true)){issuer->fail();throw std::runtime_error("Installed retained metadata source reentry");}
    struct End{detail::BnwNativeInstalledDefaultCommonSource::Impl& data;~End(){data.metadata_checking.store(false);}} end{data};
    try {
    require_installed_default_common_identity_without_callbacks(source);
    // Genuine cached Primary32/FIRST metadata may be observed under its own
    // closed Parent Header. It validates live input/Boot identity without
    // replaying registration or walking the whole retained namespace.
    data.input->require_current();require_installed_default_common_identity_without_callbacks(source);
    data.settings->validate_source();require_installed_default_common_identity_without_callbacks(source);
    i.require_active_header_without_callbacks();
    }catch(...){issuer->fail();throw;}
}
void detail::BnwNativeInstalledDefaultCommonSource::require_current()const {
    need(impl_->thread==std::this_thread::get_id(),"Installed Common current foreign creator");
    const auto issuer=impl_->self.lock();need(issuer&&issuer.get()==this,"Installed Common actual issuer retired");
    auto& data=*issuer->impl_;
    if(data.busy.load()||data.metadata_checking.load()||!data.returned_publication){
        issuer->fail();throw std::runtime_error("Installed Warm requested before actual publication normal return or during source reentry");
    }
    if(data.checking.exchange(true)){issuer->fail();throw std::runtime_error("Installed Common source validation reentry");}
    struct Exit{Impl& data;~Exit(){data.checking.store(false);}} exit{data};
    try {
        auto parent=data.parent.lock();need(bool(parent),"Installed Common startup Parent retired");
        const auto storage=parent->impl_; // Keep storage through borrowed Parent destruction and all unwinding.
        BnwNativePackageDiscoveryOwner::Impl::InstallationParentLoan retained(*storage,std::move(parent));
        retained.parent->validate_installed_default_common_current(*issuer);
    }catch(...){issuer->fail();throw;}
}
void detail::BnwNativeInstalledDefaultCommonSource::require_publication_current(const PublicationOperation& operation)const {
    need(impl_->thread==std::this_thread::get_id(),"Installed publication check foreign creator");
    const auto issuer=impl_->self.lock();need(issuer&&issuer.get()==this,"Installed publication issuer retired");
    auto& data=*issuer->impl_;
    need(data.publication_operation==&operation&&operation.source.get()==this&&operation.thread==data.thread&&
         data.busy.load()&&!data.returned_publication&&data.phase==Impl::Phase::returned,
         "Installed full publication check requires SAME actual private operation");
    if(data.checking.exchange(true)){issuer->fail();throw std::runtime_error("Installed publication validation reentry");}
    data.publication_validation=&operation;
    struct Exit{Impl& data;~Exit(){data.publication_validation=nullptr;data.checking.store(false);}} exit{data};
    try{
        auto parent=data.parent.lock();need(bool(parent),"Installed publication Parent retired");
        const auto storage=parent->impl_;
        BnwNativePackageDiscoveryOwner::Impl::InstallationParentLoan retained(*storage,std::move(parent));
        retained.parent->validate_installed_default_common_current(*issuer);
    }catch(...){issuer->fail();throw;}
}
void BnwNativePackageDiscoveryOwner::validate_installed_default_common_current(
    const detail::BnwNativeInstalledDefaultCommonSource& source)const {
    const auto storage=impl_;auto& i=*storage;auto& data=*source.impl_;
    const auto same=[](const auto& a,const auto& b){return !a.owner_before(b)&&!b.owner_before(a);};
    require_installed_default_common_identity_without_callbacks(source);
    need(data.checking.load()&&!data.active_validation,"Installed Common validation has no actual new source scope");
    std::unique_ptr<Impl::HeaderGuard> issued;
    if(!i.header_active){
        need(!i.active_header_fence&&!i.active_live_query_owner,"Installed Common found orphaned Header scope");
        issued=std::make_unique<Impl::HeaderGuard>(i.header_callback_state,i.self);
        need(issued->valid(),"Installed Common Header retired/reentered");
    }else{
        need(i.active_header_fence&&i.active_header_fence->valid()&&i.active_live_query_owner==i.live_resource_query.get(),
             "Installed Common cannot borrow unrelated active producer Header");
    }
    struct Active{Impl& i;bool own;Active(Impl& value,const Impl::HeaderGuard* fence):i(value),own(fence!=nullptr){
        if(own){i.header_active=true;i.active_header_fence=fence;}}
        ~Active(){if(own){i.active_header_fence=nullptr;i.header_active=false;}}} active{i,issued.get()};
    const detail::BnwNativeInstalledDefaultCommonSource::Impl::Validation context{source,*i.active_header_fence};
    data.active_validation=&context;
    struct ContextEnd{detail::BnwNativeInstalledDefaultCommonSource::Impl& data;~ContextEnd(){data.active_validation=nullptr;}} context_end{data};
    const auto check=[&]{require_installed_default_common_active_current(source);};
    const bool publication_check=data.publication_validation!=nullptr;
    if(!publication_check){
        need(data.returned_publication&&!data.busy.load(),"Installed source has no genuine normal publication return");
        check();data.input->require_current();check();
        const auto& boot=data.settings->common_operation_frame(*this);check();
        need(boot.normal_launch_source_closed&&!boot.leader_viewer&&boot.effective_texture_quality&&*boot.effective_texture_quality!=0&&
             GetACP()==data.code_page,"Installed retained namespace lost actual HIGH/code-page source");
        data.settings->validate_source();check();return;
    }
    need(data.publication_validation==data.publication_operation&&data.publication_operation&&data.busy.load()&&
         data.publication_operation->source.get()==&source&&data.publication_operation->thread==i.thread,
         "Installed full publication check lacks actual typed driver scope");
    need(i.default_state->selected.record_names==data.returned_names,
         "Installed full publication lost actual returned record-name projection");
    for(const auto& binding:data.selected_bindings){
        const auto found=i.default_state->selected.records.find(binding.key);
        need(found!=i.default_state->selected.records.end()&&&found->second==binding.actual&&
             found->second.index()==binding.returned_value.index(),"Installed published Default replaced actual moved record node");
        if(const auto* expected=std::get_if<assets::AssetHandle>(&binding.returned_value)){
            const auto* current=std::get_if<assets::AssetHandle>(&found->second);need(current,"Installed Default asset alternative changed");
            const auto& a=expected->candidate();const auto& b=current->candidate();
            need(a.original_name==b.original_name&&a.normalized_name==b.normalized_name&&a.payload_size==b.payload_size&&
                 a.provenance.kind==b.provenance.kind&&a.provenance.source_id==b.provenance.source_id&&
                 a.provenance.source_relative_path==b.provenance.source_relative_path&&
                 a.provenance.archive_relative_path==b.provenance.archive_relative_path,
                 "Installed published asset lost actual returned canonical provenance");
        }else{
            const auto& a=std::get<UnreadableLoose>(binding.returned_value);const auto& b=std::get<UnreadableLoose>(found->second);
            need(a.native_path==b.native_path&&a.relative==b.relative&&a.error==b.error&&
                 a.failed_original_stat.get()==b.failed_original_stat.get()&&same(a.failed_original_stat,b.failed_original_stat),
                 "Installed unreadable result changed actual Original stat/open source");
        }
    }
    const auto& names=i.first_lineage->storage_.names();
    for(const auto& [key,name]:data.returned_first){const auto found=names.find(key);
        need(found!=names.end()&&found->second==name,"Installed final FIRST overwrote/removed actual prior name");}
    for(const auto& [key,node]:i.default_state->loose_native){
        const auto record=i.default_state->selected.records.find(key);
        need(node&&record!=i.default_state->selected.records.end()&&node->selected_entry_==&record->second&&
             node->word_.crc==key&&node->word_.offset==0&&node->original_stat_,
             "Installed native loose word lost SAME actual selected record/stat");
    }
    check();data.input->require_current();check();
    need(scan(i.original)==i.journal,"Installed final namespace package source universe changed");check();
    for(const auto& file:i.pins){const auto now=runtime::identify_file(file.path);check();
        need(now.bytes==file.identity.bytes&&now.sha256==file.identity.sha256,"Installed final namespace retained metadata changed");}
    i.cold->require_same_input(*data.input);check();i.stat_service->require_same_input(*data.input);check();
    i.prefixes->require_same_input(*i.cold);check();i.entry_arena->require_same_input(*i.cold);check();
    i.archive_cache->require_same_arena(*i.entry_arena);check();i.current_default_source->require_current(*this);check();
    const auto whole=i.primary32_journal->current();check();
    need(whole.positive_primary32_complete&&whole.completed_archives==32&&i.primary32_steps.size()==32&&
         i.primary32_source_completed==32&&!i.primary32_failed,"Installed final namespace lost actual completed Primary32 source");
    for(const auto& step:i.primary32_steps){i.require_exact_step_without_callbacks(*step);check();}
    i.entry_arena->require_retained_common_projection_origin_without_callbacks(*i.prepared_default_projection->source,
        i.prepared_default_projection->records,i.common_steps,i.installation_common_storage->records);check();
    for(const auto& actual:i.common_steps){const auto& step=*actual;
        need(step.phase==detail::BnwNativeCommon13Step::Phase::terminal&&step.channel&&step.request&&step.lookup,
             "Installed final Common source step is not actual terminal result");
        step.channel->require_current();check();step.request->require_current();check();step.lookup->require_current();check();
        if(step.absent){const auto original=step.request->original_absolute_io_candidate();check();
            const auto attributes=GetFileAttributesA(native_loose_detail::acp_path(original).c_str());const auto error=GetLastError();
            need(attributes==INVALID_FILE_ATTRIBUTES&&(error==ERROR_FILE_NOT_FOUND||error==ERROR_PATH_NOT_FOUND),
                 "Installed final Common actual absence changed");check();
        }else{need(step.header&&step.prefix&&step.entries,"Installed final positive Common result missing actual objects");
            step.header->require_current();check();step.prefix->require_current();check();step.entries->require_current();check();
            if(step.publication){step.publication->require_current();check();}}
    }
    for(const auto& walk:i.loose_journals){need(loose_walk_matches(walk),"Installed returned DefaultLoose enumeration changed");check();}
    for(const auto& file:i.resource_stamps){need(stamp(file.path)==file,"Installed returned loose resource changed");check();}
    for(const auto& [key,node]:i.default_state->loose_native){
        node->original_stat_->require_same_service(*i.stat_service);check();const auto stat=node->original_stat_->current();check();
        need(stat.return_value==0&&stat.size&&*stat.size>=0&&node->word_.length==std::uint32_t(*stat.size)&&
             node->word_.packed_prefix==(node->prefix_index_<<20),"Installed returned loose native word/stat changed");
        need(i.prefixes->read_registered_loose_prefix(node->prefix_index_)==node->registered_prefix_,
             "Installed returned loose prefix identity changed");check();
    }
    for(const auto& [key,record]:i.default_state->selected.records)if(const auto* unavailable=std::get_if<UnreadableLoose>(&record)){
        need(native_missing_open(unavailable->native_path)==unavailable->error,"Installed unreadable source open result changed");check();
        if(unavailable->failed_original_stat){unavailable->failed_original_stat->require_same_service(*i.stat_service);check();
            const auto stat=unavailable->failed_original_stat->current();check();
            need(stat.return_value!=0&&!stat.size&&!stat.mtime&&i.default_state->loose_native.find(key)==i.default_state->loose_native.end(),
                 "Installed unknown stat acquired fabricated successful native words");}
    }
    const auto& boot=data.settings->common_operation_frame(*this);check();
    need(boot.normal_launch_source_closed&&!boot.leader_viewer&&boot.effective_texture_quality&&*boot.effective_texture_quality!=0&&
         GetACP()==data.code_page,"Installed returned Common lost actual normal HIGH/code-page source");
    data.settings->validate_source();check();data.input->require_current();check();
}
void BnwNativePackageDiscoveryOwner::publish_installation_default_common_source()const {
    const auto storage=impl_;auto& i=*storage;
    need(i.thread==std::this_thread::get_id(),"Installed final publication foreign creator");
    if(i.installed_default_loose_attempted&&!i.installed_default_common){
        i.header_poisoned=true;
        throw std::runtime_error("Installed final publication reentered before actual DefaultLoose issuer construction");
    }
    const auto source=i.installed_default_common;need(bool(source),"Installed final publication has no actual DefaultLoose issuer");
    auto& data=*source->impl_;
    if(data.checking.load()||data.metadata_checking.load()){
        source->fail();i.header_poisoned=true;throw std::runtime_error("Installed publication reentered actual source/metadata validation");
    }
    if(data.busy.exchange(true)){source->fail();i.header_poisoned=true;throw std::runtime_error("Installed final publication reentry");}
    struct Exit{detail::BnwNativeInstalledDefaultCommonSource::Impl& data;~Exit(){data.busy.store(false);}} exit{data};
    need(data.phase==detail::BnwNativeInstalledDefaultCommonSource::Impl::Phase::scanned&&!data.failed.load()&&
         !data.returned_publication&&data.completed_callers==3&&data.frame.resource_registration_order.size()==48&&
         !i.live_resource_query&&!i.category_source_attempted&&!i.global_color_bank,
         "Installed final publication requires actual three returned DefaultLoose callers once");
    data.phase=detail::BnwNativeInstalledDefaultCommonSource::Impl::Phase::publishing;
    struct Failure{std::shared_ptr<Impl> storage;const std::shared_ptr<detail::BnwNativeInstalledDefaultCommonSource>& source;bool done=false;
        ~Failure(){if(!done){storage->header_poisoned=true;source->fail();}}} failure{storage,source};
    Impl::InstallationParentLoan retained(i);const auto& actual=retained.parent;
    need(actual&&actual.get()==this,"Installed final publication lost canonical Parent");
    detail::BnwNativeInstalledDefaultCommonSource::PublicationOperation operation(source);
    // Allocate the receipt before commit; it grants nothing until installed
    // after the real typed publication POST has returned normally.
    auto normal_return=std::make_unique<const detail::BnwNativeInstalledDefaultCommonSource::Impl::NormalReturn>(source);
    {
        BnwPrimaryDefaultStageTrace trace("Installed.DefaultCommon.publish");
        i.validate_installation_prefix_current();
        Impl::HeaderGuard fence(i.header_callback_state,i.self);need(fence.valid(),"Installed final publication Header retired/reentered");
        struct Active{Impl& i;Active(Impl& value,const Impl::HeaderGuard& fence):i(value){i.header_active=true;i.active_header_fence=&fence;}
            ~Active(){i.active_header_fence=nullptr;i.header_active=false;}} active{i,fence};
        const auto check=[&]{i.require_active_header_without_callbacks();require_pre_dx9_discovery_current(*i.pre_dx9_discovery);
            need(!data.failed.load()&&data.busy.load()&&data.publication_operation==&operation&&
                 data.phase==detail::BnwNativeInstalledDefaultCommonSource::Impl::Phase::publishing&&
                 i.installed_default_common.get()==source.get()&&i.installation_common_storage.get()==data.common_storage&&
                 i.common_steps.size()==13&&!i.common_pending&&!i.common_driver_failed&&!i.live_resource_query,
                 "Installed actual publication source/controller changed before commit");};
        check();i.installation_input->require_current();check();
        i.entry_arena->require_retained_common_projection_origin_without_callbacks(*i.prepared_default_projection->source,
            i.prepared_default_projection->records,i.common_steps,i.installation_common_storage->records);check();
        for(const auto& walk:data.walks){need(loose_walk_matches(walk),"Installed loose enumeration changed before final publication");check();}
        for(const auto& file:data.stamps){need(stamp(file.path)==file,"Installed loose payload changed before final publication");check();}
        for(const auto& [key,node]:data.native_loose){
            node->original_stat_->require_same_service(*i.stat_service);check();const auto stat=node->original_stat_->current();check();
            need(stat.return_value==0&&stat.size&&*stat.size>=0&&node->word_.length==std::uint32_t(*stat.size)&&
                 node->word_.offset==0&&node->word_.packed_prefix==(node->prefix_index_<<20),
                 "Installed staged loose words changed actual Original stat");
            need(i.prefixes->read_registered_loose_prefix(node->prefix_index_)==node->registered_prefix_,
                 "Installed staged loose prefix changed");check();
        }
        for(const auto& [key,record]:data.records.records)if(const auto* unavailable=std::get_if<UnreadableLoose>(&record)){
            need(native_missing_open(unavailable->native_path)==unavailable->error,"Installed staged unknown/open result changed");check();
            if(unavailable->failed_original_stat){const auto stat=unavailable->failed_original_stat->current();check();
                need(stat.return_value!=0&&!stat.size&&!stat.mtime&&data.native_loose.find(key)==data.native_loose.end(),
                     "Installed staged unknown acquired fabricated successful words");}
        }
        auto first=i.first_lineage->prepare_extension(data.records.first_record_names);check();
        data.selected_bindings.reserve(data.records.records.size());
        for(const auto& [key,record]:data.records.records)data.selected_bindings.push_back({key,&record,record});
        data.returned_names=data.records.record_names;data.returned_first=first;
        data.returned_registration_order=data.frame.resource_registration_order;
        for(auto& [key,node]:data.native_loose){const auto row=data.records.records.find(key);
            need(row!=data.records.records.end(),"Installed staged native word lacks actual selected row");node->selected_entry_=&row->second;}
        data.frame.common_high_quality_registration_complete=true;
        data.frame.next_resource_dependency="54CD3E64C610/656440 then52D590 actual global Color/category/Skin source";
        decltype(i.registry.unresolved_provider) unresolved{
            BnwNativeColorUnresolvedProvider{std::make_shared<const std::uint8_t>(0),data.frame.next_resource_dependency}};
        auto warm=std::shared_ptr<detail::BnwNativeGlobalResourceWarmOwner>(new detail::BnwNativeGlobalResourceWarmOwner(i.self,i.identity,source));
        i.settings->validate_source();check();need(GetACP()==data.code_page,"Installed actual code page changed before final commit");
        need(i.registry.revision!=UINT64_MAX,"Installed publication revision overflow");
        // Same-allocator moves preserve selected record addresses. No source
        // callback or allocation occurs through phase commit.
        static_assert(std::is_nothrow_move_assignable_v<decltype(i.default_state->selected)>);
        static_assert(std::is_nothrow_move_assignable_v<decltype(i.default_state->loose_native)>);
        static_assert(std::is_nothrow_move_assignable_v<decltype(i.resource_stamps)>);
        static_assert(std::is_nothrow_move_assignable_v<decltype(i.absent_archives)>);
        static_assert(std::is_nothrow_move_assignable_v<decltype(i.loose_journals)>);
        static_assert(std::is_nothrow_move_assignable_v<decltype(i.frame)>);
        static_assert(std::is_nothrow_move_assignable_v<decltype(i.registry.unresolved_provider)>);
        i.first_lineage->adopt_prepared(first);
        i.default_state->selected=std::move(data.records);i.default_state->loose_native=std::move(data.native_loose);
        i.resource_stamps=std::move(data.stamps);i.absent_archives=std::move(data.absent);i.loose_journals=std::move(data.walks);
        i.loose_code_page=data.code_page;i.frame=std::move(data.frame);++i.registry.revision;
        data.publication_revision=i.registry.revision;i.registry.unresolved_provider=std::move(unresolved);
        i.live_resource_query=std::move(warm);data.phase=detail::BnwNativeInstalledDefaultCommonSource::Impl::Phase::returned;
    }
    // Full real publication POST, including each returned record, cached
    // archive source, actual stat/prefix and file/journal checks, happens here.
    // Ordinary Warm.current refuses while this private driver is still busy.
    source->require_publication_current(operation);
    require_pre_dx9_discovery_current(*i.pre_dx9_discovery);
    data.returned_publication=std::move(normal_return);failure.done=true;
}
void BnwNativePackageDiscoveryOwner::refuse_installed_default_rewrite()const {
    const auto storage=impl_;need(storage->thread==std::this_thread::get_id(),"Installed Default mutation foreign creator");
    const auto source=storage->installed_default_common;
    if(source){source->fail();storage->header_poisoned=true;
        throw std::runtime_error("Installed retained Default requires its own actual mutation source before another Base Default write");}
}
