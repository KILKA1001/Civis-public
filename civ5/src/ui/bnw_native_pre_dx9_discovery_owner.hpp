#pragma once
#include "bnw_native_package_fstring_field0.hpp"
#include <memory>
#include <cstddef>
#include <filesystem>
namespace civ5::runtime {class BnwSelectedLanguageInputOwner;}
namespace civ5::ui {
class BnwNativePackageDiscoveryOwner;class BnwNativeResourceBootSettingsOwner;
namespace detail {
class BnwNativeResourceDiscoveryActor;class BnwNativePackageLoadActor;class BnwNativeStandalonePackageLoaded;
class BnwNativeGlobalResourceWarmOwner;class BnwNativeCompletedColdSourceLoan;class BnwNativeColdFilesystemOwner;
class BnwNativeInitialDescriptorSource;
class BnwNativePreDx9DiscoveryOwner final {
    friend class BnwNativeInitialDescriptorSource;
    friend class ::civ5::ui::BnwNativePackageDiscoveryOwner;
    friend class ::civ5::runtime::BnwSelectedLanguageInputOwner;
    friend class BnwNativeResourceDiscoveryActor;friend class BnwNativePackageLoadActor;
    friend class BnwNativeStandalonePackageLoaded;friend class BnwNativeGlobalResourceWarmOwner;
    struct Impl;const std::unique_ptr<Impl> impl_;
    explicit BnwNativePreDx9DiscoveryOwner(std::unique_ptr<Impl>);
    struct ParentPin;class DescriptorStreamSource;
    void require_current()const;void fail()const noexcept;
    void require_source_identity(const void*,std::size_t)const;
    void require_initial_default_miss(const BnwNativePackageLoadActor&)const;
    void require_descriptor_source_identity(const BnwNativePackageLoadActor&,const void*)const;
    void require_completed_cold(const BnwNativeCompletedColdSourceLoan&,const BnwNativeColdFilesystemOwner&)const;
    void require_core_platform_capture()const;void require_metadata_language_observation()const;
    std::filesystem::path game_root_for_language_source()const;
    std::shared_ptr<const runtime::BnwSelectedLanguageInputOwner> borrow_core_platform_input(const BnwNativePackageLoadActor&);
    void require_same_metadata_input(const std::shared_ptr<const runtime::BnwSelectedLanguageInputOwner>&)const;
    void require_actual_boot_consumer(const ::civ5::ui::BnwNativeResourceBootSettingsOwner&)const;
    void require_returned_package_current(const BnwNativeStandalonePackageLoaded&)const;
    void require_adopted_factory_identity_without_callbacks(const BnwNativeGlobalResourceWarmOwner&)const;
    void require_adopted_warm_identity_without_callbacks(const BnwNativeGlobalResourceWarmOwner&,
        const std::weak_ptr<const BnwNativePackageLoadActor>&)const;
    void require_initial_descriptor_identity_without_callbacks(const BnwNativePackageLoadActor&,
        const BnwNativeStandalonePackageLoaded&)const;
    std::filesystem::path actual_initial_descriptor_path(const BnwNativePackageLoadActor&)const;
    void initialize_actual_category2_pair(const std::shared_ptr<BnwNativePackageLoadActor>&,
        const BnwNativeResourceDiscoveryFilename::Source::ConstructionOperation&);
    void join_actual_initial_prefix();
    void drive_initial_receivers();void adopt_into_actual_common_warm(const std::shared_ptr<BnwNativeGlobalResourceWarmOwner>&);
public:
    ~BnwNativePreDx9DiscoveryOwner();
    BnwNativePreDx9DiscoveryOwner(const BnwNativePreDx9DiscoveryOwner&)=delete;
    BnwNativePreDx9DiscoveryOwner&operator=(const BnwNativePreDx9DiscoveryOwner&)=delete;
};
}}
