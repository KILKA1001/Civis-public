// Included only after both real Cold/request Impl definitions. This distinct
// source origin is never an initial/Common/category ordinal or ready flag.
BnwNativeInstalledLateDefaultArchiveAttempt::BnwNativeInstalledLateDefaultArchiveAttempt(
    const BnwNativeColdFilesystemOwner& cold,const BnwNativeInstalledLateDefaultOperation& operation,
    const ::civ5::ui::BnwNativePackageDiscoveryOwner& parent,const std::string& actual_find_basename)
    :parent_(operation.parent_without_callbacks()),cold_(cold.impl_->self),
     source_brand_(operation.source_brand_without_callbacks()),core_brand_(operation.core_brand_without_callbacks()),
     request_brand_(std::make_shared<const std::uint8_t>(0)),
     core_identity_(operation.core_identity_without_callbacks()),ordinal_(operation.next_archive_ordinal_without_callbacks()),
     relative_ansi_([&]{
         operation.require_fence_without_callbacks(parent);
         need(&operation.cold_without_callbacks(parent)==&cold&&cold.impl_->installation_input&&!cold.impl_->lease,
              "Late Default attempt lost SAME installed Cold source");
         need(!actual_find_basename.empty()&&actual_find_basename.size()<=127&&
              actual_find_basename.find_first_of("\\/:*?\0",0,6)==std::string::npos&&
              std::all_of(actual_find_basename.begin(),actual_find_basename.end(),[](unsigned char c){return c>=32&&c<127;}),
              "Late Default FPK requires its actual finite FindA basename");
         std::string request=operation.emitted_speech_path(parent);
         need(!request.empty()&&request.size()<=259&&
              std::all_of(request.begin(),request.end(),[](unsigned char c){return c>=32&&c<127;}),
              "Late Default emitted source path lacks finite native ASCII owner");
         std::replace(request.begin(),request.end(),'/','\\');
         if(request.back()!='\\')request+='\\';
         request+=actual_find_basename;
         need(request.size()<=259,"Late Default FPK stream path exceeds finite native extent");
         operation.require_fence_without_callbacks(parent);return request;
     }()),
     raw_stream_(bnw_primary_archive_ascii_stream_token(relative_ansi_)),relative_(primary_relative_io_path(raw_stream_)),
     original_(confined_original_stat_path(cold.impl_->original_source_base,relative_)),
     canonical_(confined_original_stat_path(cold.impl_->canonical_payload_base,relative_)) {
    need(source_brand_&&core_brand_&&request_brand_&&core_identity_&&ordinal_!=SIZE_MAX,
         "Late Default FPK attempt lacks its genuine source/Core/ordinal brands");
    operation.require_fence_without_callbacks(parent);
}

std::shared_ptr<const BnwNativeColdArchiveRequest>
BnwNativeColdFilesystemOwner::request_from_installed_late_default(
    std::shared_ptr<const BnwNativeInstalledLateDefaultArchiveAttempt> attempt)const {
    need(GetCurrentProcessId()==impl_->epoch->process&&GetCurrentThreadId()==impl_->epoch->thread,
         "Late Default archive request foreign process/thread");
    need(impl_->installation_input&&!impl_->lease&&attempt&&attempt->privatebrand_without_callbacks(),
         "Late Default archive request lacks SAME sealed installed source attempt");
    need(impl_->late_default_request_brands.insert(attempt->privatebrand_without_callbacks()).second,
         "Late Default archive request already issued for this actual attempt");
    impl_->archive_request_started=true; // Consume before callbacks/request allocation.
    attempt->require_active_request();require_fresh_direct_original_branch();
    attempt->require_same_input(*this);attempt->require_active_request();
    auto value=std::make_unique<BnwNativeColdArchiveRequest::Impl>();
    value->owner=impl_->self;value->epoch=impl_->epoch.get();value->late_default=attempt;value->is_late_default=true;
    value->ordinal=attempt->source_ordinal();value->raw_ansi=attempt->raw_relative_ansi_request();
    value->raw_utf16=attempt->raw_utf16_stream_name_candidate();value->relative=attempt->original_relative_io_token();
    value->original_candidate=confined_original_stat_path(impl_->original_source_base,value->relative);
    value->payload_candidate=confined_original_stat_path(impl_->canonical_payload_base,value->relative);
    need(value->raw_utf16==bnw_primary_archive_ascii_stream_token(value->raw_ansi)&&
         value->relative==primary_relative_io_path(value->raw_utf16)&&
         value->original_candidate==attempt->original_absolute_io_candidate()&&
         value->payload_candidate==attempt->canonical_payload_io_candidate(),
         "Late Default retained raw/stream/Original/canonical request routes differ");
    auto out=std::shared_ptr<const BnwNativeColdArchiveRequest>(new BnwNativeColdArchiveRequest(std::move(value)));
    require_current_request(*out);attempt->require_active_request();return out;
}

void BnwNativeColdArchiveRequest::require_same_installed_late_default_attempt(
    const BnwNativeInstalledLateDefaultArchiveAttempt& attempt)const {
    require_current();attempt.require_current();
    const auto actual=impl_->late_default.lock();
    need(impl_->is_late_default&&!impl_->is_common&&!impl_->is_category&&actual&&actual.get()==&attempt,
         "Late Default request belongs to another actual retained attempt");
}
