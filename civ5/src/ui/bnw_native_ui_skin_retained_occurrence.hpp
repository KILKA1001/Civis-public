#pragma once
#include "civ5/ui/bnw_native_package_discovery_owner.hpp"
#include <stdexcept>
#include <type_traits>
#include <utility>

namespace civ5::ui::detail {
class BnwNativeUiSkinSelectedSourceOwner;
// Authentic retained descriptor input only. This is not the798CE0 native
// constructor, a selected namespace, slot availability, enabled state or lease
// for reading a resource. No public issuer accepts a node, ordinal or flags.
class BnwNativeUiSkinRetainedOccurrence final {
  public:
    ~BnwNativeUiSkinRetainedOccurrence() = default;
    BnwNativeUiSkinRetainedOccurrence(const BnwNativeUiSkinRetainedOccurrence&) = delete;
    BnwNativeUiSkinRetainedOccurrence& operator=(const BnwNativeUiSkinRetainedOccurrence&) = delete;
  private:
    friend class ::civ5::ui::BnwNativePackageDiscoveryOwner;
    friend class BnwNativeUiSkinSelectedSourceOwner;
    std::weak_ptr<const ::civ5::ui::BnwNativePackageDiscoveryOwner> parent_;
    std::shared_ptr<const ::civ5::ui::BnwNativeEarlyInputLease> lease_;
    std::shared_ptr<const ::civ5::ui::BnwNativeInstallationInputOwner> installation_input_;
    std::shared_ptr<const void> package_epoch_, channel_;
    std::size_t descriptor_, occurrence_, channel_index_;
    const content::XmlNode* node_;
    BnwNativeUiSkinRetainedOccurrence(
        std::weak_ptr<const ::civ5::ui::BnwNativePackageDiscoveryOwner> parent,
        std::shared_ptr<const ::civ5::ui::BnwNativeEarlyInputLease> lease,
        std::shared_ptr<const void> epoch, std::shared_ptr<const void> channel,
        std::size_t descriptor, std::size_t occurrence, std::size_t channel_index,
        const content::XmlNode& node)
        : parent_(std::move(parent)), lease_(std::move(lease)),
          package_epoch_(std::move(epoch)), channel_(std::move(channel)),
          descriptor_(descriptor), occurrence_(occurrence), channel_index_(channel_index), node_(&node) {}
    BnwNativeUiSkinRetainedOccurrence(
        std::weak_ptr<const ::civ5::ui::BnwNativePackageDiscoveryOwner> parent,
        std::shared_ptr<const ::civ5::ui::BnwNativeInstallationInputOwner> installation,
        std::shared_ptr<const void> epoch,std::shared_ptr<const void> channel,
        std::size_t descriptor,std::size_t occurrence,std::size_t channel_index,
        const content::XmlNode& node)
        :parent_(std::move(parent)),installation_input_(std::move(installation)),
         package_epoch_(std::move(epoch)),channel_(std::move(channel)),
         descriptor_(descriptor),occurrence_(occurrence),channel_index_(channel_index),node_(&node){}
    // Borrowed only inside the future private consumer's own operation fence.
    // That consumer must retain this SAME token and parent until input use ends.
    const content::XmlNode& borrowed_node() const {
        const auto parent = parent_.lock();
        if (!parent) throw std::runtime_error("Native UISkin retained Package retired");
        parent->require_ui_skin_retained_occurrence_current(*this);
        return *node_;
    }
};
static_assert(!std::is_default_constructible_v<BnwNativeUiSkinRetainedOccurrence>);
static_assert(!std::is_copy_constructible_v<BnwNativeUiSkinRetainedOccurrence>);
}
