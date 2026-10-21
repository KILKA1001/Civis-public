#pragma once
#include <memory>
namespace civ5::ui {class BnwNativePackageDiscoveryOwner;}
namespace civ5::ui::detail {
class BnwNativeGlobalResourceWarmOwner;
// Closed installation-domain result of actual Common13 + three DefaultLoose
// operations and final publication. No saved lease or caller-ready input.
class BnwNativeInstalledDefaultCommonSource final {
    friend class ::civ5::ui::BnwNativePackageDiscoveryOwner;
    friend class BnwNativeGlobalResourceWarmOwner;
    struct Impl;const std::unique_ptr<Impl> impl_;
    struct PublicationOperation;
    explicit BnwNativeInstalledDefaultCommonSource(std::unique_ptr<Impl>);
    bool publication_returned_without_callbacks()const;
    bool driver_active_without_callbacks()const;
    bool loose_returned_without_callbacks()const;
    void fail()const noexcept;
    void require_current()const;
    void require_publication_current(const PublicationOperation&)const;
public:
    ~BnwNativeInstalledDefaultCommonSource();
    BnwNativeInstalledDefaultCommonSource(const BnwNativeInstalledDefaultCommonSource&)=delete;
    BnwNativeInstalledDefaultCommonSource&operator=(const BnwNativeInstalledDefaultCommonSource&)=delete;
};
}
