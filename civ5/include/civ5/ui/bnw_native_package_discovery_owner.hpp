#pragma once
namespace civ5::runtime {class BnwSelectedLanguageInputOwner;}

#include <array>
#include "civ5/ui/bnw_native_ui_color_owner.hpp"
#include "civ5/assets/resource_mounts.hpp"
#include "civ5/runtime/save_preflight.hpp"
#include <string_view>
#include <functional>
#include <utility>
#include <type_traits>

namespace civ5::assets {class CivisResourceBankOwner;}
namespace civ5::runtime {class BnwSelectedStockLuaSourceOwner;class BnwSelectedUserOptionsOwner;class BnwWorldViewSourceOperationLoan;}
namespace civ5::ui {
class BnwNativePackageCategorySourceOwner;
class BnwAudioCompletedResourceOwner;
class BnwNativeUserDirectoryOwner;
class BnwNativePackageArchiveTimeOwner;
class BnwNativePackagePreSkinOwner;
namespace detail {
class BnwNativeResourceDiscoveryActor;
class BnwNativeInstalledDescriptorEnumeration;
class BnwNativePackageCategory2PairOperation;
class BnwNativePackageCategory2ConstructorRead;
class BnwNativeCategoryCompletionParentPin;
class BnwNativeUiSkinRetainedOccurrence;
class BnwNativeUiSkinPoststageQueryLease;
class BnwNativeUiSkinPoststageInstance;
class BnwNativeOwnedSkinOccurrence;
class BnwNativeOwnedSkinSelectedInput;
class BnwNativeOwnedSkinBootPolicy;
class BnwNativeOwnedSkinConsumer;
class BnwNativeOwnedSkinGlobalSelectionKernel;
class BnwNativeCompletedResourceBankCapability;
class BnwNativeOwnedStartupCompletion;
class BnwNativeCompletedSkinInstanceObservation;
class BnwNativeCompletedColorQuery;
class BnwNativeCompletedSelectedRead;
class BnwNativeCompletedUnitArtInputs;
class BnwNativeCompletedParentObservation;
class BnwNativeCompletedColdSourceLoan;
class BnwNativeStandalonePackageLoaded;
class BnwNativeOwnedUiSkinPoststageInstance;
class BnwNativeGlobalColorRegistryBank;
class BnwNativeGlobalResourceWarmOwner;struct BnwNativeEarlyDefaultDiscoveryRow;class BnwNativeDescriptorSelectedStreamLoan;
struct BnwNativeArchiveRecordDiagnostic;
class BnwNativeCurrentDefaultSource;
class BnwNativeCurrentDefaultSelection;
struct NativeResourceRecordIndex;
class BnwNativeSharedFirstLineage;
class BnwNativeCategoryNamespaceLineage;
class BnwNativeArchiveEntryAdmissionArena;
class BnwNativeArchiveV6EntryAdmissionOwner;
class BnwNativeArchiveV6StreamHeaderOwner;
class BnwNativeGlobalArchivePrefixRecord;
class BnwNativeArchiveBasenameCacheOwner;
class BnwNativeArchiveCacheLookupOwner;
class BnwNativeArchiveCachePublicationOwner;
class BnwNativeColdArchiveRequest;
class BnwNativeColdFilesystemOwner;
class BnwNativePrimary32SourceBindings;
template<class> class BnwNativeCallbackOwnerGuard;
class BnwNativePrimary32LoopJournalOwner;
class BnwNativePrimary32PostpublishedJournalOperation;
struct BnwNativePrimary32JournalFrameDiagnostic;
class BnwNativePrimary32RecordSourceBindings;
class BnwNativePrimary32RecordSnapshotOwner;
}
class BnwNativeEarlyInputLease;
class BnwNativeInstallationInputOwner;
class BnwNativePinnedInstallationEpoch;
class BnwNativePinnedInputEpoch;
class BnwNativeResourceBootSettingsOwner;
struct BnwNativeResourceBootSettingsFrame;
class BnwNativePackagePreSkinOwner;
struct BnwNativeResourceCollisionReceipt {
    std::uint32_t hash = 0;
    std::string previous_name, incoming_name;
    unsigned mode = 0;
    bool replaced = false;
};
// FindA diagnostic metadata only. Native83A9FD record size and83A9F5/9 time
// come from _stat64i32; failed stat leaves those native fields unknown/stale.
// The selected unreadable payload remains an explicit read-boundary refusal.
struct BnwNativeUnreadableLooseReceipt {
    std::string relative_path, native_basename;
    std::uint64_t bytes = 0, last_write_time_raw = 0;
    std::uint32_t attributes = 0, open_error = 0;
};
// Read-only observations of authentic retained native text-getter inputs.
// These copies cannot issue a token, selected namespace or resource authority.
struct BnwNativeUiSkinTextDiagnostic {
    std::string descriptor;
    std::size_t occurrence = 0, slot = 0;
    std::string selector;
    std::vector<std::string> getter_values;
};
struct BnwNativePackageDiscoveryFrame {
    std::size_t packages = 0, skin_contexts = 0, slot_maps = 0;
    std::size_t descriptor_index_misses = 0, descriptor_direct_reads = 0;
    // Complete potential package maps are NOT an enabled/native stack receipt.
    bool potential_package_color_maps_complete = false;
    bool color_source_set_closed = false, native_skin_loading_complete = false;
    std::vector<std::string> original_descriptor_order;
    bool primary_dx9_registration_complete = false;
    bool low_resolution_source_branch_complete = false;
    bool high_texture_quality_source_branch = false;
    bool low_resource_mount_present = false;
    bool common_high_quality_registration_complete = false;
    std::size_t loose_registration_requests = 0, loose_files_registered = 0;
    std::size_t loose_file_registration_requests = 0;
    std::uint32_t native_loose_code_page = 0;
    std::size_t low_indexed_names = 0;
    std::size_t archive_registration_requests = 0, archives_registered = 0;
    std::size_t default_indexed_names = 0;
    std::string next_resource_dependency;
    std::vector<std::string> resource_registration_order;
    // Source FindA records whose reconstructed CP_ACP/UTF16 OS read fails.
    // These remain indexed, are never mapped to a Unicode canonical alias.
    std::vector<BnwNativeUnreadableLooseReceipt> indexed_unreadable_loose;
    std::vector<BnwNativeResourceCollisionReceipt> resource_crc_collisions;
};
// A copied diagnostic projection of the first genuine source constructor.
// It cannot authorize entry admission, archive cache insertion or UI startup.
struct BnwNativeFirstArchiveMetadataDiagnostic {
    std::u16string raw_prefix;
    std::uint32_t prefix_index = 0, declared_entry_count = 0;
    std::uint64_t original_byte_length = 0;
    std::int64_t checked_original_mtime = 0;
    std::size_t canonical_metadata_entries = 0;
    // Retained source82A050 INITIAL state, not current enabled/stack permission.
    std::uint8_t constructor_archive_priority = 0, constructor_enabled = 0;
    std::size_t constructor_prefix_filter_count = 0;
};
struct BnwNativeFirstArchiveAdmissionDiagnostic {
    std::uint32_t declared_entry_count = 0, admitted_entry_count = 0;
};
struct BnwNativeFirstArchiveCacheDiagnostic {
    std::int64_t checked_original_mtime = 0;
    std::uint64_t canonical_metadata_entries = 0;
};
struct BnwNativePrimary32ArchiveDiagnostic {
    std::size_t completed_archives = 0;
    std::uint64_t executed_admitted_total = 0;
    bool positive_primary32_complete = false;
};
struct BnwNativePrimary32RecordQueryDiagnostic {
    bool matched=false;
    std::uint32_t requested_crc=0,selected_crc=0,payload_length=0,payload_offset=0,packed_prefix=0,source_slot=0;
    std::string first_raw_name;
    std::size_t archive_ordinal=0;
};
// Owns a fresh EARLY resource epoch: native 821690 reset/default construction
// precedes original Assets package discovery and all FPK registration.
// No post-Rules plan can be moved into this factory. The resource object must
// later be evolved by separate source-owned registration stages, not replaced
// by another UI resolver. This stage alone keeps all readiness gates closed.
namespace detail {class BnwNativeCommon13ArchiveChannel;class BnwNativeDefaultCommonProduction;class BnwNativePreDx9DiscoveryOwner;class BnwNativeInstalledDefaultCommonSource;
class BnwNativeInstalledLateSpeechCoreDonor;
class BnwNativeInstalledLateDefaultOperation;
class BnwNativeInstalledLateDefaultArchiveAttempt;}
class BnwNativePackageDiscoveryOwner {
  public:
    // Production EARLY epoch: no SQL host or Color owner is a predecessor.
    static std::shared_ptr<const BnwNativePackageDiscoveryOwner> create(
        std::shared_ptr<const BnwNativeEarlyInputLease>);
    // Real zero-save prefix ONLY: SAME installation/source owns initial empty
    // Default/resources and actual metadata reads. Missing installation-aware
    // Category2/FIRST/Cold/Stat successors remain mandatory before Boot/Skin.
    static std::shared_ptr<const BnwNativePackageDiscoveryOwner> create(
        std::shared_ptr<const BnwNativeInstallationInputOwner>);
    // Separate initial Default domain BEFORE Primary32. The closed issuer owns
    // actual filename/receiver/read/Parser/Core/metadata occurrences and later
    // moves those SAME objects into genuine Common Warm. Initial prefix and
    // profile admission still require their separate source-owned operations.
    static std::shared_ptr<const BnwNativePackageDiscoveryOwner> create_pre_dx9_installation(
        std::shared_ptr<const BnwNativeInstallationInputOwner>);
    void advance_pre_dx9_discovery_before_primary32()const;
    // Actual installation constructor/append sequence and SAME empty FIRST;
    // next retained UI source remains mandatory, no ready-state receipt.
    void advance_installation_category2_first_prefix()const;
    void advance_installation_default_loose_source()const;
    void publish_installation_default_common_source()const;
    // Original initial Tablet/64C610/686D80 on SAME adopted pre-DX9 Core.
    // Does not apply a saved profile or construct category/Skin managers.
    void advance_installation_startup_admission()const;
    // Executes original initial cursor through the first genuine category
    // constructor. Actual Paks/final effects remain an explicit successor.
    void advance_installation_initial_category_prefix()const;
    void drive_installation_initial_categories()const;
    void drive_installation_initial_color_skin()const;
    void advance_installation_initial_skin_selection_prefix()const;
    void drive_installation_first_common_skin()const;
    void advance_installation_pc_publication()const;
    void drive_installation_pc_skin()const;
    void close_installation_initial_constraints()const;
    void drive_installation_global_skin()const;
    void drive_installation_late_speech_and_mappings()const;
    void advance_installation_retained_ui_prefix()const;
    void advance_installation_cold_stat_prefix()const;
    void advance_installation_prefix_arena_cache()const;
    void advance_installation_current_first_archive_header()const;
    void advance_installation_first_archive_publication()const;
    void advance_installation_primary32_archives()const;
    std::size_t advance_installation_record_snapshot_default_projection()const;
    void advance_installation_default_publication()const;
    BnwNativeResourceBootSettingsFrame advance_installation_boot_settings()const;
    void advance_installation_high_texture_quality_branch()const;
    std::optional<std::uint32_t> advance_installation_first_common_archive_header()const;
    void advance_installation_first_common_archive_metadata()const;
    std::uint32_t advance_installation_first_common_archive_admission()const;
    std::uint64_t advance_installation_remaining_common_archives()const;
    BnwNativePrimary32ArchiveDiagnostic inspect_installation_primary32_archives()const;
    BnwNativeFirstArchiveAdmissionDiagnostic inspect_installation_first_archive_admission()const;
    //Canonical factory identity/retention only; no Early/provider callback or
    //resource readiness. Compare caller pointer/control BEFORE currentness.
    std::shared_ptr<const BnwNativePackageDiscoveryOwner> retain_factory_owner() const;
    std::shared_ptr<const BnwNativeEarlyInputLease> early_input_lease() const;
    // Legacy post-Rules diagnostic compatibility; cannot become an adopted
    // production epoch merely by matching header/database values.
    static std::shared_ptr<const BnwNativePackageDiscoveryOwner> create(
        const std::filesystem::path& game_root,
        const std::filesystem::path& shared_root, const content::Catalog&,
        const runtime::SavedContentProfile&,
        std::shared_ptr<const BnwNativeUiColorOwner> same_rules_client);
    ~BnwNativePackageDiscoveryOwner();
    const BnwNativePackageDiscoveryFrame& current() const;
    BnwNativeColorRegistryProvider color_provider() const;
    const assets::ResourceMountTable& resources() const;
    // Authenticates the retained noncopyable host/private unique session/SQL.
    // Does not equate this EARLY table with that host's later Rules resolver.
    void require_same_selected_host(const SelectedBnwPreviewHost&) const;
    // Executes the pinned initial 54CB60..54CBDC 32-DX9 archive prefix.
    // Transactional, once per owned epoch. No supplied mounts or alias adapter.
    void register_native_dx9_primary_prefix() const;
    // Fresh production EARLY epoch only; exact ordinal0 header -> shared prefix
    // -> entry metadata. Once, terminal on failure. Does not insert records.
    BnwNativeFirstArchiveMetadataDiagnostic prepare_native_dx9_first_archive_metadata() const;
    // Execute the finite source mode0 loop against the retained same namespace.
    // Copied counts do not authorize archive-cache publication or primary32.
    BnwNativeFirstArchiveAdmissionDiagnostic admit_native_dx9_first_archive_entries() const;
    // Genuine positive EntryReceipt -> SAME pre-constructor cache miss -> store.
    // Copied facts do not complete primary32 or publish the ResourceMountTable.
    BnwNativeFirstArchiveCacheDiagnostic publish_native_dx9_first_archive_cache() const;
    // Resume the genuine positive source route in SAME namespace/cache after
    // completed ordinal0. This diagnostic does not publish VFS or UI readiness.
    BnwNativePrimary32ArchiveDiagnostic complete_native_dx9_primary32_archives() const;
    BnwNativePrimary32ArchiveDiagnostic inspect_native_dx9_primary32_archives() const;
    // Execute private Category0/1 source operations. Unsupported native routes
    // refuse; completion is sealed only after actual loads and owned mounts.
    void prepare_native_package_category_sources(
        std::shared_ptr<const BnwNativePackagePreSkinOwner>,
        std::shared_ptr<const BnwNativePackageArchiveTimeOwner>) const;
    void require_native_package_category_sources_complete() const;
    // Genuine SAME completed source records, once. Copied diagnostics ONLY:
    // no native VFS miss/read/activation, stack, Config or mounted resources.
    std::size_t capture_native_dx9_primary32_record_snapshot() const;
    BnwNativePrimary32RecordQueryDiagnostic query_native_dx9_primary32_record(std::u16string_view) const;
    // Verify authentic retained UISkin inputs only. No native construction,
    // selection, availability/enabled bit, mount or readiness is produced here.
    void require_native_ui_skin_retained_inputs() const;
    std::vector<BnwNativeUiSkinTextDiagnostic> inspect_native_ui_skin_retained_text_inputs() const;
    // Mandatory source83B2A0 remains a separate transition; metadata is no proof.
    void require_native_dx9_first_archive_entry_admission() const;
    // Source64A540 fresh nonzero-D80DE0 or zero separate mounted22-FPK branch.
    void register_native_dx9_texture_quality_branch(
        std::shared_ptr<const BnwNativeResourceBootSettingsOwner>) const;
    // Specifically the nonzero source branch; cannot be used to skip low textures.
    void register_native_dx9_high_quality_branch(
        std::shared_ptr<const BnwNativeResourceBootSettingsOwner>) const;
    // Source54CBE9 Common13/default loose stage, scoped to actual high setting.
    // Low13C4 needs additional bit0x10 scan; mandatory Color/Skin guards remain.
    void register_native_common_high_quality_prefix() const;
  private:
    friend class ::civ5::assets::CivisResourceBankOwner;
    static std::shared_ptr<const detail::BnwNativeOwnedStartupCompletion> capture_bank_context_lua_include_source_owned(
        const std::shared_ptr<const BnwNativeEarlyInputLease>&,const std::function<void()>&);
    void activate_bank_context_lua_include_mods_owned(const std::function<void()>&)const;
    std::vector<std::string> collect_bank_context_lua_include_names_owned(
        const std::shared_ptr<const detail::BnwNativeCompletedResourceBankCapability>&,const std::function<void()>&)const;
    friend class ::civ5::runtime::BnwSelectedStockLuaSourceOwner;
    friend class ::civ5::runtime::BnwWorldViewSourceOperationLoan;
    std::shared_ptr<const runtime::BnwSelectedUserOptionsOwner> borrow_selected_user_options_for_source()const;
    friend class detail::BnwNativeGlobalColorRegistryBank;
    std::array<std::vector<std::string>,3> read_native_skin_resource_declarations(const detail::BnwNativeGlobalColorRegistryBank&,std::size_t,unsigned)const;
    std::vector<std::string> read_native_skin_resource_query(const detail::BnwNativeGlobalColorRegistryBank&,std::size_t,unsigned,unsigned)const;
    void require_native_skin_direct_stream_source(const detail::BnwNativeGlobalColorRegistryBank&,std::size_t,unsigned)const;
    void remember_native_skin_resource_name(const detail::BnwNativeGlobalColorRegistryBank&,std::uint32_t,std::string_view)const;
    std::uint32_t native_skin_texture_quality(const detail::BnwNativeGlobalColorRegistryBank&)const;
    std::uint32_t register_native_skin_resource_prefix(const detail::BnwNativeGlobalColorRegistryBank&,std::size_t,unsigned,std::string_view)const;
    friend class detail::BnwNativeGlobalResourceWarmOwner;
    friend class detail::BnwNativePreDx9DiscoveryOwner;
    friend class detail::BnwNativeInstalledDefaultCommonSource;
    void require_installed_default_common_identity_without_callbacks(
        const detail::BnwNativeInstalledDefaultCommonSource&)const;
    void require_installed_default_common_active_current(
        const detail::BnwNativeInstalledDefaultCommonSource&)const;
    void validate_installed_default_common_current(
        const detail::BnwNativeInstalledDefaultCommonSource&)const;
    void refuse_installed_default_rewrite()const;
    friend class detail::BnwNativeInstalledLateDefaultOperation;
    friend class detail::BnwNativeInstalledLateDefaultArchiveAttempt;
    friend unsigned bnw_native_installed_late_default_phase_for_test(const BnwNativePackageDiscoveryOwner&)noexcept;
    void require_installed_mapping_bootstrap_before_late_tail()const;
    void append_installed_late_mappings()const;
    void require_installed_late_mappings_returned_source()const;
    void begin_installed_late_default_speech(const detail::BnwNativeGlobalResourceWarmOwner&,
        const detail::BnwNativeInstalledLateSpeechCoreDonor&)const;
    void register_installed_late_default_core_directory(const detail::BnwNativeInstalledLateSpeechCoreDonor&)const;
    void finish_installed_late_default_speech(const detail::BnwNativeInstalledLateSpeechCoreDonor&)const;
    void require_installed_late_default_identity_without_callbacks()const;
    void require_installed_late_default_observation_without_callbacks()const;
    void require_installed_default_common_origin_without_callbacks(
        const detail::BnwNativeInstalledDefaultCommonSource&)const;
    void validate_installed_late_default_namespace_without_callbacks()const;
    void require_installed_late_default_selected_crc_without_callbacks(std::uint32_t)const;
    void fail_installed_late_default_without_callbacks()const noexcept;
    void require_installed_late_default_writer_idle_without_callbacks()const;
    unsigned installed_late_default_normal_loose_mode(const detail::BnwNativeInstalledLateDefaultOperation&)const;
    void require_installed_late_default_attempt_current(
        const detail::BnwNativeInstalledLateDefaultArchiveAttempt&)const;
    void require_installed_late_default_attempt_identity_without_callbacks(
        const detail::BnwNativeInstalledLateDefaultArchiveAttempt&,bool actual_request_entry)const;
    bool installed_late_default_selected_is_archive_without_callbacks(
        const detail::BnwNativeInstalledLateDefaultOperation&,std::uint32_t)const;
    void begin_installed_late_default_selected_store_without_callbacks(
        const detail::BnwNativeInstalledLateDefaultOperation&,std::uint32_t)const;
    void record_installed_late_default_selected_store(
        const detail::BnwNativeInstalledLateDefaultOperation&,std::uint32_t)const;
    void commit_installed_late_default_archive_projection(
        const detail::BnwNativeInstalledLateDefaultOperation&,
        const detail::BnwNativeInstalledLateDefaultArchiveAttempt&,
        const detail::BnwNativeArchiveV6StreamHeaderOwner&,const detail::BnwNativeGlobalArchivePrefixRecord&,
        const std::shared_ptr<const assets::FpkSource>&,const assets::FpkEntry&,const assets::AssetHandle&,
        const detail::BnwNativeArchiveRecordDiagnostic&)const;
    void execute_installed_late_default_loose(const detail::BnwNativeInstalledLateDefaultOperation&)const;
    void execute_installed_late_default_archive(const detail::BnwNativeInstalledLateDefaultOperation&)const;

