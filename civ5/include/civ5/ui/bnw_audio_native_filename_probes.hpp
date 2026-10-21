#pragma once
#include "civ5/ui/bnw_audio_source_catalog.hpp"
#include "civ5/graphics/bnw_owned_camera_renderer_window_lease.hpp"
#include <cstdint>
#include <memory>
#include <string>
#include <vector>
#ifdef _WIN32
namespace civ5::ui {
struct BnwAudioInspectedFilename {
 std::string basename;
 BnwAudioNativeBasenameResult result;
};
struct BnwAudioInspectedVariationCount {
 std::string queried_stem,selected_extension;
 std::int32_t authored_variation_cap=0;
 std::uint32_t count=0;
 std::uint64_t actual_activation_sequence=0;
 std::vector<BnwAudioInspectedFilename> inspected;
};
// Inspected resource availability component only. Caller stem remains a query,
// NOT issued SoundData/manager+A34 prefix, native cache or voice/RNG authority.
class BnwAudioNativeFilenameProbes final {
 public:
 static std::shared_ptr<const BnwAudioNativeFilenameProbes> from_actual_activation(
  std::shared_ptr<const BnwAudioCompletedResourceOwner>,const BnwAudioWindowActivationLoan&,
  const graphics::BnwOwnedCameraRendererWindowLease::Loan&);
 ~BnwAudioNativeFilenameProbes();
 BnwAudioNativeFilenameProbes(const BnwAudioNativeFilenameProbes&)=delete;
 BnwAudioNativeFilenameProbes&operator=(const BnwAudioNativeFilenameProbes&)=delete;
 void require_current()const;
 BnwAudioInspectedVariationCount inspect_uncached_variation_count(std::string_view)const;
 private:
 struct Impl;std::shared_ptr<Impl> impl_;
 explicit BnwAudioNativeFilenameProbes(std::shared_ptr<Impl>);
};
}
#endif
