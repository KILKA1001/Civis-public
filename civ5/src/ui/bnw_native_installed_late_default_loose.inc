// Parent-only actual normal1 continuation. The closed operation supplies the
// genuine held Core/62EC20 output and SAME Default; there is no path/flags API.
void BnwNativePackageDiscoveryOwner::execute_installed_late_default_loose(
    const detail::BnwNativeInstalledLateDefaultOperation& operation)const {
    const auto& held=*operation.impl_;auto& i=*held.storage;auto& directory=held.directory;
    const auto pure=[&]{operation.require_fence_without_callbacks(*this);
        need(&held.parent==this&&directory.root_appended&&!directory.loose_returned&&
             !directory.archives_returned&&!directory.active_archive,
             "Late Default loose producer escaped actual root/Core/directory phase");};
    const auto current=[&]{pure();operation.require_active_core_current(*this);pure();};
    current();need(directory.walks.empty()&&directory.folder_prefixes.empty()&&directory.loose_stats.empty(),
         "Late Default loose producer replayed its actual source enumeration");
    const auto mode=operation.normal_default_loose_mode(*this);current();
    need(mode==1,"Late Default loose requires actual fresh normal1 Boot operand");
    // THIS closed54CDF3 ->82A470(path,0,normal1,0,0) role forwards NULL
    // exclusion/override at82A5F5/F8 ->83A720. No caller filter API exists.
    const auto& emitted=operation.emitted_speech_path(*this);
    need(emitted==directory.query,"Late Default loose changed actual held62EC20 output");
    const auto root_query=native_category_directory_detail::normalized_query_ascii(emitted);
    const auto relative_root=std::filesystem::path(native_loose_detail::acp_wide(root_query));
    const auto root=native_category_source_detail::source_path(i.original.parent_path(),relative_root);current();
    const auto root_ansi=native_loose_detail::acp_path(root);
    // Actual829560/83A720 branches on the ANSI attribute probe, not on a
    // filesystem exists/is_directory surrogate. INVALID returns native0
    // before either Prefix registration or FindA.
    const auto root_attributes=GetFileAttributesA(root_ansi.c_str());
    LooseWalk walk{root,false,root_attributes!=INVALID_FILE_ATTRIBUTES,{},GetACP(),true};current();
    ++i.frame.loose_registration_requests;i.frame.resource_registration_order.push_back(emitted);current();
    assets::LooseResourceIndex canonical(i.shared);
    const auto observe_return=[&](bool stored){
        directory.loose_returns.push_back(stored?1u:0u); // Real mode1 insert return, never declared file count.
        current();
    };
    const auto admit=[&](const native_loose_detail::LooseEntry& entry){
        if(entry.attributes&FILE_ATTRIBUTE_DIRECTORY)return;
        current();++i.frame.loose_file_registration_requests;
        const auto slash=entry.raw_relative.find_last_of('\\');
        const auto child=slash==std::string::npos?std::string{}:entry.raw_relative.substr(0,slash);
        const auto registered=directory.folder_prefixes.find(child);
        need(registered!=directory.folder_prefixes.end(),"Late Default FindA entry lacks SAME actual folder Prefix");
        const auto relative=std::filesystem::path(native_loose_detail::acp_wide(root_query+entry.raw_relative));
        const auto source=native_category_source_detail::source_path(i.original.parent_path(),relative);current();
        const auto mapped=native_category_source_detail::source_path(i.shared,relative);current();
        const auto stat=i.stat_service->read_original_relative(relative);current();
        directory.loose_stats.push_back(stat);current();const auto fields=stat->current();current();
        const auto* locale=std::setlocale(LC_CTYPE,nullptr);
        need(locale&&std::string_view(locale)=="C","Late Default loose CRC requires actual C CRT locale");
        const auto crc=DefaultRecords::source_hash(entry.native_name);
        const auto store=[&](auto incoming,std::shared_ptr<DefaultLooseNativeNode> node){
            current();stat->require_same_service(*i.stat_service);current();
            need(stat->current()==fields,"Late Default Original stat changed before actual selected store");current();
            const auto* actual_locale=std::setlocale(LC_CTYPE,nullptr);
            need(actual_locale&&std::string_view(actual_locale)=="C","Late Default loose store lost actual C CRT locale");
            const bool preserve_archive=installed_late_default_selected_is_archive_without_callbacks(operation,crc);
            if(!preserve_archive)begin_installed_late_default_selected_store_without_callbacks(operation,crc);
            pure();
            const bool stored=i.default_state->selected.insert(entry.native_name,std::move(incoming),mode);
            need(stored!=preserve_archive,"Late Default actual normal1 insert disagrees with current selected source kind");
            if(stored){
                if(node){node->selected_entry_=&i.default_state->selected.records.at(crc);
                    i.default_state->loose_native.insert_or_assign(crc,std::move(node));}
                else i.default_state->loose_native.erase(crc); // UNKNOWN stat never inherits successful native words.
                i.first_lineage->remember(crc,entry.native_name);
                record_installed_late_default_selected_store(operation,crc);
                ++i.frame.loose_files_registered;
            }
            pure();stat->require_same_service(*i.stat_service);current();
            need(stat->current()==fields,"Late Default Original stat changed after actual selected store");current();
            observe_return(stored);
        };
        const auto relative_name=assets::path_utf8(relative);
        if(fields.return_value!=0&&!fields.size&&!fields.mtime){
            const auto error=native_missing_open(source);current();
            need(error==ERROR_FILE_NOT_FOUND||error==ERROR_PATH_NOT_FOUND,
                 "Late Default failed Original stat requires genuine reconstructed-path missing open");
            store(UnreadableLoose{source,relative_name,error,
                {relative_name,entry.native_name,entry.bytes,entry.written,entry.attributes,error},stat},{});
            return;
        }
        need(fields.return_value==0&&fields.size&&*fields.size>=0&&fields.mtime,
             "Late Default Original stat words unknown; no successful native length can be issued");
        const auto node=[&]{
            current();auto result=std::shared_ptr<DefaultLooseNativeNode>(new DefaultLooseNativeNode);
            result->word_.crc=crc;result->word_.length=static_cast<std::uint32_t>(*fields.size);result->word_.offset=0;
            result->prefix_index_=registered->second.first;result->word_.packed_prefix=result->prefix_index_<<20;
            result->original_find_basename_=entry.native_name;result->original_folder_query_=root_query+(child.empty()?"":child+'\\');
            result->registered_prefix_=registered->second.second;result->original_stat_=stat;current();return result;
        };
        if(!std::filesystem::exists(source)){
            const auto error=native_missing_open(source);current();
            need(error==ERROR_FILE_NOT_FOUND||error==ERROR_PATH_NOT_FOUND,
                 "Late Default unavailable source requires actual missing open");
            store(UnreadableLoose{source,relative_name,error,
                {relative_name,entry.native_name,entry.bytes,entry.written,entry.attributes,error}},node());return;
        }
        current();need(std::filesystem::is_regular_file(mapped),"Late Default canonical loose mirror missing");current();
        const auto original_stamp=stamp(source),mapped_stamp=stamp(mapped);current();
        const auto original_bytes=runtime::identify_file(source);current();const auto mapped_bytes=runtime::identify_file(mapped);current();
        need(original_bytes.bytes==mapped_bytes.bytes&&original_bytes.sha256==mapped_bytes.sha256&&
             original_bytes.bytes==static_cast<std::uint64_t>(*fields.size),
             "Late Default canonical payload differs from actual Original bytes/stat");
        need(stamp(source)==original_stamp&&stamp(mapped)==mapped_stamp,"Late Default loose source changed during byte qualification");current();
        directory.stamps.push_back(original_stamp);directory.stamps.push_back(mapped_stamp);current();
        canonical.add_file(mapped);current();const auto found=canonical.find(assets::AssetKey(entry.native_name));current();
        const auto* handle=std::get_if<assets::AssetHandle>(&found);
        need(handle&&handle->candidate().provenance.kind==assets::SourceKind::loose,
             "Late Default actual canonical loose handle unavailable/ambiguous");
        store(*handle,node());
    };
    if(walk.exists){
        native_loose_detail::NativeEnumeration enumeration;
        native_loose_detail::visit_loose_entries_with_folder(root_ansi,false,enumeration,
            [&](const std::string& raw,const std::string& name,DWORD attributes,std::uint64_t bytes,std::uint64_t written){
                pure();walk.entries.push_back({std::filesystem::path(native_loose_detail::acp_wide(raw)),name,attributes,bytes,written,raw});
                admit(walk.entries.back());pure();
            },[&](const std::string& child){
                current();
                if(!child.empty()){
                    const auto folder=native_category_source_detail::source_path(i.original.parent_path(),
                        std::filesystem::path(native_loose_detail::acp_wide(root_query+child)));
                    // Recursive83A720 probes again before Prefix. A directory
                    // lost since its parent FindA is an unqualified changing
                    // traversal here, and must not append a spurious Prefix.
                    need(GetFileAttributesA(native_loose_detail::acp_path(folder).c_str())!=INVALID_FILE_ATTRIBUTES,
                         "Late Default recursive directory changed before actual Prefix");
                }
                const auto query=child.empty()?root_query:root_query+child+'\\';
                const auto wide=native_loose_detail::acp_wide(query);const std::u16string raw(wide.begin(),wide.end());
                // The registry invokes only the real callback-free donor fence
                // under its OWN guard. Full physical checks bracket this call.
                const auto index=i.prefixes->register_loose_source_prefix(raw,pure);current();
                directory.folder_prefixes.emplace(child,std::make_pair(index,raw));current();
            },"*.*");
    }else{directory.absent.push_back(root);current();} // Genuine attrs-INVALID branch: no Prefix/FindA.
    current();need(GetACP()==walk.code_page&&loose_walk_matches(walk),"Late Default loose traversal changed before actual return");current();
    for(const auto& before:directory.stamps){need(stamp(before.path)==before,"Late Default selected mirror changed before actual return");current();}
    directory.walks.push_back(std::move(walk));current();
    need(directory.loose_returns.size()==directory.loose_stats.size(),"Late Default actual stat/insert return ledger differs");
    i.frame.default_indexed_names=i.default_state->selected.records.size();
    i.frame.resource_crc_collisions=i.default_state->selected.collisions;current();
    directory.loose_returned=true; // Only after real enumeration/store/FULL POST.
}
