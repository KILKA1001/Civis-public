// Parent-only actual82A470 top-FPK continuation, after this SAME Core's real
// normal1 loose recursion. No supplied path, category Arena, or table seed.
void BnwNativePackageDiscoveryOwner::execute_installed_late_default_archive(
    const detail::BnwNativeInstalledLateDefaultOperation& operation)const {
    const auto& held=*operation.impl_;auto& i=*held.storage;auto& directory=held.directory;auto& source=*held.source;
    using Effect=Impl::InstalledLateDefaultSuccessor::ArchiveEffect;
    const auto pure=[&]{operation.require_fence_without_callbacks(*this);
        need(&held.parent==this&&directory.root_appended&&directory.loose_returned&&!directory.archives_returned,
             "Late Default FPK producer escaped actual returned loose/Core operation");};
    const auto current=[&]{pure();operation.require_active_core_current(*this);pure();};
    current();need(directory.archives.empty()&&directory.top_fpk_entries.empty()&&!directory.active_archive&&
         directory.top_fpk_end_error==ERROR_SUCCESS,"Late Default top-FPK enumeration replay");
    need(operation.normal_default_loose_mode(*this)==1,"Late Default FPK requires actual normal1 source");current();
    const auto& emitted=operation.emitted_speech_path(*this);
    need(emitted==directory.query,"Late Default FPK changed SAME actual held62EC20 output");
    const auto query=native_category_directory_detail::normalized_query_ascii(emitted);
    const auto root=native_category_source_detail::source_path(i.original.parent_path(),
        std::filesystem::path(native_loose_detail::acp_wide(query)));current();
    const auto load=[&](const std::string& actual_name){
        current();need(!directory.active_archive&&source.attempts!=SIZE_MAX,"Late Default actual archive attempt exhausted/reentered");
        auto attempt=std::shared_ptr<const detail::BnwNativeInstalledLateDefaultArchiveAttempt>(
            new detail::BnwNativeInstalledLateDefaultArchiveAttempt(*i.cold,operation,*this,actual_name));
        current();auto owned=std::make_unique<Effect>(attempt);current();
        directory.archives.push_back(std::move(owned));++source.attempts;
        auto& effect=*directory.archives.back();
        struct Active {
            Impl::InstalledLateDefaultSuccessor& source;
            Impl::InstalledLateDefaultSuccessor::DirectoryEffects& directory;Effect& effect;
            Active(Impl::InstalledLateDefaultSuccessor& actual,
                   Impl::InstalledLateDefaultSuccessor::DirectoryEffects& dir,Effect& entry)
                :source(actual),directory(dir),effect(entry){
                need(!directory.active_archive,"Late Default archive attempt nested another actual source");directory.active_archive=&effect;
            }
            ~Active(){if(directory.active_archive!=&effect)source.phase=Impl::InstalledLateDefaultSuccessor::Phase::failed;
                directory.active_archive=nullptr;}
        } active(source,directory,effect);
        current();i.frame.resource_registration_order.push_back(attempt->relative_ansi_);current();
        ++i.frame.archive_registration_requests;
        effect.request=i.cold->request_from_installed_late_default(attempt);effect.phase=Effect::Phase::request;current();
        // Native82A331/336 checks the SAME basename cache before source open.
        effect.lookup=i.archive_cache->lookup_before_constructor(effect.request,*this);effect.phase=Effect::Phase::lookup;current();
        if(effect.lookup->hit()){
            current();effect.lookup->require_current();current();
            effect.loader_return=0;effect.phase=Effect::Phase::returned;current();return;
        }
        current();const auto original=effect.request->original_absolute_io_candidate();current();
        const auto attributes=GetFileAttributesA(native_loose_detail::acp_path(original).c_str());
        if(attributes==INVALID_FILE_ATTRIBUTES){
            const auto error=GetLastError();current();
            need(error==ERROR_FILE_NOT_FOUND||error==ERROR_PATH_NOT_FOUND,
                 "Late Default archive absence has unsupported Original IO error");
            directory.absent.push_back(original);current();effect.loader_return=0;effect.phase=Effect::Phase::returned;current();return;
        }
        current();need(!(attributes&FILE_ATTRIBUTE_DIRECTORY)&&!(attributes&FILE_ATTRIBUTE_REPARSE_POINT),
             "Late Default archive source is not an ordinary Original file");
        effect.construction=detail::BnwNativeArchiveConstruction::begin(i.cold,i.installation_input,i.stat_service,effect.request);
        effect.phase=Effect::Phase::construction;current();
        effect.header=detail::BnwNativeArchiveV6StreamHeaderOwner::acquire(i.cold,effect.request,effect.construction,i.stat_service);
        effect.phase=Effect::Phase::header;current();
        effect.prefix=i.prefixes->register_header_prefix(effect.header);effect.phase=Effect::Phase::prefix;current();
        effect.prefix->prepare_bound_construction_metadata();current();
        effect.entries=detail::BnwNativeArchiveV6EntryAdmissionOwner::acquire_installed_late_default(
            i.entry_arena,effect.header,effect.prefix,attempt,operation,*this);
        effect.phase=Effect::Phase::entries;current();
        const auto count=effect.entries->admitted_count();current();
        if(!count){effect.loader_return=count;effect.phase=Effect::Phase::returned;current();return;}
        effect.publication=i.archive_cache->publish_after_entries(effect.lookup,effect.entries,*this);current();
        effect.publication->require_current();current();
        // Genuine constructor83C7EA +280=1 remains the existing Construction
        // field; no category preference setter or manufactured cached time.
        need(effect.construction->checked_preference280(),"Late Default actual constructor preference280 changed");current();
        ++i.frame.archives_registered; // Actual positive entry returns and SAME cache publication, never hit/absence0.
        effect.loader_return=count;effect.phase=Effect::Phase::returned;current();
    };
    {
        native_loose_detail::NativeEnumeration enumeration;
        auto cursor=enumeration.open(native_loose_detail::acp_path(root)+"\\*.FPK");
        if(!cursor.valid()){
            directory.top_fpk_end_error=cursor.error();current();
            need(directory.top_fpk_end_error==ERROR_FILE_NOT_FOUND||directory.top_fpk_end_error==ERROR_PATH_NOT_FOUND,
                 "Late Default actual top-FPK FindA failed with unsupported error");
        }else{
            do{
                current();const auto& event=cursor.data;const std::string name=event.cFileName;
                native_category_patterns_detail::ascii_input(name);
                directory.top_fpk_entries.push_back({std::filesystem::path(native_loose_detail::acp_wide(name)),name,
                    event.dwFileAttributes,(std::uint64_t(event.nFileSizeHigh)<<32)|event.nFileSizeLow,
                    (std::uint64_t(event.ftLastWriteTime.dwHighDateTime)<<32)|event.ftLastWriteTime.dwLowDateTime,name});
                current();
                if((event.dwFileAttributes&0x16u)==0){
                    need(!(event.dwFileAttributes&FILE_ATTRIBUTE_REPARSE_POINT),"Late Default top-FPK reparse source unsupported");
                    load(name);current();
                }
            }while(cursor.next());
            directory.top_fpk_end_error=cursor.error(); // BEFORE callbacks/FindClose can change LastError.
            need(directory.top_fpk_end_error==ERROR_NO_MORE_FILES,"Late Default actual top-FPK traversal incomplete");
        }
    } // Real OS cursor retires before physical/namespace POST.
    current();need(!directory.active_archive,"Late Default archive source escaped actual attempt cleanup");
    for(const auto& effect:directory.archives){
        need(effect&&effect->phase==Effect::Phase::returned&&effect->loader_return.has_value(),
             "Late Default incidental FPK omitted actual native loader return");current();
    }
    i.frame.default_indexed_names=i.default_state->selected.records.size();
    i.frame.resource_crc_collisions=i.default_state->selected.collisions;current();
    directory.archives_returned=true; // The real per-FPK operations and final physical POST returned.
}
