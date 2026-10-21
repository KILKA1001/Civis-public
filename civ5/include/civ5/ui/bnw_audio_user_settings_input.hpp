#pragma once

#include "civ5/ui/bnw_audio_source_catalog.hpp"
#include "civ5/ui/bnw_native_user_directory_owner.hpp"
#include <array>

namespace civ5::ui {
struct BnwAudioUserVolumeInput {
    std::string preference_key;
    std::string knob_name;
    std::int32_t saved_percent;
    float numeric_value;
};

// Actual selected existing UserSettings source, not a live audio startup grant.
// Holds the same early packages, user directory and Source174 resource catalog.
// Missing settings requiring native default insertion/save remain unavailable.
class BnwAudioUserSettingsInput final {
  public:
    static std::shared_ptr<const BnwAudioUserSettingsInput> from_existing_native_directory(
        std::shared_ptr<const BnwNativePackageDiscoveryOwner>,
        std::shared_ptr<const BnwNativeUserDirectoryOwner>,
        std::shared_ptr<const BnwAudioSourceCatalog>);
    static std::shared_ptr<const BnwAudioUserSettingsInput> from_completed_resources(
        std::shared_ptr<const BnwAudioCompletedResourceOwner>,
        std::shared_ptr<const BnwAudioSourceCatalog>);
    void require_current() const;
    void require_same_catalog(const BnwAudioSourceCatalog&) const;
    std::array<BnwAudioUserVolumeInput, 4> inputs() const;
  private:
    friend class BnwAudioLiveKnobOwner;
    std::weak_ptr<const BnwAudioUserSettingsInput> factory_owner_;
    std::shared_ptr<const BnwAudioUserSettingsInput> retain_factory_owner(
        const std::shared_ptr<const BnwAudioUserSettingsInput>&) const;
    struct Impl;
    std::shared_ptr<Impl> impl_;
    explicit BnwAudioUserSettingsInput(std::shared_ptr<Impl>);
};
} // namespace civ5::ui
