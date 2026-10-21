#pragma once
#include "bnw_native_color_set64_storage.hpp"
#include "civ5/ui/detail/bnw_native_resource_bucket_order.hpp"
#include "bnw_native_skin_document_registration.hpp"
#include "bnw_native_fpk_mounted_catalog.hpp"
#include "civ5/runtime/bnw_stock_lua_operation_loan.hpp"
#include "civ5/ui/bnw_native_ui_color_owner.hpp"
#include <atomic>
#include <array>
#include <map>
#include <memory>
#include <optional>
#include <thread>
#include <stdexcept>
#include <vector>

namespace civ5::ui {
class BnwNativePackageDiscoveryOwner;
class BnwNativePackageCategorySourceOwner;
class BnwNativeUiColorOwner;
class BnwCivilopediaNativeWindowBinding;
class SelectedBnwPreviewHost;
namespace detail {
class BnwNativeUiSkinPoststageInstance;
class BnwNativeOwnedUiSkinPoststageInstance;
class BnwNativeGlobalColorRegistryBank;
class BnwNativeMountedStyleReader;
class BnwNativeOwnedSkinSelectedInput;
class BnwNativeCompletedResourceBankCapability;
class BnwNativeGlobalResourceQuery;
class BnwNativeGlobalResourceWarmOwner;
class BnwNativeResourceDiscoveryActor;
class BnwNativeCurrentDefaultSource;
struct NativeResourceRecordIndex;
struct BnwNativeArchiveRecordDiagnostic;
// Actual empty constructor fields, attached only to the SAME Package-issued
// instance. They have no public constructor, copied-map admission or issuer.
class BnwNativeColorSlotStorage final {
    friend class BnwNativeGlobalColorRegistryBank;
    struct Value {
        native_color_load_detail::ColorBits bits{};
        std::array<bool,4> known{};
    };
    std::map<std::uint32_t,Value> colors_;
    std::unique_ptr<BnwNativeColorSet64Storage> sets_;
    //8C is the source factory allocation extent, not a native field offset.
    //It retains the actual catalog DocumentHandle, never a loaded Style DOM.
    std::shared_ptr<void> resource_manager8c_;
    std::shared_ptr<void> resource_stage_; //SAME manager producer, Bank-only writes.
    BnwNativeColorSlotStorage()=default;
public:
    ~BnwNativeColorSlotStorage()=default;
    BnwNativeColorSlotStorage(const BnwNativeColorSlotStorage&)=delete;
};
// Private source producer prefix. Its cold reset/base load and Rules writes
// do NOT close the Skin stack or authorize a global hit/MISS. The missing
// Directory/Paks/style/priority effects and final462C20 cannot be supplied by
// a readiness bool, diagnostic registry or this prefix's empty slot fields.
class BnwNativeGlobalColorRegistryBank final
    :public std::enable_shared_from_this<BnwNativeGlobalColorRegistryBank> {
    friend class ::civ5::ui::BnwNativePackageDiscoveryOwner;
    friend class ::civ5::runtime::BnwSelectedStockLuaBootstrap;
    friend class BnwNativeGlobalResourceWarmOwner;
    friend class BnwNativeGlobalResourceQuery;
    friend class BnwNativeResourceDiscoveryActor;
    using Bits=native_color_load_detail::ColorBits;
    //initial_finalized has NO writer in this prefix. Only a future genuine
    //Skin/462C20 scheduler may add that transition; it cannot be supplied.
    enum class Phase {cold,base_colors,base_sets,base_loaded,roster_bound,initial_finalized,rules_written,failed};
    struct XmlOccurrence {
        std::shared_ptr<const content::XmlNode> document;
        std::uint64_t ordinal;
        bool colors_loaded=false;
    };
    const std::thread::id thread_=std::this_thread::get_id();
    std::weak_ptr<const BnwNativePackageDiscoveryOwner> parent_;
    std::shared_ptr<const void> epoch_;
    std::atomic<bool> closed_{false},busy_{false};
    Phase phase_=Phase::cold;
    std::shared_ptr<BnwNativeColorSlotStorage> global_;
    std::vector<std::weak_ptr<const BnwNativeUiSkinPoststageInstance>> roster_;
    // Separate owned-Parser constructor domain. Only the authentic Parent
    // scheduler may attach this roster after real798CE0 construction. Merely
    // retaining these weak slots neither changes phase nor enables lookup.
    std::vector<std::weak_ptr<const BnwNativeOwnedUiSkinPoststageInstance>> owned_roster_;
    // Closed genuine issuer contract for the owned descriptor/Skin route.
    // Its terminal checks authenticate executed journals and SAME actual Bank;
    // neither a roster count nor a supplied bool can advance the bank phase.
    class OwnedSource {
        friend class ::civ5::ui::BnwNativePackageDiscoveryOwner;
        friend class BnwNativeGlobalResourceWarmOwner;
      protected:
        OwnedSource()=default;
      public:
        virtual ~OwnedSource()=default;
        OwnedSource(const OwnedSource&)=delete;
        virtual void require_same_bank_current(const BnwNativeGlobalColorRegistryBank&,
            const content::XmlNode*,std::uint64_t)const=0;
        // Actual selected base Color XML read/parser occurrence, distinct from
        // descriptor query Parser nodes and reauthenticated by currentness.
        virtual void with_actual_base_color_document(
            const std::function<void(const std::shared_ptr<const content::XmlNode>&)>&)const=0;
        // Distinct actual installed52D590 selected stream. Saved sources
        // cannot issue it; the installed source binds its real active read loan.
        virtual void with_actual_installed_base_color_input(const std::function<void(
            const std::shared_ptr<const BnwNativeOwnedSkinSelectedInput>&)>&)const {
            throw std::runtime_error("Actual installed base Color stream source absent");
        }
        virtual void with_actual_constructed_roster(
            const std::function<void(const std::vector<std::shared_ptr<const BnwNativeOwnedUiSkinPoststageInstance>>&)>&)const=0;
        // Requires successful actual660330/kernel return under SAME source,
        // roster/fence/epoch and known actual Boot/client selection input.
        // An empty genuine roster is allowed only after real462C20 global
        // execution; guessed match-all settings cannot issue this transition.
        virtual void require_initial_skin_effects_terminal(const BnwNativeGlobalColorRegistryBank&)const=0;
        virtual void invalidate_actual_operation()const noexcept=0;
    };
    std::shared_ptr<const OwnedSource> owned_source_;
    void require_owned_numeric_context()const;
    void attach_same_owned_source(std::shared_ptr<const OwnedSource>);
    void require_owned_current(const content::XmlNode* document=nullptr,std::uint64_t occurrence=0)const;
    void initialize_owned_field(const std::shared_ptr<BnwNativeColorSlotStorage>&);
    void load_owned_base();
    bool load_installed_base();
    void bind_same_owned_roster();
    void require_owned_instance_destination(const std::shared_ptr<const BnwNativeOwnedUiSkinPoststageInstance>&,unsigned)const;
    void begin_owned_style_loading(const std::shared_ptr<const BnwNativeOwnedUiSkinPoststageInstance>&,unsigned);
    bool write_owned_style_destination_834b80(const std::shared_ptr<const BnwNativeOwnedUiSkinPoststageInstance>&,unsigned,
        const std::shared_ptr<BnwNativeMountedStyleReader>&);
    bool try_load_owned_style_8348c0(const std::shared_ptr<const BnwNativeOwnedUiSkinPoststageInstance>&,unsigned,
        const std::shared_ptr<const BnwNativeOwnedSkinSelectedInput>&);
    std::optional<Bits> ordered_owned_lookup(std::uint32_t)const;
    //Stored values under the actual fresh completed roster observations only.
    //Legacy/public lookup and initial constructor currentness stay separate.
    std::optional<Bits> ordered_completed_lookup(std::uint32_t)const;
    void reconcile_owned_colors_body_462c20();
    void reconcile_owned_colors_462c20();
    std::uint64_t owned_selected_color_serial_=1; //actual base load is occurrence1
    bool try_load_owned_color_601ca0(const std::shared_ptr<const BnwNativeOwnedUiSkinPoststageInstance>&,unsigned,
        const std::shared_ptr<const BnwNativeOwnedSkinSelectedInput>&);
    void finalize_owned_initial_skin_colors();

    std::vector<XmlOccurrence> sources_;
    // SAME early Package registry, never a second lazy cache.
    std::shared_ptr<BnwNativeSkinDocumentRegistration> resource_catalog_;
    std::weak_ptr<BnwNativeGlobalResourceWarmOwner> live_resource_query_; // SAME pre-Category owner, never replacement
    std::optional<runtime::BnwStockLuaAttemptLease> operation_;
    std::weak_ptr<const SelectedBnwPreviewHost> selected_;
    std::weak_ptr<const BnwCivilopediaNativeWindowBinding> window_;
    std::vector<BnwNativeGlobalColor> rules_rows_; //Pinned observations, no Host/Package ownership cycle.
    std::shared_ptr<const runtime::LiveSessionState> rules_world_;
    std::uintptr_t window_occurrence_=0;
    explicit BnwNativeGlobalColorRegistryBank(
        std::weak_ptr<const BnwNativePackageDiscoveryOwner>,std::shared_ptr<const void>,
        std::shared_ptr<BnwNativeSkinDocumentRegistration>);
    void require_current(const content::XmlNode* document=nullptr,std::uint64_t occurrence=0)const;
    void poison()noexcept;
    struct Scope {
        std::shared_ptr<BnwNativeGlobalColorRegistryBank> bank;
        explicit Scope(std::shared_ptr<BnwNativeGlobalColorRegistryBank>);
        ~Scope();
        Scope(const Scope&)=delete;
    };
    //Private authentic operation loan for the complete owned660330 kernel.
    //Original Scope still refuses nested/public reentry. Only this real stack
    //owner lets private owned calls borrow its SAME already active Scope.
    struct OwnedSkinOperation {
        Scope scope;
        explicit OwnedSkinOperation(std::shared_ptr<BnwNativeGlobalColorRegistryBank>);
        ~OwnedSkinOperation();
        OwnedSkinOperation(const OwnedSkinOperation&)=delete;
        OwnedSkinOperation&operator=(const OwnedSkinOperation&)=delete;
    };
    const OwnedSkinOperation* active_owned_skin_operation_=nullptr;
    struct PackagePin;
    // A newly issued completed Header/Loaded loan owns the original Scope.
    // It never revives retired Skin operation or bypasses public reentry.
    struct CompletedResourceOperation {
        Scope scope;
        std::shared_ptr<const BnwNativeCompletedResourceBankCapability> owner;
        std::unique_ptr<PackagePin> parent_pin;
        const std::thread::id thread=std::this_thread::get_id();
        explicit CompletedResourceOperation(std::shared_ptr<BnwNativeGlobalColorRegistryBank>,
            std::shared_ptr<const BnwNativeCompletedResourceBankCapability>);
        ~CompletedResourceOperation();
        CompletedResourceOperation(const CompletedResourceOperation&)=delete;
        CompletedResourceOperation&operator=(const CompletedResourceOperation&)=delete;
    };
    const CompletedResourceOperation* active_completed_resource_operation_=nullptr;
    struct CompletedCallScope {
        std::shared_ptr<BnwNativeGlobalColorRegistryBank> bank;
        const CompletedResourceOperation* borrowed=nullptr;
        explicit CompletedCallScope(std::shared_ptr<BnwNativeGlobalColorRegistryBank>);
        CompletedCallScope(const CompletedCallScope&)=delete;
        CompletedCallScope&operator=(const CompletedCallScope&)=delete;
    };
    void require_completed_operation_loan_current(const content::XmlNode* document=nullptr,
        std::uint64_t occurrence=0)const;
    struct OwnedCallScope {
        std::shared_ptr<BnwNativeGlobalColorRegistryBank> bank;
        std::unique_ptr<Scope> issued;
        const OwnedSkinOperation* borrowed=nullptr;
        explicit OwnedCallScope(std::shared_ptr<BnwNativeGlobalColorRegistryBank>);
        OwnedCallScope(const OwnedCallScope&)=delete;
        OwnedCallScope&operator=(const OwnedCallScope&)=delete;
    };
    void require_owned_operation_loan_current()const;
    void initialize_field(const std::shared_ptr<BnwNativeColorSlotStorage>&);
    void load_base(std::shared_ptr<const content::XmlNode>);
    void bind_same_roster(const std::vector<std::shared_ptr<const BnwNativeUiSkinPoststageInstance>>&);
    using ResourceManager=BnwNativeSkinDocumentRegistration::DocumentHandle;
    struct ResourceStage;
    mutable long internal_package_pins_=0;
    struct PackagePin {
        const BnwNativeGlobalColorRegistryBank& bank;
        std::shared_ptr<const BnwNativePackageDiscoveryOwner> parent;
        explicit PackagePin(const BnwNativeGlobalColorRegistryBank&);
        ~PackagePin();
        PackagePin(const PackagePin&)=delete;
    };
    struct MountSource {
        enum class Origin {directory_file,directory_fpk,file_query,paks_preference};
        const Origin origin;
        const std::size_t query_ordinal;
        const std::string original_query,selected_relative;
        const std::filesystem::path path;
        const runtime::FileIdentity pin;
        const std::uint64_t mtime;
        const std::optional<std::uint32_t> loose_prefix_index;
        std::optional<std::uint32_t> archive_prefix_index; //Bank-owned83C9D0 publication only.
        std::weak_ptr<ResourceStage> stage;
        MountSource(Origin o,std::size_t ordinal,std::string query,std::string selected,
            std::filesystem::path p,runtime::FileIdentity id,std::uint64_t time,std::optional<std::uint32_t> prefix)
            :origin(o),query_ordinal(ordinal),original_query(std::move(query)),
             selected_relative(std::move(selected)),path(std::move(p)),pin(std::move(id)),
             mtime(time),loose_prefix_index(prefix){}
    };
    struct ResourceRecord {
        std::uint32_t length=0,offset=0;
        std::shared_ptr<const MountSource> source;
        std::shared_ptr<BnwNativeFpkMountedCatalog> archive;
        std::shared_ptr<const BnwNativeFpkMountedCatalog::Entry> entry;
    };
    struct ResourceStage {
        ResourceManager manager;
        std::weak_ptr<const BnwNativeUiSkinPoststageInstance> instance;
        std::shared_ptr<const content::XmlNode> query_source;
        const std::size_t roster_index;
        const unsigned slot;
        const std::array<std::vector<std::string>,3> raw_declarations;
        const std::vector<std::string> directories,paks;
        std::shared_ptr<const std::vector<std::string>> files; //Bank issues once AFTER Directory effects.
        enum class ProducerPhase {queries_captured,paks_directories_finished,file_query_captured,files_finished};
        ProducerPhase producer_phase=ProducerPhase::queries_captured;
        std::map<std::string,std::uint32_t> prefixes;
        std::optional<std::uint32_t> actual_texture_quality;
        std::vector<std::shared_ptr<MountSource>> sources;
        std::map<std::uint32_t,ResourceRecord> records;
        NativeResourceBucketOrder record_order; // SAME admissions/removal/reinsertion, not sorted CRCs
        std::map<std::string,std::shared_ptr<BnwNativeFpkMountedCatalog>> archive_cache38;
        bool failed=false,prefix_executed=false;
        ResourceStage(ResourceManager m,std::shared_ptr<const BnwNativeUiSkinPoststageInstance> i,
            std::shared_ptr<const content::XmlNode> q,std::size_t n,unsigned s,
            std::array<std::vector<std::string>,3> raw,std::array<std::vector<std::string>,3> outputs)
            :manager(std::move(m)),instance(std::move(i)),query_source(std::move(q)),roster_index(n),slot(s),
             raw_declarations(std::move(raw)),directories(std::move(outputs[0])),
             paks(std::move(outputs[1])){}
    };
    //821690 direct82A050 global Default role; NOT a cache/autoregistration.
    //Borrow the actual published Primary/Common index, never clone records.
    std::weak_ptr<const BnwNativeCurrentDefaultSource> query_default_source_;
    std::weak_ptr<const void> query_default_state_;
    const NativeResourceRecordIndex* query_default_records_=nullptr;
    struct GlobalResourceSelection {
        //Prefix borrow only: actual high-quality category and Skin managers
        //share the SAME registry. Low global/warm output and5EB290 remain
        //incomplete; these values cannot authorize full startup/winner/MISS.
        enum class Kind {no_query,prefix_no_match,prefix_stack,prefix_category,prefix_default};
        Kind kind=Kind::no_query;
        ResourceManager manager;
        std::shared_ptr<ResourceStage> stage;
        const ResourceRecord* stack_record=nullptr;
        std::shared_ptr<const ::civ5::ui::BnwNativePackageCategorySourceOwner> category_owner;
        const void* category_entry=nullptr;
        const assets::AssetHandle* category_handle=nullptr;
        //SAME private live Default map entry, retained only during Scope.
        std::shared_ptr<const void> default_state;
        const assets::AssetHandle* default_handle=nullptr;
        const BnwNativeArchiveRecordDiagnostic* default_native_record=nullptr;
        const assets::AssetHandle* default_native_handle=nullptr;
        const void* default_entry=nullptr; //UnreadableLoose remains a real hit.
        std::uint32_t key=0;
    };
    std::shared_ptr<BnwNativeResourceDiscoveryActor> discovery_actor_;
    struct DiscoveryDefaultRow {
        std::uint32_t key;
        const void* actual_record;
        const std::string* actual_first;
        std::string filename;
    };
    std::vector<DiscoveryDefaultRow> borrow_default_discovery_rows()const;
    std::vector<ResourceManager> borrow_discovery_manager_order()const;
    void require_discovery_manager_order_current(const std::vector<ResourceManager>&)const;
    std::vector<DiscoveryDefaultRow> borrow_manager_discovery_rows(const ResourceManager&)const;
    void require_manager_discovery_row_current(const ResourceManager&,const DiscoveryDefaultRow&)const;
    void require_default_discovery_row_current(const DiscoveryDefaultRow&)const;
    void require_discovery_directory_current(const ResourceManager&,std::size_t,const std::string&)const;
    // The genuine private Package-load scheduler must consume the actor's
    // Batch; no public global-completion or filename API is introduced.
    void require_global_default_query_current()const;
    GlobalResourceSelection select_global_resource_record(const char*);
    void require_resource_stage_current(const std::shared_ptr<ResourceStage>&)const;
    std::shared_ptr<ResourceStage> retained_skin_resource_stage(std::size_t,unsigned)const;
    void require_mount_source_current(const std::shared_ptr<const MountSource>&,
        const std::filesystem::path&,const runtime::FileIdentity&)const;
    std::uint32_t resource_prefix_index(ResourceStage&,const std::string&);
    bool insert_resource_record(ResourceStage&,std::string,const ResourceRecord&,unsigned);
    void issue_selected_resource_file(const std::shared_ptr<ResourceStage>&,MountSource::Origin,
        std::size_t,const std::string&,const std::string&,unsigned,bool);
    void publish_same_mount_records(const std::shared_ptr<ResourceStage>&,
        const std::shared_ptr<MountSource>&,const std::shared_ptr<BnwNativeFpkMountedCatalog>&);
    void execute_slot_resource_prefix(std::size_t,unsigned);
    void require_skin_query_current(const content::XmlNode*,std::uint64_t,std::uint32_t)const;
    //419930/822A50 first getter: callable only by the future private source
    //load scheduler. No current Style/Directory/Paks caller is implemented.
    ResourceManager ensure_slot_resource_manager(std::size_t,unsigned);
    // Only the authentic SourceOwner's loan has a private selected-host link.
    // The bank is recovered from that host's uniquely consumed EARLY Package.
    static std::shared_ptr<BnwNativeGlobalColorRegistryBank> borrow_from_actual_stock(
        const runtime::BnwStockLuaAttemptLease&,
        std::shared_ptr<const BnwCivilopediaNativeWindowBinding>);
    void overwrite_rules_from_actual_stock(const runtime::BnwStockLuaAttemptLease&,
        std::shared_ptr<const SelectedBnwPreviewHost>,
        std::shared_ptr<const BnwCivilopediaNativeWindowBinding>);
    // Internal normal5307B0 source-order read. Only the current base ColorSet
    // load can reach a genuine cold-prefix miss; future Skin/final writers must
    // acquire their own actual phase before this method can be expanded.
    std::optional<Bits> normal_lookup_for_base(std::uint32_t)const;
    std::optional<Bits> ordered_lookup(std::uint32_t)const;
    std::optional<Bits> lookup_rgba(std::uint32_t)const;
public:
    ~BnwNativeGlobalColorRegistryBank();
    BnwNativeGlobalColorRegistryBank(const BnwNativeGlobalColorRegistryBank&)=delete;
};
static_assert(!std::is_default_constructible_v<BnwNativeGlobalColorRegistryBank>);
static_assert(!std::is_default_constructible_v<BnwNativeColorSlotStorage>);
}
}
