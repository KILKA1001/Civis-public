#pragma once
#include "civ5/ui/bnw_native_package_archive_time_owner.hpp"
#include "bnw_native_shared_first_lineage.hpp"
#include "bnw_native_ui_skin_raw_text.hpp"
#include "bnw_native_callback_owner_guard.hpp"
#include "bnw_native_loose_walk.hpp"
#include "bnw_native_category_directory_walk.hpp"
#include "civ5/ui/detail/bnw_native_original_stat_path.hpp"
#include <functional>
namespace civ5::ui {class BnwNativeInstallationInputOwner;}

namespace civ5::ui::native_category_source_detail {
// Pure lifetime predicate only. The production consumer supplies the outer
// reference through the private PreSkin-issued token below, never a count/flag.
template<class Parent>
bool completion_borrow_has_external_parent(const std::shared_ptr<const Parent>& inner,
    const std::shared_ptr<const Parent>& outer,
    const detail::BnwNativeCallbackOwnerGuard<Parent>& fence)noexcept {
    return std::addressof(inner)!=std::addressof(outer) && inner && outer &&
        inner.get()==outer.get() && !inner.owner_before(outer) && !outer.owner_before(inner) &&
        fence.valid() && inner.use_count()>3; // inner + fence + exact outer
}
inline std::filesystem::path source_path(const std::filesystem::path& root,const std::filesystem::path& relative) {
    const auto path=detail::confined_original_stat_path(root,relative);
    auto stop=root.lexically_normal();
    if(stop.has_relative_path() && stop.filename().empty())stop=stop.parent_path();
    for(auto parent=path;parent!=stop;parent=parent.parent_path()) {
        native_ui_skin_text_detail::need(!parent.empty(),"Category source parent escaped owned root");
        const auto attributes=GetFileAttributesA(native_loose_detail::acp_path(parent).c_str());
        if(attributes!=INVALID_FILE_ATTRIBUTES)
            native_ui_skin_text_detail::need(!(attributes&FILE_ATTRIBUTE_REPARSE_POINT),
                "Category source reparse path requires genuine target owner");
        else {
            const auto error=GetLastError();
            native_ui_skin_text_detail::need(error==ERROR_FILE_NOT_FOUND || error==ERROR_PATH_NOT_FOUND,
                "Category source parent probe has unknown OS error");
        }
    }
    return path;
}
// Pure parser kernel. This neither issues a Package nor accepts caller rows
// into a live namespace. Production invokes it only on retained root pointers
// and lexical runs produced from that SAME descriptor's pinned bytes.
inline std::vector<std::string> values_in_scope(const content::XmlNode& scope,
    const native_ui_skin_text_detail::LexicalDocument& lexical, std::string_view selector) {
    std::vector<std::string> result;
    for(const auto& leaf:scope.children) {
        if(leaf.name!=selector)continue;
        std::string scratch;
        const auto read=[&](const content::XmlNode& node) {
            const auto found=lexical.values.find(&node);
            native_ui_skin_text_detail::need(found!=lexical.values.end(),
                "Native category node lost SAME raw descriptor provenance");
            const auto& bytes=found->second.bytes;
            if(std::none_of(bytes.begin(),bytes.end(),[](unsigned char c){return c&0x80;}))scratch=bytes;
            result.push_back(scratch);
        };
        if(leaf.children.empty())read(leaf);
        else for(const auto& file:leaf.children)if(file.name=="File")read(file);
    }
    return result;
}
inline std::vector<std::string> category_values(const content::XmlNode& root,
    const native_ui_skin_text_detail::LexicalDocument& lexical,unsigned category,std::string_view selector) {
    native_ui_skin_text_detail::need(category<2,"Native source category0/1 required");
    std::vector<std::string> result;
    const auto name=category==0?"Common":"Gameplay";
    for(const auto& node:root.children)if(node.name==name){result=values_in_scope(node,lexical,selector);break;}
    // Native54B8D0 Common also reads package root AFTER the first Common node.
    if(category==0){auto tail=values_in_scope(root,lexical,selector);result.insert(result.end(),tail.begin(),tail.end());}
    return result;
}
}

