#pragma once
#include "civ5/content/manifest.hpp"
#include "civ5/runtime/file_identity.hpp"
#include <memory>
#include <string>
namespace civ5::ui {class BnwNativePackageDiscoveryOwner;}
namespace civ5::ui::native_ui_skin_text_detail {struct LexicalDocument;}
namespace civ5::ui::detail {
class BnwNativePreDx9DiscoveryOwner;class BnwNativePackageLoadActor;
class BnwNativeStandalonePackageLoaded;class BnwNativePackageCategory2Pair;
class BnwNativeUiSkinSelectedSourceOwner;
// Closed metadata adapter over the SAME actual Parsed/Core/metadata objects.
// The content tree is a derived compatibility view, not a second XML parse or
// a package/namespace capability. Only the real initial issuer can create it.
class BnwNativeInitialDescriptorSource final {
    friend class ::civ5::ui::BnwNativePackageDiscoveryOwner;
    friend class BnwNativePreDx9DiscoveryOwner;
    friend class BnwNativeUiSkinSelectedSourceOwner;
    struct Impl;const std::unique_ptr<Impl> impl_;
    explicit BnwNativeInitialDescriptorSource(std::unique_ptr<Impl>);
    static std::shared_ptr<const BnwNativeInitialDescriptorSource> capture(
        const std::shared_ptr<BnwNativePreDx9DiscoveryOwner>&,
        const std::shared_ptr<BnwNativePackageLoadActor>&,
        const std::shared_ptr<const BnwNativeStandalonePackageLoaded>&);
    void require_current()const;
    // Parent-only retained aliases: actual Scope/source/selected PREPOST, no nested metadata borrower.
    void require_retained_metadata_current()const;
    const content::PackageManifest& manifest_without_callbacks()const;
    const runtime::FileIdentity& file_without_callbacks()const;
    const std::string& original_bytes_without_callbacks()const;
    std::shared_ptr<BnwNativePackageCategory2Pair> actual_pair_without_callbacks()const;
    native_ui_skin_text_detail::LexicalDocument borrow_actual_native_values(const content::XmlNode&)const;
public:
    ~BnwNativeInitialDescriptorSource();
    BnwNativeInitialDescriptorSource(const BnwNativeInitialDescriptorSource&)=delete;
    BnwNativeInitialDescriptorSource&operator=(const BnwNativeInitialDescriptorSource&)=delete;
};
}
