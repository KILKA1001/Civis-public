#pragma once
#include "civ5/ui/bnw_audio_sound_data_owner.hpp"
#include "civ5/ui/ui_audio_assets.hpp"
#include <memory>
#ifdef _WIN32
namespace civ5::ui {
struct BnwAudioNativePcmDeviceSource {
 std::string basename,sha256;
 std::uint64_t payload_bytes=0;
 UiPcmFormat pcm;
};
// Genuine SelectedRead singleton PCM source plus an actual OS format/device
// admission. This does not publish original8611f0 cache/load counters, native
// manager prefix, AudioRNG history, voice admission or audible playback.
class BnwAudioNativePcmDeviceOwner final {
 public:
 static std::shared_ptr<const BnwAudioNativePcmDeviceOwner> from_actual_activation(
  std::shared_ptr<const BnwAudioCompletedResourceOwner>,const BnwAudioWindowActivationLoan&,
  const graphics::BnwOwnedCameraRendererWindowLease::Loan&);
 ~BnwAudioNativePcmDeviceOwner();
 BnwAudioNativePcmDeviceOwner(const BnwAudioNativePcmDeviceOwner&)=delete;
 BnwAudioNativePcmDeviceOwner& operator=(const BnwAudioNativePcmDeviceOwner&)=delete;
 void require_current()const;
 BnwAudioNativePcmDeviceSource source_format()const;
 // Actual close result is checked; admission alone is not a release receipt.
 void close()const;
 private:
 struct Impl;std::shared_ptr<Impl> impl_;
 explicit BnwAudioNativePcmDeviceOwner(std::shared_ptr<Impl>);
};
}
#endif
