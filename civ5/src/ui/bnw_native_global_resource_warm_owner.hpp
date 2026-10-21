#pragma once
#include "bnw_native_package_fstring_field0.hpp"
#include "civ5/ui/bnw_native_package_discovery_owner.hpp"
#include <atomic>
#include "bnw_native_owned_skin_occurrence.hpp"
#include <functional>
#include <memory>
#include <thread>
#include <vector>
#include "bnw_native_package_descriptor_byte_reader.hpp"
namespace civ5::runtime {class BnwSelectedLanguageInputOwner;}
namespace civ5::ui::detail {
class BnwNativePreDx9DiscoveryOwner;
class BnwNativeInstalledDefaultCommonSource;
class BnwNativeInstalledDescriptorEnumeration;
class BnwNativeSkinResourceQueryOutputProducer;
class BnwNativePackageLoadActor;
class BnwNativeStandalonePackageLoaded;
class BnwNativeOwnedUiSkinPoststageInstance;
class BnwNativeOwnedSkinSelectedInput;
class BnwNativeOwnedSkinBootPolicy;
class BnwNativeOwnedSkinConsumer;
class BnwNativeOwnedSkinGlobalSelectionKernel;
class BnwNativeGlobalColorRegistryBank;
class BnwNativeResourceDiscoveryActor;
class BnwNativeArchiveV6StreamHeaderOwner;
class BnwNativeCategoryNamespaceChannel;
class BnwNativeCompletedColdSourceLoan;
class BnwNativeCategoryArchiveChannel;
class BnwNativeCategoryNamespaceLineage;
class BnwNativeArchiveCachePublicationOwner;
class BnwNativeArchiveBasenameCacheOwner;
class BnwNativeArchiveCacheLookupOwner;
class BnwNativeColdArchiveRequest;
class BnwNativeOriginalStatRead;
class BnwNativeSharedFirstLineage;
//Closed actual Parent-issued source loan, no public aggregate/factory or copied
//path can issue it. Only authentic selected node, prefix/cache or Original stat.
class BnwNativeDescriptorSelectedStreamLoan final {
    friend class ::civ5::ui::BnwNativePackageDiscoveryOwner;
    friend class BnwNativeGlobalResourceWarmOwner;
    const BnwNativeArchiveRecordDiagnostic* native_node_=nullptr;
    std::uint32_t crc_=0,length_=0,offset_=0,packed_prefix_=0,symbolic_slot_=0;
    const std::string* actual_first_=nullptr;
    // Sole Parent mint binds genuine installed receipt control, not an address
    // into FIRST's swappable map. Empty control keeps the legacy pointer path.
    std::weak_ptr<const void> installed_descriptor_row_;
    std::shared_ptr<BnwNativeSharedFirstLineage> installed_first_owner_;
    std::string installed_first_text_;
    std::string receiver_acp_;
    std::u16string prefix_;
    std::weak_ptr<const BnwNativeArchiveV6StreamHeaderOwner> header_;
    std::weak_ptr<const BnwNativeArchiveCachePublicationOwner> publication_;
    std::weak_ptr<const BnwNativeOriginalStatRead> original_stat_;
    std::filesystem::path original_path_;
    BnwNativeDescriptorSelectedStreamLoan()=default;
  public:
    BnwNativeDescriptorSelectedStreamLoan(const BnwNativeDescriptorSelectedStreamLoan&)=delete;
};
// Private quote only; pointers are reauthenticated by the actual Warm owner.
struct BnwNativeEarlyDefaultDiscoveryRow {
    std::uint32_t key;const void* actual_record;const std::string* actual_first;std::string filename;
};
//Actual Parent-owned live source owner, issued after genuine Default/Common
//returns, BEFORE Category/ColorBank. Future managers are not synthetic misses.
//Current native-node borrow supports the exact pre-Category HIGH phase;
//Category/Skin stage-aware queries and Q0 remain explicit absent suppliers.
class BnwNativeGlobalResourceWarmOwner final {
    friend class BnwNativePreDx9DiscoveryOwner;
    std::shared_ptr<BnwNativePreDx9DiscoveryOwner> pre_dx9_owner_;
    const std::shared_ptr<const BnwNativeInstalledDefaultCommonSource> installed_common_source_;
    friend class BnwNativeInstalledDescriptorEnumeration;
    friend class ::civ5::ui::BnwNativePackageDiscoveryOwner;
    friend class BnwNativeSkinResourceQueryOutputProducer;
    friend class BnwNativeGlobalColorRegistryBank; //Actual PackagePin accounts Warm internal pins.
    friend class BnwNativePackageLoadActor;
    friend class BnwNativeStandalonePackageLoaded;
    friend class BnwNativeOwnedSkinOccurrence;
    friend class BnwNativeCategoryNamespaceChannel;
    friend class BnwNativeCategoryArchiveChannel;
    friend class BnwNativeColdFilesystemOwner;
    friend class BnwNativeArchiveBasenameCacheOwner;
    friend class BnwNativeArchiveCacheLookupOwner;
    friend class BnwNativeCategoryNamespaceLineage;
    friend class BnwNativeResourceDiscoveryActor;
    const std::thread::id thread_=std::this_thread::get_id();
    const std::weak_ptr<const ::civ5::ui::BnwNativePackageDiscoveryOwner> parent_;
    const std::shared_ptr<const void> epoch_;
    std::atomic<bool> closed_{false};
    mutable std::atomic<bool> failed_{false}; // Const validation can permanently record failure.
    std::atomic<bool> busy_{false};
    mutable std::size_t parent_pins_=0; //Only actual weak-lock operations below.
    std::shared_ptr<BnwNativeResourceDiscoveryActor> discovery_actor_;
    std::vector<std::shared_ptr<BnwNativePackageLoadActor>> preload_receivers_;
    //Only actual successful738F00/40CD60 returns enter this ordered factory
    //vector. Token weak-locks or a copied package list are not its authority.
    std::vector<std::shared_ptr<const BnwNativeStandalonePackageLoaded>> returned_standalone_packages_;
    void require_returned_standalone_package_loaded_current(const BnwNativeStandalonePackageLoaded&)const;
    void require_current_completed(const BnwNativeCompletedColdSourceLoan&,const BnwNativeColdFilesystemOwner&)const;
    void require_returned_standalone_package_loaded_current_completed(
        const BnwNativeStandalonePackageLoaded&,const BnwNativeCompletedColdSourceLoan&,const BnwNativeColdFilesystemOwner&)const;
    void require_same_selected_language_input_completed(
        const runtime::BnwSelectedLanguageInputOwner&,const BnwNativeCompletedColdSourceLoan&,const BnwNativeColdFilesystemOwner&)const;
    void require_owned_namespace_channel_current_completed(
        const BnwNativeCategoryNamespaceChannel&,const BnwNativeCompletedColdSourceLoan&,const BnwNativeColdFilesystemOwner&)const;
    class DescriptorStreamSource;
    struct ParentPin;
    class PreselectionCanonicalInputLoan;
    PreselectionCanonicalInputLoan* active_preselection_canonical_input_=nullptr;
    void require_preselection_canonical_query_without_callbacks(
        const PreselectionCanonicalInputLoan&,const void*,const char*)const;
    std::unique_ptr<BnwNativeDescriptorSelectedStreamLoan> select_preselection_canonical_input(
        const std::shared_ptr<BnwNativePackageLoadActor>&,const ParentPin&);
    std::vector<std::shared_ptr<BnwNativePackageDescriptorByteReader::Source>> descriptor_sources_;
    std::shared_ptr<BnwNativePackageDescriptorByteReader::Source> issue_descriptor_stream_for_receiver(
        const std::shared_ptr<BnwNativePackageLoadActor>&);
    void require_descriptor_stream_loan_current(const BnwNativeDescriptorSelectedStreamLoan&)const;
    void require_descriptor_stream_loan_current_completed(const BnwNativeDescriptorSelectedStreamLoan&,
        const BnwNativeCompletedColdSourceLoan&,const BnwNativeColdFilesystemOwner&)const;
    bool require_descriptor_stream_loan_if_active(const BnwNativeDescriptorSelectedStreamLoan&)const;
    std::shared_ptr<const runtime::BnwSelectedLanguageInputOwner> borrow_selected_language_input()const;
    void require_same_selected_language_input(const runtime::BnwSelectedLanguageInputOwner&)const;
    struct OwnedActivationReceipt;
    struct OwnedActivationDescriptorOperation;
    OwnedActivationDescriptorOperation* active_owned_activation_descriptor_=nullptr;
    void with_owned_activation_initial_descriptor(const BnwNativeStandalonePackageLoaded&,
        const std::function<void()>&,const std::function<void(OwnedActivationDescriptorOperation&)>&);
    std::vector<std::shared_ptr<OwnedActivationReceipt>> owned_activation_receipts_;
    bool configured_activation_policy_returned_=false;
    void prepare_owned_configured_activation_policy();
    struct InstalledStartupAdmission;
    std::shared_ptr<InstalledStartupAdmission> installed_startup_admission_;
    bool installed_startup_admission_attempted_=false;
    void prepare_installed_startup_admission();
    void require_installed_startup_admission_returned()const;
    bool apply_owned_category_mask_transition(const BnwNativeStandalonePackageLoaded&,std::uint32_t,bool);
    struct OwnedCategoryStage;
    struct InstalledInitialCategoryDriver;
    std::shared_ptr<InstalledInitialCategoryDriver> installed_initial_category_driver_;
    bool installed_initial_category_prefix_attempted_=false;
    void prepare_installed_initial_category_prefix();
    bool construct_installed_initial_category_current();
    void drive_installed_initial_categories();
    void require_installed_initial_categories_returned()const;
    void finish_installed_initial_category_effects(const std::shared_ptr<OwnedCategoryStage>&);
    bool installed_initial_category_driver_attempted_=false;
    struct InstalledColorSkinDriver;
    std::shared_ptr<InstalledColorSkinDriver> installed_color_skin_driver_;
    bool installed_color_skin_attempted_=false;
    void drive_installed_color_skin();
    void require_installed_color_skin_identity()const;
    void require_installed_color_skin_operation_current()const;
    void require_installed_color_skin_category_stage_identity(const OwnedCategoryStage&)const;
    void require_installed_color_skin_category_stage_current(const OwnedCategoryStage&)const;
    void with_installed_color_skin_base_input(const std::function<void(
        const std::shared_ptr<const BnwNativeOwnedSkinSelectedInput>&)>&);
    void require_installed_color_skin_returned()const;
    struct InstalledInitialSkinSelection;
    std::shared_ptr<InstalledInitialSkinSelection> installed_initial_skin_selection_;
    bool installed_initial_skin_selection_attempted_=false;
    void prepare_installed_initial_skin_selection_prefix();
    void require_installed_initial_skin_selection_prefix_returned()const;
    struct InstalledInitialSkinEffects;
    std::shared_ptr<InstalledInitialSkinEffects> installed_initial_skin_effects_;
    bool installed_initial_skin_effects_attempted_=false;
    void drive_installed_first_common_skin();
    void require_installed_initial_skin_effects_identity()const;
    void require_installed_initial_skin_effects_current()const;
    void require_installed_initial_skin_effect_stage_identity(const OwnedCategoryStage&)const;
    void require_installed_initial_skin_effect_stage_current(const OwnedCategoryStage&)const;
    void require_installed_first_common_skin_returned()const;
    struct InstalledPcPublication;
    std::shared_ptr<InstalledPcPublication> installed_pc_publication_;
    bool installed_pc_publication_attempted_=false;
    void publish_installed_pc();
    void require_installed_pc_publication_returned()const;
    struct InstalledPcSkinEffects;
    std::shared_ptr<InstalledPcSkinEffects> installed_pc_skin_effects_;
    bool installed_pc_skin_effects_attempted_=false;
    void drive_installed_pc_skin();
    void require_installed_pc_skin_returned()const;
    struct InstalledInitialConstraintClose;
    std::shared_ptr<InstalledInitialConstraintClose> installed_initial_constraint_close_;
    bool installed_initial_constraint_close_attempted_=false;
    void close_installed_initial_constraints();
    void require_installed_initial_constraint_close_returned()const;
    struct InstalledGlobalSkinEffects;
    std::shared_ptr<InstalledGlobalSkinEffects> installed_global_skin_effects_;
    bool installed_global_skin_effects_attempted_=false;
    void drive_installed_global_skin();
    void require_installed_global_skin_returned()const;
    struct InstalledLateSpeechTail;
    std::shared_ptr<InstalledLateSpeechTail> installed_late_speech_tail_;
    bool installed_late_speech_tail_attempted_=false;
    void drive_installed_late_speech_and_mappings();
    void require_installed_late_speech_and_mappings_returned()const;
    void require_installed_global_selected_resource_current(const BnwNativeOwnedSkinOccurrence&,unsigned)const;
    void require_installed_skin_read_receipt_identity(const void*)const;
    void require_installed_initial_category_stage_identity(const OwnedCategoryStage&)const;
    void require_installed_initial_category_stage_current(const OwnedCategoryStage&)const;
    class OwnedSelectedCategorySource;
    class OwnedLooseSource;
    std::vector<std::shared_ptr<OwnedCategoryStage>> owned_category_stages_;
    const OwnedCategoryStage* active_owned_category_stage_=nullptr;
    struct OwnedCategoryPreconstructorOperation;
    void require_installed_initial_preconstructor_current(const OwnedCategoryPreconstructorOperation&)const;
    OwnedCategoryPreconstructorOperation* active_owned_category_preconstructor_=nullptr;
    void require_owned_category_preconstructor_query_without_callbacks(
        const OwnedCategoryPreconstructorOperation&,const void*)const;
    bool owned_preconstructor_prior_stage_without_callbacks(const OwnedCategoryStage&)const;
    void with_owned_category_preconstructor_record(const void*,const char*,
        const std::function<void(const BnwNativeArchiveRecordDiagnostic*,const std::u16string*,const std::string*)>&);
    const BnwNativeCategoryNamespaceChannel* initializing_owned_namespace_=nullptr;
    void require_owned_category_stage_current(const OwnedCategoryStage&)const;
    void require_owned_completed_category_stage_current(const OwnedCategoryStage&)const;
    void require_owned_completed_category_stage_current(const OwnedCategoryStage&,
        const BnwNativeCompletedColdSourceLoan&,const BnwNativeColdFilesystemOwner&)const;
    void replay_owned_category_stage_io_current_completed(const OwnedCategoryStage&,
        const BnwNativeCompletedColdSourceLoan&,const BnwNativeColdFilesystemOwner&)const;
    void with_owned_manager_native_record_completed(const OwnedCategoryStage&,std::uint32_t,
        const BnwNativeCompletedColdSourceLoan&,const BnwNativeColdFilesystemOwner&,
        const std::function<void(const BnwNativeArchiveRecordDiagnostic*,const std::u16string*,const std::string*)>&)const;
    void require_owned_completed_category_stage_fence_without_callbacks(const OwnedCategoryStage&)const;
    void replay_owned_category_stage_io_current(const OwnedCategoryStage&)const;
    void require_owned_namespace_channel_current(const BnwNativeCategoryNamespaceChannel&,bool initialization)const;
    void require_owned_namespace_lineage_current(const BnwNativeCategoryNamespaceLineage&)const;
    void prepare_owned_first_common_category_manager();
    void prepare_owned_category_manager(std::size_t,std::uint32_t);
    std::shared_ptr<OwnedCategoryStage> borrow_owned_skin_manager_stage(
        const std::shared_ptr<const BnwNativeOwnedUiSkinPoststageInstance>&,unsigned,bool construct);
    void require_owned_skin_stage_identity_without_callbacks(const OwnedCategoryStage&)const;
    void apply_owned_native_paks_list(OwnedCategoryStage&,const std::vector<std::string>&);
    void resume_owned_category_paks_effects(const std::shared_ptr<OwnedCategoryStage>&);
    void resume_owned_category_final_effects(const std::shared_ptr<OwnedCategoryStage>&);
    void drive_owned_common_gameplay_categories();

