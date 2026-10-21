#pragma once
#include "civ5/ui/bnw_audio_source_catalog.hpp"
#include "civ5/graphics/bnw_owned_camera_renderer_window_lease.hpp"
#include <array>
#include <cstdint>
#include <memory>
#include <string>
#ifdef _WIN32
namespace civ5::ui {
struct BnwAudioNativeCategoryFactor {
 std::string category;
 std::uint32_t factor_bits=0,master_current_bits=0;
 std::array<std::int32_t,4> saved_percentages{};
 std::uint64_t activation_sequence=0;
};
// Fresh Civis settings/Master state, NOT imported original audio startup history.
// No voice gain, RNG/event receipt, backend admission, device or playback grant.
class BnwAudioNativeSettingsOwner final {
 public:
 static std::shared_ptr<BnwAudioNativeSettingsOwner> from_first_actual_activation(
  std::shared_ptr<const BnwAudioCompletedResourceOwner>,
  const BnwAudioWindowActivationLoan&,
  const graphics::BnwOwnedCameraRendererWindowLease::Loan&);
 ~BnwAudioNativeSettingsOwner();
 BnwAudioNativeSettingsOwner(const BnwAudioNativeSettingsOwner&)=delete;
 BnwAudioNativeSettingsOwner& operator=(const BnwAudioNativeSettingsOwner&)=delete;
 void consume_actual_activation(const BnwAudioWindowActivationLoan&);
 void require_current()const;
 BnwAudioNativeCategoryFactor category_factor(std::string_view)const;
 private:
 struct Impl;std::shared_ptr<Impl> impl_;
 explicit BnwAudioNativeSettingsOwner(std::shared_ptr<Impl>);
};
}
#endif
