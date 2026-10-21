// Existing Boot/Directory parent accounting remains the outer authority. This
// closed loan adopts the genuine pre-Default paired Roots, never caller text.
std::shared_ptr<BnwNativePackageDiscoveryOwner::InstalledFilesystemDocumentsLoan>
BnwNativePackageDiscoveryOwner::borrow_installed_filesystem_documents()const {
    const auto storage=impl_;auto& i=*storage;
    need(i.thread==std::this_thread::get_id(),"FS Documents factory foreign creator");
    (void)installation_boot_source_table();
    const auto owner=i.filesystem_mappings;
    need(owner&&i.installation_input&&i.cold&&owner->bound_&&owner->cold_bound_&&
         owner->parent_identity_==this&&detail::BnwNativeFilesystemMappingOwner::same(owner->parent_,i.self)&&
         detail::BnwNativeFilesystemMappingOwner::same(owner->input_,i.installation_input)&&
         detail::BnwNativeFilesystemMappingOwner::same(owner->cold_,i.cold),
         "FS Documents factory lacks SAME actual bootstrap/Input/Parent/Cold ancestry");
    owner->mapping_bootstrap_identity_without_callbacks();owner->source_current();(void)installation_boot_source_table();
    const auto result=std::shared_ptr<InstalledFilesystemDocumentsLoan>(new InstalledFilesystemDocumentsLoan(owner,*this));
    (void)installation_boot_source_table();owner->mapping_bootstrap_identity_without_callbacks();return result;
}

// Existing Boot/Directory parent accounting remains the outer authority. This
// closed loan adopts the genuine pre-Default paired Roots, never caller text.
std::shared_ptr<BnwNativePackageDiscoveryOwner::InstalledFilesystemDocumentsLoan>
BnwNativePackageDiscoveryOwner::borrow_installed_filesystem_documents(const InstalledBootConstructionOrigin& origin)const {
    const auto storage=impl_;auto& i=*storage;
    need(i.thread==std::this_thread::get_id(),"FS Documents factory foreign creator");
    (void)installation_boot_source_table(origin);
    const auto owner=i.filesystem_mappings;
    need(owner&&i.installation_input&&i.cold&&owner->bound_&&owner->cold_bound_&&
         owner->parent_identity_==this&&detail::BnwNativeFilesystemMappingOwner::same(owner->parent_,i.self)&&
         detail::BnwNativeFilesystemMappingOwner::same(owner->input_,i.installation_input)&&
         detail::BnwNativeFilesystemMappingOwner::same(owner->cold_,i.cold),
         "FS Documents factory lacks SAME actual bootstrap/Input/Parent/Cold ancestry");
    owner->mapping_bootstrap_identity_without_callbacks();owner->source_current();(void)installation_boot_source_table(origin);
    const auto result=std::shared_ptr<InstalledFilesystemDocumentsLoan>(new InstalledFilesystemDocumentsLoan(owner,*this));
    (void)installation_boot_source_table(origin);owner->mapping_bootstrap_identity_without_callbacks();return result;
}