    friend class detail::BnwNativeResourceDiscoveryActor;
    void require_pre_dx9_discovery_current(const detail::BnwNativePreDx9DiscoveryOwner&)const;
    void join_pre_dx9_initial_prefix(detail::BnwNativePreDx9DiscoveryOwner&)const;
    std::shared_ptr<const runtime::BnwSelectedLanguageInputOwner> borrow_initial_package_language_for_boot(
        const BnwNativeResourceBootSettingsOwner&)const;
    friend class detail::BnwNativeInstalledDescriptorEnumeration;
    void require_live_resource_query_current(const detail::BnwNativeGlobalResourceWarmOwner&) const;
    std::vector<detail::BnwNativeEarlyDefaultDiscoveryRow> borrow_live_default_discovery_rows(const detail::BnwNativeGlobalResourceWarmOwner&)const;
    void require_live_default_discovery_row_current(const detail::BnwNativeGlobalResourceWarmOwner&,
        const detail::BnwNativeEarlyDefaultDiscoveryRow&)const;
    void require_live_default_discovery_row_current_in_enumeration(
        const detail::BnwNativeGlobalResourceWarmOwner&,const detail::BnwNativeEarlyDefaultDiscoveryRow&,
        const detail::BnwNativeInstalledDescriptorEnumeration&)const;
    void prepare_native_early_package_receivers()const;
    void prepare_native_owned_configured_activation_policy()const;
    void drive_native_owned_common_gameplay_categories()const;
    void with_native_owned_skin_operation(const std::function<void()>&)const;
    class OwnedSkinConstructorSource;
    class OwnedColorBankSource;
    class InstalledColorSkinBankSource;
    std::shared_ptr<detail::BnwNativeGlobalColorRegistryBank> construct_installed_color_skin_bank_receiver()const;
    bool prepare_installed_color_skin_bank()const;
    void require_installed_color_skin_operation_current()const;
    void require_installed_color_skin_bank_current(const detail::BnwNativeGlobalColorRegistryBank&,
        const content::XmlNode*,std::uint64_t)const;
    void with_installed_color_skin_base_input(const std::function<void(
        const std::shared_ptr<const detail::BnwNativeOwnedSkinSelectedInput>&)>&)const;
    void construct_installed_color_skin_instance(
        const std::shared_ptr<const detail::BnwNativeOwnedSkinOccurrence>&)const;
    void prepare_native_owned_color_bank()const;
    void bind_native_owned_color_bank_constructed_roster()const;
    void require_native_owned_skin_operation_current(const BnwNativeEarlyInputLease&,const assets::ResourceMountTable&)const;
    void require_native_owned_skin_operation_current()const;
    void require_installed_initial_skin_effect_bank_current(const detail::BnwNativeGlobalColorRegistryBank&,
        const content::XmlNode*,std::uint64_t)const;
    const std::filesystem::path& installed_initial_skin_effects_root()const;
    void with_installed_first_common_skin_effect_bank(const std::function<void(
        const std::shared_ptr<detail::BnwNativeGlobalColorRegistryBank>&,
        const std::shared_ptr<detail::BnwNativeOwnedSkinBootPolicy>&)>&)const;
    class OwnedSkinEffectsSource; // Same private issuer type; no new public authority.
    class OwnedSkinGlobalEffectsLoan;
    class OwnedInstalledSkinGlobalSelectionSource;
    class OwnedInstalledSkinGlobalSelectionRecord;
    class OwnedInstalledSkinGlobalSelectionOccurrence;
    void record_installed_global_skin_set_read(
        const std::shared_ptr<const detail::BnwNativeOwnedUiSkinPoststageInstance>&,
        const std::optional<std::string>&)const;
    void disable_installed_global_skin_pair_then_instance(
        const std::shared_ptr<const detail::BnwNativeOwnedUiSkinPoststageInstance>&,unsigned)const;
    enum class InstalledGlobalBorrowCall {factory,borrow};
    std::uint64_t require_installed_global_effects_join(const detail::BnwNativeOwnedSkinConsumer&,
        const detail::BnwNativeOwnedSkinBootPolicy&,const void*,InstalledGlobalBorrowCall,std::uint64_t)const;
    std::shared_ptr<OwnedSkinGlobalEffectsLoan> make_installed_global_effects_loan(
        const std::weak_ptr<OwnedSkinEffectsSource>&,
        const std::shared_ptr<detail::BnwNativeOwnedSkinConsumer>&,
        const std::shared_ptr<detail::BnwNativeOwnedSkinBootPolicy>&)const;
    void with_installed_global_skin_inputs(const std::function<void(
        const std::weak_ptr<OwnedSkinEffectsSource>&,
        const std::shared_ptr<detail::BnwNativeOwnedSkinConsumer>&,
        const std::shared_ptr<detail::BnwNativeOwnedSkinBootPolicy>&)>&)const;
    void record_installed_global_source(const std::shared_ptr<const void>&,const std::shared_ptr<OwnedSkinGlobalEffectsLoan>&)const;
    void record_installed_global_kernel(const std::shared_ptr<detail::BnwNativeOwnedSkinGlobalSelectionKernel>&)const;
    void require_installed_global_source_current(const void*)const;
    void begin_installed_global_kernel(const std::shared_ptr<detail::BnwNativeOwnedSkinGlobalSelectionKernel>&)const;
    void record_installed_global_kernel_return(const std::shared_ptr<detail::BnwNativeOwnedSkinGlobalSelectionKernel>&,bool)const;
    void run_installed_global_skin_kernel()const;
    void with_installed_global_skin_slot(unsigned,const std::function<void(std::vector<std::size_t>&)>&)const;
    void with_installed_global_match_pass(const std::string&,const std::string&,bool,std::vector<std::size_t>&,
        const std::function<void()>&)const;
    void begin_installed_global_match_index(std::size_t)const;
    void record_installed_global_match_index(std::size_t,bool)const;
    void with_installed_global_kernel_roster(const std::function<void(
        const std::shared_ptr<const detail::BnwNativeOwnedUiSkinPoststageInstance>&,
        const std::shared_ptr<const detail::BnwNativeOwnedSkinOccurrence>&)>&)const;
    void begin_installed_global_base_compare(const std::string&)const;
    void record_installed_global_base_compare(const std::string&,bool)const;
    void prepare_installed_global_660(const std::string&,const std::string&,int)const;
    void record_installed_global_660_return(const std::shared_ptr<detail::BnwNativeOwnedSkinConsumer>&,int,bool)const;
    void reconcile_installed_global_bank()const;
    void run_installed_first_common_skin_kernel()const;
    void with_installed_pc_skin_effect_bank(const std::function<void(
        const std::shared_ptr<detail::BnwNativeGlobalColorRegistryBank>&,
        const std::shared_ptr<detail::BnwNativeOwnedSkinBootPolicy>&,
        const std::shared_ptr<detail::BnwNativeOwnedSkinConsumer>&)>&)const;
    void run_installed_pc_skin_kernel()const;
    void begin_installed_pc_skin_invocation(const std::shared_ptr<detail::BnwNativeOwnedSkinConsumer>&)const;
    void record_installed_pc_skin_consumer(const std::shared_ptr<detail::BnwNativeOwnedSkinConsumer>&,bool)const;
    void record_installed_first_common_skin_consumer(const std::shared_ptr<detail::BnwNativeOwnedSkinConsumer>&,
        const std::shared_ptr<OwnedSkinEffectsSource>&,bool)const;
    std::shared_ptr<detail::BnwNativeGlobalColorRegistryBank> construct_native_owned_color_bank_receiver()const;
    void begin_native_owned_color_bank_source_attachment(const detail::BnwNativeGlobalColorRegistryBank&,const void*)const;
    void complete_native_owned_color_bank_source_attachment(const detail::BnwNativeGlobalColorRegistryBank&)const;
    void require_native_owned_color_bank_current(const detail::BnwNativeGlobalColorRegistryBank&,const content::XmlNode*,std::uint64_t)const;
    void with_native_owned_base_color_document(const detail::BnwNativeGlobalColorRegistryBank&,
        const std::function<void(const std::shared_ptr<const content::XmlNode>&)>&)const;
    void require_native_owned_initial_skin_effects_terminal(const detail::BnwNativeGlobalColorRegistryBank&)const;
    void invalidate_native_owned_color_operation()const noexcept;
    std::shared_ptr<detail::BnwNativeOwnedSkinBootPolicy> capture_native_owned_skin_boot_policy()const;
    std::shared_ptr<const detail::BnwNativeCompletedResourceBankCapability>
        drive_native_owned_skin_base_game_boot(const std::shared_ptr<detail::BnwNativeOwnedSkinBootPolicy>&)const;
    void begin_native_owned_skin_effect_source(const detail::BnwNativeOwnedSkinBootPolicy&,const void*)const;
    void require_native_owned_skin_effect_source_current(const detail::BnwNativeOwnedSkinBootPolicy&,const void*)const;
    void complete_native_owned_skin_base_game_effects(const std::shared_ptr<detail::BnwNativeOwnedSkinBootPolicy>&,
        const std::shared_ptr<const detail::BnwNativeOwnedSkinConsumer>&)const;
    void with_native_owned_returned_package_roster(
        const std::function<void(const std::vector<std::shared_ptr<const detail::BnwNativeStandalonePackageLoaded>>&)>&)const;
    class OwnedSkinGlobalSelectionSource;
    class OwnedSkinGlobalSelectionOccurrence;
    class OwnedSkinGlobalSelectionRecord;
    void begin_native_owned_skin_global_source(const detail::BnwNativeOwnedSkinBootPolicy&,
        const std::shared_ptr<const detail::BnwNativeOwnedSkinConsumer>&,const void*)const;
    void require_native_owned_skin_global_source_current(const detail::BnwNativeOwnedSkinBootPolicy&,
        const std::shared_ptr<const detail::BnwNativeOwnedSkinConsumer>&,const void*)const;
    void complete_native_owned_skin_global_effects(const std::shared_ptr<detail::BnwNativeOwnedSkinBootPolicy>&,
        const std::shared_ptr<const detail::BnwNativeOwnedSkinConsumer>&,
        const std::shared_ptr<const detail::BnwNativeOwnedSkinGlobalSelectionKernel>&)const;
    class CompletedNativeResourceSource;
    class CompletedSkinInstanceSource;
    class CompletedColorQuerySource;
    class CompletedSelectedReadSource;
    class CompletedUnitArtInputsSource;
    class CompletedParentObservationSource;
    class CompletedColdSource;
    class CompletedSourceOpeningLoan;
    void require_native_owned_completed_warm_source_current(
        const detail::BnwNativeGlobalResourceWarmOwner&,const detail::BnwNativeCompletedColdSourceLoan&,
        const detail::BnwNativeColdFilesystemOwner&)const;
    const BnwNativePinnedInputEpoch& lend_native_owned_completed_loose_source_epoch(
        const detail::BnwNativeGlobalResourceWarmOwner&,const detail::BnwNativeCompletedColdSourceLoan&,
        const detail::BnwNativeColdFilesystemOwner&)const;
    void require_native_owned_completed_common_sources_current(
        const detail::BnwNativeCompletedColdSourceLoan&,const detail::BnwNativeColdFilesystemOwner&)const;
    void require_native_owned_completed_source_opening_current(
        const detail::BnwNativeCompletedResourceBankCapability&,const CompletedParentObservationSource&)const;
    void with_native_owned_completed_source_opening(
        const std::shared_ptr<const detail::BnwNativeCompletedResourceBankCapability>&,
        const std::function<void(const detail::BnwNativeCompletedColdSourceLoan&)>&)const;
    void record_native_owned_skin_normal_return(
        const std::shared_ptr<detail::BnwNativeGlobalResourceWarmOwner>&,std::uint64_t actual_epoch_before)const;
    std::shared_ptr<const detail::BnwNativeCompletedParentObservation>
        capture_native_owned_completed_parent_observation(
            const std::shared_ptr<const detail::BnwNativeCompletedResourceBankCapability>&)const;
    std::shared_ptr<const detail::BnwNativeCompletedParentObservation>
        capture_native_owned_completed_parent_observation(
            const std::shared_ptr<const detail::BnwNativeCompletedResourceBankCapability>&,
            std::shared_ptr<const CompletedParentObservationSource>& actual_typed_source)const;
    std::shared_ptr<const detail::BnwNativeCompletedSelectedRead> capture_native_owned_completed_selected_read(
        const std::shared_ptr<const detail::BnwNativeCompletedResourceBankCapability>&)const;
    std::shared_ptr<const detail::BnwNativeCompletedUnitArtInputs> capture_native_owned_completed_unit_art_inputs(
        const std::shared_ptr<const detail::BnwNativeCompletedResourceBankCapability>&,
        const std::shared_ptr<const detail::BnwNativeCompletedSelectedRead>&)const;
    std::shared_ptr<const detail::BnwNativeCompletedColorQuery> capture_native_owned_completed_color_query(
        const std::shared_ptr<const detail::BnwNativeCompletedResourceBankCapability>&)const;
    void with_native_owned_completed_resource_record(
        const std::shared_ptr<const detail::BnwNativeCompletedResourceBankCapability>&,
        const char*,
        const std::function<void(const detail::BnwNativeArchiveRecordDiagnostic*,const std::u16string*,const std::string*)>&)const;
    void require_native_owned_completed_skin_instance_current(
        const detail::BnwNativeCompletedResourceBankCapability&,
        const detail::BnwNativeOwnedUiSkinPoststageInstance&)const;
    std::shared_ptr<const detail::BnwNativeCompletedSkinInstanceObservation>
        capture_native_owned_completed_skin_instance_observation(
            const std::shared_ptr<const detail::BnwNativeCompletedResourceBankCapability>&,
            const std::shared_ptr<const detail::BnwNativeOwnedUiSkinPoststageInstance>&)const;
    void with_native_owned_completed_skin_instance_observations(
        const std::function<void(const std::vector<std::shared_ptr<const detail::BnwNativeCompletedSkinInstanceObservation>>&)>&)const;
    void with_native_owned_completed_resource_operation(
        const std::shared_ptr<const detail::BnwNativeCompletedResourceBankCapability>&,
        const std::function<void()>&)const;
    void require_native_owned_completed_resource_operation_current(
        const detail::BnwNativeCompletedResourceBankCapability&)const;
    void begin_native_owned_completed_color_bank_operation(
        const detail::BnwNativeGlobalColorRegistryBank&,const detail::BnwNativeCompletedResourceBankCapability&,const void*)const;
    void end_native_owned_completed_color_bank_operation(
        const detail::BnwNativeGlobalColorRegistryBank&,const detail::BnwNativeCompletedResourceBankCapability&,const void*)const noexcept;
    void require_native_owned_completed_color_bank_current(
        const detail::BnwNativeGlobalColorRegistryBank&,const detail::BnwNativeCompletedResourceBankCapability&,
        const void*,const content::XmlNode*,std::uint64_t)const;
    std::shared_ptr<const detail::BnwNativeCompletedResourceBankCapability>
        capture_native_owned_completed_resource_bank_capability()const;
    void with_native_owned_completed_resource_owner(
        const std::function<void(const std::shared_ptr<const detail::BnwNativeCompletedResourceBankCapability>&)>&)const;
    void with_native_owned_completed_input_epoch(
        const std::function<void(const std::shared_ptr<const BnwNativePinnedInputEpoch>&)>&)const;
    void complete_native_owned_color_bank_initialization()const;
    void disable_native_owned_skin_pair_then_instance(
        const std::shared_ptr<const detail::BnwNativeOwnedUiSkinPoststageInstance>&,unsigned)const;
    class OwnedSkinEffectsOccurrence;
    bool native_owned_skin_first_pair_present(const std::shared_ptr<const detail::BnwNativeOwnedUiSkinPoststageInstance>&,unsigned)const;
    unsigned char native_owned_skin_first_pair_enabled(const std::shared_ptr<const detail::BnwNativeOwnedUiSkinPoststageInstance>&,unsigned)const;
    void load_native_owned_skin_paks(const std::shared_ptr<const detail::BnwNativeOwnedUiSkinPoststageInstance>&,unsigned)const;
    void register_native_owned_skin_directory(const std::shared_ptr<const detail::BnwNativeOwnedUiSkinPoststageInstance>&,unsigned,std::size_t)const;
    void register_native_owned_skin_file(const std::shared_ptr<const detail::BnwNativeOwnedUiSkinPoststageInstance>&,unsigned,std::size_t)const;
    void publish_native_owned_skin_name(const std::shared_ptr<const detail::BnwNativeOwnedUiSkinPoststageInstance>&,unsigned)const;
    void enable_native_owned_skin_pair(const std::shared_ptr<const detail::BnwNativeOwnedUiSkinPoststageInstance>&,unsigned)const;
    void write_native_owned_skin_instance_enabled(const std::shared_ptr<const detail::BnwNativeOwnedUiSkinPoststageInstance>&,unsigned)const;
    void with_native_owned_skin_effect_bank(const std::function<void(const std::shared_ptr<detail::BnwNativeGlobalColorRegistryBank>&)>&)const;
    std::vector<std::string> query_native_owned_skin_outputs(
        const std::shared_ptr<const detail::BnwNativeOwnedSkinOccurrence>&,unsigned,const char*,std::uint32_t)const;
    void begin_native_owned_skin_selected_read(const std::shared_ptr<const detail::BnwNativeOwnedSkinOccurrence>&,unsigned,const char*)const;
    void end_native_owned_skin_selected_read(const std::shared_ptr<const detail::BnwNativeOwnedSkinOccurrence>&,unsigned,const char*)const;
    std::shared_ptr<const detail::BnwNativeOwnedSkinSelectedInput> read_native_owned_skin_selected_input(
        const std::shared_ptr<const detail::BnwNativeOwnedSkinOccurrence>&,unsigned,const char*,std::size_t)const;
    void require_native_owned_skin_read_receipt_current(const void*)const;
    void prepare_native_owned_skin_extra_attribute_query(const std::shared_ptr<const detail::BnwNativeOwnedSkinOccurrence>&)const;
    void bind_native_owned_skin_extra_attributes(const std::shared_ptr<const detail::BnwNativeOwnedSkinOccurrence>&,
        const std::shared_ptr<const detail::BnwNativeOwnedUiSkinPoststageInstance>&)const;
    void with_native_owned_skin_extra_attributes(const std::shared_ptr<const detail::BnwNativeOwnedUiSkinPoststageInstance>&,
        const std::function<void(const std::vector<std::pair<std::string,std::string>>&)>&)const;
    void construct_native_owned_skin_instances()const;
    void retain_native_owned_skin_instance(const std::shared_ptr<const detail::BnwNativeOwnedSkinOccurrence>&,
        const std::shared_ptr<const detail::BnwNativeOwnedUiSkinPoststageInstance>&)const;
    void complete_native_owned_skin_instance_constructors()const;
    void with_native_owned_skin_constructed_roster(
        const std::function<void(const std::vector<std::shared_ptr<const detail::BnwNativeOwnedUiSkinPoststageInstance>>&)>&)const;
    void prepare_native_owned_skin_occurrences()const;
    void with_native_owned_skin_occurrences(
        const std::function<void(const std::shared_ptr<const detail::BnwNativeOwnedSkinOccurrence>&)>&)const;
    void prepare_native_owned_first_common_category_manager()const;
    void resume_native_owned_first_common_paks_effects()const;
    void resume_native_owned_first_common_category_effects()const;
    void with_native_first_common_paks_checkpoint(
        const std::function<void(int,const std::vector<std::string>&)>&)const;
    void require_live_default_discovery_row_current_completed(const detail::BnwNativeGlobalResourceWarmOwner&,
        const detail::BnwNativeEarlyDefaultDiscoveryRow&,std::size_t actual_issued_ordinal,
        const detail::BnwNativeCompletedColdSourceLoan&,const detail::BnwNativeColdFilesystemOwner&)const;
    std::unique_ptr<detail::BnwNativeDescriptorSelectedStreamLoan> lend_native_descriptor_stream_source(
        const detail::BnwNativeGlobalResourceWarmOwner&,const detail::BnwNativeArchiveRecordDiagnostic*,
        const std::u16string*,const std::string*,const std::string&)const;
    void require_native_descriptor_stream_loan_current_completed(const detail::BnwNativeGlobalResourceWarmOwner&,
        const detail::BnwNativeDescriptorSelectedStreamLoan&,const detail::BnwNativeCompletedColdSourceLoan&,
        const detail::BnwNativeColdFilesystemOwner&)const;
    bool require_native_descriptor_stream_loan_if_active(const detail::BnwNativeGlobalResourceWarmOwner&,
        const detail::BnwNativeDescriptorSelectedStreamLoan&)const;
    void require_native_descriptor_stream_loan_current(const detail::BnwNativeGlobalResourceWarmOwner&,
        const detail::BnwNativeDescriptorSelectedStreamLoan&)const;
    void with_precategory_native_resource_record_completed(const detail::BnwNativeGlobalResourceWarmOwner&,const char*,
        const detail::BnwNativeCompletedColdSourceLoan&,const detail::BnwNativeColdFilesystemOwner&,
        const std::function<void(const detail::BnwNativeArchiveRecordDiagnostic*,const std::u16string*,const std::string*)>&)const;
    void require_native_owned_completed_resource_binding_current(const detail::BnwNativeCompletedResourceBankCapability&,
        const detail::BnwNativeCompletedColdSourceLoan&,const detail::BnwNativeColdFilesystemOwner&)const;
    void require_native_owned_completed_skin_instance_current(const detail::BnwNativeCompletedResourceBankCapability&,
        const detail::BnwNativeOwnedUiSkinPoststageInstance&,const detail::BnwNativeCompletedColdSourceLoan&,
        const detail::BnwNativeColdFilesystemOwner&)const;
    void with_precategory_native_resource_record(const detail::BnwNativeGlobalResourceWarmOwner&,const char*,
        const std::function<void(const detail::BnwNativeArchiveRecordDiagnostic*,const std::u16string*,const std::string*)>&,const void* actual_preselection_invocation=nullptr) const;
    void require_global_color_bank_current(const detail::BnwNativeGlobalColorRegistryBank&,
        const content::XmlNode*,std::uint64_t)const;
    friend class detail::BnwNativeDefaultCommonProduction;
    friend class detail::BnwNativePackageCategory2PairOperation;
    std::vector<std::shared_ptr<const detail::BnwNativePackageCategory2ConstructorRead>>
        lend_native_package_category2_constructor_reads(
            const std::shared_ptr<const BnwNativePackageDiscoveryOwner>&) const;
    friend class detail::BnwNativeCommon13ArchiveChannel;
    void publish_native_primary_default_source()const;
    void execute_common13_source(detail::NativeResourceRecordIndex&,
        BnwNativePackageDiscoveryFrame&,std::vector<std::filesystem::path>&)const;
    void require_common_channel_current(const detail::BnwNativeCommon13ArchiveChannel&,bool)const;

