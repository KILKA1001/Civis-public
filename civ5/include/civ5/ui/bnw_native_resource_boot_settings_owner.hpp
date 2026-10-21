#pragma once
#include "civ5/ui/bnw_native_package_discovery_owner.hpp"
#include <optional>

namespace civ5::runtime {class BnwSelectedLanguageInputOwner;class BnwSelectedUserOptionsOwner;class BnwNetMessageDebugConfigOwner;class BnwNetMessageDebugManagerOwner;}
namespace civ5::ui::detail {class BnwNativeGlobalResourceWarmOwner;class BnwNativePreDx9DiscoveryOwner;}
namespace civ5::ui {
class BnwNativeUserDirectoryOwner;
namespace detail {class BnwNativeDefaultLooseQueryFolder;}
struct BnwNativeResourceBootSettingsFrame {
    std::filesystem::path graphics_ini;
    bool ini_loaded = false, normal_launch_source_closed = false;
    bool leader_viewer = false;
    std::optional<std::uint32_t> raw_texture_quality_override;
    // Only a present override closes this slot independently of adapter presets.
    std::optional<std::uint32_t> effective_texture_quality;
    bool material_adapter_ready = false;
    bool adapter_caps_source_closed = false;
    std::optional<std::uint32_t> adapter_preset;
    std::uint32_t actual_adapter_vendor = 0, actual_adapter_device = 0;
};
// Pinned 5C8F60 path producer and post-preset integer registration, bound to
// the actual current process and SAME early resource/client epoch. No path,
// caller bool, fixture preset or raw DWORD input can manufacture this owner.
class BnwNativeResourceBootSettingsOwner {
  public:
    static std::shared_ptr<const BnwNativeResourceBootSettingsOwner> create(
        std::shared_ptr<const BnwNativePackageDiscoveryOwner>);
    ~BnwNativeResourceBootSettingsOwner();
    //Real product preflight before texture/Common/descriptor/rules stages.
    std::shared_ptr<const runtime::BnwSelectedLanguageInputOwner> selected_language_input()const;
    std::shared_ptr<const runtime::BnwSelectedUserOptionsOwner> selected_user_options()const;
    std::shared_ptr<const runtime::BnwNetMessageDebugConfigOwner> selected_net_debug_config()const;
    std::shared_ptr<const runtime::BnwNetMessageDebugManagerOwner> selected_net_debug_manager()const;
    const BnwNativeResourceBootSettingsFrame& current() const;
    void require_same_resources(const BnwNativePackageDiscoveryOwner&) const;
    // Optional strict read-only source predecessor even when INI overrides D80.
    // Actual API only, once and before texture publication; never creates device.
    void capture_native_dx9_adapter_source() const;
  private:
    friend class BnwNativePackageDiscoveryOwner;
    friend class detail::BnwNativePreDx9DiscoveryOwner;
    static std::shared_ptr<const BnwNativeResourceBootSettingsOwner> create_from_installation_source(const BnwNativePackageDiscoveryOwner&);
    static std::shared_ptr<const BnwNativeResourceBootSettingsOwner> create_from_installation_source(
        const BnwNativePackageDiscoveryOwner&,const BnwNativePackageDiscoveryOwner::InstalledBootConstructionOrigin&);
    void require_installation_parent_without_callbacks(const BnwNativePackageDiscoveryOwner&,
        const std::weak_ptr<const BnwNativePackageDiscoveryOwner>&,const assets::ResourceMountTable&)const;
    void require_installation_directory_without_callbacks(const BnwNativeUserDirectoryOwner&,
        const std::weak_ptr<const BnwNativePackageDiscoveryOwner>&,const assets::ResourceMountTable&)const;
    std::shared_ptr<const BnwNativeResourceBootSettingsOwner> retain_factory_owner_without_callbacks(
        const BnwNativePackageDiscoveryOwner&,
        const std::weak_ptr<const BnwNativePackageDiscoveryOwner>&,
        const assets::ResourceMountTable&)const;
    std::shared_ptr<const runtime::BnwSelectedUserOptionsOwner> borrow_retained_user_options_for_completed_source()const;
    friend class detail::BnwNativeGlobalResourceWarmOwner;
    friend class ::civ5::runtime::BnwSelectedLanguageInputOwner;
    friend class ::civ5::runtime::BnwSelectedUserOptionsOwner;
    friend class ::civ5::runtime::BnwNetMessageDebugConfigOwner;
    friend class ::civ5::runtime::BnwNetMessageDebugManagerOwner;
    std::filesystem::path selected_language_profile_path_without_parent_callbacks()const;
    std::filesystem::path selected_language_game_root_without_parent_callbacks()const;

    friend class detail::BnwNativeDefaultLooseQueryFolder;
    unsigned normal_default_loose_mode(const BnwNativePackageDiscoveryOwner&)const;
    const BnwNativeResourceBootSettingsFrame& common_operation_frame(const BnwNativePackageDiscoveryOwner&)const;
    void validate_source() const;
    struct Impl;
    std::shared_ptr<Impl> impl_;
    explicit BnwNativeResourceBootSettingsOwner(std::shared_ptr<Impl>);
};
}