namespace civ5::ui::detail {
class BnwNativeGlobalResourceWarmOwner;
class BnwNativeCategoryLooseAdmissionSource;
class BnwNativeCompletedColdSourceLoan;
class BnwNativeGlobalColorRegistryBank;
// Borrows exactly ONE actual PreSkin local weak-lock reference. No additional
// strong owner, count parameter, public issuer, copy or move is possible.
class BnwNativeCategoryCompletionParentPin final {
  public:
    BnwNativeCategoryCompletionParentPin(const BnwNativeCategoryCompletionParentPin&)=delete;
    BnwNativeCategoryCompletionParentPin(BnwNativeCategoryCompletionParentPin&&)=delete;
  private:
    friend class ::civ5::ui::BnwNativePackagePreSkinOwner;
    friend class ::civ5::ui::BnwNativePackageDiscoveryOwner;
    friend class ::civ5::ui::BnwNativePackageCategorySourceOwner;
    using Parent=::civ5::ui::BnwNativePackageDiscoveryOwner;
    const std::shared_ptr<const Parent>& parent_pin_;
    const ::civ5::ui::BnwNativePackagePreSkinOwner* predecessor_;
    const std::thread::id thread_=std::this_thread::get_id();
    BnwNativeCategoryCompletionParentPin(const std::shared_ptr<const Parent>& parent,
        const ::civ5::ui::BnwNativePackagePreSkinOwner& predecessor)noexcept
        :parent_pin_(parent),predecessor_(&predecessor){}
    bool has_external_parent(const std::shared_ptr<const Parent>& parent,
        const BnwNativeCallbackOwnerGuard<Parent>& fence,
        const ::civ5::ui::BnwNativePackagePreSkinOwner& predecessor)const noexcept {
        return thread_==std::this_thread::get_id() && predecessor_==&predecessor &&
            native_category_source_detail::completion_borrow_has_external_parent(parent,parent_pin_,fence);
    }
};
// Source constructor channel has no filename. Empty Paks must initialize the
// real namespace/arena/cache without inventing a request or an archive load.
class BnwNativeCategoryNamespaceChannel final {
  public:
    ~BnwNativeCategoryNamespaceChannel()=default;
    BnwNativeCategoryNamespaceChannel(const BnwNativeCategoryNamespaceChannel&)=delete;
    void require_current()const;
    void require_initialization()const;
    void require_same_input(const BnwNativeColdFilesystemOwner&)const;
  private:
    friend class ::civ5::ui::BnwNativePackageCategorySourceOwner;
    friend class ::civ5::ui::BnwNativePackageDiscoveryOwner;
    friend class BnwNativeGlobalResourceWarmOwner;
    friend class BnwNativeCategoryArchiveChannel;
    friend class BnwNativeArchiveEntryAdmissionArena;
    friend class BnwNativeArchiveBasenameCacheOwner;
    friend class BnwNativeArchiveV6EntryAdmissionOwner;
    friend class BnwNativeCategoryLooseAdmissionSource;
    std::weak_ptr<const ::civ5::ui::BnwNativePackageCategorySourceOwner> owner_;
    std::optional<std::weak_ptr<const BnwNativeGlobalResourceWarmOwner>> owned_warm_;
    std::shared_ptr<const BnwNativeColdFilesystemOwner> cold_;
    std::shared_ptr<const ::civ5::ui::BnwNativeEarlyInputLease> lease_;
    // Distinct actual initial installation source; never a saved lease alias.
    std::shared_ptr<const ::civ5::ui::BnwNativeInstallationInputOwner> installation_input_;
    std::shared_ptr<BnwNativeSharedFirstLineage> first_;
    std::shared_ptr<BnwNativeCategoryNamespaceLineage> namespace_;
    std::shared_ptr<const void> epoch_,brand_;
    BnwNativeCategoryNamespaceChannel(
        std::weak_ptr<const ::civ5::ui::BnwNativePackageCategorySourceOwner>,
        std::shared_ptr<const BnwNativeColdFilesystemOwner>,
        std::shared_ptr<const ::civ5::ui::BnwNativeEarlyInputLease>,
        std::shared_ptr<BnwNativeSharedFirstLineage>,
        std::shared_ptr<BnwNativeCategoryNamespaceLineage>,std::shared_ptr<const void>);
    BnwNativeCategoryNamespaceChannel(
        std::weak_ptr<const BnwNativeGlobalResourceWarmOwner>,
        std::shared_ptr<const BnwNativeColdFilesystemOwner>,
        std::shared_ptr<const ::civ5::ui::BnwNativeEarlyInputLease>,
        std::shared_ptr<BnwNativeSharedFirstLineage>,
        std::shared_ptr<BnwNativeCategoryNamespaceLineage>,std::shared_ptr<const void>);
    const std::shared_ptr<BnwNativeSharedFirstLineage>& shared_first()const;
    BnwNativeCategoryNamespaceChannel(
        std::weak_ptr<const BnwNativeGlobalResourceWarmOwner>,
        std::shared_ptr<const BnwNativeColdFilesystemOwner>,
        std::shared_ptr<const ::civ5::ui::BnwNativeInstallationInputOwner>,
        std::shared_ptr<BnwNativeSharedFirstLineage>,
        std::shared_ptr<BnwNativeCategoryNamespaceLineage>,std::shared_ptr<const void>);
    const void* namespace_identity()const;
    NativeResourceRecordIndex& record_index()const;
    void require_current_completed(const BnwNativeCompletedColdSourceLoan&,const BnwNativeColdFilesystemOwner&)const;
    const NativeResourceRecordIndex& record_index_completed(
        const BnwNativeCompletedColdSourceLoan&,const BnwNativeColdFilesystemOwner&)const;
    BnwArchiveBasenameCacheStoragePolicy<std::shared_ptr<const BnwNativeArchiveCachePublicationOwner>>& cache_storage()const;
    const std::shared_ptr<const BnwNativeColdFilesystemOwner>& cold_owner()const;
    void claim_initial_arena()const;
    mutable bool arena_claimed_=false;
};
// Issued by ONE real category source operation. Readonly getters do not create
// a request; ColdFilesystem additionally checks the active request phase.
class BnwNativeCategoryArchiveChannel final {
  public:
    ~BnwNativeCategoryArchiveChannel()=default;
    BnwNativeCategoryArchiveChannel(const BnwNativeCategoryArchiveChannel&)=delete;
    void require_current()const;
    void require_active_request()const;
    void require_same_input(const BnwNativeColdFilesystemOwner&)const;
    std::size_t source_ordinal()const;
    const std::string& raw_relative_ansi_request()const;
    const std::u16string& raw_utf16_stream_name_candidate()const;
    const std::filesystem::path& original_relative_io_token()const;
    const std::filesystem::path& original_absolute_io_candidate()const;
    const std::filesystem::path& canonical_payload_io_candidate()const;
  private:
    friend class ::civ5::ui::BnwNativePackageCategorySourceOwner;
    friend class ::civ5::ui::BnwNativePackageDiscoveryOwner;
    friend class BnwNativeGlobalResourceWarmOwner;
    friend class BnwNativeColdFilesystemOwner;
    friend class BnwNativeArchiveEntryAdmissionArena;
    friend class BnwNativeArchiveBasenameCacheOwner;
    friend class BnwNativeArchiveV6EntryAdmissionOwner;
    std::weak_ptr<const ::civ5::ui::BnwNativePackageCategorySourceOwner> owner_;
    std::optional<std::weak_ptr<const BnwNativeGlobalResourceWarmOwner>> owned_warm_;
    std::shared_ptr<const BnwNativeColdFilesystemOwner> cold_;
    std::shared_ptr<const ::civ5::ui::BnwNativeEarlyInputLease> lease_;
    std::shared_ptr<const ::civ5::ui::BnwNativeInstallationInputOwner> installation_input_;
    std::shared_ptr<BnwNativeSharedFirstLineage> first_;
    std::shared_ptr<BnwNativeCategoryNamespaceLineage> namespace_;
    std::shared_ptr<const BnwNativeCategoryNamespaceChannel> namespace_channel_;
    std::shared_ptr<const void> epoch_,brand_;
    std::size_t ordinal_;
    std::string raw_;
    std::u16string token_;
    std::filesystem::path relative_,original_,canonical_;
    BnwNativeCategoryArchiveChannel(
        std::weak_ptr<const ::civ5::ui::BnwNativePackageCategorySourceOwner>,
        std::shared_ptr<const BnwNativeColdFilesystemOwner>,
        std::shared_ptr<const ::civ5::ui::BnwNativeEarlyInputLease>,
        std::shared_ptr<BnwNativeSharedFirstLineage>,
        std::shared_ptr<BnwNativeCategoryNamespaceLineage>,
        std::shared_ptr<const BnwNativeCategoryNamespaceChannel>,
        std::shared_ptr<const void>,std::size_t,std::string);
    BnwNativeCategoryArchiveChannel(
        std::weak_ptr<const BnwNativeGlobalResourceWarmOwner>,
        std::shared_ptr<const BnwNativeColdFilesystemOwner>,
        std::shared_ptr<const ::civ5::ui::BnwNativeEarlyInputLease>,
        std::shared_ptr<BnwNativeSharedFirstLineage>,
        std::shared_ptr<BnwNativeCategoryNamespaceLineage>,
        std::shared_ptr<const BnwNativeCategoryNamespaceChannel>,
        std::shared_ptr<const void>,std::size_t,std::string);
    BnwNativeCategoryArchiveChannel(
        std::weak_ptr<const BnwNativeGlobalResourceWarmOwner>,
        std::shared_ptr<const BnwNativeColdFilesystemOwner>,
        std::shared_ptr<const ::civ5::ui::BnwNativeInstallationInputOwner>,
        std::shared_ptr<BnwNativeSharedFirstLineage>,
        std::shared_ptr<BnwNativeCategoryNamespaceLineage>,
        std::shared_ptr<const BnwNativeCategoryNamespaceChannel>,
        std::shared_ptr<const void>,std::size_t,std::string);
    const std::shared_ptr<BnwNativeSharedFirstLineage>& shared_first()const;
    const void* namespace_identity()const;
    const std::shared_ptr<const BnwNativeCategoryNamespaceChannel>& namespace_channel()const;
};
}
namespace civ5::ui {
class BnwNativePackageCategorySourceOwner final {
  public:
    ~BnwNativePackageCategorySourceOwner()=default;
    BnwNativePackageCategorySourceOwner(const BnwNativePackageCategorySourceOwner&)=delete;
  private:
    friend class BnwNativePackageDiscoveryOwner;
    friend class BnwNativePackageArchiveTimeOwner;
    friend class BnwNativePackagePreSkinOwner;
    friend class detail::BnwNativeCategoryArchiveChannel;
    friend class detail::BnwNativeGlobalColorRegistryBank;
    friend class detail::BnwNativeCategoryNamespaceChannel;
    using Parent=BnwNativePackageDiscoveryOwner;
    using Namespace=detail::BnwNativeCategoryNamespaceLineage;
    using Channel=detail::BnwNativeCategoryArchiveChannel;
    using Incoming=std::variant<assets::AssetHandle,detail::UnreadableLoose>;
    using Guard=detail::BnwNativeCallbackOwnerGuard<BnwNativePackageCategorySourceOwner>;
    struct ReadFailure {
        const BnwNativePackageCategorySourceOwner& owner;
        bool complete=false;
        ~ReadFailure(){if(!complete)owner.failed_=true;}
    };
    struct FileWitness {
        std::filesystem::path relative;
        Incoming incoming;
        BnwNativeStat64Receipt original_stat;
        runtime::FileIdentity original_bytes,canonical_bytes;
    };
    struct ArchiveRecordWitness {
        std::shared_ptr<const detail::BnwNativeGlobalArchivePrefixRecord> prefix;
        std::shared_ptr<const detail::BnwNativeArchiveCachePublicationOwner> cache;
        std::size_t ordinal;
        assets::AssetHandle handle;
        bool preferred=true;
    };
    struct Stage {
        std::shared_ptr<Namespace> source;
        // Native Package category field is nonowning; the registry retains
        // the manager. Weak reverse edge avoids Source -> Doc -> Source cycles.
        std::weak_ptr<void> warm_manager;
        bool warm_manager_attempted=false,warm_factory_active=false;
        std::shared_ptr<const detail::BnwNativeCategoryNamespaceChannel> namespace_channel;
        std::shared_ptr<const detail::BnwNativeArchiveEntryAdmissionArena> arena;
        std::shared_ptr<const detail::BnwNativeArchiveBasenameCacheOwner> cache;
        const content::XmlNode* retained_root=nullptr;
        const std::string* retained_bytes=nullptr;
        native_ui_skin_text_detail::LexicalDocument lexical;
        std::vector<std::string> paks,directories,map_directories,files,directory_exclusions;
        std::vector<std::shared_ptr<const Channel>> attempts;
        std::map<std::uint32_t,ArchiveRecordWitness> archived_selected;
        std::vector<FileWitness> loose;
        std::vector<native_category_directory_detail::DirectoryWalk> walks;
        std::vector<std::filesystem::path> absent;
        std::string name;
        bool operations_complete=false,published=false;
    };
    struct Mode7Borrow {
        const BnwNativePackageCategorySourceOwner& owner;
        Stage& stage;
        FileWitness& file;
        void require_current(const detail::NativeResourceRecordIndex&,const detail::BnwNativeSharedFirstLineage&)const;
        BnwNativeStat64Receipt original_loose_stat(const Incoming&)const;
        struct Cached {bool source_cache_present;std::optional<std::int64_t> original_cached_mtime;};
        Cached cached_archive_mtime(const assets::AssetHandle&)const;
    };
    struct Lookup {
        std::weak_ptr<const BnwNativePackageCategorySourceOwner> owner;
        std::shared_ptr<const Namespace> source;
        assets::ResourceMountHit operator()(const assets::AssetKey&)const;
    };
    struct Keys {
        std::weak_ptr<const BnwNativePackageCategorySourceOwner> owner;
        std::shared_ptr<const Namespace> source;
        std::vector<std::string> operator()(std::string_view)const;
    };
    std::weak_ptr<const Parent> parent_;
    std::weak_ptr<const BnwNativePackageCategorySourceOwner> self_;
    std::shared_ptr<const BnwNativePackagePreSkinOwner> predecessor_;
    std::shared_ptr<const BnwNativePackageArchiveTimeOwner> archives_;
    std::shared_ptr<const BnwNativeEarlyInputLease> lease_;
    std::shared_ptr<const detail::BnwNativeColdFilesystemOwner> cold_;
    std::shared_ptr<const detail::BnwNativeGlobalArchivePrefixOwner> prefixes_;
    std::shared_ptr<const void> epoch_,channel_;
    std::vector<Stage> stages_;
    mutable detail::BnwNativeCallbackOwnerState state_;
    mutable bool failed_=false;
    mutable const Guard* active_fence_=nullptr;
    const Stage* active_operation_stage_=nullptr; //Only actual producer's stack journal.
    const std::thread::id native_query_thread_=std::this_thread::get_id();
    mutable std::size_t native_query_parent_pins_=0;
    mutable bool native_record_borrow_active_=false;
    struct NativeQueryParentPin {
        const BnwNativePackageCategorySourceOwner& source;
        const std::shared_ptr<const Parent> parent;
        explicit NativeQueryParentPin(const BnwNativePackageCategorySourceOwner& actual)
            :source(actual),parent(actual.parent_.lock()) {
            //Immutable thread identity is checked BEFORE touching any source
            //counter/flag. Foreign-thread calls cannot race its size_t journal.
            if(source.native_query_thread_!=std::this_thread::get_id())
                throw std::runtime_error("Category actual Parent pin foreign thread");
            if(!parent)throw std::runtime_error("Category actual Package expired");
            ++source.native_query_parent_pins_;
        }
        ~NativeQueryParentPin(){--source.native_query_parent_pins_;}
        NativeQueryParentPin(const NativeQueryParentPin&)=delete;
        NativeQueryParentPin(NativeQueryParentPin&&)=delete;
        NativeQueryParentPin&operator=(const NativeQueryParentPin&)=delete;
        NativeQueryParentPin&operator=(NativeQueryParentPin&&)=delete;
    };
    const Channel* active_request_=nullptr;
    const detail::BnwNativeCategoryNamespaceChannel* initializing_namespace_=nullptr;
    bool sealed_=false;
    BnwNativePackageCategorySourceOwner(std::weak_ptr<const Parent>,
        std::shared_ptr<const BnwNativePackagePreSkinOwner>,
        std::shared_ptr<const BnwNativePackageArchiveTimeOwner>,
        std::shared_ptr<const BnwNativeEarlyInputLease>,std::shared_ptr<const void>);
    struct WarmRecordBorrow {
        const void* entry=nullptr;
        const assets::AssetHandle* handle=nullptr;
        bool preferred=true;
    };
    WarmRecordBorrow borrow_warm_manager_record(const void* actual_manager,std::uint32_t key)const;
    void with_native_warm_manager_record(const void* actual_manager,std::uint32_t key,
        const std::function<void(const detail::BnwNativeArchiveRecordDiagnostic*,const std::u16string*,const std::string*)>&)const;
    struct WarmDiscoveryRow {std::uint32_t key;const void* entry;};
    std::vector<WarmDiscoveryRow> borrow_warm_manager_rows(const void* actual_manager)const;
    void require_current()const;
    void require_from_parent_without_callbacks(const Parent&)const;
    void require_archive_channel_current(const Channel&,bool active)const;
    void require_namespace_channel_current(const detail::BnwNativeCategoryNamespaceChannel&,bool initializing)const;
    bool load_archive(Stage&,const std::string&);
    void add_loose(Stage&,const std::filesystem::path&,const native_loose_detail::LooseEntry&);
    void scan_directory(Stage&,const std::filesystem::path&);
    void require_complete_for(const BnwNativePackagePreSkinOwner&,
        const detail::BnwNativeCategoryCompletionParentPin* = nullptr)const;
};
}