    bool common_gameplay_driver_returned_=false;
    std::uint64_t owned_skin_operation_epoch_=0,active_owned_skin_operation_epoch_=0;
    std::weak_ptr<BnwNativeGlobalColorRegistryBank> owned_bank_receiver_;
    const void* owned_bank_source_identity_=nullptr;
    bool owned_bank_source_attachment_returned_=false;
    std::weak_ptr<const BnwNativeStandalonePackageLoaded> active_owned_skin_package_;
    const void* active_owned_skin_resource_loan_=nullptr;
    std::uint64_t active_owned_skin_resource_epoch_=0;
    struct OwnedSkinExtraAttributeReceipt;
    std::vector<std::shared_ptr<OwnedSkinExtraAttributeReceipt>> owned_skin_extra_attribute_receipts_;
    std::shared_ptr<BnwNativeOwnedSkinBootPolicy> owned_skin_boot_policy_;
    const void* owned_skin_effect_source_identity_=nullptr;
    std::uint64_t owned_skin_effect_operation_epoch_=0;
    const void* owned_skin_effect_bank_operation_=nullptr;
    std::shared_ptr<const BnwNativeOwnedSkinConsumer> owned_skin_base_game_consumer_;
    const void* owned_skin_global_source_identity_=nullptr;
    std::shared_ptr<const BnwNativeOwnedSkinGlobalSelectionKernel> owned_skin_global_kernel_;
    struct OwnedSkinBankFinalizationReceipt;
    std::shared_ptr<const OwnedSkinBankFinalizationReceipt> owned_skin_bank_finalization_return_;
    struct OwnedSkinStateWrite;
    std::vector<std::shared_ptr<OwnedSkinStateWrite>> owned_skin_state_writes_;
    struct OwnedSkinEnableReceipt;
    std::vector<std::shared_ptr<OwnedSkinEnableReceipt>> owned_skin_enable_receipts_;
    struct OwnedSkinOutputReceipt;
    std::shared_ptr<OwnedSkinOutputReceipt> borrow_owned_skin_output_receipt(
        const std::shared_ptr<const BnwNativeOwnedUiSkinPoststageInstance>&,unsigned,const char*,std::uint32_t)const;
    void ensure_owned_skin_loose_source(const std::shared_ptr<OwnedCategoryStage>&);
    void with_owned_stage_find_a(OwnedCategoryStage&,const std::string&,const std::function<void(const std::string&)>&);
    std::vector<std::shared_ptr<OwnedSkinOutputReceipt>> owned_skin_output_receipts_;
    struct OwnedSkinReadOperation;
    struct OwnedSkinReadReceipt;
    std::vector<std::shared_ptr<OwnedSkinReadOperation>> owned_skin_read_operations_;
    std::vector<std::shared_ptr<OwnedSkinReadReceipt>> owned_skin_read_receipts_;
    std::shared_ptr<OwnedSkinReadOperation> active_owned_skin_read_operation_;
    void restore_owned_skin_default_after_failure()noexcept;
    void require_owned_common_gameplay_terminal_current()const;
    void with_owned_skin_operation(const std::function<void()>&);
    std::vector<std::shared_ptr<const BnwNativeOwnedSkinOccurrence>> owned_skin_occurrences_;
    bool owned_skin_occurrence_driver_returned_=false;
    void prepare_owned_skin_occurrences();
    std::vector<std::shared_ptr<const BnwNativeOwnedUiSkinPoststageInstance>> owned_skin_instances_;
    bool owned_skin_instance_ctor_driver_returned_=false;
    void retain_owned_skin_instance(const std::shared_ptr<const BnwNativeOwnedSkinOccurrence>&,
        const std::shared_ptr<const BnwNativeOwnedUiSkinPoststageInstance>&);
    void complete_owned_skin_instance_constructors();
    void with_owned_skin_constructor_loan(const BnwNativeOwnedSkinOccurrence&,
        const std::function<void(const BnwNativeOwnedSkinOccurrence::ConstructorLoan&)>&)const;
    void resume_owned_first_common_paks_effects();
    void resume_owned_first_common_category_effects();
    void resume_owned_directory_file_effects(OwnedCategoryStage&);
    void require_owned_category_stage_fence_without_callbacks(const OwnedCategoryStage&)const;
    void with_owned_manager_native_record(const OwnedCategoryStage&,std::uint32_t,
        const std::function<void(const BnwNativeArchiveRecordDiagnostic*,const std::u16string*,const std::string*)>&)const;
    void require_owned_archive_channel_current(const BnwNativeCategoryArchiveChannel&,bool active)const;
    // Synchronous, source-owned factory/lookup only; never a public verdict.
    void with_owned_archive_factory_common_scope(
        const ::civ5::ui::BnwNativePackageDiscoveryOwner&,const BnwNativeColdFilesystemOwner&,
        const std::shared_ptr<const BnwNativeCategoryArchiveChannel>&,
        const std::function<void()>&,const std::function<void()>&)const;
    void with_owned_archive_lookup_common_scope(
        const ::civ5::ui::BnwNativePackageDiscoveryOwner&,const BnwNativeArchiveBasenameCacheOwner&,
        const std::shared_ptr<const BnwNativeColdArchiveRequest>&,
        const std::shared_ptr<const BnwNativeCategoryArchiveChannel>&,
        const std::function<void()>&,const std::function<void()>&)const;
    void with_owned_archive_common_operation(
        const ::civ5::ui::BnwNativePackageDiscoveryOwner&,const BnwNativeCategoryArchiveChannel&,
        const BnwNativeArchiveBasenameCacheOwner*,const std::shared_ptr<const BnwNativeColdArchiveRequest>*,
        const std::function<void()>&,const std::function<void()>&)const;
    bool load_owned_category_archive(OwnedCategoryStage&,const std::string&);
    class InstalledDescriptorRow;
    std::vector<std::shared_ptr<const InstalledDescriptorRow>> installed_descriptor_rows_;
    bool installed_descriptor_attempted_=false;
    mutable const BnwNativeInstalledDescriptorEnumeration* installed_enumeration_=nullptr;
    BnwNativeEarlyDefaultDiscoveryRow installed_descriptor_selected_quote(const InstalledDescriptorRow&)const;
    void require_installed_descriptor_loan_first_identity_without_callbacks(
        const BnwNativeDescriptorSelectedStreamLoan&,const std::string*)const;
    void prepare_owned_installed_descriptor_registration();
    std::vector<std::shared_ptr<const InstalledDescriptorRow>> borrow_installed_descriptor_discovery_rows()const;
    std::vector<std::shared_ptr<const InstalledDescriptorRow>> borrow_installed_descriptor_discovery_rows(
        const BnwNativeInstalledDescriptorEnumeration&)const;
    std::uint64_t installed_constructor_revision_without_callbacks()const;
    void require_installed_constructor_identity_without_callbacks(const InstalledDescriptorRow&,
        std::size_t,const void*,std::uint64_t)const;
    void require_installed_descriptor_discovery_row_current(const InstalledDescriptorRow&,std::size_t)const;