    // Genuine owned archive operations. DiagnosticProjection is a private
    // compile-time observation choice, never caller authority or readiness.
    template<bool DiagnosticProjection>
    std::conditional_t<DiagnosticProjection, BnwNativeFirstArchiveMetadataDiagnostic, void> execute_owned_prepare_native_dx9_first_archive_metadata() const;
    template<bool DiagnosticProjection>
    std::conditional_t<DiagnosticProjection, BnwNativeFirstArchiveAdmissionDiagnostic, void> execute_owned_admit_native_dx9_first_archive_entries() const;
    template<bool DiagnosticProjection>
    std::conditional_t<DiagnosticProjection, BnwNativeFirstArchiveCacheDiagnostic, void> execute_owned_publish_native_dx9_first_archive_cache() const;
    template<bool DiagnosticProjection>
    std::conditional_t<DiagnosticProjection, BnwNativePrimary32ArchiveDiagnostic, void> execute_owned_complete_native_dx9_primary32_archives() const;
    // Uncalled production prerequisite until genuine Default/Common bridge
    // and native activation operands are implemented. No caller switch.
    void prepare_native_dx9_primary_source_prerequisite() const;
    // Private staged production operand only; no externally visible Default
    // commit or native activation is authorized by this capture.
    friend class detail::BnwNativeCurrentDefaultSource;
    void assign_fresh_native_current_default_source() const;
    void require_current_default_source_operation_current(const detail::BnwNativeCurrentDefaultSource&) const;
    std::unique_ptr<const detail::BnwNativeCurrentDefaultSelection>
        select_native_current_default_record(std::u16string_view) const;
    void require_current_native_default_selection(const detail::BnwNativeCurrentDefaultSelection&) const;
    void prepare_native_dx9_default_record_projection() const;
    void require_prepared_native_dx9_default_record_projection() const;
    void require_default_projection_capture_current(
        const detail::BnwNativeArchiveEntryAdmissionArena&) const;
    void require_default_projection_operation_current(
        const detail::BnwNativePrimary32RecordSourceBindings&,
        const detail::NativeResourceRecordIndex&) const;
    void validate_default_projection_source(
        const detail::BnwNativePrimary32RecordSourceBindings&,
        const detail::NativeResourceRecordIndex&,bool publication_fields_loan=false) const;
    // Only the actual pending/prepared installed issuer calls; no public switch.
    void validate_owned_installed_default_projection_source(
        const detail::BnwNativePrimary32RecordSourceBindings&,
        const detail::NativeResourceRecordIndex&) const;
    void require_primary32_snapshot_capture_current(const detail::BnwNativeArchiveEntryAdmissionArena&) const;
    void require_primary32_snapshot_operation_current(
        const detail::BnwNativePrimary32RecordSourceBindings&,
        const detail::BnwNativePrimary32RecordSnapshotOwner*) const;
    friend class detail::BnwNativePrimary32LoopJournalOwner;
    detail::BnwNativePrimary32JournalFrameDiagnostic validate_postpublished_primary32_journal(
        const detail::BnwNativePrimary32PostpublishedJournalOperation&,
        const detail::BnwNativeCallbackOwnerGuard<BnwNativePackageDiscoveryOwner>&)const;
    void validate_postpublished_stored_primary32_snapshot(
        const detail::BnwNativeCallbackOwnerGuard<BnwNativePackageDiscoveryOwner>&)const;
    void validate_postpublished_prepared_default_projection(
        const detail::BnwNativeCallbackOwnerGuard<BnwNativePackageDiscoveryOwner>&)const;
    void require_postpublished_snapshot_binding_current(
        const detail::BnwNativePrimary32RecordSourceBindings&,
        const detail::BnwNativePrimary32RecordSnapshotOwner*)const;
    // Diagnostic only: 0 inactive, 1 ordinary Journal, 2 stored Snapshot, 3 projection.
    friend unsigned bnw_native_primary32_postpublished_prefix_phase_for_test(
        const BnwNativePackageDiscoveryOwner&)noexcept;
    friend unsigned bnw_native_postpublisher_prefix_prelude_phase_for_test(
        const BnwNativePackageDiscoveryOwner&)noexcept;
    // Hidden ADL observer only; never grants retained-source/current authority.
    friend bool bnw_native_primary32_new_step_acceptance_active_for_test(
        const BnwNativePackageDiscoveryOwner&)noexcept;
    friend bool bnw_native_primary32_completed_pass_active_for_test(
        const BnwNativePackageDiscoveryOwner&)noexcept;
    // 0 inactive; 1 pre-brand capture; 2 pending projection; 3 prepared projection.
    friend unsigned bnw_native_primary32_record_projection_journal_phase_for_test(
        const BnwNativePackageDiscoveryOwner&)noexcept;
    // Read-only actual leaf operation: 0 inactive, 1 construction, 2 stored current.
    friend unsigned bnw_native_primary32_snapshot_leaf_phase_for_test(
        const BnwNativePackageDiscoveryOwner&)noexcept;
    // Read-only actual post-Journal tail: 0 inactive, 2 pending, 3 prepared.
    friend unsigned bnw_native_primary32_default_projection_tail_phase_for_test(
        const BnwNativePackageDiscoveryOwner&)noexcept;
    // Read-only actual precommit Prefix pass: 0 inactive, 1 prerequisite, 2 projection.
    friend unsigned bnw_native_primary32_publication_prefix_phase_for_test(
        const BnwNativePackageDiscoveryOwner&)noexcept;
    // Read-only actual firstarchive returned-state post-Prefix borrower.
    friend bool bnw_native_firstarchive_post_prefix_active_for_test(
        const BnwNativePackageDiscoveryOwner&)noexcept;
    // Diagnostic actual unpublished Default passes: 0 inactive, 1 A, 2 B.
    friend unsigned bnw_native_primary32_default_publishvalidation_phase_for_test(
        const BnwNativePackageDiscoveryOwner&)noexcept;
    bool has_installation_primary32_source_without_callbacks()const noexcept;
    std::pair<std::size_t,std::uint32_t> validate_installation_primary32_new_step_acceptance(
        const detail::BnwNativePrimary32LoopJournalOwner&,const detail::BnwNativePrimary32SourceBindings&,
        const detail::BnwNativePrimary32SourceBindings&,
        const detail::BnwNativeCallbackOwnerGuard<detail::BnwNativePrimary32LoopJournalOwner>&,std::size_t)const;
    void claim_primary32_journal_creation() const;
    std::weak_ptr<const BnwNativePackageDiscoveryOwner> primary32_journal_source_identity() const;
    detail::BnwNativePrimary32SourceBindings capture_completed_primary32_first_step() const;
    detail::BnwNativePrimary32SourceBindings produce_next_primary32_step(
        const detail::BnwNativePrimary32LoopJournalOwner&, std::size_t) const;
    void require_primary32_pending_step_current(const detail::BnwNativePrimary32SourceBindings&) const;
    void require_primary32_retained_step_current(const detail::BnwNativePrimary32LoopJournalOwner&,
        const detail::BnwNativePrimary32SourceBindings&) const;
    void require_primary32_publication_fields_current(const detail::BnwNativePrimary32LoopJournalOwner&,
        const detail::BnwNativePrimary32SourceBindings&) const;
    void require_primary32_completion_observation_current(const detail::BnwNativePrimary32LoopJournalOwner&,
        const detail::BnwNativeCallbackOwnerGuard<BnwNativePackageDiscoveryOwner>&)const;
    friend class detail::BnwNativeArchiveV6EntryAdmissionOwner;
    friend class detail::BnwNativeArchiveEntryAdmissionArena;
    friend class detail::BnwNativeUiSkinRetainedOccurrence;
    void require_ui_skin_retained_occurrence_current(
        const detail::BnwNativeUiSkinRetainedOccurrence&) const;
    friend class detail::BnwNativeSharedFirstLineage;
    friend class detail::BnwNativeCategoryNamespaceLineage;
    friend class BnwNativePackageCategorySourceOwner;
    friend class BnwAudioCompletedResourceOwner;
    // Only the actual installed Boot constructor can issue this stack origin.
    class InstalledBootConstructionOrigin;
    friend unsigned bnw_native_installed_boot_construction_phase_for_test(const BnwNativePackageDiscoveryOwner&)noexcept;
    friend class BnwNativeResourceBootSettingsOwner;
    const assets::ResourceMountTable& installation_boot_source_table(const BnwNativeResourceBootSettingsOwner* actual=nullptr)const;
    const assets::ResourceMountTable& installation_directory_source_table(const BnwNativeUserDirectoryOwner&)const;
    std::weak_ptr<const BnwNativePackageDiscoveryOwner> installation_boot_source_factory()const;
    const assets::ResourceMountTable& installation_boot_source_table(const InstalledBootConstructionOrigin&)const;
    std::weak_ptr<const BnwNativePackageDiscoveryOwner> installation_boot_source_factory(const InstalledBootConstructionOrigin&)const;
    std::filesystem::path installation_boot_source_game_root(const InstalledBootConstructionOrigin&)const;
    const assets::ResourceMountTable& installation_directory_source_table(const BnwNativeUserDirectoryOwner&,const InstalledBootConstructionOrigin&)const;
    void register_installation_boot_directory(std::shared_ptr<const BnwNativeUserDirectoryOwner>,const InstalledBootConstructionOrigin&)const;
    class InstalledFilesystemDocumentsLoan;
    std::shared_ptr<InstalledFilesystemDocumentsLoan> borrow_installed_filesystem_documents()const;
    std::shared_ptr<InstalledFilesystemDocumentsLoan> borrow_installed_filesystem_documents(const InstalledBootConstructionOrigin&)const;
    std::filesystem::path installation_boot_source_game_root()const;
    void register_installation_boot_directory(std::shared_ptr<const BnwNativeUserDirectoryOwner>)const;
    static void with_installation_boot_parent_loan(const BnwNativeResourceBootSettingsOwner&,
        const std::weak_ptr<const BnwNativePackageDiscoveryOwner>&,
        const std::function<void(const BnwNativePackageDiscoveryOwner&)>&);
    friend class BnwNativeUserDirectoryOwner;
    //Directory alone invokes this callback-only actual factory Parent loan.
    //No shared internal Parent pin or caller count is lent to the consumer.
    static void with_native_user_directory_parent_loan(
        const BnwNativeUserDirectoryOwner&,
        const std::weak_ptr<const BnwNativePackageDiscoveryOwner>& actual_factory,
        const std::function<void(const BnwNativePackageDiscoveryOwner&)>&);
    friend class BnwNativePackageArchiveTimeOwner;
    void require_category_source_operation_current(const BnwNativePackageCategorySourceOwner&) const;
    void require_category_native_query_parent_current(const BnwNativePackageCategorySourceOwner&)const;
    // Sole private continuation of the actual category seal operation.
    void issue_native_ui_skin_poststage_instances(const BnwNativePackagePreSkinOwner&) const;
    void require_native_ui_skin_poststage_instances_current() const;
    void require_category_archive_borrow_current(const BnwNativePackageArchiveTimeOwner&) const;
    const std::string& retained_descriptor_raw_for_category(std::string_view) const;
    void require_category_pre_skin_borrow_current(const BnwNativePackagePreSkinOwner&) const;
    void require_category_completion_for(const BnwNativePackagePreSkinOwner&,
        const detail::BnwNativeCategoryCompletionParentPin&) const;
    std::shared_ptr<detail::BnwNativeSharedFirstLineage> shared_first_for_arena(
        const detail::BnwNativeColdFilesystemOwner&) const;
    void require_shared_first_current(const detail::BnwNativeSharedFirstLineage&) const;
    std::shared_ptr<detail::BnwNativeCategoryNamespaceLineage> claim_category_namespace_lineage(
        std::shared_ptr<const BnwNativePackagePreSkinOwner>,std::size_t,unsigned) const;
    void require_category_namespace_lineage_current(
        const detail::BnwNativeCategoryNamespaceLineage&) const;
    void require_archive_namespace_construction_current(
        const detail::BnwNativeColdFilesystemOwner&) const;
    friend class detail::BnwNativeArchiveBasenameCacheOwner;
    void require_archive_cache_initialization_current(
        const detail::BnwNativeArchiveEntryAdmissionArena&) const;
    void require_archive_cache_lookup_current(
        const detail::BnwNativeArchiveBasenameCacheOwner&,
        const detail::BnwNativeArchiveEntryAdmissionArena&,
        const detail::BnwNativeColdArchiveRequest&) const;
    void require_archive_cache_publication_current(
        const detail::BnwNativeArchiveBasenameCacheOwner&,
        const detail::BnwNativeArchiveEntryAdmissionArena&,
        const detail::BnwNativeArchiveCacheLookupOwner&,
        const detail::BnwNativeArchiveV6EntryAdmissionOwner&) const;
    // Callback-free check of the actual active Package fence and SAME brands.
    // The private entry producer cannot hide caught Package reentry/loss.
    void require_archive_entry_operation_current(
        const detail::BnwNativeArchiveEntryAdmissionArena&,
        const detail::BnwNativeArchiveV6StreamHeaderOwner&,
        const detail::BnwNativeGlobalArchivePrefixRecord&) const;
    friend class BnwNativePackagePreSkinOwner;
    friend SelectedBnwBootstrapResult bootstrap_selected_bnw_preview_from_early_package(
        std::shared_ptr<const BnwNativePackageDiscoveryOwner>, std::int64_t);
    // Sole source-ordered real bootstrap consumes this capability after every
    // throwing guard. Retains a nonowning identity only; no Package->Host cycle.
    friend SelectedBnwBootstrapResult bootstrap_selected_bnw_preview_from_completed_resources(
        std::shared_ptr<const BnwNativePackageDiscoveryOwner>,
        std::shared_ptr<const detail::BnwNativeCompletedResourceBankCapability>,
        std::shared_ptr<const detail::BnwNativeCompletedColorQuery>,
        std::shared_ptr<const detail::BnwNativeCompletedSelectedRead>,std::int64_t);
    void require_native_owned_completed_host_observation_current(
        const detail::BnwNativeCompletedParentObservation&)const;
    void require_same_selected_host_completed(const SelectedBnwPreviewHost&)const;
    void require_selected_host_adoption_available_completed(
        const detail::BnwNativeCompletedParentObservation&)const;
    void consume_selected_host_adoption_completed(const SelectedBnwPreviewHost&,
        const detail::BnwNativeCompletedParentObservation&)const;
    void require_selected_host_adoption_available() const;
    void consume_selected_host_adoption(const SelectedBnwPreviewHost&) const;
    const std::filesystem::path& original_assets_root_for_pre_skin() const;
    // Existing PreSkin friend only. Caller XML/language/GUID cannot issue this
    // SAME parent's retained original descriptor; no public snapshot API.
    const content::PackageManifest& retained_descriptor_for_pre_skin(std::string_view relative) const;
    static std::shared_ptr<const BnwNativePackageDiscoveryOwner> create_from_inputs(
        const std::filesystem::path&, const std::filesystem::path&,
        const content::Catalog&, const runtime::SavedContentProfile&,
        std::shared_ptr<const BnwNativeUiColorOwner>,
        std::shared_ptr<const BnwNativeEarlyInputLease>);
    // Root's actual post-return wrapper only. No ordinary caller can issue it.
    std::shared_ptr<const BnwNativePinnedInstallationEpoch>
    capture_native_owned_installation_epoch_after_normal_return(
        const std::shared_ptr<detail::BnwNativeGlobalResourceWarmOwner>&,
        std::uint64_t before) const;
    struct Impl;
    std::shared_ptr<Impl> impl_;
    explicit BnwNativePackageDiscoveryOwner(std::shared_ptr<Impl>);
};
}
