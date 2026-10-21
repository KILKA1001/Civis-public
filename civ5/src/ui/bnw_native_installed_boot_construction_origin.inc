// Actual installed Boot construction only. No ordinary-current fast dispatch.
BnwNativePackageDiscoveryOwner::InstalledBootConstructionOrigin::InstalledBootConstructionOrigin(
    const BnwNativePackageDiscoveryOwner& actual,const void* actual_registered_loan):
    state_(actual.impl_.get()),parent_(&actual),loan_(actual_registered_loan),
    identities_([&]{const auto& i=*actual.impl_;return std::array<const void*,26>{i.identity.get(),i.default_state.get(),&i.resources,i.current_default_source.get(),
        i.entry_arena.get(),i.archive_cache.get(),i.cold.get(),i.stat_service.get(),i.first_lineage.get(),
        i.primary32_journal.get(),i.record_snapshot.get(),i.record_snapshot_brand.get(),
        i.prepared_default_projection.get(),i.default_projection_brand.get(),i.pre_dx9_discovery.get(),
        i.filesystem_mappings.get(),i.installation_input.get(),
        i.prepared_default_projection?i.prepared_default_projection->source.get():nullptr,
        i.first_source_step_brand.get(),i.first_header.get(),i.first_prefix.get(),i.first_admission.get(),
        i.first_cache_publication.get(),i.first_request.get(),i.first_construction.get(),i.first_cache_lookup.get()};}()),
    factory_(actual.impl_->self),rows_(actual.impl_->default_state->selected.records.size()),
    packages_(actual.impl_->frame.packages),descriptors_(actual.impl_->retained_descriptors.size()){
    auto& i=*state_;need(!i.active_boot_construction,"Installed Boot construction origin nested");
    i.active_boot_construction=this;
    try{require_identity(actual);}catch(...){i.active_boot_construction=nullptr;i.header_poisoned=true;throw;}
}
BnwNativePackageDiscoveryOwner::InstalledBootConstructionOrigin::~InstalledBootConstructionOrigin()noexcept{
    // Existing Root storage/registered loan outlive this cleanup. No facade read.
    if(state_->active_boot_construction!=this)state_->header_poisoned=true;
    state_->active_boot_construction=nullptr;
}
void BnwNativePackageDiscoveryOwner::InstalledBootConstructionOrigin::require_identity(
    const BnwNativePackageDiscoveryOwner& actual)const{
    const auto& i=*state_;
    need(i.thread==std::this_thread::get_id(),"Installed Boot construction foreign creator");
    bool registered=false;
    for(auto* loan=i.active_installation_parent_loan;loan;loan=loan->previous)
        if(static_cast<const void*>(loan)==loan_)registered=loan->parent.get()==parent_&&loan->parent->impl_.get()==state_;
    need(&actual==parent_&&actual.impl_.get()==state_&&i.active_boot_construction==this&&registered&&
         !factory_.owner_before(i.self)&&!i.self.owner_before(factory_)&&!factory_.expired()&&
         i.installation_has_outside_parent()&&!failed_&&!i.header_poisoned&&!i.category_lineage_failed&&
         !i.primary32_failed&&!i.record_snapshot_failed&&!i.default_projection_failed&&!i.header_active&&!i.active_header_fence,
         "Installed Boot construction lost actual source/operation/outside Parent");
    need(i.installation_input&&!i.lease&&!i.rules&&i.installation_has_actual_published_default()&&
         i.current_default_source&&!i.current_default_source->failed_&&!i.primary32_pending&&!i.default_projection_pending&&
         i.installation_boot_attempted&&!i.installation_high_texture_attempted&&!i.installation_high_texture_returned&&
         !i.common_driver_attempted&&!i.common_pending&&i.common_steps.empty()&&!i.installed_default_common&&
         !i.live_resource_query&&!i.global_color_bank&&!i.production_default_active&&i.registry.revision==2&&
         i.primary32_source_completed==32&&i.primary32_steps.size()==32&&i.frame.primary_dx9_registration_complete&&
         !i.frame.low_resolution_source_branch_complete&&!i.frame.native_skin_loading_complete&&i.resources.mounts().empty()&&
         i.default_state->selected.records.size()==rows_&&i.frame.default_indexed_names==rows_&&
         i.frame.packages==packages_&&i.retained_descriptors.size()==descriptors_&&i.frame.resource_registration_order.size()==32&&
         ((!boot_identity_&&!i.settings&&!i.installation_boot_returned)||
          (boot_identity_&&i.settings.get()==boot_identity_&&i.installation_boot_returned&&
           !boot_.owner_before(i.settings)&&!i.settings.owner_before(boot_)))&&
         ((!directory_identity_&&i.installation_boot_directory.expired())||
          (directory_identity_&&!directory_.expired()&&!directory_.owner_before(i.installation_boot_directory)&&
           !i.installation_boot_directory.owner_before(directory_))),
         "Installed Boot construction changed its actual construction/source phase");
    need(identities_==std::array<const void*,26>{i.identity.get(),i.default_state.get(),&i.resources,i.current_default_source.get(),
        i.entry_arena.get(),i.archive_cache.get(),i.cold.get(),i.stat_service.get(),i.first_lineage.get(),
        i.primary32_journal.get(),i.record_snapshot.get(),i.record_snapshot_brand.get(),
        i.prepared_default_projection.get(),i.default_projection_brand.get(),i.pre_dx9_discovery.get(),
        i.filesystem_mappings.get(),i.installation_input.get(),
        i.prepared_default_projection?i.prepared_default_projection->source.get():nullptr,
        i.first_source_step_brand.get(),i.first_header.get(),i.first_prefix.get(),i.first_admission.get(),
        i.first_cache_publication.get(),i.first_request.get(),i.first_construction.get(),i.first_cache_lookup.get()},
         "Installed Boot construction changed SAME published Default operand");
}
void BnwNativePackageDiscoveryOwner::InstalledBootConstructionOrigin::require_current(
    const BnwNativePackageDiscoveryOwner& actual)const{
    try{
        require_identity(actual);state_->installation_input->require_current();require_identity(actual);
        state_->installation_input->require_current();require_identity(actual);
    }catch(...){failed_=true;state_->header_poisoned=true;throw;}
}
void BnwNativePackageDiscoveryOwner::InstalledBootConstructionOrigin::record_actual_directory(
    const std::shared_ptr<const BnwNativeUserDirectoryOwner>& actual)const{
    need(!directory_identity_&&actual&&!state_->installation_boot_directory.owner_before(actual)&&
         !actual.owner_before(state_->installation_boot_directory),"Installed Boot construction lost actual Directory store");
    directory_=actual;directory_identity_=actual.get();require_identity(*parent_);
}
void BnwNativePackageDiscoveryOwner::InstalledBootConstructionOrigin::record_actual_boot(
    const std::shared_ptr<const BnwNativeResourceBootSettingsOwner>& actual){
    need(!boot_identity_&&actual&&state_->settings.get()==actual.get()&&state_->installation_boot_returned,
         "Installed Boot construction lost genuine returned Boot adoption");
    boot_=actual;boot_identity_=actual.get();require_identity(*parent_);
}
void BnwNativePackageDiscoveryOwner::InstalledBootConstructionOrigin::refuse_ordinary_current()const{
    need(state_->thread==std::this_thread::get_id(),"Installed Boot construction reentry foreign creator");
    // In a real Input provider this MUST encounter the original busy gate,
    // preserving sticky retirement even when the callback catches its error.
    try{state_->installation_input->require_current();}
    catch(...){failed_=true;state_->header_poisoned=true;throw;}
    failed_=true;state_->header_poisoned=true;
    throw std::runtime_error("Installed Boot construction Parent.current reentry");
}
const assets::ResourceMountTable& BnwNativePackageDiscoveryOwner::installation_boot_source_table(
    const InstalledBootConstructionOrigin& origin)const{
    origin.require_current(*this);return impl_->resources;
}
std::weak_ptr<const BnwNativePackageDiscoveryOwner> BnwNativePackageDiscoveryOwner::installation_boot_source_factory(
    const InstalledBootConstructionOrigin& origin)const{
    (void)installation_boot_source_table(origin);return impl_->self;
}
std::filesystem::path BnwNativePackageDiscoveryOwner::installation_boot_source_game_root(
    const InstalledBootConstructionOrigin& origin)const{
    (void)installation_boot_source_table(origin);const auto root=impl_->installation_input->game_root();origin.require_identity(*this);
    (void)installation_boot_source_table(origin);auto value=std::filesystem::absolute(root).lexically_normal();origin.require_identity(*this);return value;
}
void BnwNativePackageDiscoveryOwner::register_installation_boot_directory(
    std::shared_ptr<const BnwNativeUserDirectoryOwner> actual,const InstalledBootConstructionOrigin& origin)const{
    (void)installation_boot_source_table(origin);auto& i=*impl_;
    need(i.installation_boot_directory.expired()&&actual,"Installed user directory factory already registered/absent");
    const auto canonical=actual->retain_factory_owner(actual);
    need(canonical.get()==actual.get()&&!canonical.owner_before(actual)&&!actual.owner_before(canonical),
         "Installed user directory factory control forged");
    i.installation_boot_directory=actual;
    origin.record_actual_directory(actual);
}
const assets::ResourceMountTable& BnwNativePackageDiscoveryOwner::installation_directory_source_table(
    const BnwNativeUserDirectoryOwner& actual,const InstalledBootConstructionOrigin& origin)const{
    origin.require_identity(*this);const auto storage=impl_;auto& i=*storage;
    const auto canonical=i.installation_boot_directory.lock();need(canonical&&canonical.get()==&actual,
         "Installed user directory supplied another/unregistered factory");
    origin.require_current(*this);
    need(i.installation_has_outside_parent(),"Installed user directory source retired outside Parent");return i.resources;
}
