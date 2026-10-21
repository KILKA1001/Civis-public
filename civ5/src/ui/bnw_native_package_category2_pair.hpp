#pragma once
#include "civ5/runtime/file_identity.hpp"
#include <array>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <optional>
#include <stdexcept>
#include <thread>
#include <type_traits>

namespace civ5::content { struct XmlNode; }
namespace civ5::ui {
class BnwNativeEarlyInputLease;
class BnwNativeInstallationInputOwner;
class BnwNativePackageDiscoveryOwner;
class BnwNativePackageCategorySourceOwner;
class BnwNativePackagePreSkinOwner;
namespace detail {
class BnwNativePreDx9DiscoveryOwner;class BnwNativeInitialDescriptorSource;class BnwNativePackageLoadActor;
class BnwNativeUiSkinRetainedOccurrence;
class BnwNativePackageCategory2PairOperation;
// Exact native6F7D30 constructor fields+60/+64 only. This data predicate issues
// no Package/read/Source capability. Nonzero fields are not modeled as nullptr.
namespace native_package_category2_pair_detail {
constexpr bool constructor_zero(const std::array<std::uint32_t,2>& words)noexcept {
    return words[0]==0 && words[1]==0;
}
}
class BnwNativePackageCategory2Pair final {
  public:
    ~BnwNativePackageCategory2Pair()=default;
    BnwNativePackageCategory2Pair(const BnwNativePackageCategory2Pair&)=delete;
  private:
    friend class ::civ5::ui::BnwNativePackageDiscoveryOwner;
    friend class BnwNativePreDx9DiscoveryOwner;
    friend class BnwNativeInitialDescriptorSource;
    // Pair fields are DWORD pointer storage, not a guessed namespace type.
    // Actual Source/enabled89/registered-record writers remain unsupported.
    std::array<std::uint32_t,2> native_words_{0,0};
    bool constructor_tracked_=true,retired_=false;
    std::uint64_t revision_=0;
    const std::thread::id thread_=std::this_thread::get_id();
    std::weak_ptr<const ::civ5::ui::BnwNativePackageDiscoveryOwner> parent_;
    std::shared_ptr<const BnwNativeEarlyInputLease> lease_;
    std::shared_ptr<const BnwNativeInstallationInputOwner> installation_input_;
    std::shared_ptr<const void> epoch_;
    const content::XmlNode* document_=nullptr;
    std::size_t descriptor_=0;
    // Exact existing creation operand/file witness. Do NOT treat this wide
    // filesystem path as native Package+0 FString bytes or canonical payload.
    const std::filesystem::path creation_filename_;
    const runtime::FileIdentity creation_file_;
    std::weak_ptr<const BnwNativePackageLoadActor> initial_receiver_;
    std::weak_ptr<BnwNativePreDx9DiscoveryOwner> initial_source_;
    std::optional<runtime::FileIdentity> actual_read_identity_;
    const bool initial_constructor_source_=false;
    BnwNativePackageCategory2Pair(std::filesystem::path filename,
        std::shared_ptr<const BnwNativeInstallationInputOwner> installation,std::shared_ptr<const void> epoch,
        const std::shared_ptr<BnwNativePackageLoadActor>& receiver,
        const std::shared_ptr<BnwNativePreDx9DiscoveryOwner>& source)
        :installation_input_(std::move(installation)),epoch_(std::move(epoch)),creation_filename_(std::move(filename)),
         creation_file_{},initial_receiver_(receiver),initial_source_(source),initial_constructor_source_(true){}
    const runtime::FileIdentity& creation_source_identity()const {
        if(initial_constructor_source_){
            if(initial_source_.expired()||initial_receiver_.expired()||!actual_read_identity_)
                throw std::runtime_error("Initial Category2 has no live actual Reader witness");
            return *actual_read_identity_;
        }
        return creation_file_;
    }
    BnwNativePackageCategory2Pair(std::filesystem::path filename,runtime::FileIdentity file,
        std::shared_ptr<const BnwNativeEarlyInputLease> lease,std::shared_ptr<const void> epoch)
        :lease_(std::move(lease)),epoch_(std::move(epoch)),
         creation_filename_(std::move(filename)),creation_file_(std::move(file)){}
    BnwNativePackageCategory2Pair(std::filesystem::path filename,runtime::FileIdentity file,
        std::shared_ptr<const BnwNativeInstallationInputOwner> installation,std::shared_ptr<const void> epoch)
        :installation_input_(std::move(installation)),epoch_(std::move(epoch)),
         creation_filename_(std::move(filename)),creation_file_(std::move(file)){}
    // Every future genuine category2 writer must revoke before any partial
    // effect; this operation cannot turn an unknown write into known absence.
    void revoke_unknown_writer()noexcept {constructor_tracked_=false;retired_=true;}
};
class BnwNativePackageCategory2ConstructorRead final {
  public:
    ~BnwNativePackageCategory2ConstructorRead()=default;
    BnwNativePackageCategory2ConstructorRead(const BnwNativePackageCategory2ConstructorRead&)=delete;
  private:
    friend class ::civ5::ui::BnwNativePackageDiscoveryOwner;
    std::weak_ptr<const BnwNativePackageCategory2Pair> pair_;
    std::weak_ptr<const ::civ5::ui::BnwNativePackageDiscoveryOwner> parent_;
    std::shared_ptr<const BnwNativeEarlyInputLease> lease_;
    std::shared_ptr<const BnwNativeUiSkinRetainedOccurrence> occurrence_;
    std::shared_ptr<const void> epoch_,channel_;
    const content::XmlNode* document_=nullptr;
    const content::XmlNode* node_=nullptr;
    std::size_t descriptor_=0,ordinal_=0,channel_index_=0;
    std::uint64_t pair_revision_=0;
    std::uint32_t flags_mask_=0,flags_value_=0,category_mask_=0;
    std::array<std::uint32_t,2> observed_words_{};
    BnwNativePackageCategory2ConstructorRead()=default;
};
// Private Source-only route; a genuine completed Package is the only input.
// No pair bits/descriptor path/flags/absence DTO can issue a read. This checks
// the constructor read phase only, never native selected invocation/readiness.
class BnwNativePackageCategory2PairOperation final {
  public:
    static void require_owned_constructor_reads(
        const std::shared_ptr<const ::civ5::ui::BnwNativePackageDiscoveryOwner>&);
  private:
    BnwNativePackageCategory2PairOperation()=delete;
};
static_assert(!std::is_default_constructible_v<BnwNativePackageCategory2Pair>);
static_assert(!std::is_copy_constructible_v<BnwNativePackageCategory2Pair>);
static_assert(!std::is_default_constructible_v<BnwNativePackageCategory2ConstructorRead>);
static_assert(!std::is_copy_constructible_v<BnwNativePackageCategory2ConstructorRead>);
}
}