    void require_installed_descriptor_discovery_row_current_completed(const InstalledDescriptorRow&,std::size_t,
        const BnwNativeCompletedColdSourceLoan&,const BnwNativeColdFilesystemOwner&)const;
    bool discovery_attempted_=false;
    //Actual private execution journal, never a supplied completeness flag.
    bool early_receiver_driver_returned_=false,first_common_paks_attempted_=false;
    struct ParentPin {
        const BnwNativeGlobalResourceWarmOwner& source;
        const std::shared_ptr<const ::civ5::ui::BnwNativePackageDiscoveryOwner> parent;
        explicit ParentPin(const BnwNativeGlobalResourceWarmOwner& actual):source(actual),parent(actual.parent_.lock()) {
            if(!parent)throw std::runtime_error("Live query Package expired");++source.parent_pins_;
        }
        ~ParentPin(){--source.parent_pins_;}
        ParentPin(const ParentPin&)=delete;
    };
    std::vector<BnwNativeEarlyDefaultDiscoveryRow> borrow_default_discovery_rows()const {
        if(thread_!=std::this_thread::get_id()||closed_.load()||failed_.load())throw std::runtime_error("Early Discovery Warm retired/foreign thread");
        ParentPin retained(*this);return retained.parent->borrow_live_default_discovery_rows(*this);
    }
    void require_default_discovery_row_current(const BnwNativeEarlyDefaultDiscoveryRow& issued)const;
    void require_default_discovery_row_current_completed(const BnwNativeEarlyDefaultDiscoveryRow&,
        std::size_t actual_issued_ordinal,const BnwNativeCompletedColdSourceLoan&,const BnwNativeColdFilesystemOwner&)const;
    std::shared_ptr<BnwNativeResourceDiscoveryActor> borrow_actual_discovery_actor()const;
    BnwNativeGlobalResourceWarmOwner(std::weak_ptr<const ::civ5::ui::BnwNativePackageDiscoveryOwner> parent,
        std::shared_ptr<const void> epoch):parent_(std::move(parent)),epoch_(std::move(epoch)){}
    BnwNativeGlobalResourceWarmOwner(std::weak_ptr<const ::civ5::ui::BnwNativePackageDiscoveryOwner> parent,
        std::shared_ptr<const void> epoch,std::shared_ptr<const BnwNativeInstalledDefaultCommonSource> actual)
        :installed_common_source_(std::move(actual)),parent_(std::move(parent)),epoch_(std::move(epoch)){}
    static std::shared_ptr<BnwNativeGlobalResourceWarmOwner> borrow_from_actual_package(
        const ::civ5::ui::BnwNativePackageDiscoveryOwner&);
    void require_current()const {
        if(thread_!=std::this_thread::get_id()||closed_.load()||failed_.load())throw std::runtime_error("Live query source retired/foreign thread");
        ParentPin retained(*this);retained.parent->require_live_resource_query_current(*this);
    }
    void with_native_record_completed(const char*,const BnwNativeCompletedColdSourceLoan&,
        const BnwNativeColdFilesystemOwner&,const std::function<void(
        const BnwNativeArchiveRecordDiagnostic*,const std::u16string*,const std::string*)>&);
    void with_native_record(const char* input,const std::function<void(
        const BnwNativeArchiveRecordDiagnostic*,const std::u16string*,const std::string*)>& callback) {
        if(thread_!=std::this_thread::get_id()||closed_.load()||failed_.load())throw std::runtime_error("Live query source retired/foreign thread");
        //Retain exactly one accounted Package pin across Exit/catch. If the
        //consumer drops the last external Package, Parent rejects it, but its
        //owned Warm facade must stay alive until Exit has cleared the scope.
        ParentPin retained(*this);
        if(busy_.exchange(true)){failed_.store(true);throw std::runtime_error("Live query source reentry");}
        struct Exit{BnwNativeGlobalResourceWarmOwner& source;~Exit(){source.busy_.store(false);}} exit{*this};
        try{
            //Exactly one weak-lock Package pin, accounted by Parent's fence.
            retained.parent->with_precategory_native_resource_record(*this,input,callback);
            if(closed_.load()||failed_.load())throw std::runtime_error("Live query retired in consumer");
        }catch(...){failed_.store(true);throw;}
    }
public:
    ~BnwNativeGlobalResourceWarmOwner(){closed_.store(true);}
    BnwNativeGlobalResourceWarmOwner(const BnwNativeGlobalResourceWarmOwner&)=delete;
};
// Closed one synchronous installed enumeration, never a caller-owned grant.
// Real per-row Header/Default/Stat validators remain; only repeated Common
// infrastructure closes at the batch POST rather than at every row fence.
class BnwNativeInstalledDescriptorEnumeration final {
    friend class BnwNativeResourceDiscoveryActor;
    friend class BnwNativeGlobalResourceWarmOwner;
    friend class ::civ5::ui::BnwNativePackageDiscoveryOwner;
    using Source=BnwNativeResourceDiscoveryFilename::Source;
    struct Impl;
    std::unique_ptr<Impl> impl_;
    explicit BnwNativeInstalledDescriptorEnumeration(BnwNativeGlobalResourceWarmOwner&);
    void require_same(const BnwNativeGlobalResourceWarmOwner&)const;
    void record_source(const std::shared_ptr<const Source>&,std::size_t);
    void finish(std::uint32_t); // Call only after real final State.current_warm.
  public:
    ~BnwNativeInstalledDescriptorEnumeration();
    BnwNativeInstalledDescriptorEnumeration(const BnwNativeInstalledDescriptorEnumeration&)=delete;
    BnwNativeInstalledDescriptorEnumeration& operator=(const BnwNativeInstalledDescriptorEnumeration&)=delete;
};
} // namespace civ5::ui::detail
