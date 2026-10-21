#pragma once
#include "civ5/ui/bnw_audio_user_settings_input.hpp"
#include "civ5/ui/bnw_audio_window_activation_loan.hpp"
#include <cstdint>
#include <memory>
#include <string>
#include <string_view>

#ifdef _WIN32
namespace civ5::ui {
struct BnwAudioLiveKnobFields {
    std::string name;
    std::uint32_t current_bits,target_bits,slope_bits,cached_effective_bits;
    std::int32_t transition_mode,parent_index;
    bool inherited_flag;
    std::uint64_t observed_window_sequence;
};
struct BnwAudioScriptCategoryFactor {
    std::string script_id,category;
    float category_factor;
    std::uint64_t observed_window_sequence;
};
// Concrete retained numeric owner. Constructor is private; no caller XML,
// percentage, window bool, callback acknowledgement or mutable ready flag.
// All three facade control blocks are private-factory authenticated and
// retained before source callbacks. Raw-pointer equality grants no ownership.
// First actual loan must be sequence1. Standalone Civis initializes its own
// cached-minimized0 state; no Original pre-attachment history is inferred.
class BnwAudioLiveKnobOwner final {
  public:
    static std::shared_ptr<BnwAudioLiveKnobOwner> from_first_actual_activation(
        std::shared_ptr<const BnwNativePackageDiscoveryOwner>,
        std::shared_ptr<const BnwAudioSourceCatalog>,
        std::shared_ptr<const BnwAudioUserSettingsInput>,
        std::shared_ptr<const BnwAudioWindowActivationLoan>);
    static std::shared_ptr<BnwAudioLiveKnobOwner> from_completed_resources(
        std::shared_ptr<const BnwAudioCompletedResourceOwner>,
        std::shared_ptr<const BnwAudioSourceCatalog>,
        std::shared_ptr<const BnwAudioUserSettingsInput>,
        std::shared_ptr<const BnwAudioWindowActivationLoan>);
    ~BnwAudioLiveKnobOwner();
    BnwAudioLiveKnobOwner(const BnwAudioLiveKnobOwner&)=delete;
    BnwAudioLiveKnobOwner& operator=(const BnwAudioLiveKnobOwner&)=delete;
    void consume_actual_activation(std::shared_ptr<const BnwAudioWindowActivationLoan>);
    void require_current()const;
    BnwAudioLiveKnobFields knob(std::string_view)const;
    // Actual retained Script2D row -> SoundType -> recursive category factor.
    // This is not voice64/58 gain, device playback or complete audio startup.
    BnwAudioScriptCategoryFactor script_category_factor(std::string_view)const;
  private:
    struct Impl;
    std::shared_ptr<Impl> impl_;
    explicit BnwAudioLiveKnobOwner(std::shared_ptr<Impl>);
};
}
#endif
