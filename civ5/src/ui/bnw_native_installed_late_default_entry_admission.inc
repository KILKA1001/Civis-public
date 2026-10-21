// Same nonempty Default Arena; no category namespace/initialization or copied
// native table. The actual normal1 call supplies entry mode2 (83CB1A/83CC9E).
void BnwNativeArchiveEntryAdmissionArena::require_installed_late_archive_record_identity_without_callbacks(
    const BnwNativeInstalledLateDefaultArchiveAttempt& attempt,
    const BnwNativeArchiveV6StreamHeaderOwner& header,const BnwNativeGlobalArchivePrefixRecord& prefix,
    const std::shared_ptr<const assets::FpkSource>& source,const assets::FpkEntry& entry,
    const assets::AssetHandle& handle,const BnwNativeArchiveRecordDiagnostic& words)const {
    auto& state=*impl_;state.require_thread_before_snapshot();
    need(!state.failed&&!state.category_namespace&&state.active_late_default_attempt==&attempt&&
         state.active_late_default_fence&&state.active_late_default_fence->valid(),
         "Late Default native projection escaped SAME actual Arena entry store");
    const auto crc=bnw_native_admission_ascii_crc(entry.original_name);
    const auto record=state.kernel.records().find(crc);
    const auto selected=state.selected_inputs.find(crc);
    const auto canonical=state.canonical_selected.find(crc);
    need(record!=state.kernel.records().end()&&&record->second==&words&&
         selected!=state.selected_inputs.end()&&canonical!=state.canonical_selected.end()&&&canonical->second==&handle&&
         source&&selected->second.source.get()==source.get()&&
         !selected->second.source.owner_before(source)&&!source.owner_before(selected->second.source)&&
         selected->second.entry==&entry&&selected->second.header==&header&&selected->second.prefix==&prefix&&
         selected->second.archive_ordinal==attempt.ordinal_&&selected->second.entry_ordinal==entry.ordinal&&
         entry.ordinal<source->entries().size()&&&source->entries()[entry.ordinal]==&entry&&
         words.crc==crc&&words.length==entry.payload_length&&words.offset==entry.payload_offset&&
         words.packed_prefix==(selected->second.prefix_index<<20),
         "Late Default projection is not SAME actual native/canonical/source-entry/Header/Prefix store");
    need(state.active_late_default_fence->valid(),"Late Default Arena fence changed during pure projection proof");
}

