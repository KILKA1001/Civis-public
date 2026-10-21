#pragma once
#include "civ5/ui/bnw_native_package_discovery_owner.hpp"
#include "civ5/ui/detail/bnw_native_resource_record_index.hpp"
#include "bnw_native_archive_basename_cache.hpp"
#include "bnw_native_default_namespace_policy.hpp"
#include <map>
#include <thread>
namespace civ5::ui { class BnwNativePackagePreSkinOwner; class BnwNativeInstallationInputOwner; }
namespace civ5::ui::detail {
// Pure storage formula, used by the privately issued lineage below. A caller
// may test this policy but cannot bind it to an authentic Package or namespace.
class BnwSharedFirstNamesStorageDiagnostic final {
    std::map<std::uint32_t,std::string> first_;
  public:
    void remember(std::uint32_t crc,std::string_view raw) { first_.try_emplace(crc,raw); }
    const auto& names() const noexcept { return first_; }
    static void require_extension(const std::map<std::uint32_t,std::string>& before,
                                  const std::map<std::uint32_t,std::string>& after) {
        for (const auto& [crc,name]:before) {
            const auto found=after.find(crc);
            if(found==after.end() || found->second!=name)
                throw std::runtime_error("Shared native FIRST projection overwrote/removed an earlier name");
        }
    }
    void adopt(std::map<std::uint32_t,std::string>& prepared) noexcept { first_.swap(prepared); }
};
// Pointer/control-block identity, not an integer/GUID/caller-provided epoch.
// This formula itself issues no owner. Tests use synthetic lifetimes only.
template<class Parent> std::shared_ptr<const Parent> bnw_shared_first_parent_diagnostic(
    const std::weak_ptr<const Parent>& issued,const Parent& expected) {
    auto parent=issued.lock();
    if(!parent || parent.get()!=&expected || parent.use_count()<=1)
        throw std::runtime_error("Shared native FIRST foreign/retired persistent parent");
    return parent;
}
class BnwNativeCategoryNamespaceChannel;
class BnwNativeGlobalResourceWarmOwner;
class BnwNativeStandalonePackageLoaded;
class BnwNativeArchiveEntryKernelDiagnostic;
class BnwNativeCategoryNamespaceLineage;
// No public factory, snapshot issuer, mutator or constructor. ONE object is
// retained at the actual fresh default table; all namespaces share its storage.
class BnwNativeSharedFirstLineage final {
  public:
    ~BnwNativeSharedFirstLineage()=default;
    BnwNativeSharedFirstLineage(const BnwNativeSharedFirstLineage&)=delete;
  private:
    friend class ::civ5::ui::BnwNativePackageDiscoveryOwner;
    friend class BnwNativeArchiveEntryAdmissionArena;
    friend class BnwNativeGlobalColorRegistryBank;
    friend class BnwNativeGlobalResourceWarmOwner;
    friend class BnwNativeArchiveEntryKernelDiagnostic;
    friend class ::civ5::ui::BnwNativePackageCategorySourceOwner;
    friend class BnwNativeCategoryNamespaceLineage;
    friend struct NativeResourceRecordIndex;
    std::weak_ptr<const ::civ5::ui::BnwNativePackageDiscoveryOwner> parent_;
    std::shared_ptr<const BnwNativeEarlyInputLease> lease_;
    std::shared_ptr<const ::civ5::ui::BnwNativeInstallationInputOwner> installation_input_;
    std::thread::id thread_=std::this_thread::get_id();
    BnwSharedFirstNamesStorageDiagnostic storage_;
    BnwNativeSharedFirstLineage(
        std::weak_ptr<const ::civ5::ui::BnwNativePackageDiscoveryOwner> parent,
        std::shared_ptr<const BnwNativeEarlyInputLease> lease):parent_(std::move(parent)),lease_(std::move(lease)){}
    BnwNativeSharedFirstLineage(
        std::weak_ptr<const ::civ5::ui::BnwNativePackageDiscoveryOwner> parent,
        std::shared_ptr<const ::civ5::ui::BnwNativeInstallationInputOwner> installation)
        :parent_(std::move(parent)),installation_input_(std::move(installation)){}
    void require_current() const;
    void remember(std::uint32_t crc,std::string_view raw);
    const std::map<std::uint32_t,std::string>& names() const;
    std::map<std::uint32_t,std::string> prepare_extension(
        const std::map<std::uint32_t,std::string>&) const;
    void adopt_prepared(std::map<std::uint32_t,std::string>& prepared) noexcept { storage_.adopt(prepared); }
};
// Constructor state only, not an enabled mount/positive archive load. The same
// parent issues this after genuine PreSkin flags and retained settings checks.
// Records and namespace+38 cache are separate; FIRST is the retained shared one.
class BnwNativeCategoryNamespaceLineage final {
  public:
    ~BnwNativeCategoryNamespaceLineage()=default;
    BnwNativeCategoryNamespaceLineage(const BnwNativeCategoryNamespaceLineage&)=delete;
  private:
    friend class ::civ5::ui::BnwNativePackageDiscoveryOwner;
    friend class ::civ5::ui::BnwNativePackageCategorySourceOwner;
    friend class BnwNativeCategoryNamespaceChannel;
    friend class BnwNativeGlobalResourceWarmOwner;
    std::weak_ptr<const ::civ5::ui::BnwNativePackageDiscoveryOwner> parent_;
    std::shared_ptr<const ::civ5::ui::BnwNativePackagePreSkinOwner> eligibility_;
    std::optional<std::weak_ptr<const BnwNativeGlobalResourceWarmOwner>> owned_warm_;
    std::weak_ptr<const BnwNativeStandalonePackageLoaded> owned_package_;
    std::shared_ptr<BnwNativeSharedFirstLineage> first_;
    std::size_t descriptor_;
    unsigned category_;
    //82A050 initialized manager+88/+89, independently of UISkin slot enabled.
    //This storage policy cannot issue the namespace; the private ctor above
    //and actual completed factory/load operations remain mandatory.
    const BnwDefaultNamespaceInitialPolicyDiagnostic constructor_policy_;
    NativeResourceRecordIndex records_;
    BnwArchiveBasenameCacheStoragePolicy<std::shared_ptr<const BnwNativeArchiveCachePublicationOwner>> cache_;
    BnwNativeCategoryNamespaceLineage(
        std::weak_ptr<const ::civ5::ui::BnwNativePackageDiscoveryOwner>,
        std::shared_ptr<const ::civ5::ui::BnwNativePackagePreSkinOwner>,
        std::shared_ptr<BnwNativeSharedFirstLineage>,std::size_t,unsigned);
    BnwNativeCategoryNamespaceLineage(
        std::weak_ptr<const ::civ5::ui::BnwNativePackageDiscoveryOwner>,
        std::weak_ptr<const BnwNativeGlobalResourceWarmOwner>,
        std::weak_ptr<const BnwNativeStandalonePackageLoaded>,
        std::shared_ptr<BnwNativeSharedFirstLineage>,std::size_t,unsigned);
    void require_current(const ::civ5::ui::BnwNativePackageDiscoveryOwner&) const;
};
}