std::shared_ptr<const BnwNativeArchiveV6EntryAdmissionOwner>
BnwNativeArchiveV6EntryAdmissionOwner::acquire_installed_late_default(
    std::shared_ptr<const BnwNativeArchiveEntryAdmissionArena> arena,
    std::shared_ptr<const BnwNativeArchiveV6StreamHeaderOwner> header,
    std::shared_ptr<const BnwNativeGlobalArchivePrefixRecord> prefix,
    std::shared_ptr<const BnwNativeInstalledLateDefaultArchiveAttempt> attempt,
    const BnwNativeInstalledLateDefaultOperation& operation,
    const ::civ5::ui::BnwNativePackageDiscoveryOwner& parent) {
    need(arena&&header&&prefix&&attempt,"Late Default entry admission lacks actual retained suppliers");
    parent.require_archive_entry_operation_current(*arena,*header,*prefix);
    operation.require_active_core_current(parent);attempt->require_active_request();
    need(operation.normal_default_loose_mode(parent)==1,"Late Default archive requires actual normal1 Boot operand");
    auto& state=*arena->impl_;auto fence=state.guard();state.check(fence,2);
    struct Failure {BnwNativeArchiveEntryAdmissionArena::Impl& state;bool done=false;
        ~Failure(){if(!done)state.failed=true;}} failure{state};
    need(!state.category_namespace&&state.installation_input&&!state.lease&&state.cache_claim_attempted&&
         state.current_default_source_claimed&&state.next_primary_ordinal==32&&!state.active_late_default_attempt,
         "Late Default admission requires SAME published installed Default Arena");
    const auto cold=state.validate(fence,2);
    const auto pure=[&]{state.check(fence,2);operation.require_fence_without_callbacks(parent);
        parent.require_installed_late_default_attempt_identity_without_callbacks(*attempt,true);
        parent.require_archive_entry_operation_current(*arena,*header,*prefix);
        need(fence.valid_parent(cold),"Late Default entry lost persistent actual Cold");};
    const auto current=[&]{pure();attempt->require_active_request();pure();};
    current();attempt->require_same_input(*cold);current();
    const auto& request=header->source_request();current();request.require_same_owner(*cold);current();
    request.require_same_installed_late_default_attempt(*attempt);current();
    const auto& actual_header=prefix->header_owner();current();
    need(&actual_header==header.get(),"Late Default entry Prefix belongs to another Header");
    const auto ordinal=request.source_ordinal();current();
    need(ordinal==attempt->ordinal_&&(!state.last_late_default_ordinal||ordinal>*state.last_late_default_ordinal),
         "Late Default actual archive attempt repeated/out of source order");
    state.last_late_default_ordinal=ordinal;
    const auto& construction=header->construction();current();
    const auto source=construction.canonical_payload_source();current();
    const auto declared=header->header_fields().declared_entry_count;current();
    const auto bytes=header->original_byte_length();current();const auto prefix_index=prefix->source_index();current();
    need(source->filename_scrambling()==0&&source->extra_header_byte()==0,
         "Late Default archive requires actual finite transform0 source");
    bnw_native_v6_entry_inputs(source->entries(),declared,source->directory_end(),bytes);
    auto receipt=std::make_unique<Impl>();receipt->arena=arena;receipt->header=header;receipt->prefix=prefix;
    receipt->source=source;receipt->ordinal=ordinal;receipt->declared=declared;receipt->installed_late_attempt=attempt;
    receipt->entry_returns.reserve(declared);
    std::vector<assets::AssetHandle> handles;handles.reserve(declared);
    std::unordered_map<std::string,std::size_t> occurrences;
    for(const auto& entry:source->entries()) {
        const auto all=source->find_all(assets::AssetKey(entry.original_name));auto& next=occurrences[entry.normalized_name];
        need(next<all.size(),"Late Default canonical payload ordinal absent");
        const auto& handle=all[next++];
        need(handle.candidate().original_name==entry.original_name&&handle.candidate().payload_size==entry.payload_length,
             "Late Default canonical payload differs from actual prepared directory entry");
        handles.push_back(handle);current();
    }
    header->require_current();current();prefix->require_current();current();ascii_locale();
    {
        struct Active {
            BnwNativeArchiveEntryAdmissionArena::Impl& state;
            Active(BnwNativeArchiveEntryAdmissionArena::Impl& value,
                   const BnwNativeInstalledLateDefaultArchiveAttempt& attempt,
                   const BnwNativeArchiveEntryAdmissionArena::Impl::Guard& held):state(value) {
                need(!state.active_late_default_attempt&&!state.active_late_default_fence,
                     "Late Default native entry loop reentry");
                state.active_late_default_attempt=&attempt;state.active_late_default_fence=&held;
            }
            ~Active(){state.active_late_default_attempt=nullptr;state.active_late_default_fence=nullptr;}
        } active(state,*attempt,fence);
        for(const auto& entry:source->entries()) {
            // Native/FIRST, canonical, source witness and selected projection
            // form ONE actual store. No FULL callback observes a partial row.
            pure();const auto crc=bnw_native_admission_ascii_crc(entry.original_name);
            parent.begin_installed_late_default_selected_store_without_callbacks(operation,crc);pure();
            const auto result=state.kernel.admit(entry.original_name,prefix_index,
                static_cast<std::uint32_t>(entry.payload_length),static_cast<std::uint32_t>(entry.payload_offset),2);
            need(result==1,"Late Default actual normal1 archive entry store returned unsupported zero");
            pure();state.canonical_selected.insert_or_assign(crc,handles[entry.ordinal]);pure();
            state.selected_inputs.insert_or_assign(crc,BnwNativeArchiveEntryAdmissionArena::Impl::SelectedInput{
                source,&entry,header.get(),prefix.get(),ordinal,entry.ordinal,prefix_index});pure();
            const auto& words=state.kernel.records().at(crc);const auto& actual_handle=state.canonical_selected.at(crc);
            parent.commit_installed_late_default_archive_projection(operation,*attempt,*header,*prefix,source,entry,actual_handle,words);
            pure();receipt->entry_returns.push_back(result);pure();receipt->admitted+=result;
        }
    }
    current();need(state.kernel.records().size()==state.canonical_selected.size()&&
         state.kernel.records().size()==state.selected_inputs.size(),"Late Default completed native/archive witness projection differs");
    auto out=std::shared_ptr<const BnwNativeArchiveV6EntryAdmissionOwner>(new BnwNativeArchiveV6EntryAdmissionOwner(std::move(receipt)));
    out->impl_->self=out;current();failure.done=true;return out;
}

void BnwNativeArchiveV6EntryAdmissionOwner::require_installed_late_default_origin_without_callbacks(
    const BnwNativeArchiveEntryAdmissionArena& arena,const BnwNativeInstalledLateDefaultArchiveAttempt& attempt)const {
    const auto& value=*impl_;
    need(!value.arena.expired()&&!value.arena.owner_before(arena.impl_->self)&&!arena.impl_->self.owner_before(value.arena)&&
         !value.installed_late_attempt.expired()&&value.installed_late_attempt.lock().get()==&attempt&&
         value.category_channel.expired()&&value.header&&value.prefix&&value.source,
         "Late Default entry receipt changed SAME Arena/attempt/original suppliers");
}
